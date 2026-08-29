# Manuel Utilisateur — gtk3sermo

**Version :** 1.0.0  
**Date :** Mai 2026  
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

`gtk3sermo` est un utilitaire en ligne de commande qui permet de créer des **fenêtres graphiques GTK 3** depuis n'importe quel script shell, Python, Perl ou autre langage interprété — sans écrire une seule ligne de code GTK.

Le principe est simple : vous décrivez votre interface en XML, et gtk3sermo l'affiche. Quand l'utilisateur ferme la fenêtre, les valeurs saisies sont renvoyées sur la sortie standard sous forme de variables shell.

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

gtk3sermo --program=DIALOG
# Sortie : NOM="Jean"  EXIT="ok"
```

### 1.1 Différences avec gtk3sermo 0.8.3

gtk3sermo est **100% compatible** avec les scripts écrits pour la version 0.8.3. Les seules différences visibles sont :

- L'apparence suit le thème GTK3 du système (plus moderne)
- Les couleurs sont spécifiées au format `rgba(r,g,b,a)` ou `#RRGGBB`
- Le terminal embarqué requiert VTE 2.91 (plus stable, meilleures performances)

---

## 2. Installation

### 2.1 Depuis les sources (recommandé)

```bash
# 1. Installer les dépendances
sudo apt-get install libgtk-3-dev libvte-2.91-dev \
  flex bison autoconf automake pkg-config gcc

# 2. Compiler
cd gtk3sermo/
./autogen.sh
./configure
make -j$(nproc)
sudo make install

# 3. Vérifier
gtk3sermo --version
```

### 2.2 Vérification de l'installation

```bash
echo '<window><vbox>
  <text><label>gtk3sermo fonctionne !</label></text>
  <button><label>Fermer</label><action>EXIT:ok</action></button>
</vbox></window>' | gtk3sermo --stdin
```

Une fenêtre GTK doit apparaître. Si elle s'ouvre, l'installation est réussie.

### 2.3 Désinstallation

```bash
cd gtk3sermo/
sudo make uninstall
```

---

## 3. Premiers pas

### 3.1 Modes d'utilisation

gtk3sermo accepte son XML de trois façons :

**Depuis stdin :**
```bash
echo '<window>...</window>' | gtk3sermo --stdin
```

**Depuis une variable d'environnement :**
```bash
export MON_DIALOG='<window>...</window>'
gtk3sermo --program=MON_DIALOG
```

**Depuis un fichier :**
```bash
gtk3sermo --file=mon_interface.xml
```

### 3.2 Récupérer les valeurs saisies

Quand l'utilisateur ferme la fenêtre, gtk3sermo affiche sur stdout les valeurs de tous les widgets nommés. Pour les utiliser dans votre script :

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

# --do : les valeurs arrivent comme variables d'environnement, sans
# jamais repasser par le shell. C'est la voie recommandée.
gtk3sermo --program=DIALOG --do='
    if [ "$EXIT" = "valide" ]; then
        echo "Bonjour $PRENOM $NOM_FAM !"
    fi'
```

---

## 4. Syntaxe de base

### 4.1 Structure d'un document gtk3sermo

```xml
<window title="Titre de la fenêtre" resizable="true" width="400" height="300">
  <vbox>
    <!-- Widgets ici -->
  </vbox>
</window>
```

Tout document gtk3sermo commence par `<window>`. Les widgets sont imbriqués dans des conteneurs (`<vbox>`, `<hbox>`, `<frame>`, `<notebook>`).

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


### 5.13 Nouveaux widgets GTK3 (1.0.0)

Les six widgets suivants ont été ajoutés dans la version 1.0.0.
Ils sont natifs GTK3 et n'existent pas dans gtk3sermo 0.8.x.

#### switch — Interrupteur on/off

```xml
<switch>
  <variable>MON_SWITCH</variable>
  <default>true</default>
  <action>echo "État : $MON_SWITCH"</action>
</switch>
```

Variable : `true` quand activé, `false` sinon.

#### filechooser — Sélecteur de fichier/dossier

```xml
<filechooser>
  <label>Choisir un fichier</label>
  <variable>FICHIER</variable>
  <default>/home/user</default>
  <action>echo "Sélectionné : $FICHIER"</action>
</filechooser>
```

Pour sélectionner un dossier : `<filechooser action="select-folder">`.
Variable : chemin absolu du fichier/dossier sélectionné.

#### calendar — Sélecteur de date

```xml
<calendar>
  <variable>DATE</variable>
  <default>2026-05-21</default>
  <action>echo "Date : $DATE"</action>
</calendar>
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

GTK3 ouvre automatiquement l'URI dans le navigateur par défaut.
Variable : l'URI courante du bouton.

#### searchentry — Champ de recherche

```xml
<searchentry>
  <label>Rechercher...</label>
  <variable>TERME</variable>
  <action>grep "$TERME" /var/log/syslog | head -20</action>
</searchentry>
```

