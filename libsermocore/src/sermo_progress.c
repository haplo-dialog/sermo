/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * sermo_progress.c — la commande <input> d'une <progressbar>, lue au fil de
 * l'eau. Contrat et raisons : include/sermo_progress.h.
 *
 * Copyright (C) 2026 S. Cage
 * haplo-dialog <devel@haplo-dialog.fr>
 *
 * Pas de fil d'exécution : la commande écrit dans un tube mis en mode non
 * bloquant, et le port vient lire ce qui est arrivé depuis sa propre boucle
 * (minuterie Qt, FLTK, EFL, image ImGui, tour de ncurses). Les actions partent
 * donc toujours du fil principal, comme celles d'un bouton.
 */

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <gtk/gtk.h>
#include "gtk3d.h"
#include "actions.h"
#include "attributes.h"
#include "stringman.h"
#include "widgets.h"
#include "sermo_progress.h"
#include "sermo_input.h"

/* signals.c — pas déclarée dans un en-tête. */
gboolean widget_signal_executor_eval_condition(gchar *condition);

/* L'étalon lit par fgets(ligne, 512, tube) : 511 octets au plus par morceau. */
#define LIGNE_MAX 511

/* Octets lus au plus par appel : une commande bavarde ne doit pas affamer la
 * boucle du toolkit. Le reste attend le prochain passage. */
#define LECTURE_MAX_PAR_POLL (64 * 1024)

struct sermo_progress {
	void                       *widget;
	AttributeSet               *Attr;
	FILE                       *flux;
	int                         fd;
	char                        ligne[LIGNE_MAX + 1];
	size_t                      lg;
	int                         actions_faites;
	int                         en_lecture;
	int                         a_liberer;
	sermo_progress_fraction_cb  on_fraction;
	sermo_progress_text_cb      on_text;
};

static void fermer(sermo_progress *p)
{
	if (p->flux) {
		/* fclose, jamais pclose : le flux vient de fdopen (safe_popen). */
		fclose(p->flux);
		p->flux = NULL;
		p->fd = -1;
	}
}

/* Les actions du signal par défaut de la barre, comme widget_signal_executor
 * le fait pour GTK_IS_PROGRESS_BAR — macro qui vaut 0 hors de GTK, d'où ce
 * chemin direct. */
static void lancer_actions(sermo_progress *p)
{
	GList *el = NULL;
	gchar *commande, *fonction, *signal, *condition;

	for (commande = attributeset_get_first(&el, p->Attr, ATTR_ACTION);
	     commande != NULL;
	     commande = attributeset_get_next(&el, p->Attr, ATTR_ACTION)) {
		fonction = attributeset_get_this_tagattr(&el, p->Attr, ATTR_ACTION, "function");
		if (fonction == NULL)   /* « type » : l'ancien nom, avant gtkdialog 0.8.3 */
			fonction = attributeset_get_this_tagattr(&el, p->Attr, ATTR_ACTION, "type");
		signal = attributeset_get_this_tagattr(&el, p->Attr, ATTR_ACTION, "signal");
		condition = attributeset_get_this_tagattr(&el, p->Attr, ATTR_ACTION, "condition");

		if (signal != NULL && g_ascii_strcasecmp(signal, "time-out") != 0)
			continue;
		if (!widget_signal_executor_eval_condition(condition))
			continue;
		if (execute_action((GtkWidget *)p->widget, commande, fonction) == 2)
			break;   /* « break: » arrête la suite des actions */
	}
}

