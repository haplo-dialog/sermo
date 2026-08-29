# haplo-dialog

**Un descendant moderne, durci et maintenu de gtkdialog — ports GTK 3 et GTK 4.**

> ### ⚠️ Si vous aviez installé un paquet `gtk3dialog` de ce projet
>
> Ce nom est aussi celui d'un paquet de **BunsenLabs**. Deux paquets de même nom
> ne cohabitent pas : le nôtre a pu **écraser le vôtre en silence**. Les nôtres
> sont retirés — si vous en aviez un, **réinstallez le `gtk3dialog` de votre
> distribution**.
>
> Nos paquets s'appellent aujourd'hui `gtk3sermo`, `gtk4sermo` et `gtksermo`.

[![Licence](https://img.shields.io/badge/licence-GPL--2.0--or--later-blue.svg)](LICENSE)
[![Version](https://img.shields.io/badge/version-1.1.3-informational.svg)](CHANGELOG.md)
[![Toolkit](https://img.shields.io/badge/toolkit-GTK%203%20%2B%20GTK%204-success.svg)](#les-deux-ports)
[![Tests](https://img.shields.io/badge/tests-55%2F55%20XML%20·%2011%2F11%20comportement-brightgreen.svg)](#tests--qualité)

Décrivez une interface en XML, exportez-la dans une variable, lancez le binaire —
une vraie fenêtre **native** s'ouvre — GTK 3 ou GTK 4, au choix du port —, et
les valeurs saisies reviennent dans vos variables shell. Là où **gtkdialog** est abandonné, haplo-dialog le fait
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

À la validation, la sortie revient au shell :

```sh
NOM="Ada"
EXIT="OK"
```

Le plus simple, et le plus sûr, est de laisser `--do` faire le travail : les
valeurs arrivent comme variables d'environnement, sans jamais repasser par le
shell.

```sh
gtk3sermo --program=MAIN_DIALOG --do='echo "Bonjour $NOM"'
```

> **Ne passez pas cette sortie à `eval` sans y réfléchir.** La valeur d'un champ
> est tapée par la personne qui se sert du dialogue, et ce n'est pas forcément
> celle qui a écrit le script. Depuis le 2026-08-25 le programme échappe les
> quatre caractères que le shell développe entre guillemets doubles (`\`, `"`,
> `$` et l'accent grave), donc `eval` ne les exécute plus — un banc le vérifie à
> chaque poussée. Avant cette date, il n'en échappait que deux, et `eval`
> exécutait ce qu'on avait tapé.

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
> Les documents internes de chaque port citent un autre chiffre — **43** et
> **50** — parce qu'ils comptent les **fichiers d'implémentation**
> (`ls src/widget_*.c`). Les deux sont exacts : plusieurs balises partagent un
> même fichier (les alias, les variantes `h`/`v`). Quand un chiffre apparaît
> quelque part, il dit lequel des deux il est.
>
> La commande `gtkdialog` elle-même n'est PAS dans ces paquets : elle est fournie
> par un paquet séparé, **`gtksermo`**. Voir [Installation](#installation).

---

## Installation

### Télécharger la release

Les paquets sont joints à la release [**v1.1.3**](https://gitlab.com/haplo-dialog/sermo/-/releases/v1.1.3), avec leurs
sommes de contrôle. Il n'y a **pas de dépôt APT** : on télécharge, on vérifie,
on installe.

```sh
U=https://gitlab.com/api/v4/projects/85674825/packages/generic/sermo/1.1.3
for f in gtk3sermo_1.1.3-1_amd64.deb gtksermo_1.1.3-1_all.deb SHA256SUMS; do
    curl -fLO "$U/$f"
done

sha256sum --ignore-missing -c SHA256SUMS
sudo apt install ./gtk3sermo_1.1.3-1_amd64.deb
```

| Paquet | Commande installée | Conflits |
|---|---|---|
| `gtk3sermo` 1.1.3-1 | `/usr/bin/gtk3sermo` | aucun |
| `gtk4sermo` 1.1.3-1 | `/usr/bin/gtk4sermo` | aucun |
| `gtksermo` 1.1.3-1 | `/usr/bin/gtkdialog` | **avec `gtkdialog` et `gtk3dialog`** |

> ⚠️ **Les paquets de la release `v1.0.0` ont été retirés** (versions `1.0.0-10`
> et `1.0.0-11`). Ils portaient trois défauts : la sortie rendue au shell y était
> exécutable par `eval` si l'utilisateur du dialogue tapait `$(commande)` dans un
> champ, cliquer un `<switch>` tuait le programme, et sous locale française un
> nombre écrit avec un point valait zéro en silence. **Si vous en aviez installé
> un, remplacez-le** par le paquet correspondant de la `v1.1.3`.
>
> La release `v1.0.0-4` porte, elle, les trois correctifs : ses paquets sont sains,
> simplement remplacés par ceux de la `v1.1.3`.

Les deux premiers s'installent sans conflit, y compris à côté du `gtk3dialog` de
BunsenLabs. Le troisième fournit la commande `gtkdialog`, donc il entre en
conflit avec lui : ne l'installez que si vous voulez cette commande.

Les paquets `-dbgsym` ne servent qu'à lire une trace d'exécution.

### Construire le paquet Debian

```sh
git clone https://gitlab.com/haplo-dialog/sermo.git
cd sermo/gtk3sermo/gtk3sermo_1.1.3        # ou gtk4sermo/gtk4sermo_1.1.3
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
cd gtk3sermo/gtk3sermo_1.1.3 && autoreconf -fi && ./configure && make -j"$(nproc)" && sudo make install
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

- **Régression XML** : `./tests/xml/run_tests.sh all`, **55/55** (parse headless `--print-ir`).
- **Comportement** : `./tests/run_unit_tests.sh all`, `safe_exec`, **9/9** (sans serveur X).
- **Fuzzing** du parser : `./tests/fuzz/run_fuzz.sh gtk3sermo 60` (afl++ ou repli intégré).
- **CI** : build + tests, `.gitlab-ci.yml` et `.gitea/workflows/`.

---

## Documentation

- Manuels utilisateur/développeur (`gtk3sermo/gtk3sermo_1.1.3/MANUEL_*.md`).
- Pages de manuel `gtk3sermo(1)` et référence XML `haplo-dialog-xml(5)`.
- Manuels Texinfo (`gtk3sermo/gtk3sermo_1.1.3/doc/`).
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
