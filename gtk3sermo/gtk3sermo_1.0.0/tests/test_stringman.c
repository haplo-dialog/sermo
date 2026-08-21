/*
 * test_stringman.c — Tests unitaires pour stringman.c (classification de commandes)
 *
 * Teste les fonctions command_is_*() et command_get_*() de stringman.c.
 * Ces fonctions n'utilisent que GLib (pas GTK), mais stringman.c inclut
 * gtk/gtk.h.  Ce fichier de test fournit les stubs minimaux nécessaires
 * pour compiler sans un gtk_init() complet.
 *
 * Utilise la bibliothèque Check (https://libcheck.github.io/check/).
 *
 * IMPORTANT : Ce test est compilé avec -DGTKDIALOG_TESTS pour désactiver
 * les includes GTK dans gtk3d.h (voir tests/Makefile.am).
 * Si cette approche n'est pas disponible, utiliser le script run_tests.sh
 * qui valide les patterns de commandes par expression régulière.
 *
 * Compilation manuelle (alternative) :
 *   gcc -Wall -Wextra -DGTKDIALOG_TESTS \
 *       $(pkg-config --cflags --libs check glib-2.0 gtk+-3.0) \
 *       test_stringman.c ../src/stringman.c \
 *       -I../src -o test_stringman
 *
 * Haplo-Linux — 2026 — GPL-2.0-or-later
 */

#include <check.h>
#include <stdlib.h>
#include <string.h>
#include <glib.h>
#include <gtk/gtk.h>

/* Module under test */
#include "../src/stringman.h"

/*
 * Forward declarations for command_is_* / command_get_* functions.
 * Ces fonctions existent dans stringman.c mais leur déclaration dans
 * stringman.h est dans un bloc commenté (« Redundant: Not being used »).
 * Elles sont en réalité utilisées en interne et testées ici directement.
 */
gboolean     command_is_exit_command       (const gchar *command);
const gchar *command_get_exit_command      (const gchar *command);
gboolean     command_is_refresh_command    (const gchar *command);
const gchar *command_get_refresh_command   (const gchar *command);
gboolean     command_is_closewindow_command(const gchar *command);
gboolean     command_is_enable_command     (const gchar *command);
gboolean     command_is_disable_command    (const gchar *command);
gboolean     command_is_save_command       (const gchar *command);
gboolean     command_is_clear_command      (const gchar *command);

/* =========================================================================
 * Suite 1 — input_is_shell_command / input_get_shell_command
 * ========================================================================= */

/*
 * "bash -c echo hello" → reconnaissance commande shell d'entrée.
 */
START_TEST(test_input_is_shell_command_yes)
{
    ck_assert(input_is_shell_command("bash -c echo hello") == TRUE);
}
END_TEST

START_TEST(test_input_is_shell_command_no)
{
    /* Plain file path — not a shell command */
    ck_assert(input_is_shell_command("/path/to/file.txt") == FALSE);
    ck_assert(input_is_shell_command("") == FALSE);
}
END_TEST

START_TEST(test_input_get_shell_command)
{
    const gchar *cmd = input_get_shell_command("bash -c echo hello");
    ck_assert_ptr_nonnull(cmd);
    /* Should return the part after the prefix */
    ck_assert(strlen(cmd) > 0);
}
END_TEST

/* =========================================================================
 * Suite 2 — command_is_exit_command / command_get_exit_command
 * ========================================================================= */

START_TEST(test_command_is_exit_yes)
{
    ck_assert(command_is_exit_command("exit:0") == TRUE);
    ck_assert(command_is_exit_command("exit:1") == TRUE);
    ck_assert(command_is_exit_command("exit:42") == TRUE);
}
END_TEST

START_TEST(test_command_is_exit_no)
{
    ck_assert(command_is_exit_command("refresh:MY_VAR") == FALSE);
    ck_assert(command_is_exit_command("echo hello") == FALSE);
    ck_assert(command_is_exit_command("") == FALSE);
}
END_TEST

START_TEST(test_command_get_exit)
{
    const gchar *val = command_get_exit_command("exit:42");
    ck_assert_ptr_nonnull(val);
    ck_assert_str_eq(val, "42");
}
END_TEST

/* =========================================================================
 * Suite 3 — command_is_refresh_command / command_get_refresh_command
 * ========================================================================= */

START_TEST(test_command_is_refresh_yes)
{
    ck_assert(command_is_refresh_command("refresh:MY_VAR") == TRUE);
    ck_assert(command_is_refresh_command("refresh:BUTTON_1") == TRUE);
}
END_TEST

START_TEST(test_command_is_refresh_no)
{
    ck_assert(command_is_refresh_command("exit:0") == FALSE);
    ck_assert(command_is_refresh_command("") == FALSE);
}
END_TEST

START_TEST(test_command_get_refresh)
{
    const gchar *val = command_get_refresh_command("refresh:MY_VAR");
    ck_assert_ptr_nonnull(val);
    ck_assert_str_eq(val, "MY_VAR");
}
END_TEST

