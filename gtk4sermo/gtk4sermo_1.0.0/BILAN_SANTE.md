# Bilan de Santé — gtk4sermo (gtk4sermo)

## État vérifié — 2026-06-07 — Score : **91/100**

> ⚠️ **Ce que valent les chiffres de ce document.** Les notes sur 100 sont une
> **auto-évaluation** interne, posée à la main au fil du développement. Elles ne
> sortent d'aucun outil, ne sont rejouables par personne, et ce fichier en donne
> plusieurs qui se contredisent. Ne leur accordez pas plus de poids qu'à un avis.
>
> Ce qui est vérifiable, en revanche, l'est par une commande :
>
> | Garantie | Comment la vérifier |
> |---|---|
> | Durcissement du binaire (PIE, RELRO, BIND_NOW, NX, CET IBT/SHSTK, stack protector, FORTIFY) | `tests/garde_durcissement.sh src/<port>` |
> | Le thread de la barre de progression ne touche pas GTK | `tests/garde_progressbar_thread.sh src/widget_progressbar.c` |
> | Borne MAXWIDGETS sur la recopie des widgets | `tests/garde_maxwidgets.sh src/<port>` |
> | `--do` annoncée, exécutée, ordonnée, fail-closed | `tests/garde_option_do.sh src/<port>` |
> | `HAPLO_ALLOWED_CMDS` mordante et incontournable | `tests/garde_allowed_cmds.sh src/<port>` |
> | En-tête SPDX sur chaque source | `tests/garde_spdx.sh src` |
> | Exemples qui s'ouvrent vraiment | `tests/run_examples.sh src/<port>` |
> | Corpus XML | `tests/xml/run_tests.sh <port>` |
>
> Ces huit bancs tournent à chaque passage de l'intégration continue.

| Critère | État |
|---|:--:|
| Build + install | ✅ rc=0 |
| Tests XML (55 cas) | ✅ 55/55 |
| Tests de comportement (`run_unit_tests.sh`) | ✅ 9/9 |
| Warnings dangereux (casts/uninit) | ✅ 0 |
| Paquet `.deb` (lintian) | ✅ 0 tag |
| CI (`.gitlab-ci.yml`) | ✅ build + tests |
| Widgets | 50 |

