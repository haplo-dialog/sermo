/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * test_safe_exec.c — Tests de COMPORTEMENT de safe_system() / safe_popen()
 * haplo-dialog — cœur partagé (GPL-2.0-or-later)
 *
 * Complète la suite XML (qui ne teste que le PARSE) : ici on exécute
 * réellement le cœur sécurité et on vérifie le comportement.
 *
 * « main() pur » : ni libcheck ni serveur X requis — runnable en CI.
 * Compilé contre chaque copie de safe_exec.c du cœur (voir run_unit_tests.sh).
 * Sortie : exit 0 si tout passe, 1 sinon.
 */
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <glib.h>
#include "safe_exec.h"

static int failures = 0;

#define EXPECT(cond, msg)                                              \
    do {                                                               \
        if (!(cond)) {                                                 \
            fprintf(stderr, "FAIL  %s\n", (msg)); failures++;          \
        } else {                                                       \
            printf("PASS  %s\n", (msg));                               \
        }                                                              \
    } while (0)

int main(void)
{
    printf("=== test_safe_exec (cœur haplo-dialog) ===\n");

    /* Exécution : code de retour propagé */
    EXPECT(safe_system("true")  == 0, "safe_system(\"true\")  -> 0");
    EXPECT(safe_system("false") != 0, "safe_system(\"false\") -> non-zero");

    /* Lecture de sortie via safe_popen + fclose (pas pclose : fdopen interne) */
    FILE *fp = safe_popen("echo hello");
    EXPECT(fp != NULL, "safe_popen(\"echo hello\") -> non-NULL");
    if (fp) {
        char buf[64] = {0};
        EXPECT(fgets(buf, sizeof(buf), fp) != NULL, "safe_popen: lit des données");
        EXPECT(strncmp(buf, "hello", 5) == 0, "safe_popen: sortie = \"hello\"");
        fclose(fp);
        printf("PASS  safe_popen: fclose() sans crash\n");
    }

    /* Sortie multi-lignes */
    FILE *fp2 = safe_popen("printf 'a\\nb\\nc\\n'");
    EXPECT(fp2 != NULL, "safe_popen(3 lignes) -> non-NULL");
    if (fp2) {
        int n = 0; char b[64];
        while (fgets(b, sizeof(b), fp2)) n++;
        fclose(fp2);
        EXPECT(n == 3, "safe_popen: lit 3 lignes");
    }

    /* Robustesse : commande vide -> pas de crash (comportement défini) */
    (void)safe_system("");
    printf("PASS  safe_system(\"\") sans crash\n");

    /* SERMO_ALLOWED_CMDS : la commande telle qu'elle sera lancée, pas un nom de
     * base. Un faux « true » posé hors du PATH ne doit pas franchir la liste. */
    {
        char modele[] = "/tmp/sermo-test-XXXXXX";
        char *dossier = mkdtemp(modele);
        char faux[256];
        gchar *resolu = g_find_program_in_path("true");

        EXPECT(dossier != NULL, "dossier temporaire pour le faux programme");
        EXPECT(resolu != NULL, "le PATH trouve true");
        if (dossier && resolu) {
            FILE *f;
            snprintf(faux, sizeof(faux), "%s/true", dossier);
            f = fopen(faux, "w");
            if (f) { fputs("#!/bin/sh\nexit 0\n", f); fclose(f); }
            chmod(faux, 0755);

            unsetenv("SERMO_ALLOWED_CMDS"); unsetenv("HAPLO_ALLOWED_CMDS");
            EXPECT(safe_system(faux) == 0, "sans liste : le faux true s'exécute (témoin)");

            setenv("SERMO_ALLOWED_CMDS", "true", 1);
            EXPECT(safe_system("true") == 0, "liste « true » : true passe");
            EXPECT(safe_system(faux) == -1, "liste « true » : un faux true hors du PATH est refusé");
            EXPECT(safe_system(resolu) == 0, "liste « true » : le chemin où le PATH trouve true passe");
            EXPECT(safe_system("false") == -1, "liste « true » : false est refusé");

            setenv("SERMO_ALLOWED_CMDS", resolu, 1);
            EXPECT(safe_system("true") == 0, "liste « <chemin résolu> » : le nom seul passe");
            EXPECT(safe_system("true | true") == -1, "liste posée : le repli shell est refusé");
            unsetenv("SERMO_ALLOWED_CMDS");

            /* Noms de la 1.x : encore lus quand le nom SERMO_… est absent */
            setenv("HAPLO_ALLOWED_CMDS", "false", 1);
            EXPECT(safe_system("true") == -1, "HAPLO_ALLOWED_CMDS (1.x) : la liste mord");
            setenv("SERMO_ALLOWED_CMDS", "true", 1);
            EXPECT(safe_system("true") == 0, "SERMO_ALLOWED_CMDS l'emporte sur HAPLO_ALLOWED_CMDS");
            unsetenv("SERMO_ALLOWED_CMDS"); unsetenv("HAPLO_ALLOWED_CMDS");

            EXPECT(safe_system("true | true") == 0, "sans variable : le repli shell passe (témoin)");
            setenv("HAPLO_NO_SHELL_FALLBACK", "1", 1);
            EXPECT(safe_system("true | true") == -1, "HAPLO_NO_SHELL_FALLBACK (1.x) : repli refusé");
            unsetenv("HAPLO_NO_SHELL_FALLBACK");

            unlink(faux);
            rmdir(dossier);
        }
        g_free(resolu);
    }

    /* 2.7.3 : ce que rend safe_popen() est plafonné (sermo_input.h), 16 Mio
     * par défaut. Joué contre chaque copie de safe_exec.c : une copie qui
     * oublierait l'enveloppe lirait `yes` sans fin. */
    unsetenv("SERMO_INPUT_MAX");
    {
        FILE  *fy = safe_popen("yes");
        size_t total = 0, n;
        char   tampon[65536];

        EXPECT(fy != NULL, "safe_popen(\"yes\") -> non-NULL");
        if (fy) {
            /* Borné à 32 Mio : sans l'enveloppe, le test échoue au lieu de
             * lire sans fin. */
            while (total <= (size_t) 32 * 1024 * 1024
                   && (n = fread(tampon, 1, sizeof(tampon), fy)) > 0)
                total += n;
            fclose(fy);
            EXPECT(total == (size_t) 16 * 1024 * 1024,
                   "safe_popen(\"yes\") : la lecture s'arrête à 16 Mio");
        }
    }

    printf("\n=== %d échec(s) ===\n", failures);
    return failures ? 1 : 0;
}
