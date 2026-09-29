/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * test_sermo_input.c — Tests de COMPORTEMENT de la limite des <input>
 * (libsermocore/include/sermo_input.h, 2.7.3).
 * haplo-dialog — cœur partagé (GPL-2.0-or-later)
 *
 * La limite se lit UNE fois par processus (SERMO_INPUT_MAX) : chaque réglage
 * se joue donc dans un processus à lui. Sans argument, ce programme se relance
 * lui-même, une fois par cas, avec l'environnement du cas ; avec un nom de cas,
 * il joue ce cas.
 *
 * « main() pur » : ni libcheck ni serveur X requis. Compilé avec
 * libsermocore/src/sermo_input.c et safe_exec.c (voir run_unit_tests.sh).
 * Sortie : exit 0 si tout passe, 1 sinon.
 */
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <glib.h>
#include <glib/gstdio.h>
#include "safe_exec.h"
#include "sermo_input.h"

static int failures = 0;

#define EXPECT(cond, msg)                                              \
    do {                                                               \
        if (!(cond)) {                                                 \
            fprintf(stderr, "FAIL  %s\n", (msg)); failures++;          \
        } else {                                                       \
            printf("PASS  %s\n", (msg));                               \
        }                                                              \
    } while (0)

/* Les avertissements du cœur, captés au lieu d'aller sur la sortie d'erreur. */
static GString *journal;
static int      avertissements;

static void capter(const gchar *domaine, GLogLevelFlags niveau,
                   const gchar *message, gpointer donnees)
{
    (void) domaine; (void) donnees;
    if (niveau & G_LOG_LEVEL_WARNING)
        avertissements++;
    g_string_append_printf(journal, "%s\n", message);
}

static void remettre_journal(void)
{
    g_string_truncate(journal, 0);
    avertissements = 0;
}

/* Lit jusqu'à la fin, borné à 64 Mio : une limite qui ne mordrait plus fait
 * échouer le test au lieu de le faire tourner sans fin. */
static size_t tout_lire(FILE *f)
{
    char   tampon[4096];
    size_t n, total = 0;

    while (total <= (size_t) 64 * 1024 * 1024
           && (n = fread(tampon, 1, sizeof tampon, f)) > 0)
        total += n;
    return total;
}

/* Un fichier temporaire de @taille octets, fait de lignes de 100 octets. */
static char *fichier(const char *dossier, size_t taille)
{
    char  *chemin = g_strdup_printf("%s/f%zu", dossier, taille);
    FILE  *f = fopen(chemin, "w");
    size_t i;

    if (f == NULL) { perror(chemin); exit(2); }
    for (i = 0; i < taille; i++)
        fputc((i % 100 == 99) ? '\n' : 'x', f);
    fclose(f);
    return chemin;
}

/* ─── SERMO_INPUT_MAX absente : 16 Mio ─────────────────────────────────── */
static void cas_defaut(void)
{
    FILE *f;

    EXPECT(sermo_input_max() == 16u * 1024 * 1024, "défaut : la limite vaut 16 Mio");

    remettre_journal();
    f = sermo_fopen_input("/dev/zero");
    EXPECT(f != NULL, "défaut : /dev/zero s'ouvre");
    if (f) {
        EXPECT(fileno(f) == -1, "défaut : le flux plafonné n'a pas de descripteur propre");
        EXPECT(tout_lire(f) == 16u * 1024 * 1024, "défaut : /dev/zero s'arrête à 16 Mio pile");
        EXPECT(fclose(f) == 0, "défaut : fclose() du flux plafonné rend 0");
    }
    EXPECT(avertissements == 1, "défaut : un avertissement, un seul");
    EXPECT(strstr(journal->str, "/dev/zero") && strstr(journal->str, "16777216"),
           "défaut : l'avertissement nomme la source et la limite");

    /* Les deux copies de safe_exec.c passent par la même enveloppe : une
     * commande sans fin s'arrête aussi. */
    remettre_journal();
    f = safe_popen("yes");
    EXPECT(f != NULL, "défaut : safe_popen(\"yes\") s'ouvre");
    if (f) {
        EXPECT(tout_lire(f) == 16u * 1024 * 1024, "défaut : `yes` s'arrête à 16 Mio pile");
        EXPECT(fclose(f) == 0, "défaut : fclose() rend la main (yes reçoit SIGPIPE)");
    }
    EXPECT(avertissements == 1 && strstr(journal->str, "« yes »"),
           "défaut : un avertissement, qui nomme la commande");
}

