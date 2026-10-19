<!-- SPDX-License-Identifier: CC0-1.0 -->
# Exemples de sermo

[English](README.en.md)

Les 76 dossiers d'`examples/`, une ligne chacun, rangés par usage. Un lien mène au README du dossier quand
il en a un.

## Où ils sont installés

Le paquet `sermo-doc` installe une partie de ces dossiers sous `/usr/share/doc/sermo-doc/examples/` :

- `metiers/` et `boites-a-outils/`, en entier ;
- de `modbus/`, `opcua/` et `mavlink/`, les scripts et les sources seulement (pas un programme compilé sur
  le poste) ;
- les quinze exemples de la 3.1.0 (`approbation/`, `bluetooth-scan/`, `bus-can/`, `camera-apercu/`,
  `certificat-tls/`, `dns-testeur/`, `domotique-mqtt/`, `gnss-moniteur/`, `heure-ntp/`, `journal-direct/`,
  `lecteur-media/`, `liaison-serie/`, `reseau-testeur/`, `usb-moniteur/`, `wifi-scan/`), tous leurs fichiers.

Les autres dossiers sont dans les sources seulement.

## Langue

Les README des exemples sont en français, sauf ceux de `metiers/` et `boites-a-outils/`, qui existent aussi
en anglais. Ceux de `pfeme/`, `pfontview/` et `playmusic/` sont les README d'origine, en anglais.

## Licence

La plupart des fichiers écrits pour sermo disent leur licence en tête (`SPDX-License-Identifier`) : le plus
souvent CC0-1.0 (domaine public), parfois GPL-2.0 ou plus récente (`metiers/verifie.sh`,
`mavlink/faux-drone.py`, `glade/*-gtk4.ui`). Une partie d'`examples/` vient de gtkdialog : ces fichiers, sans
cette ligne, sont sous GPL-2.0 ou plus récente (voir `debian/copyright`) ; `c_embedded/example01.c` le dit
par sa notice GPL en tête. Les README de `metiers/`, `boites-a-outils/`, `showcase/` et `system-tools/` n'ont
pas de ligne SPDX : `debian/copyright` les range, comme le reste d'`examples/`, sous GPL-2.0 ou plus récente.

## Widgets : un dossier par balise

La plupart viennent de gtkdialog. Chaque script choisit son binaire par `GTKDIALOG` (défaut `gtk3sermo`) :
`GTKDIALOG=qt6sermo ./button`.

- `aspectframe/` — `<aspectframe>` : un conteneur qui garde le rapport largeur/hauteur de son enfant.
- `button/` — `<button>` : libellé, image, icône, position de l'image, signaux.
- `c_embedded/` — piloter un dialogue sermo depuis un programme C (`example01.c`).
- `calendar/` — `<calendar>` : choisir une date.
- `checkbox/` — `<checkbox>` : attributs, rafraîchissement conditionnel.
- `colorbutton/` — `<colorbutton>` : choisir une couleur.
- `combobox/` — `<combobox>` : liste, attributs, `<input>`.
- `comboboxentry/` — `<comboboxentry>` : liste avec saisie libre.
- `comboboxtext/` — `<comboboxtext>` : liste déroulante simple.
- `edit/` — `<edit>` : texte sur plusieurs lignes.
- `entry/` — `<entry>` : champ d'une ligne, attributs, `<input>`.
- `eventbox/` — `<eventbox>` : une zone qui capte le clic.
- `expander/` — `<expander>` : une section repliable.
- `filechooser/` — `<filechooser>` : choisir un fichier ou un dossier.
- `fontbutton/` — `<fontbutton>` : choisir une police.
- `frame/` — `<frame>` : un cadre avec titre.
- `glade/` — `--glade-xml` : une interface GtkBuilder (ports GTK 3 et GTK 4 seulement).
- `hbox/` — `<hbox>` : boîte horizontale, défilante.
- `hscale/` — `<hscale>` : curseur horizontal.
- `hseparator/` — `<hseparator>` : séparateur horizontal.
- `image/` — `<image>` : une image.
- `infobar/` — `<infobar>` : une barre de notification.
- `layer-shell/` — attributs de `<window>` pour ancrer une fenêtre sous Wayland (barre, dock).
- `linkbutton/` — `<linkbutton>` : un bouton lien.
- `list/` — `<list>` : liste avec sélection, actions, signaux.
- `menu/` — `<menu>` : un menu.
- `menubar/` — `<menubar>` : une barre de menus.
- `menuitem/` — `<menuitem>` : entrées de menu.
- `menuitemseparator/` — `<menuitemseparator>` : séparateur de menu.
- `miscellaneous/` — divers : actions conditionnelles, signaux, icônes, fichiers d'entrée, fenêtres.
- `notebook/` — `<notebook>` : des onglets.
- `password/` — `<password>` : champ masqué.
- `pixmap/` — `<pixmap>` : image, rafraîchie ou suivie sur disque.
- `progressbar/` — `<progressbar>` : barre de progression.
- `pulse/` — `<pulse>` : barre d'activité sans fin connue.
- `radiobutton/` — `<radiobutton>` : choix exclusif.
- `searchentry/` — `<searchentry>` : champ de recherche.
- `spinbutton/` — `<spinbutton>` : sélecteur numérique.
- `spinner/` — `<spinner>` : indicateur d'attente.
- `statusbar/` — `<statusbar>` : barre de statut.
- `switch/` — `<switch>` : interrupteur.
- `table/` — `<table>` : tableau.
- `terminal/` — `<terminal>` : terminal intégré (gtk3sermo et gtk4sermo, avec VTE).
- `text/` — `<text>` : étiquette de texte.
- `timer/` — `<timer>` : minuterie.
- `togglebutton/` — `<togglebutton>` : bouton à bascule.
- `tree/` — `<tree>` : tableau à colonnes, icônes, insertion, sélection.
- `vbox/` — `<vbox>` : boîte verticale, défilante.
- `vscale/` — `<vscale>` : curseur vertical.
- `vseparator/` — `<vseparator>` : séparateur vertical.
- `window/` — `<window>` : attributs et signaux de la fenêtre.

