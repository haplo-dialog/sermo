# Manuel Utilisateur — sermo

[English](MANUEL_UTILISATEUR.en.md)

**Version :** 2.7.4
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

**Ce manuel décrit le langage XML**, qui est **identique sur les sept backends**
de sermo (gtk3, gtk4, qt6, fltk1, efl1, sdl3, ncurses) : le même script rend une
interface équivalente partout. `sermo` désigne dans ce document le backend
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
virtuel `gtkdialog` pointe le backend GTK 3, de sorte que les scripts qui
appellent `gtkdialog` continuent de fonctionner. Différences visibles :

- L'apparence suit le **thème du système** en cours (clair/sombre).
- Les couleurs se spécifient au format `rgba(r,g,b,a)` ou `#RRGGBB`.
- Le terminal embarqué requiert VTE et n'existe que sur gtk3sermo et gtk4sermo.

---

## 2. Installation

### 2.1 Depuis les paquets

sermo produit des paquets **séparés** — un cœur commun plus un backend par
toolkit :

```bash
apt install sermo-backend-gtk3     # GTK 3
apt install sermo-backend-fltk1    # FLTK, sans tirer GTK ni Qt
apt install sermo-backend-qt6      # Qt 6
```

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

Le rendu hors écran est disponible sur **les six backends graphiques** (`gtk3`,
`gtk4`, `qt6`, `fltk1`, `efl1`, `sdl3`) — `gtk3`/`gtk4`/`fltk1` via un display
virtuel (xvfb) où le backend écrit lui-même le PNG, `qt6`/`efl1`/`sdl3` via leur
plateforme offscreen. `ncurses` reste un affichage **terminal** (pas d'image ;
aperçu texte via `--print-ir`).

La taille demandée par `default-width`/`default-height` est respectée sur les
six. Jusqu'à la 2.7.1, `qt6` l'ignorait et sortait parfois une image de 2×2
pixels quand la fenêtre contenait une liste déroulante — `tests/garde_taille_fenetre.sh`
le vérifie désormais sur chaque port graphique.

---

## 4. Syntaxe de base

### 4.1 Structure d'un document

```xml
<window title="Titre de la fenêtre" resizable="true" width="400" height="300">
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
| `tooltip-text` | texte | Infobulle au survol |
| `width-request` | nombre | Largeur minimale en pixels |
| `height-request` | nombre | Hauteur minimale en pixels |

### 4.3 Balises de contenu communes

| Balise | Description |
|--------|-------------|
| `<variable>NOM</variable>` | Nom de la variable exportée à la sortie (voir §3.2) |
| `<label>texte</label>` | Étiquette affichée dans le widget |
| `<default>valeur</default>` | Valeur initiale du widget |
| `<input>commande</input>` | Commande shell dont la sortie alimente le widget |
| `<action>ACTION:arg</action>` | Action déclenchée par une interaction |
| `<sensitive>false</sensitive>` | Désactiver le widget au démarrage |
| `<width>N</width>` et `<height>N</height>` | Forme sous-élément de `width-request` / `height-request` |

**Toute balise s'écrit ouverte puis fermée.** La forme auto-fermante n'est pas
reconnue par l'analyseur : `<separator></separator>` et non `<separator/>`,
`<button ok></button>` et non `<okbutton/>`. Un conteneur vide est refusé lui
aussi : un `<vbox>` doit contenir au moins un widget.

---

## 5. Référence des widgets

### 5.1 Conteneurs

#### `<window>` — Fenêtre principale

```xml
<window title="Mon Application" resizable="true" width="500" height="400">
  <vbox>
    <text><label>Contenu de la fenêtre</label></text>
  </vbox>
</window>
```

Attributs spécifiques : `title`, `resizable`, `width`, `height`, `decorated`, `icon-name`.

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
**attribut** posé sur le widget lui-même :

```xml
<tree scrollable="true">
  <variable>CHOIX</variable>
  <label>Nom|Taille</label>
  <input>echo -e "a|1\nb|2"</input>
</tree>
```

L'attribut est reconnu par l'automate du cœur et enveloppe le widget dans une
zone défilante à sa création.

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

#### `<pixmap>` — Image

```xml
<pixmap width-request="64" height-request="64">
  <input file>/chemin/vers/image.png</input>
</pixmap>
```

#### Règle commune aux valeurs numériques

`<hscale>`, `<vscale>`, `<spinbutton>` et `<levelbar>` exportent leur valeur
selon l'attribut **`digits=`** : `digits="0"` (le défaut) donne un entier,
`digits="2"` donne deux décimales.

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

#### `<terminal>` — Terminal embarqué (gtk3sermo et gtk4sermo, nécessite VTE)

```xml
<terminal width-request="600" height-request="300">
  <variable>TERMINAL</variable>
  <input>echo "ls -la"</input>
