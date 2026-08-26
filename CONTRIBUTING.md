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
haplo-dialog fournit deux ports, `gtk3sermo` (backend GTK 3) et `gtk4sermo` (backend GTK 4). L'alias rétro-compatible `gtkdialog` est fourni par un paquet séparé, `gtksermo`. C'est un descendant maintenu de gtkdialog (fork de gtkdialog 0.8.3 de Laszlo Pere), corrigé et durci. D'autres descendants existent, notamment le fork BunsenLabs de Mick Amadio, qui porte lui aussi gtkdialog sur GTK 3.

---

## 2. Structure du projet

```
haplo-dialog/
├── gtk3sermo/
│   └── gtk3sermo_1.1.0/
│       ├── src/
│       │   ├── gtk3sermo.c      ← point d'entrée principal
│       │   ├── safe_exec.c       ← GPL-2.0-or-later
│       │   ├── variables.c       ← GPL-2.0-or-later
│       │   ├── stringman.c       ← GPL-2.0-or-later
│       │   └── widget_*.c        ← un fichier par widget (GPL-2.0-or-later)
│       ├── tests/
│       ├── examples/             ← scripts #!/bin/sh par widget (GPL ; showcase/ et system-tools/ en CC0)
│       ├── doc/
│       └── packaging/
└── tests/
    ├── xml/                      ← régression XML (55 cas)
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
cd gtk3sermo/gtk3sermo_1.1.0
autoreconf -fi
./configure --prefix=/usr/local CFLAGS="-g -O0 -fsanitize=address"
make -j$(nproc)
make check        # lance les tests
```

### Outils recommandés

```sh
sudo apt install clang-format cppcheck valgrind
# Sur un fichier NEUF seulement — les fichiers hérités ne sont pas conformes,
# et --Werror y échouerait toujours (voir §5) :
clang-format --dry-run src/mon_nouveau_fichier.c
```

---

## 4. Workflow de contribution

```
1. Fork / clone du dépôt
2. Créer une branche : git checkout -b liste-plante-sur-item-vide
3. Modifier le code, un seul sujet par branche
4. Tester le port touché : make check, puis
   sh tests/xml/run_tests.sh <port> et sh tests/run_unit_tests.sh <port>
5. Si le correctif touche les DEUX ports, le porter dans les deux :
   ils partagent le cœur, et une divergence silencieuse est vite là
6. Commit (voir ci-dessous)
7. Ouvrir une Merge Request sur gitlab.com/haplo-dialog/sermo,
   ou envoyer le patch à devel@haplo-dialog.fr
```

### Format des messages de commit

Ce dépôt n'utilise **pas** les *conventional commits* (`fix(scope): …`). Sa
convention, lisible dans `git log`, est une **phrase française** qui dit ce qui
changeait de comportement — pas quel fichier a bougé :

```
La barre de progression faisait tourner GTK depuis son thread de lecture

Corps : le pourquoi, la mesure avant/après, le test qui l'empêche de revenir.
Référence : Fixes #123
```

Trois règles, et c'est tout :

- **Sujet en français**, moins de 72 caractères, pas de point final.
- Il décrit **le défaut ou l'effet**, du point de vue de qui s'en sert. « Le
  dépôt disait à ses lecteurs d'installer un fichier qui n'existe pas » plutôt
  que « corrige README ».
- Le corps porte **le pourquoi et la mesure**. Un correctif de sécurité ou de
  plantage cite le chiffre avant/après et le banc qui le verrouille.

`git log --oneline -20` donne le ton mieux que cette liste.

---

## 5. Standards de code

### C (cœur et widgets, les deux ports)

- Standard : **C11** (`-std=c11`)
- Formatage : **ne pas** lancer `clang-format -i` sur les fichiers existants.
  Le `.clang-format` fourni à la racine décrit le style visé pour du code
  **neuf** ; il ne décrit pas le code hérité de gtkdialog. Mesuré le 2026-08-25
  sur `gtkdialog.c`, `actions.c` et `variables.c` (3 766 lignes) : le réglage
  actuel voudrait en reformater **2 460**, et le meilleur réglage essayé encore
  **1 321**. Un `clang-format -i src/*.c` produirait donc un diff illisible où
  le vrai correctif serait introuvable. Pour du code neuf :
  `clang-format --dry-run <votre-fichier.c>`. Pour une retouche dans un fichier
  existant : imiter les lignes voisines (tabulations, accolades K&R).