Affiche une icône loupe et un bouton effacer. Signal `search-changed`
se déclenche après chaque frappe (avec anti-rebond automatique).
Variable : texte saisi.

#### infobar — Barre de notification

```xml
<infobar>
  <label>Opération réussie.</label>
  <default>info</default>
  <variable>STATUS</variable>
</infobar>
```

Types disponibles pour `<default>` : `info` (bleu), `warning` (jaune),
`error` (rouge), `question`, `other`. Le texte peut être mis à jour
dynamiquement via `REFRESH:STATUS`.
Variable : texte courant de la barre.


---

## 6. Actions et signaux

### 6.1 Actions disponibles

| Action | Syntaxe | Description |
|--------|---------|-------------|
| `EXIT` | `EXIT:valeur` | Ferme la fenêtre, exporte EXIT=valeur |
| `CLOSEWINDOW` | `CLOSEWINDOW:NOM_WIDGET` | Ferme la fenêtre qui contient le widget nommé. Le nom du préfixe est bien `closewindow`, pas `close` : un préfixe inconnu n'est pas signalé, il part au shell comme une commande ordinaire, et la fenêtre ne se ferme pas. |
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

À la fermeture, gtk3sermo émet sur stdout : `MA_VALEUR="contenu saisi"`

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
gtk3sermo --include=/chemin/fonctions.sh --program=DIALOG
```

Permet d'utiliser des fonctions shell définies dans `fonctions.sh` dans les attributs `<input>` et `<action>`.

---

## 8. Exemples pratiques

### 8.1 Boîte de confirmation

> ⚠️ **`eval` et les valeurs saisies.** Les exemples qui suivent passent la
> sortie du programme à `eval`. C'est l'usage historique de gtkdialog et il
> marche — mais la valeur d'un champ est **tapée par la personne qui se sert du
> dialogue**, qui n'est pas forcément celle qui a écrit le script.
>
> Depuis le 2026-08-25, le programme échappe les quatre caractères que le shell
> développe entre guillemets doubles (`\`, `"`, `$` et l'accent grave) : `eval`
> ne les exécute plus, et `tests/garde_echappement_sortie.sh` le vérifie à chaque
> poussée. Avant cette date il n'en échappait que deux, et saisir `$(commande)`
> dans un champ suffisait à la faire exécuter.
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

eval $(echo "$CONFIRM" | gtk3sermo --stdin)
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

eval $(echo "$SELECTOR" | gtk3sermo --stdin)
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

eval $(gtk3sermo --program=FORMULAIRE)

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

gtk3sermo --program=MONITEUR
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

eval $(echo "$ONGLETS" | gtk3sermo --stdin)
```

---

## 9. Intégration dans un script shell

### 9.1 Modèle de script complet

```bash
#!/bin/bash
# mon_app.sh — Application gtk3sermo

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
eval $(gtk3sermo --include="$0" --program=INTERFACE)

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
    <button><label>Fermer</label><action>closewindow:PARAMETRES</action></button>
  </vbox>
</window>'

gtk3sermo --program=FENETRE_PRINCIPALE
```

---

## 10. FAQ et dépannage

**Q : La fenêtre ne s'affiche pas, j'obtiens une erreur `Cannot open display`**  
R : Vous êtes probablement en session SSH sans transmission X11 ou Wayland. Lancez `export DISPLAY=:0` ou utilisez `ssh -X`.

**Q : Mon script fonctionnait avec gtk3sermo 0.8.3 mais plus maintenant**  
R : Vérifiez que vos attributs de couleur utilisent `#RRGGBB` et non les noms GTK2 (`GdkColor`). Le comportement fonctionnel est identique.

**Q : Le widget `<terminal>` ne s'affiche pas**  
R : Le terminal nécessite `libvte-2.91`. Vérifiez : `pkg-config --modversion vte-2.91`. Si absent, installez `libvte-2.91-0`.

**Q : Comment passer des données volumineuses à un widget `<edit>` ?**  
R : Utilisez `<input file>/chemin/fichier</input>` plutôt qu'une commande shell — c'est plus efficace pour les fichiers > 1 Mo.

**Q : Ma commande `<input>` contient des guillemets et ne fonctionne pas**  
R : Échappez les guillemets simples : `'"'"'` dans bash, ou utilisez un fichier de fonctions avec `--include`.

**Q : Comment afficher une barre de progression animée pendant une tâche longue ?**  
R : Utilisez un `<timer>` qui actualise la `<progressbar>` via `REFRESH`, et lancez votre tâche en arrière-plan en écrivant la progression dans un fichier temporaire.

**Q : Puis-je utiliser gtk3sermo en Python ?**  
R : Oui. Construisez la chaîne XML en Python et passez-la via `subprocess.run(['gtk3sermo', '--stdin'], input=xml_str)`.

**Q : Comment déboguer une interface qui ne se comporte pas comme prévu ?**  
R : Lancez avec `gtk3sermo --debug --program=VAR` pour activer les messages de diagnostic. Redirigez stderr : `gtk3sermo --program=VAR 2>debug.log`.

---

