/*
 * test_safe_exec.c — Tests de COMPORTEMENT de safe_system() / safe_popen()
 * haplo-dialog — cœur partagé (GPL-2.0-or-later)
 *
 * Complète la suite XML (qui ne teste que le PARSE) : ici on exécute
 * réellement le cœur sécurité et on vérifie le comportement.
 *
 * « main() pur » : ni libcheck ni serveur X requis — runnable en CI.
 * Compilé contre le src/safe_exec.c de chaque port (voir run_unit_tests.sh).
 * Sortie : exit 0 si tout passe, 1 sinon.
 */
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
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

    printf("\n=== %d échec(s) ===\n", failures);
    return failures ? 1 : 0;
}
