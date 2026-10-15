/*
 * sermo_win32.c — ce que sermo demande directement à Windows : la console,
 * le lancement des commandes, la lecture non bloquante d'un tube, l'ouverture
 * d'un lien.
 *
 * Fichier à part, compilé sans le shim du cœur : <windows.h> y entrerait en
 * collision avec les noms du cœur (le jeton INPUT de l'analyseur, entre
 * autres). Construit seulement pour Windows. Les chaînes reçues sont en UTF-8
 * et passent en UTF-16 pour les fonctions « W » de Windows.
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <shellapi.h>
#include <errno.h>
#include <fcntl.h>
#include <io.h>
#include <locale.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void sermo_win32_utf8(void)
{
	/* sermo lit et écrit de l'UTF-8 (fichiers XML, variables). La locale de
	 * l'utilisateur donnerait une page de code ANSI (Windows-1252 en
	 * français), où « à » devient « Ã  ». On garde ses nombres et sa langue,
	 * mais les caractères passent en UTF-8, et la console aussi. */
	setlocale(LC_CTYPE, ".UTF-8");
	SetConsoleOutputCP(CP_UTF8);
	SetConsoleCP(CP_UTF8);
	/* Sortie en LF, comme partout : en mode texte, chaque ligne NOM="…"
	 * finissait par \r\n, et un script qui la relit gardait le \r dans la
	 * valeur (mesuré le 2026-10-11). */
	_setmode(_fileno(stdout), _O_BINARY);
}

/* ── Conversions ─────────────────────────────────────────────────────────── */

static wchar_t *en_large(const char *s)
{
	int n = MultiByteToWideChar(CP_UTF8, 0, s, -1, NULL, 0);
	wchar_t *w;

	if (n <= 0)
		return NULL;
	w = malloc((size_t) n * sizeof *w);
	if (w != NULL && MultiByteToWideChar(CP_UTF8, 0, s, -1, w, n) <= 0) {
		free(w);
		w = NULL;
	}
	return w;
}

static void poser_erreur(char **erreur, const char *msg, DWORD code)
{
	char tampon[160];

	if (erreur == NULL)
		return;
	if (code != 0)
		snprintf(tampon, sizeof tampon, "%s (erreur Windows %lu)", msg, (unsigned long) code);
	else
		snprintf(tampon, sizeof tampon, "%s", msg);
	*erreur = strdup(tampon);
}

/* Nom du fichier, sans le dossier. */
static const char *nom_seul(const char *chemin)
{
	const char *n = chemin;

	for (const char *p = chemin; *p; p++)
		if (*p == '\\' || *p == '/' || *p == ':')
			n = p + 1;
	return n;
}

int sermo_win32_est_cmd(const char *chemin)
{
	const char *n = nom_seul(chemin);

	return _stricmp(n, "cmd.exe") == 0 || _stricmp(n, "cmd") == 0;
}

/* ── Ligne de commande ───────────────────────────────────────────────────
 * Windows ne passe pas un tableau d'arguments mais UNE ligne, que le
 * programme découpe lui-même. On la compose selon les règles du CRT
 * (CommandLineToArgvW) : chaque argument entre guillemets s'il le faut, un
 * guillemet intérieur précédé d'une barre oblique inverse, et les barres
 * obliques inverses qui précèdent un guillemet doublées.
 *
 * cmd.exe ne suit PAS ces règles : il lit le reste de la ligne tel quel. Pour
 * « cmd /C commande », on compose donc « "cmd.exe" /S /C "commande" » : avec
 * /S, cmd retire seulement la première et la dernière paire de guillemets et
 * exécute le reste sans y toucher. */

static void ajouter(char **tampon, size_t *lg, size_t *cap, const char *s, size_t n)
{
	if (*tampon == NULL)
		return;
	if (*lg + n + 1 > *cap) {
		size_t neuve = (*cap + n + 1) * 2;
		char *t = realloc(*tampon, neuve);

		if (t == NULL) {
			free(*tampon);
			*tampon = NULL;
			return;
		}
		*tampon = t;
		*cap = neuve;
	}
	memcpy(*tampon + *lg, s, n);
	*lg += n;
	(*tampon)[*lg] = '\0';
}

