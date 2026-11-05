/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * widget_drawingarea.c:
 * Gtkdialog - A small utility for fast and easy GUI building.
 * Copyright (C) 2026  haplo-dialog <devel@haplo-dialog.fr>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 * GtkDrawingArea — zone de dessin vierge.
 *
 * Syntaxe XML :
 *   <drawingarea width-request="300" height-request="200">
 *     <variable>CANVAS</variable>
 *   </drawingarea>
 *
 * Attributs de tag :
 *   width-request="x"   — largeur souhaitée en pixels
 *   height-request="x"  — hauteur souhaitée en pixels
 *   embed="true"        — exporter l'identifiant X11 de la zone (vidéo)
 *   media="NOM"         — hors X11 (Wayland), afficher les images du lecteur
 *                         <action media> NOM rendu « dans la zone » : la
 *                         variable vaut alors « zone » (avec embed="true")
 *
 * Note : aucun rendu n'est effectué par défaut ; ce widget fournit une
 * surface que des scripts externes ou de futures extensions pourront
 * peindre. Avec embed="true", sous X11, sa variable vaut l'identifiant X11
 * de la zone (une fois affichée) : <action media> y incruste la vidéo
 * (dans=$CANVAS). Sans cet attribut, ou ailleurs (Wayland), elle est vide :
 * un script qui lit la variable à la fermeture garde la valeur d'avant.
 */

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include <gtk/gtk.h>
#ifdef GDK_WINDOWING_X11
#include <gdk/gdkx.h>
#endif
#include "config.h"
#include "gtk3d.h"
#include "attributes.h"
#include "automaton.h"
#include "widgets.h"
#include "signals.h"
#include "tag_attributes.h"
#include "widget_drawingarea.h"
#include "sermo_images.h"
#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

/* ── Images d'un lecteur rendu « dans la zone » (sermo_images.h) ──────────
 * Le fichier vient d'un décodeur non fiable : en-tête borné, lecture par
 * pread (jamais mmap), fichier ordinaire à nous en 600 seulement. */
typedef struct {
	char           chemin[640];
	int            fd;
	dev_t          dev;
	ino_t          ino;
	guint64        sequence;
	guint          largeur, hauteur;
	unsigned char *pixels;
	guint          minuteur;
} ZoneImages;

static void zone_fermer(ZoneImages *z)
{
	if (z->fd >= 0)
		close(z->fd);
	z->fd = -1;
	z->sequence = 0;
}

static void zone_vider(GtkWidget *w, ZoneImages *z)
{
	if (z->pixels) {
		g_free(z->pixels);
		z->pixels = NULL;
		gtk_widget_queue_draw(w);
	}
}

static gboolean zone_tic(gpointer d)
{
	GtkWidget *w = d;
	ZoneImages *z = g_object_get_data(G_OBJECT(w), "_zone");
	struct stat st;
	if (!z)
		return G_SOURCE_REMOVE;
	/* Fichier remplacé (nouvelle taille), effacé (lecteur arrêté) ? */
	if (lstat(z->chemin, &st) != 0 || !S_ISREG(st.st_mode)) {
		zone_fermer(z);
		zone_vider(w, z);
		return G_SOURCE_CONTINUE;
	}
	if (z->fd < 0 || st.st_dev != z->dev || st.st_ino != z->ino) {
		zone_fermer(z);
		z->fd = open(z->chemin, O_RDONLY | O_NOFOLLOW | O_CLOEXEC);
		if (z->fd < 0)
			return G_SOURCE_CONTINUE;
		if (fstat(z->fd, &st) != 0 || st.st_uid != geteuid() || (st.st_mode & 077)) {
			zone_fermer(z);
			return G_SOURCE_CONTINUE;
		}
		z->dev = st.st_dev;
		z->ino = st.st_ino;
	}
	struct sermo_images_entete e;
	if (pread(z->fd, &e, sizeof e, 0) != (ssize_t)sizeof e || memcmp(e.magie, SERMO_IMAGES_MAGIE, 8) != 0
	    || e.version != SERMO_IMAGES_VERSION || e.largeur < 1 || e.largeur > SERMO_IMAGES_LARGEUR_MAX
	    || e.hauteur < 1 || e.hauteur > SERMO_IMAGES_HAUTEUR_MAX || e.pas != e.largeur * 4u || e.courant > 1
	    || e.sequence == 0 || e.sequence == z->sequence)
		return G_SOURCE_CONTINUE;
	size_t taille = (size_t)e.hauteur * e.pas;
	if (fstat(z->fd, &st) != 0 || (size_t)st.st_size < SERMO_IMAGES_DONNEES + 2 * taille)
		return G_SOURCE_CONTINUE;
	if (!z->pixels || e.largeur != z->largeur || e.hauteur != z->hauteur) {
		g_free(z->pixels);
		z->pixels = g_malloc(taille);
		z->largeur = e.largeur;
		z->hauteur = e.hauteur;
	}
	if (pread(z->fd, z->pixels, taille, (off_t)(SERMO_IMAGES_DONNEES + e.courant * taille)) != (ssize_t)taille)
		return G_SOURCE_CONTINUE;
	z->sequence = e.sequence;
	gtk_widget_queue_draw(w);
	return G_SOURCE_CONTINUE;
}