/* ─── SERMO_INPUT_MAX=1000 ─────────────────────────────────────────────── */
static void cas_petit(const char *dossier)
{
    FILE  *f, *brut;
    char  *pile = fichier(dossier, 1000), *plus = fichier(dossier, 1001);
    char  *lignes = fichier(dossier, 2000);
    char   ligne[512];
    int    n;
    size_t lu;

    EXPECT(sermo_input_max() == 1000, "1000 : la limite vaut 1000 octets");

    /* Exactement la limite : pas de troncature, donc pas d'avertissement. */
    remettre_journal();
    f = sermo_fopen_input(pile);
    lu = f ? tout_lire(f) : 0;
    if (f) fclose(f);
    EXPECT(lu == 1000, "1000 : un fichier de 1000 octets se lit en entier");
    EXPECT(avertissements == 0, "1000 : aucun avertissement à la limite pile");

    /* Un octet de trop : 1000 lus, un avertissement. */
    remettre_journal();
    f = sermo_fopen_input(plus);
    lu = f ? tout_lire(f) : 0;
    if (f) {
        EXPECT(fgetc(f) == EOF && feof(f), "1000 : après la limite, fgetc() voit la fin de fichier");
        EXPECT(fgetc(f) == EOF, "1000 : une relecture voit encore la fin de fichier");
        fclose(f);
    }
    EXPECT(lu == 1000, "1000 : un fichier de 1001 octets s'arrête à 1000");
    EXPECT(avertissements == 1, "1000 : un avertissement, un seul, même relu");

    /* Les lecteurs des ports lisent ligne à ligne : ils voient une fin de
     * fichier après la dernière ligne entière. */
    remettre_journal();
    f = sermo_fopen_input(lignes);
    n = 0;
    if (f) {
        while (fgets(ligne, sizeof ligne, f) != NULL)
            n++;
        fclose(f);
    }
    EXPECT(n == 10, "1000 : fgets() lit 10 lignes de 100 octets, puis la fin");

    /* Une commande sans fin. */
    remettre_journal();
    f = safe_popen("yes");
    lu = f ? tout_lire(f) : 0;
    if (f) EXPECT(fclose(f) == 0, "1000 : fclose() de `yes` rend la main");
    EXPECT(lu == 1000 && avertissements == 1, "1000 : `yes` s'arrête à 1000 octets, un avertissement");

    /* La barre de progression reprend le flux réel : plus de limite. */
    remettre_journal();
    f = safe_popen("yes");
    brut = sermo_input_sans_limite(f);
    EXPECT(brut != NULL && brut != f, "1000 : sans_limite() rend le flux réel");
    if (brut) {
        char tampon[5000];
        EXPECT(fileno(brut) >= 0, "1000 : le flux réel a son descripteur");
        EXPECT(fread(tampon, 1, sizeof tampon, brut) == sizeof tampon,
               "1000 : le flux réel se lit au-delà de la limite");
        EXPECT(fclose(brut) == 0, "1000 : fclose() du flux réel rend 0");
    }
    EXPECT(avertissements == 0, "1000 : sans limite, aucun avertissement");

    /* Les cas limites de l'interface. */
    f = fopen(pile, "r");
    EXPECT(f != NULL && sermo_input_sans_limite(f) == f, "1000 : sans_limite() rend un flux ordinaire tel quel");
    if (f) fclose(f);
    EXPECT(sermo_input_sans_limite(NULL) == NULL, "1000 : sans_limite(NULL) rend NULL");
    EXPECT(sermo_input_wrap(NULL, "x") == NULL, "1000 : wrap(NULL) rend NULL");
    EXPECT(sermo_fopen_input(NULL) == NULL, "1000 : sermo_fopen_input(NULL) rend NULL");
    errno = 0;
    EXPECT(sermo_fopen_input("/nonexistent/sermo") == NULL && errno == ENOENT,
           "1000 : un fichier absent rend NULL, errno ENOENT gardé");

    /* Une source au nom très long : l'avertissement n'en cite que le début,
     * et reste de l'UTF-8 valide même coupé au milieu d'un « é ». */
    {
        GString *nom = g_string_new(NULL);
        int i;

        for (i = 0; i < 150; i++)
            g_string_append(nom, "é");     /* 2 octets : l'octet 117 coupe un « é » */
        remettre_journal();
        f = sermo_input_wrap(fopen(plus, "r"), nom->str);
        if (f) { tout_lire(f); fclose(f); }
        EXPECT(avertissements == 1, "nom long : un avertissement");
        EXPECT(strstr(journal->str, nom->str) == NULL, "nom long : le nom n'est pas cité en entier");
        EXPECT(strstr(journal->str, "…") != NULL, "nom long : la coupure est marquée « … »");
        EXPECT(g_utf8_validate(journal->str, -1, NULL), "nom long : l'avertissement est de l'UTF-8 valide");
        g_string_free(nom, TRUE);
    }

    g_unlink(pile); g_unlink(plus); g_unlink(lignes);
    g_free(pile); g_free(plus); g_free(lignes);
}

