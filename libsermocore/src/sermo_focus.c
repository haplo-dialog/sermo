/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * sermo_focus.c — voir sermo_focus.h. C simple : compilé dans les trois
 * variantes du cœur, dont la neutre qui n'a qu'un sous-ensemble de GLib.
 */
#include <ctype.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "sermo_focus.h"

typedef struct {
	char nom[24];
	int  fenetre;
	int  visible, actif;
} Balise;

static Balise *liste;
static int     nb, cap, fenetre;
static char    attr[24];        /* dernier nom d'attribut lu */
static char    attente[24];     /* <visible> ou <sensitive> ouvert, valeur à venir */
static int     dernier_widget = -1;

/* Les balises qui prennent le focus au clavier sous gtk3sermo. */
static const char *FOCALISABLES[] = {
	"entry", "password", "searchentry", "edit", "spinbutton", "button", "checkbox",
	"radiobutton", "togglebutton", "switch", "linkbutton", "colorbutton", "fontbutton",
	"filechooser", "chooser", "comboboxtext", "combobox", "comboboxentry", "calendar",
	"hscale", "vscale", "notebook", "list", "tree", "table", "menubutton", "expander",
	"terminal", NULL
};

static int focalisable(const char *n)
{
	for (int i = 0; FOCALISABLES[i]; i++)
		if (strcmp(n, FOCALISABLES[i]) == 0) return 1;
	return 0;
}

static int faux(const char *v)
{
	return v && (strcmp(v, "false") == 0 || strcmp(v, "no") == 0 || strcmp(v, "0") == 0
	             || strcmp(v, "FALSE") == 0 || strcmp(v, "No") == 0);
}

void sermo_focus_ouvre(const char *t)
{
	char n[24];
	size_t i = 0;
	if (!t || t[0] != '<' || !isalpha((unsigned char)t[1])) return;
	for (t++; *t && (isalnum((unsigned char)*t) || *t == '_') && i < sizeof n - 1; t++)
		n[i++] = *t;
	n[i] = '\0';
	attr[0] = '\0';
	if (strcmp(n, "window") == 0) { fenetre++; return; }
	if (strcmp(n, "visible") == 0 || strcmp(n, "sensitive") == 0) {
		snprintf(attente, sizeof attente, "%s", n);
		return;
	}
	if (nb == cap) {
		int c = cap ? cap * 2 : 64;
		Balise *b = realloc(liste, (size_t)c * sizeof *b);
		if (!b) return;
		liste = b; cap = c;
	}
	snprintf(liste[nb].nom, sizeof liste[nb].nom, "%s", n);
	liste[nb].fenetre = fenetre;
	liste[nb].visible = liste[nb].actif = 1;
	dernier_widget = nb++;
}

void sermo_focus_attribut(const char *nom)
{
	size_t l = nom ? strlen(nom) : 0;
	if (l >= sizeof attr) l = sizeof attr - 1;
	memcpy(attr, nom ? nom : "", l);
	attr[l] = '\0';
}

void sermo_focus_valeur(const char *v)
{
	if (dernier_widget < 0 || !faux(v)) return;
	if (strcmp(attr, "visible") == 0)   liste[dernier_widget].visible = 0;
	if (strcmp(attr, "sensitive") == 0) liste[dernier_widget].actif = 0;
}

void sermo_focus_contenu(const char *texte)
{
	if (!attente[0]) return;
	if (dernier_widget >= 0 && faux(texte)) {
		if (strcmp(attente, "visible") == 0)   liste[dernier_widget].visible = 0;
		if (strcmp(attente, "sensitive") == 0) liste[dernier_widget].actif = 0;
	}
	attente[0] = '\0';
}

int sermo_focus_initial(const char *balise)
{
	static char noms[8][24];
	static int  comptes[8];
	int k = -1, rang = 0, i;
	/* rang de cet appel pour cette balise (0, 1, 2…) */
	for (i = 0; i < 8 && noms[i][0]; i++)
		if (strcmp(noms[i], balise) == 0) { rang = comptes[i]++; k = i; break; }
	if (k < 0 && i < 8) { strncpy(noms[i], balise, 23); comptes[i] = 1; rang = 0; }
	/* la rang-ième occurrence de la balise dans le document */
	int j, vue = 0, moi = -1;
	for (j = 0; j < nb; j++)
		if (strcmp(liste[j].nom, balise) == 0 && vue++ == rang) { moi = j; break; }
	if (moi < 0) return 0;
	/* le premier focalisable, visible et actif, de sa fenêtre */
	for (j = 0; j < nb; j++) {
		if (liste[j].fenetre != liste[moi].fenetre) continue;
		if (focalisable(liste[j].nom) && liste[j].visible && liste[j].actif)
			return j == moi;
	}
	return 0;
}