*Le plus complet ; architecture modulaire (un widget par fichier) — ce bloc reflète l'état vérifié et fait foi ; les sections « patch … » ci-dessous sont l'historique antérieur (scores d'époque).*

**gtk4sermo 1.0.0 — Haplo-Linux (Debian Testing)**
**Mainteneur :** haplo-dialog <devel@haplo-dialog.fr>
**Date :** 2026-06-01 *(audit #4 — compilation complète)*
**Toolkit :** GTK4 ≥ 4.6 (GNOME/Freedesktop)

---

## ✅ Audit #4 (2026-06-01) — Compilation complète

Le port GTK4 **compile et lie sans erreur**. Jalon technique majeur atteint :

| Métrique | Avant | Après |
|----------|------:|------:|
| Erreurs de compilation | bloquantes | **0** ✅ |
| Édition de liens | échec | **OK** (gtk4sermo ~1,45 Mo) |
| Warnings | 1549 | **128** (~92 % de réduction) |

Correctifs sources appliqués :

- **gtk4-compat.h** : portage complet des API GTK2/3 retirées en GTK4
  (radio→check groupés, gtk_init, scrolled_window_new, GtkIconSize,
  image_*_from_icon_name, label_set_wrap, editable_set_width_chars,
  calendar_select_*, window_set_position/icon, GTK_WIN_POS_*,
  FILE_CHOOSER_ACTION_*, vte_terminal_get_padding…)
- **variables.c / gtkdialog.c** : valeurs de retour manquantes ajoutées
- **gtkdialog.h/.c** : `extern` + définition unique (fin des définitions multiples)
- **widget_combobox/comboboxtext.c** : portés sur `GtkComboBoxText`
- **widget_colorbutton.c / widget_terminal.c** : `gdk_rgba_parse` (ordre args)
- **glade_support.c** : `GSList`, GtkBuilder GTK4, `populate-popup` retiré
- **widget_tree.c** : formats `%lli/%llu/%i` corrigés (gint64/guint64/GType)
- **automaton.c** : `-Waddress` + `format-extra-args` corrigés
- **safe_exec.h** : attribut `nonnull` retiré (vérification runtime conservée)
- **variables `*string`** initialisées (suppression des `-Wuninitialized`)

Les 128 warnings résiduels sont des idiomes hérités de l'amont
(assignation en condition, casts entier↔pointeur) — **non bloquants**.

---

## Score global

```
┌─────────────────────────────────────────────────────────────────────────┐
│  gtk4sermo 1.0.0 — Score de santé                                   │
├──────────────────────────────────────┬──────────────────────────────────┤
│  Sécurité                            ██████████████████████  92/100  ✅ │
│  Migrations GTK4 (compat.h v2)       ████████████████████░░  97/100  ✅ │
│  Qualité code                        ████████████████████░░  95/100  ✅ │
│  Build system                        ███████████████████░░░  95/100  ✅ │
│  Documentation                       ████████████████████░░  95/100  ✅ │
│  Maintenabilité                      ████████████████████░░ 100/100  ✅ │
├──────────────────────────────────────┼──────────────────────────────────┤
│  TOTAL                               ████████████████████░░  97/100  ✅ │
└──────────────────────────────────────┴──────────────────────────────────┘
```

> **Référence :** gtk3d = 100/100 (production)
> Score précédent : 98/100 (stubs menubar/table/override_*)
> Cible release 1.0.0 : >= 95/100 — **ATTEINT** ✅

---

## Statistiques

| Métrique | Valeur |
|----------|--------|
| Build (`make`) | rc=0, 0 erreur ✅ |
| Tests XML `tests/xml/` (55 cas, `--print-ir`) | 55/55 ✅ (était 0/52 avant l'audit #5) |
| Warnings | 77 (idiomes amont, non bloquants) |
| Widgets implémentés | 50/50 ✅ |
| GTK4_TODO résiduels (bloquants) | 0 ✅ |
| GTK4_TODO non-bloquants | 3 (gvim socket, filechooser async, draw signal) |
| Fichiers sources | 65 |
| Dépendances runtime | gtk4, vte-2.91-gtk4 (optionnel) |
| Licence | GPL-2.0-or-later |
| Binaire | `gtk4sermo` |

---

## Corrections v2 (2026-05-29)

### 1. gtk_widget_override_* / modify_* → vraies injections CSS

**Avant :** 9 macros no-op silencieuses — les couleurs et polices personnalisées
étaient ignorées.

**Après :** vraies injections CSS via `GtkCssProvider` + `gtk_style_context_add_provider()` :
- `_compat_override_bg()` → `background-color: rgba(...)`
- `_compat_override_color()` → `color: rgba(...)`
- `_compat_override_font()` → `font-family: ...; font-size: ...pt;`
- Helper `_compat_rgba_to_css()` et `_compat_pango_to_css_font()`
- Compatible GTK 4.12+ (`gtk_css_provider_load_from_string`) et antérieur

### 2. gtk_dialog_run → émulation synchrone async-safe

**Avant :** macro retournant toujours `GTK_RESPONSE_ACCEPT` sans afficher
le dialogue.

**Après :** `_compat_gtk_dialog_run()` — émulation complète :
- Crée un `GMainLoop` local (le loop GTK principal continue de tourner)
- Connecte le signal `"response"` du dialogue
- Bloque jusqu'à la réponse de l'utilisateur via `g_main_loop_run()`
- Déconnecte le signal + libère le loop proprement
- Utilisé dans `actions.c` (file chooser) et `printing.c` (dialog erreur)

### 3. widget_menubar.c → GtkPopoverMenuBar + GMenuModel

**Avant :** `GtkLabel` stub `"[menubar — not available in GTK4]"`.

**Après :** implémentation complète via l'API officielle GTK4 :
- `GMenu` racine avec `g_menu_append_submenu()`
- `GSimpleAction` par item, callbacks via `safe_system()`
- Noms d'actions sanitisés (caractères invalides → `_`)
- `GtkPopoverMenuBar` créée depuis le `GMenuModel`
- `GSimpleActionGroup` enregistré sur la fenêtre parente via
  `gtk_widget_insert_action_group(window, "menubar", ag)`

### 4. widget_menuitem.c → GMenuItem + GSimpleAction

**Avant :** `GtkLabel` stub `"[menuitem — not available in GTK4]"`.

**Après :** `GtkPopoverMenu` créé depuis un `GMenuModel` :
- Items `"Label | commande"` parsés dynamiquement
- `GSimpleActionGroup` attaché à la fenêtre via realize

### 5. widget_table.c → GtkColumnView + GListStore

**Avant :** `GtkLabel` stub `"[table -- not available in GTK4 (GtkCList removed)]"`.

**Après :** implémentation complète avec les API GTK4 natives :
- `RowObject` — GObject wrappant les cellules d'une ligne
- `GListStore<RowObject>` — modèle de données
- `GtkSingleSelection` — sélection simple
- `GtkColumnView` avec `GtkColumnViewColumn` par colonne
- `GtkSignalListItemFactory` — rendu des cellules via `GtkLabel`
- Colonnes redimensionnables, séparateurs lignes/colonnes
- TSV (tab-separated), première ligne = en-têtes
- `TABLE_ROWS_MAX` (2048) + `TABLE_LINE_MAX` (4096) bornés
- `fclose()` sur le pipe de commande

### 6. GTK4_NO_MENU + GTK4_NO_CLIST supprimés

Les guards `#define GTK4_NO_MENU 1` et `#define GTK4_NO_CLIST 1` dans
`gtk4-compat.h` activaient les blocs stub. Supprimés — les vrais widgets
sont maintenant compilés sans conditions.

---

## GTK4_TODO résiduels (non-bloquants)

| TODO | Statut | Impact |
|------|--------|--------|
| `<gvim>` / GtkSocket | Stub label X11-only | ⚠ Wayland : fonctionnel impossible |
| `<filechooser>` async | GtkFileChooserDialog déprécié GTK4.10+ | ⚠ Warning compilateur |
| Signal `"draw"` → `set_draw_func` | `widget_drawingarea.c` | ℹ Non-bloquant |

---

## Tableau des widgets (50/50 ✅)

| Widget XML | API GTK4 | Statut |
|------------|----------|--------|
| `<window>` | `gtk_window_new()` | ✅ |
| `<button>` | `GtkButton` | ✅ |
| `<checkbox>` | `GtkCheckButton` | ✅ |
| `<radiobutton>` | `GtkCheckButton` groupe | ✅ |
| `<entry>` | `GtkEntry` + editable | ✅ |
| `<password>` | `GtkPasswordEntry` | ✅ |
| `<edit>` | `GtkTextView` | ✅ |
| `<text>` | `GtkLabel` | ✅ |
| `<hbox>` / `<vbox>` | `GtkBox` | ✅ |
| `<frame>` | `GtkFrame` | ✅ |
| `<notebook>` | `GtkNotebook` | ✅ |
| `<expander>` | `GtkExpander` | ✅ |
| `<list>` | `GtkListBox` | ✅ |
| `<combobox>` | `GtkComboBoxText` | ✅ |
| `<progressbar>` | `GtkProgressBar` | ✅ |
| `<hscale>` / `<vscale>` | `GtkScale` | ✅ |
| `<spinbutton>` | `GtkSpinButton` | ✅ |
| `<statusbar>` | `GtkStatusbar` | ✅ |
| `<timer>` | `g_timeout_add` | ✅ |
| `<togglebutton>` | `GtkToggleButton` | ✅ |
| `<switch>` | `GtkSwitch` | ✅ |
| `<separator>` | `GtkSeparator` | ✅ |
| `<pixmap>` / `<image>` | `GtkPicture` | ✅ |
| `<colorbutton>` | `GtkColorButton` | ✅ |
| `<fontbutton>` | `GtkFontButton` | ✅ |
| `<tree>` | `GtkTreeView` + `GtkTreeStore` | ✅ |
| `<table>` | `GtkColumnView` + `GListStore` | ✅ |
| `<menubar>` | `GtkPopoverMenuBar` + `GMenuModel` | ✅ |
| `<menuitem>` | `GtkPopoverMenu` + `GMenuItem` | ✅ |
| `<terminal>` | VTE `vte-2.91-gtk4` | ✅ |
| `<calendar>` | `GtkCalendar` | ✅ |
| `<filechooser>` | `GtkFileChooserDialog` | ✅ |
| `<infobar>` | `GtkBox` + CSS | ✅ |
| `<levelbar>` | `GtkLevelBar` | ✅ |
| `<revealer>` | `GtkRevealer` | ✅ |
| `<stack>` | `GtkStack` | ✅ |
| `<overlay>` | `GtkOverlay` | ✅ |
| `<flowbox>` | `GtkFlowBox` | ✅ |
| `<drawingarea>` | `GtkDrawingArea` | ✅ |
| `<aspectframe>` | `GtkAspectFrame` | ✅ |
| `<searchentry>` | `GtkSearchEntry` | ✅ |
| `<spinner>` | `GtkSpinner` | ✅ |
| `<linkbutton>` | `GtkLinkButton` | ✅ |
| `<pulse>` | `GtkProgressBar` pulse | ✅ |
| `<eventbox>` | `GtkBox` shim | ✅ |
| `<vseparator>` | `GtkSeparator` vertical | ✅ |
| `<hseparator>` | `GtkSeparator` horizontal | ✅ |

---

## Sécurité

| Mécanisme | Statut |
|-----------|--------|
| `safe_exec()` (exec direct hors métacaractères, repli shell journalisé) | ✅ |
| Liste de commandes `HAPLO_ALLOWED_CMDS` — **facultative, éteinte par défaut** | ✅ |
| `FORTIFY_SOURCE=3` | ✅ |
| PIE + Full RELRO | ✅ |
| Stack canary + clash | ✅ |
| CF protection | ✅ |
| NX stack | ✅ |
| CSS injection sécurisée | ✅ v2 |
| Dialog run sans blocage event loop | ✅ v2 |

---

## Historique des scores

| Date | Score | Événement |
|------|-------|-----------|
| 2026-05-26 | 60/100 | Infrastructure GTK4, 20 widgets |
| 2026-05-27 | 98/100 | 50 widgets, gtk4-compat.h, stubs menu/table |
| 2026-05-29 | **97/100** | v2 : CSS réel, dialog_run async, menubar GMenuModel, table ColumnView |
| 2026-05-30 | **98/100** | audit #3 : identité doc corrigée, packaging (VTE gtk4, licence) |
| 2026-06-01 | **99/100** | audit #4 : compilation complète (0 erreur), warnings 1549 → 128, port API GTK2/3 finalisé |
| 2026-06-06 | **99/100** | audit #5 : grammaire réparée → tests XML **0/52 → 55/55** ; build rc=0 ; warnings 128 → 77 |

---

## ✅ Audit #5 (2026-06-06) — Réparation de la grammaire : tests XML 0/52 → 55/55

Jusqu'à cette session, le port GTK4 **compilait** (audit #4) mais **échouait à
TOUS les tests XML** : **0/52**. Cause racine — une grammaire incomplète :

- les **commentaires `<!-- -->`** n'étaient pas gérés (le `>` de `-->` fuitait comme token) ;
- les **espaces / sauts de ligne entre attributs** d'un même tag déclenchaient une erreur de syntaxe ;
- **8 widgets** disposaient d'un fichier `widget_*.c` mais n'étaient **pas câblés dans la grammaire**,
  donc inutilisables : **switch, password, calendar, infobar, spinner, searchentry, levelbar, drawingarea**.

Correctifs appliqués cette session :

- **Lexer** : nouvel état `ST_COMMENT` (entrée sur `<!--`, sortie sur `-->`, contenu ignoré,
  `\n` incrémente `linenumber`) ; nouvel état `ST_TAG_ATTR` consommant les espaces / tabs /
  sauts de ligne entre attributs.
- **`--print-ir`** : `gtk_init_check()` utilisé en mode `--print-ir` — l'analyse ne nécessite
  plus de serveur X (exécution headless possible).
- **Câblage des 8 widgets** dans la grammaire (lexer + parser + dispatch création).

| Métrique | Avant (audit #4) | Après (audit #5) |
|----------|-----------------:|-----------------:|
| Build (`make`) | rc=0 | **rc=0, 0 erreur** ✅ |
| Tests XML `tests/xml/` (55 cas, `--print-ir`, headless) | **0/52** ❌ | **55/55 PASS** ✅ |
| Warnings | 128 | **77** (idiomes amont, non bloquants) |

Les 8 widgets concernés (switch, password, calendar, infobar, spinner, searchentry,
levelbar, drawingarea) sont **désormais reconnus par la grammaire** et donc utilisables ;
aucun nouveau fichier widget n'a été créé (le compteur de 50 widgets est inchangé).

### Bugs réels corrigés (communs à la famille)

- **Troncature des noms de widgets auto-générés** : `g_snprintf(name, sizeof(name), …)` où
  `name` est un `char*` (limité à 8 octets) → corrigé en `64`. Restaure le comportement gtkdialog d'origine.
- **`instruction inst;` lu non initialisé** (`inst.ival`) → `instruction inst = {0};`.

---

*haplo-dialog — devel@haplo-dialog.fr — 2026*

---


---

## Mise à jour v1.0.0 (2026-05-29)

### Tests XML communs (suite partagée)
- Suite étendue de 20 à **50 cas** — tous les widgets couverts
- Compatible tous les ports via `run_tests.sh gtk4sermo`

## Mise à jour — Audit #3 (2026-05-30)

Revue documentaire et packaging (aucun changement de code source).

- Identité corrigée : MANUEL_DEVELOPPEUR, HTML de référence (37) et site (titres `gtk4sermo:`, URLs Haplo-Linux).
- Packaging : `debian/control` runtime VTE → `libvte-2.91-gtk4-0` ; `rpm` → `vte291-gtk4`.
- `arch/.SRCINFO` régénéré (pkgbase aligné sur `gtk4sermo`), `provides=gtk4sermo`.
- Licence : l'essai GPL-3.0+ de l'audit #3 a été **annulé** ; le port reste en **GPL-2.0-or-later**, cohérent avec l'en-tête source et l'amont gtkdialog.
- **Documentation : 95 → 98 ; Packaging : 95 → 98. Score global 97 → 98.**


## Remerciements

> *Ce travail a été réalisé avec l'assistance de **Claude**,
>le modèle de langage d'[Anthropic](https://www.anthropic.com),
>dans le cadre du développement de la famille **haplo-dialog** pour Haplo-Linux.*

---

*Document horodaté — dernière mise à jour : 2026-06-07 (audit #5 — grammaire réparée, tests XML 0/52 → 55/55).*
