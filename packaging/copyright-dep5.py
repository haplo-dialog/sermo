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
    ["L'ancrage Wayland (layer-shell) vient de la lignée woof-CE (Puppy Linux), par gtk3dialog."]))
sortie.append(bloc(['examples/*', 'sermoman-mcp/data/examples/*'], [LASZLO, THUNOR_EX, CAGE], 'GPL-2+',
    ['Mesuré en 2.8.0 : 188 des 213 fichiers de examples/ existaient dans',
     "gtkdialog 0.8.3 (73 à l'identique) ; les fichiers ajoutés depuis sont écrits",
     'pour sermo et, pour la plupart, déclarés CC0 (bloc suivant). pfeme,',
     'pfontview et playmusic sont des applications de Thunor.',
     'sermoman-mcp/data/examples/ en garde des copies.']))
# Fichiers CC0 qui ne peuvent pas porter d'en-tête (données JSON Lines).
CC0_SANS_ENTETE = ['examples/gnss-moniteur/echantillons/demo.jsonl']
sortie.append(bloc(sorted(cc0() + CC0_SANS_ENTETE), [CAGE], 'CC0-1.0',
    ['Cas de test XML, démos qt6 et exemples écrits pour sermo, qui le déclarent',
     "dans leur en-tête SPDX ; et l'échantillon de données de gnss-moniteur, qui ne",
     'peut pas porter d\'en-tête.']))
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
MAV = 'sermo-source/tiers/mavlink/'
sortie.append(bloc([MAV + '*'], ['les auteurs de MAVLink (https://mavlink.io)'], 'LGPL-3 with MAVLink exception',
    ['En-têtes C engendrés par le générateur MAVLink (c_library_v2, commit',
     '8c63a49fe35aead7f9ef14031381c5e076ff743a ; voir sermo-source/tiers/mavlink/LISEZMOI).',
     'Sous forme source, ils relèvent de la licence du générateur, LGPL-3 ; intégrés',
     'à un binaire (sermo-source-mavlink, sermo-cible-mavlink), ils sont sous licence',
     "MIT par l'exception écrite (COPYING.generateur, copié dans le même dossier)."]))
sortie.append(bloc([MAV + 'mavlink_sha256.h'],
    ['1995-2001 Kungliga Tekniska Högskolan (Royal Institute of Technology, Stockholm, Sweden)'], 'BSD-3-clause-heimdal',
    ['SHA-256 tiré de Heimdal, adapté par MAVLink (signature des messages).']))
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

# ── LVGL embarqué (sermo-backend-lvgl/tiers/lvgl) ───────────────────────────
# LVGL est sous licence MIT ; certaines de ses bibliothèques internes ont la leur
# (COPYRIGHTS.md). Chacune est nommée ici avec le texte de SON fichier de
# licence, copié tel quel : le texte d'une licence « BSD-2-clause » varie par
# son avis de copyright, d'où des noms propres à chaque bibliothèque.
LV = 'sermo-backend-lvgl/tiers/lvgl/'
LVL = LV + 'src/libs/'
sortie.append(bloc([LV + '*'], ['2025 LVGL Kft', 'NXP', 'SiFli Technologies', 'Think Silicon, STMicroelectronics',
                                'EDGEMTech Ltd.', 'Gaëtan Renaudeau'], 'Expat',
    ['LVGL v9.6.0, embarqué sans retouche (PROVENANCE). Les bibliothèques tierces',
     "qu'il contient ont les licences ci-dessous ; seules celles que",
     'sermo-backend-lvgl/lv_conf.h active sont compilées dans lvglsermo :',
     'TinyTTF, libpng (celle du système), ThorVG avec Lottie et rapidjson, la',
     'police Montserrat 14 et la bibliothèque C. Les pilotes matériels (NXP, SiFli,',
     'Think Silicon, Vivante…) ne sont pas compilés.']))
sortie.append(bloc(['sermo-backend-lvgl/lv_conf.h'], ['2025 LVGL Kft', CAGE], 'Expat',
    ["Réglages de LVGL pour lvglsermo, tirés de tiers/lvgl/lv_conf_template.h."]))
LICENCES_LVGL = []   # (nom, fichier) — texte repris tel quel
def lv(chemins, copyright, licence, fichier=None, commentaire=None):
    sortie.append(bloc([LVL + c if not c.startswith('sermo-') else c for c in chemins], copyright, licence, commentaire))
    if fichier:
        LICENCES_LVGL.append((licence, fichier))
