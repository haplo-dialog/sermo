# Compiler gtk3sermo 1.0.0 (GTK3, haplo-dialog)

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
cd gtk3sermo_1.0.0/
./autogen.sh
make -j$(nproc)
sudo make install
```

> **Note — compatibilité ascendante gtkdialog :** `make install` pose un symlink
> `gtkdialog` → `gtk3sermo` (binaire) et un alias man `gtkdialog.1` → `gtk3sermo.1`
> via le hook autotools (`install-exec-hook` / `install-data-hook`). Les dialogues
> d'époque (XML / CLI / `--program` via variable d'environnement) restent ainsi
> invocables sous le nom `gtkdialog`. Seul gtk3sermo fournit cet alias.

> **Note — chemin avec espaces ou `&` :** autotools exige que le répertoire de
> build ne contienne ni espaces ni métacaractères shell. Si votre arborescence
> source est dans un tel chemin (ex. `Projets-Minus & Cortex/…`), `autogen.sh`
> le détecte automatiquement et effectue un **build hors-arbre** :
> - symlink source : `/tmp/gtk3sermo-src`
> - répertoire de build : `/tmp/gtk3sermo-build` (ou `GTK3DIALOG_BUILDDIR`)
>
> `make` et `make install` doivent alors être lancés depuis ce répertoire :
>
> ```bash
> ./autogen.sh                        # détecte & crée /tmp/gtk3sermo-build
> cd /tmp/gtk3sermo-build
> make -j$(nproc)
> sudo make install
> ```
>
> Pour forcer un répertoire de build personnalisé :
> ```bash
> GTK3DIALOG_BUILDDIR=~/build/gtk3sermo ./autogen.sh
> ```

---

## Options de compilation

### Mode debug (active DEBUG_TRANSITS, DEBUG_CONTENT, PIP_DEBUG)

```bash
./configure --enable-debug
make -j$(nproc)
# Activer les traces g_debug() à l'exécution :
G_MESSAGES_DEBUG=all ./src/gtk3sermo --debug --file=mon_dialog.xml
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

> ⚠️ **Deux scripts portent le nom `run_tests.sh`**, et ils ne font pas la même
> chose. Celui de ce dossier (`tests/run_tests.sh`) contient les tests
> fonctionnels shell ; celui de la racine du dépôt (`tests/xml/run_tests.sh`)
> rejoue la suite XML. Les commandes ci-dessous disent chaque fois lequel.

### Tests fonctionnels shell (sans affichage requis)

```bash
bash tests/run_tests.sh                    # gtk3sermo trouvé dans le PATH
bash tests/run_tests.sh src/gtk3sermo      # binaire tout juste compilé
```

Résultat mesuré le 2026-08-25 : **49 passés | 0 échoués | 0 ignorés**.

### Suite XML commune aux deux ports (sans affichage requis)

Le script vit à la **racine du dépôt**, pas dans ce dossier — la suite est
partagée entre `gtk3sermo` et `gtk4sermo` :

```bash
bash <racine>/tests/xml/run_tests.sh gtk3sermo   # ou gtk4sermo, ou « all »
```

55 cas, analysés via `--print-ir`, sans serveur X. Résultat mesuré le
2026-08-25, en locale `fr_FR.UTF-8` : **55/55 PASS pour chacun des deux ports**.

### Tests unitaires C (libcheck + `make check`)

```bash
./configure --enable-unit-tests
make -j$(nproc)
make check
# Journaux détaillés dans tests/*.log
```

Résultat mesuré le 2026-08-25 : **3 PASS, 0 FAIL** — `test_safe_exec`,
`test_stringman` et le wrapper shell.

