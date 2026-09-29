# sermo — prise en main

**Créez des interfaces graphiques depuis un simple script XML.**
sermo lit une description de fenêtre en **XML** et l'affiche, pilotable depuis un
shell ou n'importe quel langage interprété.

```sh
export MAIN_DIALOG='
<window title="Bonjour">
  <vbox>
    <text><label>Votre nom ?</label></text>
    <entry><variable>NOM</variable></entry>
    <hbox><button ok></button><button cancel></button></hbox>
  </vbox>
</window>'

sermo --program=MAIN_DIALOG
echo "Bonjour, $NOM !"
# Sortie : NOM="Jean"  EXIT="OK"
```

`sermo` est l'alias du backend choisi par défaut (voir plus bas).

---

## Ce qu'est sermo aujourd'hui

sermo descend de **gtkdialog** (László Pere, GPL-2.0+). L'édition actuelle est
**modulaire** : elle sépare un **cœur** unique et des **backends de rendu**.

- **Le cœur `libsermocore`** — analyse le XML (Flex/Bison), exécute les actions
  de façon durcie (`safe_system` / `safe_popen`), gère les variables. Il n'a
  **aucune dépendance graphique**.
- **Les backends** — un par bibliothèque graphique. Ils ne font que **dessiner**
  ce que le cœur décrit, à travers un contrat `sermo_be_*`.

Il y a **sept backends** :

| Backend | Toolkit | Binaire |
|---|---|---|
| `sermo-backend-gtk3` | GTK 3 | `gtk3sermo` |
| `sermo-backend-gtk4` | GTK 4 | `gtk4sermo` |
| `sermo-backend-qt6`  | Qt 6  | `qt6sermo` |
| `sermo-backend-fltk1`| FLTK 1.4 | `fltk1sermo` |
| `sermo-backend-efl1` | Enlightenment/Elementary | `efl1sermo` |
| `sermo-backend-sdl3` | SDL 3 + Dear ImGui | `sdl3sermo` |
| `sermo-backend-ncurses` | ncurses (terminal) | `ncursessermo` |

Le **même script XML** rend une interface équivalente sur les sept : six en
graphique, `ncurses` en terminal (le plus léger, sans serveur d'affichage).

### À quoi ça sert

Le shell orchestre, filtre, décide — mais ne sait pas afficher. sermo est le
chaînon manquant : une syntaxe XML déclarative, une commande à invoquer, une
interface qui s'intègre au bureau (ou au terminal). La structure vit dans le XML,
la logique dans le shell : les deux restent séparés.

### Choisir son backend

- **Bureau GTK (GNOME, Xfce)** → `gtk3` (ou `gtk4`).
- **Bureau Qt / KDE** → `qt6`.
- **Installation minimale, sans GTK ni Qt** → `fltk1`.
- **Aucun serveur graphique (SSH, serveur, console)** → `ncurses`.
- `efl1` et `sdl3` existent aussi ; chaque backend ne tire que **sa** pile
  graphique, jamais celle des autres.

---

## Installer

L'édition modulaire produit des paquets **séparés**, un par backend :

```sh
apt install sermo-backend-gtk3     # GTK 3
apt install sermo-backend-fltk1    # FLTK, sans GTK ni Qt
```

L'alias `sermo` (`/usr/bin/sermo`) est géré par `update-alternatives` : chaque
backend installé s'y enregistre, vous choisissez le défaut.

### Compatibilité gtkdialog

sermo lit le **XML gtkdialog** tel quel : un script écrit pour `gtkdialog`
tourne sans modification. Pour que les scripts historiques qui appellent la
commande **`gtkdialog`** marchent aussi, installez le paquet **séparé** :

```sh
apt install sermo-gtkdialog        # pose /usr/bin/gtkdialog → gtk3sermo
```

Ce paquet est séparé et optionnel **exprès** : d'autres projets (le `gtkdialog`
original, le `gtk3dialog` de BunsenLabs) fournissent aussi `/usr/bin/gtkdialog`.
`sermo-gtkdialog` les remplace de façon **mutuellement exclusive**
(`Conflicts` / `Replaces` / `Provides: gtkdialog`) — on installe l'un ou
l'autre, jamais les deux.