lv(['barcode/*'], ['2013 LKC Technologies, Inc.'], 'BSD-2-clause-barcode', LVL + 'barcode/LICENSE.txt')
lv(['frogfs/*'], ['les auteurs de FrogFS (Jeff Kent)'], 'MPL-2.0')
lv(['FT800-FT813/*'], ['2016-2024 Rudolph Riedel'], 'Expat')
lv(['gif/*'], ['2020 BitBank Software, Inc.'], 'Apache-2.0')
lv(['lodepng/*'], ['2005-2023 Lode Vandevenne'], 'Zlib-lodepng', LVL + 'lodepng/LICENSE.txt')
lv(['lz4/*'], ['2011-2023 Yann Collet'], 'BSD-2-clause-lz4', LVL + 'lz4/LICENSE.txt')
lv(['nanovg/*'], ['2013 Mikko Mononen'], 'Zlib-nanovg', LVL + 'nanovg/LICENSE.txt')
lv(['qrcode/*'], ['Project Nayuki'], 'Expat')
lv(['thorvg/*'], ['2020-2025 les auteurs de ThorVG', 'Samsung Electronics Co., Ltd.'], 'Expat',
   commentaire=['ThorVG est compilé dans lvglsermo depuis la 2.8.2 (LV_USE_THORVG vaut 1) :',
                'il dessine les icônes SVG et les animations Lottie.'])
lv(['thorvg/tvgSvgLoader.cpp', 'thorvg/tvgSvgPath.cpp'], ['2020-2024 the ThorVG project', 'EFL developers'],
   'Expat and BSD-2-clause-efl',
   commentaire=['Analyseur SVG de ThorVG, repris en partie des EFL (notice BSD-2 dans le fichier).'])
lv(['thorvg/tvgLottieInterpolator.cpp'], ['2020-2024 the ThorVG project', 'Mozilla Foundation'], 'Expat and MPL-2.0',
   commentaire=['Une partie de ce fichier (interpolation de courbes) est sous MPL-2.0.'])
lv(['thorvg/rapidjson/*'], ['2015 THL A29 Limited, a Tencent company, and Milo Yip'], 'Expat',
   commentaire=["rapidjson, lecteur JSON des animations Lottie ; compilé. Son assistant",
                'msinttypes (bloc suivant) ne sert qu\'à Visual Studio antérieur à 2013.'])
lv(['thorvg/rapidjson/msinttypes/*'], ['2006-2013 Alexander Chemeris'], 'BSD-3-clause-msinttypes',
   commentaire=['Non compilé ici.'])
lv(['tiny_ttf/*'], ['2017 Sean Barrett'], 'Expat or public-domain',
   commentaire=['TinyTTF, issu de stb_truetype : au choix, MIT ou domaine public (Unlicense).'])
lv(['tjpgd/*'], ['2021 ChaN'], 'TJpgDec', LVL + 'tjpgd/LICENSE.txt',
   commentaire=['Non compilé ici (LV_USE_TJPGD vaut 0).'])
lv(['vg_lite_driver/*', LV + 'src/debugging/vg_lite_tvg/vg_lite.h'], ['2012-2023 Vivante Corporation'], 'MIT-Vivante', LVL + 'vg_lite_driver/LICENSE.txt',
   commentaire=['Pilote matériel VeriSilicon/Vivante, non compilé ici.'])
lv(['vg_lite_driver/VGLiteKernel/*'], ['2014-2022 Vivante Corporation'], 'Expat or GPL-2+',
   commentaire=['Partie noyau du pilote, au choix MIT ou GPL-2+ (notice de chaque fichier) ; non compilée.'])
lv(['freetype/LiberationSans-Regular.ttf', 'freetype/LiberationSans-LICENSE.txt'],
   ['2010 Google Corporation', '2012 Red Hat, Inc.'], 'OFL-1.1-LiberationSans', LVL + 'freetype/LiberationSans-LICENSE.txt',
   commentaire=['Police de démonstration livrée avec les interfaces FreeType de LVGL ;',
                'non utilisée par lvglsermo (LV_USE_FREETYPE vaut 0).'])
lv(['freetype/ftmodule.h', 'freetype/ftoption.h', 'freetype/freetype-LICENSE.txt'],
   ['David Turner, Robert Wilhelm, Werner Lemberg (FreeType)'], 'FTL or GPL-2+',
   commentaire=["Interfaces FreeType ; FreeType lui-même n'est pas dans LVGL."])
LICENCES_LVGL.append(('FTL', LVL + 'freetype/freetype-LICENSE.txt'))
sortie.append(bloc([LV + 'src/stdlib/builtin/lv_tlsf.c', LV + 'src/stdlib/builtin/lv_tlsf.h', LV + 'src/stdlib/builtin/lv_tlsf_private.h'],
    ['2006-2016 Matthew Conte'], 'BSD-3-clause-tlsf',
    ["Allocateur TLSF ; texte : LICENSE_TLSF.txt à côté. Non compilé ici (la",
     'bibliothèque C fournit malloc).']))
