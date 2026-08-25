# Manuel Utilisateur — gtk4sermo (GTK4)

**Version :** 1.0.0-gtk4  
**Date :** Juin 2026 (mis à jour 2026-06-01)  
**Licence :** GPL-2.0-or-later | **Projet :** haplo-dialog (devel@haplo-dialog.fr)  

> **Note (2026-06-01) :** `gtk4sermo` compile désormais sans erreur et s'installe
> via `make install` (`/usr/bin/gtk4sermo`). Voir la section Installation ci-dessous.

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

`gtk4sermo` est un utilitaire en ligne de commande qui permet de créer des **fenêtres graphiques GTK 4** depuis n'importe quel script shell, Python, Perl ou autre langage interprété — sans écrire une seule ligne de code GTK.

Le principe est simple : vous décrivez votre interface en XML, et gtk3d l'affiche. Quand l'utilisateur ferme la fenêtre, les valeurs saisies sont renvoyées sur la sortie standard sous forme de variables shell.

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

gtk4sermo --program=DIALOG
# Sortie : NOM="Jean"  EXIT="ok"
```

### 1.1 Différences avec gtk3d 0.8.3

gtk3d-modern est **100% compatible** avec les scripts écrits pour la version 0.8.3. Les seules différences visibles sont :

- L'apparence suit le thème GTK3 du système (plus moderne)
- Les couleurs sont spécifiées au format `rgba(r,g,b,a)` ou `#RRGGBB`
- Le terminal embarqué requiert VTE 2.91 (plus stable, meilleures performances)

---

## 2. Installation

### 2.1 Depuis les sources (recommandé)

```bash
# 1. Installer les dépendances GTK4
sudo apt-get install libgtk-4-dev libvte-2.91-gtk4-dev \
  flex bison autoconf automake pkg-config gcc

# 2. Compiler
cd gtk3d-gtk4/
./autogen.sh
./configure
make -j$(nproc)
sudo make install   # installe /usr/local/bin/gtk4sermo

# 3. Vérifier (coexiste avec gtk3d GTK3)
gtk4sermo --version
gtk4sermo --version   # GTK3 toujours disponible
```

### 2.2 Vérification de l'installation

```bash
echo '<window><vbox>
  <text><label>gtk4sermo (GTK4) fonctionne !</label></text>
  <button><label>Fermer</label><action>EXIT:ok</action></button>
</vbox></window>' | gtk4sermo --stdin
```

Une fenêtre GTK doit apparaître. Si elle s'ouvre, l'installation est réussie.

### 2.3 Désinstallation

```bash
cd gtk3d-modern/
sudo make uninstall
```

---

## 3. Premiers pas

### 3.1 Modes d'utilisation

gtk3d accepte son XML de trois façons :

**Depuis stdin :**
```bash
echo '<window>...</window>' | gtk4sermo --stdin
```

**Depuis une variable d'environnement :**
```bash
export MON_DIALOG='<window>...</window>'
gtk4sermo --program=MON_DIALOG
```

**Depuis un fichier :**
```bash
gtk4sermo --file=mon_interface.xml
```

### 3.2 Récupérer les valeurs saisies

Quand l'utilisateur ferme la fenêtre, gtk3d affiche sur stdout les valeurs de tous les widgets nommés. Pour les utiliser dans votre script :

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

# Évaluer la sortie pour créer les variables shell
eval $(gtk4sermo --program=DIALOG)

if [ "$EXIT" = "valide" ]; then
    echo "Bonjour $PRENOM $NOM_FAM !"
fi
```

---

## 4. Syntaxe de base

### 4.1 Structure d'un document gtk3d

```xml
<window title="Titre de la fenêtre" resizable="true" width="400" height="300">
  <vbox>
    <!-- Widgets ici -->
  </vbox>
</window>
```

Tout document gtk3d commence par `<window>`. Les widgets sont imbriqués dans des conteneurs (`<vbox>`, `<hbox>`, `<frame>`, `<notebook>`).

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
| `<variable>NOM</variable>` | Nom de la variable exportée à la fermeture |
| `<label>texte</label>` | Étiquette affichée dans le widget |
| `<default>valeur</default>` | Valeur initiale du widget |
| `<input>commande</input>` | Commande shell dont la sortie alimente le widget |
| `<action>ACTION:arg</action>` | Action déclenchée par une interaction |
| `<sensitive>false</sensitive>` | Désactiver le widget au démarrage |

---

## 5. Référence des widgets

### 5.1 Conteneurs

#### `<window>` — Fenêtre principale

```xml
<window title="Mon Application" resizable="true" width="500" height="400">
  <vbox>
    <!-- contenu -->
  </vbox>
