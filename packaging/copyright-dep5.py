#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-or-later
# packaging/copyright-dep5.py — génère debian/copyright (DEP-5) depuis l'arbre SUIVI par git :
# listes de fichiers exactes, pas de motif qui prétendrait plus qu'il ne couvre.
# À relancer quand des fichiers entrent ou sortent (nouveaux cas CC0, sources
# reprises de l'amont…) ; lintian ne voit pas un bloc devenu faux.
# Usage : python3 packaging/copyright-dep5.py   (depuis n'importe où dans le dépôt)
import re, subprocess, sys
R = subprocess.run(['git','rev-parse','--show-toplevel'],capture_output=True,check=True,text=True).stdout.strip()
def git_ls():
    return subprocess.run(['git','-C',R,'ls-files','-z'],capture_output=True,check=True).stdout.decode().split('\0')[:-1]
FICHIERS = git_ls()
def lire(f):
    try:
        return open(f'{R}/{f}','rb').read()
    except OSError:
        return b''
AMONT = re.compile(rb'Laszlo Pere|L\xc3\xa1szl\xc3\xb3 Pere|L\xe1szl\xf3 Pere|Pere L\xc3\xa1szl\xc3\xb3|Pere L\xe1szl\xf3|Pere Laszlo|Thunor')
def sources_amont():
    garde = []
    for f in FICHIERS:
        if not (f.startswith('libsermocore/') or re.match(r'sermo-backend-[a-z0-9]+/src/', f) or f in ('doc/sermo.1.in','doc/sermo.texi.in')):
            continue
        if f == 'sermo-backend-gtk3/src/widget_window.c':
            continue
        if AMONT.search(lire(f)):
            garde.append(f)
    return sorted(garde)
SPDX = re.compile(rb'SPDX-License-Identifier:\s*([A-Za-z0-9.+-]+)')
def cc0():
    # La PREMIÈRE déclaration SPDX du fichier fait foi : un fichier qui cite
    # l'identifiant CC0 plus bas (ce générateur-ci) n'est pas CC0 pour autant.
    def licence(f):
        m = SPDX.search(lire(f)[:4096])
        return m.group(1) if m else None
    return sorted(f for f in FICHIERS if licence(f) == b'CC0-1.0')
def existe(motif):
    rx = re.compile('^' + re.escape(motif).replace(r'\*', '.*').replace(r'\?', '.') + '$')
    return any(rx.match(f) for f in FICHIERS)
def bloc(fichiers, copyright, licence, commentaire=None):
    for m in fichiers:
        if m != 'debian/*' and not existe(m):
            sys.exit(f'motif sans fichier : {m}')
    lignes = ['Files: ' + '\n       '.join(fichiers), 'Copyright: ' + '\n           '.join(copyright), 'License: ' + licence]
    if commentaire:
        lignes.append('Comment: ' + '\n '.join(commentaire))
    return '\n'.join(lignes) + '\n'

CAGE = '2026 S. Cage <devel@haplo-dialog.fr>'
LASZLO = '2003-2007 László Pere <pipas@linux.pte.hu>'
THUNOR_CODE = '2011-2012 Thunor <thunorsif@hotmail.com>'
THUNOR_EX = '2011-2013 Thunor <thunorsif@hotmail.com>'

sortie = ['''Format: https://www.debian.org/doc/packaging-manuals/copyright-format/1.0/
Upstream-Name: sermo
Upstream-Contact: s.cage <devel@haplo-dialog.fr>
Source: https://gitlab.com/haplo-dialog/sermo
Comment: sermo descend de gtkdialog 0.8.3, écrit par László Pere (2003-2007) et
 repris par Thunor (2011-2012), sous GPL-2+. Les fichiers repris de l'amont
 conservent leur copyright d'origine ; la provenance mesurée, port par port, est
 dans LICENCES.md. Ce fichier est produit depuis l'arbre suivi par git : chaque
 liste de fichiers est exacte.
''']
sortie.append(bloc(['*'], [CAGE], 'GPL-2+'))
sortie.append(bloc(sources_amont(), [LASZLO, THUNOR_CODE, CAGE], 'GPL-2+',
    ['Fichiers du cœur, des backends et de la documentation qui portent la notice de',
     'gtkdialog : lexer et parser, en-têtes du cœur, widgets GTK portés vers GTK 3',
     'et GTK 4, manuels.']))
