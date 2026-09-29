/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright (C) 2026 S. Cage
 * haplo-dialog <devel@haplo-dialog.fr>
 */
/*
 * widget_progressbar.cpp — Barre de progression Qt6 (QProgressBar)
 * sermo — haplo-dialog <devel@haplo-dialog.fr>
 * Licence : GPL-2.0-or-later
 *
 * <progressbar> → QProgressBar (0-100)
 * Export : rien, comme l'étalon gtk3 (un afficheur, pas une saisie)
 */

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include "qt6-compat.h"
#include "gtk3d.h"
#include "attributes.h"
#include "automaton.h"
#include "widgets.h"
#include "tag_attributes.h"
#include "widget_progressbar.h"

#include <QtCore/QTimer>
#include <QtWidgets/QProgressBar>

#include <memory>

#include "sermo_progress.h"

#include <string.h>
#include <stdlib.h>
#include <stdio.h>

/* Un « % » du texte serait lu comme un code de format (%p, %v, %m) :
 * QProgressBar veut « %% » pour afficher le caractère lui-même. */
static QString qt6_barre_format(const char *texte)
{
    return QString::fromUtf8(texte).replace(QLatin1Char('%'), QLatin1String("%%"));
}

static void qt6_barre_fraction(void *widget, double fraction)
{
    static_cast<QProgressBar *>(widget)->setValue((int)(fraction * 100.0 + 0.5));
}

static void qt6_barre_texte(void *widget, const char *texte)
{
    static_cast<QProgressBar *>(widget)->setFormat(qt6_barre_format(texte));
}

GtkWidget *widget_progressbar_create(AttributeSet *Attr, tag_attr *attr, gint Type)
{
    QProgressBar *pb = new QProgressBar();
    pb->setMinimum(0);
    pb->setMaximum(100);
    pb->setValue(0);

    if (Attr) {
        GList *element = NULL;
        const char *val = NULL;
        if (attr) val = get_tag_attribute(attr, "value");
        if (!val) val = attributeset_get_first(&element, Attr, ATTR_DEFAULT);
        if (val) pb->setValue((int)g_ascii_strtod(val, NULL));

        element = NULL;
        gchar *txt = attributeset_get_first(&element, Attr, ATTR_LABEL);
        if (txt && *txt) pb->setFormat(qt6_barre_format(txt));
    }

    if (attr) {
        const char *v;
        if ((v = get_tag_attribute(attr, "width-request")))  pb->setMinimumWidth(atoi(v));
        if ((v = get_tag_attribute(attr, "height-request"))) pb->setMinimumHeight(atoi(v));
    }

    /* La commande <input> est lue AU FIL DE L'EAU par le cœur
     * (sermo_progress.h) : chaque ligne fait avancer la barre ou change son
     * texte, et la première qui vaut 100 déclenche les actions. Jusqu'à la
     * 2.6.8, ce port lisait la première ligne puis fermait le tube : la barre
     * ne bougeait plus et l'action de fin ne partait jamais. */
    sermo_progress *lecture = sermo_progress_start(pb, Attr, qt6_barre_fraction, qt6_barre_texte);
    if (lecture) {
        /* Partagé entre la minuterie et la destruction du widget : quand
         * l'un a rendu la lecture, l'autre ne doit plus y toucher. */
        auto tient = std::make_shared<sermo_progress *>(lecture);
        QTimer *releve = new QTimer(pb);   /* détruite avec la barre */
        releve->setInterval(40);
        QObject::connect(releve, &QTimer::timeout, [releve, tient]() {
            if (*tient && !sermo_progress_poll(*tient)) {
                *tient = nullptr;
                releve->stop();
            }
        });
        QObject::connect(pb, &QObject::destroyed, [tient]() {
            sermo_progress_free(*tient);
            *tient = nullptr;
        });
        releve->start();
    }

    return (GtkWidget *)pb;
}

gchar *widget_progressbar_envvar_construct(GtkWidget *widget)
{
    /* L'étalon gtk3 n'exporte RIEN pour une barre de progression : c'est un
     * afficheur, pas une saisie. */
    (void) widget;
    return g_strdup("");
}

gchar *widget_progressbar_envvar_all_construct(variable *var)
{
    if (!var || !var->Widget) return NULL;
    return widget_progressbar_envvar_construct(var->Widget);
}

void widget_progressbar_clear(variable *var)
{
    if (!var || !var->Widget) return;
    static_cast<QProgressBar *>(var->Widget)->setValue(0);
}

void widget_progressbar_refresh(variable *var)
{
    if (!var || !var->Widget) return;
    static_cast<QWidget *>(var->Widget)->update();
}

void widget_progressbar_fileselect(variable *var, const char *n, const char *v) {}
void widget_progressbar_removeselected(variable *var) {}
void widget_progressbar_save(variable *var) {}
