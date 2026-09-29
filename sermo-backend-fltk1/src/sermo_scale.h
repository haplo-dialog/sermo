/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright (C) 2026 S. Cage
 * haplo-dialog <devel@haplo-dialog.fr>
 */
/*
 * sermo_scale.h — l'échelle qui se souvient de ses décimales (FLTK)
 * sermo — haplo-dialog <devel@haplo-dialog.fr>
 * Licence : GPL-2.0-or-later
 *
 * Fl_Valuator::precision() est un POSEUR sans lecteur : sans ce type, l'export
 * ne sait pas avec combien de décimales écrire la valeur, et le port rend
 * « 2,5 » là où l'étalon rend « 2 » (digits=0) ou « 2.50 » (digits=2).
 *
 * ⛔ POURQUOI UN EN-TÊTE PARTAGÉ, ET PAS UNE CLASSE PAR FICHIER : le cœur
 * route <vscale> vers widget_hscale_create (automaton.c) mais son EXPORT vers
 * widget_vscale_envvar_construct. L'objet est donc créé dans une unité de
 * compilation et relu dans une autre. Avec une classe définie dans un
 * namespace anonyme de chaque fichier, ce sont DEUX types distincts portant le
 * même nom : le dynamic_cast échoue, silencieusement, et les décimales sont
 * perdues. Mesuré le 2026-09-14 : <vscale digits="1"><default>3.5</default>
 * ressortait « 4 ».
 */
#ifndef SERMO_SCALE_H
#define SERMO_SCALE_H

#include <FL/Fl_Value_Slider.H>

class SermoScale : public Fl_Value_Slider {
public:
    SermoScale(int x, int y, int w, int h)
        : Fl_Value_Slider(x, y, w, h, nullptr), digits_(0) {}
    void decimales(int d) { digits_ = (d < 0 ? 0 : (d > 15 ? 15 : d)); precision(digits_); }
    int  decimales() const { return digits_; }
private:
    int digits_;
};

#endif /* SERMO_SCALE_H */
