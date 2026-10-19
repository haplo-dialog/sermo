# Manuel Utilisateur — sermo

[English](MANUEL_UTILISATEUR.en.md)

**Version :** 3.1.2
**Licence :** GPL-2.0-or-later | **Distributeur :** haplo-dialog (devel@haplo-dialog.fr)

---

## Table des matières

1. [Introduction](#1-introduction)
2. [Installation](#2-installation)
3. [Premiers pas](#3-premiers-pas)
4. [Syntaxe de base](#4-syntaxe-de-base)
5. [Référence des widgets](#5-référence-des-widgets)
6. [Actions et signaux](#6-actions-et-signaux)
7. [Variables et entrées/sorties](#7-variables-et-entréessorties)
8. [Exemples pratiques](#8-exemples-pratiques)
9. [Intégration dans un script shell](#9-intégration-dans-un-script-shell)
10. [FAQ et dépannage](#10-faq-et-dépannage)

---

## 1. Introduction

`sermo` est un utilitaire en ligne de commande qui construit des **interfaces
graphiques** depuis n'importe quel script shell, Python, Perl ou autre langage
interprété — sans écrire une seule ligne de code graphique.

Le principe est simple : vous décrivez votre interface en **XML**, et sermo
l'affiche. Quand le dialogue se termine **par un bouton**, les valeurs saisies
sont renvoyées sur la sortie standard sous forme de variables shell.

**Ce manuel décrit le langage XML**, qui est **identique sur les neuf backends**
de sermo (gtk3, gtk4, qt6, fltk1, efl1, sdl3, lvgl, ncurses, html) : le même script
rend une interface équivalente partout ; html, lui, l'écrit en page HTML au lieu
de l'afficher. `sermo` désigne dans ce document le backend
sélectionné par défaut sur votre système (`update-alternatives`), mais vous
pouvez aussi appeler un backend précis (`gtk3sermo`, `qt6sermo`…).

**Exemple minimal :**

```bash
export DIALOG='
<window title="Bienvenue">
  <vbox>
    <text><label>Entrez votre nom :</label></text>
    <entry><variable>NOM</variable></entry>
    <button><label>OK</label><action>EXIT:ok</action></button>
  </vbox>
</window>'

sermo --program=DIALOG
# Sortie : NOM="Jean"  EXIT="ok"
```

### 1.1 Compatibilité

sermo est **compatible** avec les scripts gtkdialog historiques. Le paquet
`sermo-gtkdialog` (qui fournit le paquet virtuel `gtkdialog`) fait de la commande
`gtkdialog` un lien vers le backend GTK 3, de sorte que les scripts qui
appellent `gtkdialog` continuent de fonctionner. Différences visibles :

- L'apparence suit le **thème du système** en cours (clair/sombre).
- Les couleurs se spécifient au format `rgba(r,g,b,a)` ou `#RRGGBB`.
- Le terminal interactif embarqué requiert VTE (gtk3sermo, gtk4sermo) ; sur les
  autres ports, `<terminal>` montre la sortie de sa commande (voir §5).

---

## 2. Installation

### 2.1 Depuis les paquets

sermo produit des paquets Debian **séparés** — un backend par toolkit, chacun
avec le cœur lié dans son binaire. Il n'y a pas de dépôt apt : les paquets sont
joints à chaque publication, avec le fichier `SHA256SUMS` et sa signature
`SHA256SUMS.asc`. On télécharge, on vérifie, on installe :

```bash
gpg --import cle-haplo-dialog.asc            # https://haplo-dialog.fr/cle-haplo-dialog.asc
gpg --verify SHA256SUMS.asc SHA256SUMS       # « Bonne signature de haplo-dialog <devel@haplo-dialog.fr> »
sha256sum --ignore-missing -c SHA256SUMS
sudo apt install ./sermo-backend-gtk3_3.1.0-*_amd64.deb     # GTK 3
sudo apt install ./sermo-backend-fltk1_3.1.0-*_amd64.deb    # FLTK, sans tirer GTK ni Qt
sudo apt install ./sermo-gtkdialog_3.1.0-*_all.deb          # + la commande gtkdialog
```

Le `*` tient la place de la révision Debian de la publication (le chiffre
après le tiret, par exemple `3.1.0-10`) : un seul fichier de chaque paquet doit
se trouver dans le dossier.

L'empreinte de la clé doit être
`66C7 F4AD 7A58 A037 967F  8339 3799 9F99 7570 F1ED` : comparez-la à celle du
README sur GitLab et sur GitHub. Ce sont les sommes qui sont signées, pas
chaque `.deb`. Téléchargement et détails : [README.md](README.md#installer).

L'alias `sermo` (`/usr/bin/sermo`) est géré par `update-alternatives` : chaque
backend installé s'y enregistre et vous choisissez le défaut.

### 2.2 Depuis les sources

Voir [COMPILE.md](COMPILE.md). En résumé, on compile d'abord le cœur
`libsermocore`, puis le backend de son choix contre ce cœur.

### 2.3 Vérification de l'installation

```bash
echo '<window><vbox>
  <text><label>sermo fonctionne !</label></text>
  <button><label>Fermer</label><action>EXIT:ok</action></button>
</vbox></window>' | sermo --stdin
```

Une fenêtre doit apparaître. Si elle s'ouvre, l'installation est réussie.

---

## 3. Premiers pas

### 3.1 Modes d'utilisation

sermo accepte son XML de trois façons :

**Depuis stdin :**
```bash
echo '<window>...</window>' | sermo --stdin
```

**Depuis une variable d'environnement :**
```bash
export MON_DIALOG='<window>...</window>'
sermo --program=MON_DIALOG
```

**Depuis un fichier :**
```bash
sermo --file=mon_interface.xml
```

### 3.2 Récupérer les valeurs saisies

Quand le dialogue se termine par une action `EXIT:` — c'est ce que fait un
bouton —, sermo affiche sur stdout les valeurs de tous les widgets nommés, puis
la ligne `EXIT`. Pour les utiliser dans votre script :

```bash
export DIALOG='
<window title="Formulaire">
  <vbox>
    <entry><variable>PRENOM</variable></entry>
    <entry><variable>NOM_FAM</variable></entry>
    <button><label>Valider</label><action>EXIT:valide</action></button>
    <button><label>Annuler</label><action>EXIT:annule</action></button>
  </vbox>
</window>'

# --do : les valeurs arrivent comme variables d'environnement, sans jamais
# repasser par le shell. C'est la voie recommandée (voir §8.1).
sermo --program=DIALOG --do='
    if [ "$EXIT" = "valide" ]; then
        echo "Bonjour $PRENOM $NOM_FAM !"
    fi'
```

⚠️ **Fermer la fenêtre par la croix ne rend aucune valeur.** Si l'utilisateur
ferme par le gestionnaire de fenêtres (la croix, `Alt+F4`), la sortie complète
est `EXIT="abort"` : aucune ligne `PRENOM=…`. Mesuré le 2026-09-19 sur le binaire
du paquet 2.7.1. De même, l'action `closewindow:` sur la dernière fenêtre ne rend
que `EXIT="closewindow"`. Votre script doit donc traiter ces deux valeurs comme
un abandon, et ne compter sur les variables que lorsque `EXIT` porte la valeur
d'un de vos boutons.

### 3.3 Rendu PNG hors écran (`--render-png`)

Pour produire une image de l'interface **sans ouvrir de fenêtre** — capture
déterministe pour la documentation, une vignette ou un test visuel :

```bash
sermo --render-png sortie.png --file=mon_interface.xml
```

Le rendu hors écran est disponible sur **six backends graphiques** (`gtk3`,
`gtk4`, `qt6`, `fltk1`, `efl1`, `sdl3`) — `gtk3`/`gtk4`/`fltk1` via un display
virtuel (xvfb) où le backend écrit lui-même le PNG, `qt6`/`efl1`/`sdl3` via leur
plateforme offscreen. `lvgl` a aussi l'option : il dessine l'image sans ouvrir
de fenêtre. `ncurses` reste un affichage **terminal** (pas d'image ;
aperçu texte via `--print-ir`), et `html` écrit une page, pas une image.

La taille demandée par `default-width`/`default-height` est respectée sur ces
six. Jusqu'à la 2.7.1, `qt6` l'ignorait et sortait parfois une image de 2×2
pixels quand la fenêtre contenait une liste déroulante — `tests/garde_taille_fenetre.sh`
le vérifie désormais sur chaque port graphique.

---

## 4. Syntaxe de base

### 4.1 Structure d'un document

```xml
<window title="Titre de la fenêtre" resizable="true" default-width="400" default-height="300">
  <vbox>
    <text><label>Vos widgets ici</label></text>
  </vbox>
</window>
```

Tout document sermo commence par `<window>`. Les widgets sont imbriqués dans des
conteneurs (`<vbox>`, `<hbox>`, `<frame>`, `<notebook>`).

### 4.2 Attributs communs à tous les widgets

| Attribut | Valeurs | Description |
|----------|---------|-------------|
| `sensitive` | `true` / `false` | Activer/désactiver le widget |
| `visible` | `true` / `false` | Afficher/cacher le widget |
| `tooltip-text` | texte | Infobulle au survol (gtk3 et gtk4 seulement) |
| `width-request` | nombre | Largeur minimale en pixels (selon le port, voir ci-dessous) |
| `height-request` | nombre | Hauteur minimale en pixels (selon le port, voir ci-dessous) |

**`width-request` et `height-request` ne valent pas partout.** Ce que lit
chaque port (relu dans le code le 2026-10-19) :

- **gtk3, gtk4** : tous les widgets (c'est une propriété de tout widget GTK).
- **qt6, fltk1** : la plupart des widgets de saisie et d'affichage (bouton,
  saisie, liste, `<edit>`, `<drawingarea>`…), widget par widget ; pas
  `<hbox>`, `<paned>`, `<switch>`, `<spinner>`, `<calendar>` ni `<timer>`.
- **efl1** : `<window>`, `<drawingarea>`, `<paned>` (pour placer la séparation
  `position=` donnée en pixels) et `height-request` d'`<image>` (taille de
  l'icône).
- **sdl3, lvgl, ncurses, html** : `<window>` seulement, plus `height-request`
  d'`<image>` (taille de l'icône). Leur `<drawingarea>` lit les sous-éléments
  `<width>` et `<height>`.

Ailleurs, l'attribut est accepté et sans effet.

### 4.3 Balises de contenu communes

| Balise | Description |
|--------|-------------|
| `<variable>NOM</variable>` | Nom de la variable exportée à la sortie (voir §3.2) |
| `<label>texte</label>` | Étiquette affichée dans le widget |
| `<default>valeur</default>` | Valeur initiale du widget |
| `<input>commande</input>` | Commande shell dont la sortie alimente le widget |
| `<action>ACTION:arg</action>` | Action déclenchée par une interaction |
| `<sensitive>false</sensitive>` | Désactiver le widget au démarrage |
| `<width>N</width>` et `<height>N</height>` | Taille lue par certains widgets seulement : la plupart des widgets gtk3 et gtk4, `<pixmap>` de qt6 et html, `<drawingarea>` de sdl3, lvgl, ncurses et html |

**Toute balise s'écrit ouverte puis fermée.** La forme auto-fermante n'est pas
reconnue par l'analyseur : `<separator></separator>` et non `<separator/>`,
`<button ok></button>` et non `<okbutton/>`. Un conteneur vide est refusé lui
aussi : un `<vbox>` doit contenir au moins un widget.

---

## 5. Référence des widgets

### 5.1 Conteneurs

#### `<window>` — Fenêtre principale

```xml
<window title="Mon Application" resizable="true" default-width="500" default-height="400">
  <vbox>
    <text><label>Contenu de la fenêtre</label></text>
  </vbox>
</window>
```

Attributs spécifiques :

| Attribut | Lu par |
|----------|--------|
| `title` | les neuf ports |
| `default-width`, `default-height` | les neuf ports : taille de départ, en pixels |
| `resizable` | gtk3, gtk4, qt6, fltk1 |
| `decorated`, `icon-name` | gtk3 et gtk4 seulement |

⚠️ `width` et `height` posés **en attribut** sur `<window>` ne sont lus par
aucun port : la fenêtre garde sa taille naturelle. Écrivez `default-width` et
`default-height`.

gtk3 et gtk4 appliquent aussi, en propriété GTK, tout attribut qui porte le nom
d'une propriété du widget (c'est ainsi que `tooltip-text`, `decorated` ou
`icon-name` y fonctionnent) ; les autres ports ne lisent que les attributs
qu'ils connaissent.

#### `<vbox>` et `<hbox>` — Boîtes de disposition

```xml
<vbox space-expand="true" space-fill="true">
  <hbox homogeneous="false" spacing="5">
    <text><label>Gauche</label></text>
    <text><label>Droite</label></text>
  </hbox>
</vbox>
```

`<vbox>` empile verticalement, `<hbox>` horizontalement. Attributs :
`homogeneous`, `spacing`, `space-expand`, `space-fill`.

#### `<frame>` — Cadre avec titre

```xml
<frame label="Options" label-xalign="0.0">
  <vbox><text><label>Contenu du cadre</label></text></vbox>
</frame>
```

#### `<notebook>` — Onglets

```xml
<notebook tab-labels="Onglet 1|Onglet 2">
  <vbox><text><label>Contenu de l'onglet 1</label></text></vbox>
  <vbox><text><label>Contenu de l'onglet 2</label></text></vbox>
</notebook>
```

Les titres des onglets viennent de `tab-labels="A|B"`, dans l'ordre des pages.

Page affichée à l'ouverture : l'attribut `page="1"` (les pages se comptent à
partir de 0), ou un `<input>` dont la première ligne donne le numéro de la page.
Si les deux sont là, `page=` l'emporte à l'ouverture ; un `refresh:` relit
ensuite l'`<input>`. Même règle sur les 9 ports.

Variable : **l'index de la page visible** (`0` pour la première).

#### `<expander>` — Section repliable

```xml
<expander expanded="true">
  <vbox><text><label>Contenu replié</label></text></vbox>
  <label>Détails</label>
  <variable>OUVERT</variable>
</expander>
```

Une section que l'utilisateur ouvre et replie. Elle **exporte son état** :
`true` si elle est ouverte, `false` sinon.

- `expanded="true"` (ou `yes`, ou `1`) l'ouvre au démarrage. **Sans cet
  attribut, la section est repliée** — c'est le comportement de référence.
- Le titre s'écrit au choix : attribut de balise `label="Détails"`, ou élément
  `<label>Détails</label>`. Les deux marchent.
  ⚠️ **L'élément `<label>` se place APRÈS le contenu**, avec les autres
  attributs : la grammaire est `<expander>` contenu, puis attributs. Placé
  avant, l'analyseur le refuse — `syntax error near '<label>'`.
  ⚠️ La forme courte `<expander Titre>` n'existe pas ici.

#### Zone défilante — l'attribut `scrollable`

Il n'existe **pas** de balise `<scrolledwindow>`. Le défilement est un
**attribut** posé sur une boîte, `<vbox>` ou `<hbox>` :

```xml
<vbox scrollable="true" height-request="200">
  <text><label>Ligne 1</label></text>
  <text><label>Ligne 2</label></text>
  <text><label>…</label></text>
</vbox>
```

`scrollable="true"` (ou `yes`, ou `1`) enveloppe la boîte dans une zone
défilante à sa création. L'attribut n'est lu que sur `<vbox>` et `<hbox>`, et
seulement par l'automate des ports gtk3 et gtk4 ; les autres ports ne le lisent
pas. Sur un autre widget, il est sans effet.

`<tree>`, `<list>`, `<table>` et `<terminal>` n'en ont pas besoin : sur gtk3 et
gtk4, ils sont **toujours** placés dans une zone défilante.

---

### 5.2 Widgets de saisie

#### `<entry>` — Champ texte monoligne

```xml
<entry>
  <variable>TEXTE</variable>
  <default>valeur initiale</default>
  <action signal="activate">EXIT:ok</action>
</entry>
```

#### `<edit>` — Zone de texte multiligne

```xml
<edit width-request="400" height-request="200">
  <variable>CONTENU</variable>
  <default>Ligne 1
Ligne 2</default>
</edit>
```

#### `<spinbutton>` — Sélecteur numérique

```xml
<spinbutton range-min="0" range-max="100" range-step="1" digits="0">
  <variable>VALEUR</variable>
  <default>50</default>
</spinbutton>
```

#### `<hscale>` / `<vscale>` — Curseur

```xml
<hscale range-min="0" range-max="255" range-step="1" draw-value="true">
  <variable>LUMINOSITE</variable>
  <default>128</default>
</hscale>
```

---

### 5.3 Widgets de sélection

#### `<checkbox>` — Case à cocher

```xml
<checkbox>
  <label>Activer les notifications</label>
  <variable>NOTIF</variable>
  <default>true</default>
  <action>REFRESH:AUTRE_WIDGET</action>
</checkbox>
```

La variable vaut `true` ou `false`.

#### `<radiobutton>` — Bouton radio (choix exclusif)

```xml
<vbox>
  <radiobutton><label>Option A</label><variable>CHOIX_A</variable></radiobutton>
  <radiobutton><label>Option B</label><variable>CHOIX_B</variable></radiobutton>
</vbox>
```

Les boutons d'un même conteneur forment un groupe. Sans `<default>true</default>`
sur l'un d'eux, le premier est coché (sur les neuf ports depuis la 2.8.2).

#### `<comboboxtext>` — Liste déroulante simple

```xml
<comboboxtext>
  <variable>COULEUR</variable>
  <item>Rouge</item>
  <item>Vert</item>
  <item>Bleu</item>
  <default>Vert</default>
</comboboxtext>
```

#### `<list>` — Liste avec sélection

```xml
<list>
  <variable>SELECTION</variable>
  <item>Élément 1</item>
  <item>Élément 2</item>
</list>
```

Sans `<default>`, la 1re ligne d'une `<list>`, d'un `<tree>` ou d'une `<table>`
n'est choisie que si ce widget prend le focus à l'ouverture, c'est-à-dire s'il
est le premier widget focalisable de la fenêtre (visible et actif) ; sinon rien
n'est choisi. `<default>` choisit la ligne voulue. Même
règle sur les neuf ports depuis la 2.8.2.

#### `<tree>` — Arbre/tableau multi-colonnes

```xml
<tree selection-mode="single" column-header-active="true">
  <variable>LIGNE</variable>
  <label>Nom|Taille|Date</label>
  <input>ls -lh --time-style=short | awk 'NR>1{print $9"|"$5"|"$6" "$7}'</input>
</tree>
```

---

### 5.4 Boutons et actions

#### `<button>` — Bouton générique

```xml
<button>
  <label>Cliquez ici</label>
  <action>EXIT:clique</action>
</button>
```

Avec icône :
```xml
<button>
  <input file stock="gtk-open"></input>
  <label>Ouvrir</label>
  <action>FILESELECT:FICHIER</action>
</button>
```

#### Boutons de dialogue standard

```xml
<hbox>
  <button ok></button>
  <button cancel></button>
</hbox>
```

Ces boutons ont les labels et raccourcis standard et génèrent `EXIT="OK"` ou
`EXIT="Cancel"`.

Cinq formes existent : `<button ok>`, `<button cancel>`, `<button help>`,
`<button yes>` et `<button no>`. Elles s'écrivent **exactement ainsi** — une
seule espace, et une balise fermante `</button>` : l'analyseur les reconnaît
comme un tout (`gtkdialog_lexer.l`), il n'y a pas de balise `<okbutton/>`.

#### `<togglebutton>` — Bouton à bascule

```xml
<togglebutton>
  <label>Activer</label>
  <variable>ETAT</variable>
  <default>false</default>
  <action>REFRESH:AUTRE</action>
</togglebutton>
```

---

### 5.5 Affichage

#### `<text>` — Étiquette de texte

```xml
<text use-markup="true">
  <label>&lt;b&gt;Texte en gras&lt;/b&gt; et &lt;i&gt;italique&lt;/i&gt;</label>
</text>
```

⚠️ **Un `<text>` garde la fin de ligne de son `<input>`.** Avec
`<input>echo 21.5</input>` (ou un `<input mqtt>` qui reçoit `21.5`), la valeur
exportée est `"21.5⏎"`, saut de ligne compris ; un `<entry>` la retire. C'est le
comportement hérité de gtkdialog, identique sur gtk3, gtk4, qt6, fltk1, sdl3,
efl1, lvgl et ncurses (mesuré le 2026-10-18, paquets 3.1.0-6) : il est gardé pour
ne pas changer les valeurs que lisent les scripts existants. Pour une valeur sans
saut de ligne, utilisez `printf` plutôt que `echo`, ou retirez-le dans le script
qui lit la sortie (`v=${TEMPERATURE%$'\n'}`).

#### `<pixmap>` — Image

```xml
<pixmap width-request="64" height-request="64">
  <input file>/chemin/vers/image.png</input>
</pixmap>
```

#### Règle commune aux valeurs numériques

`<hscale>`, `<vscale>` et `<spinbutton>` exportent leur valeur
selon l'attribut **`digits=`** : `digits="0"` (le défaut) donne un entier,
`digits="2"` donne deux décimales. `<levelbar>` ne lit pas `digits=` : il
écrit sa valeur au plus court (`0.5`, `1`).

Le nombre s'écrit **toujours avec un point**, jamais une virgule, quelle que
soit la locale de la machine. Un script qui compare `"2.50"` obtient la même
chose partout.

`<progressbar>` **n'exporte rien** : c'est un afficheur, pas une saisie.

#### `<progressbar>` — Barre de progression

```xml
<progressbar>
  <variable>PROGRESSION</variable>
  <input>echo 75</input>
</progressbar>
```

La valeur attendue est un **pourcentage, entre `0` et `100`** — pas une
fraction. Ce manuel a longtemps annoncé « entre `0.0` et `1.0` », et son exemple
écrivait `echo 0.75` : mesuré le 2026-09-20 sur sdl3sermo, `0.75` donne une barre
à **0 %**, `75` la remplit aux trois quarts.

⚠️ **Écrivez un entier.** gtk3 lit la valeur avec `strtol` : `0.75` y vaut `0`.
Les ports neutres divisent par 100 : `0.75` y donne 0,75 % — vide à l'œil.
Une décimale ne « marche un peu » nulle part.

`<progressbar>` **n'exporte rien** : c'est un afficheur, pas une saisie. Pour une
jauge de niveau, voir `<levelbar>`, qui a ses propres `range-min` / `range-max`.

#### `<statusbar>` — Barre de statut

```xml
<statusbar>
  <variable>STATUT</variable>
  <default>Prêt</default>
</statusbar>
```

---

### 5.6 Widgets spéciaux

#### `<colorbutton>` — Sélecteur de couleur

```xml
<colorbutton>
  <variable>COULEUR_HEX</variable>
  <default>#ff6600</default>
</colorbutton>
```

#### `<fontbutton>` — Sélecteur de police

```xml
<fontbutton>
  <variable>POLICE</variable>
  <default>Sans 12</default>
</fontbutton>
```

#### `<terminal>` — Terminal embarqué

```xml
<terminal width-request="600" height-request="300">
  <variable>TERMINAL</variable>
  <input>echo "ls -la"</input>
</terminal>
```

Le terminal lance un shell (`/bin/sh`, ou les attributs `argv0`, `argv1`…). La
**sortie** de la commande `<input>` est tapée dans ce shell, comme au clavier :
ici, `ls -la` s'exécute dans le terminal. La variable rend le PID du shell.

Ce shell interactif n'existe que sur gtk3 et gtk4 (VTE), et sur qt6 construit
avec QTermWidget (le paquet Debian ne l'est pas : il ne demande pas
qtermwidget6). Ailleurs (qt6 sans QTermWidget, fltk1, efl1, sdl3, lvgl,
ncurses, html), `<terminal>` lance la commande `<input>` et affiche sa sortie,
en lecture seule ; fltk1 l'affiche dans un `Fl_Terminal`, qui rend les couleurs
ANSI.

#### `<timer>` — Minuterie

```xml
<timer milliseconds="true" interval="1000" visible="false">
  <variable>HORLOGE</variable>
  <action>REFRESH:AFFICHAGE</action>
</timer>
```

Déclenche une action à intervalle régulier : `interval` compte en secondes
entières, ou en millisecondes si `milliseconds="true"` (un booléen, pas une durée).

---

### 5.7 Menus

```xml
<menubar>
  <menu label="Fichier">
    <menuitem>
      <label>Ouvrir</label>
      <action>FILESELECT:FICHIER</action>
    </menuitem>
    <menuitemseparator></menuitemseparator>
    <menuitem>
      <label>Quitter</label>
      <action>EXIT:quitte</action>
    </menuitem>
  </menu>
</menubar>
```

### 5.8 Widgets additionnels

Disponibles sur tous les backends (natifs pour GTK, réimplémentés ailleurs) :

#### switch — Interrupteur on/off

```xml
<switch><variable>MON_SWITCH</variable><default>true</default></switch>
```

Variable : `true` quand activé, `false` sinon.

#### filechooser — Sélecteur de fichier/dossier

```xml
<filechooser>
  <label>Choisir un fichier</label>
  <variable>FICHIER</variable>
  <default>/home/user</default>
</filechooser>
```

Pour un dossier : `<filechooser action="select-folder">`.

Le tag `<chooser>`, lui, embarque le sélecteur DANS la fenêtre (GTK 3 et GTK 4).
Sur les autres ports, il devient un `<filechooser>` : un bouton qui ouvre le
dialogue du toolkit.
Variable : chemin absolu du fichier/dossier sélectionné.

Selon le port : gtk3/gtk4 et qt6 ouvrent le sélecteur de leur toolkit, `sdl3`
celui **du système** (portail XDG), `efl1` `elm_fileselector`, `ncurses` un
navigateur **dans le terminal** (flèches, `..`, `h` pour les fichiers cachés).

#### calendar — Sélecteur de date

```xml
<calendar><variable>DATE</variable><default>2026-05-21</default></calendar>
```

Variable : date au format ISO 8601 `YYYY-MM-DD`.

#### linkbutton — Bouton hyperlien

```xml
<linkbutton>
  <label>Visiter haplo-dialog</label>
  <default>https://haplo-dialog.fr</default>
  <variable>LIEN</variable>
</linkbutton>
```

Variable : l'URI (`<default>`), pas le libellé affiché. Le clic ouvre l'adresse
dans l'application par défaut.

#### searchentry — Champ de recherche

```xml
<searchentry>
  <label>Rechercher...</label>
  <variable>TERME</variable>
  <action>grep "$TERME" /var/log/syslog | head -20</action>
</searchentry>
```

#### infobar — Barre de notification

```xml
<infobar>
  <label>Opération réussie.</label>
  <default>info</default>
  <variable>STATUS</variable>
</infobar>
```

Types pour `<default>` : `info`, `warning`, `error`, `question`, `other`.

#### grid — Mise en page en tableau

```xml
<grid columns="2" row-spacing="4" column-spacing="8">
  <text><label>Nom</label></text>      <entry><variable>NOM</variable></entry>
  <text><label>Courriel</label></text> <entry><variable>MEL</variable></entry>
</grid>
```

Les enfants se rangent **en flot** : dans l'ordre du document, de gauche à
droite, retour à la ligne tous les `columns` enfants. Les colonnes sont
**alignées d'une rangée à l'autre** — c'est ce qu'un empilement de `<hbox>`
dans une `<vbox>` ne sait pas faire.

Attributs : `columns` (obligatoire en pratique ; à défaut une seule colonne et
un avertissement), `row-spacing`, `column-spacing`, `homogeneous`.
Par enfant, comme dans les boîtes : `space-expand`, `space-fill`.

⚠️ **`<grid>` n'est pas `<table>`.** `<table>` est la liste à colonnes héritée
de gtkdialog : des **données**. `<grid>` est une **mise en page**.

Variable : chaîne vide — un conteneur n'a pas de valeur propre.

#### paned — Deux zones et une poignée

```xml
<paned orientation="horizontal" position="35%">
  <list><variable>FICHIERS</variable><item>a.txt</item><item>b.txt</item></list>
  <edit><variable>CONTENU</variable></edit>
</paned>
```

Donne à l'**utilisateur** le moyen de redistribuer la place entre deux zones.
`<hbox>` fige le partage à l'écriture du script ; `<paned>` le laisse bouger.

Attributs : `orientation` (`horizontal` par défaut — deux zones côte à côte,
poignée verticale ; `vertical` — l'une au-dessus de l'autre), `position`
(position initiale, en pixels ou en `%`), `resizable="false"` (poignée figée).

⚠️ **Exactement deux enfants.** Un troisième est **refusé avec un message** sur
la sortie d'erreur — pour en mettre plus, emballer dans une `<vbox>`.

Selon le port : `GtkPaned` (gtk3, gtk4), `QSplitter` (qt6), `Fl_Tile` (fltk1),
`elm_panes` (efl1), poignée glissable en immediate-mode (sdl3). En terminal la
poignée se déplace aux **flèches** quand elle a le focus.

Variable : chaîne vide — un conteneur n'a pas de valeur propre.

#### levelbar — Jauge de niveau

```xml
<levelbar range-min="0" range-max="1"><variable>NIVEAU</variable><default>0.5</default></levelbar>
```

Une jauge : elle montre **où l'on en est** dans une plage. Pour une progression
qui avance, c'est `<progressbar>` ; pour une activité sans fin connue, `<pulse>`.

Variable : la valeur, écrite avec un **point** décimal — jamais une virgule,
quelle que soit la locale de la machine. Un script qui compare `"0.5"` obtient
la même chose partout.

#### drawingarea — Zone de dessin

```xml
<drawingarea width-request="200" height-request="120">
  <width>200</width><height>120</height>
  <variable>ZONE</variable>
</drawingarea>
```

Une surface libre, réservée au dessin. **Variable : chaîne vide.** Avec
`embed="true"`, et seulement avec gtk3 sous X11, elle vaut l'identifiant X11 de la
zone une fois affichée : c'est ce qui permet d'y afficher une vidéo
(`<action media>`, clé `dans=$ZONE`). Ce nombre change à chaque lancement.

Taille : gtk3, gtk4, qt6, fltk1 et efl1 lisent les attributs `width-request` /
`height-request` ; sdl3, lvgl, ncurses et html lisent les sous-éléments
`<width>` / `<height>`. Écrire les deux, comme ci-dessus, donne la même taille
sur les 9 ports. Sans taille, la zone fait 300 × 200 sur sdl3, lvgl, ncurses et
html, 200 × 150 sur efl1.

#### wizard — Une suite d'étapes

```xml
<wizard>
  <vbox><text><label>Étape 1</label></text></vbox>
  <vbox><text><label>Étape 2</label></text></vbox>
  <variable>ETAPE</variable>
  <action>echo "assistant terminé"</action>
</wizard>
```

Chaque enfant est une **étape** ; le tag fabrique la navigation — Précédent,
Suivant, Terminer. « Terminer » joue l'`<action>` du wizard ; il ne **ferme
rien** de lui-même, c'est au script de décider (un `<button ok>` fait ça).

En terminal, les flèches gauche/droite changent d'étape quand la rangée a le
focus. Variable : **l'index de l'étape courante**.

#### menubutton — Un menu local

```xml
<menubutton>
  <menuitem><label>Copier</label><action>…</action></menuitem>
  <menuitem><label>Coller</label><action>…</action></menuitem>
  <label>Actions</label>
  <variable>CHOIX</variable>
</menubutton>
```

Un menu **là où on est**, pas seulement en haut de fenêtre comme `<menubar>`.
Ses enfants sont des `<menuitem>` ordinaires.

⚠️ **L'ordre compte** : les `<menuitem>` d'abord, puis `<label>` et
`<variable>` — comme pour `<eventbox>`.

Variable : **le libellé du dernier élément choisi**, vide avant tout choix.

#### stack — Des pages sans onglets

```xml
<stack page="0" switcher="true">
  <vbox><text><label>Page 0</label></text></vbox>
  <vbox><text><label>Page 1</label></text></vbox>
  <variable>PAGE</variable>
</stack>
```

C'est `<notebook>` **sans les onglets** : une seule page s'affiche, et c'est le
script (ou l'assistant) qui décide laquelle. Attributs : `page` (index de la
page initiale, base 0), `switcher="true"` (ajoute une rangée de boutons
numérotés pour changer de page).

Variable : **l'index de la page visible**, comme `<notebook>`.

#### flowbox — Des enfants qui se rangent seuls

```xml
<flowbox max-children-per-line="3" column-spacing="8" row-spacing="6">
  <button><label>Un</label></button>
  <button><label>Deux</label></button>
  <button><label>Trois</label></button>
  <button><label>Quatre</label></button>
</flowbox>
```

Les enfants remplissent une ligne puis passent à la suivante. Là où `<grid>`
fixe le nombre de colonnes, celui-ci s'y adapte.

Attributs : `min-children-per-line`, `max-children-per-line`,
`column-spacing`, `row-spacing`, `selection-mode` (`none` par défaut |
`single` | `browse` | `multiple`).

Variable : **l'index de l'enfant sélectionné**, vide s'il n'y en a pas.

⚠️ **Selon le port** : gtk3 et gtk4 refluent vraiment quand la fenêtre
rétrécit (GtkFlowBox). Les autres rangent en **N colonnes fixes** — même
rendu au départ, sans reflux. Et seuls gtk3/gtk4 savent **sélectionner** :
ailleurs la variable reste vide.

#### overlay — Des enfants empilés

```xml
<overlay>
  <pixmap><input file>/usr/share/pixmaps/debian-logo.png</input></pixmap>
  <text><label>badge</label></text>
</overlay>
```

Le **premier** enfant est le fond ; les suivants flottent par-dessus. C'est le
seul conteneur du langage qui superpose.

⚠️ **En terminal, rien ne se superpose** : le port ncurses dessine le fond
puis les couches l'une sous l'autre, précédées d'un repère « · au-dessus : ».
Le contenu reste lisible, et la dégradation se voit.

Variable : chaîne vide — c'est un conteneur.

#### revealer — Un enfant qui se montre et se cache

```xml
<revealer transition="slide-down" duration="300" reveal="true">
  <text><label>Ce texte apparaît</label></text>
</revealer>
```

Attributs : `transition` (`none` | `crossfade` | `slide-left` | `slide-right` |
`slide-up` | `slide-down`), `duration` (millisecondes), `reveal` (`true` pour
démarrer visible). `<default>true</default>` fait la même chose.

⚠️ **Un seul enfant** ; un second est refusé avec un message.
⚠️ **La transition n'est animée que sur gtk3 et gtk4** : les autres ports
montrent ou cachent, sans animation. L'attribut est accepté et ignoré — c'est
dit plutôt que tu.

Variable : **`true`** ou **`false`**.

#### toolbar — Barre d'actions

```xml
<toolbar spacing="6">
  <button><label>Ouvrir</label><action>…</action></button>
  <checkbox><label>Gras</label><variable>GRAS</variable></checkbox>
</toolbar>
```

Une rangée qui **se dit** barre d'outils : le thème lui donne son fond et ses
espacements. Elle contient ce qu'on veut — boutons, cases, champs.

Attributs : `orientation` (`horizontal` par défaut | `vertical`), `spacing`.
Variable : chaîne vide — c'est un conteneur.

#### pulse — Barre d'activité indéterminée

```xml
<pulse text="Téléchargement…"><variable>ACTIVITE</variable></pulse>
```

Une barre SANS valeur : elle dit « ça travaille », pas « 42 % ». Pour un
pourcentage, c'est `<progressbar>`. Variable : la chaîne fixe `pulse`.

#### eventbox — Zone qui capte le clic

```xml
<eventbox>
  <text><label>Cliquer ici</label></text>
  <variable>ZONE</variable>
  <action>echo clic</action>
</eventbox>
```

Conteneur invisible : il emballe son contenu pour lui donner une zone
cliquable. **L'ordre compte** — le contenu d'abord, puis `<variable>` et
`<action>`. Variable : chaîne vide (c'est l'`<action>` qui porte l'effet).

---

## 6. Actions et signaux

### 6.1 Actions disponibles

| Action | Syntaxe | Description |
|--------|---------|-------------|
| `EXIT` | `EXIT:valeur` | Ferme la fenêtre, exporte EXIT=valeur |
| `CLOSEWINDOW` | `CLOSEWINDOW:NOM_WIDGET` | Ferme la fenêtre contenant le widget nommé. Le préfixe est bien `closewindow` : un préfixe inconnu n'est pas signalé, il part au shell comme commande ordinaire. |
| `LAUNCH` | `LAUNCH:NOM_VAR` | Ouvre la fenêtre décrite dans la variable d'environnement `NOM_VAR` (voir §9.2) |
| `REFRESH` | `REFRESH:NOM_WIDGET` | Relance l'`<input>` d'un widget |
| `SAVE` | `SAVE:NOM_WIDGET` | Sauvegarde l'état d'un widget |
| `CLEAR` | `CLEAR:NOM_WIDGET` | Vide le contenu d'un widget |
| `APPEND` | `APPEND:SOURCE,CIBLE` | Copie la valeur du widget `SOURCE` dans le widget `CIBLE` : ajout d'une ligne pour une liste ou un arbre, texte inséré pour un champ |
| `INSERT` | `INSERT:SOURCE,CIBLE` | Même effet qu'`APPEND` (le cœur appelle la même fonction) |
| `REMOVESELECTED` | `REMOVESELECTED:NOM_WIDGET` | Retire la ou les lignes choisies d'une liste ou d'un arbre |
| `FILESELECT` | `FILESELECT:NOM_VAR` | Ouvre un sélecteur de fichier |
| `ENABLE` / `DISABLE` | `ENABLE:NOM_WIDGET` | (Dés)active un widget |
| `SHOW` / `HIDE` | `SHOW:NOM_WIDGET` | (In)visibilise un widget |
| `GRABFOCUS` | `GRABFOCUS:NOM_WIDGET` | Donne le focus clavier |
| `PRESENTWINDOW` | `PRESENTWINDOW:NOM` | Met la fenêtre au premier plan |
| `ACTIVATE` | `ACTIVATE:NOM_WIDGET` | Active le widget, comme un clic sur un bouton ou la touche Entrée dans un champ : ses propres actions jouent |
| `COMMAND` | `COMMAND:commande` | Lance la commande, comme une action sans préfixe (§6.3) |
| `BREAK` | `BREAK:` | Arrête la liste : les `<action>` suivantes du même signal ne jouent pas |
| `PULSE` | `PULSE:NOM_WIDGET` | Fait avancer d'un pas le mouvement d'une `<progressbar>` (mode « activité », sans pourcentage) |
| `START` / `STOP` | `START:NOM_WIDGET` | Lance / arrête l'animation d'un `<spinner>` |

Le préfixe s'écrit en majuscules ou en minuscules (`exit:` vaut `EXIT:`). Il
peut aussi se donner par l'attribut `function` : `<action
function="refresh">NOM</action>` vaut `<action>refresh:NOM</action>`. Les
`<action>` d'un widget jouent dans l'ordre du document. Après un `LAUNCH:`, les
actions suivantes ne jouent pas : la nouvelle fenêtre prend la main.

### 6.2 Signaux disponibles

Par défaut, `<action>` réagit au signal principal du widget (clic pour un
bouton). Pour d'autres signaux :

```xml
<entry>
  <action signal="activate">EXIT:ok</action>         <!-- Touche Entrée -->
  <action signal="changed">REFRESH:APERCU</action>   <!-- À chaque frappe -->
</entry>
```

Signaux courants : `activate`, `changed`, `clicked`, `toggled`,
`value-changed`, `cursor-changed`, `select-row`.

### 6.3 Exécuter une commande shell

```xml
<button>
  <label>Ouvrir le navigateur</label>
  <action>xdg-open https://exemple.com</action>
</button>
```

Toute action non reconnue comme mot-clé est exécutée comme commande shell via
`safe_system()` du cœur.

Une commande **sans** syntaxe shell est lancée directement, sans shell. Une
commande qui en contient (`|`, `;`, `$`, `>`…) passe par un shell :

- le shell par défaut : `/bin/sh` ; sous Windows, celui que désigne
  `%ComSpec%` (d'habitude `cmd.exe`) ;
- ou le programme que désigne la variable **`SERMO_SHELL`**, par son chemin
  absolu (par exemple `SERMO_SHELL=/bin/bash`). Une autre valeur (chemin relatif,
  fichier non exécutable) est refusée, un message le dit, et le shell par défaut
  sert.

Avec `SERMO_NO_SHELL_FALLBACK` ou `SERMO_ALLOWED_CMDS` posée, une commande qui
contient de la syntaxe shell est **refusée** : elle ne passe par aucun shell, et
un message le dit sur la sortie d'erreur.

**Sous Windows**, le shell par défaut est `cmd.exe` : les commandes s'écrivent en
syntaxe cmd (`echo %NOM%`, `&` pour enchaîner). Les commandes internes de cmd
(`echo`, `dir`, `cd`…) passent aussi par lui. Pour des commandes écrites pour
`sh`, faites désigner à `SERMO_SHELL` un shell Unix, par exemple busybox-w32
copié sous le nom `sh.exe` (sermo ne le fournit pas) :

```bat
set SERMO_SHELL=%CD%\sh.exe
ncursessermo.exe --file=dialogue.xml
```

Dans les valeurs rendues sur la sortie, une barre oblique inverse s'écrit
doublée (`C:\\Users\\…`) : la ligne peut ainsi être relue par un script sans
risque. Voir aussi [SECURITY.md](SECURITY.md), « Hors de Linux ».

### 6.4 Actions conditionnelles

```xml
<button>
  <label>Action selon état</label>
  <action condition="command_is_true(echo $CASE)">REFRESH:WIDGET_A</action>
  <action condition="active_is_false(OPTION)">REFRESH:WIDGET_B</action>
</button>
```

Ici, `WIDGET_A` n'est rafraîchi que si `CASE` vaut un nombre non nul (ou
`true`, `yes`), et `WIDGET_B` que si la case `OPTION` n'est pas cochée.

L'attribut `condition` décide si l'action joue. Cinq formes, chacune avec son
contraire en `_false` :

| Condition | Vraie quand |
|-----------|-------------|
| `command_is_true(commande)` | la **première ligne affichée** par la commande vaut `true`, `yes` ou un nombre non nul. Ce n'est pas le code de retour : une commande qui n'affiche rien donne « faux ». |
| `command_is_false(commande)` | cette première ligne vaut `false`, `no` ou `0` |
| `file_is_true(chemin)` / `file_is_false(chemin)` | même règle, sur la première ligne du fichier |
| `active_is_true(NOM)` / `active_is_false(NOM)` | le widget `NOM` est coché, enfoncé ou déplié (case, bouton à bascule, interrupteur, `<expander>`) |
| `sensitive_is_true(NOM)` / `sensitive_is_false(NOM)` | le widget `NOM` est actif (non grisé) |
| `visible_is_true(NOM)` / `visible_is_false(NOM)` | le widget `NOM` est visible |

Une condition inconnue fait jouer l'action quand même. Elle n'est signalée sur
la sortie d'erreur que si elle porte des parenthèses ; sans parenthèses, elle
est ignorée sans message (seuls `active_is_true` et `active_is_false` sont
admis nus, pour un `<timer>`). Un `NOM` inconnu rend la
condition fausse. Sur un port qui ne sait pas dire l'état d'un widget,
`active_is_true` et `active_is_false` sont toutes deux fausses, et
`sensitive_is_…` / `visible_is_…` le voient actif et visible.

Sur une case, un bouton radio, un bouton à bascule, un interrupteur
(`<switch>`), un `<expander>` ou une entrée de menu à cocher, une action peut
aussi commencer par `if true ` ou `if false ` : elle ne joue que si le widget
vient d'être coché, ou déplié (ou le contraire). Avec un attribut `condition`, les deux doivent être
vrais.

---

## 7. Variables et entrées/sorties

### 7.1 Nommer un widget

```xml
<entry><variable>MA_VALEUR</variable></entry>
```

À la fermeture, sermo émet sur stdout : `MA_VALEUR="contenu saisi"`.

### 7.2 Alimenter un widget depuis une commande

```xml
<text>
  <variable>DATE_HEURE</variable>
  <input>date "+%H:%M:%S"</input>
</text>
```

La commande est relancée à chaque `REFRESH:DATE_HEURE`.

### 7.3 Alimenter depuis un fichier

```xml
<edit><input file>/etc/hostname</input></edit>
```

Ce qu'un `<input>` lit — commande ou fichier — s'arrête à **16 Mio**, avec un
avertissement sur la sortie d'erreur. La variable d'environnement
`SERMO_INPUT_MAX` change cette limite, en octets ; `0` la retire. La barre de
progression n'est pas plafonnée. Voir
[SECURITY.md](SECURITY.md#taille-de-ce-que-lit-un-input).

#### Lire un automate : `<input modbus>`, `<input opcua>`

```xml
<levelbar range-min="0" range-max="100">
  <variable>TEMP</variable>
  <input modbus>tcp://automate:502?esclave=1&registre=1&diviseur=10</input>
</levelbar>
<text>
  <variable>PIECES</variable>
  <input opcua>opc.tcp://serveur:4840?noeud=ns=1;s=pieces</input>
</text>
```

Une **adresse tout-en-un** décrit ce qu'il faut lire ; le widget reçoit la
valeur, comme d'une commande. **Lecture seule** : rien n'est jamais écrit vers
l'équipement. Ces balises demandent leur paquet : `sermo-source-modbus` ou
`sermo-source-opcua` (programmes d'aide de `/usr/libexec/sermo`).

| Modbus (`tcp://hôte[:port]`, port 502 par défaut, ou `rtu:///dev/ttyUSB0`) | |
|---|---|
| `registre` | obligatoire, numéroté à partir de 1 (1 à 65536 ; la lecture ne doit pas dépasser le registre 65536) |
| `type` | `maintien` (défaut), `entree`, `bobine`, `discret` |
| `esclave`, `nombre` | défauts 1 et 1 (une valeur par ligne) ; `esclave` de 0 à 255 (de 1 à 247 en RTU), `nombre` de 1 à 125 |
| `format`, `ordre` | `entier` (défaut), `signe`, `entier32`, `signe32`, `flottant` ; `abcd` (défaut), `cdab`, `badc`, `dcba` (ni l'un ni l'autre pour `bobine` et `discret`) |
| `diviseur`, `delai` | défauts 1 (un nombre positif, point décimal) et 1500 ms (de 10 à 60 000) |
| `cache`, `perime` | fraîcheur du cache commun, défaut 900 ms, de 0 à 60 000 (`0` : lecture directe, rien gardé) ; ancienneté au-delà de laquelle une valeur n'est plus montrée, défaut 10 000 ms, de 0 à 600 000 (un `perime` plus petit que `cache` compte comme `cache`) |
| `vitesse`, `parite`, `bits`, `arret` | RTU seulement : défauts 19200 (de 50 à 4 000 000), `E` (`N`, `E` ou `O`), 8 (7 ou 8), 1 (1 ou 2) |

| OPC UA (`opc.tcp://hôte:port`) | |
|---|---|
| `noeud` | obligatoire : `ns=1;s=nom`, `ns=0;i=2258`… |
| `delai`, `cache`, `perime` | défauts 1500, 900 et 10 000 ms ; mêmes bornes que Modbus |
| `mode` | `none` (défaut), `sign`, `chiffre` |
| `certificat`, `cle`, `confiance`, `uri` | avec `sign`/`chiffre` seulement (refusés avec `none`) : fichiers du poste, certificat(s) de serveur admis (répétable, 16 au plus), ApplicationUri (défaut `urn:sermo:opcua-lire`). `certificat`, `cle` et au moins une `confiance` sont **obligatoires** avec `sign` et `chiffre` |

Un paramètre inconnu est refusé (une faute de frappe ne passe pas pour la
valeur par défaut). Une adresse fait 4096 octets au plus, avec 32 paramètres au
plus ; une adresse vide, trop longue ou qui contient un caractère de contrôle est
refusée à la lecture du dialogue. Un `&` ou un `%` voulu dans une valeur s'écrit `%26` ou
`%25`. Après un échec, le même équipement n'est plus interrogé pendant 5 s : un
automate injoignable ne fige pas la fenêtre à chaque rafraîchissement. Une
réponse est gardée 900 ms (`cache=`) et les lectures vers un même équipement
passent une à une : une jauge et son texte sur la même adresse ne font qu'UNE
connexion par tour. Des adresses différentes en font une chacune : pour
beaucoup de valeurs, préférer un seul `<input modbus>` avec `nombre=` dans un
`<tree>`.

**La fenêtre ne se fige jamais.** Une valeur gardée depuis moins de 10 s
(`perime=`) est montrée tout de suite, et relue en arrière-plan : le tour
suivant montre la nouvelle. Au-delà, la case se vide plutôt que de montrer une
valeur ancienne comme si elle était présente. Seule la **toute première**
lecture d'une adresse attend l'équipement (au plus `delai=`), pour qu'un
dialogue sans minuteur ait sa valeur. Si un équipement lent est lu à beaucoup
d'adresses, un tour complet peut dépasser `perime=` et les valeurs se vider :
regroupez-les (`nombre=`) ou augmentez `perime=`. Avec `SERMO_ALLOWED_CMDS`,
nommer le programme d'aide (`/usr/libexec/sermo/sermo-source-modbus`) ;
`SERMO_SOURCE_DIR` change son dossier.

#### Lire la domotique : `<input mqtt>`

```xml
<text>
  <variable>SALON</variable>
  <input mqtt>mqtt://serveur:1883?sujet=maison/salon/temperature</input>
</text>
```

Le widget montre le message d'un sujet MQTT : le message **retenu** s'il y en
a un (il arrive tout de suite), sinon le prochain publié, attendu au plus
`delai=`. Même mécanique que Modbus et OPC UA (cache, `perime=`, pause de 5 s
après un échec, fenêtre jamais figée). **Lecture seule** : le programme d'aide
s'abonne et lit, il ne publie jamais. Paquet : `sermo-source-mqtt`.

| MQTT (`mqtt://hôte[:port]`, port 1883 ; chiffré : `mqtts://hôte[:port]`, port 8883) | |
|---|---|
| `sujet` | obligatoire : un sujet précis ; les jokers `+` et `#` sont refusés (un widget montre UNE valeur) |
| `delai`, `cache`, `perime` | défauts 1500, 900 et 10 000 ms ; mêmes bornes que Modbus |
| `identifiants` | chemin **absolu** d'un fichier qui contient `utilisateur:mot de passe` sur sa première ligne. Sur `mqtt://` (non chiffré), refusé sans `clair=oui` |
| `clair` | `mqtt://` seulement, seule valeur admise : `oui`. Permet `identifiants=` sans chiffrement (voir plus bas) |
| `confiance` | `mqtts://` : **obligatoire**, certificat (PEM) de l'autorité qui a signé le serveur ; le nom du serveur est vérifié contre son certificat |
| `certificat`, `cle` | `mqtts://`, ensemble : certificat et clé du poste, pour un serveur qui en exige un ; clé en `chmod 600` |

Un message de plus de 64 Kio (65 536 octets) est refusé, et c'est dit ; un
message vide aussi.

**Le mot de passe n'est jamais dans l'adresse** : l'adresse est un argument de
programme, que tout utilisateur de la machine peut lire (`ps`). Il vit dans le
fichier d'`identifiants`, qui est **refusé** s'il n'est pas à vous ou si un
autre peut le lire : `chmod 600 fichier`. En `mqtts://`, il n'y a **aucun mode « tout
accepter »** : sans `confiance=`, la lecture est refusée, et un serveur dont le
certificat n'est pas signé par cette autorité, ou porte un autre nom, est refusé.

**Pas de mot de passe en clair sans le dire.** Sur `mqtt://`, rien n'est chiffré :
le mot de passe passerait en clair sur le réseau. `identifiants=` y est donc
**refusé avant toute connexion**, et un message propose `mqtts://`. Sur un réseau
local de confiance, l'auteur du dialogue peut l'accepter lui-même en ajoutant
`clair=oui` à l'adresse :

```xml
<input mqtt>mqtt://serveur?sujet=maison/salon/temperature&identifiants=/chemin/vers/mqtt-id&clair=oui</input>
```

`clair=` n'admet que `oui`, et n'est admis qu'avec `mqtt://` (avec `mqtts://`, il
est refusé : la connexion est déjà chiffrée). La règle vaut pour la lecture
(`<input mqtt>`) et pour l'écriture (`<action mqtt>`).

#### Commander la domotique : `<action mqtt>`

```xml
<button><label>Allumer le salon</label>
  <action mqtt>mqtt://serveur?sujet=maison/salon/lampe&valeur=on</action>
</button>
<hscale range-min="15" range-max="25"><variable>CONSIGNE</variable></hscale>
<button><label>Régler</label>
  <action mqtt>mqtt://serveur?sujet=maison/salon/consigne&valeur=$CONSIGNE</action>
</button>
```

Un clic publie **une** valeur sur **un** sujet. C'est une des trois écritures de
sermo vers un équipement, avec `<action mavlink>` et `<action media>`, et elle
est encadrée :

- **seulement un clic** : `<action mqtt>` n'est accepté que dans un `<button>`
  ou un `<menuitem>`. Dans un `<timer>`, un champ, une case… (qui peuvent jouer
  leurs actions sans clic), le dialogue est **refusé à la lecture** ;
- **liste blanche fermée**, dans un fichier écrit exprès :
  `~/.config/sermo/cibles-mqtt` (à vous, `chmod 600`) et
  `/etc/sermo/cibles-mqtt` (à root, `chmod 644`). Sans liste, rien ne part. Un
  fichier mal protégé est refusé en entier ;
- **valeurs bornées** par sujet : une liste (`on|off`) ou un intervalle
  (`18..24`) ; une valeur hors liste est refusée, et c'est dit ;
- **paquet à part** : `sermo-cible-mqtt`. Sans lui, sermo lit la domotique
  mais ne publie jamais.

Une ligne de la liste : `<serveur> <sujet> <valeurs>` (`#` commente).

```
mqtt://serveur maison/salon/lampe     on|off
mqtt://serveur maison/salon/consigne  18..24
```

| Clé de `<action mqtt>` | |
|---|---|
| `sujet` | obligatoire, un sujet précis (jokers `+` et `#` refusés) |
| `valeur` | obligatoire : la valeur à publier, ou `$NOM` |
| `delai` | délai total (connexion et accusé du serveur), défaut 1500 ms, de 10 à 60 000 |
| `identifiants`, `clair`, `confiance`, `certificat`, `cle` | comme `<input mqtt>` |

`valeur=$NOM` prend la variable `NOM` du dialogue (un curseur, une liste),
contrôlée comme une valeur écrite en dur. Publication en QoS 1, **non retenue**
(le serveur ne la garde pas pour les abonnés à venir), jamais répétée.

**Limite, dite franchement** : un auteur de dialogue peut toujours lancer une
commande à lui (par exemple `mosquitto_pub`) dans une action. Ce cadre garantit
que **sermo lui-même** n'écrit jamais sans un clic ni hors de votre liste ; il ne
protège pas contre un dialogue malveillant. Le nom `sermo-cible-` écrit à la
main dans un `<input>` ou une `<action>` est refusé.

#### Lire un drone : `<input mavlink>`

```xml
<levelbar range-min="0" range-max="100">
  <variable>BATT</variable>
  <input mavlink>udp://:14550?champ=batterie</input>
</levelbar>
<text><variable>HAUT</variable><input mavlink>udp://:14550?champ=hauteur</input></text>
```

Le widget montre une valeur de télémétrie de l'**autopilote** d'un drone
(protocole MAVLink). **Lecture seule** : rien n'est émis sur le lien — ni
commande, ni battement, ni demande de flux. Paquet : `sermo-source-mavlink`.

| Liaison | |
|---|---|
| `udp://[hôte]:port` | écoute UDP (défaut `127.0.0.1` : un relais local comme MAVProxy envoie là ; `udp://0.0.0.0:14550` pour tout le réseau) |
| `tcp://hôte:port` | connexion TCP (simulateur SITL : `5760`) |
| `serie:///dev/ttyUSB0` | radio USB ; `vitesse=` (défaut 57600 ; admises : 9600, 19200, 38400, 57600, 115200, 230400, 460800, 921600 ; refusée hors de `serie://`) |

| Clé | |
|---|---|
| `champ` | obligatoire : `tension` (V), `courant` (A), `batterie` (%), `latitude`, `longitude`, `altitude` (m, niveau de la mer), `hauteur` (m, au-dessus du départ), `cap` (°), `vitesse` (m/s), `montee` (m/s), `satellites`, `fix`, `arme` (1/0), `mode` |
| `systeme` | le système à lire (1 à 255) ; sans lui, le premier autopilote entendu |
| `delai`, `cache`, `perime` | comme Modbus (défauts 1500, 900 et 10 000 ms) |

Seul l'autopilote est écouté : une station sol, une caméra ou une nacelle qui
parlent sur le même lien sont ignorées. Une valeur que l'autopilote dit « non
mesurée » est refusée, et c'est dit. **Limite** : sermo n'émettant rien, il ne
demande pas de flux ; sur une radio seule, sans station sol ni relais, seul le
battement (`arme`, `mode`) arrive. Le remède est côté drone : sur ArduPilot
4.7, les réglages `MAV1_EXT_STAT`, `MAV1_POSITION` et `MAV1_EXTRA2` (par
exemple à 2, deux envois par seconde) font diffuser batterie, position et
vitesse sur le premier lien ; avant la 4.7, ils s'appelaient `SR0_…`, et les
anciens noms sont ignorés sans un mot. Exemple complet et faux drone (sans
matériel) : `examples/mavlink/`.

#### Commander un drone : `<action mavlink>`

```xml
<button><label>Retour au départ</label>
  <action mavlink>tcp://127.0.0.1:5760?ordre=rtl</action>
</button>
<hscale range-min="1" range-max="15"><variable>VITESSE</variable></hscale>
<button><label>Régler la vitesse de mission</label>
  <action mavlink>tcp://127.0.0.1:5760?parametre=WP_SPD&valeur=$VITESSE</action>
</button>
```

Un clic règle **un** paramètre de l'autopilote, lui donne **un** ordre de
sécurité, ou lui envoie **une** mission. Pas de pilotage : ni décollage, ni
armement, ni changement de mode, ni déplacement commandé autre que les ordres
de sécurité (`rtl` ramène le drone à son départ, `land` le pose sur place). Une
mission est envoyée, **jamais lancée**. Même cadre que `<action mqtt>` :

- **seulement un clic** : dans un `<button>` ou un `<menuitem>`, refusé
  ailleurs à la lecture du dialogue ;
- **liste blanche fermée** : `~/.config/sermo/cibles-mavlink` (à vous,
  `chmod 600`) et `/etc/sermo/cibles-mavlink` (à root, `chmod 644`) ;
- **paquet à part** : `sermo-cible-mavlink`.

Une ligne de la liste : `parametre NOM min..max`, `ordre rtl`, `ordre land`,
`ordre terminaison`, ou `mission rayon R hauteur H` (`#` commente).

```
parametre WP_SPD 1..15
ordre rtl
ordre land
```

| Clé | |
|---|---|
| `parametre` + `valeur` | un nom exact (`A-Z`, `0-9`, `_`, 16 caractères au plus) et un nombre (point décimal), ou `$NOM` (variable du dialogue) |
| `ordre` | `rtl` (retour au départ), `land` (atterrir sur place), `terminaison` (couper les moteurs) |
| `mission` | chemin absolu d'un fichier de mission (points de passage, voir plus bas) |
| `systeme`, `vitesse` | comme `<input mavlink>` |
| `delai` | délai total en ms (défaut 5000, 20 000 pour une mission ; de 2000 à 60 000) |

Une adresse donne **exactement une** des clés `parametre`, `ordre` ou
`mission` ; `valeur` ne va qu'avec `parametre`.

Les garde-fous, dans l'ordre où ils jouent :

- **familles jamais réglées**, ni dans l'adresse, ni dans la liste : `ARMING_*`,
  `FLTMODE*`, `SERVOn_FUNCTION`, `BRD_SAFETY*`, `FENCE_*` (armement, modes,
  sorties moteur, interrupteur de sécurité, barrière). Une liste qui en
  contient une est refusée en entier ;
- **liste blanche** : la permission doit y être (pour un paramètre, avec la
  valeur dans son intervalle) ; pour une mission, la hauteur de chaque point est
  vérifiée ici, avant tout réseau ;
- **couper les moteurs se confirme** : le premier appui n'envoie rien, n'ouvre
  même pas la liaison, et le dit ; un second appui **dans les 5 s**, vers le
  même drone (même liaison, même `systeme=`), envoie l'ordre. Le drone tombe.
  La confirmation demande `$XDG_RUNTIME_DIR` (chemin absolu, dossier à vous
  seul) : sans lui, rien n'est envoyé ;
- **un seul émetteur** : sermo attend le battement de l'autopilote, lui envoie
  une **demande de lecture** (ArduPilot ne relaie les messages des autres liens
  que vers un lien qui lui a déjà parlé), puis écoute un peu plus d'une seconde ;
  si une autre station sol parle déjà au drone, rien n'est réglé ni ordonné ;
- **réglage au sol seulement** : un paramètre n'est réglé, et une mission
  envoyée, que si le drone est désarmé, d'après son battement lu juste avant
  l'envoi ; le paramètre doit exister sur l'autopilote, et un entier refuse une
  valeur décimale ;
- **preuve du drone** : un réglage ne réussit que si le drone renvoie la même
  valeur ; un ordre, que s'il répond « accepté ». Une seule tentative, jamais
  répétée.

sermo n'envoie jamais de battement : ArduPilot surveille celui de sa station
sol pour déclencher une sécurité, et un battement qui s'arrête à la fin de
l'envoi pourrait la déclencher en vol.

**Envoyer une mission** (des points de passage, drone au sol) :

```xml
<button><label>Envoyer le vol d'inspection</label>
  <action mavlink>tcp://127.0.0.1:5760?mission=/srv/vols/toiture.waypoints</action>
</button>
```

Le fichier est celui qu'enregistrent Mission Planner ou QGroundControl
(« QGC WPL 110 »), chemin absolu, à vous, que personne d'autre ne peut
modifier. Seuls des **points de passage** sont acceptés : hauteur au-dessus du
départ, attente au point de 0 à 60 s. Un décollage, un atterrissage, un saut,
un changement de vitesse ou un ordre caméra refusent le fichier entier, et
c'est dit. 100 points au plus. Le fichier fait 256 Kio au plus, et un lien
symbolique est refusé. La liste blanche borne la mission :

```
mission rayon 500 hauteur 120
```

Chaque point doit être à 500 m au plus de la **position actuelle du drone**,
et entre 0 et 120 m de hauteur. Dans la liste, `rayon` va de 1 à 10 000 m et
`hauteur` de 1 à 120 m ; avec deux lignes `mission`, la plus stricte compte. Après l'envoi,
sermo **relit toute la mission sur le drone** et la compare point par point :
le succès n'est dit que si elle est identique. La mission n'est **pas
lancée** : la lancer, c'est piloter. Sans position du drone (sur ArduPilot
4.7 : `MAV1_POSITION`), rien n'est envoyé. `delai` vaut 20 000 ms par défaut
pour une mission.

Chaque appel écrit sa conclusion, une ligne, sur la sortie d'erreur et dans
`$XDG_RUNTIME_DIR/sermo-cible-mavlink/message` (si `XDG_RUNTIME_DIR` est un
chemin absolu ; sinon, sur la sortie d'erreur seulement). Un dialogue la montre avec
`<input file>` et un `refresh:` après l'action (voir
`examples/mavlink/drone-commande.sh`).

**Essayé contre un vrai autopilote simulé** (ArduPilot Copter 4.7.2 en SITL) :
réglage relu, refus en vol, retour au départ, atterrissage, coupure confirmée.
**Pas essayé sur un vrai drone.** Les noms de paramètres changent d'une version
d'ArduPilot à l'autre (`WPNAV_SPEED` est devenu `WP_SPD`) : un nom inconnu du
drone est refusé, et c'est dit.

#### Lire un son ou une vidéo : `<input media>` / `<action media>`

Un bouton lance un lecteur (mpv), d'autres le commandent, et un `<input>` lit son
état. Deux programmes d'aide, chacun dans son paquet : `sermo-cible-media` (les
commandes) et `sermo-source-media` (l'état). Paquets : `sermo-cible-media`,
`sermo-source-media`, et `mpv` (avec `bubblewrap` pour les flux réseau).

```xml
<button><label>Lire</label>
  <action media>media://salon?commande=lire&amp;source=file:///srv/sons/annonce.ogg</action></button>
<button><label>Pause</label><action media>media://salon?commande=pause</action></button>
<hscale range-min="0" range-max="100"><variable>VOLUME</variable></hscale>
<button><label>Volume</label><action media>media://salon?commande=volume&amp;valeur=$VOLUME</action></button>
<text><variable>ETAT</variable><input media>media://salon?donnee=etat</input></text>
<timer interval="1" visible="false"><action>refresh:ETAT</action></timer>
```

- **Nom du lecteur** (`salon`) : lettres, chiffres, `_` et `-`, 32 au plus. Un mpv
  par nom, **4 lecteurs au plus** à la fois. Relancer `lire` sur un nom remplace la
  lecture en cours.
- **Commandes** (liste fermée) : `lire&source=…`, `pause`, `reprendre`, `arreter`,
  `aller&valeur=SECONDES` (0 à 86400), `volume&valeur=0..100`, `muet`, `son`.
  `valeur=$NOM` prend la variable `NOM` du dialogue (un curseur, un champ).
- **Données** de `<input media>` : `donnee=etat` (défaut) vaut toujours `stopped`,
  `playing`, `paused` ou `error` ; `position` et `duree` en secondes (`12.3`), vides
  quand rien ne joue.
- **Sources** : `file:///chemin` (fichier ordinaire lisible), `http://`, `https://`,
  `rtsp://`, `rtsps://`, 2048 octets au plus. **Refusé** : tout autre schéma (sauf `v4l2://` pour une caméra, ci-dessous) (mpv
  accepterait `edl://`, `av://lavfi:…` : pas sermo), une adresse avec identifiants
  (`u:mot@hôte`).
- **Liste de lecture** : elle n'est pas refusée, mais mpv ne suit pas ses entrées
  non sûres (il est lancé avec `--load-unsafe-playlists=no` et
  `--access-references=no`).
- **Caméra** : `source=v4l2:///dev/video0`, avec en option `taille=` (`320x240`,
  `640x480` par défaut, `1280x720`, `1920x1080`) et `format=` (`mjpeg` par défaut,
  `yuyv422`). Le nœud doit être une vraie caméra (pas un nœud de métadonnées).
  La fenêtre de mpv est titrée « sermo — CAMÉRA ACTIVE — <nom> ». Si `bwrap` est
  installé, la caméra seule est visible dans le bac à sable, sans réseau. `lire` ne répond qu'à la première
  image reçue. Une caméra **occupée** par un autre programme est
  refusée après une seconde tentative, sans rien forcer ; mpv ne distingue pas
  « occupée » d'« indisponible », le message dit donc les deux.
- **Dans la fenêtre** : `dans=$ZONE` (la variable d'un `<drawingarea embed="true">`) affiche
  l'image dans cette zone au lieu d'une fenêtre à part. Seulement avec gtk3 sous
  X11. Ailleurs (Wayland, autres ports, zone pas encore affichée), la variable est
  vide : la vidéo s'ouvre dans sa propre fenêtre, et le message le dit. Une caméra
  incrustée porte « CAMÉRA ACTIVE — <nom> » écrit dans l'image. Un `dans=` qui
  n'est pas de la forme `$NOM`, ou une variable dont la valeur n'est pas un
  nombre (de 1 à 2³²−1), est refusé : code 1, mpv n'est pas lancé.
  ```xml
  <drawingarea width-request="320" height-request="240" embed="true"><variable>ECRAN</variable></drawingarea>
  <button><label>Lire</label>
    <action media>media://film?commande=lire&source=file:///chemin/film.mp4&dans=$ECRAN</action>
  </button>
  ```
- **Flux HTTP(S) à mot de passe** : `identifiants=/chemin` d'un fichier à vous, en
  mode **exactement** `600` (`chmod 600`), avec une ligne `utilisateur:mot de passe`. Il est passé à mpv par un
  fichier d'en-tête privé, effacé après lecture, **jamais en argument**. RTSP à mot
  de passe : non pris en charge (mpv ne le lit que dans l'adresse).
- **Bac à sable** : un flux réseau ne se lit **que** dans `bwrap` (refusé sans) ; un
  fichier local ou une caméra y est lu si `bwrap` est installé, réseau coupé. Sans
  `bwrap`, un fichier local ou une caméra est lu **sans cage**, et seul un message
  le dit (« bwrap absent,
  mpv tourne sans bac à sable »). Dans la cage, le son passe par pulse
  (`--ao=pulse,null`) : sous PipeWire, `pipewire-pulse` est nécessaire, sinon la
  lecture se fait sans son.
- **`XDG_RUNTIME_DIR`** doit être posé, en chemin absolu : chaque lecteur y a son
  dossier privé (`sermo-media/NOM/`). Sans lui, aucun lecteur ne se lance (un
message le dit), et `<input media>` répond `stopped`.
- **Un geste** : `<action media>` n'est permise que sur un bouton ou une entrée de
  menu (pas de lecture automatique, pas de minuteur qui lance un son).
- **Pas d'évènement de fin** : un `<timer>` qui relit `etat` voit `playing` devenir
  `stopped`.
- Quand la fenêtre se ferme (même si sermo est tué), le lecteur s'arrête. Une vidéo
  s'affiche dans la fenêtre de mpv, sauf avec `dans=` (gtk3, X11).

Exemple complet : `examples/lecteur-media/`.

### 7.4 Inclusion d'un fichier de fonctions

```bash
sermo --include=/chemin/fonctions.sh --program=DIALOG
```

Permet d'utiliser des fonctions shell définies dans `fonctions.sh` dans les
attributs `<input>` et `<action>`. Un chemin relatif part du dossier où le
programme a été lancé. Le fichier est lu **devant chaque commande** : n'y mettez
que des fonctions, pas le script lui-même (voir §9.1).

### 7.5 Une interface dessinée avec Glade (gtk3, gtk4)

```bash
gtk3sermo --glade-xml=interface.ui --program=fenetre_principale
```

`--glade-xml` remplace le XML de sermo par un fichier d'interface GtkBuilder, le
format qu'enregistre Glade. Seuls gtk3sermo et gtk4sermo le chargent, chacun
dans le format de sa version de GTK ; les autres ports refusent l'option.

- La fenêtre affichée est l'objet dont `--program` donne l'identifiant
  (`MAIN_WINDOW` par défaut).
- Chaque widget qui a un identifiant devient une variable de ce nom, exportée à
  la sortie comme les autres.
- Un gestionnaire de signal n'est pas une fonction C mais une **action sermo** :
  `<signal name="clicked" handler="exit:OK"/>`, ou une commande shell, soumise aux
  mêmes règles que `<action>` (`SERMO_ALLOWED_CMDS`, `--include`…).
- Sur `realize`, la sortie du gestionnaire **remplit** le widget, comme `<input>`.

Exemple complet, en GTK 3 et en GTK 4 : `examples/glade/`.

---

## 8. Exemples pratiques

### 8.1 Boîte de confirmation

> ⚠️ **`eval` et les valeurs saisies.** Les exemples ci-dessous passent la
> sortie du programme à `eval`. C'est l'usage historique de gtkdialog — mais la
> valeur d'un champ est **tapée par la personne qui se sert du dialogue**, pas
> forcément celle qui a écrit le script.
>
> sermo échappe les quatre caractères que le shell développe entre guillemets
> doubles (`\`, `"`, `$` et l'accent grave) : `eval` ne les exécute plus, et
> `tests/garde_echappement_sortie.sh` le vérifie à chaque poussée.
>
> Si le dialogue peut être utilisé par quelqu'un d'autre que vous, préférez
> `--do` : les valeurs arrivent par l'environnement et ne sont jamais relues
> comme du code.

```bash
#!/bin/bash
FICHIER="$1"     # le fichier à supprimer, donné en argument au script
[ -f "$FICHIER" ] || { echo "usage : $0 FICHIER" >&2; exit 1; }

CONFIRM='
<window title="Confirmer" resizable="false">
  <vbox>
    <text><label>Voulez-vous supprimer ce fichier ?</label></text>
    <hbox>
      <button><label>Oui</label><action>EXIT:oui</action></button>
      <button><label>Non</label><action>EXIT:non</action></button>
    </hbox>
  </vbox>
</window>'

eval "$(echo "$CONFIRM" | sermo --stdin)"
[ "$EXIT" = "oui" ] && rm "$FICHIER" && echo "Supprimé."
```

### 8.2 Formulaire complet avec validation

```bash
#!/bin/bash
export FORMULAIRE='
<window title="Nouveau profil" default-width="350">
  <vbox>
    <frame label="Informations">
      <vbox>
        <hbox>
          <text><label>Prénom :</label></text>
          <entry><variable>PRENOM</variable></entry>
        </hbox>
        <hbox>
          <text><label>Âge :</label></text>
          <spinbutton range-min="1" range-max="120">
            <variable>AGE</variable><default>25</default>
          </spinbutton>
        </hbox>
        <hbox>
          <text><label>Pays :</label></text>
          <comboboxtext>
            <variable>PAYS</variable>
            <item>France</item><item>Belgique</item><item>Suisse</item>
          </comboboxtext>
        </hbox>
      </vbox>
    </frame>
    <hbox><button ok></button><button cancel></button></hbox>
  </vbox>
</window>'

sermo --program=FORMULAIRE --do='
    [ "$EXIT" = "OK" ] && echo "Profil : $PRENOM, $AGE ans, $PAYS"'
```

### 8.3 Moniteur de processus en temps réel

```bash
#!/bin/bash
export MONITEUR='
<window title="Processus actifs" default-width="600" default-height="400">
  <vbox>
    <tree column-header-active="true">
      <variable>PROC</variable>
      <label>PID|Utilisateur|CPU%|Commande</label>
      <input>ps aux --no-headers | awk '"'"'{print $2"|"$1"|"$3"|"$11}'"'"' | head -20</input>
    </tree>
    <hbox>
      <button><label>Rafraîchir</label><action>REFRESH:PROC</action></button>
      <button><label>Fermer</label><action>EXIT:ok</action></button>
    </hbox>
  </vbox>
</window>'

sermo --program=MONITEUR
```

### 8.4 Lire un automate (Modbus, OPC UA)

Deux exemples complets, **en lecture seule**, montrent des valeurs d'atelier
en direct (jauge + valeur, rafraîchies par un `<timer>`) :

- `examples/modbus/modbus-lecture.sh` : registres d'un automate Modbus/TCP,
  lus par `mbpoll` (paquet `mbpoll`). Réglages : `MODBUS_HOTE`, `MODBUS_PORT`,
  `MODBUS_ESCLAVE` ; les registres se décrivent dans le tableau `REGISTRES`.
- `examples/opcua/opcua-lecture.sh` : variables d'un serveur OPC UA, lues par
  `opcua-lire`, un petit client C du même dossier (`make`, paquet
  `libopen62541-1.4-dev`). Réglage : `OPCUA_ADRESSE` ; les variables se
  décrivent dans le tableau `VARIABLES`. Pour un serveur qui exige des
  certificats : `OPCUA_MODE=sign` ou `chiffre`, `OPCUA_CERTIFICAT` et
  `OPCUA_CLE` (ceux du poste), `OPCUA_CONFIANCE` (le certificat du serveur ;
  aucun autre n'est accepté). `certificats-essai.sh` en fabrique pour l'essai.

Ces deux exemples passent par un script ; les balises `<input modbus>` et
`<input opcua>` (section 7.3) font la même lecture directement dans le XML.
Sans matériel, `faux-automate.py` (paquet `python3-pymodbus`) et
`faux-serveur-opcua` simulent l'équipement sur la machine même. Aucun des deux
exemples n'écrit vers l'équipement. La lecture part en fond : un équipement
injoignable affiche « pas de réponse » sans figer la fenêtre.

### 8.5 Exemples prêts à l'emploi

La 3.1.0 ajoute quinze exemples complets, un dossier chacun, avec son
`README.md`. Le paquet `sermo-doc` les installe sous
`/usr/share/doc/sermo-doc/examples/<dossier>/` ; dans les sources, ils sont sous
`examples/<dossier>/`.

| Dossier | Ce qu'il fait |
|---------|---------------|
| `approbation` | une fenêtre *Refuser* / *Autoriser* avant une action risquée d'un agent IA ou d'un programme ; seul un clic sur *Autoriser* vaut accord |
| `bluetooth-scan` | les appareils Bluetooth à portée et ceux que le poste connaît, en lecture seule |
| `bus-can` | écouter un bus CAN ; n'y envoyer une trame qu'après la fenêtre d'approbation |
| `camera-apercu` | regarder une webcam ou une caméra IP, sans rien enregistrer |
| `certificat-tls` | le certificat TLS d'un serveur : chaîne, nom, jours avant expiration |
| `dns-testeur` | résoudre un nom et comparer jusqu'à 6 résolveurs |
| `domotique-mqtt` | lire des capteurs (`<input mqtt>`) et commander d'un clic (`<action mqtt>`), avec un faux serveur d'essai |
| `gnss-moniteur` | l'état d'un récepteur GNSS/GPS (position, qualité, satellites), avec un mode démonstration |
| `heure-ntp` | l'horloge de la machine est-elle juste ? (NTP, écart, bilan) |
| `journal-direct` | suivre le journal systemd en direct, filtré par gravité, unité ou texte |
| `lecteur-media` | lire un son ou une vidéo (`<action media>`, `<input media>`) |
| `liaison-serie` | écouter un port série ; n'y écrire qu'après la fenêtre d'approbation |
| `reseau-testeur` | connexions TCP, test de port, ping, trame TCP ou UDP, requête HTTP |
| `usb-moniteur` | les appareils USB branchés, et les branchements en direct |
| `wifi-scan` | les réseaux Wi-Fi à portée et l'occupation des canaux, en lecture seule |

Le `README.md` de chaque dossier dit comment lancer l'exemple, avec quel binaire
sermo, et ce qu'il faut installer.

---

## 9. Intégration dans un script shell

### 9.1 Modèle de script complet

Les fonctions vont dans **un fichier à part**, `mon_app-fonctions.sh` :

```sh
# mon_app-fonctions.sh — chargé par sermo devant chaque commande
charger_config() { cat ~/.mon_app/config 2>/dev/null || echo "Aucune config"; }
sauvegarder_config() { mkdir -p ~/.mon_app; printf '%s\n' "$1" > ~/.mon_app/config; }
```

```bash
#!/bin/bash
# mon_app.sh — Application sermo
DOSSIER=$(dirname "$(readlink -f "$0")")
FONCTIONS="$DOSSIER/mon_app-fonctions.sh"
. "$FONCTIONS"          # le script s'en sert aussi, après le dialogue

export INTERFACE='
<window title="Mon Application">
  <vbox>
    <edit><variable>CONFIG</variable><input>charger_config</input></edit>
    <hbox>
      <button><label>Sauvegarder</label><action>EXIT:sauvegarder</action></button>
      <button cancel></button>
    </hbox>
  </vbox>
</window>'

eval "$(sermo --include="$FONCTIONS" --program=INTERFACE)"

case "$EXIT" in
    sauvegarder) sauvegarder_config "$CONFIG"; echo "Sauvegardé." ;;
    *) echo "Annulé." ;;
esac
```

⚠️ **N'écrivez pas `--include="$0"`.** sermo lit le fichier inclus **devant
chaque `<input>` et chaque action** (`. fichier; commande`). Inclure le script
lui-même le relancerait en entier à chaque fois, avec son appel à sermo. Le
fichier inclus est lu par `/bin/sh` (ou `SERMO_SHELL`) : écrivez-y des fonctions
en syntaxe `sh`.

### 9.2 Fenêtres multiples

Chaque fenêtre a **sa propre variable d'environnement**, exportée avant de
lancer sermo :

```bash
#!/bin/bash
export PRINCIPALE='
<window title="Principal">
  <vbox>
    <button><label>Ouvrir paramètres</label><action>LAUNCH:PARAMETRES</action></button>
    <button><label>Quitter</label><action>EXIT:quitte</action></button>
  </vbox>
  <variable>PRINCIPALE</variable>
</window>'

export PARAMETRES='
<window title="Paramètres">
  <vbox>
    <text><label>Fenêtre de paramètres</label></text>
    <button><label>Fermer</label><action>closewindow:PARAMETRES</action></button>
  </vbox>
  <variable>PARAMETRES</variable>
</window>'

sermo --program=PRINCIPALE
```

- `LAUNCH:PARAMETRES` lit le XML dans la variable d'environnement
  `PARAMETRES`. Si une fenêtre déjà ouverte porte un widget de ce nom, elle est
  seulement remise au premier plan. Un `LAUNCH:` ne marche ni depuis un
  `<timer>`, ni avant que la première fenêtre soit tout à fait chargée (un
  avertissement le dit).
- `closewindow:PARAMETRES` cherche le **widget** nommé `PARAMETRES` et ferme la
  fenêtre qui le contient. D'où le `<variable>PARAMETRES</variable>` de la
  fenêtre (après son contenu) : c'est lui que l'action retrouve. Fermer la
  dernière fenêtre termine sermo avec `EXIT="closewindow"` (§3.2).

---

## 10. FAQ et dépannage

**Q : La fenêtre ne s'affiche pas, `Cannot open display`**
R : Session SSH sans transmission graphique. Lancez `export DISPLAY=:0` ou `ssh -X`.

**Q : Quel backend est utilisé quand j'appelle `sermo` ?**
R : Celui choisi par `update-alternatives`. Pour forcer, appelez le binaire du
backend : `gtk3sermo`, `gtk4sermo`, `qt6sermo`, `fltk1sermo`, `efl1sermo`,
`sdl3sermo`, `lvglsermo`, `ncursessermo` ou `htmlsermo`.

**Q : Le widget `<terminal>` ne s'affiche pas**
R : Le terminal nécessite VTE et n'existe que sur `gtk3sermo` et `gtk4sermo`.
Vérifiez que le binaire a été construit avec : `gtk3sermo --version` doit afficher
« Built with additional support for: …, VTE ». Sur les autres ports, un dialogue qui
contient `<terminal>` s'ouvre quand même, sans terminal.

**Q : Comment passer des données volumineuses à `<edit>` ?**
R : Utilisez `<input file>/chemin/fichier</input>` plutôt qu'une commande shell.

**Q : Puis-je utiliser sermo en Python ?**
R : Oui. Construisez la chaîne XML et passez-la via
`subprocess.run(['sermo', '--stdin'], input=xml_str, text=True)`.

**Q : Comment déboguer une interface ?**
R : Lancez avec `sermo --debug --program=VAR` et redirigez stderr :
`sermo --program=VAR 2>debug.log`. Pour vérifier le parse sans affichage :
`sermo --program=VAR --print-ir`.

---