LICENCES_LVGL.append(('BSD-3-clause-tlsf', (LV + 'src/stdlib/builtin/lv_tlsf.h', 18, 41, '** ')))
LVF = LV + 'src/font/'
sortie.append(bloc([LVF + 'lv_font_montserrat_*.c'], ['2011 Julieta Ulanovsky (Montserrat)', 'Fonticons, Inc. (Font Awesome Free 5)'],
    'OFL-1.1',
    ['Glyphes de Montserrat et de Font Awesome Free 5 (polices), convertis en tableaux C',
     'par lv_font_conv ; lv_font_montserrat_14.c est compilé (police par défaut).']))
sortie.append(bloc([LVF + 'lv_font_dejavu_16_persian_hebrew.c'], ['2003 Bitstream, Inc. (Bitstream Vera)', 'Fonticons, Inc. (Font Awesome Free 5)'],
    'Bitstream-Vera and OFL-1.1',
    ['Glyphes de DejaVu Sans (modifications DejaVu dans le domaine public) et de Font',
     'Awesome Free 5 ; non compilé.']))
sortie.append(bloc([LVF + 'lv_font_source_han_sans_sc_*_cjk.c'], ['2014-2021 Adobe (Source Han Sans)'], 'OFL-1.1',
    ['Glyphes de Source Han Sans ; non compilé.']))
sortie.append(bloc([LVF + 'lv_font_unscii_*.c'], ['Viznut (unscii)'], 'public-domain',
    ['Glyphes de la police unscii, versée par son auteur dans le domaine public ; non compilé.']))
sortie.append(bloc([LV + 'src/drivers/opengles/glad/*'], ['2013-2020 The Khronos Group Inc.', 'David Herberth (glad)'],
    '(WTFPL or CC0-1.0) and Apache-2.0',
    ["Chargeur OpenGL engendré par glad (WTFPL ou CC0) à partir des définitions Khronos",
     '(Apache-2.0) ; non compilé (LV_USE_OPENGLES vaut 0).']))
sortie.append(bloc([LV + 'src/drivers/opengles/glad/include/KHR/khrplatform.h'], ['2008-2018 The Khronos Group Inc.'], 'Expat',
    ['Non compilé.']))
sortie.append(bloc([LV + 'src/osal/lv_cmsis_rtos2.c', LV + 'src/osal/lv_cmsis_rtos2.h'], ['2023 Arm Limited'], 'Apache-2.0',
    ['Couche CMSIS-RTOS2 ; non compilée (LV_OS_NONE).']))
sortie.append(bloc([LV + 'src/debugging/test/lv_test_screenshot_compare.c'], ['2002-2010 Guillaume Cottenceau'], 'X11',
    ['Outil de test de LVGL ; non compilé (LV_USE_TEST vaut 0).']))
sortie.append(bloc([LVL + 'gltf/stb_image/stb_image.h'], ['2017 Sean Barrett'], 'Expat or public-domain',
    ['stb_image, au choix MIT ou domaine public ; non compilé (LV_USE_GLTF vaut 0).']))
sortie.append(bloc([LV + 'src/stdlib/builtin/lv_sprintf_builtin.c'], ['2014-2019 Marco Paland'], 'Expat',
    ['Formatage printf embarqué ; texte : LICENSE_SPRINTF.txt à côté.']))
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

def paragraphe(nom, fichier, commun=None):
    if isinstance(fichier, tuple):   # (fichier, première ligne, dernière ligne, préfixe de commentaire)
        f, d, fin, pre = fichier
        t = open(f'{R}/{f}', encoding='utf-8', errors='replace').read().split('\n')[d - 1:fin]
        t = [l[len(pre):].rstrip() if l.startswith(pre) else ('' if l.strip() in (pre.strip(), '') else l.strip()) for l in t]
    else:
        t = open(f'{R}/{fichier}', encoding='utf-8', errors='replace').read().strip('\n').split('\n')
    corps = [(' ' + l.rstrip()) if l.strip() else ' .' for l in t]
    return 'License: ' + nom + '\n' + '\n'.join(corps) + '\n'
for nom, fichier in LICENCES_LVGL:
    sortie.append(paragraphe(nom, fichier))
