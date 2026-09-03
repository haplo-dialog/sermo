/*
 * widget_timer.cpp — Minuterie Qt6 (QTimer + QLabel invisible)
 * qt6sermo 1.0.0 — haplo-dialog <devel@haplo-dialog.fr>
 * Licence : GPL-2.0-or-later
 *
 * <timer> → QLabel invisible + QTimer (répétitif)
 * Attributs : interval (secondes, float), milliseconds (ms), function (action)
 * Export : nombre de ticks écoulés
 *
 * Le QTimer est stocké comme QObject enfant du QLabel pour partager
 * la durée de vie. Le compteur de ticks est stocké dans une propriété.
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
#include "widget_timer.h"
#include "actions.h"
#include "safe_exec.h"

#include <QtWidgets/QLabel>
#include <QtCore/QTimer>
#include <QtCore/QVariant>

#include <string.h>
#include <stdlib.h>
#include <stdio.h>

/* Callback slot — utilise une lambda via QTimer::connect */

GtkWidget *widget_timer_create(AttributeSet *Attr, tag_attr *attr, gint Type)
{
    QLabel *placeholder = new QLabel();
    placeholder->hide();
    placeholder->setFixedSize(0, 0);

    /* ⚠️ PAS atof() : gtkdialog.c appelle setlocale(LC_ALL, ""), donc sous une
     * locale française atof("2.5") rend 2 — la partie décimale disparaît EN
     * SILENCE. Mesuré le 2026-09-03 : interval="2.5" déclenchait à 2000 ms sous
     * fr_FR et à 2500 ms sous C. Les ports GTK n'ont pas ce défaut : ils
     * n'utilisent jamais atof. g_ascii_strtod lit toujours le point décimal,
     * quelle que soit la locale. Corrigé partout dans le port (24 appels).
     */
    double interval_ms = 1000.0;

    if (attr) {
        const char *v;
        if ((v = get_tag_attribute(attr, "interval")))
            interval_ms = g_ascii_strtod(v, NULL) * 1000.0;
        if ((v = get_tag_attribute(attr, "milliseconds")))
            interval_ms = g_ascii_strtod(v, NULL);
    }

    placeholder->setProperty("timerTicks", (qlonglong)0);

    QTimer *timer = new QTimer(placeholder);
    timer->setInterval((int)interval_ms);

    /* Au tick : dispatcher les <action> via execute_action() (préfixes
     * EXIT:/Command:/… et type="exit"), comme widget_button — pas
     * safe_system(), qui traiterait « EXIT:ok » comme une commande shell. */
    AttributeSet *acap = Attr;
    QObject::connect(timer, &QTimer::timeout, [placeholder, acap]() {
        qlonglong ticks = placeholder->property("timerTicks").toLongLong();
        ticks++;
        placeholder->setProperty("timerTicks", ticks);
        if (!acap) return;
        GList *ae = nullptr;
        gchar *cmd = attributeset_get_first(&ae, acap, ATTR_ACTION);
        while (cmd) {
            gchar *function = attributeset_get_this_tagattr(&ae, acap, ATTR_ACTION, (gchar *)"function");
            if (!function)
                function = attributeset_get_this_tagattr(&ae, acap, ATTR_ACTION, (gchar *)"type");
            gchar *signal = attributeset_get_this_tagattr(&ae, acap, ATTR_ACTION, (gchar *)"signal");
            if (!signal || g_ascii_strcasecmp(signal, "tick") == 0)
                execute_action(nullptr, cmd, function);
            cmd = attributeset_get_next(&ae, acap, ATTR_ACTION);
        }
    });

    timer->start();
    placeholder->setProperty("timerPtr", QVariant::fromValue((QObject*)timer));

    return (GtkWidget *)placeholder;
}

gchar *widget_timer_envvar_construct(GtkWidget *widget)
{
    /* Parité gtk3sermo : la variable d'un <timer> rend "true"/"false" selon que
     * le widget est sensible (activé), PAS un compteur de ticks. Les actions
     * disable:/enable: basculent cet état. Voir gtk3sermo widget_timer.c. */
    QWidget *w = static_cast<QWidget *>(widget);
    if (!w) return g_strdup("false");
    return g_strdup(w->isEnabled() ? "true" : "false");
}

gchar *widget_timer_envvar_all_construct(variable *var)
{
    if (!var || !var->Widget) return NULL;
    return widget_timer_envvar_construct(var->Widget);
}

void widget_timer_clear(variable *var)
{
    if (!var || !var->Widget) return;
    QLabel *lbl = static_cast<QLabel *>(var->Widget);
    lbl->setProperty("timerTicks", (qlonglong)0);
    QObject *obj   = lbl->property("timerPtr").value<QObject *>();
    QTimer  *timer = qobject_cast<QTimer *>(obj);
    if (timer) timer->stop();
}

void widget_timer_refresh(variable *var) {}
void widget_timer_fileselect(variable *var, const char *n, const char *v) {}
void widget_timer_removeselected(variable *var) {}
void widget_timer_save(variable *var) {}
