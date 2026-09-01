# Cahier des Charges — qt6sermo

**Distributeur :** haplo-dialog | **Distribution :** haplo-dialog (Debian Testing)  
**Noyau :** Linux 7.0.4+deb14-amd64 | **DE :** Xfce4 4.20 / KDE Plasma 6 (Wayland)  
**Toolkit :** Qt6 >= 6.2.0  
**Projet :** qt6sermo-1.0.0  
**Version :** 1.0.0  
**Date :** Mai 2026 (mis à jour 2026-05-29)
**Statut :** 🟢 STABLE — 47 fichiers `widget_*.cpp` (49 widgets fonctionnels) ; bibliothèque compilée (Qt6 + CMake, bilan 90/100)
**Contact :** devel@haplo-dialog.fr  
**Dépôt :** https://haplo-dialog.fr  

---

## Table des matières

1. [Contexte et historique](#1-contexte-et-historique)
2. [Périmètre du projet](#2-périmètre-du-projet)
3. [Objectifs](#3-objectifs)
4. [Exigences fonctionnelles](#4-exigences-fonctionnelles)
5. [Exigences non fonctionnelles](#5-exigences-non-fonctionnelles)
6. [Contraintes techniques](#6-contraintes-techniques)
7. [Architecture cible](#7-architecture-cible)
8. [Correspondance widgets GTK3 → Qt6](#8-correspondance-widgets-gtk3--qt6)
9. [Couche de compatibilité qt6-compat.h](#9-couche-de-compatibilité-qt6-compath)
10. [Livrables](#10-livrables)
11. [Planification et jalons](#11-planification-et-jalons)
12. [Critères d'acceptation](#12-critères-dacceptation)
13. [Risques identifiés](#13-risques-identifiés)

---

## 1. Contexte et historique

### 1.1 Origine

`qt6sermo` est le port de la famille haplo-dialog vers Qt6. À l'origine du projet,
il était le seul membre de la famille à offrir les 30 widgets du catalogue gtkdialog
d'origine, y compris `<menubar>` et `<table>` alors en stub dans les autres ports.
Aujourd'hui le catalogue s'est étendu : qt6sermo offre 49 widgets fonctionnels,
et gtk4dialog (54) est le port le plus fourni de la famille.

### 1.2 Motivation

Qt6 est la toolkit graphique la plus complète et la mieux documentée. `kdialog` (KDE)
existe mais ne fournit que des boîtes de dialogue ponctuelles. `qt6sermo` sera le
premier constructeur XML complet pour Qt6, s'intégrant nativement dans les environnements
KDE Plasma 6. Les widgets `<menubar>`, `<table>` et `<terminal>` (via `QTermWidget`) sont
tous disponibles nativement dans Qt6, ce qui en faisait à l'époque le port le plus
complet (gtk4dialog, 54 widgets, est aujourd'hui le plus fourni).

---

## 2. Périmètre du projet

### 2.1 Inclus

- Réutilisation du core gtk3d (32 fichiers, ~23 500 lignes) sans modification
- Couche de compatibilité `qt6-compat.h` / `qt6-compat.cpp` (shims GLib → stdlib, `extern "C"`)
- Réécriture de la couche widget en C++/Qt6 (`widget_*.cpp`, ~30 fichiers)
- Build system CMake avec `CMAKE_AUTOMOC=ON` (obligatoire pour QObject/signaux Qt)
- `<menubar>`, `<menu>`, `<menuitem>` via `QMenuBar` / `QAction`
- `<table>` via `QTableWidget`
- `<terminal>` via `QTermWidget` (optionnel — paquet `qtermwidget6`)
- Documentation complète

### 2.2 Hors périmètre (version 0.9.x)

- Support Glade/GtkBuilder (non applicable Qt6)
- Portage Windows/macOS (prévu ultérieurement)
- Thème non-Qt (le look Qt6/Breeze est natif)

---

## 3. Objectifs

| Priorité | Objectif |
|----------|---------|
| P1 | Produire un binaire `qt6sermo` compilant sur Debian Testing |
| P1 | Coexistence sans conflit avec `gtk3d`, `gtk4d`, `fltk1d` |
| P1 | 30/30 widgets du catalogue gtkdialog d'origine (objectif initial — 49 widgets offerts aujourd'hui) |
| P1 | Intégration thème KDE Plasma 6 native |
| P2 | `<menubar>` et `<table>` fonctionnels (manquants dans GTK4) |
| P2 | `<terminal>` via `QTermWidget` |
| P3 | Package `.deb` sur haplo-dialog |

---

## 4. Exigences fonctionnelles

### 4.1 Parsing XML

Identique aux autres ports — moteur Flex/Bison hérité sans modification.

### 4.2 Widgets complets (30/30)

| Widget XML | Composant Qt6 | Priorité |
|------------|---------------|----------|
| `<window>` | `QMainWindow` | P1 |
| `<button>` | `QPushButton` | P1 |
| `<checkbox>` | `QCheckBox` | P1 |
| `<radiobutton>` | `QRadioButton` + `QButtonGroup` | P1 |
| `<entry>` | `QLineEdit` | P1 |
| `<text>` | `QTextEdit` (readOnly) | P1 |
| `<edit>` | `QTextEdit` | P1 |
| `<hbox>` | `QWidget` + `QHBoxLayout` | P1 |
| `<vbox>` | `QWidget` + `QVBoxLayout` | P1 |
| `<frame>` | `QGroupBox` | P1 |
| `<notebook>` | `QTabWidget` | P1 |
| `<expander>` | `QGroupBox` collapsible custom | P2 |
| `<list>` | `QListWidget` | P1 |
| `<tree>` | `QTreeWidget` | P2 |
| `<combobox>` | `QComboBox` | P1 |
| `<comboboxtext>` | `QComboBox` (editable=true) | P1 |
| `<progressbar>` | `QProgressBar` | P1 |
| `<hscale>` | `QSlider` (Qt::Horizontal) | P1 |
| `<vscale>` | `QSlider` (Qt::Vertical) | P1 |
| `<spinbutton>` | `QSpinBox` / `QDoubleSpinBox` | P1 |
| `<statusbar>` | `QStatusBar` | P2 |
| `<colorbutton>` | `QPushButton` → `QColorDialog` | P2 |
| `<fontbutton>` | `QPushButton` → `QFontDialog` | P2 |
| `<timer>` | `QTimer` | P2 |
| `<togglebutton>` | `QPushButton` (checkable=true) | P1 |
| `<hseparator>` | `QFrame` (HLine) | P1 |
| `<vseparator>` | `QFrame` (VLine) | P1 |
| `<pixmap>` | `QLabel` + `QPixmap` | P2 |
| `<terminal>` | `QTermWidget` (optionnel) | P3 |
| `<menubar>` | **`QMenuBar`** ✅ Qt6_BONUS | P2 |
| `<menu>` | **`QMenu`** ✅ Qt6_BONUS | P2 |
| `<menuitem>` | **`QAction`** ✅ Qt6_BONUS | P2 |
| `<table>` | **`QTableWidget`** ✅ Qt6_BONUS | P2 |

### 4.3 Signaux Qt6

La connexion signaux/slots Qt6 utilise la syntaxe pointeurs de fonctions membres.
Elle remplace les `g_signal_connect()` GTK dans la couche widget :

```cpp
// GTK3 (core C)
g_signal_connect(widget, "clicked", G_CALLBACK(handler), data);

// Qt6 (widget_*.cpp)
connect(button, &QPushButton::clicked, [=]() { /* handler */ });
```

Le core C (`signals.c`) n'est pas modifié — il gère les signaux au niveau XML
(attributs `<action>`). Les connexions Qt6 sont dans les `widget_*.cpp`.

---

## 5. Exigences non fonctionnelles

| Critère | Cible |
|---------|-------|
| Taille dépendances | ~50 Mo Qt6 (lourd mais acceptable pour KDE) |
| Intégration thème | Thème Breeze/KDE natif sous KDE Plasma |
| Wayland | Qt6 est Wayland-first — aucune adaptation requise |
| Sécurité | Hérite des correctifs du core audité |
| Licence | Qt6 LGPL-2.0-or-later — compatible redistribution .deb |

---

## 6. Contraintes techniques

### 6.1 Meta-Object Compiler (MOC)

Toute classe héritant de `QObject` (pour signaux/slots) doit passer par `moc`.
CMake avec `CMAKE_AUTOMOC=ON` génère automatiquement les fichiers `moc_*.cpp`.
**Contrainte :** les fichiers `.h` avec `Q_OBJECT` doivent être dans les `SOURCES`
de la cible CMake, pas seulement les `.cpp`.

### 6.2 Interopérabilité C/C++

Le core est en C, les widgets en C++. Le pont est assuré par `extern "C"` dans
`qt6-compat.h`. Toutes les fonctions du core exposées dans les headers sont encadrées :

```c
#ifdef __cplusplus
extern "C" {
#endif
/* ... déclarations C ... */
#ifdef __cplusplus
}
#endif
```

### 6.3 QString ↔ char*

Les conversions entre `QString` (Qt6) et `char*` (core C) doivent être
systématisées :
- `char*` → `QString` : `QString::fromUtf8(ptr)`
- `QString` → `char*` : `qPrintable(str)` (valide pendant l'expression)
- Copie permanente : `strdup(qPrintable(str))` + `g_free()` plus tard

### 6.4 QTermWidget

`QTermWidget` est fourni par le paquet `qtermwidget6`. Le `CMakeLists.txt`
le détecte via `pkg_check_modules(QTERMWIDGET qtermwidget6)`. En cas d'absence,
`<terminal>` retourne un `QLabel` d'avertissement (stub).

---

## 7. Architecture cible

```
qt6sermo/qt6sermo_1.0.0/
├── CMakeLists.txt            ← Qt6, AUTOMOC, Flex/Bison, QTermWidget
├── src/
│   ├── qt6-compat.h/.cpp     ← shims GLib→stdlib, extern "C", GSList minimal
│   ├── [32 fichiers core]    ← copiés depuis gtk3d, non modifiés
│   └── [~30 fichiers widget] ← à implémenter en C++/Qt6
│       widget_window.cpp  widget_button.cpp  widget_menubar.cpp
│       widget_table.cpp   widget_terminal.cpp ...
├── examples/
└── doc/
```

---

## 8. Correspondance widgets GTK3 → Qt6

| Widget XML | GTK3 | Qt6 | Complexité |
|------------|------|-----|------------|
| `<window>` | GtkWindow | QMainWindow | Faible |
| `<button>` | GtkButton | QPushButton | Faible |
| `<checkbox>` | GtkCheckButton | QCheckBox | Faible |
| `<radiobutton>` | GtkRadioButton | QRadioButton | Faible |
| `<entry>` | GtkEntry | QLineEdit | Faible |
| `<text>` | GtkTextView (RO) | QTextEdit (readOnly) | Faible |
| `<edit>` | GtkTextView (RW) | QTextEdit | Faible |
| `<hbox>` | GtkHBox | QHBoxLayout | Faible |
| `<vbox>` | GtkVBox | QVBoxLayout | Faible |
| `<frame>` | GtkFrame | QGroupBox | Faible |
| `<notebook>` | GtkNotebook | QTabWidget | Faible |
| `<expander>` | GtkExpander | QGroupBox custom | Moyenne |
| `<list>` | GtkTreeView | QListWidget | Faible |
| `<tree>` | GtkTreeView | QTreeWidget | Moyenne |
| `<combobox>` | GtkComboBox | QComboBox | Faible |
| `<comboboxtext>` | GtkComboBoxText | QComboBox (edit.) | Faible |
| `<progressbar>` | GtkProgressBar | QProgressBar | Faible |
| `<hscale>` | GtkHScale | QSlider (Horiz.) | Faible |
| `<vscale>` | GtkVScale | QSlider (Vert.) | Faible |
| `<spinbutton>` | GtkSpinButton | QSpinBox | Faible |
| `<statusbar>` | GtkStatusbar | QStatusBar | Faible |
| `<colorbutton>` | GtkColorButton | QColorDialog | Faible |
| `<fontbutton>` | GtkFontButton | QFontDialog | Faible |
| `<timer>` | GLib timeout | QTimer | Faible |
| `<togglebutton>` | GtkToggleButton | QPushButton(check.) | Faible |
| `<hseparator>` | GtkHSeparator | QFrame(HLine) | Faible |
| `<vseparator>` | GtkVSeparator | QFrame(VLine) | Faible |
| `<pixmap>` | GdkPixbuf | QLabel + QPixmap | Faible |
| `<terminal>` | VTE | QTermWidget | Moyenne |
| `<menubar>` | GtkMenuBar | **QMenuBar** | Moyenne |
| `<menu>` | GtkMenu | **QMenu** | Moyenne |
| `<menuitem>` | GtkMenuItem | **QAction** | Moyenne |
| `<table>` | GtkCList | **QTableWidget** | Élevée |

---

## 9. Couche de compatibilité qt6-compat.h

| Catégorie | Couverture | Statut |
|-----------|------------|--------|
| Types GLib | 100 % | ✅ |
| Mémoire GLib | 100 % | ✅ |
| Chaînes GLib | 90 % | ✅ |
| Messages | 100 % | ✅ |
| GSList / GList (minimal) | 80 % | ✅ |
| `extern "C"` encapsulation | 100 % | ✅ |
| `gtk_init` / `gtk_main` stubs | 100 % | ✅ |
| API GTK widgets | 0 % | 🔴 Dans widget_*.cpp |

---

## 10. Livrables

| Livrable | Chemin | Format |
|----------|--------|--------|
| Binaire | `/usr/bin/qt6sermo` | ELF x86_64 |
| Package Debian | `qt6sermo_1.0.0_amd64.deb` | .deb |

---

## 11. Planification et jalons

| Jalon | Description | Durée |
|-------|-------------|-------|
| J0 | ✅ Infrastructure initialisée, core copié | Fait |
| J1 | `qt6-compat.h` complète, CMake valide | 2 jours |
| J2 | Fenêtre principale + 12 widgets P1 simples | 2 semaines |
| J3 | Menus (`QMenuBar`/`QAction`) + `<table>` (`QTableWidget`) | 1 semaine |
| J4 | Terminal (`QTermWidget`) + widgets P2 restants | 1 semaine |
| J5 | examples, packaging .deb | 3 jours |
| J6 | Documentation finale, BILAN_SANTE >= 85/100 | 2 jours |

**Durée totale estimée : 4–5 semaines**

---

## 12. Critères d'acceptation

- [ ] `cmake -B build && cmake --build build` réussit sans erreur
- [ ] `cmake --install build` installe `/usr/bin/qt6sermo`
- [ ] 30/30 widgets fonctionnels (dont menus et table)
- [ ] `<menubar>` et `<table>` pleinement opérationnels
- [ ] `checksec` : PIE ✅ RELRO ✅ Stack canary ✅
- [ ] BILAN_SANTE.md >= 80/100

---

## 13. Risques identifiés

| Risque | Probabilité | Impact | Mitigation |
|--------|-------------|--------|------------|
| MOC complexifie le build | Haute | Faible | `CMAKE_AUTOMOC=ON` gère automatiquement |
| `QTableWidget` vs modèle CList incompatible | Moyenne | Moyen | Adapter l'attribut `<table>` aux colonnes QTableWidget |
| `QString`/`char*` conversions manquées | Moyenne | Moyen | Wrapper `qstr()` / `cstr()` dans qt6-compat.h |
| QTermWidget non disponible sur Debian | Faible | Faible | Stub acceptable |
| Dépendances Qt6 lourdes (~50 Mo) | Certaine | Faible | Acceptable — Haplo supporte KDE |

---

## Remerciements — Assistance IA

Ce travail a été réalisé avec l'assistance de **Claude**,
le modèle de langage d'[Anthropic](https://www.anthropic.com),
dans le cadre du développement de la famille **haplo-dialog** sur haplo-dialog.

L'IA a contribué à : la conception de l'architecture, l'implémentation des widgets,
la rédaction de la documentation, la création des tests unitaires (CTest),
l'intégration de QTermWidget6, la sous-classe Fl_Table hiérarchique (FLTK),
l'amélioration de l'export d'état SDL3/ImGui, et la mise à jour de tous les
fichiers de packaging et de documentation.

> *"Ce projet a bénéficié de l'assistance de Claude (Anthropic) —
> mai 2026."*

---

*Document horodaté — dernière mise à jour : 2026-06-10 (audit #6 — haplo-dialog v1.0.0).*