---

## Compiler depuis les sources

La compilation se fait en **deux temps** : d'abord le cœur, dans la variante qui
convient au backend visé, puis le backend contre ce cœur.

**Dépendances communes** (le cœur) : `flex`, `bison`, `cmake` (≥ 3.16), `gcc`,
`pkg-config`.

```sh
sudo apt install flex bison cmake gcc pkg-config
```

**Par backend**, ajouter les `-dev` de sa toolkit : `libgtk-3-dev`,
`libvte-2.91-dev` et `libgtk-layer-shell-dev` (gtk3), `libgtk-4-dev` et
`libvte-2.91-gtk4-dev` (gtk4), `qt6-base-dev` (qt6), `libfltk1.4-dev` (fltk1),
`libefl-all-dev` (efl1), `libsdl3-dev` (sdl3), `libncurses-dev` (ncurses). Sans
VTE ou sans gtk-layer-shell, la construction de gtk3/gtk4 s'arrête plutôt que de
produire un binaire privé de `<terminal>` ou de l'ancrage Wayland ; les options
`-DSERMOCORE_VTE=OFF` et `-DSERMO_LAYER_SHELL=OFF` y renoncent explicitement.

Le cœur a **trois variantes de build** ; on en choisit **une** selon le backend :

| Backend | Variante du cœur | Option cmake |
|---|---|---|
| qt6, fltk1, efl1, sdl3, ncurses | neutre (défaut, sans toolkit) | — |
| gtk3 | GLib | `-DSERMOCORE_GLIB=ON` |
| gtk4 | GTK 4 | `-DSERMOCORE_GTK4=ON` |

```sh
# 1. le cœur (exemple : variante neutre, pour qt6/fltk1/efl1/sdl3/ncurses)
cd libsermocore
cmake -S . -B _build -DCMAKE_INSTALL_PREFIX=$PWD/_install
make -C _build && make -C _build install

# 2. un backend (il trouve le cœur par pkg-config)
export PKG_CONFIG_PATH=$PWD/_install/lib/pkgconfig
cmake -S sermo-backend-qt6 -B sermo-backend-qt6/_build
make -C sermo-backend-qt6/_build
# → binaire : sermo-backend-qt6/_build/qt6sermo
```

Pour gtk3, compiler le cœur avec `-DSERMOCORE_GLIB=ON` et pointer
`PKG_CONFIG_PATH` sur ce préfixe-là ; pour gtk4, `-DSERMOCORE_GTK4=ON` (GTK 4 a
retiré `gtk_main` / `gtk_socket` / l'ancien modèle d'évènements, d'où sa variante
dédiée).

Tout est détaillé dans `COMPILE.md` et `DEPENDENCIES.md`. Les paquets `.deb` se
construisent avec `bash packaging/construire-paquets.sh`, dans une Debian testing
vierge : `sermo-core-dev`, les 7 `sermo-backend-*`, `sermo-gtkdialog` et quatre
paquets de transition depuis la 1.x, soit **13 paquets** (voir `PACKAGING.md`).

---

## Sécurité — le comportement réel

La **frontière de confiance est l'auteur local du script XML**. sermo n'est pas
un bac à sable qui exécuterait du XML hostile : qui écrit le script peut déjà
lancer des commandes. Le rôle de sermo est de **ne pas transformer une saisie
utilisateur en exécution de code** que l'auteur n'a pas voulue. Toute
l'exécution passe par le cœur : `safe_exec.c`, en deux copies (`src/` et
`src-gtk4/`) qui ne diffèrent que par des commentaires, testées toutes les deux.

**`safe_system` / `safe_popen`** remplacent `system` / `popen` :