static void ajouter_argument(char **t, size_t *lg, size_t *cap, const char *a)
{
	int guillemets = (*a == '\0') || strpbrk(a, " \t\n\v\"") != NULL;

	if (!guillemets) {
		ajouter(t, lg, cap, a, strlen(a));
		return;
	}
	ajouter(t, lg, cap, "\"", 1);
	for (const char *p = a; ; p++) {
		size_t barres = 0;

		while (*p == '\\') {
			barres++;
			p++;
		}
		if (*p == '\0') {
			/* Avant le guillemet fermant : barres doublées. */
			for (size_t i = 0; i < barres * 2; i++)
				ajouter(t, lg, cap, "\\", 1);
			break;
		}
		if (*p == '"') {
			for (size_t i = 0; i < barres * 2 + 1; i++)
				ajouter(t, lg, cap, "\\", 1);
			ajouter(t, lg, cap, "\"", 1);
		} else {
			for (size_t i = 0; i < barres; i++)
				ajouter(t, lg, cap, "\\", 1);
			ajouter(t, lg, cap, p, 1);
		}
	}
	ajouter(t, lg, cap, "\"", 1);
}

static char *composer_ligne(char *const argv[])
{
	size_t lg = 0, cap = 256;
	char *t = malloc(cap);

	if (t == NULL)
		return NULL;
	t[0] = '\0';
	if (argv[1] != NULL && strcmp(argv[1], "/C") == 0 && argv[2] != NULL
	    && argv[3] == NULL && sermo_win32_est_cmd(argv[0])) {
		ajouter_argument(&t, &lg, &cap, argv[0]);
		ajouter(&t, &lg, &cap, " /S /C \"", 8);
		ajouter(&t, &lg, &cap, argv[2], strlen(argv[2]));
		ajouter(&t, &lg, &cap, "\"", 1);
		return t;
	}
	for (int i = 0; argv[i] != NULL; i++) {
		if (i > 0)
			ajouter(&t, &lg, &cap, " ", 1);
		ajouter_argument(&t, &lg, &cap, argv[i]);
	}
	return t;
}

/* Bloc d'environnement UTF-16 : « NOM=valeur\0 … \0\0 ». */
static wchar_t *composer_env(char *const envp[])
{
	size_t total = 1, pos = 0;
	wchar_t *bloc;

	for (int i = 0; envp[i] != NULL; i++) {
		int n = MultiByteToWideChar(CP_UTF8, 0, envp[i], -1, NULL, 0);

		if (n <= 0)
			return NULL;
		total += (size_t) n;
	}
	bloc = malloc(total * sizeof *bloc);
	if (bloc == NULL)
		return NULL;
	for (int i = 0; envp[i] != NULL; i++) {
		int n = MultiByteToWideChar(CP_UTF8, 0, envp[i], -1, bloc + pos, (int) (total - pos));

		if (n <= 0) {
			free(bloc);
			return NULL;
		}
		pos += (size_t) n;
	}
	bloc[pos] = L'\0';
	return bloc;
}

/* ── Lancement ───────────────────────────────────────────────────────────
 * attendre : 1 = synchrone (statut), 0 = détaché (fd_sortie, pid).
 * Rend 0 si le programme est lancé ; -1 sinon, errno = ENOENT quand Windows
 * ne trouve pas le programme (l'appelant peut alors essayer le shell). */
