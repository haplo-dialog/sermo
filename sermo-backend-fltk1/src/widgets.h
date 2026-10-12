/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * widgets.h — Dispatch des widgets FLTK
 * sermo — haplo-dialog <devel@haplo-dialog.fr>
 * Licence : GPL-2.0-or-later
 *
 * Adapté de gtk3d/widgets.h (László Pere, Thunor)
 */

#ifndef WIDGETS_H
#define WIDGETS_H

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
/* FLTK port: pas de <gtk/gtk.h> — fltk-compat.h est force-inclus */
#include "gtkdialog.h"
#include "stack.h"
#include "attributes.h"
#include "stringman.h"
#include "variables.h"
#include "automaton.h"

/* Listes de visibilité différée — définies dans widgets.cpp (C++ linkage) */
extern GList *widget_show_list;
extern GList *widget_hide_list;

#ifdef __cplusplus
extern "C" {
#endif

extern int SERMO_FLEX_TAG;   /* user_data() des conteneurs Fl_Flex du port */
char    *widget_get_text_value(GtkWidget *widget, int type);
FILE    *widget_opencommand(const char *command);
gchar   *widget_input_text(AttributeSet *Attr);
/* Lignes du texte de widget_input_text(), sans CR/LF ; g_strfreev ; NULL si rien. */
gchar  **widget_input_lines(AttributeSet *Attr);
char    *widgets_to_str(int itype);
gboolean widget_connect_signals(GtkWidget *widget, AttributeSet *Attr);
void     widget_visibility_list_add(GtkWidget *widget, tag_attr *attr);
void     widget_show_all(void);


#ifdef __cplusplus
}
bool sermo_widget_expands(Fl_Widget *w);   /* C++ seulement */
bool sermo_widget_noexpand(Fl_Widget *w);
/* space-expand / space-fill déclarés sur la balise du widget :
 * 1 = vrai, 0 = faux, -1 = non dit */
int  sermo_widget_expand_rule(Fl_Widget *w);
int  sermo_widget_fill_rule(Fl_Widget *w);

#include <map>

/* Boîte <vbox>/<hbox> du port : un Fl_Flex dont la RÉPARTITION suit celle de
 * gtk_box (étalon gtk3) au lieu de celle de Fl_Flex.
 *   - chaque enfant a une taille naturelle sur l'axe (retenue ici :
 *     Fl_Flex::fixed(w) ne rend qu'un booléen ; l'expander la remet à jour
 *     par natural()), et une règle (expand, fill) ;
 *   - le surplus se partage entre les enfants « expand », en plus de leur
 *     taille naturelle ; homogeneous = cases égales sur toute la longueur ;
 *   - un enfant sans fill est CENTRÉ à sa taille naturelle dans sa case ;
 *   - rangée rangée par la fin (pack_end) : sans enfant étiré, le groupe se
 *     cale à droite ;
 *   - sur l'axe croisé, une feuille qui n'a pas à grandir (barre, curseur,
 *     champ, étiquette) garde sa hauteur propre, centrée.
 * Fl_Flex::layout() n'est pas virtuel : passer par resize() (virtuel). */
class SermoFlex : public Fl_Flex {
public:
    SermoFlex(int X, int Y, int W, int H, int dir) : Fl_Flex(X, Y, W, H, dir) {}
    void pack_end(bool v) { pack_end_ = v; }
    void homogeneous(bool v) { homog_ = v; }
    bool homogeneous() const { return homog_; }
    /* à appeler APRÈS add(c) ; nat = taille naturelle sur l'axe,
     * cross = taille naturelle sur l'axe croisé, cross_fill = occupe
     * toute l'épaisseur de la boîte */
    void child_rule(Fl_Widget *c, bool expand, bool fill, int nat,
                    int cross, bool cross_fill);
    void natural(Fl_Widget *c, int nat);   /* nouvelle taille naturelle */
    int  natural_w() const { return nat_w_; }
    int  natural_h() const;   /* hauteur naturelle (règles en cours) */
    void natural_w(int v) { nat_w_ = v; }
    void resize(int X, int Y, int W, int H) FL_OVERRIDE;
protected:
    void draw() FL_OVERRIDE;
    void on_remove(int index) FL_OVERRIDE;
private:
    struct Rule { bool expand, fill, cross_fill; int nat, cross; };
    std::map<Fl_Widget *, Rule> rules_;
    bool pack_end_ = false, homog_ = false;
    int  nat_w_ = 0;
    void place();
};

/* largeur naturelle d'un widget (boîte du port = sa largeur naturelle) */
int  sermo_natural_w(Fl_Widget *w);
/* un widget peut-il s'étirer sur la hauteur sans être dessiné de travers ?
 * (conteneurs, zones, boutons simples : oui ; barre, curseur, champ : non) */
bool sermo_can_grow_v(Fl_Widget *w);
/* attribut booléen de boîte lu comme gtk3 (true/yes/1 = 1, autre = 0),
 * dflt s'il est absent */
int  sermo_attr_rule(tag_attr *attr, const char *name, int dflt);
#endif

#endif /* WIDGETS_H */