LIB = LVL + 'thorvg/'
sortie.append(paragraphe('BSD-2-clause-efl', (LIB + 'tvgSvgLoader.cpp', 27, 51, ' * ')))
sortie.append(paragraphe('BSD-3-clause-heimdal', (MAV + 'mavlink_sha256.h', 15, 40, ' * ')))
sortie.append(paragraphe('BSD-3-clause-msinttypes', (LIB + 'rapidjson/msinttypes/stdint.h', 6, 29, '// ')))
sortie.append(paragraphe('OFL-1.1', (LVL + 'freetype/LiberationSans-LICENSE.txt', 12, 10000, '')))
sortie.append(paragraphe('LGPL-3 with MAVLink exception', (MAV + 'COPYING.generateur', 1, 32, '')).rstrip('\n') + """
 .
 On Debian systems, the complete text of the GNU Lesser General Public License
 version 3 can be found in "/usr/share/common-licenses/LGPL-3" (and the GNU GPL
 version 3 in "/usr/share/common-licenses/GPL-3").
""")
sortie.append("""License: Bitstream-Vera
 Permission is hereby granted, free of charge, to any person obtaining a copy
 of the fonts accompanying this license ("Fonts") and associated
 documentation files (the "Font Software"), to reproduce and distribute the
 Font Software, including without limitation the rights to use, copy, merge,
 publish, distribute, and/or sell copies of the Font Software, and to permit
 persons to whom the Font Software is furnished to do so, subject to the
 following conditions:
 .
 The above copyright and trademark notices and this permission notice shall
 be included in all copies of one or more of the Font Software typefaces.
 .
 The Font Software may be modified, altered, or added to, and in particular
 the designs of glyphs or characters in the Fonts may be modified and
 additional glyphs or characters may be added to the Fonts, only if the fonts
 are renamed to names not containing either the words "Bitstream" or the word
 "Vera".
 .
 This License becomes null and void to the extent applicable to Fonts or Font
 Software that has been modified and is distributed under the "Bitstream
 Vera" names.
 .
 The Font Software may be sold as part of a larger software package but no
 copy of one or more of the Font Software typefaces may be sold by itself.
 .
 THE FONT SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 OR IMPLIED, INCLUDING BUT NOT LIMITED TO ANY WARRANTIES OF MERCHANTABILITY,
 FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT OF COPYRIGHT, PATENT,
 TRADEMARK, OR OTHER RIGHT. IN NO EVENT SHALL BITSTREAM OR THE GNOME
 FOUNDATION BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, INCLUDING
 ANY GENERAL, SPECIAL, INDIRECT, INCIDENTAL, OR CONSEQUENTIAL DAMAGES,
 WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF
 THE USE OR INABILITY TO USE THE FONT SOFTWARE OR FROM OTHER DEALINGS IN THE
 FONT SOFTWARE.
 .
 Except as contained in this notice, the names of Gnome, the Gnome
 Foundation, and Bitstream Inc., shall not be used in advertising or
 otherwise to promote the sale, use or other dealings in this Font Software
 without prior written authorization from the Gnome Foundation or Bitstream
 Inc., respectively. For further information, contact: fonts at gnome dot
 org.
""")
sortie.append("""License: X11
 Permission is hereby granted, free of charge, to any person obtaining a copy
 of this software and associated documentation files (the "Software"), to
 deal in the Software without restriction, including without limitation the
 rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
 sell copies of the Software, and to permit persons to whom the Software is
 furnished to do so, subject to the following conditions:
 .
 The above copyright notice and this permission notice shall be included in
 all copies or substantial portions of the Software.
 .
 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
 X CONSORTIUM BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
 AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNEC-
 TION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 .
 Except as contained in this notice, the name of the X Consortium shall not
 be used in advertising or otherwise to promote the sale, use or other deal-
 ings in this Software without prior written authorization from the X Consor-
 tium.
""")
sortie.append("""License: WTFPL
 DO WHAT THE FUCK YOU WANT TO PUBLIC LICENSE
 Version 2, December 2004
 .
 Copyright (C) 2004 Sam Hocevar <sam@hocevar.net>
 .
 Everyone is permitted to copy and distribute verbatim or modified
 copies of this license document, and changing it is allowed as long
 as the name is changed.
 .
 DO WHAT THE FUCK YOU WANT TO PUBLIC LICENSE
 TERMS AND CONDITIONS FOR COPYING, DISTRIBUTION AND MODIFICATION
 .
 0. You just DO WHAT THE FUCK YOU WANT TO.
""")
sortie.append("""License: MPL-2.0
 This Source Code Form is subject to the terms of the Mozilla Public License,
 v. 2.0. If a copy of the MPL was not distributed with this file, You can
 obtain one at https://mozilla.org/MPL/2.0/.
 .
 On Debian systems, the complete text of the Mozilla Public License version 2.0
 can be found in "/usr/share/common-licenses/MPL-2.0".
""")
sortie.append("""License: Apache-2.0
 Licensed under the Apache License, Version 2.0 (the "License"); you may not
 use this file except in compliance with the License.
 .
 On Debian systems, the complete text of the Apache License version 2.0 can be
 found in "/usr/share/common-licenses/Apache-2.0".
""")
open(f'{R}/debian/copyright','w',encoding='utf-8').write('\n'.join(sortie))
print('debian/copyright écrit :', len(FICHIERS), 'fichiers suivis ;', len(sources_amont()), 'sources à notice amont ;', len(cc0()), 'fichiers CC0')
