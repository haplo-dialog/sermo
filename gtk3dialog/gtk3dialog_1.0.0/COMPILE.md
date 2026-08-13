# Compiler gtk3dialog 1.0.0 (GTK3, haplo-dialog)

## Dépendances requises

```bash
# Debian 12 / Ubuntu 24.04
sudo apt-get install \
  libgtk-3-dev libglib2.0-dev libvte-2.91-dev \
  flex bison \
  autoconf automake \
  pkg-config gcc

# Tests unitaires C (optionnel — make check)
sudo apt-get install libcheck-dev

# Arch Linux
sudo pacman -S gtk3 vte3 flex bison autoconf automake pkgconf gcc check
```

---

## Compilation standard

```bash
cd gtk3dialog_1.0.0/
./autogen.sh
make -j$(nproc)
sudo make install
```

> **Note — compatibilité ascendante gtkdialog :** `make install` pose un symlink
> `gtkdialog` → `gtk3dialog` (binaire) et un alias man `gtkdialog.1` → `gtk3dialog.1`
> via le hook autotools (`install-exec-hook` / `install-data-hook`). Les dialogues
> d'époque (XML / CLI / `--program` via variable d'environnement) restent ainsi
> invocables sous le nom `gtkdialog`. Seul gtk3dialog fournit cet alias.

> **Note — chemin avec espaces ou `&` :** autotools exige que le répertoire de
> build ne contienne ni espaces ni métacaractères shell. Si votre arborescence
> source est dans un tel chemin (ex. `Projets-Minus & Cortex/…`), `autogen.sh`
> le détecte automatiquement et effectue un **build hors-arbre** :
> - symlink source : `/tmp/gtk3dialog-src`
> - répertoire de build : `/tmp/gtk3dialog-build` (ou `GTK3DIALOG_BUILDDIR`)
>
> `make` et `make install` doivent alors être lancés depuis ce répertoire :
>
> ```bash
> ./autogen.sh                        # détecte & crée /tmp/gtk3dialog-build
> cd /tmp/gtk3dialog-build
> make -j$(nproc)
> sudo make install
> ```
>
> Pour forcer un répertoire de build personnalisé :
> ```bash
> GTK3DIALOG_BUILDDIR=~/build/gtk3dialog ./autogen.sh
> ```

---

## Options de compilation

### Mode debug (active DEBUG_TRANSITS, DEBUG_CONTENT, PIP_DEBUG)

```bash
./configure --enable-debug
make -j$(nproc)
# Activer les traces g_debug() à l'exécution :
G_MESSAGES_DEBUG=all ./src/gtk3dialog --debug --file=mon_dialog.xml
```

### Sans support terminal VTE

```bash
./configure --without-vte
make -j$(nproc)
```

### Avec tests unitaires C (libcheck requis)

```bash
./configure --enable-unit-tests
make -j$(nproc)
make check
```

---

## Lancer les tests

### Tests fonctionnels shell (sans display requis)

```bash
bash tests/run_tests.sh                    # gtk3dialog du PATH
bash tests/run_tests.sh src/gtk3dialog      # binaire compilé
```

Résultat attendu : `44 passés | 0 échoués`.

### Tests XML communs (`tests/xml/`, sans display requis)

Suite XML de gtk3dialog (52 cas, parsés via `--print-ir`, headless) :

```bash
bash tests/run_tests.sh gtk3dialog
```

Résultat vérifié (2026-06-06) : **52/52 PASS**.

### Tests unitaires C (libcheck + make check)

```bash
./configure --enable-unit-tests
make -j$(nproc)
make check
# Résultats dans tests/*.log
```

---

## Construire un paquet .deb (Debian)

### Dépendances supplémentaires pour le packaging

```bash
sudo apt-get install dpkg-dev debhelper devscripts lintian
```

### Méthode rapide (make deb)

```bash
cd gtk3dialog_1.0.0/
./autogen.sh    # génère configure
make deb        # appelle dpkg-buildpackage -us -uc -b
```

Le paquet `.deb` est créé dans le répertoire parent (`../`) :

```
../gtk3dialog_1.0.0-1_amd64.deb
```

### Installation du paquet

