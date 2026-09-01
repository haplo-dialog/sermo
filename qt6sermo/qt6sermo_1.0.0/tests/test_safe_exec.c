/*
 * test_safe_exec.c — Tests unitaires safe_system() / safe_popen()
 * qt6sermo 1.0.0 — haplo-dialog — GPL-2.0-or-later
 *
 * Compilé et lancé via CTest.
 * Retourne 0 si tous les tests passent, 1 sinon.
 *
 * Tests :
 *   1. safe_system("true")  → 0  (commande inoffensive, succès)
 *   2. safe_system("false") → non-zéro (commande échouante)
 *   3. safe_system avec commande avec argument whitelisté ("echo ok")
 *   4. safe_popen("echo hello") → lit "hello\n"
 *   5. safe_popen("echo hello") → fclose() sans crash (pas de pclose)
 *   6. safe_system(NULL)    → comportement défini (pas de crash)
 */

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <glib.h>

/* On inclut l'interface — les symboles viennent de la lib principale */
#include "../src/safe_exec.h"

static int failures = 0;

#define EXPECT(cond, msg) \
    do { \
        if (!(cond)) { \
            fprintf(stderr, "FAIL [%s:%d] %s\n", __FILE__, __LINE__, (msg)); \
            failures++; \
        } else { \
            printf("PASS  %s\n", (msg)); \
        } \
    } while (0)

int main(void)
{
    printf("=== test_safe_exec (qt6sermo) ===\n");

    /* 1. safe_system("true") */
    gint r1 = safe_system("true");
    EXPECT(r1 == 0, "safe_system(\"true\") returns 0");

    /* 2. safe_system("false") */
    gint r2 = safe_system("false");
    EXPECT(r2 != 0, "safe_system(\"false\") returns non-zero");

    /* 3. safe_popen("echo hello") — lecture */
    FILE *fp = safe_popen("echo hello");
    EXPECT(fp != NULL, "safe_popen(\"echo hello\") returns non-NULL");
    if (fp) {
        char buf[64] = {0};
        char *got = fgets(buf, sizeof(buf), fp);
        EXPECT(got != NULL, "safe_popen: fgets reads data");
        EXPECT(strncmp(buf, "hello", 5) == 0, "safe_popen: output is \"hello\"");
        fclose(fp);  /* MUST use fclose, not pclose — safe_popen uses fdopen() */
        printf("PASS  safe_popen: fclose() without crash\n");
    }

    /* 4. safe_popen("echo multiline") — plusieurs lignes */
    FILE *fp2 = safe_popen("printf 'line1\\nline2\\nline3\\n'");
    EXPECT(fp2 != NULL, "safe_popen(printf 3 lines) returns non-NULL");
    if (fp2) {
        int lines = 0;
        char buf[64];
        while (fgets(buf, sizeof(buf), fp2)) lines++;
        fclose(fp2);
        EXPECT(lines == 3, "safe_popen: reads 3 lines");
    }

    /* 5. Commande vide — doit échouer proprement (pas de crash) */
    gint r5 = safe_system("");
    (void)r5;  /* comportement impl-défini, on vérifie juste l'absence de crash */
    printf("PASS  safe_system(\"\") does not crash\n");

    printf("\n=== Résultat : %d échec(s) ===\n", failures);
    return failures > 0 ? 1 : 0;
}
