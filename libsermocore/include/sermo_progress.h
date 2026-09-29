/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * sermo_progress.h — la commande <input> d'une <progressbar>, lue au fil de
 * l'eau, sans fil d'exécution et sans bloquer la boucle du toolkit.
 *
 * Copyright (C) 2026 S. Cage
 * haplo-dialog <devel@haplo-dialog.fr>
 *
 * POURQUOI DANS LE CŒUR. Jusqu'à la 2.6.8, seuls gtk3 et gtk4 faisaient vivre
 * une barre de progression. qt6 lisait la première ligne de la commande puis
 * fermait le tube ; fltk1, efl1, sdl3 et ncurses la lisaient EN ENTIER avant
 * d'ouvrir la fenêtre. Aucun des cinq ne faisait avancer la barre, et l'action
 * prévue à 100 % ne partait jamais : un dialogue qui s'en remettait à elle pour
 * se fermer restait ouvert. La règle est écrite ici une fois ; chaque port
 * n'a plus qu'à appeler sermo_progress_poll() depuis sa propre boucle.
 *
 * CONTRAT — celui de l'étalon (sermo-backend-gtk3/src/widget_progressbar.c) :
 *  - chaque ligne est appliquée dès qu'elle arrive ; une ligne de plus de
 *    511 octets arrive en morceaux, comme par fgets(…, 512, …) ;
 *  - une ligne qui commence par un nombre (strtol, base 0) règle la barre :
 *    fraction = nombre / 100, bornée à [0, 1] ;
 *  - une ligne qui ne commence pas par un nombre devient le texte de la barre ;
 *    la fraction ne bouge pas ;
 *  - la première ligne qui vaut 100 déclenche les actions du signal par défaut
 *    de la barre, « time-out » (<action> sans signal, ou signal="time-out"),
 *    condition comprise ; une ligne qui ne vaut pas 100 réarme ce déclenchement ;
 *  - la fin de la commande ne déclenche rien.
 */
#ifndef SERMO_PROGRESS_H
#define SERMO_PROGRESS_H

#ifdef __cplusplus
extern "C" {
#endif

typedef struct sermo_progress sermo_progress;

/* Appliquer une fraction (0..1) ou un texte au widget du toolkit. */
typedef void (*sermo_progress_fraction_cb)(void *widget, double fraction);
typedef void (*sermo_progress_text_cb)(void *widget, const char *text);

/* Lance la commande <input> de la barre (widget : l'objet du toolkit, Attr :
 * son AttributeSet). Rend NULL s'il n'y a pas de commande, ou si son lancement
 * est refusé (SERMO_ALLOWED_CMDS, SERMO_NO_SHELL_FALLBACK) : la barre reste
 * alors où elle est. */
sermo_progress *sermo_progress_start(void *widget, void *Attr,
                                     sermo_progress_fraction_cb on_fraction,
                                     sermo_progress_text_cb on_text);

/* Lit, sans attendre, ce que la commande a déjà écrit, et l'applique ligne
 * par ligne. Rend 1 tant qu'elle peut encore écrire. Rend 0 quand elle a fini :
 * la lecture est alors fermée et la mémoire rendue — ne plus utiliser p. */
int sermo_progress_poll(sermo_progress *p);

/* Le widget disparaît avant la fin de la commande : ferme la lecture et rend
 * la mémoire. Appelé pendant sermo_progress_poll (une action a détruit la
 * fenêtre), la libération est différée à la fin de ce poll, qui rend 0.
 * NULL accepté. */
void sermo_progress_free(sermo_progress *p);

#ifdef __cplusplus
}
#endif

#endif /* SERMO_PROGRESS_H */
