/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright (C) 2026 S. Cage
 * haplo-dialog <devel@haplo-dialog.fr>
 */
/*
 * sermo_stack_pages.h — la pile de pages qui SE SOUVIENT de sa page (FLTK)
 * sermo — haplo-dialog <devel@haplo-dialog.fr>
 * Licence : GPL-2.0-or-later
 *
 * Fl_Wizard montre un enfant à la fois mais ne dit pas LEQUEL : value() rend
 * un pointeur, pas un index. On garde donc l'index.
 *
 * ⛔ EN-TÊTE PARTAGÉ, jamais une classe par fichier : <stack> et <wizard>
 * manipulent le même objet depuis deux unités de compilation. Deux classes de
 * même nom dans deux namespaces anonymes seraient deux TYPES différents, et le
 * dynamic_cast échouerait en silence — la leçon des échelles (2026-09-14).
 */
#ifndef SERMO_STACK_PAGES_H
#define SERMO_STACK_PAGES_H

#include <FL/Fl_Wizard.H>

class SermoPages : public Fl_Wizard {
public:
    SermoPages(int x, int y, int w, int h) : Fl_Wizard(x, y, w, h), page_(0) {}
    void aller_a(int i)
    {
        if (i < 0) i = 0;
        if (i >= children()) i = children() ? children() - 1 : 0;
        page_ = i;
        if (children()) value(child(i));
    }
    int page() const { return page_; }
private:
    int page_;
};

#endif /* SERMO_STACK_PAGES_H */
