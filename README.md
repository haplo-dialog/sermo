# haplo-dialog

**Un descendant moderne, durci et maintenu de gtkdialog — ports GTK 3 et GTK 4.**

> ## ⚠️ Renommage : ce qui a changé, et ce qu'il faut savoir
>
> Ce projet a distribué son paquet sous le nom **`gtk3dialog`**. Ce nom est déjà
> utilisé par un paquet distribué par **BunsenLabs**, ce que nous n'avions pas vu.
> Deux paquets de même nom ne cohabitent pas : l'un remplace l'autre en silence.
>
> La famille GTK s'appelle donc désormais **`gtk3sermo`**, **`gtk4sermo`** et
> **`gtksermo`** (compatibilité gtkdialog), sous la suite **haplo-dialog** — qui
> redevient ce qu'elle a toujours été : le nom du **produit**, pas celui d'un port.
>
> **Les anciens paquets `gtk3dialog` sont retirés** : ils ne sont plus
> téléchargeables nulle part. Si vous en aviez installé un sous BunsenLabs, il a pu
> écraser votre `gtk3dialog` — réinstallez celui de votre distribution.
>
> **Aucun paquet n'est publié pour l'instant.** On construit depuis les sources :
> voir [Installation](#installation).

[![Licence](https://img.shields.io/badge/licence-GPL--2.0--or--later-blue.svg)](LICENSE)
[![Version](https://img.shields.io/badge/version-1.0.0-informational.svg)](CHANGELOG.md)
[![Toolkit](https://img.shields.io/badge/toolkit-GTK%203%20%2B%20GTK%204-success.svg)](#les-deux-ports)
[![Tests](https://img.shields.io/badge/tests-52%2F52%20XML%20·%209%2F9%20comportement-brightgreen.svg)](#tests--qualité)

Décrivez une interface en XML, exportez-la dans une variable, lancez le binaire —
une vraie fenêtre **GTK 3 native** s'ouvre, et les valeurs saisies reviennent
dans vos variables shell. Là où **gtkdialog** est abandonné, haplo-dialog le fait
revivre : corrigé, durci, maintenu. Du grec *haplóos*, « simple ».

---

## Le geste signature

```sh
export MAIN_DIALOG='
<window title="Bonjour !" width-request="320">
  <vbox>
    <text><label>Entrez votre nom :</label></text>
    <entry><variable>NOM</variable></entry>
    <hbox>
      <button ok></button>
      <button cancel></button>
    </hbox>
  </vbox>
</window>'

gtk3sermo --program=MAIN_DIALOG     # → une fenêtre GTK 3 native s'ouvre
```

À la validation, la sortie revient au shell, prête à `eval` :

```sh
NOM="Ada"
EXIT="OK"
```

**~10 lignes de shell → une fenêtre native.**

---

## Pourquoi haplo-dialog ?

| | haplo-dialog | zenity / yad | gtkdialog (amont) |
|---|:--:|:--:|:--:|
| Interfaces XML arbitraires (widgets, conteneurs, signaux) | ✅ | ⚠️ limité | ✅ |
| Valeurs renvoyées dans des variables shell | ✅ | partiel | ✅ |
| Durci par défaut (pas de `system()`, FORTIFY, PIE, RELRO) | ✅ | — | ❌ |
| Maintenu | ✅ | ✅ | ❌ (abandonné) |
| Rétro-compatible gtkdialog | ✅ (alias) | — | — |

---

## Les deux ports

Un cœur C commun (grammaire flex/bison + automate + `safe_exec`), deux backends.

| Port | Binaire | Toolkit | Balises de widget | Particularité |
|------|---------|---------|:------:|---------------|
| **gtk3sermo** | `gtk3sermo` | GTK 3 | 52 | **Référence** · le plus éprouvé |
| **gtk4sermo** | `gtk4sermo` | GTK 4 | 56 | les 52 mêmes, plus `flowbox`, `overlay`, `revealer`, `stack` |

> Une seule grammaire pour les deux ports. Le compte est celui des **balises de
> widget acceptées par la grammaire** (`src/gtkdialog_lexer.l`), alias compris —
> il ne compte pas les balises de structure comme `<action>` ou `<variable>`.
>
> La commande `gtkdialog` elle-même n'est PAS dans ces paquets : elle est fournie
> par un paquet séparé, **`gtksermo`**. Voir [Installation](#installation).

---

## Installation

### Aucun paquet publié pour l'instant

Le dépôt ne publie **aucune release** et **aucun `.deb`** téléchargeable : les
anciens paquets ont été retirés au renommage. En attendant, on construit.

### Construire le paquet Debian

```sh
git clone https://gitlab.com/haplo-dialog/sermo.git
cd sermo/gtk3sermo/gtk3sermo_1.0.0        # ou gtk4sermo/gtk4sermo_1.0.0
dpkg-buildpackage -us -uc -b
```

Les paquets obtenus, et le découpage est volontaire :

| Paquet | Vient de | Installe | À savoir |
|---|---|---|---|
| `gtk3sermo` | port GTK 3 | `/usr/bin/gtk3sermo` | aucun conflit |
| `gtksermo` | port GTK 3 | `/usr/bin/gtkdialog` | **en conflit** avec `gtkdialog` et `gtk3dialog` |
| `gtk4sermo` | port GTK 4 | `/usr/bin/gtk4sermo` | aucun conflit |

> **La commande `gtkdialog` est dans `gtksermo`, pas dans `gtk3sermo`.**
> C'est voulu. Le paquet `gtk3dialog` de BunsenLabs portait le même nom que
> celui que nous distribuions : deux paquets de même nom ne sont pas des rivaux
> pour apt, le plus haut écrase l'autre en silence. Avec des noms distincts, apt
> refuse et explique. N'installez `gtksermo` que si vous voulez la commande
> `gtkdialog` — sinon vos scripts marchent en appelant directement `gtk3sermo`
> ou `gtk4sermo`.

### Depuis les sources, sans paquet

```sh
cd gtk3sermo/gtk3sermo_1.0.0 && autoreconf -fi && ./configure && make -j"$(nproc)" && sudo make install
```

⚠️ Ici `make install` pose aussi le lien `gtkdialog`, sans le garde-fou des
conflits de paquets : à éviter si vous avez déjà un `gtkdialog` ou un
`gtk3dialog` installé par votre distribution.

Des recettes RPM, Arch, Gentoo et Slackware existent dans `packaging/`, mais
**aucune n'a jamais été construite par nous** et leur URL de source ne répond pas
encore. Voir [PACKAGING.md](PACKAGING.md).

---

## Sécurité

Durci par défaut : exécution via **`safe_exec`** (pas de `system()`),
**`_FORTIFY_SOURCE=3`**, **PIE**, **Full RELRO**, `stack-protector-strong`,
`-fcf-protection`. Le parser **rejette proprement** l'XML malformé (message +
code de sortie non nul, jamais d'`abort`), validé par **fuzzing** (`tests/fuzz/`).
Signalement de vulnérabilité : voir [SECURITY.md](SECURITY.md).

---

## Tests & qualité

- **Régression XML** : `./tests/xml/run_tests.sh all`, **52/52** (parse headless `--print-ir`).
- **Comportement** : `./tests/run_unit_tests.sh all`, `safe_exec`, **9/9** (sans serveur X).
- **Fuzzing** du parser : `./tests/fuzz/run_fuzz.sh 60` (afl++ ou repli intégré).
- **CI** : build + tests, `.gitlab-ci.yml` et `.gitea/workflows/`.

---

## Documentation

- Manuels utilisateur/développeur (`gtk3sermo/gtk3sermo_1.0.0/MANUEL_*.md`).
- Pages de manuel `gtk3sermo(1)` et référence XML `haplo-dialog-xml(5)`.
- Manuels Texinfo (`gtk3sermo/gtk3sermo_1.0.0/doc/`).
- Site : <https://haplo-dialog.fr>.

---

## Contribuer & licence

Contributions bienvenues, voir [CONTRIBUTING.md](CONTRIBUTING.md) et
[CODE_OF_CONDUCT.md](CODE_OF_CONDUCT.md).

**GPL-2.0-or-later**, voir [LICENSE](LICENSE). Les exemples que le projet a écrits
(`examples/showcase/`, `examples/system-tools/`) sont en CC0 ; le reste d'`examples/`
vient de gtkdialog et suit la GPL, et trois jeux d'icônes de tiers portent leur
propre `COPYING`. Détail dans [LICENCES.md](LICENCES.md).

---

## Développement assisté par IA - dit franchement

gtk3sermo a été écrit avec l'aide d'une IA (Claude, d'Anthropic) —
conception, code, tests, documentation, sous ma relecture et ma responsabilité.
Je le dis ouvertement : l'honnêteté ne se négocie pas. Je crois qu'un outil bien
employé, sans rien masquer, sert la qualité au lieu de lui nuire. Le code est à
moi ; les erreurs aussi.

> *haplo-dialog*
