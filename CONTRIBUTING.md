# Guide de contribution - haplo-dialog

Merci de votre intérêt pour haplo-dialog. Ce document explique comment contribuer efficacement au projet.

---

## Table des matières

1. [Avant de commencer](#1-avant-de-commencer)
2. [Structure du projet](#2-structure-du-projet)
3. [Configurer l'environnement](#3-configurer-lenvironnement)
4. [Workflow de contribution](#4-workflow-de-contribution)
5. [Standards de code](#5-standards-de-code)
6. [Ajouter un widget](#6-ajouter-un-widget)
7. [Tests](#7-tests)
8. [Documentation](#8-documentation)
9. [Sécurité](#9-sécurité)
10. [Revue de code](#10-revue-de-code)

---

## 1. Avant de commencer

- Lire [SECURITY.md](SECURITY.md) **en entier** avant toute modification du core
- Respecter le [Code de Conduite](CODE_OF_CONDUCT.md)
- Pour les bugs de sécurité : écrire à `devel@haplo-dialog.fr`, **ne pas ouvrir un ticket public**
- Pour les bugs fonctionnels : ouvrir un ticket avec un script XML reproductible minimal

**Port de référence :**  
haplo-dialog fournit un seul port, `gtk3dialog` (backend GTK 3), qui fournit l'alias rétro-compatible `gtkdialog`. C'est un descendant maintenu de gtkdialog (fork de gtkdialog 0.8.3 de Laszlo Pere), corrigé et durci. D'autres descendants existent, notamment le fork BunsenLabs de Mick Amadio, qui porte lui aussi gtkdialog sur GTK 3.

---

## 2. Structure du projet

```
haplo-dialog/
├── gtk3dialog/
│   └── gtk3dialog_1.0.0/
│       ├── src/
│       │   ├── gtk3dialog.c      ← point d'entrée principal
│       │   ├── safe_exec.c       ← GPL-2.0-or-later
│       │   ├── variables.c       ← GPL-2.0-or-later
│       │   ├── stringman.c       ← GPL-2.0-or-later
│       │   └── widget_*.c        ← un fichier par widget (GPL-2.0-or-later)
│       ├── tests/
│       ├── examples/             ← scripts #!/bin/sh par widget (CC0)
│       ├── doc/
│       └── packaging/
└── tests/
    ├── xml/                      ← régression XML (52 cas)
    ├── unit/test_safe_exec.c     ← tests de comportement (safe_exec)
    └── fuzz/                     ← corpus de fuzzing
```

---

## 3. Configurer l'environnement

### Dépendances de build

```sh
# Debian / Ubuntu
sudo apt install build-essential flex bison libgtk-3-dev pkg-config

# Arch
sudo pacman -S base-devel flex bison gtk3

# Fedora
sudo dnf install gcc flex bison gtk3-devel
```

### Build de développement

```sh
cd gtk3dialog/gtk3dialog_1.0.0
autoreconf -fi
./configure --prefix=/usr/local CFLAGS="-g -O0 -fsanitize=address"
make -j$(nproc)
make check        # lance les tests
```

### Outils recommandés

```sh
sudo apt install clang-format cppcheck valgrind
# Vérifier le formatage avant commit :
clang-format --dry-run --Werror src/widget_button.c
```

---

## 4. Workflow de contribution

```
1. Fork / clone du dépôt
2. Créer une branche : git checkout -b fix/widget-entry-null-crash
3. Modifier le code, un seul sujet par branche
4. Tester : make check
5. Vérifier le format : clang-format -i src/*.c
6. Commit : git commit -m "fix(widget_entry): handle NULL label gracefully"
7. Soumettre un patch par email à devel@haplo-dialog.fr
   ou ouvrir une Merge Request si un dépôt Git public est disponible
```

### Format des messages de commit

```
type(scope): description courte en anglais (< 72 chars)

Corps optionnel : explication du pourquoi, pas du comment.
Référence : Fixes #123
```

Types valides : `fix`, `feat`, `docs`, `test`, `refactor`, `security`, `build`, `ci`

Scopes valides : `core`, `widget_button`, `gtk3dialog`, `packaging`, `doc`

---

## 5. Standards de code

### C (core et widgets GTK 3)

- Standard : **C11** (`-std=c11`)
- Formatage : `.clang-format` fourni à la racine, **obligatoire**
- Pas de `system()`, `popen()`, `strcpy()`, `sprintf()` directs, utiliser `safe_system()`, `safe_popen()`, `g_strlcpy()`, `snprintf()`
- Tout `FILE*` issu de `safe_popen()` → `fclose()`, **jamais `pclose()`**
- Vérifier le retour de `malloc()` / `calloc()`, `NULL` = fatal log + return
- Pas d'allocation dynamique non bornée dans les widgets

```c
/* ✅ Correct */
char *buf = calloc(count, size);
if (!buf) { fprintf(stderr, "OOM\n"); return NULL; }

/* ❌ Interdit */
system("ls");
char *p = malloc(strlen(user_input) + 1); /* non borné */
```

### Shell (exemples)

- Shebang : `#!/bin/sh` (POSIX, pas bash-spécifique sauf si nécessaire)
- `set -e` recommandé dans les scripts complexes
- Pas de `eval` sur des variables utilisateur

---

## 6. Ajouter un widget

Un widget se branche en **plusieurs points** : l'implémentation, la grammaire
(lexer + parser) et les aiguillages (`automaton.c`, `widgets.c`). Prendre un
widget existant comme modèle, ici `<switch>` (`src/widget_switch.c/.h`).

**1. Implémenter `src/widget_monwidget.c` + `src/widget_monwidget.h`**

Le prototype réel suit celui des widgets existants (cf. `src/widget_switch.h`) :

```c
/* widget_monwidget.h */
GtkWidget *widget_monwidget_create(AttributeSet *Attr, tag_attr *attr, gint Type);
gchar     *widget_monwidget_envvar_construct(GtkWidget *widget);
```

```c
/* widget_monwidget.c */
#include <gtk/gtk.h>
#include "gtk3d.h"
#include "widget_monwidget.h"

GtkWidget *widget_monwidget_create(AttributeSet *Attr, tag_attr *attr, gint Type)
{
    GtkWidget *widget = /* … créer le GtkWidget natif … */;
    return widget;
}

/* Exporte l'état du widget au format historique VAR="valeur" */
gchar *widget_monwidget_envvar_construct(GtkWidget *widget)
{
    /* … lire la valeur et renvoyer la chaîne … */
}
```

**2. Déclarer la constante de type dans `src/automaton.h`**

```c
#define WIDGET_MONWIDGET  0x00C00000   /* prochaine valeur libre de la séquence */
```

**3. Reconnaître les balises dans le lexer `src/gtkdialog_lexer.l`**

Sur le modèle de `<switch>`, renvoyer les tokens ouvrant / avec-attributs / fermant :

```
\<monwidget\>    { Token="<monwidget>"; return(MONWIDGET); }
\<monwidget[ ]+  { Token="<monwidget>"; BEGIN(ST_TAG_ATTR); return(PART_MONWIDGET); }
\<\/monwidget\>  { Token="</monwidget>"; return(EMONWIDGET); }
```

**4. Déclarer les tokens et les règles dans `src/gtkdialog_parser.y`**

```
%token MONWIDGET PART_MONWIDGET EMONWIDGET
```

puis ajouter les productions qui empilent le type, comme pour `SWITCH`, qui
appelle `token_store(PUSH | WIDGET_SWITCH)` (et `token_store_attr(...)` pour la
forme avec attributs).

**5. Instancier dans `src/automaton.c`**

C'est ici, et non dans `widgets.c`, que le widget est créé (cf. le
`case WIDGET_SWITCH`) :

```c
case WIDGET_MONWIDGET:
    Widget = widget_monwidget_create(Attr, tag_attributes, Widget_Type);
    push_widget(Widget, Widget_Type);
    break;
```

Ajouter aussi le `case WIDGET_MONWIDGET:` dans la représentation `--print-ir`
du même fichier.

**6. Aiguiller la sortie dans `src/widgets.c`**

Dans `widget_get_text_value()` (export de la valeur) et `widgets_to_str()`
(nom lisible) :

```c
case WIDGET_MONWIDGET:
    string = widget_monwidget_envvar_construct(widget);
    return string;
/* … et, dans widgets_to_str() … */
case WIDGET_MONWIDGET:
    type = "MONWIDGET";
    break;
```

**7. Ajouter les sources dans `src/Makefile.am`**

```
    widget_monwidget.c widget_monwidget.h \
```

**8. Fournir un exemple et un test**

- un script d'exemple sous `examples/monwidget/` (comme les exemples existants :
  `#!/bin/sh`, nommé d'après le widget) ;
- un cas de régression XML dans `tests/xml/`.

---

## 7. Tests

### Tests unitaires (sans toolkit)

```sh
cd gtk3dialog/gtk3dialog_1.0.0
autoreconf -fi && ./configure && make check
```

Les tests unitaires ne doivent **pas** dépendre de GTK 3, uniquement du core C pur (`safe_exec`, `variables`, `stringman`).

### Tests de régression XML

```sh
cd tests/xml
./run_tests.sh gtk3dialog    # teste les 52 XML de référence (mode --print-ir, headless)
```

### Tests de comportement (unitaires)

Au-delà du parse, on **exécute** réellement la logique cœur (sécurité
`safe_exec` : `safe_system`/`safe_popen`). « `main()` pur » : ni libcheck ni
serveur X requis, donc runnable en CI. 9 tests de comportement.

```sh
./tests/run_unit_tests.sh gtk3dialog
```

### Valgrind (mémoire)

```sh
valgrind --leak-check=full --error-exitcode=1 \
  gtk3dialog --file tests/xml/01_button.xml --do EXIT:exit
```

### Intégration continue (CI)

Le dépôt fournit `.gitlab-ci.yml` (GitLab CI, image `debian:testing`, cible : Debian testing). À chaque push / merge request, le job :

1. installe les dépendances de build ;
2. compile (autotools) ;
3. valide le binaire contre les 52 cas XML (`tests/xml/run_tests.sh`, `--print-ir`,
   sans serveur X).

Un build cassé ou un seul test en échec fait échouer le pipeline. Avant de pousser,
on peut reproduire le job en local :

```sh
cd gtk3dialog/gtk3dialog_1.0.0
autoreconf -fi && ./configure --prefix="$PWD/inst" && make -j"$(nproc)" && make install
PATH="$PWD/inst/bin:$PATH" sh ../../tests/xml/run_tests.sh gtk3dialog
```

---

## 8. Documentation

- Les manpages sont en format **roff** (`src/gtk3dialog.1`), à jour avec chaque nouveau widget
- La référence XML est dans `haplo-dialog-xml(5)`, `man/haplo-dialog-xml.5`
- La documentation Texinfo est dans `doc/`
- Les exemples (`examples/`) sont en **CC0**, les utiliser sans restriction

---

## 9. Sécurité

**Règles absolues, tout patch qui les viole sera rejeté :**

1. Jamais `system()` / `popen()` direct, toujours `safe_system()` / `safe_popen()`
2. Les commandes `<action>`/`<input>` passent par `safe_exec` : exécution directe sans shell quand c'est possible ; repli `/bin/sh -c` **journalisé** et désactivable (`HAPLO_NO_SHELL_FALLBACK`)
3. Bornes systématiques sur les tampons, `g_strlcpy`/`g_strlcat`, jamais `strcpy`/`strcat`/`sprintf`
4. Pas d'`eval` : les valeurs reviennent par l'environnement/la sortie, jamais évaluées par l'outil
5. `fclose()` sur tout `FILE*`, jamais `pclose()`

Pour signaler une vulnérabilité : `devel@haplo-dialog.fr` (chiffrement GPG disponible).

---

## 10. Revue de code

La revue vérifie dans l'ordre :
1. **Sécurité**, les 5 règles absolues ci-dessus
2. **Compilation**, gcc et clang sans warning avec `-Wall -Wextra`
3. **Format**, `clang-format` sans diff
4. **Tests**, `make check` passe
5. **Documentation**, manpage(s) et exemples mis à jour si nouveau widget
6. **Style**, clarté, nommage cohérent avec le code existant

---

*Merci de contribuer à haplo-dialog, chaque script shell qui affiche une vraie interface vaut mieux qu'un `echo "Entrez un choix : "` dans un terminal.*

---

