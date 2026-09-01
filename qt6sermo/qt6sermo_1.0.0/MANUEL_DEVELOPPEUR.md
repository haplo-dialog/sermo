# Manuel Développeur et Mainteneur — qt6sermo (qt6sermo)

**Version :** 1.0.0-qt6
**Date :** Mai 2026 (mis à jour 2026-05-29)
**Audience :** Développeurs C/C++, mainteneurs haplo-dialog
**Mainteneur :** S.Cage, D.Marques <devel@haplo-dialog.fr> | **Dépôt :** https://haplo-dialog.fr

---

## Table des matières

1. [Vue d'ensemble de l'architecture](#1-vue-densemble-de-larchitecture)
2. [Environnement de développement](#2-environnement-de-développement)
3. [Structure des fichiers source](#3-structure-des-fichiers-source)
4. [La couche de compatibilité Qt6](#4-la-couche-de-compatibilité-qt6)
5. [Flux d'exécution](#5-flux-dexécution)
6. [Conventions de code](#6-conventions-de-code)
7. [Sécurité — règles obligatoires](#7-sécurité--règles-obligatoires)
8. [Ajouter un nouveau widget](#8-ajouter-un-nouveau-widget)
9. [Système de build](#9-système-de-build)
10. [Tests et débogage](#10-tests-et-débogage)
11. [Tâches de maintenance récurrentes](#11-tâches-de-maintenance-récurrentes)
12. [Références](#12-références)

---

## 1. Vue d'ensemble de l'architecture

qt6sermo est le port **Qt6** du fork gtk3dialog (origine gtkdialog 0.8.3) et le
**port phare** de la famille. Il interprète le **même format XML** que gtkdialog
et construit l'interface avec les Qt Widgets (Qt6 Core + Widgets). Le cœur reste
en C ; widgets et shim sont en C++ (Qt est C++).

```
+-----------------------------------------------------------+
|                    COUCHE D'ENTREE (C)                    |
|   gtkdialog.c  <- arguments CLI, lecture XML              |
|   gtkdialog_lexer.l + gtkdialog_parser.y  <- parsing      |
+--------------------------+--------------------------------+
                           | Arbre d'instructions (AST)
+--------------------------v--------------------------------+
|                    COUCHE MOTEUR (C)                      |
|   automaton.c / variables.c / signals.c / actions.c       |
|   safe_exec.c <- execution securisee de commandes         |
+--------------------------+--------------------------------+
                           | Appels via qt6-compat
+--------------------------v--------------------------------+
|              COUCHE WIDGETS + COMPAT (C++)               |
|   qt6-compat.cpp/.h <- GtkWidget* -> QWidget*             |
|   widget_*.cpp      <- implementations Qt Widgets         |
+-----------------------------------------------------------+
```

### 1.1 Principes de conception

- **Compatibilité XML d'abord** : un dialogue gtk3d tourne sous qt6sermo sans
  modification.
- **Cœur partagé** : actions/automaton/variables/signals/safe_exec/stack/
  stringman/lexer/parser communs aux ports.
- **Spécificités Qt** : les signaux/slots Qt remplacent les signaux GTK ; le shim
  fait le pont entre le système d'actions C et le mécanisme signal/slot.

> **Note d'identité :** le fichier `src/gtk3d.h` encore présent est un en-tête de
> compatibilité de noms hérité ; il ne doit pas être confondu avec le port gtk3d.
> Le binaire et le projet s'appellent bien `qt6sermo` / qt6sermo.

---

## 2. Environnement de développement

Voir `COMPILE.md`. En résumé :

```bash
sudo apt-get install cmake qt6-base-dev flex bison pkg-config gcc g++
# tests unitaires : libglib2.0-dev
cd qt6sermo_1.0.0/
cmake -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build -j$(nproc)
```

> **Qt6 (Core + Widgets)** requis, C++17 minimum. flex + bison obligatoires.

---

## 3. Structure des fichiers source

| Fichier | Langage | Rôle |
|---------|---------|------|
| `gtkdialog.c` | C | `main()`, options CLI, lecture XML |
| `gtkdialog_lexer.l` / `gtkdialog_parser.y` | C | Lexer/parser du format XML |
| `automaton.c` | C | Exécute l'AST, instancie les widgets |
| `variables.c` | C | État des widgets, export stdout |
| `signals.c` | C | Lie signaux/slots Qt au système d'actions |
| `actions.c` | C | Dispatch des actions |
| `safe_exec.c` | C | Exécution sécurisée des commandes |
| `qt6-compat.cpp/.h` | C++ | Shim : `GtkWidget*` → `QWidget*`, stubs Gdk |
| `widget_*.cpp/.h` | C++ | Une implémentation par widget |
| `gtk3d.h` | C | En-tête de compat de noms hérité (pas le port gtk3d) |

---

## 4. La couche de compatibilité Qt6

`qt6-compat.h/.cpp` traduit le modèle GTK vers Qt Widgets :

- `GtkWidget*` → enveloppe sur `QWidget*` ;
- événements Gdk → structures opaques pour que `signals.c` compile sans GTK ;
- fonctions GTK courantes → équivalents Qt (création, layout, show/hide,
  données attachées via `QObject::setProperty`/`property`) ;
- pont signaux : un callback C est connecté à un signal Qt via une lambda/objet
  de liaison côté C++.

> **Règle :** tout nouvel appel Qt passe par le shim ou `widget_*.cpp`, jamais
> dans le cœur C.

---

## 5. Flux d'exécution

1. `main()` lit options + XML.
2. Lexer/parser → AST.
3. `automaton.c` instancie les QWidgets via la couche compat.
4. `signals.c` connecte les signaux Qt aux actions.
5. Boucle d'événements Qt (`QApplication::exec()`).
6. À la fermeture, `variables.c` exporte l'état sur stdout.

---

## 6. Conventions de code

- Cœur en C, widgets/shim en C++17.
- `stackelement s = {0};` à l'initialisation.
- Pas d'appel Qt direct hors shim/`widget_*.cpp`.
- Respecter les conventions MOC si introduction de classes `QObject` (mais le
  port évite autant que possible le MOC au profit de lambdas).

---

## 7. Sécurité — règles obligatoires

Identiques au port de référence : `safe_exec.c` (exec direct sans `/bin/sh -c`
quand possible, fallback métacaractères, récupération des zombies, garde chaîne
vide). Ne pas remplacer `fclose()` par `pclose()` sur un flux `safe_popen`. Les
flags de hardening sont posés via `target_link_options` dans `CMakeLists.txt`.

> **Règle (2026-06-10) :** les `<action>` d'un widget passent par
> `execute_action()` (itération de `ATTR_ACTION`, tagattr `function`/`type`),
> jamais directement par `safe_system()` — sinon les directives comme
> `EXIT:ok` sont traitées comme des commandes shell (défaut corrigé dans
> `widget_timer.cpp`).

---

## 8. Ajouter un nouveau widget

1. `widget_xxx.cpp` + `.h`.
2. Implémenter création Qt + get/set de valeur + connexion signal→action.
3. Déclarer dans le dispatch (`automaton`).
4. Ajouter aux `CORE_SOURCES`/`WIDGET_SOURCES` de `CMakeLists.txt`.
5. Documenter dans `doc/reference/xxx.html`.

---

## 9. Système de build

**CMake**, flex + bison :

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j$(nproc)
ctest --test-dir build --output-on-failure   # tests si GLib présent
```

`test_safe_exec` et `test_compat` ne sont compilés que si GLib est détecté
(`find_package(PkgConfig QUIET)`). CMake gère les chemins avec espaces/`&`.

---

## 10. Tests et débogage

```bash
echo '<window><vbox><text><label>Hi</label></text></vbox></window>' | ./build/qt6sermo --stdin
cmake -B build -DCMAKE_BUILD_TYPE=Debug && cmake --build build -j$(nproc)
QT_LOGGING_RULES="*.debug=true" ./build/qt6sermo --stdin < dialog.xml
gdb ./build/qt6sermo
```

Trois suites partagées à la racine du dépôt (résultats 2026-06-10) :

```bash
bash ../../tests/xml/run_tests.sh qt6sermo                      # parse, 52/52
sh   ../../tests/run_unit_tests.sh qt6sermo                     # cœur safe_exec, 9/9
xvfb-run -a sh ../../tests/behaviour/run_behaviour_tests.sh qt6sermo  # widgets, 12/12
```

La suite `tests/behaviour/` ouvre de vraies fenêtres (auto-fermées par un
`<timer>` → `EXIT:ok`) et vérifie les exports `VAR="valeur"` sur stdout ;
la CI l'exécute sous `xvfb-run` (étages docs / build-test / qa).

---

## 11. Tâches de maintenance récurrentes

- Suivre Qt6 (mises à jour mineures) et les dépréciations d'API Widgets.
- Garder `qt6-compat` synchronisé avec les besoins du cœur partagé.
- Vérifier l'identité (binaire/projet = qt6sermo ; `gtk3d.h` reste un alias hérité).
- `cppcheck --enable=warning,portability,style src/*.c` avant release ; exécuter
  les tests unitaires (`ctest`).

---

## 12. Références

- **Qt6 Widgets** : https://doc.qt.io/qt-6/qtwidgets-index.html
- **gtkdialog (origine)** : format XML hérité, voir `doc/qt6sermo.texi`
- **Cahier des charges** : `CAHIER_DES_CHARGES.md`
- **Bilan de santé** : `BILAN_SANTE.md`

---

*Document horodaté — dernière mise à jour : 2026-06-10 (audit #6 — haplo-dialog v1.0.0).*