## Applications de Thunor

Trois applications complètes, écrites par Thunor pour gtkdialog (GPL-2.0 ou plus récente).

- `pfeme/` — une interface pour lancer des émulateurs ([README](pfeme/README), en anglais).
- `pfontview/` — voir une police TrueType ([README](pfontview/README), en anglais).
- `playmusic/` — un lecteur de musique ([README](playmusic/README), en anglais).

## Vitrines

- `showcase/` — cinq repères visuels, pour voir d'un coup d'œil ce qu'un port sait afficher ([README](showcase/README.md)).
- `system-tools/` — un outil d'administration Debian à six onglets, qui va au bout ([README](system-tools/README.md)).

## Métiers et boîtes à outils

- `metiers/` — soixante scripts prêts à lancer, cinq par métier, et le test de réussite des ports ([README](metiers/README.md)).
- `boites-a-outils/` — quatre boîtes (système, serveur, administrateur, utilisateur), un lanceur et dix outils chacune ([README](boites-a-outils/README.md)).

## Lire et écrire vers l'extérieur

- `modbus/` — lire un automate Modbus/TCP, en lecture seule, avec un faux automate ([README](modbus/README.md)).
- `opcua/` — lire un serveur OPC UA, en lecture seule, avec un faux serveur et des certificats d'essai ([README](opcua/README.md)).
- `domotique-mqtt/` — lire des capteurs MQTT et commander d'un clic, en balises natives ([README](domotique-mqtt/README.md)).
- `mavlink/` — télémétrie d'un drone, et ordres de sécurité sur liste blanche ([README](mavlink/README.md)).
- `lecteur-media/` — lire un son ou une vidéo : lire, pause, volume, état ([README](lecteur-media/README.md)).

## Réseau et matériel

- `approbation/` — une porte d'approbation humaine en une fenêtre ; sert aussi aux envois de `bus-can` et `liaison-serie` ([README](approbation/README.md)).
- `bluetooth-scan/` — les appareils Bluetooth à portée, en lecture seule ([README](bluetooth-scan/README.md)).
- `bus-can/` — écouter un bus CAN, et n'y envoyer une trame qu'avec confirmation ([README](bus-can/README.md)).
- `camera-apercu/` — voir une caméra sur un clic, sans rien enregistrer ([README](camera-apercu/README.md)).
- `certificat-tls/` — le certificat TLS d'un serveur : chaîne, nom, jours restants ([README](certificat-tls/README.md)).
- `dns-testeur/` — résoudre un nom et comparer des résolveurs ([README](dns-testeur/README.md)).
- `gnss-moniteur/` — l'état d'un récepteur GNSS/GPS, avec un mode démo ([README](gnss-moniteur/README.md)).
- `heure-ntp/` — l'horloge de la machine est-elle juste ? ([README](heure-ntp/README.md)).
- `journal-direct/` — suivre le journal systemd en direct, filtré ([README](journal-direct/README.md)).
- `liaison-serie/` — écouter un port série, et n'y écrire qu'avec confirmation ([README](liaison-serie/README.md)).
- `reseau-testeur/` — connexions TCP, test de port, ping, trame, UDP et HTTP ([README](reseau-testeur/README.md)).
- `usb-moniteur/` — les appareils USB branchés, et les branchements en direct ([README](usb-moniteur/README.md)).
- `wifi-scan/` — les réseaux Wi-Fi à portée et l'occupation des canaux ([README](wifi-scan/README.md)).