</terminal>
```

Le terminal lance un shell (`/bin/sh`, ou les attributs `argv0`, `argv1`…). La
**sortie** de la commande `<input>` est tapée dans ce shell, comme au clavier :
ici, `ls -la` s'exécute dans le terminal. La variable rend le PID du shell.

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
Sur les cinq autres ports, il devient un `<filechooser>` : un bouton qui ouvre le
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
<drawingarea width="200" height="120"><variable>ZONE</variable></drawingarea>
```

Une surface libre, réservée au dessin. **Variable : chaîne vide** — une zone de
dessin n'a pas de valeur à rendre.

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
| `LAUNCH` | `LAUNCH:NOM_FENETRE` | Ouvre une nouvelle fenêtre |
| `REFRESH` | `REFRESH:NOM_WIDGET` | Relance l'`<input>` d'un widget |
| `SAVE` | `SAVE:NOM_WIDGET` | Sauvegarde l'état d'un widget |
| `CLEAR` | `CLEAR:NOM_WIDGET` | Vide le contenu d'un widget |
| `APPEND` | `APPEND:NOM_WIDGET` | Ajoute du contenu à un widget |
| `FILESELECT` | `FILESELECT:NOM_VAR` | Ouvre un sélecteur de fichier |
| `ENABLE` / `DISABLE` | `ENABLE:NOM_WIDGET` | (Dés)active un widget |
| `SHOW` / `HIDE` | `SHOW:NOM_WIDGET` | (In)visibilise un widget |
| `GRABFOCUS` | `GRABFOCUS:NOM_WIDGET` | Donne le focus clavier |
| `PRESENTWINDOW` | `PRESENTWINDOW:NOM` | Met la fenêtre au premier plan |

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

### 6.4 Actions conditionnelles

```xml
<button>
  <label>Action selon état</label>
  <action condition="command_is_true(test $CASE = 1)">REFRESH:WIDGET_A</action>
  <action condition="command_is_false(test $CASE = 1)">REFRESH:WIDGET_B</action>
</button>
```

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

### 7.4 Inclusion d'un fichier de fonctions

```bash
sermo --include=/chemin/fonctions.sh --program=DIALOG
```

Permet d'utiliser des fonctions shell définies dans `fonctions.sh` dans les
attributs `<input>` et `<action>`. Un chemin relatif part du dossier où le
programme a été lancé.

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
<window title="Nouveau profil" width="350">
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
<window title="Processus actifs" width="600" height="400">
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

---

## 9. Intégration dans un script shell

### 9.1 Modèle de script complet

```bash
#!/bin/bash
# mon_app.sh — Application sermo

charger_config() { cat ~/.mon_app/config 2>/dev/null || echo "Aucune config"; }
sauvegarder_config() { mkdir -p ~/.mon_app; echo "$1" > ~/.mon_app/config; }

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

eval "$(sermo --include="$0" --program=INTERFACE)"

case "$EXIT" in
    sauvegarder) sauvegarder_config "$CONFIG"; echo "Sauvegardé." ;;
    *) echo "Annulé." ;;
esac
```

### 9.2 Fenêtres multiples

```bash
#!/bin/bash
export FENETRE_PRINCIPALE='
<window title="Principal" name="PRINCIPALE">
  <vbox>
    <button><label>Ouvrir paramètres</label><action>LAUNCH:PARAMETRES</action></button>
    <button><label>Quitter</label><action>EXIT:quitte</action></button>
  </vbox>
</window>

<window title="Paramètres" name="PARAMETRES" visible="false">
  <vbox>
    <text><label>Fenêtre de paramètres</label></text>
    <button><label>Fermer</label><action>closewindow:PARAMETRES</action></button>
  </vbox>
</window>'

sermo --program=FENETRE_PRINCIPALE
```

---

## 10. FAQ et dépannage

**Q : La fenêtre ne s'affiche pas, `Cannot open display`**
R : Session SSH sans transmission graphique. Lancez `export DISPLAY=:0` ou `ssh -X`.

**Q : Quel backend est utilisé quand j'appelle `sermo` ?**
R : Celui choisi par `update-alternatives`. Pour forcer, appelez le binaire du
backend : `gtk3sermo`, `qt6sermo`, `fltk1sermo`, `efl1sermo`, `sdl3sermo`, `gtk4sermo`.

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
