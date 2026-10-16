# sermo — recettes hors Debian

Six recettes pour les neuf ports modulaires (`gtk3 gtk4 qt6 fltk1 efl1 sdl3 lvgl
ncurses html`), plus le cœur (`sermo-core-dev`) et le manuel (`sermo-doc`) ; quatre
autres, Termux (Android), FreeBSD, OpenBSD et Homebrew (macOS), pour le seul port
`ncurses` (sous Termux, les ports graphiques se construisent à la main : voir plus bas).
Elles remplacent l'ancien kit `sermo-backend-qt6/packaging/`, écrit pour un
`qt6sermo 1.0.0` autonome qui n'existe plus (retiré ; l'historique git le garde).

| Système | Fichier | Paquets produits | Commande `sermo` |
|---|---|---|---|
| Arch Linux | `arch/PKGBUILD` | 11 (paquets scindés d'un seul PKGBUILD) | non : `<port>sermo` |
| Fedora / RPM | `rpm/sermo.spec` | 11 sous-paquets | oui : `alternatives` |
| Gentoo | `gentoo/dev-util/sermo/sermo-2.8.2.ebuild` | 1, un drapeau USE par port | non : `<port>sermo` |
| Slackware | `slackware/sermo.SlackBuild` | un `.txz` par port demandé | non : `<port>sermo` |
| Alpine | `alpine/APKBUILD` | 8 sous-paquets (sans efl1 ni fltk1 : EFL absent, FLTK en 1.3), `provider_priority` | non : `<port>sermo` |
| NixOS | `nix/default.nix` | une dérivation par port + `all` | non : `<port>sermo` |
| Termux (Android) | `termux/build.sh` (format termux-packages) | 1, le port `ncurses` | oui : lien vers `ncursessermo` |
| FreeBSD | `freebsd/devel/sermo/` (port : Makefile, pkg-plist, pkg-descr) | 1, le port `ncurses` | oui : lien vers `ncursessermo` |
| OpenBSD | `openbsd/devel/sermo/` (port : Makefile, pkg/PLIST, pkg/DESCR) | 1, le port `ncurses` | oui : lien vers `ncursessermo` |
| macOS (Homebrew) | `homebrew/sermo.rb` (formule) | 1, le port `ncurses` | oui : lien vers `ncursessermo` |

Debian (`debian/`) reste la référence : il a `update-alternatives`, le paquet de
transition `sermo-gtkdialog`, et c'est lui que `packaging/construire-paquets.sh`
éprouve dans une Debian vierge.

## Une seule construction

Les six recettes appellent le même script, `construire-os.sh` : le cœur dans ses
variantes (neutre, GLib, GTK 4), chaque port contre la bonne variante, puis
`cmake --install` — ce sont les CMakeLists des ports qui savent quels fichiers
ils livrent. C'est la suite de gestes de `debian/rules`, écrite une fois.

```sh
sh packaging/autres-os/construire-os.sh construire  . _os gtk3 ncurses html
sh packaging/autres-os/construire-os.sh installer   . _os html /tmp/dest /usr
sh packaging/autres-os/construire-os.sh coeur-dev   . _os /tmp/dest /usr lib
sh packaging/autres-os/construire-os.sh manuel      . _os /tmp/dest /usr
```

## Windows (port ncurses, en natif)

Pas de recette de paquet : un seul `ncursessermo.exe`, compilé en croisé depuis
Debian par mingw-w64 (paquet `gcc-mingw-w64-ucrt64`), lié en statique. Il ne
dépend que de DLL du système (KERNEL32, USER32, ADVAPI32, SHELL32 et l'UCRT de
Windows 10/11). Le fichier de chaîne est `windows/mingw-w64.cmake` ; ncurses se
construit à part pour la même cible (`--host=x86_64-w64-mingw32ucrt`,
`--enable-term-driver --enable-exp-win32 --with-fallbacks=ms-terminal`, en
statique).

```sh
cmake -S libsermocore -B _win/coeur -DCMAKE_TOOLCHAIN_FILE=packaging/autres-os/windows/mingw-w64.cmake -DCMAKE_INSTALL_PREFIX=$RACINE
cmake --build _win/coeur && cmake --install _win/coeur
PKG_CONFIG_LIBDIR=$RACINE/lib/pkgconfig cmake -S sermo-backend-ncurses -B _win/ncurses -DCMAKE_TOOLCHAIN_FILE=packaging/autres-os/windows/mingw-w64.cmake
PKG_CONFIG_LIBDIR=$RACINE/lib/pkgconfig cmake --build _win/ncurses --clean-first
```

`--clean-first` : le port ne se relie pas tout seul quand le cœur installé change.

Ce que Windows change :
- **commandes** : `CreateProcess` ; le shell par défaut est celui du système
  (`%ComSpec%`, cmd.exe), appelé en `cmd /S /C "commande"`. Une commande sans
  syntaxe shell que Windows ne trouve pas comme programme (`echo`, `dir`, `type`…,
  internes à cmd) est relancée par le shell, sous les mêmes garde-fous
  (`SERMO_NO_SHELL_FALLBACK`, `SERMO_ALLOWED_CMDS`). Pour des scripts écrits pour
  `sh`, `SERMO_SHELL` peut désigner busybox-w32 copié sous le nom `sh.exe` ;
  sermo ne le livre pas ;
- **texte** : UCRT, locale UTF-8, console en UTF-8, manifeste `activeCodePage`
  UTF-8 (Windows 10 1903 ou plus) ; la sortie standard est en LF ;
- **liens** : `ShellExecute`, pour `http`, `https` et `mailto` seulement (il
  lancerait un programme qu'on lui désignerait) ;
- **limite des `<input>` (`SERMO_INPUT_MAX`)** : **absente sous Windows**. Le CRT
  n'a ni `fopencookie` ni `funopen` ; une commande sans fin lue par un `<input>`
  peut épuiser la mémoire, comme avant la 2.7.3 sous Linux ;
- le sélecteur de fichiers du port ne connaît pas encore les lettres de lecteur.

Rejoué sous Wine 10 (12/10/2026) : banc de comportement 61/61 avec busybox-w32
comme `SERMO_SHELL`, scripts métier et boîtes à outils 187 dialogues sur 187
identiques à gtk3sermo, garde-fous de la relance vérifiés ; banc de comportement
78/78 pour la 3.0.0. Sur un vrai Windows 11 : affichage, accents, sortie vers un
fichier. Sur un vrai Windows 10 : commandes par cmd (accents justes) et par
busybox-w32, lien `https` ouvert, programme (`calc.exe`) refusé. Les accents ne se vérifient pas sous Wine (son
`mbrtowc` UTF-8 rend U+FFFD).

## Termux : les ports graphiques, à la main

La recette `termux/build.sh` ne livre que `ncurses`. Les autres ports se
construisent sur l'appareil par `ci/construire.sh`, à condition d'ajouter le
dépôt X11 de Termux. Huit ports sur neuf y passent ; **efl1 ne peut pas** :
EFL n'existe pas dans Termux.

```sh
pkg install x11-repo
pkg install cmake flex bison pkg-config glib json-glib ncurses \
    gtk3 gtk4 libvte gtk-layer-shell \
    qt6-qtbase qt6-qtbase-gtk-platformtheme \
    sdl3 mesa-dev libx11 libpng libwayland libwayland-protocols libxkbcommon
bash ci/construire.sh gtk3 gtk4 qt6 sdl3 lvgl ncurses html
```

- `qt6-qtbase-gtk-platformtheme` : les fichiers CMake de Qt 6 empaquetés par
  Termux réclament `libqgtk3.so`, que ce paquet seul fournit ; sans lui,
  `find_package(Qt6 … Widgets)` échoue.
- `mesa-dev` : les en-têtes OpenGL (`GL/gl.h`), que Qt 6 et SDL 3 demandent.
- Le shell du repli reste `/bin/sh` (celui d'Android) ; la recette du port
  `ncurses` pose celui de Termux, ce que `-DSERMO_SHELL_DEFAUT=$PREFIX/bin/sh`
  au cœur ferait aussi ici.

**Deux pièges de Termux, sans rapport avec sermo :**
- **`xdg-utils` reste à moitié installé** (et Qt 6 avec lui, qui en dépend) :
  son script d'installation lance `cpan`, qui sous Android croit que Perl est
  le script lui-même (`$^X`) et échoue (« Could not expand [Makefile.PL] »).
  Le contourner en lançant `cpan` par `perl`, puis finir l'installation :

  ```sh
  perl $PREFIX/bin/cpan -Ti File::MimeInfo
  dpkg --configure -a
  ```
- **gtk3sermo s'arrête net (code 134)** dès qu'il cherche une icône absente :
  GTK 3 n'arrive pas à charger `image-missing.svg` du thème Adwaita 50 de
  Termux. Le thème `AdwaitaLegacy` (déjà installé) l'évite :

  ```sh
  mkdir -p ~/.config/gtk-3.0
  printf '[Settings]\ngtk-icon-theme-name=AdwaitaLegacy\n' >> ~/.config/gtk-3.0/settings.ini
  ```

**fltk1 demande FLTK 1.4** (`Fl_Flex`) ; Termux n'empaquette que la 1.3, que
le port refuse dès la configuration. FLTK 1.4 se construit à part, dans un
préfixe à soi, avec trois réglages : `HAVE_TRUNC=1` (son essai, fait sans
`-lm`, rate le `trunc` de bionic), `-include pwd.h` (Termux renomme
`getpwnam` par une macro, que la déclaration de FLTK doit voir aussi) et
`-mbranch-protection=standard` (sans quoi le binaire perd la marque BTI) :

```sh
cmake -S fltk-1.4.4 -B _fltk -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=$HOME/fltk-1.4 \
    -DFLTK_BACKEND_WAYLAND=OFF -DFLTK_BUILD_TEST=OFF -DFLTK_BUILD_EXAMPLES=OFF \
    -DFLTK_BUILD_FLUID=OFF -DFLTK_BUILD_FLTK_OPTIONS=OFF -DFLTK_BUILD_GL=OFF \
    -DHAVE_TRUNC=1 \
    "-DCMAKE_C_FLAGS=-include pwd.h -mbranch-protection=standard" \
    "-DCMAKE_CXX_FLAGS=-include pwd.h -mbranch-protection=standard"
cmake --build _fltk && cmake --install _fltk
PKG_CONFIG_PATH=$PWD/libsermocore/_install/lib/pkgconfig \
    cmake -S sermo-backend-fltk1 -B sermo-backend-fltk1/_build -DFLTK_CONFIG=$HOME/fltk-1.4/bin/fltk-config
PKG_CONFIG_PATH=$PWD/libsermocore/_install/lib/pkgconfig cmake --build sermo-backend-fltk1/_build
```

(`libsermocore/_install` : le cœur neutre, tel que `ci/construire.sh` le pose.)

Ce que sermo adapte de lui-même sous Android, après essai par CMake : BTI/PAC
à la place de CET, `funopen` à la place de `fopencookie`, et pour lvgl
`shm_open`/`shm_unlink` fournis par `memfd_create` (absents de bionic, le
pilote Wayland de LVGL les appelle).

## Ce qui a été vérifié, et ce qui ne l'a pas été

**Reconstruit dans un conteneur de chaque système** (12/10/2026, sur l'arbre exact
de la 2.8.2, archive identique à celle de l'étiquette, recettes **sans aucune
correction** ; journaux gardés hors dépôt). Dans chaque conteneur, les binaires ont
été lancés : `--version` pour chaque port, et un vrai dialogue sans écran pour
`html` et `ncurses` (`NOM` et `EXIT` rendus) :
- **Arch** et **Fedora 44** : les neuf ports construits, **installés** (`pacman -U`,
  `dnf install` ; `alternatives` pointe `sermo` vers gtk3sermo) et lancés ;
- **NixOS** (nixpkgs-unstable) : les neuf ports, du premier coup ;
- **Gentoo** : les neuf ports avec le profil `default/linux/amd64/23.0/desktop`
  (environ 20 min, sans autounmask) ; avec le profil de base, la demande d'EFL
  bloque sur `media-libs/freetype[harfbuzz]` (réglage du système, pas de la
  recette) ; `html` et `ncurses` passent avec n'importe quel profil ;
- **Alpine** (edge) : les sept ports prévus (ni EFL, absent d'Alpine, ni FLTK, en
  1.3 sans `Fl_Flex`), dépôt local signé par `abuild-keygen -a -i`, paquets
  installés par `apk add` ;
- **Slackware 15.0** : `html`, `ncurses` et `coeur-dev`, construits, installés par
  `installpkg` et lancés ; depuis le 13/10/2026 (sur l'arbre qui suit la 2.8.2),
  **`gtk3` et `lvgl` aussi**. gtk-layer-shell n'existe pas dans Slackware 15.0 :
  le SlackBuild construit alors gtk3 sans l'ancrage Wayland
  (`-DSERMO_LAYER_SHELL=OFF`) et le dit ; lvgl prend le `linux-dmabuf` de
  `unstable/` (wayland-protocols 1.25) et lie `librt` (glibc 2.33). Pas de Qt 6,
  d'EFL ni de SDL 3 ; FLTK en 1.3.

Les essais du 10/10/2026 (version de travail 2.8.0) avaient trouvé et fait
corriger : le lien en PIE (Slackware), la glibc antérieure à 2.34 (Slackware),
musl (Alpine), FLTK 1.3 (Alpine).

Avant ces essais, à l'écriture des recettes, aucun de ces six systèmes n'était
disponible ; les rejouages ci-dessous datent de ce moment.

**Rejoué sur une Debian testing** (10/10/2026) :
- les neuf ports se construisent et s'installent par `construire-os.sh`
  (59 s) ; la liste des fichiers livrés par port est celle que déclarent les
  sections `%files` du RPM et les `amove` d'Alpine ;
- `PKGBUILD` : `build()` puis `package_sermo-core-dev`, `package_sermo-doc` et
  quatre `package_sermo-backend-*` rejoués avec un `srcdir`/`pkgdir` factices ;
- `APKBUILD` : `core_dev`, `backend_html`, `backend_gtk3` rejoués avec un
  `amove` factice qui vérifie l'existence des fichiers ;
- `sermo.SlackBuild` : `html`, `ncurses` et `coeur-dev` rejoués avec un
  `makepkg` factice ; il n'efface rien (répertoires `mktemp`) ;
- `sh -n` / `bash -n` sur toutes les recettes.

**Termux, sur l'appareil** (12/10/2026, Android 15, arm64, Termux de Google Play,
clang 21) : `termux/build.sh` rejoué comme termux-packages (`termux_step_make`
puis `termux_step_make_install`, dans un préfixe factice) ; le binaire installé
répondait 2.9.0 (numéro de travail de la 3.0.0) et lance ses commandes par le shell de Termux. Le port y rend les
mêmes valeurs que l'étalon gtk3sermo : banc de comportement 61/61, scripts métier
et boîtes à outils 187 dialogues sur 187. Trois adaptations, choisies par CMake
après essai : shell de Termux (`SERMO_SHELL_DEFAUT`), `funopen` à la place de
`fopencookie` (absent de bionic), BTI/PAC à la place de CET. Le binaire final
perdait la marque BTI/PAC : non pas à cause d'Android, comme on l'avait cru,
mais d'un objet du port compilé sans protection (`neutral_glist.c`, corrigé le
12/10, le lieur l'a nommé par `-z bti-report`). Construit depuis à la main, le
port la garde ; la recette elle-même n'a pas été rejouée depuis.

**Termux, les ports graphiques** (12-13/10/2026, même appareil, à la main
comme ci-dessus, X11 sous Xvfb) : gtk3, gtk4, qt6, sdl3, lvgl, ncurses, html et
fltk1 (contre FLTK 1.4.4 construit à part) se construisent sans avertissement ;
banc de comportement 61/61 pour chacun (gtk3 avec `AdwaitaLegacy`). Les
binaires portent BTI+PAC, sauf qt6 : les fonctions atomiques de la
bibliothèque d'exécution de clang fournie par Termux ne l'ont pas. Trouvé en
route et corrigé : les widgets frères rangés à l'envers par tout compilateur
clang (ordre d'évaluation de `_sum(pop(), pop())`), des objets à part compilés
sans protection, le test `test_safe_exec` qui supposait `/tmp` et
`fopencookie`, et trois défauts du CMakeLists de fltk1.

**FreeBSD, dans une machine virtuelle** (12/10/2026, FreeBSD 15.1, amd64, clang 19,
ncurses du système de base) : les cibles `do-build` et `do-install` du port jouées
par le `make` de FreeBSD, `bsd.port.mk` remplacé par un fichier vide (le cadre des
ports n'est pas rejoué) ; fichiers installés conformes à `pkg-plist`. Le port y rend
les mêmes valeurs que l'étalon gtk3sermo : banc de comportement 61/61, scripts
métier et boîtes à outils 187 dialogues sur 187. Trouvé en route : `strtod_l` non
déclaré sans `<xlocale.h>` (chaque nombre décimal aurait été faux), `usleep` caché
par POSIX 2008, quatre cas du banc qui dépendaient du `wc -l` de GNU, et
`MAKEFLAGS` du make de FreeBSD que GNU make lisait comme des cibles. Le lieur lld
ignore `-z ibt` (avertissement).

**OpenBSD, dans une machine virtuelle** (12/10/2026, OpenBSD 7.9, amd64, clang 19,
curses du système de base) : même rejeu du port (`do-build`, `do-install`,
`bsd.port.mk` vide) ; fichiers conformes à `pkg/PLIST` (pages de manuel sous
`${PREFIX}/man`). Banc de comportement 61/61, scripts métier et boîtes à outils
187 dialogues sur 187 identiques à gtk3sermo. OpenBSD n'a pas `strtod_l` : les
shims lisent alors les nombres sous la locale C par `uselocale` le temps d'un
`strtod` (`SERMO_SANS_STRTOD_L`, posé par CMake après essai).

**Non vérifié, ou à reprendre** :
- une installation sur un système réel, hors conteneur ;
- les sommes de contrôle de Termux, FreeBSD et OpenBSD (`SKIP`, `distinfo`) ;
  celle de la formule Homebrew est posée (archive de l'étiquette `v3.0.0`).
- Termux : la construction croisée de la ferme termux-packages (seule la
  construction sur l'appareil est rejouée) ; les ports graphiques sur un
  vrai écran (Termux:X11), le banc ne les ayant joués que sous Xvfb ; le
  pilote Wayland de lvgl, faute de compositeur.
- FreeBSD : le cadre des ports lui-même (`bsd.port.mk`, `make stage-qa`,
  `portlint`).
- OpenBSD : le cadre des ports lui-même (`bsd.port.mk`, `make port-lib-depends-check`,
  `distinfo`).
- macOS : une installation sur un vrai Mac. Sur un runner macOS de GitHub
  (`.github/workflows/macos.yml`) : construction, banc de comportement 61/61,
  187 dialogues identiques à gtk3sermo (`examples/metiers/verifie.sh --ports gtk3
  --rejouer passe-macos`), formule installée et son test vert.