</window>
```

Attributs spécifiques : `title`, `resizable`, `width`, `height`, `decorated`, `icon-name`

#### `<vbox>` et `<hbox>` — Boîtes de disposition

```xml
<vbox space-expand="true" space-fill="true">
  <hbox homogeneous="false" spacing="5">
    <!-- widgets côte à côte -->
  </hbox>
</vbox>
```

`<vbox>` empile les widgets verticalement, `<hbox>` horizontalement.

Attributs : `homogeneous` (même taille), `spacing` (espace en pixels), `space-expand`, `space-fill`

#### `<frame>` — Cadre avec titre

```xml
<frame label="Options" label-xalign="0.0">
  <vbox>
    <!-- contenu du cadre -->
  </vbox>
</frame>
```

#### `<notebook>` — Onglets

```xml
<notebook tab-labels="Onglet 1|Onglet 2">
  <vbox><!-- contenu onglet 1 --></vbox>
  <vbox><!-- contenu onglet 2 --></vbox>
</notebook>
```

#### Zone défilante — l'attribut `scrollable`

Il n'existe **pas** de balise `<scrolledwindow>`. Le défilement est un
**attribut** posé sur le widget lui-même :

```xml
<tree scrollable="true">...</tree>
```

L'attribut est reconnu par l'automate (`src/automaton.c`, `get_tag_attribute`)
et enveloppe le widget dans une zone défilante à sa création.

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
<edit>
  <variable>CONTENU</variable>
  <default>Ligne 1
Ligne 2</default>
  <width-request>400</width-request>
  <height-request>200</height-request>
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
  <radiobutton>
    <label>Option A</label>
    <variable>CHOIX_A</variable>
  </radiobutton>
  <radiobutton>
    <label>Option B</label>
    <variable>CHOIX_B</variable>
  </radiobutton>
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
  <item>Élément 3</item>
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
  <okbutton/>
  <cancelbutton/>
</hbox>
```

Ces boutons ont les labels et raccourcis clavier standard et génèrent `EXIT=OK` ou `EXIT=Cancel`.

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
  <label><b>Texte en gras</b> et <i>italique</i></label>
</text>
```

#### `<pixmap>` — Image

```xml
<pixmap>
  <input file>/chemin/vers/image.png</input>
  <width-request>64</width-request>
  <height-request>64</height-request>
</pixmap>
```

#### `<progressbar>` — Barre de progression

```xml
<progressbar>
  <variable>PROGRESSION</variable>
  <input>echo 0.75</input>
</progressbar>
```

La valeur attendue est entre `0.0` et `1.0`.

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

La variable exportée est au format `#RRGGBB|alpha`.

#### `<fontbutton>` — Sélecteur de police

```xml
<fontbutton>
  <variable>POLICE</variable>
  <default>Sans 12</default>
</fontbutton>
```

#### `<terminal>` — Terminal embarqué (nécessite VTE)

```xml
<terminal>
  <variable>TERMINAL</variable>
  <width-request>600</width-request>
  <height-request>300</height-request>
  <input>echo "ls -la" | bash</input>
</terminal>
```

#### `<timer>` — Minuterie

```xml
<timer milliseconds="1000" visible="false">
  <variable>HORLOGE</variable>
  <action>REFRESH:AFFICHAGE</action>
</timer>
```

Déclenche une action toutes les N millisecondes.

---

### 5.7 Menus

```xml
<menubar>
  <menu>
    <label>Fichier</label>
    <menuitem>
      <label>Ouvrir</label>
      <action>FILESELECT:FICHIER</action>
    </menuitem>
    <menuitemseparator/>
    <menuitem>
      <label>Quitter</label>
      <action>EXIT:quitte</action>
    </menuitem>
  </menu>
</menubar>
```

---

## 6. Actions et signaux