sortie.append(bloc(['sermo-backend-gtk3/src/widget_window.c'],
    [LASZLO, THUNOR_CODE, '2021 Dima Krasner <dima@dimakrasner.com>', '2021-2024 Mick Amadio <01micko@gmail.com>', CAGE], 'GPL-2+',
    ["L'ancrage Wayland (layer-shell) vient de la lignée BunsenLabs / Puppy Linux."]))
sortie.append(bloc(['examples/*', 'sermoman-mcp/data/examples/*'], [LASZLO, THUNOR_EX, CAGE], 'GPL-2+',
    ['188 des 213 fichiers de examples/ existent dans gtkdialog 0.8.3 (73 à',
     "l'identique) ; pfeme, pfontview et playmusic sont des applications de Thunor.",
     'sermoman-mcp/data/examples/ en garde des copies.']))
sortie.append(bloc(cc0(), [CAGE], 'CC0-1.0',
    ['Cas de test XML, démos qt6 et exemples écrits pour sermo, qui le déclarent',
     'dans leur en-tête SPDX.']))
sortie.append(bloc(['sermo-backend-qt6/data/fr.haplo_dialog.qt6sermo.metainfo.xml'], [CAGE], 'FSFAP',
    ['Métadonnées AppStream : AppStream demande une licence permissive pour elles,',
     'déclarée dans le fichier (metadata_license).']))
sortie.append(bloc(['examples/button/no.svg', 'examples/button/yes.svg', 'examples/togglebutton/false.svg', 'examples/togglebutton/true.svg',
                    'sermoman-mcp/data/examples/togglebutton/false.svg', 'sermoman-mcp/data/examples/togglebutton/true.svg',
                    'examples/pfeme/default.png', 'examples/pfeme/pfeme48.png'],
    ["les auteurs du jeu d'icônes elementary"], 'GPL-2',
    ["Icônes reprises de gtkdialog ; licence jointe à côté d'elles",
     '(COPYING-elementary-icons). Attribution : examples/pfeme/ChangeLog.']))
sortie.append(bloc(['examples/pfeme/profile.png', 'examples/pfeme/profileselected.png', 'examples/pfeme/profileselectedvisible.png', 'examples/pfeme/profilevisible.png'],
    ["les auteurs du jeu d'icônes Nuvola", "les auteurs du jeu d'icônes Fast Forward", THUNOR_EX], 'GPL-2 and LGPL-2.1',
    ['Images de boutons composées par Thunor : manette du jeu Nuvola (LGPL-2.1),',
     'coche du jeu Fast Forward (GPL-2) — examples/pfeme/ChangeLog, 30 décembre 2012.',
     'Licences jointes : examples/pfeme/COPYING-nuvola-icons et',
     'examples/pfeme/COPYING-fast-forward-icons.']))
sortie.append(bloc(['contract/*'], [CAGE], 'Expat',
    ['Le contrat de frontière entre le cœur et les backends est volontairement',
     'sous licence permissive.']))
sortie.append(bloc(['sermo-backend-sdl3/src/imgui/*'], ['2014-2026 Omar Cornut'], 'Expat',
    ['Dear ImGui 1.92.9 WIP, compilé dans sdl3sermo.']))
sortie.append(bloc(['sermo-backend-sdl3/src/imgui/imgui_draw.cpp'],
    ['2014-2026 Omar Cornut', '2004-2005, 2019, 2023 Tristan Grimmer', '2026 Disco Hello',
     'Agency for Cultural Affairs, Government of Japan', 'Ministry of Justice, Government of Japan'],
    'Expat and CC-BY-4.0',
    ['Contient les polices ProggyClean (Tristan Grimmer) et ProggyForever (Disco',
     'Hello, Tristan Grimmer), sous licence MIT, et une table des points de code',
     'des kanji Joyo et Jinmeiyo tirée des listes officielles de l\'Agence pour',
     'les affaires culturelles et du ministère de la Justice du Japon, sous CC BY',
     '4.0 (table générée par https://github.com/vaiorabbit/everyday_use_kanji).']))