/* ─── SERMO_INPUT_MAX=0 : sans limite ──────────────────────────────────── */
static void cas_zero(const char *dossier)
{
    char *chemin = fichier(dossier, 2000);
    FILE *f = fopen(chemin, "r");

    EXPECT(sermo_input_max() == 0, "0 : la limite est retirée");
    EXPECT(f != NULL && sermo_input_wrap(f, chemin) == f, "0 : wrap() rend le flux tel quel");
    if (f) {
        EXPECT(tout_lire(f) == 2000, "0 : le fichier se lit en entier");
        fclose(f);
    }
    EXPECT(avertissements == 0, "0 : aucun avertissement");
    g_unlink(chemin);
    g_free(chemin);
}

/* ─── SERMO_INPUT_MAX illisible : la limite par défaut, un avertissement ── */
static void cas_illisible(void)
{
    const char *v = g_getenv("SERMO_INPUT_MAX");
    char *msg = g_strdup_printf("« %s » : limite par défaut, un avertissement « illisible »", v ? v : "(absente)");

    remettre_journal();
    EXPECT(sermo_input_max() == SERMO_INPUT_MAX_DEFAUT && avertissements == 1
           && strstr(journal->str, "illisible") != NULL, msg);
    remettre_journal();
    EXPECT(sermo_input_max() == SERMO_INPUT_MAX_DEFAUT && avertissements == 0,
           "… et la valeur n'est lue qu'une fois");
    g_free(msg);
}

/* ─── SERMO_INPUT_MAX vide : comme absente, en silence ─────────────────── */
static void cas_vide(void)
{
    remettre_journal();
    EXPECT(sermo_input_max() == SERMO_INPUT_MAX_DEFAUT && avertissements == 0,
           "vide : limite par défaut, sans avertissement");
}

/* ─── Le chef d'orchestre ──────────────────────────────────────────────── */
typedef struct { const char *cas; const char *valeur; } Scenario;

static int jouer(const char *cas)
{
    char *dossier = g_dir_make_tmp("test_sermo_input-XXXXXX", NULL);

    if (dossier == NULL) { fprintf(stderr, "dossier temporaire impossible\n"); return 2; }
    journal = g_string_new(NULL);
    g_log_set_default_handler(capter, NULL);

    if (strcmp(cas, "defaut") == 0)         cas_defaut();
    else if (strcmp(cas, "petit") == 0)     cas_petit(dossier);
    else if (strcmp(cas, "zero") == 0)      cas_zero(dossier);
    else if (strcmp(cas, "illisible") == 0) cas_illisible();
    else if (strcmp(cas, "vide") == 0)      cas_vide();
    else { fprintf(stderr, "cas inconnu : %s\n", cas); failures++; }

    g_rmdir(dossier);
    g_free(dossier);
    return failures ? 1 : 0;
}

int main(int argc, char **argv)
{
    static const Scenario scenarios[] = {
        { "defaut",    NULL },
        { "petit",     "1000" },
        { "zero",      "0" },
        { "illisible", "16M" },
        { "illisible", "-1" },
        { "illisible", " 5" },
        { "illisible", "5 " },
        { "illisible", "abc" },
        { "illisible", "99999999999999999999999" },
        { "vide",      "" },
    };
    size_t i;
    int    rates = 0;

    if (argc > 1)
        return jouer(argv[1]);

    printf("=== test_sermo_input (cœur haplo-dialog) ===\n");
    fflush(stdout);
    for (i = 0; i < G_N_ELEMENTS(scenarios); i++) {
        const char *args[] = { "/proc/self/exe", scenarios[i].cas, NULL };
        char      **env = g_get_environ();
        GError     *err = NULL;
        gint        statut = 0;

        env = scenarios[i].valeur
            ? g_environ_setenv(env, "SERMO_INPUT_MAX", scenarios[i].valeur, TRUE)
            : g_environ_unsetenv(env, "SERMO_INPUT_MAX");
        printf("── cas %s, SERMO_INPUT_MAX=%s\n", scenarios[i].cas,
               scenarios[i].valeur ? scenarios[i].valeur : "(absente)");
        fflush(stdout);
        if (!g_spawn_sync(NULL, (char **) args, env, G_SPAWN_CHILD_INHERITS_STDIN,
                          NULL, NULL, NULL, NULL, &statut, &err)) {
            fprintf(stderr, "FAIL  relance impossible : %s\n", err->message);
            g_error_free(err);
            rates++;
        } else if (!g_spawn_check_wait_status(statut, NULL)) {
            rates++;
        }
        g_strfreev(env);
    }
    printf("%zu cas joués, %d en échec\n", G_N_ELEMENTS(scenarios), rates);
    return rates ? 1 : 0;
}
