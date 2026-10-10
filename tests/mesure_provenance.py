#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-or-later
# Copyright (C) 2026 S. Cage
# haplo-dialog <devel@haplo-dialog.fr>
#
# tests/mesure_provenance.py — le tableau « Provenance » de LICENCES.md, rejouable.
#
# Archive d'amont : gtkdialog-0.8.3.tar.gz (Thunor, Google Code), 571 974 octets,
# SHA-256 ff89d2d7f1e6488e5df5f895716ac1d4198c2467a2a5dc1f51ab408a2faec38e.
#   tar -xzf gtkdialog-0.8.3.tar.gz
#   python3 tests/mesure_provenance.py gtkdialog-0.8.3 . gtk3,gtk4,qt6,sdl3,lvgl,ncurses,efl1,fltk1,html
#
# Méthode : celle du premier tableau (12 septembre 2026),
# retrouvée le 10 octobre 2026 en rejouant cet état du dépôt : 8 lignes sur 8 reproduites.
#   - fichiers : sermo-backend-<port>/src/**/*.{c,cpp,h,hpp}, sous-dossiers compris, sauf le code
#     tiers embarqué (src/imgui de sdl3, et tiers/ de lvgl, hors de src/) ;
#   - lignes significatives : ni vides, ni faites d'accolades/parenthèses/points-virgules,
#     ni commentaires, ni directives du préprocesseur (#include, #define, #if…) ;
#   - « hérité » : au moins la moitié des lignes se retrouvent à l'identique dans le fichier
#     d'amont de même nom (extension ignorée) ;
#   - « réécrit » : un fichier d'amont de même nom existe, moins de la moitié des lignes ;
#   - « neuf » : pas de fichier d'amont de même nom, ou aucune ligne significative.
import os, re, sys
amont, arbre, ports = sys.argv[1], sys.argv[2], sys.argv[3].split(',')
EXT = ('.c', '.cpp', '.h', '.hpp')
def sig(chemin):
    t = open(chemin, encoding='utf-8', errors='replace').read()
    t = re.sub(r'/\*.*?\*/', '', t, flags=re.S)
    out = []
    for l in t.split('\n'):
        l = re.sub(r'//.*$', '', l).strip()
        if not l or re.fullmatch(r'[{}();]+', l) or l.startswith('#'):
            continue
        out.append(l)
    return out
up = {}
for r, _, fs in os.walk(amont):
    for f in fs:
        if f.endswith(EXT):
            up.setdefault(os.path.splitext(f)[0], set()).update(sig(os.path.join(r, f)))
print('| Backend | Hérités | Réécrits | Neufs | Total |')
print('|---|---|---|---|---|')
for p in ports:
    src = os.path.join(arbre, 'sermo-backend-' + p, 'src'); h = rw = n = 0
    fichiers = []
    for r, ds, fs in os.walk(src):
        ds[:] = sorted(d for d in ds if d != 'imgui')
        fichiers += [os.path.join(r, f) for f in sorted(fs) if f.endswith(EXT)]
    for fp in fichiers:
        f = os.path.basename(fp)
        L = sig(fp); stem = os.path.splitext(f)[0]
        fr = (sum(l in up.get(stem, set()) for l in L) / len(L)) if L else 0
        if L and fr >= 0.5: h += 1
        elif L and stem in up: rw += 1
        else: n += 1
    print(f'| `{p}` | {h} | {rw} | {n} | {h + rw + n} |')