### 6.1 Actions disponibles

| Action | Syntaxe | Description |
|--------|---------|-------------|
| `EXIT` | `EXIT:valeur` | Ferme la fenêtre, exporte EXIT=valeur |
| `CLOSE` | `CLOSE:NOM_FENETRE` | Ferme une fenêtre par nom |
| `LAUNCH` | `LAUNCH:NOM_FENETRE` | Ouvre une nouvelle fenêtre |
| `REFRESH` | `REFRESH:NOM_WIDGET` | Relance l'`<input>` d'un widget |
| `SAVE` | `SAVE:NOM_WIDGET` | Sauvegarde l'état d'un widget |
| `CLEAR` | `CLEAR:NOM_WIDGET` | Vide le contenu d'un widget |
| `APPEND` | `APPEND:NOM_WIDGET` | Ajoute du contenu à un widget |
| `FILESELECT` | `FILESELECT:NOM_VAR` | Ouvre un sélecteur de fichier |
| `ENABLE` | `ENABLE:NOM_WIDGET` | Active un widget désactivé |
| `DISABLE` | `DISABLE:NOM_WIDGET` | Désactive un widget |
| `SHOW` | `SHOW:NOM_WIDGET` | Rend un widget visible |
| `HIDE` | `HIDE:NOM_WIDGET` | Cache un widget |
| `GRABFOCUS` | `GRABFOCUS:NOM_WIDGET` | Donne le focus clavier |
| `PRESENTWINDOW` | `PRESENTWINDOW:NOM` | Met la fenêtre au premier plan |

### 6.2 Signaux disponibles

Par défaut, `<action>` réagit au signal principal du widget (clic pour un bouton, etc.). Pour d'autres signaux :

```xml
<entry>
  <action signal="activate">EXIT:ok</action>         <!-- Touche Entrée -->
  <action signal="changed">REFRESH:APERCU</action>   <!-- À chaque frappe -->
</entry>
```

Signaux courants : `activate`, `changed`, `clicked`, `toggled`, `value-changed`, `cursor-changed`, `select-row`

### 6.3 Exécuter une commande shell

```xml
<button>
  <label>Ouvrir le navigateur</label>
  <action>xdg-open https://exemple.com</action>
</button>
```

Toute action non reconnue comme mot-clé ACTION est exécutée comme commande shell via `safe_system()`.

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

À la fermeture, gtk3d émet sur stdout : `MA_VALEUR="contenu saisi"`

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
<edit>
  <input file>/etc/hostname</input>
</edit>
```

### 7.4 Inclusion d'un fichier de fonctions

```bash
gtk4sermo --include=/chemin/fonctions.sh --program=DIALOG
```

Permet d'utiliser des fonctions shell définies dans `fonctions.sh` dans les attributs `<input>` et `<action>`.

---

## 8. Exemples pratiques

### 8.1 Boîte de confirmation

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

eval $(echo "$CONFIRM" | gtk4sermo --stdin)
[ "$EXIT" = "oui" ] && rm "$FICHIER" && echo "Supprimé."
```

### 8.2 Sélecteur de fichier

```bash
#!/bin/bash
SELECTOR='
<window title="Choisir un fichier">
  <vbox>
    <entry>
      <variable>FICHIER</variable>
      <default>/home</default>
    </entry>
    <button>
      <label>Parcourir...</label>
      <action>FILESELECT:FICHIER</action>
    </button>
    <hbox><okbutton/><cancelbutton/></hbox>
  </vbox>
</window>'

eval $(echo "$SELECTOR" | gtk4sermo --stdin)
[ "$EXIT" = "OK" ] && echo "Fichier choisi : $FICHIER"
```

### 8.3 Formulaire complet avec validation

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
    <hbox><okbutton/><cancelbutton/></hbox>
  </vbox>
</window>'

eval $(gtk4sermo --program=FORMULAIRE)

if [ "$EXIT" = "OK" ]; then
    echo "Profil créé : $PRENOM, $AGE ans, $PAYS"
fi
```

### 8.4 Moniteur de processus en temps réel

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
      <button>
        <label>Rafraîchir</label>
        <action>REFRESH:PROC</action>
      </button>
      <button><label>Fermer</label><action>EXIT:ok</action></button>
    </hbox>
  </vbox>
</window>'

gtk4sermo --program=MONITEUR
```

