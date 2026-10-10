/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * tag_set_property.c — pont attributs→widget (implémentation indépendante)
 *
 * Copyright (C) 2026 S. Cage
 * haplo-dialog <devel@haplo-dialog.fr>
 */

#include "gtkdialog.h"
#include "attributes.h"
#include "tag_attributes.h"

#include <string.h>

/*
 * Une paire est "tuée" quand son nom a été vidé ailleurs dans le cœur
 * (marquage de suppression sans réallocation du tableau). On l'ignore
 * silencieusement : ce n'est pas un échec, juste une entrée à sauter.
 */
static gboolean
pair_is_dead(const namevalue *pair)
{
    return pair->name == NULL || pair->name[0] == '\0';
}

/*
 * L'attribut "visible" a un porteur dédié ailleurs dans le backend
 * (champ de widget géré par le code de construction). Le dupliquer ici
 * créerait deux sources de vérité concurrentes : on le laisse de côté,
 * sans que ce soit compté comme un échec.
 */
static gboolean
pair_is_reserved(const namevalue *pair)
{
    return strcmp(pair->name, "visible") == 0;
}

gint
widget_set_tag_attributes(GtkWidget *widget, tag_attr *attr)
{
    gint stored;
    gint i;

    if (attr == NULL) {
        return -1;
    }

    if (widget == NULL || !GTK_IS_WIDGET(widget)) {
        return -1;
    }

    stored = 0;

    for (i = 0; i < attr->n; i++) {
        namevalue *pair = &attr->pairs[i];

        if (pair_is_dead(pair)) {
            continue;
        }

        if (pair_is_reserved(pair)) {
            continue;
        }

        g_object_set_data(G_OBJECT(widget), pair->name, g_strdup(pair->value));
        stored++;
    }

    return stored;
}