static gboolean zone_dessiner(GtkWidget *w, cairo_t *cr, gpointer d)
{
	ZoneImages *z = g_object_get_data(G_OBJECT(w), "_zone");
	if (!z || !z->pixels)
		return FALSE;
	double lw = gtk_widget_get_allocated_width(w), hw = gtk_widget_get_allocated_height(w);
	double r = MIN(lw / z->largeur, hw / z->hauteur);
	cairo_set_source_rgb(cr, 0, 0, 0);            /* bandes noires autour de l'image */
	cairo_paint(cr);
	cairo_surface_t *s = cairo_image_surface_create_for_data(z->pixels, CAIRO_FORMAT_RGB24,
	                                                         (int)z->largeur, (int)z->hauteur, (int)z->largeur * 4);
	cairo_translate(cr, (lw - z->largeur * r) / 2, (hw - z->hauteur * r) / 2);
	cairo_scale(cr, r, r);
	cairo_set_source_surface(cr, s, 0, 0);
	cairo_paint(cr);
	cairo_surface_destroy(s);
	return TRUE;
}

static void zone_liberer(gpointer d)
{
	ZoneImages *z = d;
	if (z->minuteur)
		g_source_remove(z->minuteur);
	zone_fermer(z);
	g_free(z->pixels);
	g_free(z);
}

/* media="NOM" : le nom d'un lecteur (lettres, chiffres, _ et -, 32 au plus). */
static void zone_installer(GtkWidget *w, const char *nom)
{
	size_t l = strlen(nom);
	const char *run = g_getenv("XDG_RUNTIME_DIR");
	if (l == 0 || l > 32 || strspn(nom, "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789_-") != l) {
		g_warning("<drawingarea media=\"%s\"> : nom de lecteur refusé", nom);
		return;
	}
	if (!run || run[0] != '/')
		return;
	ZoneImages *z = g_new0(ZoneImages, 1);
	z->fd = -1;
	g_snprintf(z->chemin, sizeof z->chemin, "%s/sermo-media/%s/images", run, nom);
	g_object_set_data_full(G_OBJECT(w), "_zone", z, zone_liberer);
	g_signal_connect(w, "draw", G_CALLBACK(zone_dessiner), NULL);
	z->minuteur = g_timeout_add(15, zone_tic, w);   /* ~60 lectures d'en-tête par seconde */
}

//#define DEBUG_CONTENT
//#define DEBUG_TRANSITS

/***********************************************************************
 * Clear                                                               *
 ***********************************************************************/

void widget_drawingarea_clear(variable *var)
{
#ifdef DEBUG_TRANSITS
	fprintf(stderr, "%s(): Entering.\n", __func__);
#endif
	if (var && var->Widget)
		gtk_widget_queue_draw(var->Widget);
#ifdef DEBUG_TRANSITS
	fprintf(stderr, "%s(): Exiting.\n", __func__);
#endif
}

/***********************************************************************
 * Create                                                              *
 ***********************************************************************/

