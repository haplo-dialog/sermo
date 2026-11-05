/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * sermo-cible-media — programme d'aide de <action media> : lance un
 * lecteur son/vidéo (mpv) et lui envoie des commandes.
 *
 * Copyright (C) 2026 S. Cage
 * haplo-dialog <devel@haplo-dialog.fr>
 *
 * Appelé par sermo (libsermocore/include/sermo_source.h), sur un bouton ou une
 * entrée de menu seulement, avec l'adresse encodée en un argument :
 *   media://NOM?commande=lire&source=file:///chemin/son.ogg
 *   media://NOM?commande=lire&source=https://hôte/flux&identifiants=/chemin/fichier
 *   media://NOM?commande=lire&source=v4l2:///dev/video0[&taille=640x480][&format=mjpeg]
 *   media://NOM?commande=lire&source=…&dans=$VARIABLE   (image DANS la fenêtre)
 *   media://NOM?commande=photo&fichier=/chemin/photo.png      (caméra en cours)
 *   media://NOM?commande=enregistrer&fichier=/chemin/video.mkv&duree=SECONDES
 *   media://NOM?commande=couper                              (fin d'enregistrement)
 *   media://NOM?commande=pause | reprendre | arreter | muet | son
 *   media://NOM?commande=aller&valeur=SECONDES      (0 à 86400, ou $VARIABLE)
 *   media://NOM?commande=volume&valeur=0..100       (ou $VARIABLE)
 *
 * SOURCES (mpv n'a aucune liste blanche de protocoles : edl://, av://lavfi… y
 * passent ; c'est ICI qu'on filtre) :
 *   file:///…        chemin absolu, résolu, fichier ordinaire lisible
 *   v4l2:///dev/videoN  une caméra : nœud réel, périphérique caractère, nœud
 *                    d'image (pas de métadonnées) ; l'adresse « av://v4l2:… »
 *                    donnée à mpv est construite ici ; fenêtre titrée
 *                    « CAMÉRA ACTIVE » ; caméra occupée : refus, rien de forcé
 *   http(s)://, rtsp(s)://   hôte (nom ou [IPv6]), port, chemin aux caractères
 *                    sûrs ; jamais d'identifiants dans l'adresse
 *   identifiants=    HTTP(S) seulement : fichier à vous, en 600, une ligne
 *                    « utilisateur:mot de passe » ; passé à mpv par --include
 *                    d'un fichier 600 (en-tête Authorization), JAMAIS en argument.
 *                    RTSP à mot de passe : refusé (mpv ne le lit que dans l'adresse).
 *
 * DANS LA ZONE (Wayland…) : si la variable vaut « zone » (un <drawingarea
 * embed="true" media="NOM"> hors X11), le lecteur est sermo-media-images
 * (libmpv, à côté de ce programme) au lieu de mpv : il rend les images en
 * logiciel dans DOSSIER/images (sermo_images.h), que la zone lit et peint.
 * La cage ne monte alors AUCUN accès à l'écran.
 *
 * DANS LA FENÊTRE : dans=$VARIABLE nomme la variable d'un <drawingarea> ; sermo
 * (gtk3, sous X11) y exporte l'identifiant X11 de la zone, donné à mpv par
 * --wid. Variable absente ou vide (Wayland, autre port, zone pas encore
 * affichée) : REPLI sur une fenêtre séparée, et c'est dit. Caméra incrustée :
 * « CAMÉRA ACTIVE » est écrit dans l'image, le titre n'étant plus visible.
 *
 * PHOTO ET ENREGISTREMENT (caméra seulement) : un AUTRE geste que l'aperçu, que le
 * dialogue fait précéder d'une confirmation montrant le chemin et la durée
 * (exemple camera-capture). Le fichier est le chemin exact donné : absolu,
 * extension fermée (.png, .jpg ; .mkv), dans un dossier à vous que ni le
 * groupe ni les autres ne peuvent écrire ; jamais écrasé (O_EXCL) ; écrit en
 * 600. mpv l'écrit d'abord dans le dossier privé du lecteur (700, en mémoire
 * sous XDG_RUNTIME_DIR), puis il est recopié et l'original effacé.
 * Enregistrement : MJPEG seulement (les paquets sont recopiés tels quels),
 * 1 à 600 s, « ENREGISTREMENT » écrit dans l'image et dans le titre ; il
 * s'arrête à la durée, sur commande=couper, ou quand le lecteur s'arrête.
 *
 * BAC À SABLE : bwrap OBLIGATOIRE pour une source réseau ; pour un fichier
 * local, utilisé s'il est là (sinon c'est dit). Système en lecture seule,
 * dossier personnel en tmpfs, son (PulseAudio) et affichage (X11, Wayland)
 * permis, réseau coupé pour un fichier local.
 *
 * DURÉE DE VIE : un gardien (processus à part, sans rien d'hérité de sermo)
 * tue mpv quand le processus sermo qui a lancé l'action disparaît (retrouvé
 * en remontant les parents ; SERMO_MEDIA_MAITRE=PID pour les essais).
 *
 * Code de sortie : 0 fait ; 1 refus ou échec (la raison sur la sortie d'erreur).
 */
#define _GNU_SOURCE
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <locale.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

#include "commun.h"
#include "media-commun.h"

#define PROGRAMME "sermo-cible-media"
#define ARGS_MAX 96   /* 71 écritures possibles au pire ; garde avant le lancement */

static void
dormir_ms(long ms)
{
	struct timespec t = { ms / 1000, (ms % 1000) * 1000000L };
	nanosleep(&t, NULL);
}

/* Un exécutable trouvé dans le PATH, en chemin absolu. */
static int
chercher(const char *nom, char *chemin, size_t taille)
{
	const char *path = getenv("PATH");
	if (!path)
		return -1;
	char *copie = strdup(path), *svg = NULL;
	if (!copie)
		return -1;
	for (char *d = strtok_r(copie, ":", &svg); d; d = strtok_r(NULL, ":", &svg)) {
		if (d[0] != '/')
			continue;
		snprintf(chemin, taille, "%s/%s", d, nom);
		if (access(chemin, X_OK) == 0) {
			free(copie);
			return 0;
		}
	}
	free(copie);
	return -1;
}

/* ── Sources ─────────────────────────────────────────────────────────────── */

static int
caractere_chemin_sur(char c)
{
	return (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9')
	       || strchr("-._~/%?=&+,;:", c) != NULL;
}

/* Caméra : « /dev/videoN » réel, périphérique caractère, et nœud d'IMAGE (index 0
 * dans /sys/class/video4linux ; les nœuds de métadonnées refusent la capture). */
static int
camera_valider(const char *noeud, char *reel, size_t taille)
{
	char r[PATH_MAX], sysfs[PATH_MAX], idx[16] = "";
	struct stat st;
	int num = -1, lu = 0;
	if (sscanf(noeud, "/dev/video%3d%n", &num, &lu) != 1 || noeud[lu] != '\0' || num < 0
	    || !realpath(noeud, r) || sscanf(r, "/dev/video%3d%n", &num, &lu) != 1 || r[lu] != '\0'
	    || stat(r, &st) != 0 || !S_ISCHR(st.st_mode)) {
		fprintf(stderr, PROGRAMME ": caméra refusée : %s n'est pas un nœud /dev/videoN existant\n", noeud);
		return -1;
	}
	const char *racine = getenv("SERMO_MEDIA_V4L_SYSFS");   /* essais seulement */
	snprintf(sysfs, sizeof sysfs, "%s/video%d/index", (racine && racine[0] == '/') ? racine : "/sys/class/video4linux", num);
	FILE *f = fopen(sysfs, "r");
	if (f) { if (!fgets(idx, sizeof idx, f)) idx[0] = 0; fclose(f); }
	if (idx[0] != '0' || (idx[1] != '\n' && idx[1] != '\0')) {
		fprintf(stderr, PROGRAMME ": caméra refusée : %s n'est pas un nœud d'image (métadonnées ?)\n", r);
		return -1;
	}
	if (strlen(r) >= taille)
		return -1;
	snprintf(reel, taille, "%s", r);
	return 2;
}

/* 0 local (chemin réel dans @reel), 1 réseau, 2 caméra (nœud réel dans @reel), -1 refus. */
static int
source_valider(const char *s, char *reel, size_t taille)
{
	if (!s || !*s) {
		fputs(PROGRAMME ": source manquante (source=file:///chemin, ou http(s)://, rtsp(s)://)\n", stderr);
		return -1;
	}
	if (strlen(s) > 2048) {
		fputs(PROGRAMME ": source trop longue (2048 octets au plus)\n", stderr);
		return -1;
	}
	if (strncmp(s, "v4l2://", 7) == 0)
		return camera_valider(s + 7, reel, taille);
	if (strncmp(s, "file://", 7) == 0) {
		const char *p = s + 7;
		char r[PATH_MAX];
		struct stat st;
		if (p[0] != '/' || !realpath(p, r) || stat(r, &st) != 0 || !S_ISREG(st.st_mode) || access(r, R_OK) != 0) {
			fprintf(stderr, PROGRAMME ": source locale refusée : %s n'est pas un fichier ordinaire lisible\n", p);
			return -1;
		}
		if (strlen(r) >= taille)
			return -1;
		snprintf(reel, taille, "%s", r);
		return 0;
	}
	static const char *const schemas[] = { "https://", "http://", "rtsps://", "rtsp://", NULL };
	const char *h = NULL;
	for (int i = 0; schemas[i]; i++)
		if (strncmp(s, schemas[i], strlen(schemas[i])) == 0) {
			h = s + strlen(schemas[i]);
			break;
		}
	if (!h) {
		fputs(PROGRAMME ": schéma refusé : file, v4l2, http, https, rtsp ou rtsps seulement "
		      "(edl://, av://, concat:… sont refusés)\n", stderr);
		return -1;
	}
	/* Hôte : [IPv6] ou nom/IPv4, jamais commençant par « - » ; pas d'« @ ». */
	const char *q = h;
	if (*q == '[') {
		q++;
		while (*q && (strchr("0123456789abcdefABCDEF:", *q)))
			q++;
		if (*q != ']' || q == h + 1) {
			fputs(PROGRAMME ": hôte IPv6 mal formé\n", stderr);
			return -1;
		}
		q++;
	} else {
		if (*q == '-' || *q == '.' || *q == '\0') {
			fputs(PROGRAMME ": hôte refusé\n", stderr);
			return -1;
		}
		while (*q && ((*q >= 'A' && *q <= 'Z') || (*q >= 'a' && *q <= 'z') || (*q >= '0' && *q <= '9') || *q == '.' || *q == '-'))
			q++;
	}
	if (*q == ':') {
		q++;
		const char *d = q;
		while (*q >= '0' && *q <= '9')
			q++;
		long port = (q > d && q - d <= 5) ? strtol(d, NULL, 10) : 0;
		if (port < 1 || port > 65535) {
			fputs(PROGRAMME ": port refusé\n", stderr);
			return -1;
		}
	}
	if (*q != '\0' && *q != '/') {
		fputs(PROGRAMME ": adresse refusée (identifiants « …@ » ou caractère inattendu après l'hôte)\n", stderr);
		return -1;
	}
	for (; *q; q++)
		if (!caractere_chemin_sur(*q)) {
			fputs(PROGRAMME ": chemin de l'adresse refusé (espace, guillemet, « @ », caractère de contrôle…)\n", stderr);
			return -1;
		}
	return 1;
}

static const char b64[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

/* Fichier 600 « utilisateur:mot de passe » → <dossier>/auth.conf (600), en-tête Authorization. */
/* Lit « utilisateur:mot de passe » (fichier à vous, en 600) dans ligne. */
static int
identifiants_lire(const char *fichier, char *ligne, size_t taille)
{
	struct stat st;
	if (fichier[0] != '/' || lstat(fichier, &st) != 0 || !S_ISREG(st.st_mode)
	    || st.st_uid != geteuid() || (st.st_mode & 0777) != 0600) {
		fputs(PROGRAMME ": fichier d'identifiants refusé : chemin absolu d'un fichier ordinaire, à vous, en mode 600\n", stderr);
		return -1;
	}
	FILE *f = fopen(fichier, "r");
	if (!f || !fgets(ligne, (int)taille, f)) {
		if (f)
			fclose(f);
		fputs(PROGRAMME ": fichier d'identifiants illisible ou vide\n", stderr);
		return -1;
	}
	fclose(f);
	ligne[strcspn(ligne, "\r\n")] = '\0';
	size_t n = strlen(ligne);
	if (n == 0 || n > 256 || !strchr(ligne, ':')) {
		fputs(PROGRAMME ": identifiants : une ligne « utilisateur:mot de passe » (256 caractères au plus)\n", stderr);
		return -1;
	}
	for (size_t i = 0; i < n; i++)
		if ((unsigned char)ligne[i] < 0x20 || ligne[i] == 0x7f) {
			fputs(PROGRAMME ": identifiants : caractère de contrôle refusé\n", stderr);
			return -1;
		}
	return 0;
}

static int
identifiants_preparer(const char *fichier, const char *dossier, char *inclure, size_t taille)
{
	char ligne[300];
	if (identifiants_lire(fichier, ligne, sizeof ligne) != 0)
		return -1;
	size_t n = strlen(ligne);
	char code[400];
	size_t j = 0;
	for (size_t i = 0; i < n; i += 3) {
		unsigned v = (unsigned char)ligne[i] << 16;
		if (i + 1 < n) v |= (unsigned char)ligne[i + 1] << 8;
		if (i + 2 < n) v |= (unsigned char)ligne[i + 2];
		code[j++] = b64[(v >> 18) & 63];
		code[j++] = b64[(v >> 12) & 63];
		code[j++] = i + 1 < n ? b64[(v >> 6) & 63] : '=';
		code[j++] = i + 2 < n ? b64[v & 63] : '=';
	}
	code[j] = '\0';
	explicit_bzero(ligne, sizeof ligne);
	snprintf(inclure, taille, "%s/auth.conf", dossier);
	unlink(inclure);
	int fd = open(inclure, O_WRONLY | O_CREAT | O_EXCL | O_CLOEXEC | O_NOFOLLOW, 0600);
	if (fd < 0) {
		fputs(PROGRAMME ": impossible d'écrire le fichier d'en-tête privé\n", stderr);
		return -1;
	}
	dprintf(fd, "http-header-fields=\"Authorization: Basic %s\"\n", code);
	close(fd);
	explicit_bzero(code, sizeof code);
	return 0;
}

/* ── Le processus sermo à surveiller ─────────────────────────────────────── */

/* RTSP à mot de passe : l'adresse « rtsp://u:p@hôte/… » n'existe qu'en mémoire,
 * et part vers mpv par son socket privé (loadfile), JAMAIS en argument.
 * Utilisateur et mot de passe encodés « pour cent » (seuls A-Z a-z 0-9 - . _ ~
 * restent tels quels) : rien ne peut sortir de la chaîne JSON. */
static int
rtsp_avec_identifiants(const char *source, const char *fichier, char *url, size_t taille)
{
	char ligne[300];
	if (identifiants_lire(fichier, ligne, sizeof ligne) != 0)
		return -1;
	const char *reste = strstr(source, "://");
	for (const char *c = source; *c; c++)
		if (*c == '"' || *c == '\\' || (unsigned char)*c < 0x20) {
			explicit_bzero(ligne, sizeof ligne);
			return -1;
		}
	size_t j = 0;
	int n = snprintf(url, taille, "%.*s://", (int)(reste - source), source);
	j = (size_t)n;
	static const char hex[] = "0123456789ABCDEF";
	for (const char *c = ligne; *c && j + 4 < taille; c++) {
		unsigned char o = (unsigned char)*c;
		if (c == strchr(ligne, ':'))
			url[j++] = ':';
		else if ((o >= 'A' && o <= 'Z') || (o >= 'a' && o <= 'z') || (o >= '0' && o <= '9')
		         || o == '-' || o == '.' || o == '_' || o == '~')
			url[j++] = (char)o;
		else {
			url[j++] = '%'; url[j++] = hex[o >> 4]; url[j++] = hex[o & 15];
		}
	}
	explicit_bzero(ligne, sizeof ligne);
	n = snprintf(url + j, taille - j, "@%s", reste + 3);
	if (n < 0 || (size_t)n >= taille - j) {
		explicit_bzero(url, taille);
		return -1;
	}
	return 0;
}

static pid_t
parent_de(pid_t pid)
{
	char chemin[64], tampon[512];
	snprintf(chemin, sizeof chemin, "/proc/%d/stat", (int)pid);
	FILE *f = fopen(chemin, "r");
	if (!f)
		return 0;
	size_t n = fread(tampon, 1, sizeof tampon - 1, f);
	fclose(f);
	tampon[n] = '\0';
	char *p = strrchr(tampon, ')');
	int ppid = 0;
	if (!p || sscanf(p + 1, " %*c %d", &ppid) != 1)
		return 0;
	return (pid_t)ppid;
}

static int
est_sermo(pid_t pid)
{
	char chemin[64], nom[64] = "";
	snprintf(chemin, sizeof chemin, "/proc/%d/comm", (int)pid);
	FILE *f = fopen(chemin, "r");
	if (!f)
		return 0;
	if (!fgets(nom, sizeof nom, f))
		nom[0] = '\0';
	fclose(f);
	nom[strcspn(nom, "\n")] = '\0';
	size_t l = strlen(nom);
	return l >= 5 && strcmp(nom + l - 5, "sermo") == 0;
}

static pid_t
maitre_trouver(void)
{
	const char *m = getenv("SERMO_MEDIA_MAITRE");
	if (m && *m) {
		char *fin;
		long v = strtol(m, &fin, 10);
		if (*fin == '\0' && v > 1 && kill((pid_t)v, 0) == 0)
			return (pid_t)v;
	}
	pid_t p = getppid();
	for (int i = 0; i < 6 && p > 1; i++) {
		if (est_sermo(p))
			return p;
		p = parent_de(p);
	}
	return getppid();
}

/* ── Lancement ───────────────────────────────────────────────────────────── */

static void
ecrire_fichier(const char *dossier, const char *nom, const char *texte)
{
	char chemin[600], tmp[620];
	snprintf(chemin, sizeof chemin, "%s/%s", dossier, nom);
	snprintf(tmp, sizeof tmp, "%s.n", chemin);
	int fd = open(tmp, O_WRONLY | O_CREAT | O_TRUNC | O_CLOEXEC | O_NOFOLLOW, 0600);
	if (fd < 0)
		return;
	if (write(fd, texte, strlen(texte)) < 0) { /* rien à faire : le lecteur verra l'absence */ }
	close(fd);
	rename(tmp, chemin);
}

static int
lecteurs_actifs(const char *nom_exclu)
{
	char racine[512], chemin[800];
	if (media_racine(PROGRAMME, racine, sizeof racine, 0) != 0)
		return 0;
	int n = 0;
	DIR *d = opendir(racine);
	if (!d)
		return 0;
	struct dirent *e;
	while ((e = readdir(d))) {
		if (e->d_name[0] == '.' || strcmp(e->d_name, nom_exclu) == 0)
			continue;
		snprintf(chemin, sizeof chemin, "%s/%s", racine, e->d_name);
		if (media_pid_vivant(chemin, "gardien"))
			n++;
	}
	closedir(d);
	return n;
}

static void
arreter_existant(const char *dossier)
{
	char rep[512];
	pid_t g = media_pid_vivant(dossier, "gardien");
	if (!g)
		return;
	ecrire_fichier(dossier, "arret-demande", "1\n");
	media_ipc(dossier, "{\"command\":[\"quit\"]}", rep, sizeof rep);
	for (int i = 0; i < 30 && kill(g, 0) == 0; i++)
		dormir_ms(100);
	if (kill(g, 0) == 0)
		kill(g, SIGTERM);
	for (int i = 0; i < 30 && kill(g, 0) == 0; i++)
		dormir_ms(100);
}

/* Le gardien : sans rien d'hérité de sermo (sinon sermo attendrait la fin de
 * l'action), il lance mpv, note son PID, puis le surveille. */
static void
gardien(const char *dossier, pid_t maitre, char *const argv[], int effacer_auth)
{
	setsid();
	int nul = open("/dev/null", O_RDWR);
	char err[620];
	snprintf(err, sizeof err, "%s/mpv.err", dossier);
	int e = open(err, O_WRONLY | O_CREAT | O_TRUNC | O_NOFOLLOW, 0600);
	if (nul >= 0) { dup2(nul, 0); dup2(nul, 1); }
	if (e >= 0) dup2(e, 2);
	for (int fd = 3; fd < 1024; fd++)
		close(fd);
	pid_t m = fork();
	if (m < 0)
		_exit(1);
	if (m == 0) {
		execv(argv[0], argv);
		fprintf(stderr, "exec %s : %s\n", argv[0], strerror(errno));
		_exit(127);
	}
	char t[32];
	snprintf(t, sizeof t, "%d\n", (int)m);
	ecrire_fichier(dossier, "mpv", t);
	int tours = 0, statut = 0;
	for (;;) {
		pid_t w = waitpid(m, &statut, WNOHANG);
		if (w == m) {
			char marque[640];
			snprintf(marque, sizeof marque, "%s/arret-demande", dossier);
			/* Arrêt demandé, ou sermo parti (mpv a pu perdre son écran juste
			 * avant que le gardien ne le voie) : c'est un arrêt, pas une erreur. */
			int demande = access(marque, F_OK) == 0 || kill(maitre, 0) != 0;
			unlink(marque);
			ecrire_fichier(dossier, "fin", demande ? "arret\n"
			               : (WIFEXITED(statut) && WEXITSTATUS(statut) == 0) ? "fin\n" : "erreur\n");
			snprintf(marque, sizeof marque, "%s/images", dossier);   /* la zone se vide */
			unlink(marque);
			break;
		}
		if (kill(maitre, 0) != 0) {
			kill(m, SIGTERM);
			for (int i = 0; i < 20 && waitpid(m, &statut, WNOHANG) == 0; i++)
				dormir_ms(100);
			kill(m, SIGKILL);
			waitpid(m, &statut, 0);
			ecrire_fichier(dossier, "fin", "arret\n");
			char images[640];
			snprintf(images, sizeof images, "%s/images", dossier);
			unlink(images);
			break;
		}
		if (effacer_auth && ++tours == 12) {          /* 3 s : mpv a lu --include */
			char a[620];
			snprintf(a, sizeof a, "%s/auth.conf", dossier);
			unlink(a);
		}
		dormir_ms(250);
	}
	char chemin[620];
	snprintf(chemin, sizeof chemin, "%s/auth.conf", dossier);
	unlink(chemin);
	snprintf(chemin, sizeof chemin, "%s/ipc", dossier);
	unlink(chemin);
	_exit(0);
}

/* dans=$NOM : l'identifiant X11 exporté par la zone, ou "" (repli). -1 = refus. */
static int
dans_resoudre(const char *brute, char *wid, size_t taille, int *zone)
{
	wid[0] = '\0';
	*zone = 0;
	size_t i = 1;
	if (brute[0] != '$')
		i = 0;
	for (; brute[i]; i++)
		if (!((brute[i] >= 'A' && brute[i] <= 'Z') || (brute[i] >= 'a' && brute[i] <= 'z') || brute[i] == '_'
		      || (i > 1 && brute[i] >= '0' && brute[i] <= '9')))
			break;
	if (brute[0] != '$' || i == 1 || brute[i] != '\0') {
		fprintf(stderr, PROGRAMME ": dans=%s : attendu $VARIABLE (celle d'un <drawingarea>)\n", brute);
		return -1;
	}
	const char *v = getenv(brute + 1), *x = getenv("DISPLAY");
	if (v && strcmp(v, "zone") == 0) {           /* la zone lit les images elle-même */
		*zone = 1;
		return 0;
	}
	if (!v || !*v || !x || !*x) {
		fprintf(stderr, PROGRAMME ": dans=%s : pas de zone X11 (variable vide, Wayland ou autre port) : "
		        "fenêtre séparée\n", brute);
		return 0;
	}
	/* Un nombre, et rien d'autre : décimal, ou 0x hexadécimal, sur 32 bits. */
	char *fin;
	errno = 0;
	unsigned long long n = strtoull(v, &fin, strncmp(v, "0x", 2) == 0 ? 16 : 10);
	if (errno || fin == v || *fin || v[0] == '-' || v[0] == '+' || n == 0 || n > 0xFFFFFFFFULL) {
		fprintf(stderr, PROGRAMME ": dans=%s : « %.20s » n'est pas un identifiant de fenêtre X11\n", brute, v);
		return -1;
	}
	snprintf(wid, taille, "--wid=%llu", n);
	return 0;
}

static int
lire(const char *nom, const char *dossier, const adresse_t *a)
{
	char reel[PATH_MAX], mpv[PATH_MAX], bwrap[PATH_MAX], inclure[640] = "", ipc[620], titre[128];
	char av[PATH_MAX + 16], lavf[96] = "", journal[700] = "";
	const char *source = adresse_valeur(a, "source");
	const char *ident = adresse_valeur(a, "identifiants");
	int reseau = source_valider(source, reel, sizeof reel);
	if (reseau < 0)
		return 1;
	/* Caméra : taille et format en listes fermées ; ailleurs, ces clés sont refusées. */
	const char *tl = adresse_valeur(a, "taille"), *fm = adresse_valeur(a, "format");
	if (reseau != 2 && (tl || fm)) {
		fputs(PROGRAMME ": taille= et format= : pour une caméra (source=v4l2:///dev/videoN) seulement\n", stderr);
		return 1;
	}
	if (reseau == 2) {
		static const char *const tailles[] = { "320x240", "640x480", "1280x720", "1920x1080", NULL };
		static const char *const formats[] = { "mjpeg", "yuyv422", NULL };
		int ok_t = !tl, ok_f = !fm;
		for (int i = 0; tailles[i] && !ok_t; i++) ok_t = strcmp(tl, tailles[i]) == 0;
		for (int i = 0; formats[i] && !ok_f; i++) ok_f = strcmp(fm, formats[i]) == 0;
		if (!ok_t || !ok_f) {
			fputs(PROGRAMME ": taille (320x240, 640x480, 1280x720, 1920x1080) ou format (mjpeg, yuyv422) refusé\n", stderr);
			return 1;
		}
		/* L'adresse donnée à mpv est construite ICI, depuis le nœud réel : jamais
		 * le texte reçu (av:// est par ailleurs refusé comme source). */
		snprintf(av, sizeof av, "av://v4l2:%s", reel);
		snprintf(lavf, sizeof lavf, "--demuxer-lavf-o=video_size=%s,input_format=%s", tl ? tl : "640x480", fm ? fm : "mjpeg");
		/* --no-terminal coupe la sortie d'erreur de mpv : la cause d'un échec
		 * (« occupée »…) ne se lit que dans ce journal (dossier privé). */
		snprintf(journal, sizeof journal, "--log-file=%s/mpv.log", dossier);
	}
	const char *tr = adresse_valeur(a, "transport");
	char opt_transport[32] = "";
	if (tr) {
		if (strncmp(source, "rtsp", 4) != 0 || (strcmp(tr, "tcp") != 0 && strcmp(tr, "udp") != 0)) {
			fputs(PROGRAMME ": transport= : tcp ou udp, pour une source RTSP seulement\n", stderr);
			return 1;
		}
		snprintf(opt_transport, sizeof opt_transport, "--rtsp-transport=%s", tr);
	}
	int rtsp_auth = 0;
	if (ident && *ident) {
		if (reseau != 1) {
			fputs(PROGRAMME ": identifiants : pour une source réseau seulement (http, https, rtsp, rtsps)\n", stderr);
			return 1;
		}
		rtsp_auth = strncmp(source, "rtsp", 4) == 0;
	}
	char wid[40] = "", osd[160] = "", images[700] = "", opt_images[720] = "";
	int zone = 0;
	const char *dans = adresse_valeur(a, "dans");
	if (dans && dans_resoudre(dans, wid, sizeof wid, &zone) != 0)
		return 1;
	if (zone) {
		snprintf(images, sizeof images, "%s/images", dossier);
		snprintf(opt_images, sizeof opt_images, "--images=%s", images);
	}
	if ((*wid || zone) && reseau == 2)
		snprintf(osd, sizeof osd, "--osd-msg1=CAMÉRA ACTIVE — %s", nom);
	if (strlen(dossier) + sizeof "/ipc" > 108) {   /* sun_path : 108 octets, NUL compris */
		fprintf(stderr, PROGRAMME ": chemin du socket trop long (%s/ipc) : XDG_RUNTIME_DIR trop profond\n", dossier);
		return 1;
	}
	if (zone) {
		/* sermo-media-images est installé à côté de ce programme. */
		char moi[PATH_MAX];
		ssize_t l = readlink("/proc/self/exe", moi, sizeof moi - 1);
		char *barre = l > 0 ? (moi[l] = '\0', strrchr(moi, '/')) : NULL;
		if (!barre || snprintf(mpv, sizeof mpv, "%.*s/sermo-media-images", (int)(barre - moi), moi) >= (int)sizeof mpv
		    || access(mpv, X_OK) != 0) {
			fputs(PROGRAMME ": sermo-media-images absent (construit avec libmpv) : image dans la zone impossible\n", stderr);
			return 1;
		}
	} else if (chercher("mpv", mpv, sizeof mpv) != 0) {
		fputs(PROGRAMME ": mpv est absent (sudo apt install mpv)\n", stderr);
		return 1;
	}
	int avec_bac = chercher("bwrap", bwrap, sizeof bwrap) == 0;
	if (reseau == 1 && !avec_bac) {
		fputs(PROGRAMME ": source réseau refusée sans bac à sable (bwrap absent : sudo apt install bubblewrap)\n", stderr);
		return 1;
	}
	if (lecteurs_actifs(nom) >= MEDIA_LECTEURS_MAX) {
		fprintf(stderr, PROGRAMME ": déjà %d lecteurs en cours (le maximum) : arrêtez-en un\n", MEDIA_LECTEURS_MAX);
		return 1;
	}
	arreter_existant(dossier);
	{   /* la marque « caméra » d'un lancement précédent ne doit pas survivre */
		char marque[620];
		snprintf(marque, sizeof marque, "%s/camera", dossier);
		unlink(marque);
		snprintf(marque, sizeof marque, "%s/images", dossier);   /* l'image d'un lecteur précédent */
		unlink(marque);
	}
	static char url_auth[1200];
	if (rtsp_auth && rtsp_avec_identifiants(source, ident, url_auth, sizeof url_auth) != 0) {
		fputs(PROGRAMME ": identifiants RTSP refusés (fichier, ou adresse trop longue)\n", stderr);
		return 1;
	}
	if (ident && *ident && !rtsp_auth && identifiants_preparer(ident, dossier, inclure, sizeof inclure) != 0)
		return 1;
	char fin[620];
	snprintf(fin, sizeof fin, "%s/fin", dossier);
	unlink(fin);
	snprintf(ipc, sizeof ipc, "--input-ipc-server=%s/ipc", dossier);
	snprintf(titre, sizeof titre, reseau == 2 ? "--title=sermo — CAMÉRA ACTIVE — %s" : "--title=sermo — %s", nom);
	char opt_inclure[700] = "";
	if (*inclure)
		snprintf(opt_inclure, sizeof opt_inclure, "--include=%s", inclure);

	const char *argv[ARGS_MAX];
	int n = 0;
	const char *home = getenv("HOME"), *run = getenv("XDG_RUNTIME_DIR"), *xa = getenv("XAUTHORITY");
	const char *wl = getenv("WAYLAND_DISPLAY");
	char pulse[600] = "", wayland[600] = "";
	if (avec_bac) {
		argv[n++] = bwrap;
		argv[n++] = "--ro-bind"; argv[n++] = "/"; argv[n++] = "/";
		argv[n++] = "--dev"; argv[n++] = "/dev";
		argv[n++] = "--proc"; argv[n++] = "/proc";
		argv[n++] = "--tmpfs"; argv[n++] = "/tmp";
		if (home && home[0] == '/' && access(home, F_OK) == 0) { argv[n++] = "--tmpfs"; argv[n++] = home; }
		/* Les sockets de session (bus D-Bus, agents…) : un --ro-bind ne les rend
		 * PAS injoignables (mesuré : connect réussit). On masque tout /run/user
		 * et le dossier de session ; seuls le dossier du lecteur, le son et
		 * l'affichage sont remontés ensuite. */
		if (access("/run/user", F_OK) == 0) { argv[n++] = "--tmpfs"; argv[n++] = "/run/user"; }
		if (run && run[0] == '/' && strncmp(run, "/run/user/", 10) != 0 && access(run, F_OK) == 0) {
			argv[n++] = "--tmpfs"; argv[n++] = run;
		}
		argv[n++] = "--unsetenv"; argv[n++] = "DBUS_SESSION_BUS_ADDRESS";
		if (zone) {                                   /* aucun écran pour le décodeur */
			argv[n++] = "--unsetenv"; argv[n++] = "WAYLAND_DISPLAY";
			argv[n++] = "--unsetenv"; argv[n++] = "DISPLAY";
		}
		argv[n++] = "--bind"; argv[n++] = dossier; argv[n++] = dossier;
		argv[n++] = "--ro-bind"; argv[n++] = mpv; argv[n++] = mpv;
		if (reseau == 0) { argv[n++] = "--ro-bind"; argv[n++] = reel; argv[n++] = reel; }
		if (reseau == 2) { argv[n++] = "--dev-bind"; argv[n++] = reel; argv[n++] = reel; }   /* ce nœud seul */
		if (run && run[0] == '/') {
			snprintf(pulse, sizeof pulse, "%s/pulse", run);
			if (access(pulse, F_OK) == 0) { argv[n++] = "--ro-bind"; argv[n++] = pulse; argv[n++] = pulse; }
			if (!zone && wl && *wl && !strchr(wl, '/')) {
				snprintf(wayland, sizeof wayland, "%s/%s", run, wl);
				if (access(wayland, F_OK) == 0) { argv[n++] = "--ro-bind"; argv[n++] = wayland; argv[n++] = wayland; }
			}
		}
		if (!zone && access("/tmp/.X11-unix", F_OK) == 0) { argv[n++] = "--ro-bind"; argv[n++] = "/tmp/.X11-unix"; argv[n++] = "/tmp/.X11-unix"; }
		if (!zone && xa && xa[0] == '/' && access(xa, R_OK) == 0) { argv[n++] = "--ro-bind"; argv[n++] = xa; argv[n++] = xa; }
		if (reseau != 1) argv[n++] = "--unshare-net";
		argv[n++] = "--unshare-pid";
		argv[n++] = "--die-with-parent";
		argv[n++] = "--new-session";
		argv[n++] = "--";
	}
	argv[n++] = mpv;
	argv[n++] = "--no-config";
	argv[n++] = "--no-terminal";
	argv[n++] = rtsp_auth ? "--idle=once" : "--idle=no";   /* RTSP à mot de passe : la source arrive par le socket */
	argv[n++] = "--ytdl=no";
	argv[n++] = "--load-scripts=no";
	argv[n++] = "--load-auto-profiles=no";
	argv[n++] = "--input-default-bindings=no";
	argv[n++] = "--load-unsafe-playlists=no";
	argv[n++] = "--access-references=no";
	/* Dans la cage, seul le son pulse est remonté : PipeWire y est injoignable
	 * (voulu : pipewire-0 donne aussi l'écran et les caméras), et mpv qui
	 * l'essaie d'abord restait BLOQUÉ au lieu de passer à pulse (mesuré). */
	if (avec_bac)
		argv[n++] = "--ao=pulse,null";
	argv[n++] = ipc;
	argv[n++] = titre;
	if (*opt_inclure) argv[n++] = opt_inclure;
	if (*lavf) argv[n++] = lavf;
	if (*journal) argv[n++] = journal;
	if (*opt_transport) argv[n++] = opt_transport;
	if (zone) argv[n++] = opt_images;
	if (*wid) {
		/* Incrusté : mpv ne lit ni souris ni clavier de la fenêtre sermo (sinon,
		 * mesuré sous xfwm4 : un clic laissé sur le bouton relançait l'action). */
		argv[n++] = wid;
		argv[n++] = "--input-cursor=no";
		argv[n++] = "--input-vo-keyboard=no";
	}
	if (*osd) {   /* lisible même dans une petite zone : taille fixe, pas proportionnelle */
		argv[n++] = "--osd-level=1"; argv[n++] = "--osd-scale-by-window=no"; argv[n++] = "--osd-font-size=24"; argv[n++] = osd;
	}
	if (!rtsp_auth) {
		argv[n++] = "--";
		argv[n++] = reseau == 0 ? reel : reseau == 2 ? av : source;
	}
	if (n >= ARGS_MAX) {                         /* ne doit jamais arriver : arrêt net */
		fputs(PROGRAMME ": trop d'arguments pour mpv (erreur interne)\n", stderr);
		abort();
	}
	argv[n] = NULL;

	pid_t maitre = maitre_trouver();
	/* Une caméra refuse parfois la PREMIÈRE ouverture qui suit une fermeture
	 * récente (mesuré : « avformat_open_input() failed » après 1,3 s, environ une
	 * fois sur trois) : une seule nouvelle tentative, 1,5 s plus tard. Jamais
	 * quand elle est occupée (rien n'est forcé). */
	for (int essai = 1; ; essai++) {
		char ancien[700];                      /* le journal d'un lancement précédent fausserait le diagnostic */
		snprintf(ancien, sizeof ancien, "%s/mpv.log", dossier);
		unlink(ancien);
		pid_t g = fork();
		if (g < 0) {
			fputs(PROGRAMME ": fork impossible\n", stderr);
			return 1;
		}
		if (g == 0)
			gardien(dossier, maitre, (char *const *)argv, *inclure != '\0');
		char t[32];
		snprintf(t, sizeof t, "%d\n", (int)g);
		ecrire_fichier(dossier, "gardien", t);
		/* Trois secondes pour voir mpv démarrer (socket) ou échouer (fin). */
		char chemin_ipc[620];
		snprintf(chemin_ipc, sizeof chemin_ipc, "%s/ipc", dossier);
		for (int i = 0; i < 30; i++) {
			if (access(chemin_ipc, F_OK) == 0 || access(fin, F_OK) == 0)
				break;
			dormir_ms(100);
		}
		if (rtsp_auth) {
			char req[1300], rep[1024];
			snprintf(req, sizeof req, "{\"command\":[\"loadfile\",\"%s\"]}", url_auth);
			int ok = 0;
			for (int i = 0; i < 20 && !ok && access(fin, F_OK) != 0; i++) {
				ok = media_ipc(dossier, req, rep, sizeof rep) == 0 && strstr(rep, "\"error\":\"success\"");
				if (!ok)
					dormir_ms(100);
			}
			explicit_bzero(req, sizeof req);
			explicit_bzero(url_auth, sizeof url_auth);
			if (!ok) {
				fprintf(stderr, PROGRAMME ": le lecteur « %s » n'a pas reçu la source RTSP\n", nom);
				arreter_existant(dossier);
				return 1;
			}
		}
		/* Caméra : le socket apparaît AVANT l'ouverture du périphérique (mesuré) ;
		 * on n'annonce la lecture qu'à la première image (video-params), ou
		 * l'échec quand mpv s'arrête. Cinq secondes au plus. */
		for (int i = 0; reseau == 2 && i < 50 && access(fin, F_OK) != 0; i++) {
			char rep[1024];
			if (media_ipc(dossier, "{\"command\":[\"get_property\",\"video-params\"]}", rep, sizeof rep) == 0
			    && strstr(rep, "\"w\""))
				break;
			dormir_ms(100);
		}
		if (access(fin, F_OK) == 0) {
			char mot[16] = "";
			FILE *f = fopen(fin, "r");
			if (f) { if (!fgets(mot, sizeof mot, f)) mot[0] = 0; fclose(f); }
			if (strncmp(mot, "erreur", 6) == 0) {
				char err[700], ligne[512];
				int occupee = 0, ouverture = 0;
				for (int j = 0; j < 2; j++) {
					snprintf(err, sizeof err, j ? "%s/mpv.log" : "%s/mpv.err", dossier);
					FILE *e = fopen(err, "r");
					while (e && fgets(ligne, sizeof ligne, e)) {
						if (strstr(ligne, "Device or resource busy"))
							occupee = 1;
						if (strstr(ligne, "avformat_open_input() failed"))
							ouverture = 1;
					}
					if (e) fclose(e);
				}
				if (reseau == 2 && (occupee || ouverture) && essai == 1) {
					unlink(fin);
					dormir_ms(1500);
					continue;
				}
				/* Tenue par un autre programme, mpv dit seulement « avformat_open_input()
				 * failed » (mesuré : rien ne nomme « busy ») : on ne peut pas trancher. */
				if (reseau == 2 && (occupee || ouverture))
					fputs(PROGRAMME ": caméra occupée ou indisponible (un autre programme la tient-il ?) : "
					      "rien n'a été forcé, réessayez dans quelques secondes\n", stderr);
				else
					fprintf(stderr, PROGRAMME ": mpv n'a pas pu lire la source (détails : %s/mpv.err)\n", dossier);
				return 1;
			}
		}
		break;
	}
	if (!avec_bac)
		fputs(PROGRAMME ": note : bwrap absent, mpv tourne sans bac à sable\n", stderr);
	if (reseau == 2)
		ecrire_fichier(dossier, "camera", fm && strcmp(fm, "yuyv422") == 0 ? "yuyv422\n" : "mjpeg\n");
	printf("lecture : %s\n", nom);
	return 0;
}

/* fichier= : chemin exact, absolu, extension fermée, dossier à vous (ni groupe
 * ni autres en écriture), fichier inexistant. Écrit le chemin résolu dans sortie. */
static int
cible_valider(const char *brute, const char *const extensions[], char *sortie, size_t taille)
{
	if (!brute || brute[0] != '/' || strlen(brute) >= PATH_MAX - 1) {
		fputs(PROGRAMME ": fichier= : chemin absolu attendu\n", stderr);
		return -1;
	}
	for (const char *c = brute; *c; c++)
		if ((unsigned char)*c < 0x20 || *c == 0x7f || *c == '"' || *c == '\\') {
			fputs(PROGRAMME ": fichier= : caractère refusé dans le chemin\n", stderr);
			return -1;
		}
	if (strstr(brute, "/../") || strstr(brute, "/./") || strstr(brute, "//")) {
		fputs(PROGRAMME ": fichier= : chemin non normalisé (.., ., //) refusé\n", stderr);
		return -1;
	}
	const char *base = strrchr(brute, '/') + 1;
	const char *point = strrchr(base, '.');
	int ok = 0;
	for (int i = 0; point && point != base && extensions[i] && !ok; i++)
		ok = strcmp(point + 1, extensions[i]) == 0;
	if (!ok || base[0] == '.') {
		fprintf(stderr, PROGRAMME ": fichier= : nom ou extension refusé (%s%s%s)\n", extensions[0],
		        extensions[1] ? ", " : "", extensions[1] ? extensions[1] : "");
		return -1;
	}
	char dossier[PATH_MAX], reel[PATH_MAX];
	size_t l = (size_t)(base - brute) - 1;
	if (l == 0)
		l = 1;                                  /* « /photo.png » : la racine */
	memcpy(dossier, brute, l);
	dossier[l] = '\0';
	struct stat st;
	if (!realpath(dossier, reel) || stat(reel, &st) != 0 || !S_ISDIR(st.st_mode)) {
		fprintf(stderr, PROGRAMME ": fichier= : le dossier %s n'existe pas\n", dossier);
		return -1;
	}
	if (st.st_uid != geteuid() || (st.st_mode & 022)) {
		fprintf(stderr, PROGRAMME ": fichier= : %s doit être à vous, sans écriture pour le groupe ni les autres\n", reel);
		return -1;
	}
	int n = snprintf(sortie, taille, "%s/%s", strcmp(reel, "/") == 0 ? "" : reel, base);
	if (n < 0 || (size_t)n >= taille)
		return -1;
	if (lstat(sortie, &st) == 0 || errno != ENOENT) {
		fprintf(stderr, PROGRAMME ": fichier= : %s existe déjà : jamais écrasé\n", sortie);
		return -1;
	}
	return 0;
}

/* Recopie src (fichier ordinaire du dossier privé) vers dst, créé (O_EXCL) en 600,
 * puis efface src. -1 : dst n'existe pas (effacé en cas d'échec). */
static int
deposer(const char *src, const char *dst)
{
	int s = open(src, O_RDONLY | O_NOFOLLOW | O_CLOEXEC);
	struct stat st;
	if (s < 0 || fstat(s, &st) != 0 || !S_ISREG(st.st_mode) || st.st_size == 0) {
		if (s >= 0) close(s);
		return -1;
	}
	int d = open(dst, O_WRONLY | O_CREAT | O_EXCL | O_NOFOLLOW | O_CLOEXEC, 0600);
	if (d < 0) {
		close(s);
		return -1;
	}
	int ok = 1;                                  /* créé en 600 par O_EXCL : rien à resserrer */
	char tampon[65536];
	ssize_t r;
	while (ok && (r = read(s, tampon, sizeof tampon)) != 0) {
		if (r < 0) { if (errno == EINTR) continue; ok = 0; break; }
		for (ssize_t f = 0; ok && f < r; ) {
			ssize_t w = write(d, tampon + f, (size_t)(r - f));
			if (w < 0) { if (errno != EINTR) ok = 0; } else f += w;
		}
	}
	ok = ok && fsync(d) == 0;
	ok = close(d) == 0 && ok;
	close(s);
	if (!ok) {
		unlink(dst);
		return -1;
	}
	unlink(src);
	return 0;
}

/* Le format de la caméra en cours (« mjpeg », « yuyv422 »), ou "" : pas une caméra. */
static void
camera_format(const char *dossier, char *format, size_t taille)
{
	char chemin[620];
	format[0] = '\0';
	snprintf(chemin, sizeof chemin, "%s/camera", dossier);
	FILE *f = fopen(chemin, "re");
	if (!f)
		return;
	if (fgets(format, (int)taille, f))
		format[strcspn(format, "\n")] = '\0';
	fclose(f);
}

static void
indicateur(const char *dossier, const char *nom, int enregistre)
{
	char r[160], rep[256];
	snprintf(r, sizeof r, "{\"command\":[\"set_property\",\"title\",\"sermo — CAMÉRA ACTIVE%s — %s\"]}",
	         enregistre ? " — ENREGISTREMENT" : "", nom);
	media_ipc(dossier, r, rep, sizeof rep);
	snprintf(r, sizeof r, "{\"command\":[\"set_property\",\"osd-msg1\",\"CAMÉRA ACTIVE%s — %s\"]}",
	         enregistre ? " — ENREGISTREMENT" : "", nom);
	media_ipc(dossier, r, rep, sizeof rep);
	media_ipc(dossier, "{\"command\":[\"set_property\",\"osd-level\",1]}", rep, sizeof rep);
}

/* Processus à part : attend la fin (durée, couper, lecteur arrêté), ferme
 * l'enregistrement et dépose le fichier. Écrit « enregistrement » : fait|échec. */
static void
enregistreur(const char *dossier, const char *nom, const char *video, const char *dst, long duree)
{
	setsid();
	int nul = open("/dev/null", O_RDWR | O_CLOEXEC);
	if (nul >= 0) { dup2(nul, 0); dup2(nul, 1); dup2(nul, 2); }
	for (int fd = 3; fd < 1024; fd++)
		close(fd);
	char t[32], couper[620], rep[256];
	snprintf(t, sizeof t, "%d\n", (int)getpid());
	ecrire_fichier(dossier, "enregistreur", t);
	snprintf(couper, sizeof couper, "%s/couper", dossier);
	for (long i = 0; i < duree * 10; i++) {
		if (access(couper, F_OK) == 0 || !media_pid_vivant(dossier, "mpv"))
			break;
		dormir_ms(100);
	}
	media_ipc(dossier, "{\"command\":[\"set_property\",\"stream-record\",\"\"]}", rep, sizeof rep);
	if (media_pid_vivant(dossier, "mpv"))
		indicateur(dossier, nom, 0);
	dormir_ms(300);                              /* mpv ferme le fichier */
	ecrire_fichier(dossier, "enregistrement", deposer(video, dst) == 0 ? "fait\n" : "échec\n");
	unlink(video);
	unlink(couper);
	snprintf(couper, sizeof couper, "%s/enregistreur", dossier);
	unlink(couper);
	_exit(0);
}

static int
capturer(const char *nom, const char *dossier, const char *commande, const adresse_t *a)
{
	char format[16], dst[PATH_MAX], src[700], r[900], rep[1024];
	camera_format(dossier, format, sizeof format);
	if (!*format) {
		fprintf(stderr, PROGRAMME ": %s : pour une caméra en cours seulement\n", commande);
		return 1;
	}
	for (const char *c = dossier; *c; c++)
		if (*c == '"' || *c == '\\')
			return 1;                              /* jamais dans une requête JSON */
	if (strcmp(commande, "couper") == 0) {
		if (!media_pid_vivant(dossier, "enregistreur")) {
			fputs(PROGRAMME ": aucun enregistrement en cours\n", stderr);
			return 1;
		}
		ecrire_fichier(dossier, "couper", "1\n");
		puts("enregistrement coupé");
		return 0;
	}
	int photo = strcmp(commande, "photo") == 0;
	static const char *const ext_photo[] = { "png", "jpg", NULL };
	static const char *const ext_video[] = { "mkv", NULL };
	const char *brute = adresse_valeur(a, "fichier");
	if (!brute || !*brute) {
		fputs(PROGRAMME ": fichier= manquant (chemin exact du fichier à écrire)\n", stderr);
		return 1;
	}
	if (cible_valider(brute, photo ? ext_photo : ext_video, dst, sizeof dst) != 0)
		return 1;
	if (photo) {
		const char *ext = strrchr(dst, '.');
		snprintf(src, sizeof src, "%s/photo%s", dossier, ext);
		unlink(src);
		snprintf(r, sizeof r, "{\"command\":[\"screenshot-to-file\",\"%s\",\"video\"]}", src);
		/* Juste après le démarrage, mpv n'a pas encore d'image affichée
		 * (« Taking screenshot failed », mesuré sur la vraie caméra) : 2 s au plus. */
		int pris = 0;
		for (int i = 0; i < 10 && !pris; i++) {
			pris = media_ipc(dossier, r, rep, sizeof rep) == 0 && strstr(rep, "\"error\":\"success\"");
			if (!pris)
				dormir_ms(200);
		}
		if (!pris || deposer(src, dst) != 0) {
			unlink(src);
			fprintf(stderr, PROGRAMME ": photo non prise (le lecteur « %s » a-t-il une image ?)\n", nom);
			return 1;
		}
		printf("photo : %s\n", dst);
		return 0;
	}
	/* Enregistrement. */
	if (strcmp(format, "mjpeg") != 0) {
		fputs(PROGRAMME ": enregistrer : caméra en format=mjpeg seulement (yuyv422 n'est pas enregistrable tel quel)\n", stderr);
		return 1;
	}
	if (media_pid_vivant(dossier, "enregistreur")) {
		fputs(PROGRAMME ": un enregistrement est déjà en cours (commande=couper)\n", stderr);
		return 1;
	}
	const char *dv = adresse_valeur(a, "duree");
	char *fin;
	errno = 0;
	long duree = dv ? strtol(dv, &fin, 10) : 0;
	if (!dv || errno || fin == dv || *fin || duree < 1 || duree > 600) {
		fputs(PROGRAMME ": duree= : un nombre entier de secondes, de 1 à 600\n", stderr);
		return 1;
	}
	char video[700], couper[620];
	snprintf(video, sizeof video, "%s/video.mkv", dossier);
	snprintf(couper, sizeof couper, "%s/couper", dossier);
	unlink(video);
	unlink(couper);
	snprintf(r, sizeof r, "{\"command\":[\"set_property\",\"stream-record\",\"%s\"]}", video);
	if (media_ipc(dossier, r, rep, sizeof rep) != 0 || !strstr(rep, "\"error\":\"success\"")) {
		fprintf(stderr, PROGRAMME ": le lecteur « %s » n'a pas commencé l'enregistrement\n", nom);
		return 1;
	}
	indicateur(dossier, nom, 1);
	fflush(stdout);
	pid_t e = fork();
	if (e < 0) {
		media_ipc(dossier, "{\"command\":[\"set_property\",\"stream-record\",\"\"]}", rep, sizeof rep);
		unlink(video);
		return 1;
	}
	if (e == 0)
		enregistreur(dossier, nom, video, dst, duree);
	for (int i = 0; i < 20 && !media_pid_vivant(dossier, "enregistreur"); i++)
		dormir_ms(50);
	printf("enregistrement : %s (%ld s au plus)\n", dst, duree);
	return 0;
}

static int
commander(const char *nom, const char *dossier, const char *commande, const adresse_t *a)
{
	char requete[160], rep[1024];
	if (!media_pid_vivant(dossier, "gardien")) {
		fprintf(stderr, PROGRAMME ": aucun lecteur « %s » en cours\n", nom);
		return 1;
	}
	if (strcmp(commande, "pause") == 0)
		snprintf(requete, sizeof requete, "{\"command\":[\"set_property\",\"pause\",true]}");
	else if (strcmp(commande, "reprendre") == 0)
		snprintf(requete, sizeof requete, "{\"command\":[\"set_property\",\"pause\",false]}");
	else if (strcmp(commande, "muet") == 0)
		snprintf(requete, sizeof requete, "{\"command\":[\"set_property\",\"mute\",true]}");
	else if (strcmp(commande, "son") == 0)
		snprintf(requete, sizeof requete, "{\"command\":[\"set_property\",\"mute\",false]}");
	else if (strcmp(commande, "arreter") == 0) {
		arreter_existant(dossier);
		puts("arrêté");
		return 0;
	} else if (strcmp(commande, "photo") == 0 || strcmp(commande, "enregistrer") == 0
	           || strcmp(commande, "couper") == 0) {
		return capturer(nom, dossier, commande, a);
	} else if (strcmp(commande, "aller") == 0 || strcmp(commande, "volume") == 0) {
		const char *brute = adresse_valeur(a, "valeur");
		if (!brute || !*brute) {
			fprintf(stderr, PROGRAMME ": valeur manquante (exemple : valeur=30, ou valeur=$NOM)\n");
			return 1;
		}
		const char *v = media_valeur(PROGRAMME, brute);
		if (!v)
			return 1;                      /* la raison est déjà dite */
		/* Un nombre, borné, écrit par nous : jamais la chaîne reçue. On accepte
		 * « 42 » et « 42.5 » (curseur) ; la partie décimale est ignorée. */
		/* Point décimal quelle que soit la locale du poste. */
		char *fin;
		locale_t lc = newlocale(LC_NUMERIC_MASK, "C", (locale_t)0);
		if (!lc)
			return 1;
		errno = 0;
		double d = strtod_l(v, &fin, lc); /* garde:primitive-locale */
		freelocale(lc);
		long max = strcmp(commande, "aller") == 0 ? 86400 : 100;
		if (errno || fin == v || *fin != '\0' || d < 0 || d > max || d != d) {
			fprintf(stderr, PROGRAMME ": valeur « %s » refusée (nombre de 0 à %ld)\n", v, max);
			return 1;
		}
		if (strcmp(commande, "aller") == 0)
			snprintf(requete, sizeof requete, "{\"command\":[\"seek\",%ld,\"absolute\"]}", (long)d);
		else
			snprintf(requete, sizeof requete, "{\"command\":[\"set_property\",\"volume\",%ld]}", (long)d);
	} else {
		fprintf(stderr, PROGRAMME ": commande « %s » inconnue "
		        "(lire, pause, reprendre, arreter, aller, volume, muet, son, photo, enregistrer, couper)\n", commande);
		return 1;
	}
	if (media_ipc(dossier, requete, rep, sizeof rep) != 0 || !strstr(rep, "\"error\":\"success\"")) {
		fprintf(stderr, PROGRAMME ": le lecteur « %s » n'a pas accepté « %s »\n", nom, commande);
		return 1;
	}
	return 0;
}

int
main(int argc, char **argv)
{
	adresse_t a;
	char nom[MEDIA_NOM_MAX + 1], dossier[600];
	static const char *const connues[] = { "commande", "source", "identifiants", "valeur", "taille", "format", "dans", "fichier", "duree", "transport", NULL };
	if (argc != 2) {
		fputs("usage : " PROGRAMME " 'media://NOM?commande=…'\n", stderr);
		return 1;
	}
	if (adresse_lire(PROGRAMME, argv[1], &a) != 0)
		return 1;
	int rc = 1;
	if (adresse_cles_connues(PROGRAMME, &a, connues) != 0 || media_nom(PROGRAMME, a.base, nom, sizeof nom) != 0)
		goto fin;
	const char *commande = adresse_valeur(&a, "commande");
	if (!commande || !*commande) {
		fputs(PROGRAMME ": commande manquante (commande=lire, pause, reprendre, arreter, aller, volume, muet, son, photo, enregistrer, couper)\n", stderr);
		goto fin;
	}
	int creer = strcmp(commande, "lire") == 0;
	if (media_dossier(PROGRAMME, nom, dossier, sizeof dossier, creer) != 0) {
		if (!creer)
			fprintf(stderr, PROGRAMME ": aucun lecteur « %s » en cours\n", nom);
		goto fin;
	}
	rc = creer ? lire(nom, dossier, &a) : commander(nom, dossier, commande, &a);
fin:
	adresse_liberer(&a);
	return rc;
}