- Pas de `system()`, `popen()`, `strcpy()`, `strcat()`, `sprintf()`, `gets()`
  directs : utiliser `safe_system()`, `safe_popen()`, `g_strlcpy()`,
  `g_strlcat()`, `g_snprintf()`. **Ce n'est pas qu'une consigne** :
  `tests/garde_fonctions_interdites.sh` la rejoue à chaque poussée sur les deux
  `src/`, et la CI passe au rouge.
- Aucun appel `gtk_*`/`gdk_*` hors du thread principal — un thread de travail
  calcule, puis confie le résultat à la boucle principale par `g_idle_add()`.
  `gdk_threads_enter()` ne verrouille plus rien depuis GTK 3.6 : du code hérité
  qui paraît protégé ne l'est pas. Vérifié par
  `tests/garde_progressbar_thread.sh`.
- Pas d'`atof()` ni de `strtod()` : sous une locale française ils lisent `0.5`
  comme `0`, en silence. Utiliser `g_ascii_strtod()`.
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
cd gtk3sermo/gtk3sermo_1.1.0
autoreconf -fi && ./configure && make check
```

Les tests unitaires ne doivent **pas** dépendre de GTK 3, uniquement du core C pur (`safe_exec`, `variables`, `stringman`).

### Tests de régression XML

```sh
cd tests/xml
./run_tests.sh gtk3sermo    # teste les 55 XML de référence (mode --print-ir, headless)
```

### Tests de comportement (unitaires)

Au-delà du parse, on **exécute** réellement la logique cœur (sécurité
`safe_exec` : `safe_system`/`safe_popen`). « `main()` pur » : ni libcheck ni
serveur X requis, donc runnable en CI. 9 tests de comportement.

```sh
./tests/run_unit_tests.sh gtk3sermo
```

### Valgrind (mémoire)

```sh
valgrind --leak-check=full --error-exitcode=1 \
  gtk3sermo --file tests/xml/01_button.xml --do EXIT:exit
```

### Intégration continue (CI)

Le dépôt fournit `.gitlab-ci.yml` (GitLab CI, image `debian:testing`, cible : Debian testing). À chaque push / merge request, le job :

1. installe les dépendances de build ;
2. compile (autotools) ;
3. valide le binaire contre les 55 cas XML (`tests/xml/run_tests.sh`, `--print-ir`,
   sans serveur X).

Un build cassé ou un seul test en échec fait échouer le pipeline. Avant de pousser,
on peut reproduire le job en local :

```sh
cd gtk3sermo/gtk3sermo_1.1.0
autoreconf -fi && ./configure --prefix="$PWD/inst" && make -j"$(nproc)" && make install
PATH="$PWD/inst/bin:$PATH" sh ../../tests/xml/run_tests.sh gtk3sermo
```

---

## 8. Documentation

- Les manpages sont en format **roff** (`src/gtk3sermo.1`), à jour avec chaque nouveau widget
- La référence XML est dans `haplo-dialog-xml(5)`, `man/haplo-dialog-xml.5`
- La documentation Texinfo est dans `doc/`
- `examples/showcase/` et `examples/system-tools/` sont en **CC0** : les utiliser sans restriction. Le reste d'`examples/` vient de gtkdialog et reste en **GPL-2.0-or-later**

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
3. **Format**, cohérent avec les lignes voisines ; `clang-format` sans diff pour
   un fichier neuf seulement
4. **Tests**, `make check` passe
5. **Documentation**, manpage(s) et exemples mis à jour si nouveau widget
6. **Style**, clarté, nommage cohérent avec le code existant

---

*Merci de contribuer à haplo-dialog, chaque script shell qui affiche une vraie interface vaut mieux qu'un `echo "Entrez un choix : "` dans un terminal.*

---

