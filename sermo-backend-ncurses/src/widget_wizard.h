/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright (C) 2026 S. Cage
 * haplo-dialog <devel@haplo-dialog.fr>
 */
/*
 * widget_wizard.h — Suite d'étapes
 * sermo — haplo-dialog <devel@haplo-dialog.fr>
 * Licence : GPL-2.0-or-later
 *
 * <wizard> : chaque enfant est une ÉTAPE, et le tag fabrique la navigation —
 * Précédent / Suivant / Terminer. C'est <stack> plus trois boutons.
 *
 * ⚠️ COMPOSÉ, pas emprunté : GTK 4 a retiré GtkAssistant et QWizard est une
 * FENÊTRE, pas un conteneur. Chaque port l'assemble avec SON mécanisme de
 * pages : c'est le seul montage qui tienne sur les sept.
 *
 * « Terminer » joue l'<action> du wizard, s'il en porte une ; il ne ferme rien
 * de lui-même — c'est au script de décider (un <button ok> fait ça très bien).
 *
 * Export : l'index de l'étape courante, comme <stack>.
 */
#ifndef WIDGET_WIZARD_H
#define WIDGET_WIZARD_H

#include "ncurses-compat.h"
#include "dialog_state.h"
#include "widgets.h"

GtkWidget *widget_wizard_create(AttributeSet *Attr, tag_attr *attr, gint Type);
gchar     *widget_wizard_envvar_construct(GtkWidget *widget);
gchar     *widget_wizard_envvar_all_construct(variable *var);
void       widget_wizard_clear(variable *var);
void       widget_wizard_refresh(variable *var);
void       widget_wizard_fileselect(variable *var, const char *name, const char *value);
void       widget_wizard_removeselected(variable *var);
void       widget_wizard_save(variable *var);

#endif /* WIDGET_WIZARD_H */