- Quand la commande **ne contient pas de métacaractères shell**, elle est
  exécutée **directement par `argv[]`, sans shell**. C'est le cas courant, et il
  évite l'injection shell.
- Quand elle en contient, il y a **repli sur `/bin/sh -c`** (fonctionnalité shell
  complète). Ce repli est **journalisé** (« injection risk ») et peut être
  **refusé** en posant la variable d'environnement `SERMO_NO_SHELL_FALLBACK`.

Il n'y a **pas de liste blanche de commandes imposée par défaut** : la confiance
est l'auteur du script, pas une allow-list. Un filtre optionnel reste disponible
pour les intégrations qui le veulent (`SERMO_ALLOWED_CMDS`).

Autres garde-fous :

- **Échappement de la sortie** : les valeurs exportées (`VAR="…"`) échappent les
  quatre caractères que le shell développe entre guillemets doubles
  (`` \ `` , `"`, `$`, accent grave), pour que `eval "$(sermo …)"` ne réexécute pas
  ce qu'un utilisateur a tapé dans un champ.
- **`--do`** est la voie sûre quand le dialogue peut servir à quelqu'un d'autre
  que l'auteur : les valeurs arrivent par l'environnement et ne sont **jamais**
  relues comme du code. Ne jamais passer la valeur d'un widget à `eval`.
- **Aucun** `pkexec`, aucune élévation de privilèges ; aucun
  `strcpy`/`strcat`/`sprintf` (bornage systématique).
- **Taille de ce que lit un `<input>`** (2.7.3) : au plus 16 Mio par `<input>`,
  commande ou fichier. Au-delà, la lecture s'arrête comme sur une fin de fichier,
  et un avertissement part sur la sortie d'erreur. `SERMO_INPUT_MAX` change la
  limite, en octets ; `0` la retire. La barre de progression n'est pas
  plafonnée : elle ne garde qu'une ligne à la fois.

**Durcissement du binaire** : cœur et backends compilent et lient avec
`-D_FORTIFY_SOURCE=3`, `-fstack-protector-strong`, `-fstack-clash-protection`,
`-fcf-protection=full` (CET IBT/SHSTK), PIE, RELRO complet, BIND_NOW, NX. Le banc
`tests/garde_durcissement.sh` le vérifie.

Signaler une faille : `devel@haplo-dialog.fr` (décrire le scénario reproductible
et la version : `sermo --version` + backend). Détail complet dans `SECURITY.md`.

---

## Licence

**GPL-2.0-or-later.** sermo descend de **gtkdialog 0.8.3** (László Pere), sous
GNU GPL version 2 « ou toute version ultérieure ». Le cœur `libsermocore` et les
sept backends sont sous cette même licence ; les fichiers C et C++ portent
l'en-tête `SPDX-License-Identifier: GPL-2.0-or-later`. Exceptions (contrat MIT,
Dear ImGui MIT dans sdl3, tests et quelques exemples en CC0) : voir `LICENCES.md`.

Les backends se lient à leur toolkit et à ses dépendances, sous leurs propres
licences. Certaines, chargées indirectement, sont sous LGPL-3 ou Apache-2.0 : le
binaire combiné circule alors sous GPL-3, ce que permet le « ou toute version
ultérieure ». Texte intégral dans `COPYING`.

Contact : `devel@haplo-dialog.fr`.

---

## Autres outils de ce serveur de documentation

Ce guide donne la vue d'ensemble. Pour écrire un dialogue **correct**, appuyez-
vous sur les autres outils de ce serveur : la **référence des widgets et des attributs**
(les balises disponibles, leurs formes qui marchent et les pièges de syntaxe) et
la boucle **apprendre → générer → vérifier → corriger** exposée pour l'IA.
Vérifier une syntaxe sans ouvrir de fenêtre : `sermo --program=VAR --print-ir`
(analyse et sort ; code de retour 0 = la syntaxe passe).
