# Cahier des Charges — gtk3sermo

**Projet :** haplo-dialog | **Distribution cible :** Debian Testing  
**GTK :** GTK 3 ≥ 3.22 | **VTE :** vte-2.91 *(optionnel)*  
**Dépôt :** https://gitlab.com/haplo-dialog/sermo  

**Projet :** gtk3sermo-1.0.0  
**Version :** 1.0.0  
**Date :** Mai 2026  
**Statut :** En développement actif  
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

`gtk3sermo` (aussi distribué sous le nom `gtkd`) est un utilitaire C open source permettant de construire des interfaces graphiques GTK à partir de descriptions XML embarquées dans des scripts shell ou d'autres langages interprétés. Initialement développé par László Pere (2003–2007) puis maintenu par Thunor (2011–2012), il a atteint la version 0.8.3 avant d'être abandonné.

Le paquet `gtkd_0.8.3-parrot0_all.deb` redistribué pour Parrot OS date de février 2016. Il dépend de **GTK 2.0**, déclaré en fin de vie depuis 2011 et définitivement retiré des distributions majeures en 2020.

### 1.2 Problème

En 2026, ce paquet ne peut plus être installé ni compilé sur aucun système moderne :

- **GTK 2.0** n'est plus packagé (Ubuntu 22+, Debian 12+, Fedora 36+)
- **libglade-2.0** est entièrement supprimé
- **python-gtk2-dev** n'existe plus
- Le code source contient des **vulnérabilités de sécurité critiques** (injection shell, buffer overflows)
- Le système de build Autotools n'est pas configuré pour les compilateurs modernes

### 1.3 Opportunité

`gtk3sermo` reste conceptuellement unique : aucun outil équivalent en C n'offre la même simplicité pour créer des GUI GTK depuis un script. Le projet mérite une modernisation plutôt qu'un abandon.

---

## 2. Périmètre du projet

### 2.1 Inclus dans ce projet

- Migration complète de GTK 2 vers **GTK 3** (>= 3.22)
- Correction de toutes les **vulnérabilités de sécurité** identifiées
- Mise à jour du **système de build** (configure.in / Makefile.am)
- Mise à jour de la dépendance **VTE** vers vte-2.91
- Remplacement de **libglade** par GtkBuilder (intégré GTK3)
- Rédaction de la **documentation** (cahier des charges, manuel utilisateur, manuel développeur)
- Conservation de **100% de la compatibilité descendante** de la syntaxe XML gtk3sermo

### 2.2 Hors périmètre (version 0.9.x)

- Migration vers GTK 4 (prévu en version 1.0)
- Réécriture du parser (Yacc/Lex actuel conservé)
- Ajout de nouveaux widgets non présents dans l'original
- Interface de configuration graphique
- Support Windows/macOS

---

## 3. Objectifs

### 3.1 Objectif principal

Produire un binaire `gtk3sermo` fonctionnel sur tout système Linux moderne avec GTK 3, compilable sans erreur, sécurisé, et compatible avec tous les scripts écrits pour gtk3sermo 0.8.3.

### 3.2 Objectifs secondaires

| ID | Objectif | Priorité |
|----|----------|----------|
| O1 | Zéro vulnérabilité critique (injection shell, buffer overflow) | HAUTE |
| O2 | Compilation sans erreur sur Debian 12, Ubuntu 22.04, Fedora 38 | HAUTE |
| O3 | Compatibilité 100% des scripts gtkdialog 0.8.3 existants | HAUTE |
| O4 | Documentation complète en français | MOYENNE |
| O5 | Réduire les avertissements de compilation (-Wall -Wextra) | MOYENNE |
| O6 | Préparation de la base pour migration GTK4 future | BASSE |

---

## 4. Exigences fonctionnelles

### 4.1 Widgets supportés (identiques à 0.8.3)

Le binaire doit créer et gérer les 30 widgets suivants, identiques en comportement à la version originale :

`button`, `cancelbutton`, `helpbutton`, `nobutton`, `okbutton`, `yesbutton`, `togglebutton`, `checkbox`, `radiobutton`, `entry`, `edit` (zone de texte multi-lignes), `text` (étiquette), `spinbutton`, `hscale`, `vscale`, `progressbar`, `combobox`, `comboboxtext`, `comboboxentry`, `list`, `tree`, `pixmap`, `colorbutton`, `fontbutton`, `notebook`, `frame`, `hbox`, `vbox`, `menubar`, `statusbar`, `timer`, `eventbox`, `expander`, `terminal`, `window`

### 4.2 Modes d'entrée du programme

- `gtk3sermo --stdin` — lire la définition XML depuis l'entrée standard
- `gtk3sermo --program=VAR_ENV` — lire depuis une variable d'environnement
- `gtk3sermo --file=chemin.xml` — lire depuis un fichier
- `gtk3sermo --geometry=WxH+X+Y` — positionner la fenêtre

### 4.3 Actions supportées

