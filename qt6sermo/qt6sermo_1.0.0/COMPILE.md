# Compiler qt6sermo 1.0.0 (Qt6, haplo-dialog)

Port Qt6 de gtkdialog (port phare). Code C / C++, build par **CMake**
(flex + bison). GLib n'est requis que pour les tests unitaires.

## Dépendances requises

```bash
# Debian 12 / haplo-dialog / Ubuntu 24.04
sudo apt-get install \
  cmake \
  qt6-base-dev \
  flex bison \
  pkg-config gcc g++

# Tests unitaires (optionnel) :
sudo apt-get install libglib2.0-dev

# Arch Linux
sudo pacman -S cmake qt6-base flex bison pkgconf gcc
```

> **Qt6 (composants Core + Widgets)** requis, détecté via
> `find_package(Qt6 REQUIRED COMPONENTS Core Widgets)`. flex et bison sont
> obligatoires (lexer/parser du format XML).

---

## Compilation standard

```bash
cd qt6sermo_1.0.0/
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j$(nproc)
sudo cmake --install build
```

> **Note :** CMake gère sans problème les chemins contenant espaces ou `&`,
> contrairement aux ports autotools.

---

## Options de compilation

### Mode debug

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build -j$(nproc)
```

### Tests unitaires C (GLib requis)

Le projet déclare `test_safe_exec` et `test_compat` quand GLib est détecté
(`find_package(PkgConfig QUIET)`).

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build -j$(nproc)
ctest --test-dir build --output-on-failure
```

### Vérifier la détection Qt6

```bash
cmake -B build -LA | grep -iE "Qt6|GLIB"
qmake6 --version    # informatif
```

---

## Test rapide de bon fonctionnement

```bash
echo '<window><vbox><text><label>Hello Qt6!</label></text>
<button><label>Quitter</label><action>EXIT:ok</action></button>
</vbox></window>' | ./build/qt6sermo --stdin
```

La syntaxe XML est identique à celle de gtkdialog : les dialogues écrits pour
gtk3d fonctionnent sans modification sous qt6sermo.

---

## Tests XML communs (`tests/xml/`, sans display requis)

Suite XML partagée par les six ports (52 cas, parsés via `--print-ir`, headless),
à la racine du dépôt :

```bash
bash ../../tests/xml/run_tests.sh qt6sermo
```

Résultat vérifié (2026-06-06) : **52/52 PASS** (auparavant 50/52 — `<levelbar>`
et `<drawingarea>` désormais câblés dans la grammaire).

## Tests de comportement par widget (`tests/behaviour/`, affichage requis)

Nouvelle suite partagée (12 cas) : chaque cas ouvre une **vraie fenêtre**,
auto-fermée par un `<timer>` qui déclenche `EXIT:ok`, et les exports
(`VAR="valeur"`) sont vérifiés sur stdout. Sans affichage, lancer sous
`xvfb-run` (c'est ce que fait la CI) :

```bash
xvfb-run -a sh ../../tests/behaviour/run_behaviour_tests.sh qt6sermo
```

Résultat vérifié (2026-06-10) : **12/12 PASS** (après correctifs
`widget_timer.cpp` — dispatch des actions via `execute_action()` — et
`widget_hscale.cpp` — lecture de l'attribut `range-value`).

Les tests du cœur (`sh ../../tests/run_unit_tests.sh qt6sermo`, safe_exec,
headless) passent **9/9**.

---

## En cas d'erreur de compilation

1. **`Could NOT find Qt6`** — installer `qt6-base-dev` ; si Qt6 est dans un préfixe
   non standard, passer `-DCMAKE_PREFIX_PATH=/chemin/vers/qt6`.

2. **`Qt6::Widgets` manquant** — le paquet `qt6-base-dev` fournit Core + Widgets ;
   sur certaines distributions Widgets est dans un sous-paquet séparé.

3. **Tests non construits** — `test_safe_exec`/`test_compat` ne sont compilés que
   si GLib est présent ; installer `libglib2.0-dev` puis reconfigurer.

4. **`flex`/`bison` introuvables** — `find_package(FLEX/BISON REQUIRED)` ; installer
   les deux paquets.

5. **Erreur MOC / standard C++** — Qt6 exige C++17 minimum ; vérifier que `g++`
   est suffisamment récent (GCC 9+).

---

*Document horodaté — dernière mise à jour : 2026-06-10 (audit #6 — haplo-dialog v1.0.0).*