/* =========================================================================
 * Suite 4 — command_is_closewindow_command
 * ========================================================================= */

START_TEST(test_command_is_closewindow_yes)
{
    ck_assert(command_is_closewindow_command("closewindow:MAIN") == TRUE);
}
END_TEST

START_TEST(test_command_is_closewindow_no)
{
    ck_assert(command_is_closewindow_command("exit:0") == FALSE);
}
END_TEST

/* =========================================================================
 * Suite 5 — command_is_enable/disable_command
 * ========================================================================= */

START_TEST(test_command_is_enable_yes)
{
    ck_assert(command_is_enable_command("enable:MY_BUTTON") == TRUE);
}
END_TEST

START_TEST(test_command_is_disable_yes)
{
    ck_assert(command_is_disable_command("disable:MY_BUTTON") == TRUE);
}
END_TEST

START_TEST(test_command_enable_disable_cross)
{
    /* enable: is not disable: */
    ck_assert(command_is_enable_command("disable:X") == FALSE);
    ck_assert(command_is_disable_command("enable:X") == FALSE);
}
END_TEST

/* =========================================================================
 * Suite 6 — command_is_save_command / command_is_clear_command
 * ========================================================================= */

START_TEST(test_command_is_save_yes)
{
    ck_assert(command_is_save_command("save:MY_WIDGET") == TRUE);
}
END_TEST

START_TEST(test_command_is_clear_yes)
{
    ck_assert(command_is_clear_command("clear:MY_WIDGET") == TRUE);
}
END_TEST

/* =========================================================================
 * Suite 7 — strnatcmp (tri naturel)
 * ========================================================================= */

START_TEST(test_strnatcmp_equal)
{
    ck_assert_int_eq(strnatcmp("abc", "abc", TRUE), 0);
}
END_TEST

START_TEST(test_strnatcmp_natural_order)
{
    /* Tri naturel : "item2" < "item10" */
    gint r = strnatcmp("item2", "item10", TRUE);
    ck_assert_int_lt(r, 0);
}
END_TEST

START_TEST(test_strnatcmp_alpha_order)
{
    gint r = strnatcmp("abc", "abd", TRUE);
    ck_assert_int_lt(r, 0);
    r = strnatcmp("abd", "abc", TRUE);
    ck_assert_int_gt(r, 0);
}
END_TEST

/* =========================================================================
 * Construction de la suite et main()
 * ========================================================================= */

static Suite *stringman_suite(void)
{
    Suite *s;
    TCase *tc_input, *tc_exit, *tc_refresh, *tc_window;
    TCase *tc_enable, *tc_save, *tc_nat;

    s = suite_create("stringman");

    tc_input = tcase_create("input_shell_command");
    tcase_add_test(tc_input, test_input_is_shell_command_yes);
    tcase_add_test(tc_input, test_input_is_shell_command_no);
    tcase_add_test(tc_input, test_input_get_shell_command);
    suite_add_tcase(s, tc_input);

    tc_exit = tcase_create("exit_command");
    tcase_add_test(tc_exit, test_command_is_exit_yes);
    tcase_add_test(tc_exit, test_command_is_exit_no);
    tcase_add_test(tc_exit, test_command_get_exit);
    suite_add_tcase(s, tc_exit);

    tc_refresh = tcase_create("refresh_command");
    tcase_add_test(tc_refresh, test_command_is_refresh_yes);
    tcase_add_test(tc_refresh, test_command_is_refresh_no);
    tcase_add_test(tc_refresh, test_command_get_refresh);
    suite_add_tcase(s, tc_refresh);

    tc_window = tcase_create("closewindow_command");
    tcase_add_test(tc_window, test_command_is_closewindow_yes);
    tcase_add_test(tc_window, test_command_is_closewindow_no);
    suite_add_tcase(s, tc_window);

    tc_enable = tcase_create("enable_disable_command");
    tcase_add_test(tc_enable, test_command_is_enable_yes);
    tcase_add_test(tc_enable, test_command_is_disable_yes);
    tcase_add_test(tc_enable, test_command_enable_disable_cross);
    suite_add_tcase(s, tc_enable);

    tc_save = tcase_create("save_clear_command");
    tcase_add_test(tc_save, test_command_is_save_yes);
    tcase_add_test(tc_save, test_command_is_clear_yes);
    suite_add_tcase(s, tc_save);

    tc_nat = tcase_create("strnatcmp");
    tcase_add_test(tc_nat, test_strnatcmp_equal);
    tcase_add_test(tc_nat, test_strnatcmp_natural_order);
    tcase_add_test(tc_nat, test_strnatcmp_alpha_order);
    suite_add_tcase(s, tc_nat);

    return s;
}

int main(void)
{
    int      number_failed;
    Suite   *s   = stringman_suite();
    SRunner *sr  = srunner_create(s);

    /* TAP output for 'make check' integration */
    srunner_set_tap(sr, "-");

    srunner_run_all(sr, CK_NORMAL);
    number_failed = srunner_ntests_failed(sr);
    srunner_free(sr);

    return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