Le moteur d'actions doit traiter : `EXIT`, `CLOSE`, `LAUNCH`, `REFRESH`, `SAVE`, `CLEAR`, `APPEND`, `FILESELECT`, `REMOVESELECTED`, `ENABLE`, `DISABLE`, `SHOW`, `HIDE`, `ACTIVATE`, `GRABFOCUS`, `PRESENTWINDOW`

### 4.4 Sortie du programme

À la fermeture, le programme doit exporter sur stdout les paires `NOM_VARIABLE=valeur` pour tous les widgets nommés, conformément au comportement gtk3sermo 0.8.3.

### 4.5 Gestion des commandes externes

Tous les attributs `<input>`, `<action>` et commandes shell associés aux widgets doivent être exécutés de manière **sécurisée** via `safe_system()` / `safe_popen()` sans invoquer `/bin/sh -c` directement.

### 4.6 Nouvelles exigences fonctionnelles — patch 18 (2026-05-23)

| Réf. | Exigence | Statut |
|------|----------|--------|
| EF-18-1 | Les commentaires XML `<!-- ... -->` dans les définitions de dialog doivent être silencieusement ignorés par le parser | ✅ Implémenté |
| EF-18-2 | `<frame label="texte">` doit afficher `texte` comme titre du cadre GTK (et non la chaîne brute `label="texte"`) | ✅ Implémenté |
| EF-18-3 | `<window>` doit supporter les attributs `default-width`, `default-height` (taille initiale suggérée), `width-request`, `height-request` (minimum) et `resizable` | ✅ Implémenté |
| EF-18-4 | La fenêtre doit s'ouvrir à sa taille naturelle et ne jamais dépasser 95 % de la zone de travail du moniteur primaire | ✅ Implémenté |
| EF-18-5 | `<edit>` avec `<input>commande</input>` doit pré-remplir la zone de texte avec la sortie de la commande | ✅ Implémenté |
| EF-18-6 | Le symlink `gtk3sermo → gtk3sermo` doit être créé à l'installation (`AC_PROG_LN_S`) | ✅ Implémenté |

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
- Chaque fichier source doit avoir un en-tête de licence GPL-2.0

### 5.5 Compatibilité descendante

Tous les scripts shell utilisant gtk3sermo 0.8.3 doivent fonctionner sans modification avec gtk3sermo 1.0.0.

---

## 6. Contraintes techniques

### 6.1 Dépendances autorisées

| Bibliothèque | Version minimale | Justification |
|---|---|---|
| GTK+ | 3.22.0 | Interface graphique principale |
| GLib | 2.56.0 | Fournie avec GTK3, utilitaires |
| GIO | 2.56.0 | Fournie avec GTK3 |
| VTE | 0.52 (vte-2.91) | Widget terminal émulateur |
| Flex | 2.6+ | Génération du lexer |
| Bison | 3.0+ | Génération du parser |

### 6.2 Dépendances supprimées

| Bibliothèque | Raison de suppression | Remplacement |
|---|---|---|
| GTK+ 2.0 | Fin de vie | GTK+ 3.x |
| libglade-2.0 | Abandonné | GtkBuilder (GTK3 intégré) |
| python-gtk2-dev | Supprimé des dépôts | Aucun (pas utilisé en runtime) |
| gthread-2.0 | Intégré dans GLib >= 2.32 | GLib/GTK3 |

### 6.3 Licence

Le projet reste sous **GPL-2.0-or-later**, conformément à l'original.

---

## 7. Architecture cible