sortie.append(bloc(['sermo-backend-sdl3/src/imgui/imstb_rectpack.h', 'sermo-backend-sdl3/src/imgui/imstb_textedit.h', 'sermo-backend-sdl3/src/imgui/imstb_truetype.h'],
    ['2017 Sean Barrett'], 'Expat or public-domain',
    ['Bibliothèques stb modifiées par Dear ImGui ; au choix, MIT ou domaine public',
     '(fin de chaque fichier).']))
sortie.append(bloc(['sermo-backend-sdl3/src/imgui/backends/imgui_impl_opengl3_loader.h'],
    ['2013-2020 The Khronos Group Inc.', '2014-2026 Omar Cornut'], 'public-domain and Expat',
    ['Chargeur OpenGL produit par gl3w (domaine public) ; définitions reprises des',
     'en-têtes Khronos (MIT).']))
sortie.append(bloc(['debian/*'], [CAGE], 'GPL-2+'))

sortie.append('''License: GPL-2+
 This program is free software; you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation; either version 2 of the License, or
 (at your option) any later version.
 .
 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.
 .
 You should have received a copy of the GNU General Public License
 along with this program. If not, see <https://www.gnu.org/licenses/>.
 .
 On Debian systems, the complete text of the GNU General Public License
 version 2 can be found in "/usr/share/common-licenses/GPL-2".
''')
sortie.append('''License: GPL-2
 This program is free software; you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation; version 2 of the License.
 .
 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.
 .
 On Debian systems, the complete text of the GNU General Public License
 version 2 can be found in "/usr/share/common-licenses/GPL-2".
''')
sortie.append('''License: LGPL-2.1
 This library is free software; you can redistribute it and/or
 modify it under the terms of the GNU Lesser General Public
 License as published by the Free Software Foundation; version 2.1
 of the License.
 .
 This library is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 Lesser General Public License for more details.
 .
 On Debian systems, the complete text of the GNU Lesser General Public
 License version 2.1 can be found in "/usr/share/common-licenses/LGPL-2.1".
''')
sortie.append('''License: Expat
 Permission is hereby granted, free of charge, to any person obtaining a copy
 of this software and associated documentation files (the "Software"), to deal
 in the Software without restriction, including without limitation the rights
 to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 copies of the Software, and to permit persons to whom the Software is
 furnished to do so, subject to the following conditions:
 .
 The above copyright notice and this permission notice shall be included in
 all copies or substantial portions of the Software.
 .
 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 SOFTWARE.
''')
sortie.append('''License: CC0-1.0
 To the extent possible under law, the author(s) have dedicated all copyright
 and related and neighboring rights to this software to the public domain
 worldwide. This software is distributed without any warranty.
 .
 On Debian systems, the complete text of the CC0 1.0 Universal license can be
 found in "/usr/share/common-licenses/CC0-1.0".
''')
sortie.append('''License: FSFAP
 Copying and distribution of this file, with or without modification, are
 permitted in any medium without royalty provided the copyright notice and
 this notice are preserved. This file is offered as-is, without any warranty.
''')
sortie.append('''License: CC-BY-4.0
 This work is licensed under the Creative Commons Attribution 4.0 International
 License. To view a copy of this license, visit
 https://creativecommons.org/licenses/by/4.0/legalcode or send a letter to
 Creative Commons, PO Box 1866, Mountain View, CA 94042, USA.
 .
 On Debian systems, the complete text of the Creative Commons Attribution 4.0
 International license can be found in "/usr/share/common-licenses/CC-BY-4.0".
''')
sortie.append('''License: public-domain
 This is free and unencumbered software released into the public domain.
 .
 Anyone is free to copy, modify, publish, use, compile, sell, or distribute
 this software, either in source code form or as a compiled binary, for any
 purpose, commercial or non-commercial, and by any means.
 .
 In jurisdictions that recognize copyright laws, the author or authors of this
 software dedicate any and all copyright interest in the software to the public
 domain. We make this dedication for the benefit of the public at large and to
 the detriment of our heirs and successors. We intend this dedication to be an
 overt act of relinquishment in perpetuity of all present and future rights to
 this software under copyright law.
''')
open(f'{R}/debian/copyright','w',encoding='utf-8').write('\n'.join(sortie))
print('debian/copyright écrit :', len(FICHIERS), 'fichiers suivis ;', len(sources_amont()), 'sources à notice amont ;', len(cc0()), 'fichiers CC0')