GtkWidget *widget_drawingarea_create(
	AttributeSet *Attr, tag_attr *attr, gint Type)
{
	GtkWidget  *widget;
	gchar      *value;
	gint        width  = -1;
	gint        height = -1;

#ifdef DEBUG_TRANSITS
	fprintf(stderr, "%s(): Entering.\n", __func__);
#endif

	widget = gtk_drawing_area_new();

	if (attr) {
		if ((value = get_tag_attribute(attr, "width-request")))
			width = atoi(value);
		if ((value = get_tag_attribute(attr, "height-request")))
			height = atoi(value);
	}

	if (width > 0 || height > 0)
		gtk_widget_set_size_request(widget, width, height);
	if (attr && (value = get_tag_attribute(attr, "embed")) && strcasecmp(value, "true") == 0)
		g_object_set_data(G_OBJECT(widget), "_embed", GINT_TO_POINTER(1));
	if (attr && (value = get_tag_attribute(attr, "media")))
		zone_installer(widget, value);

#ifdef DEBUG_TRANSITS
	fprintf(stderr, "%s(): Exiting.\n", __func__);
#endif
	return widget;
}

/***********************************************************************
 * Environment Variable All Construct                                  *
 ***********************************************************************/

gchar *widget_drawingarea_envvar_all_construct(variable *var)
{
	return var ? widget_drawingarea_envvar_construct(var->Widget) : g_strdup("");
}

/***********************************************************************
 * Environment Variable Construct                                      *
 ***********************************************************************/

gchar *widget_drawingarea_envvar_construct(GtkWidget *widget)
{
	/* embed="true" : l'identifiant X11 de la zone une fois affichée (voie
	 * --wid) ; hors X11, « zone » si elle sait peindre les images d'un
	 * lecteur (media="NOM") ; sinon rien, comme sans embed. */
	if (!widget || !g_object_get_data(G_OBJECT(widget), "_embed"))
		return g_strdup("");
#ifdef GDK_WINDOWING_X11
	GdkWindow *fenetre = gtk_widget_get_realized(widget) ? gtk_widget_get_window(widget) : NULL;
	if (fenetre && GDK_IS_X11_WINDOW(fenetre))
		return g_strdup_printf("%lu", (unsigned long)GDK_WINDOW_XID(fenetre));
#endif
	if (g_object_get_data(G_OBJECT(widget), "_zone"))
		return g_strdup("zone");
	return g_strdup("");
}

/***********************************************************************
 * Fileselect                                                          *
 ***********************************************************************/

void widget_drawingarea_fileselect(
	variable *var, const char *name, const char *value)
{
	g_warning("%s(): Fileselect not implemented for this widget.", __func__);
}

/***********************************************************************
 * Refresh                                                             *
 ***********************************************************************/

void widget_drawingarea_refresh(variable *var)
{
	GList   *element;
	gint     initialised = FALSE;

#ifdef DEBUG_TRANSITS
	fprintf(stderr, "%s(): Entering.\n", __func__);
#endif

	if (g_object_get_data(G_OBJECT(var->Widget), "_initialised") != NULL)
		initialised = GPOINTER_TO_INT(
			g_object_get_data(G_OBJECT(var->Widget), "_initialised"));

	if (!initialised) {
		if (attributeset_is_avail(var->Attributes, ATTR_LABEL))
			g_warning("%s(): <label> not implemented for this widget.", __func__);
		if ((attributeset_cmp_left(var->Attributes, ATTR_SENSITIVE, "false")) ||
		    (attributeset_cmp_left(var->Attributes, ATTR_SENSITIVE, "disabled")) ||
		    (attributeset_cmp_left(var->Attributes, ATTR_SENSITIVE, "no")) ||
		    (attributeset_cmp_left(var->Attributes, ATTR_SENSITIVE, "0")))
			gtk_widget_set_sensitive(var->Widget, FALSE);
	}

	(void) element;

#ifdef DEBUG_TRANSITS
	fprintf(stderr, "%s(): Exiting.\n", __func__);
#endif
}

/***********************************************************************
 * Removeselected                                                      *
 ***********************************************************************/

void widget_drawingarea_removeselected(variable *var)
{
	g_warning("%s(): Removeselected not implemented for this widget.", __func__);
}

/***********************************************************************
 * Save                                                                *
 ***********************************************************************/

void widget_drawingarea_save(variable *var)
{
	g_warning("%s(): Save not implemented for this widget.", __func__);
}