static void appliquer_ligne(sermo_progress *p)
{
	char   *fin;
	long    valeur;
	double  fraction;

	p->ligne[p->lg] = '\0';
	p->lg = 0;

	valeur = strtol(p->ligne, &fin, 0);
	fraction = valeur / 100.0;
	if (fraction > 1.0) fraction = 1.0;
	if (fraction < 0.0) fraction = 0.0;   /* gtk_progress_bar_set_fraction borne aussi */

	if (valeur != 100)
		p->actions_faites = 0;

	if (fin == p->ligne) {
		if (p->on_text) p->on_text(p->widget, p->ligne);
	} else {
		if (p->on_fraction) p->on_fraction(p->widget, fraction);
	}

	if (valeur == 100 && !p->actions_faites) {
		p->actions_faites = 1;
		lancer_actions(p);
	}
}

sermo_progress *sermo_progress_start(void *widget, void *Attr,
                                     sermo_progress_fraction_cb on_fraction,
                                     sermo_progress_text_cb on_text)
{
	GList          *el = NULL;
	const gchar    *entree, *commande;
	FILE           *flux;
	sermo_progress *p;
	int             drapeaux;

	if (widget == NULL || Attr == NULL ||
	    !attributeset_is_avail((AttributeSet *)Attr, ATTR_INPUT))
		return NULL;

	/* Comme l'étalon : la première <input> seulement, et seulement si c'est
	 * une commande (<input file> n'est pas pris en charge par cette barre). */
	entree = attributeset_get_first(&el, (AttributeSet *)Attr, ATTR_INPUT);
	commande = input_get_shell_command(entree);
	if (commande == NULL || *commande == '\0')
		return NULL;

	/* La barre lit une ligne à la fois et n'accumule rien : elle suit la
	 * commande jusqu'au bout, sans la limite des <input> (sermo_input.h). */
	flux = sermo_input_sans_limite(widget_opencommand(commande));
	if (flux == NULL)
		return NULL;

	p = calloc(1, sizeof *p);
	if (p == NULL) {
		fclose(flux);
		return NULL;
	}
	p->widget = widget;
	p->Attr = (AttributeSet *)Attr;
	p->flux = flux;
	p->fd = fileno(flux);
	p->on_fraction = on_fraction;
	p->on_text = on_text;

	drapeaux = fcntl(p->fd, F_GETFL);
	if (drapeaux == -1 || fcntl(p->fd, F_SETFL, drapeaux | O_NONBLOCK) == -1) {
		/* Un tube bloquant figerait la boucle du toolkit : on refuse. */
		fermer(p);
		free(p);
		return NULL;
	}
	return p;
}

int sermo_progress_poll(sermo_progress *p)
{
	char    tampon[4096];
	size_t  lus = 0;
	int     fini = 0;

	if (p == NULL)
		return 0;

	p->en_lecture = 1;
	while (!fini && !p->a_liberer && lus < LECTURE_MAX_PAR_POLL) {
		ssize_t n = read(p->fd, tampon, sizeof tampon);

		if (n > 0) {
			lus += (size_t)n;
			for (ssize_t i = 0; i < n && !p->a_liberer; i++) {
				if (tampon[i] == '\n') {
					appliquer_ligne(p);
				} else {
					p->ligne[p->lg++] = tampon[i];
					if (p->lg == LIGNE_MAX)
						appliquer_ligne(p);
				}
			}
		} else if (n == 0) {
			/* La commande a fermé sa sortie : son dernier morceau sans
			 * saut de ligne compte, comme pour fgets. */
			if (p->lg > 0 && !p->a_liberer)
				appliquer_ligne(p);
			fini = 1;
		} else if (errno == EINTR) {
			continue;
		} else if (errno == EAGAIN || errno == EWOULDBLOCK) {
			break;   /* rien de plus pour l'instant */
		} else {
			fini = 1;   /* tube inutilisable : ne pas relire en boucle */
		}
	}
	p->en_lecture = 0;

	if (fini || p->a_liberer) {
		fermer(p);
		free(p);
		return 0;
	}
	return 1;
}

void sermo_progress_free(sermo_progress *p)
{
	if (p == NULL)
		return;
	if (p->en_lecture) {
		p->a_liberer = 1;
		return;
	}
	fermer(p);
	free(p);
}
