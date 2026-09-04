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

    /* Contrat de l'ORACLE (gtk3sermo, widget_timer.c:222-241), mesuré le
     * 2026-09-04 sur les binaires installés :
     *   - « milliseconds » est un BOOLÉEN (true/yes/1) : levé, « interval »
     *     compte en millisecondes (défaut 1000) ; sinon en SECONDES ENTIÈRES
     *     (atoi, défaut 1). Une première version le lisait comme la VALEUR en
     *     ms — strtod("true") vaut 0, la minuterie partait immédiatement.
     *   - « interval » passe par atoi : interval="1.5" vaut 1 chez l'oracle.
     *     Honorer la décimale (version 1.0.1) était une INFIDÉLITÉ : le même
     *     script ne durait pas pareil selon le port. atoi est sans danger de
     *     locale pour des entiers. */
    gboolean en_ms = FALSE;
    long n = 1;                                   /* défaut oracle : 1 seconde */
    if (attr) {
        const char *v;
        if ((v = get_tag_attribute(attr, "milliseconds")) &&
            (g_ascii_strcasecmp(v, "true") == 0 ||
             g_ascii_strcasecmp(v, "yes") == 0 || atoi(v) == 1)) {
            en_ms = TRUE;
            n = 1000;                             /* défaut oracle en mode ms */
        }
        if ((v = get_tag_attribute(attr, "interval")))
            n = atoi(v);
    }
    double interval_ms = en_ms ? (double)n : (double)n * 1000.0;

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