```
gtk3sermo/
├── src/
│   ├── gtk3sermo.c/.h          # Point d'entrée, parsing des arguments
│   ├── safe_exec.c/.h          # NOUVEAU: wrappers sécurisés system/popen
│   ├── automaton.c/.h          # Moteur d'exécution (AST → widgets GTK)
│   ├── gtkdialog_parser.y      # Parser Bison (syntaxe XML gtk3sermo)
│   ├── gtkdialog_lexer.l       # Lexer Flex
│   ├── variables.c/.h          # Gestion des variables et de l'état
│   ├── signals.c/.h            # Système de signaux GTK
│   ├── actions.c/.h            # Traitement des actions (EXIT, REFRESH…)
│   ├── widgets.c/.h            # Dispatch et utilitaires widgets
│   ├── widget_*.c/.h           # Un fichier par widget (30 widgets)
│   ├── glade_support.c/.h      # Support GtkBuilder (ex-libglade)
│   ├── stringman.c/.h          # Manipulation de chaînes
│   ├── tag_attributes.c/.h     # Traitement des attributs XML
│   ├── printing.c/.h           # Support impression
│   └── Makefile.am
├── configure.in                # Autoconf — GTK3, VTE 2.91
├── doc/
│   ├── reference/              # Référence HTML par widget
│   └── gtk3sermo.texi          # Documentation Texinfo
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
automaton.c — création des widgets GTK3
    ↓
gtk_main() — boucle événements GTK
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
| L1 | Code source modernisé (dossier `gtk3sermo/`) | C17 | ✅ Livré |
| L2 | `safe_exec.c/.h` — wrappers sécurité | C17 | ✅ Livré |
| L3 | `configure.in` mis à jour GTK3/VTE-2.91 | Autoconf | ✅ Livré |
| L4 | `src/Makefile.am` avec flags sécurité | Automake | ✅ Livré |
| L5 | Cahier des charges | Markdown | ✅ Livré |
| L6 | Manuel utilisateur | Markdown | ✅ Livré |
| L7 | Manuel développeur/mainteneur | Markdown | ✅ Livré |
| L8 | Guide de compilation rapide (`COMPILE.md`) | Markdown | ✅ Livré |
| L9 | Binaire `gtk3sermo` compilé et testé | ELF x86_64 | ⏳ À faire (nécessite `libgtk-3-dev`) |
| L10 | Paquet `.deb` modernisé | Debian package | ⏳ Planifié v0.9.1 |

---

## 9. Planification et jalons

| Jalon | Description | État |
|-------|-------------|------|
| J1 | Extraction et analyse du code source original | ✅ Terminé |
| J2 | Correctifs de sécurité (safe_exec, strcpy, malloc) | ✅ Terminé |
| J3 | Migration GTK2 → GTK3 (45 fichiers) | ✅ Terminé |
| J4 | Mise à jour du build system | ✅ Terminé |
| J5 | Documentation complète | ✅ Terminé |
| J5b | Correction `pclose()` → `fclose()` (21 fichiers, UB sur fdopen) | ✅ Terminé (2026-05-21) |
| J6 | Première compilation et correction des erreurs résiduelles | ✅ Terminé (2026-05-22 patch 17) |
| J6b | Bugs runtime : commentaires XML, frame label, window attrs, edit input | ✅ Terminé (2026-05-23 patch 18) |
| J7 | Tests fonctionnels sur les 30 widgets | ⏳ Planifié |
| J8 | Création du paquet .deb modernisé | ⏳ Planifié |

---

## 10. Critères d'acceptation

Le projet sera considéré comme **terminé** (version 1.0.0 stable) lorsque :

1. Le binaire compile sans erreur sur Debian 12 et Ubuntu 22.04 avec `gcc -Wall -Wextra`
2. Les 30 widgets fonctionnent (tests manuels sur les exemples fournis dans `examples/`)
3. `checksec --file=src/gtk3sermo` retourne : RELRO=Full, Stack=Canary, NX=Enabled, PIE=Enabled
4. Aucun appel direct à `system()` ou `popen()` dans le code source (`grep` de vérification = 0 résultat)
5. Aucun `strcpy`, `strcat`, `sprintf` non borné (`grep` de vérification = 0 résultat)
6. Le script d'exemple `examples/button/button` fonctionne sans modification
7. `valgrind --leak-check=full ./gtk3sermo --stdin < examples/button/button` retourne 0 erreur mémoire

---

## 11. Risques identifiés

| Risque | Probabilité | Impact | Mitigation |
|--------|-------------|--------|------------|
| Erreurs de compilation résiduelles dans glade_support.c | Haute | Moyen | Corrections manuelles documentées dans COMPILE.md |
| API VTE spawn_sync différente de fork_command | Haute | Moyen | Paramètres supplémentaires à ajuster (GCancellable) — ✅ CORRIGÉ session 2 |
| g_object_set_data() clé "user_data" incorrecte dans widget_list | Moyenne | Faible | Tests unitaires sur les listes — ✅ CORRIGÉ session 2 |
| `pclose()` sur `FILE*` issu de `fdopen()` (comportement indéfini) | ~~Haute~~ | ~~Moyen~~ | ✅ CORRIGÉ session 3 — 21 fichiers migrés vers `fclose()` |
| Comportement différent de GdkRGBA vs GdkColor (précision couleur) | Faible | Faible | Tests visuels colorbutton |
| Migration GTK4 rompant cette base GTK3 | Faible | Élevé | Abstraire les appels GTK dans des wrappers internes |
| `safe_popen()` incompatible avec pipes/redirections shell (`\|`, `&&`, `>`) | Moyenne | Moyen | Détection de métacaractères + bascule `/bin/sh -c` si nécessaire |

---

## 12. Glossaire

| Terme | Définition |
|-------|-----------|
| **gtk3sermo** | Nom du binaire et du projet — utilitaire C pour créer des GUI GTK via XML |
| **GTK3** | Bibliothèque graphique GNOME, version 3.x (active, maintenue) |
| **GTK4** | Successeur de GTK3 (2020+), incompatible avec GTK3 |
| **GLib** | Bibliothèque de bas niveau fournie avec GTK : types, collections, threads |
| **GtkBuilder** | Système GTK3 natif de chargement d'UI depuis XML (remplace libglade) |
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
dans le cadre du développement de la famille **haplo-dialog**.

L'IA a contribué à : la conception de l'architecture, l'implémentation des widgets,
la rédaction de la documentation, la création des tests unitaires (CTest),
et la mise à jour de tous les fichiers de packaging et de documentation.

> *"Ce projet a bénéficié de l'assistance de Claude (Anthropic) —
> mai 2026."*

---

*Document horodaté — dernière mise à jour : 2026-06-07 (audit #3 — haplo-dialog v1.0.0).*
