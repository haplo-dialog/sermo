# Bilan de santé — qt6sermo 1.0.0

## État vérifié — 2026-06-10 — Score : **90/100**

| Critère | État |
|---|:--:|
| Build + install | ✅ rc=0 |
| Tests XML (52 cas) | ✅ 52/52 |
| Tests du cœur (`run_unit_tests.sh`, safe_exec) | ✅ 9/9 |
| Tests de comportement widgets (`tests/behaviour/`, 12 cas) | ✅ 12/12 |
| Warnings dangereux (casts/uninit) | ✅ 0 |
| Paquet `.deb` (lintian) | ✅ 0 tag |
| CI (`.gitlab-ci.yml`) | ✅ étages docs · build-test (xvfb-run) · qa |
| Widgets | 49 |

*Qt6 ; terminal live optionnel — ce bloc reflète l'état vérifié et fait foi ; les sections « patch … » ci-dessous sont l'historique antérieur (scores d'époque).*

## Score global

```
┌──────────────────────────────────────────────────────────────┐
│  qt6sermo 1.0.0 — Score de santé                          │
│                                                              │
│  Sécurité      ██████████████████░░   92/100  ✅              │
│  Migrations    ██████████████████░░   92/100  ✅              │
│  Code          ██████████████████░░   90/100  ✅              │
│  Build         ██████████████████░░   90/100  ✅              │
│  Documentation ███████████████████░   96/100  ✅              │
│                                                              │
│  TOTAL         ██████████████████░░   90/100  ✅              │
└──────────────────────────────────────────────────────────────┘
```

> **Référence :** gtk3d=90 · gtk4d=91 · qt6sermo=90 · fltk1d=89 · efl1d=90 · sdl3d=88

**Score précédent : 93/100.**

## ✅ Points forts

- Build CMake 3.20 moderne, Qt6 ≥ 6.2, AUTOMOC/AUTORCC
- Drapeaux de durcissement complets : FORTIFY_SOURCE=3, fstack-protector-strong,
  fstack-clash-protection, fcf-protection=full, PIE, Full RELRO, NX stack
- Pont C/C++ propre via qt6-compat.h + extern "C"
- QTermWidget6 optionnel avec fallback QPlainTextEdit
- QTreeWidget pour `<table>` multi-colonnes
- QMenuBar/QMenu complets pour `<menubar>`
- LTO Release activé

## ✅ Correctifs appliqués (v2)

| # | Fichier                  | Correction                                                  |
|---|--------------------------|-------------------------------------------------------------|
| 1 | qt6-compat.h             | `g_strdup_printf` VLA → `snprintf_safe()` deux-passes      |
| 2 | qt6-compat.h             | `gint32/gint64` + `_Static_assert` sur tailles             |
| 3 | qt6-compat.h             | `g_debug`/`g_info` conditionnels `-DDEBUG`                 |
| 4 | qt6-compat.h             | `g_strdup` → `_qt6_strdup()` `warn_unused_result`          |
| 5 | widget_terminal.cpp      | `pclose(fp)` → `fclose(fp)` (UB fdopen/pclose)             |
| 6 | widget_table.cpp         | `pclose(fp)` → `fclose(fp)` (UB fdopen/pclose)             |
| 7 | widget_menubar.cpp       | Bounds check sur dépilage stack (MENUBAR_MAX_CHILDREN=64)  |
| 8 | CMakeLists.txt           | `CMAKE_EXPORT_COMPILE_COMMANDS ON`                          |
| 9 | CMakeLists.txt           | cppcheck target ajouté                                      |

## 🟡 Points restants (~7 pts)

- `g_signal_connect` est un no-op : les signaux GTK du core ne déclenchent
  rien côté Qt — acceptable pour un port XML-driven mais à documenter
- `g_slist_last` / `g_list_last` utilisent des statement-expressions GCC
  (portables GCC/Clang, non standard ISO C)
- Terminal stub (sans QTermWidget6) : sortie figée, pas de scroll live
- Valgrind/ASan complet non lancé (les tests CTest `test_safe_exec`/`test_compat`
  existent, ainsi que les suites XML, cœur et comportement)

## Historique

| Date       | Score | Événement                              |
|------------|-------|----------------------------------------|
| 2025-01    | 78    | Audit initial                          |
| 2026-05    | 90    | Correctifs sécurité + build modernisé |
| 2026-06-06 | 90    | Audit #5 : build rc=0, tests XML 52/52 (levelbar/drawingarea câblés) |
| 2026-06-09 | 90    | Audit indépendant externe : popen() brut corrigé, score réaligné |
| 2026-06-10 | 90    | Tests de comportement par widget : 12/12 après correctifs timer + hscale |

## ✅ Widgets ajoutés (v3 — +9 widgets)

| # | Widget              | Implémentation Qt6                              |
|---|---------------------|-------------------------------------------------|
| 1 | widget_switch       | QCheckBox Catppuccin-styled, export true/false  |
| 2 | widget_password     | QLineEdit Password echo mode                    |
| 3 | widget_searchentry  | QLineEdit + find icon + clearButton             |
| 4 | widget_calendar     | QCalendarWidget, export YYYY-MM-DD              |
| 5 | widget_infobar      | QFrame border coloré info/warning/error         |
| 6 | widget_levelbar     | QProgressBar 0-1000, export 0.000-1.000         |
| 7 | widget_spinner      | QLabel + QTimer 100ms frames Braille            |
| 8 | widget_aspectframe  | QFrame + QVBoxLayout                            |
| 9 | widget_drawingarea  | DrawingArea QWidget + paintEvent                |

**Total widgets qt6sermo : 42/42 à cette étape** — dispatcher widgets.cpp câblé
(49 aujourd'hui après les ajouts ultérieurs).

---


---

## Mise à jour v1.0.0 (2026-05-29)

### Tests XML communs (suite partagée)
- Suite étendue de 20 à **50 cas** — tous les widgets couverts
- Compatible tous les ports via `run_tests.sh qt6sermo`

## Mise à jour — Audit #3 (2026-05-30)

Revue documentaire et packaging (aucun changement de code source).

- MANUEL_DEVELOPPEUR, MANUEL_UTILISATEUR et COMPILE créés pour ce port.
- `debian/control` : Build-Depends `pkg-config, g++` dédupliqués.
- `rpm/qt6sermo.spec` : `cmake >= 3.20 >= 3.16` corrigé en `cmake >= 3.20`.
- `arch/.SRCINFO` régénéré proprement.
- **Documentation : 92 → 96 ; Packaging : 95 → 98. Score global 90 (réaligné).**


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



---

## Audit #4 — Harmonisation et alignement des ports (2026-06-01)

Session transversale appliquant à ce port les correctifs éprouvés sur gtk4d,
dans le cadre de l'uniformisation de toute la famille haplo-dialog.

| Domaine | Action |
|---------|--------|
| Test `test_compat.cpp` | **Blocage levé.** Cause racine : `_Static_assert(...)` (C11) illégal en C++ dans `src/qt6-compat.h`, qui interrompait l'analyse de l'en-tête (typedef `gsize` non vu). Désormais via la macro portable `QT6_STATIC_ASSERT` (→ `static_assert` en C++). |
| Sécurité | Attribut `nonnull` retiré de `safe_exec.h` — `-Wnonnull-compare` supprimé. |
| Préprocesseur | `_GNU_SOURCE` et macros `g_*` de `qt6-compat.h` protégés par `#ifndef`. |
| Flags d'avertissement | `CMakeLists.txt` aligné sur gtk4d ; `-Wno-missing-prototypes` restreint au C via `$<$<COMPILE_LANGUAGE:C>:…>` (évite le warning cc1plus sur les cibles C++). |
| Licence | Harmonisée de GPL-3.0+ vers **GPL-2.0-or-later** (sources, site de référence, recettes de packaging). |

**Vérification (réelle, ce jour) :** `ctest` → **2/2 tests passés** (dont `compat`,
auparavant bloquant) ; plus aucun `-Wnonnull-compare` ni warning cc1plus. Le binaire
principal se construit sur la cible (en environnement de vérification, blocage limité
aux en-têtes GTK/GLib hors chemin d'inclusion CMake — point d'environnement).

Score : **90/100 (réaligné)**.

---

## Audit #5 — Vérification finale (2026-06-06)

Vérification de bout en bout sur Debian (gcc 15.2, Qt6 6.10.2). Le blocage
résiduel de l'audit #4 (limité aux en-têtes hors chemin d'inclusion CMake dans
l'environnement de vérification) est levé : le **binaire principal se construit
sur la cible**.

| Métrique | Valeur |
|----------|-------:|
| Build (`cmake --build`) | **rc=0, 0 erreur** ✅ |
| Binaire `qt6sermo` | 240 Ko, exécutable (`--version` rc=0) |
| Tests XML `tests/xml/` (52 cas, `--print-ir`, headless) | **52/52 PASS** ✅ (était 50/52) |
| Warnings | **43** (idiomes amont gtkdialog, non bloquants) |

### Grammaire — levelbar et drawingarea câblés

Les deux derniers cas XML qui échouaient (50/52) portaient sur `<levelbar>` et
`<drawingarea>` : leurs widgets existaient (`widget_levelbar.cpp`,
`widget_drawingarea.cpp`) mais n'étaient pas câblés dans la chaîne d'analyse.
**Câblage complet** ajouté (enum `automaton.h` + grammaire + dispatch création)
→ les 52 cas passent. Le compteur restait à **42/42 widgets** à cette date
(aucun nouveau fichier ; 49 aujourd'hui après les ajouts ultérieurs).

### Bugs réels corrigés (communs à la famille)

- **Troncature des noms de widgets auto-générés** : `g_snprintf(name, sizeof(name), …)`
  où `name` est un `char*` (limité à 8 octets) → corrigé en `64`. Restaure le
  comportement gtkdialog d'origine.
- **`if (var->Name == NULL)`** (toujours faux + risque de NULL-deref) →
  `if (var == NULL || var->Name[0] == '\0')`.
- **`instruction inst;` lu non initialisé** (`inst.ival`) → `instruction inst = {0};`.
- **Fonctions sans `return`** (`get_program_from_*`, `_tree_insert`, `_tree_find`) →
  `return` restauré.
- Condition morte `if (!s.widgets || …)` nettoyée (menubar).

---

## Audit indépendant externe — 2026-06-09

Audit externe (build + 52/52 + checksec + revue de code). Score réaligné à 90/100 —
le « 100/100 » antérieur masquait un vrai défaut. Durcissement (Full RELRO/PIE/
Canary/Fortify) et 52/52 tests : confirmés RÉELS. Défaut trouvé ET corrigé :
`widget_text.cpp` faisait un `popen()` BRUT contournant `safe_exec` (réinjection
shell via /bin/sh -c + bug zombie : `fclose()` sur un flux `popen()`) → remplacé
par `widget_opencommand()` (seul widget qui contournait le wrapper sûr). Dette
restante : warnings sur le C hérité, valgrind/ASan complet non lancé.

---

## Session 2026-06-10 — tests de comportement par widget

Nouvelle suite partagée `tests/behaviour/` (12 cas) : contrairement à
`tests/xml/` (parse seul), chaque cas **ouvre une vraie fenêtre**, un `<timer>`
embarqué déclenche `EXIT:ok` après ~1 s, et les exports (`VAR="valeur"`) imprimés
sur stdout sont vérifiés. qt6sermo passe **12/12 — après deux correctifs réels**
révélés par la suite :

| Fichier | Défaut constaté | Correctif |
|---------|-----------------|-----------|
| `widget_timer.cpp` | Les `<action>` du timer étaient passées directement à `safe_system()` : « EXIT:ok » était traité comme une **commande shell** — un dialogue ne pouvait pas être fermé par script via le timer. | Actions désormais dispatchées via `execute_action()` (itération complète de `ATTR_ACTION`, tagattr `function`/`type` honorés), comme `widget_button`. Les préfixes `EXIT:`/`Command:`/`REFRESH:`…, `type="exit"` et les **actions multiples** sont respectés. |
| `widget_hscale.cpp` | L'attribut `range-value` n'était pas lu (seul `value`) → slider initialisé à 0. | Lecture ajoutée : priorité `range-value`, repli `value`. |

Bilan tests du port : **XML 52/52 · cœur 9/9 · comportement widgets 12/12**
(famille : 312/312 + 54/54 + 72/72). CI réorganisée en étages
**docs / build-test (sous `xvfb-run`) / qa** ; intégration bureau
(`data/qt6sermo.desktop` + `data/qt6sermo.metainfo.xml`) et chaîne APT
(`tools/make-apt-repo.sh` à la racine — publication apt en préparation).

Score inchangé : **90/100**.

---

*Document horodaté — dernière mise à jour : 2026-06-10 (audit #6 — haplo-dialog v1.0.0).*