### 8.5 Interface avec onglets

```bash
#!/bin/bash
ONGLETS='
<window title="Configuration" width="400">
  <vbox>
    <notebook tab-labels="Général|Réseau|Avancé">
      <vbox>
        <text><label>Paramètres généraux</label></text>
        <entry><variable>NOM_HOST</variable><input>hostname</input></entry>
      </vbox>
      <vbox>
        <text><label>Configuration réseau</label></text>
        <entry><variable>IP_ADDR</variable><input>hostname -I | awk '"'"'{print $1}'"'"'</input></entry>
      </vbox>
      <vbox>
        <text><label>Options avancées</label></text>
        <checkbox><label>Mode débogage</label><variable>DEBUG</variable></checkbox>
      </vbox>
    </notebook>
    <hbox><okbutton/><cancelbutton/></hbox>
  </vbox>
</window>'

eval $(echo "$ONGLETS" | gtk4sermo --stdin)
```

---

## 9. Intégration dans un script shell

### 9.1 Modèle de script complet

```bash
#!/bin/bash
# mon_app.sh — Application gtk3d-modern

# Fonctions métier
charger_config() {
    cat ~/.mon_app/config 2>/dev/null || echo "Aucune config"
}

sauvegarder_config() {
    mkdir -p ~/.mon_app
    echo "$1" > ~/.mon_app/config
}

# Interface
export INTERFACE='
<window title="Mon Application">
  <vbox>
    <edit>
      <variable>CONFIG</variable>
      <input>charger_config</input>
    </edit>
    <hbox>
      <button>
        <label>Sauvegarder</label>
        <action>EXIT:sauvegarder</action>
      </button>
      <cancelbutton/>
    </hbox>
  </vbox>
</window>'

# Lancement avec accès aux fonctions
eval $(gtk4sermo --include="$0" --program=INTERFACE)

# Traitement
case "$EXIT" in
    sauvegarder) sauvegarder_config "$CONFIG" ; echo "Sauvegardé." ;;
    *) echo "Annulé." ;;
esac
```

### 9.2 Fenêtres multiples

```bash
#!/bin/bash
export FENETRE_PRINCIPALE='
<window title="Principal" name="PRINCIPALE">
  <vbox>
    <button>
      <label>Ouvrir paramètres</label>
      <action>LAUNCH:PARAMETRES</action>
    </button>
    <button><label>Quitter</label><action>EXIT:quitte</action></button>
  </vbox>
</window>

<window title="Paramètres" name="PARAMETRES" visible="false">
  <vbox>
    <text><label>Fenêtre de paramètres</label></text>
    <button><label>Fermer</label><action>CLOSE:PARAMETRES</action></button>
  </vbox>
</window>'

gtk4sermo --program=FENETRE_PRINCIPALE
```

---

## 10. FAQ et dépannage

**Q : La fenêtre ne s'affiche pas, j'obtiens une erreur `Cannot open display`**  
R : Vous êtes probablement en session SSH sans transmission X11 ou Wayland. Lancez `export DISPLAY=:0` ou utilisez `ssh -X`.

**Q : Mon script fonctionnait avec gtk3d 0.8.3 mais plus maintenant**  
R : Vérifiez que vos attributs de couleur utilisent `#RRGGBB` et non les noms GTK2 (`GdkColor`). Le comportement fonctionnel est identique.

**Q : Le widget `<terminal>` ne s'affiche pas**  
R : Le terminal nécessite `libvte-2.91`. Vérifiez : `pkg-config --modversion vte-2.91`. Si absent, installez `libvte-2.91-0`.

**Q : Comment passer des données volumineuses à un widget `<edit>` ?**  
R : Utilisez `<input file>/chemin/fichier</input>` plutôt qu'une commande shell — c'est plus efficace pour les fichiers > 1 Mo.

**Q : Ma commande `<input>` contient des guillemets et ne fonctionne pas**  
R : Échappez les guillemets simples : `'"'"'` dans bash, ou utilisez un fichier de fonctions avec `--include`.

