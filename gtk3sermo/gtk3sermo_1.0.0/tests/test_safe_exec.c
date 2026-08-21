/*
 * test_safe_exec.c — Tests unitaires pour safe_exec.c
 *
 * Utilise la bibliothèque Check (https://libcheck.github.io/check/).
 * Compilation :
 *   gcc -Wall -Wextra $(pkg-config --cflags --libs check glib-2.0) \
 *       test_safe_exec.c ../src/safe_exec.c -o test_safe_exec
 * Ou via Autotools : make check
 *
 * Haplo-Linux — 2026 — GPL-2.0-or-later
 */

#include <check.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <glib.h>

/* Module under test */
#include "../src/safe_exec.h"

/* =========================================================================
 * Suite 1 — safe_system()
 * ========================================================================= */

/*
 * safe_system("true") doit retourner 0.
 */
START_TEST(test_safe_system_true)
{
    gint ret = safe_system("true");
    ck_assert_int_eq(ret, 0);
}
END_TEST

/*
 * safe_system("false") doit retourner une valeur non nulle (exit status 1).
 */
START_TEST(test_safe_system_false)
{
    gint ret = safe_system("false");
    ck_assert_int_ne(ret, 0);
}
END_TEST

/*
 * safe_system(NULL) doit retourner -1 sans crash.
 */
START_TEST(test_safe_system_null)
{
    gint ret = safe_system(NULL);
    ck_assert_int_eq(ret, -1);
}
END_TEST

/*
 * safe_system("") — chaîne vide — doit retourner -1 sans crash.
 */
START_TEST(test_safe_system_empty)
{
    gint ret = safe_system("");
    ck_assert_int_eq(ret, -1);
}
END_TEST

/*
 * safe_system avec une commande simple sans métacaractères.
 * /bin/echo existe sur tout système POSIX.
 */
START_TEST(test_safe_system_echo)
{
    gint ret = safe_system("/bin/echo test_safe_exec_ok");
    ck_assert_int_eq(ret, 0);
}
END_TEST

/*
 * safe_system avec métacaractères (fallback /bin/sh -c).
 * La commande doit quand même s'exécuter correctement.
 */
START_TEST(test_safe_system_shell_fallback)
{
    /* Le pipe | est un métacaractère → fallback shell */
    gint ret = safe_system("echo hello | cat");
    ck_assert_int_eq(ret, 0);
}
END_TEST

/*
 * safe_system avec une commande introuvable → exit status ≠ 0.
 */
START_TEST(test_safe_system_not_found)
{
    gint ret = safe_system("/nonexistent_binary_gtkdialog_test_42");
    ck_assert_int_ne(ret, 0);
}
END_TEST

/* =========================================================================
 * Suite 2 — safe_popen()
 * ========================================================================= */

/*
 * safe_popen(NULL) doit retourner NULL sans crash.
 */
START_TEST(test_safe_popen_null)
{
    FILE *fp = safe_popen(NULL);
    ck_assert_ptr_null(fp);
}
END_TEST

/*
 * safe_popen("") — chaîne vide — doit retourner NULL.
 */
START_TEST(test_safe_popen_empty)
{
    FILE *fp = safe_popen("");
    ck_assert_ptr_null(fp);
}
END_TEST

/*
 * safe_popen("echo hello") → lit "hello\n" sur la sortie.
 */
START_TEST(test_safe_popen_echo)
{
    FILE *fp = safe_popen("echo hello");
    ck_assert_ptr_nonnull(fp);

    char buf[64] = {0};
    char *line = fgets(buf, sizeof(buf), fp);
    ck_assert_ptr_nonnull(line);

    /* Strip trailing newline */
    buf[strcspn(buf, "\r\n")] = '\0';
    ck_assert_str_eq(buf, "hello");

    fclose(fp);  /* MUST use fclose, not pclose (fdopen FILE*) */
}
END_TEST

/*
 * safe_popen avec commande à deux mots (pas de métacaractère) →
 * doit fonctionner via g_spawn, pas shell fallback.
 */
START_TEST(test_safe_popen_two_args)
{
    FILE *fp = safe_popen("/bin/echo gtkdialog_haplo1");
    ck_assert_ptr_nonnull(fp);

    char buf[64] = {0};
    fgets(buf, sizeof(buf), fp);
    buf[strcspn(buf, "\r\n")] = '\0';
    ck_assert_str_eq(buf, "gtkdialog_haplo1");

    fclose(fp);
}
END_TEST

/*
 * safe_popen avec métacaractère (fallback /bin/sh -c).
 * "echo a; echo b" doit produire "a" sur la première ligne.
 */
START_TEST(test_safe_popen_shell_fallback)
{
    FILE *fp = safe_popen("echo meta_a; echo meta_b");
    ck_assert_ptr_nonnull(fp);

    char buf[64] = {0};
    fgets(buf, sizeof(buf), fp);
    buf[strcspn(buf, "\r\n")] = '\0';
    ck_assert_str_eq(buf, "meta_a");

    fclose(fp);
}
END_TEST

/*
 * Appels répétés — pas de fuite de file descriptors ou zombies.
 * On ouvre et ferme 20 pipes consécutifs.
 */
START_TEST(test_safe_popen_repeated)
{
    for (int i = 0; i < 20; i++) {
        FILE *fp = safe_popen("true");
        ck_assert_ptr_nonnull(fp);
        fclose(fp);
    }
    /* If we reach here without EMFILE, no fd leak occurred */
}
END_TEST

/* =========================================================================
 * Construction de la suite et main()
 * ========================================================================= */

static Suite *safe_exec_suite(void)
{
    Suite *s;
    TCase *tc_system, *tc_popen;

    s = suite_create("safe_exec");

    /* --- safe_system() --- */
    tc_system = tcase_create("safe_system");
    tcase_set_timeout(tc_system, 10);
    tcase_add_test(tc_system, test_safe_system_true);
    tcase_add_test(tc_system, test_safe_system_false);
    tcase_add_test(tc_system, test_safe_system_null);
    tcase_add_test(tc_system, test_safe_system_empty);
    tcase_add_test(tc_system, test_safe_system_echo);
    tcase_add_test(tc_system, test_safe_system_shell_fallback);
    tcase_add_test(tc_system, test_safe_system_not_found);
    suite_add_tcase(s, tc_system);

    /* --- safe_popen() --- */
    tc_popen = tcase_create("safe_popen");
    tcase_set_timeout(tc_popen, 10);
    tcase_add_test(tc_popen, test_safe_popen_null);
    tcase_add_test(tc_popen, test_safe_popen_empty);
    tcase_add_test(tc_popen, test_safe_popen_echo);
    tcase_add_test(tc_popen, test_safe_popen_two_args);
    tcase_add_test(tc_popen, test_safe_popen_shell_fallback);
    tcase_add_test(tc_popen, test_safe_popen_repeated);
    suite_add_tcase(s, tc_popen);

    return s;
}

int main(void)
{
    int      number_failed;
    Suite   *s   = safe_exec_suite();
    SRunner *sr  = srunner_create(s);

    /* TAP output for 'make check' integration */
    srunner_set_tap(sr, "-");

    srunner_run_all(sr, CK_NORMAL);
    number_failed = srunner_ntests_failed(sr);
    srunner_free(sr);

    return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
