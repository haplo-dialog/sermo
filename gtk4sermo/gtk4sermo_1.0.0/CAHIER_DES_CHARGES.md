<div align="right"><sub><code>maj :     2026-08-20 20:34:10 · par claude</code></sub></div>
<div align="right"><sub><code>réindex : —</code></sub></div>

# Cahier des Charges — gtk4sermo (gtk4sermo)

**Distributeur :** Haplo-Linux | **Distribution :** Haplo-Linux (Debian Testing)  
**Noyau :** Linux 7.0.4+deb14-amd64 | **DE :** Xfce4 4.20 / Xfwm4 (X11)  
**GTK :** GTK4 Haplo-Dark | **Dépôt :** https://haplo-dialog.fr  

**Projet :** gtk4sermo-1.0.0  
**Version :** 1.0.0  
**Date :** Juin 2026 (mis à jour 2026-06-01)  
**Statut :** Port GTK4 complet — compile sans erreur (gtk4sermo ~1,45 Mo) ; tests fonctionnels en cours  
**Auteur original :** László Pere, Thunor  
**Fork/Modernisation :** haplo-dialog <devel@haplo-dialog.fr>  

---

## Table des matières

1. [Contexte et historique](#1-contexte-et-historique)
2. [Périmètre du projet](#2-périmètre-du-projet)
3. [Objectifs](#3-objectifs)
4. [Exigences fonctionnelles](#4-exigences-fonctionnelles)
5. [Exigences non fonctionnelles](#5-exigences-non-fonctionnelles)
6. [Contraintes techniques](#6-contraintes-techniques)
7. [Architecture cible](#7-architecture-cible)
8. [Livrables](#8-livrables)
9. [Planification et jalons](#9-planification-et-jalons)
10. [Critères d'acceptation](#10-critères-dacceptation)
11. [Risques identifiés](#11-risques-identifiés)
12. [Glossaire](#12-glossaire)

---

## 1. Contexte et historique

### 1.1 Origine du projet

`gtk4sermo` est le port GTK4 de `gtk3d` (gtk3sermo), lui-même issu de la modernisation
de `gtkdialog` (László Pere, 2003–2007 ; Thunor, 2011–2012), abandonné à la version
0.8.3. `gtk3d` a porté ce code de GTK2 vers GTK3, corrigé ses vulnérabilités et
remplacé `libglade` par `GtkBuilder`. `gtk4sermo` poursuit cette modernisation en migrant
la base GTK3 vers **GTK4** (>= 4.6), tout en conservant la même syntaxe XML.

L'objectif est de proposer un binaire `gtk4sermo` natif GTK4, coexistant avec `gtk3d`,
prêt pour les environnements GNOME modernes où GTK3 est en voie de retrait.

### 1.2 Problème

GTK4 (2020+) est **volontairement incompatible** avec GTK3 : de nombreuses APIs
utilisées par `gtk3d` ont été supprimées ou remplacées. Un simple recompilage est
impossible :

- **`gtk_container_add()`**, `gtk_box_pack_start()` et l'essentiel de l'API de
  conteneurs GTK3 ont disparu au profit de `gtk_box_append()` / `gtk_window_set_child()`
- La **boucle d'événements** et le cycle de vie des `GtkApplication` ont changé
- Les **dialogues** (couleur, police, fichier) sont devenus asynchrones
- **VTE** doit être lié contre la variante GTK4 (`vte-2.91-gtk4`)
- Plusieurs widgets GTK3 (`GtkMenuBar`, `GtkTreeView` classique…) n'ont pas
  d'équivalent direct et demandent un portage ou un stub

### 1.3 Opportunité

`gtk4sermo` reste conceptuellement unique : aucun outil équivalent en C n'offre la même
simplicité pour créer des GUI GTK4 depuis un script. Porter `gtk3d` vers GTK4 plutôt
que d'attendre permet d'anticiper le retrait de GTK3 des environnements GNOME.

---

## 2. Périmètre du projet

### 2.1 Inclus dans ce projet

- Migration complète de **GTK 3** vers **GTK 4** (>= 4.6)
- Couche de compatibilité `gtk4-compat.h` (65+ shims GTK3→GTK4) force-incluse
- Portage ou stub des widgets sans équivalent GTK4 direct
- Ajout de widgets **GTK4-natifs** (`revealer`, `stack`, `flowbox`, `overlay`, `levelbar`…)
- Mise à jour du **système de build** (Autotools) et de la dépendance **VTE** vers `vte-2.91-gtk4`
- Adaptation des **dialogues asynchrones** GTK4 (couleur, police, fichier)
- Rédaction de la **documentation** (cahier des charges, manuel utilisateur, manuel développeur)
- Conservation de **100% de la compatibilité descendante** de la syntaxe XML `gtk3d`

### 2.2 Hors périmètre (version 0.9.x)

- Réécriture du parser (Yacc/Lex actuel conservé)
- Interface de configuration graphique
- Support Windows/macOS
- Portage natif des widgets restés en stub (`menubar`, `menu`, `menuitem`, `table`) — planifié v1.2

### 2.3 Coexistence avec gtk3d

- Binaire `gtk4sermo` installable **simultanément** avec `gtk3d` (GTK3) et les autres ports
- Même syntaxe XML : un script `gtk3d` fonctionne sans modification avec `gtk4sermo`
- Documentation et exemples propres à GTK4

---

## 3. Objectifs

### 3.1 Objectif principal

Produire un binaire `gtk4sermo` fonctionnel sur tout système Linux moderne doté de GTK 4 (>= 4.6), compilable sans erreur, sécurisé, et compatible avec tous les scripts écrits pour `gtk3d`.

### 3.2 Objectifs secondaires

| ID | Objectif | Priorité |
|----|----------|----------|
| O1 | Zéro vulnérabilité critique (injection shell, buffer overflow) | HAUTE |
| O2 | Compilation sans erreur sur Debian 12, Ubuntu 22.04, Fedora 38 | HAUTE |
| O3 | Compatibilité 100% des scripts `gtk3d` existants (même syntaxe XML) | HAUTE |
| O4 | Documentation complète en français | MOYENNE |
| O5 | Réduire les avertissements de compilation (-Wall -Wextra) | MOYENNE |
| O6 | Couche de compatibilité `gtk4-compat.h` complète | ✅ RÉALISÉ |
| O7 | Ajout des widgets GTK4-natifs (`revealer`, `stack`, `flowbox`, `overlay`…) | ✅ RÉALISÉ |

---

## 4. Exigences fonctionnelles

### 4.1 Widgets supportés

**50 widgets** implémentés au total dans gtk4sermo 1.0.0 (un fichier `widget_*.c`
par widget dans `src/`), dont 4 encore au stade de **stub** (présents mais sans rendu
fonctionnel GTK4 complet) :

**Widgets hérités de gtk3d (30) :** `button`, `togglebutton`, `checkbox`, `radiobutton`, `switch`, `entry`, `edit`, `text`, `password`, `searchentry`, `spinbutton`, `hscale`, `vscale`, `progressbar`, `combobox`, `comboboxtext`, `list`, `tree`, `pixmap`, `image`, `colorbutton`, `fontbutton`, `notebook`, `frame`, `expander`, `hbox`, `vbox`, `statusbar`, `timer`, `window`

**Widgets GTK3 portés en GTK4 (12) :** `calendar`, `filechooser`, `infobar`, `linkbutton`, `pulse`, `spinner`, `aspectframe`, `eventbox`, `hseparator`, `vseparator`, `terminal`, `levelbar`

**Widgets GTK4-natifs (4) :** `revealer`, `stack`, `flowbox`, `overlay`

**Stubs (rendu GTK4 incomplet) :** `menubar`, `menuitem`, `table`, `drawingarea`

> Total : 30 + 12 + 4 + 4 = **50 widgets** (`ls src/widget_*.c` = 50 fichiers).

### 4.2 Modes d'entrée du programme

- `gtk4sermo --stdin` — lire la définition XML depuis l'entrée standard
- `gtk4sermo --program=VAR_ENV` — lire depuis une variable d'environnement
- `gtk4sermo --file=chemin.xml` — lire depuis un fichier
- `gtk4sermo --geometry=WxH+X+Y` — positionner la fenêtre

### 4.3 Actions supportées

Le moteur d'actions doit traiter : `EXIT`, `CLOSE`, `LAUNCH`, `REFRESH`, `SAVE`, `CLEAR`, `APPEND`, `FILESELECT`, `REMOVESELECTED`, `ENABLE`, `DISABLE`, `SHOW`, `HIDE`, `ACTIVATE`, `GRABFOCUS`, `PRESENTWINDOW`

### 4.4 Sortie du programme

À la fermeture, le programme doit exporter sur stdout les paires `NOM_VARIABLE=valeur` pour tous les widgets nommés, conformément au comportement de `gtk3d`.

### 4.5 Gestion des commandes externes

Tous les attributs `<input>`, `<action>` et commandes shell associés aux widgets doivent être exécutés de manière **sécurisée** via `safe_system()` / `safe_popen()` sans invoquer `/bin/sh -c` directement.

---

## 5. Exigences non fonctionnelles

### 5.1 Sécurité

| Exigence | Description | Niveau |
|----------|-------------|--------|
| SEC-1 | Aucun appel direct à `system()` ou `popen()` | Obligatoire |
| SEC-2 | Aucun `strcpy()`, `strcat()`, `sprintf()` sans borne | Obligatoire |
| SEC-3 | Compilation avec `-D_FORTIFY_SOURCE=3` | Obligatoire |
| SEC-4 | Compilation avec `-fstack-protector-strong` | Obligatoire |
| SEC-5 | Binaire Position Independent Executable (`-fPIE -pie`) | Obligatoire |
| SEC-6 | Liens avec `-Wl,-z,relro -Wl,-z,now` (RELRO complet) | Obligatoire |
| SEC-7 | Validation des entrées avant exécution de commande | Obligatoire |

### 5.2 Performances

- Démarrage d'une fenêtre simple en moins de **200 ms**
- Aucun gel de l'interface pendant l'exécution de commandes (cible versions futures)
- Empreinte mémoire inférieure à **50 Mo** pour une fenêtre simple

### 5.3 Portabilité

- Compatible : **Debian 12**, **Ubuntu 22.04 LTS**, **Ubuntu 24.04 LTS**, **Fedora 38+**, **Arch Linux**
- Architecture : **x86_64**, **ARM64** (Raspberry Pi OS)
- Compilateur : **GCC >= 10** ou **Clang >= 12**

### 5.4 Maintenabilité

- Code conforme au standard **C17** (`-std=c17`)
- Avertissements de compilation inférieurs à 20 avec `-Wall -Wextra`
- Chaque fichier source doit porter un en-tête de licence **GPL-2.0-or-later**

### 5.5 Compatibilité descendante

Tous les scripts shell utilisant `gtk3d` doivent fonctionner sans modification avec `gtk4sermo` 1.0.0 — la syntaxe XML est strictement identique.

---

## 6. Contraintes techniques

### 6.1 Dépendances autorisées

| Bibliothèque | Version minimale | Justification |
|---|---|---|
| GTK4 | 4.6.0 | Interface graphique principale (gtk4sermo) |
| GLib | 2.68.0 | Fournie avec GTK4, utilitaires |
| GIO | 2.68.0 | Fournie avec GTK4 |
| VTE | 0.70 (vte-2.91-gtk4) | Widget terminal GTK4 |
| Cairo | 1.16+ | Rendu GtkDrawingArea |
| Flex | 2.6+ | Génération du lexer |
| Bison | 3.0+ | Génération du parser |

### 6.2 Dépendances supprimées (par rapport à gtk3d / GTK3)

| Bibliothèque | Raison de suppression | Remplacement |
|---|---|---|
| GTK+ 3.x | Migration vers GTK4 | GTK4 >= 4.6 |
| `vte-2.91` (GTK3) | Variante GTK3 incompatible | `vte-2.91-gtk4` |
| GTK3 container API | Supprimée en GTK4 | Shims `gtk4-compat.h` + `gtk_box_append()` |

### 6.3 Licence

Le projet est distribué sous **GPL-2.0-or-later** (famille haplo-dialog), en
continuité directe avec `gtkdialog` 0.8.3 (László Pere), publié sous la même
licence **GPL-2.0-or-later**.

---

## 7. Architecture cible

```
gtk4sermo/gtk4sermo_1.0.0/
├── src/
│   ├── gtk4sermo.c/.h              # Point d'entrée, parsing des arguments
│   ├── safe_exec.c/.h          # Wrappers sécurisés system/popen
│   ├── automaton.c/.h          # Moteur d'exécution (AST → widgets GTK4)
│   ├── gtkdialog_parser.y      # Parser Bison (syntaxe XML héritée de gtk3d)
│   ├── gtkdialog_lexer.l       # Lexer Flex
│   ├── variables.c/.h          # Gestion des variables et de l'état
│   ├── signals.c/.h            # Système de signaux GTK4
│   ├── actions.c/.h            # Traitement des actions (EXIT, REFRESH…)
│   ├── widgets.c/.h            # Dispatch et utilitaires widgets
│   ├── widget_*.c/.h           # Un fichier par widget (50 widgets)
│   ├── gtk4-compat.h           # Shims GTK3→GTK4 (65+) force-inclus
│   ├── glade_support.c/.h      # Support GtkBuilder
│   ├── stringman.c/.h          # Manipulation de chaînes
│   ├── tag_attributes.c/.h     # Traitement des attributs XML
│   ├── printing.c/.h           # Support impression
│   └── Makefile.am
├── configure.in                # Autoconf — GTK4, vte-2.91-gtk4
├── doc/
│   ├── reference/              # Référence HTML par widget
│   └── gtk4sermo.texi         # Documentation Texinfo
├── examples/                   # Exemples par widget
├── CAHIER_DES_CHARGES.md       # Ce document
├── MANUEL_UTILISATEUR.md
├── MANUEL_DEVELOPPEUR.md
└── COMPILE.md                  # Instructions de compilation rapide
```

### 7.1 Flux de traitement d'une requête

```
Script shell → variable d'environnement / stdin / fichier
    ↓
gtkdialog_lexer.l (Flex) — tokenisation
    ↓
gtkdialog_parser.y (Bison) — arbre syntaxique
    ↓
automaton.c — création des widgets GTK4
    ↓
GMainLoop (_gtk4sermo_main_loop) — boucle événements GTK4
    ↓
signals.c + actions.c — traitement des interactions
    ↓
[safe_system() / safe_popen()] — exécution sécurisée
    ↓
variables.c — export des valeurs sur stdout
```

---

## 8. Livrables

| ID | Livrable | Format | Statut |
|----|----------|--------|--------|
| L1 | Code source GTK4 (dossier `gtk4sermo/gtk4sermo_1.0.0/`) | C17 | ✅ Livré |
| L2 | `safe_exec.c/.h` — wrappers sécurité | C17 | ✅ Livré |
| L3 | `configure.in` mis à jour GTK4/vte-2.91-gtk4 | Autoconf | ✅ Livré |
| L4 | `src/Makefile.am` avec flags sécurité | Automake | ✅ Livré |
| L5 | Cahier des charges | Markdown | ✅ Livré |
| L6 | Manuel utilisateur | Markdown | ✅ Livré |
| L7 | Manuel développeur/mainteneur | Markdown | ✅ Livré |
| L8 | Guide de compilation rapide (`COMPILE.md`) | Markdown | ✅ Livré |
| L9 | Binaire `gtk4sermo` GTK4 compilé et testé | ELF x86_64 | ⏳ À faire |
| L10 | 50 widget_*.c/.h GTK4 | C17 | ✅ Livré |
| L11 | gtk4-compat.h (65+ shims) | C17 | ✅ Livré |
| L12 | Pages doc/reference/ HTML | HTML | ✅ Livré |
| L13 | Scripts examples/ | Shell | ✅ Livré |
| L14 | ROADMAP.md | Markdown | ✅ Livré |
| L15 | Paquet `.deb` gtk4sermo | Debian package | ⏳ Planifié |

---

## 9. Planification et jalons

| Jalon | Description | État |
|-------|-------------|------|
| J1 | Reprise de la base gtk3d (GTK3) | ✅ Terminé |
| J2 | Conception de la couche `gtk4-compat.h` (shims GTK3→GTK4) | ✅ Terminé |
| J3 | Migration GTK3 → GTK4 (conteneurs, boucle, dialogues async) | ✅ Terminé |
| J4 | Mise à jour du build system (GTK4, vte-2.91-gtk4) | ✅ Terminé |
| J5 | Documentation complète | ✅ Terminé |
| J6 | Ajout des widgets GTK4-natifs (revealer, stack, flowbox, overlay) | ✅ Terminé |
| J7 | Portage des widgets GTK3 vers GTK4 | ✅ Terminé |
| J8 | Intégration parser pour les nouveaux tokens GTK4 | ⏳ En cours |
| J9 | Première compilation complète gtk4sermo | ⏳ En attente |
| J10 | Tests fonctionnels sur les 50 widgets | ⏳ Planifié |
| J11 | Portage natif des stubs (menubar, menuitem, table, drawingarea) | ⏳ Planifié |
| J12 | Création du paquet .deb gtk4sermo | ⏳ Planifié |

---

## 10. Critères d'acceptation

Le projet sera considéré comme **terminé** (version 1.0.0 stable) lorsque :

1. Le binaire compile sans erreur sur Debian 12 et Ubuntu 22.04 avec `gcc -Wall -Wextra`
2. Les 50 widgets fonctionnent (tests manuels sur les exemples fournis dans `examples/`)
3. `checksec --file=gtk4sermo` retourne : RELRO=Full, Stack=Canary, NX=Enabled, PIE=Enabled
4. Aucun appel direct à `system()` ou `popen()` dans le code source (`grep` de vérification = 0 résultat)
5. Aucun `strcpy`, `strcat`, `sprintf` non borné (`grep` de vérification = 0 résultat)
6. Le script d'exemple `examples/button/button` fonctionne sans modification
7. `valgrind --leak-check=full ./gtk4sermo --stdin < examples/button/button` retourne 0 erreur mémoire

---

## 11. Risques identifiés

| Risque | Probabilité | Impact | Mitigation |
|--------|-------------|--------|------------|
| Shims `gtk4-compat.h` incomplets pour certaines APIs GTK3 retirées | Haute | Moyen | Couverture étendue au fil des erreurs de compilation, documentée dans COMPILE.md |
| API VTE GTK4 différente de la variante GTK3 | Haute | Moyen | Liaison contre `vte-2.91-gtk4`, paramètres `GCancellable` ajustés |
| Stubs (`menubar`, `menuitem`, `table`, `drawingarea`) sans rendu complet | Moyenne | Moyen | Portage natif planifié v1.2 ; fallback documenté |
| Dialogues asynchrones GTK4 (couleur, police, fichier) changent le flux | Moyenne | Faible | Adaptation du moteur d'actions aux callbacks async |
| Divergence de rendu GTK4 vs GTK3 sur scripts existants | Faible | Moyen | Suite XML de régression partagée entre ports |

---

## 12. Glossaire

| Terme | Définition |
|-------|-----------|
| **gtk4sermo** | Nom du binaire — port GTK4 de gtk3d, crée des GUI GTK4 via XML |
| **gtk3d** | Port GTK3 (référence) dont gtk4sermo est dérivé |
| **GTK3** | Bibliothèque graphique GNOME, version 3.x (active, en voie de retrait) |
| **GTK4** | Successeur de GTK3 (2020+), incompatible avec GTK3 |
| **gtk4-compat.h** | Couche de compatibilité (65+ shims) émulant les APIs GTK3 retirées en GTK4 |
| **GLib** | Bibliothèque de bas niveau fournie avec GTK : types, collections, threads |
| **GtkBuilder** | Système GTK natif de chargement d'UI depuis XML (remplace libglade) |
| **VTE** | Virtual Terminal Emulator — bibliothèque pour widget terminal GTK |
| **Autotools** | Suite de build GNU : autoconf + automake + libtool |
| **PIE** | Position Independent Executable — protection mémoire ASLR |
| **RELRO** | RELocation Read-Only — protection contre écrasement des pointeurs ELF |
| **safe_system** | Wrapper `safe_exec.c` remplaçant `system()` par `g_spawn_sync()` |
| **safe_popen** | Wrapper `safe_exec.c` remplaçant `popen()` par `g_spawn_async_with_pipes()` |

---

## Remerciements — Assistance IA

Ce travail a été réalisé avec l'assistance de **Claude**,
le modèle de langage d'[Anthropic](https://www.anthropic.com),
dans le cadre du développement de la famille **haplo-dialog** sur Haplo-Linux.

L'IA a contribué à : la conception de la couche `gtk4-compat.h`, le portage des
widgets de GTK3 vers GTK4, l'ajout des widgets GTK4-natifs (`revealer`, `stack`,
`flowbox`, `overlay`), l'adaptation des dialogues asynchrones GTK4, la rédaction
de la documentation et la mise à jour des fichiers de packaging.

> *"Ce projet a bénéficié de l'assistance de Claude (Anthropic) —
> mai 2026."*

---

*Document horodaté — dernière mise à jour : 2026-06-07 (audit #4 — compilation complète gtk4sermo).*
