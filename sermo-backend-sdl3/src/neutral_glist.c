/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright (C) 2026 S. Cage
 * haplo-dialog <devel@haplo-dialog.fr>
 */
/* neutral_glist.c — g_list_append EXTERNE pour libsermocore (variante neutre).
 *
 * Le coeur (compile avec sermocore-shim.h) reference g_list_append comme un
 * symbole EXTERNE. Le shim du port (sdl3-compat.h) ne fournit qu'un
 * « static inline » : aucun symbole exporte. Cette unite, compilee SANS le
 * shim du port, fournit l'implementation externe attendue au lien.
 *
 * La disposition de GList ({data,next,prev}) est identique des deux cotes
 * (shim du coeur et sdl3-compat.h), donc l'echange de pointeurs est sur.
 */
#include <stdlib.h>

typedef struct _GList {
    void          *data;
    struct _GList *next;
    struct _GList *prev;
} GList;

GList *g_list_append(GList *list, void *data)
{
    GList *node = (GList *)malloc(sizeof(GList));
    node->data = data;
    node->next = NULL;
    node->prev = NULL;
    if (!list)
        return node;
    GList *last = list;
    while (last->next)
        last = last->next;
    last->next = node;
    node->prev = last;
    return list;
}
