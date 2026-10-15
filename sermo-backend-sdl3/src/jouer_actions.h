/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright (C) 2026 S. Cage
 * haplo-dialog <devel@haplo-dialog.fr>
 */
/*
 * jouer_actions.h — joue les <action> d'un widget, avec leur fonction.
 *
 * POURQUOI CE FICHIER. Une action s'écrit de deux façons :
 *     <action>refresh:NOM</action>
 *     <action function="refresh">NOM</action>
 * Le cœur les traite toutes deux, à condition qu'on lui passe la fonction :
 * c'est le troisième argument d'execute_action(). Ce port appelait
 * execute_action(…, NULL) : la seconde écriture perdait sa fonction, et son
 * texte — un nom de widget — partait au shell comme une commande. Un
 * « refresh » ne rafraîchissait rien, un « exit » ne fermait rien (mesuré le
 * 2026-10-04, cas 57 à 59 du banc de comportement).
 *
 * Même règle que le cœur GTK et que le port qt6 :
 *  - la fonction est l'attribut function=, sinon type= ;
 *  - une action qui porte signal= n'est jouée que si ce signal est celui de
 *    l'évènement (signal1 ou signal2) ; signal1 NULL = aucun tri ;
 *  - TOUTES les actions du widget sont jouées, dans l'ordre ; la fonction
 *    « break » (code 2) arrête la suite.
 *
 * Le même fichier est repris tel quel dans chaque port qui en a besoin.
 */
#ifndef SERMO_JOUER_ACTIONS_H
#define SERMO_JOUER_ACTIONS_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "attributes.h"
#include "actions.h"
/* stringman.h de certains ports n'a pas sa garde extern "C" : sans elle, un
 * fichier C++ chercherait command_prefix_get_type() sous un nom C++. */
#ifdef __cplusplus
extern "C" {
#endif
#include "stringman.h"
#ifdef __cplusplus
}
#endif

/* condition="…" d'une <action> (command_is_…, file_is_…, active_is_…,
 * sensitive_is_…, visible_is_…) : évaluée par le cœur, comme sur l'étalon
 * (widget_signal_executor_eval_condition(), signals.c). Ces ports l'ignoraient :
 * l'action jouait toujours. Sans condition : vrai. */
#ifdef __cplusplus
extern "C" {
#endif
gboolean widget_signal_executor_eval_condition(gchar *condition);
#ifdef __cplusplus
}
#endif

static inline int condition_vraie(GList **el, AttributeSet *jeu)
{
    gchar *condition = attributeset_get_this_tagattr(el, jeu, ATTR_ACTION, (gchar *)"condition");
    return condition ? (widget_signal_executor_eval_condition(condition) ? 1 : 0) : 1;
}

static inline void jouer_actions(GtkWidget *widget, AttributeSet *jeu,
                                 const char *signal1, const char *signal2)
{
    GList *el = NULL;
    gchar *cmd;

    if (!jeu) return;
    for (cmd = attributeset_get_first(&el, jeu, ATTR_ACTION); cmd;
         cmd = attributeset_get_next(&el, jeu, ATTR_ACTION)) {
        gchar *fonction, *signal;

        if (!*cmd) continue;
        fonction = attributeset_get_this_tagattr(&el, jeu, ATTR_ACTION, (gchar *)"function");
        if (!fonction)
            fonction = attributeset_get_this_tagattr(&el, jeu, ATTR_ACTION, (gchar *)"type");
        signal = attributeset_get_this_tagattr(&el, jeu, ATTR_ACTION, (gchar *)"signal");
        if (signal && signal1
            && g_ascii_strcasecmp(signal, signal1) != 0
            && !(signal2 && g_ascii_strcasecmp(signal, signal2) == 0))
            continue;
        if (!condition_vraie(&el, jeu)) continue;
        if (execute_action(widget, cmd, fonction) == 2) break;
    }
}

/* Widgets à bascule (case à cocher, bouton radio, bouton à bascule) : l'étalon
 * joue leurs actions au signal « toggled », c'est-à-dire chaque fois qu'ils
 * changent d'état. Une action peut commencer par « if true » ou « if false » :
 * elle ne joue alors que si le widget vient d'être coché, ou décoché
 * (widget_signal_executor() du cœur GTK). Ces ports ne les jouaient pas du tout.
 * actif = l'état APRÈS la bascule ; signal_bascule = le signal du widget. */
static inline void jouer_actions_bascule_sur(GtkWidget *widget, AttributeSet *jeu, int actif,
                                             const char *signal_bascule)
{
    GList *el = NULL;
    gchar *cmd;

    if (!jeu) return;
    for (cmd = attributeset_get_first(&el, jeu, ATTR_ACTION); cmd;
         cmd = attributeset_get_next(&el, jeu, ATTR_ACTION)) {
        gchar *fonction, *signal;
        int joue;

        if (!*cmd) continue;
        fonction = attributeset_get_this_tagattr(&el, jeu, ATTR_ACTION, (gchar *)"function");
        if (!fonction)
            fonction = attributeset_get_this_tagattr(&el, jeu, ATTR_ACTION, (gchar *)"type");
        signal = attributeset_get_this_tagattr(&el, jeu, ATTR_ACTION, (gchar *)"signal");
        if (signal && g_ascii_strcasecmp(signal, signal_bascule) != 0)
            continue;
        /* Le préfixe ET la condition, comme l'étalon depuis qu'il est corrigé. */
        if (g_ascii_strncasecmp(cmd, "if true ", 8) == 0) {
            joue = actif;
            cmd += 8;
        } else if (g_ascii_strncasecmp(cmd, "if false ", 9) == 0) {
            joue = !actif;
            cmd += 9;
        } else
            joue = 1;
        if (!joue || !condition_vraie(&el, jeu)) continue;
        if (execute_action(widget, cmd, fonction) == 2) break;
    }
}

static inline void jouer_actions_bascule(GtkWidget *widget, AttributeSet *jeu, int actif)
{
    jouer_actions_bascule_sur(widget, jeu, actif, "toggled");
}

/* L'interrupteur (<switch>) : même règle, mais son signal est « changed »
 * (GtkSwitch::notify::active, branché sur « changed » par le cœur GTK). */
static inline void jouer_actions_interrupteur(GtkWidget *widget, AttributeSet *jeu, int actif)
{
    jouer_actions_bascule_sur(widget, jeu, actif, "changed");
}

/* Pour les entrées de menu, dont la commande est rangée dans une chaîne puis
 * rejouée plus tard sans le jeu d'attributs : rend « fonction:texte » quand la
 * balise porte une fonction connue du cœur, sinon une copie du texte (une
 * fonction inconnue vaut « commande », comme dans execute_action()).
 * Chaîne allouée par malloc() : à rendre par free(). */
static inline char *action_avec_fonction(GList **el, AttributeSet *jeu, const char *texte)
{
    const char *f;
    size_t n;
    char *r;

    if (!texte) return NULL;
    f = attributeset_get_this_tagattr(el, jeu, ATTR_ACTION, (gchar *)"function");
    if (!f) f = attributeset_get_this_tagattr(el, jeu, ATTR_ACTION, (gchar *)"type");
    if (!f || !*f || command_prefix_get_type(f) == CommandShellCommand)
        return strdup(texte);
    n = strlen(f) + 1 + strlen(texte) + 1;
    r = (char *)malloc(n);
    if (r) snprintf(r, n, "%s:%s", f, texte);
    return r;
}

#endif /* SERMO_JOUER_ACTIONS_H */
