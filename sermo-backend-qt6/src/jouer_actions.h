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
        if (execute_action(widget, cmd, fonction) == 2) break;
    }
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