int sermo_win32_lance(char *const argv[], char *const envp[], int attendre,
                      int *statut, int *fd_sortie, int *pid, char **erreur)
{
	STARTUPINFOW si;
	PROCESS_INFORMATION pi;
	SECURITY_ATTRIBUTES sa = { sizeof sa, NULL, TRUE };
	HANDLE lecture = NULL, ecriture = NULL, nul = INVALID_HANDLE_VALUE;
	char *ligne;
	wchar_t *wligne, *wenv = NULL;
	BOOL ok;
	DWORD err;

	if (argv == NULL || argv[0] == NULL) {
		poser_erreur(erreur, "no argv", 0);
		return -1;
	}
	ligne = composer_ligne(argv);
	wligne = ligne ? en_large(ligne) : NULL;
	free(ligne);
	if (wligne == NULL) {
		poser_erreur(erreur, "ligne de commande illisible", 0);
		return -1;
	}
	if (envp != NULL && (wenv = composer_env(envp)) == NULL) {
		free(wligne);
		poser_erreur(erreur, "environnement illisible", 0);
		return -1;
	}

	memset(&si, 0, sizeof si);
	si.cb = sizeof si;
	if (!attendre) {
		/* Détaché : pas d'entrée (NUL), sortie par un tube si on la lit. */
		nul = CreateFileW(L"NUL", GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE,
		                  &sa, OPEN_EXISTING, 0, NULL);
		if (fd_sortie != NULL) {
			if (!CreatePipe(&lecture, &ecriture, &sa, 0)) {
				err = GetLastError();
				free(wligne); free(wenv);
				if (nul != INVALID_HANDLE_VALUE) CloseHandle(nul);
				poser_erreur(erreur, "pipe failed", err);
				return -1;
			}
			/* Le bout lecteur reste au parent seulement. */
			SetHandleInformation(lecture, HANDLE_FLAG_INHERIT, 0);
		}
		si.dwFlags = STARTF_USESTDHANDLES;
		si.hStdInput = nul;
		si.hStdOutput = ecriture ? ecriture : GetStdHandle(STD_OUTPUT_HANDLE);
		si.hStdError = GetStdHandle(STD_ERROR_HANDLE);
	}

	ok = CreateProcessW(NULL, wligne, NULL, NULL, TRUE,
	                    wenv ? CREATE_UNICODE_ENVIRONMENT : 0,
	                    wenv, NULL, &si, &pi);
	err = ok ? 0 : GetLastError();
	free(wligne);
	free(wenv);
	if (nul != INVALID_HANDLE_VALUE)
		CloseHandle(nul);
	if (ecriture != NULL)
		CloseHandle(ecriture);   /* sinon le lecteur n'aurait jamais sa fin */

	if (!ok) {
		if (lecture != NULL)
			CloseHandle(lecture);
		errno = (err == ERROR_FILE_NOT_FOUND || err == ERROR_PATH_NOT_FOUND) ? ENOENT : EIO;
		poser_erreur(erreur, errno == ENOENT ? "programme introuvable" : "CreateProcess a échoué", err);
		return -1;
	}
	CloseHandle(pi.hThread);

	if (attendre) {
		DWORD code = 1;

		WaitForSingleObject(pi.hProcess, INFINITE);
		GetExitCodeProcess(pi.hProcess, &code);
		CloseHandle(pi.hProcess);
		/* Même forme que le statut de waitpid : le code dans l'octet haut. */
		if (statut != NULL)
			*statut = (int) ((code & 0xff) << 8);
		return 0;
	}

	CloseHandle(pi.hProcess);   /* détaché : rien à récolter sous Windows */
	if (pid != NULL)
		*pid = (int) pi.dwProcessId;
	if (fd_sortie != NULL) {
		/* Mode texte : les \r\n des commandes Windows deviennent des \n. */
		int fd = _open_osfhandle((intptr_t) lecture, _O_RDONLY | _O_TEXT);

		if (fd < 0) {
			CloseHandle(lecture);
			poser_erreur(erreur, "descripteur du tube", 0);
			return -1;
		}
		*fd_sortie = fd;
	}
	return 0;
}

/* ── Tube non bloquant ───────────────────────────────────────────────────
 * Octets prêts à lire sur fd : >= 0 ; -1 si l'autre bout est fermé (fin) ;
 * -2 si fd n'est pas un tube (un fichier se lit sans attendre). */
long sermo_win32_octets_prets(int fd)
{
	HANDLE h = (HANDLE) _get_osfhandle(fd);
	DWORD dispo = 0;

	if (h == INVALID_HANDLE_VALUE)
		return -1;
	if (GetFileType(h) != FILE_TYPE_PIPE)
		return -2;
	if (!PeekNamedPipe(h, NULL, 0, NULL, &dispo, NULL))
		return GetLastError() == ERROR_BROKEN_PIPE ? -1 : -1;
	return (long) dispo;
}

/* ── Lien ────────────────────────────────────────────────────────────────
 * ShellExecute « open » LANCERAIT un programme qu'on lui désigne : on ne lui
 * passe que des adresses web ou de courriel. Rend 0 si l'ouverture est
 * demandée, -1 si l'adresse est refusée ou l'ouverture impossible. */
int sermo_win32_ouvrir_uri(const char *uri)
{
	static const char *const schemas[] = { "http://", "https://", "mailto:" };
	wchar_t *w;
	HINSTANCE r;
	int permis = 0;

	for (size_t i = 0; i < sizeof schemas / sizeof schemas[0]; i++)
		if (_strnicmp(uri, schemas[i], strlen(schemas[i])) == 0)
			permis = 1;
	if (!permis)
		return -1;
	w = en_large(uri);
	if (w == NULL)
		return -1;
	r = ShellExecuteW(NULL, L"open", w, NULL, NULL, SW_SHOWNORMAL);
	free(w);
	return (INT_PTR) r > 32 ? 0 : -1;
}
