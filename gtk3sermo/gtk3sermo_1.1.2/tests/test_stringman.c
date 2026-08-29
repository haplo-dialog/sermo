/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * test_stringman.c — Tests unitaires de stringman.c
 *
 * Ce que ce fichier testait avant, et pourquoi il ne compilait pas
 * -----------------------------------------------------------------
 * La version précédente déclarait elle-même vingt fonctions
 * command_is_*() / command_get_*() et les appelait, avec ce commentaire :
 * « Ces fonctions existent dans stringman.c mais leur déclaration dans
 * stringman.h est dans un bloc commenté ». C'était faux : dans stringman.c,
 * les DÉFINITIONS sont elles aussi dans un bloc commenté — le
 * « Redundant: Not being used », ouvert ligne 76 et refermé ligne 303, hérité
 * de l'amont gtkdialog. Aucune des vingt n'existe dans le binaire.
 *
 * Résultat : `./configure --enable-unit-tests && make check` échouait à
 * l'édition de liens, avec une trentaine de « undefined reference ». L'option
 * était documentée dans COMPILE.md et VERSIONING.md et ne marchait pas.
 *
 * Ce fichier teste maintenant l'API VIVANTE, celle qui a remplacé les vingt :
 * la table de préfixes de stringman.c, lue par command_get_prefix() et
 * command_prefix_get_type().
 *
 * Bibliothèque Check (https://libcheck.github.io/check/).
 *
 * haplo-dialog — 2026 — GPL-2.0-or-later
 */

#include <check.h>
#include <stdlib.h>
#include <string.h>
#include <glib.h>
#include <gtk/gtk.h>

#include "../src/stringman.h"

/*
 * str_default_name() appelle widgets_to_str(), qui vit dans widgets.c et
 * entraînerait tout le reste du programme dans l'édition de liens. On en donne
 * ici une version de test : elle ne sert qu'à rendre str_default_name()
 * éprouvable, et son contenu n'a pas à imiter la vraie table.
 */
char *widgets_to_str(int itype);
char *
widgets_to_str(int itype)
{
	static char tampon[32];
	g_snprintf(tampon, sizeof(tampon), "type%d", itype);
	return tampon;
}

/* =========================================================================
 * Suite 1 — input_is_shell_command / input_get_shell_command
 * ========================================================================= */

START_TEST(test_input_shell_command_reconnu)
{
	ck_assert(input_is_shell_command("command:bash -c echo") == TRUE);
	ck_assert_str_eq(input_get_shell_command("command:bash -c echo"),
	                 "bash -c echo");
}
END_TEST

START_TEST(test_input_shell_command_refuse)
{
	ck_assert(input_is_shell_command("bash -c echo") == FALSE);
	ck_assert(input_is_shell_command("") == FALSE);
	ck_assert(input_is_shell_command(NULL) == FALSE);
	ck_assert_ptr_null((void *) input_get_shell_command("autre chose"));
}
END_TEST

START_TEST(test_input_shell_command_insensible_a_la_casse)
{
	/* strncasecmp : « COMMAND: » doit être reconnu comme « command: ». */
	ck_assert(input_is_shell_command("COMMAND:ls") == TRUE);
	ck_assert(input_is_shell_command("Command:ls") == TRUE);
}
END_TEST

/* =========================================================================
 * Suite 2 — command_prefix_get_type
 *
 * C'est le remplaçant des vingt command_is_*() : au lieu d'une fonction par
 * préfixe, une table et une recherche. Les valeurs doivent correspondre à
 * l'énumération CommandType de stringman.h — si la table et l'énumération
 * se désalignent, chaque action est routée vers la mauvaise.
 * ========================================================================= */

START_TEST(test_prefix_type_correspond_a_l_enumeration)
{
	ck_assert_int_eq(command_prefix_get_type("command"),        CommandShellCommand);
	ck_assert_int_eq(command_prefix_get_type("exit"),           CommandExit);
	ck_assert_int_eq(command_prefix_get_type("closewindow"),    CommandCloseWindow);
	ck_assert_int_eq(command_prefix_get_type("launch"),         CommandLaunch);
	ck_assert_int_eq(command_prefix_get_type("enable"),         CommandEnable);
	ck_assert_int_eq(command_prefix_get_type("disable"),        CommandDisable);
	ck_assert_int_eq(command_prefix_get_type("show"),           CommandShow);
	ck_assert_int_eq(command_prefix_get_type("hide"),           CommandHide);
	ck_assert_int_eq(command_prefix_get_type("refresh"),        CommandRefresh);
	ck_assert_int_eq(command_prefix_get_type("save"),           CommandSave);
	ck_assert_int_eq(command_prefix_get_type("fileselect"),     CommandFileSelect);
	ck_assert_int_eq(command_prefix_get_type("clear"),          CommandClear);
	ck_assert_int_eq(command_prefix_get_type("removeselected"), CommandRemoveSelected);
	ck_assert_int_eq(command_prefix_get_type("insert"),         CommandInsert);
	ck_assert_int_eq(command_prefix_get_type("append"),         CommandAppend);
}
END_TEST

START_TEST(test_prefix_type_insensible_a_la_casse)
{
	ck_assert_int_eq(command_prefix_get_type("EXIT"),    CommandExit);
	ck_assert_int_eq(command_prefix_get_type("Refresh"), CommandRefresh);
}
END_TEST

START_TEST(test_prefix_type_inconnu_retombe_sur_shell)
{
	/* Un préfixe qui n'est pas dans la table est traité comme une commande
	 * shell — c'est le comportement voulu, pas un échec. */
	ck_assert_int_eq(command_prefix_get_type("nexistepas"), CommandShellCommand);
	ck_assert_int_eq(command_prefix_get_type(""),           CommandShellCommand);
}
END_TEST

/* =========================================================================
 * Suite 3 — command_get_prefix
 * ========================================================================= */

START_TEST(test_get_prefix_separe_prefixe_et_commande)
{
	gchar *prefixe = NULL, *commande = NULL;

	command_get_prefix("refresh:MAVARIABLE", &prefixe, &commande);
	ck_assert_str_eq(prefixe, "refresh");
	ck_assert_str_eq(commande, "MAVARIABLE");
	g_free(prefixe); g_free(commande);
}
END_TEST

START_TEST(test_get_prefix_rogne_les_espaces_de_tete)
{
	gchar *prefixe = NULL, *commande = NULL;

	command_get_prefix("exit:   Ok", &prefixe, &commande);
	ck_assert_str_eq(prefixe, "exit");
	ck_assert_str_eq(commande, "Ok");
	g_free(prefixe); g_free(commande);
}
END_TEST

START_TEST(test_get_prefix_sans_separateur_est_une_commande_shell)
{
	gchar *prefixe = NULL, *commande = NULL;

	command_get_prefix("ls -l", &prefixe, &commande);
	ck_assert_str_eq(prefixe, "command");
	ck_assert_str_eq(commande, "ls -l");
	g_free(prefixe); g_free(commande);
}
END_TEST

START_TEST(test_get_prefix_deux_points_dans_une_commande_shell)
{
	/* Le piège : « echo a:b » a bien un « : », mais « echo a » n'est pas un
	 * préfixe connu. La commande entière doit revenir intacte, sinon toute
	 * commande shell contenant un deux-points serait tronquée. */
	gchar *prefixe = NULL, *commande = NULL;

	command_get_prefix("echo a:b", &prefixe, &commande);
	ck_assert_str_eq(prefixe, "command");
	ck_assert_str_eq(commande, "echo a:b");
	g_free(prefixe); g_free(commande);
}
END_TEST

/* =========================================================================
 * Suite 4 — strnatcmp (tri « naturel »)
 * ========================================================================= */

START_TEST(test_strnatcmp_ordonne_les_nombres_par_valeur)
{
	/* Un tri lexicographique mettrait « fichier10 » avant « fichier9 ». */
	ck_assert_int_lt(strnatcmp("fichier9", "fichier10", 1), 0);
	ck_assert_int_lt(strnatcmp("2", "10", 1), 0);
	ck_assert_int_eq(strnatcmp("abc", "abc", 1), 0);
}
END_TEST

START_TEST(test_strnatcmp_sensibilite_a_la_casse)
{
	/* sensitive = 0 : « ABC » et « abc » sont équivalents. */
	ck_assert_int_eq(strnatcmp("ABC", "abc", 0), 0);
	ck_assert_int_ne(strnatcmp("ABC", "abc", 1), 0);
}
END_TEST

/* =========================================================================
 * Suite 5 — linecutter / list_t_free
 * ========================================================================= */

START_TEST(test_linecutter_decoupe_sur_le_separateur)
{
	/* Convention d'appartenance : linecutter() ne recopie PAS son argument,
	 * il le garde tel quel dans line[0], et list_t_free() le libère ensuite.
	 * Tous les appelants réels lui passent donc un g_strdup(). Lui passer un
	 * tampon de pile ferait libérer une adresse que GLib n'a jamais allouée. */
	list_t *l = linecutter(g_strdup("un|deux|trois"), '|');

	ck_assert_ptr_nonnull(l);
	ck_assert_int_eq(l->n_lines, 3);
	ck_assert_str_eq(l->line[0], "un");
	ck_assert_str_eq(l->line[1], "deux");
	ck_assert_str_eq(l->line[2], "trois");
	list_t_free(l);
}
END_TEST

START_TEST(test_linecutter_un_seul_champ)
{
	list_t *l = linecutter(g_strdup("seul"), '|');

	ck_assert_ptr_nonnull(l);
	ck_assert_int_eq(l->n_lines, 1);
	ck_assert_str_eq(l->line[0], "seul");
	list_t_free(l);
}
END_TEST

/* =========================================================================
 * Suite 6 — str_default_name
 * ========================================================================= */

START_TEST(test_str_default_name_est_unique_et_borne)
{
	char *a = str_default_name(7);
	char *b = str_default_name(7);

	ck_assert_ptr_nonnull(a);
	ck_assert_ptr_nonnull(b);
	/* Deux appels ne doivent jamais rendre le même nom : le compteur
	 * interne est ce qui empêche deux widgets de porter le même. */
	ck_assert_str_ne(a, b);
	/* Le tampon fait 64 octets ; le nom doit tenir dedans, terminateur compris. */
	ck_assert_int_lt((int) strlen(a), 64);
	g_free(a);
	g_free(b);
}
END_TEST

/* ========================================================================= */

static Suite *
stringman_suite(void)
{
	Suite   *s = suite_create("stringman");
	TCase   *tc;

	tc = tcase_create("input_shell_command");
	tcase_add_test(tc, test_input_shell_command_reconnu);
	tcase_add_test(tc, test_input_shell_command_refuse);
	tcase_add_test(tc, test_input_shell_command_insensible_a_la_casse);
	suite_add_tcase(s, tc);

	tc = tcase_create("prefix_get_type");
	tcase_add_test(tc, test_prefix_type_correspond_a_l_enumeration);
	tcase_add_test(tc, test_prefix_type_insensible_a_la_casse);
	tcase_add_test(tc, test_prefix_type_inconnu_retombe_sur_shell);
	suite_add_tcase(s, tc);

	tc = tcase_create("get_prefix");
	tcase_add_test(tc, test_get_prefix_separe_prefixe_et_commande);
	tcase_add_test(tc, test_get_prefix_rogne_les_espaces_de_tete);
	tcase_add_test(tc, test_get_prefix_sans_separateur_est_une_commande_shell);
	tcase_add_test(tc, test_get_prefix_deux_points_dans_une_commande_shell);
	suite_add_tcase(s, tc);

	tc = tcase_create("strnatcmp");
	tcase_add_test(tc, test_strnatcmp_ordonne_les_nombres_par_valeur);
	tcase_add_test(tc, test_strnatcmp_sensibilite_a_la_casse);
	suite_add_tcase(s, tc);

	tc = tcase_create("linecutter");
	tcase_add_test(tc, test_linecutter_decoupe_sur_le_separateur);
	tcase_add_test(tc, test_linecutter_un_seul_champ);
	suite_add_tcase(s, tc);

	tc = tcase_create("str_default_name");
	tcase_add_test(tc, test_str_default_name_est_unique_et_borne);
	suite_add_tcase(s, tc);

	return s;
}

int
main(void)
{
	int      echecs;
	SRunner *sr = srunner_create(stringman_suite());

	srunner_run_all(sr, CK_NORMAL);
	echecs = srunner_ntests_failed(sr);
	srunner_free(sr);
	return (echecs == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
