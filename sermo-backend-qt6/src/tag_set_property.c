/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * tag_set_property.c — pont attributs→propriétés (implémentation indépendante)
 *
 * Copyright (C) 2026 S. Cage
 *
 * Façade haplo-dialog <devel@haplo-dialog.fr>
 */

#include "gtk3d.h"
#include "attributes.h"
#include "tag_attributes.h"

#include <stdlib.h>
#include <string.h>

/* Déjà déclarée ailleurs dans le backend ; contrôle la journalisation des
 * avertissements émis par ce module. */
extern gboolean option_no_warning;

/*
 * Compare une valeur texte à la chaîne "true", insensible à la casse,
 * sans dépendre d'une fonction de comparaison non listée dans la
 * spécification.
 */
static gboolean
value_is_true(const char *value)
{
    static const char truth[] = "true";
    int i;

    if (value == NULL)
        return FALSE;

    for (i = 0; truth[i] != '\0'; i++) {
        char c = value[i];

        if (c >= 'A' && c <= 'Z')
            c = (char) (c - 'A' + 'a');

        if (c != truth[i])
            return FALSE;
    }

    return value[i] == '\0';
}

/*
 * Traite UNE paire nom/valeur sur un widget déjà validé (non NULL).
 * Renvoie vrai uniquement si une VRAIE propriété du widget a été
 * positionnée via le système de propriétés.
 */
static gboolean
set_one_tag_attribute(GtkWidget *widget, const char *name, const char *value)
{
    void *object;
    GObjectClass *klass;
    GParamSpec *pspec;

    /* La visibilité est gérée par un mécanisme dédié ailleurs dans le
     * backend : ne jamais la faire passer par le système de propriétés
     * ici, ce serait une source de vérité concurrente. */
    if (strcmp(name, "visible") == 0)
        return FALSE;

    object = G_OBJECT(widget);
    klass = G_OBJECT_GET_CLASS(widget);
    /* Sur ce backend, G_OBJECT()/G_OBJECT_GET_CLASS() sont des macros qui
     * n'utilisent pas leur argument tel quel (elles se resolvent en amont) :
     * les deux variables restent posees pour la lisibilite et pour le jour
     * ou une vraie introspection Qt les consommerait. */
    (void)object;
    (void)klass;

    pspec = g_object_class_find_property(klass, name);
    if (pspec == NULL) {
        /* Pas de propriété de ce nom : simple mémorisation générique. */
        g_object_set_data(object, name, g_strdup(value));
        return FALSE;
    }

    if ((pspec->flags & G_PARAM_WRITABLE) == 0) {
        if (!option_no_warning)
            g_warning("widget_set_tag_attributes: propriete \"%s\" en lecture seule, ignoree", name);
        return FALSE;
    }

    switch (pspec->value_type) {
    case G_TYPE_BOOLEAN: {
        gboolean v = value_is_true(value);
        g_object_set(object, name, v, NULL);
        return TRUE;
    }

    case G_TYPE_INT: {
        gint v = atoi(value);
        g_object_set(object, name, v, NULL);
        return TRUE;
    }

    case G_TYPE_FLOAT: {
        gfloat v = (gfloat) g_ascii_strtod(value, NULL);
        g_object_set(object, name, v, NULL);
        return TRUE;
    }

    case G_TYPE_DOUBLE: {
        gdouble v = g_ascii_strtod(value, NULL);
        g_object_set(object, name, v, NULL);
        return TRUE;
    }

    case G_TYPE_UINT: {
        guint v = (guint) strtoul(value, NULL, 0);
        g_object_set(object, name, v, NULL);
        return TRUE;
    }

    case G_TYPE_STRING:
        g_object_set(object, name, value, NULL);
        return TRUE;

    default:
        /*
         * Amélioration demandée : un type de propriété non géré
         * explicitement ne doit JAMAIS être écrit à l'aveugle (l'ancien
         * comportement, une conversion entière par atoi() tentée sans
         * savoir si le type réel l'acceptait, n'est pas reproduit ici).
         * On se contente d'avertir et de renvoyer faux, sans la moindre
         * tentative d'écriture — exactement comme le cas non modifiable
         * ci-dessus.
         */
        if (!option_no_warning)
            g_warning("widget_set_tag_attributes: propriete \"%s\" de type non gere, ignoree", name);
        return FALSE;
    }
}

/*
 * Point de liaison avec le cœur : transmet tous les attributs d'une
 * balise au widget correspondant.
 */
gint
widget_set_tag_attributes(GtkWidget *widget, tag_attr *attr)
{
    gint count;
    int i;

    if (attr == NULL)
        return -1;

    /* GTK_IS_WIDGET() couvre aussi le cas widget == NULL dans ce backend :
     * un attribut orphelin n'est pas une raison de tuer le programme
     * entier, on ne se fie donc pas à un simple assert(). */
    if (!GTK_IS_WIDGET(widget))
        return -1;

    count = 0;

    for (i = 0; i < attr->n; i++) {
        const char *name = attr->pairs[i].name;
        const char *value = attr->pairs[i].value;

        /* Paire « tuée » : nom vide, on l'ignore silencieusement. */
        if (name == NULL || name[0] == '\0')
            continue;

        if (set_one_tag_attribute(widget, name, value))
            count++;
    }

    return count;
}