**Q : Comment afficher une barre de progression animée pendant une tâche longue ?**  
R : Utilisez un `<timer>` qui actualise la `<progressbar>` via `REFRESH`, et lancez votre tâche en arrière-plan en écrivant la progression dans un fichier temporaire.

**Q : Puis-je utiliser gtk3d-modern en Python ?**  
R : Oui. Construisez la chaîne XML en Python et passez-la via `subprocess.run(['gtk3d', '--stdin'], input=xml_str)`.

**Q : Comment déboguer une interface qui ne se comporte pas comme prévu ?**  
R : Lancez avec `gtk4sermo --debug --program=VAR` pour activer les messages de diagnostic. Redirigez stderr : `gtk4sermo --program=VAR 2>debug.log`.


### 5.6 Widgets GTK3 portés en GTK4

Ces widgets étaient présents dans gtk3d GTK3 et sont maintenant disponibles dans gtk4sermo :

| Widget XML | Description | Variable exportée |
|---|---|---|
| `<switch>` | Interrupteur on/off (`GtkSwitch`) | `true` / `false` |
| `<calendar>` | Calendrier mensuel (`GtkCalendar`) | `YYYY-MM-DD` |
| `<filechooser>` | Sélecteur de fichier intégré | Chemin complet |
| `<image>` | Image statique PNG/SVG/icône | Nom de fichier |
| `<infobar>` | Barre d'info contextuelle (shim GTK4) | `""` |
| `<linkbutton>` | Bouton ouvrant une URI | URI |
| `<password>` | Champ de saisie masquée `GtkPasswordEntry` | Texte saisi |
| `<pulse>` | Barre de progression indéterminée | `""` |
| `<searchentry>` | Champ de recherche avec icône | Texte saisi |
| `<spinner>` | Indicateur d'activité animé | `""` |
| `<aspectframe>` | Cadre à ratio fixe (ex: 16:9) | `""` |

**Exemple — switch :**
```xml
<switch>
  <variable>SW_WIFI</variable>
  <default>true</default>
  <action>echo "WiFi actif : $SW_WIFI"</action>
</switch>
```

**Exemple — calendar :**
```xml
<calendar>
  <variable>CAL_DATE</variable>
  <action>echo "Date : $CAL_DATE"</action>
</calendar>
```

### 5.7 Widgets GTK4-natifs (nouveaux)

Ces widgets n'existent pas dans gtk3d GTK3. Ils exploitent les fonctionnalités natives GTK4 :

| Widget XML | Description | Variable exportée |
|---|---|---|
| `<vscale>` | Réglage vertical (`GtkScale` vertical) | Valeur float |
| `<vseparator>` | Séparateur vertical | `""` |
| `<levelbar>` | Jauge de niveau avec zones LOW/HIGH/FULL | Valeur actuelle |
| `<revealer>` | Affichage animé (slide, crossfade) | `true`/`false` |
| `<stack>` | Conteneur multi-pages avec transitions | Nom de page |
| `<flowbox>` | Grille adaptive réorganisable | Index sélectionné |
| `<overlay>` | Superposition de widgets (HUD, badge) | `""` |
| `<drawingarea>` | Surface de dessin Cairo | `width:height` |

**Exemple — levelbar (batterie) :**
```xml
<levelbar min-value="0" max-value="100" value="75">
  <variable>LB_BATTERY</variable>
</levelbar>
```

**Exemple — revealer (panneau animé) :**
```xml
<revealer transition="slide-down" duration="300">
  <variable>RV_PANEL</variable>
  <vbox>
    <text><label>Panneau caché</label></text>
  </vbox>
</revealer>
<button>
  <label>Afficher</label>
  <action>show:RV_PANEL</action>
</button>
```

**Exemple — stack (navigation multi-pages) :**
```xml
<stack transition="slide-left-right">
  <variable>STK_MAIN</variable>
  <vbox name="page0"><text><label>Page 1</label></text></vbox>
  <vbox name="page1"><text><label>Page 2</label></text></vbox>
</stack>
```

**Exemple — drawingarea (fond coloré Cairo) :**
```xml
<drawingarea width="400" height="200" background-color="#2d4a7a">
  <variable>DA_CANVAS</variable>
</drawingarea>
```

---

*Document horodaté — dernière mise à jour : 2026-06-07 (audit #4 — compilation complète gtk4sermo).*