> Cette option a longtemps été cassée. `test_stringman.c` était écrit contre une
> vingtaine de fonctions `command_is_*()` / `command_get_*()` qui n'existent
> dans aucun binaire : leurs définitions sont dans un bloc commenté de
> `stringman.c`, hérité de l'amont. `make check` échouait donc à l'édition de
> liens, avec une trentaine de « undefined reference ». Le test porte désormais
> sur l'API qui a remplacé ces fonctions — la table de préfixes lue par
> `command_get_prefix()` et `command_prefix_get_type()` — et sur `strnatcmp`,
> `linecutter`, `str_default_name`. Éprouvé par deux sabotages : désaligner la
> table de préfixes et l'énumération, et tronquer une commande shell qui
> contient un deux-points ; les deux sont attrapés et nommés.

---

## Construire un paquet .deb (Debian)

### Dépendances supplémentaires pour le packaging

```bash
sudo apt-get install dpkg-dev debhelper devscripts lintian
```

### Méthode rapide (make deb)

```bash
cd gtk3sermo_1.0.0/
./autogen.sh    # génère configure
make deb        # appelle dpkg-buildpackage -us -uc -b
```

Le paquet `.deb` est créé dans le répertoire parent (`../`) :

```
../gtk3sermo_1.0.0-1_amd64.deb
```

### Installation du paquet

```bash
sudo dpkg -i ../gtk3sermo_1.0.0-1_amd64.deb
# ou avec gestion automatique des dépendances manquantes :
sudo apt-get install -f ../gtk3sermo_1.0.0-1_amd64.deb
```

### Méthode complète (dpkg-buildpackage)

```bash
cd gtk3sermo_1.0.0/
./autogen.sh NOCONFIGURE=1    # ne lance pas ./configure
dpkg-buildpackage -us -uc -b  # -us/-uc = pas de signature GPG
```

### Nettoyage des artefacts de build

```bash
make deb-clean
# supprime ../gtk3sermo_*.deb, *.changes, *.buildinfo
```

---

## Build reproductible avec Docker

Le fichier `ci/Dockerfile.gtk3sermo` fournit un environnement de build isolé de
la machine locale, basé sur `debian:testing`.

⚠️ Le **contexte de build est la racine du dépôt**, pas ce dossier : le
Dockerfile fait `COPY gtk3sermo/gtk3sermo_1.0.0 .`. Lancer la commande depuis
`gtk3sermo_1.0.0/` échoue.

### Construction de l'image

```bash
cd <racine-du-depot>
docker build -f ci/Dockerfile.gtk3sermo -t gtk3sermo .
```

**Mesuré le 2026-08-25** : `rc=0`, image de 215 Mo,
`docker run --rm gtk3sermo --version` répond `gtk3sermo version 1.0.0`.

### Ce que l'image contient

Deux étages, et deux seulement :

| Étage | Base | Contenu |
|-------|------|---------|
| `builder` | `debian:testing` | outils de build + sources, `autoreconf && configure && make install` vers `/install` |
| `runtime` | `debian:testing-slim` | le binaire et les pages de manuel recopiés depuis `builder`, rien d'autre |

L'`ENTRYPOINT` est le binaire lui-même, avec `--help` par défaut :

```bash
docker run --rm gtk3sermo                    # affiche l'aide
docker run --rm gtk3sermo --version
docker run --rm --entrypoint sh gtk3sermo -c 'gtk3sermo --help'
```

### Ce que l'image ne fait PAS

- Pas d'image pour le port GTK 4 : `gtk4sermo` se construit par autotools
  seulement.

---

## Vérification de la sécurité du binaire

```bash
# Après make install (ou dans src/ après make)
checksec --file=/usr/bin/gtk3sermo
# Résultat attendu :
# RELRO: Full | Stack: Canary | NX: Enabled | PIE: Enabled

# Alternative (objdump) :
objdump -f src/gtk3sermo | grep -E "DYNAMIC|PIE"
```

Les flags de hardening sont activés dans `src/Makefile.am` :
`-D_FORTIFY_SOURCE=3 -fstack-protector-strong -fPIE -pie -Wl,-z,relro -Wl,-z,now`

---

## Test rapide de bon fonctionnement

```bash
echo '<window><vbox><text><label>Hello GTK3!</label></text>
<button><label>Quitter</label><action>EXIT:ok</action></button>
</vbox></window>' | gtk3sermo --stdin
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