```bash
sudo dpkg -i ../gtk3dialog_1.0.0-1_amd64.deb
# ou avec gestion automatique des dépendances manquantes :
sudo apt-get install -f ../gtk3dialog_1.0.0-1_amd64.deb
```

### Méthode complète (dpkg-buildpackage)

```bash
cd gtk3dialog_1.0.0/
./autogen.sh NOCONFIGURE=1    # ne lance pas ./configure
dpkg-buildpackage -us -uc -b  # -us/-uc = pas de signature GPG
```

### Nettoyage des artefacts de build

```bash
make deb-clean
# supprime ../gtk3dialog_*.deb, *.changes, *.buildinfo
```

---

## Build reproductible avec Docker

Le fichier `Dockerfile.build` fournit un environnement de build 100% reproductible
basé sur Debian 12, sans dépendance à l'environnement local.

### Construction de l'image

```bash
cd gtk3dialog_1.0.0/
docker build -f Dockerfile.build -t gtk3dialog-build .
```

### Compilation + tests + packaging en une commande

```bash
mkdir -p ../dist
docker run --rm -v "$PWD/../dist":/dist gtk3dialog-build
# Le .deb est copié dans ../dist/
ls ../di../gtk3dialog_*.deb
```

### Options Docker

```bash
# Build debug
docker build --build-arg ENABLE_DEBUG=yes \
    -f Dockerfile.build -t gtk3dialog-build-dbg .

# Build sans VTE (terminal widget)
docker build --build-arg ENABLE_VTE=no \
    -f Dockerfile.build -t gtk3dialog-build-novte .
```

### Stages disponibles dans le Dockerfile multi-stage

| Stage      | Contenu                                     |
|------------|---------------------------------------------|
| `builder`  | Compilation seule (sans tests)              |
| `tester`   | Compilation + `make check` + `run_tests.sh` |
| `packager` | Tester + `dpkg-buildpackage`                |
| `exporter` | Image minimale — export du .deb uniquement  |

Pour s'arrêter à un stage intermédiaire :

```bash
# Compiler et tester sans packager
docker build --target tester -f Dockerfile.build -t gtk3dialog-test .
docker run --rm gtk3dialog-test make check
```

---

## Vérification de la sécurité du binaire

```bash
# Après make install (ou dans src/ après make)
checksec --file=/usr/bin/gtk3dialog
# Résultat attendu :
# RELRO: Full | Stack: Canary | NX: Enabled | PIE: Enabled

# Alternative (objdump) :
objdump -f src/gtk3dialog | grep -E "DYNAMIC|PIE"
```

Les flags de hardening sont activés dans `src/Makefile.am` :
`-D_FORTIFY_SOURCE=3 -fstack-protector-strong -fPIE -pie -Wl,-z,relro -Wl,-z,now`

---

## Test rapide de bon fonctionnement

```bash
echo '<window><vbox><text><label>Hello GTK3!</label></text>
<button><label>Quitter</label><action>EXIT:ok</action></button>
</vbox></window>' | gtk3dialog --stdin
```

Les 6 nouveaux widgets (switch, filechooser, calendar, linkbutton, searchentry, infobar)
sont testés via les exemples dans `examples/` :

```bash
cd examples/switch && ./switch
cd examples/calendar && ./calendar
```

---

## En cas d'erreur de compilation

1. **`GtkBuilder` signal connect** — `gtk_builder_connect_signals_full()`
   a une signature différente de `glade_xml_signal_autoconnect_full()`
   → Voir `src/glade_support.c`

2. **`vte_terminal_spawn_sync()`** — prend un paramètre `GCancellable*`
   supplémentaire
   → Voir `src/widget_terminal.c`

3. **`autoreconf` introuvable** — installer `autoconf automake` :
   ```bash
   sudo apt-get install autoconf automake
   ```

4. **`pkg-config: check not found`** — installer `libcheck-dev` pour
   les tests unitaires, ou compiler sans : `./configure` (sans `--enable-unit-tests`)

5. **Erreurs de préprocesseur `#endif` orphelin** — normalement corrigées
   depuis le patch 10. Vérifier avec :
   ```bash
   gcc -E src/automaton.c -o /dev/null 2>&1 | grep error
   ```

---

