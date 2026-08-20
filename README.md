# haplo-dialog

**Le successeur moderne, durci et maintenu de gtkdialog (GTK 3).**

[![Licence](https://img.shields.io/badge/licence-GPL--2.0--or--later-blue.svg)](LICENSE)
[![Version](https://img.shields.io/badge/version-1.0.0-informational.svg)](CHANGELOG.md)
[![Toolkit](https://img.shields.io/badge/toolkit-GTK%203-success.svg)](#le-port-gtk3dialog)
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

gtk3dialog --program=MAIN_DIALOG     # → une fenêtre GTK 3 native s'ouvre
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

## Le port gtk3dialog

Un cœur C (grammaire flex/bison + automate + `safe_exec`) et un backend GTK 3.

| Port | Binaire | Toolkit | Widgets | Particularité |
|------|---------|---------|:------:|---------------|
| **gtk3dialog** | `gtk3dialog` | GTK 3 | 43 | **Référence** · alias rétro-compatible `gtkdialog` |

> **43 widgets** décrits par une seule grammaire. Le binaire fournit l'alias
> `gtkdialog` : les scripts écrits pour gtkdialog tournent sans modification.

---

## Installation

### Paquet Debian (.deb)

```sh
# Le .deb est joint à chaque release du dépôt, avec ses sommes de contrôle.
# Téléchargez les deux, puis :
sha256sum -c SHA256SUMS
sudo apt install ./haplo-dialog_1.0.0-8_amd64.deb
```

> **Le paquet s'appelle `haplo-dialog`, la commande reste `gtk3dialog`.**
> Le nom de paquet `gtk3dialog` n'est pas libre : BunsenLabs en distribue un
> sous ce nom depuis juillet 2025, dans son archive APT. Deux paquets qui
> portent le même nom ne sont pas des rivaux pour apt, ce sont deux versions
> d'un même paquet, et la plus haute écrase l'autre en silence. Les noms
> distincts font qu'apt refuse et explique, au lieu de substituer. Vos scripts
> ne changent pas : `/usr/bin/gtk3dialog` et son alias `gtkdialog` sont
> installés comme avant.

### Depuis les sources

```sh
# autotools
cd gtk3dialog/gtk3dialog_1.0.0 && autoreconf -fi && ./configure && make -j"$(nproc)" && sudo make install
```

Les recettes d'empaquetage sont fournies (`packaging/` : Debian, RPM, Arch,
Gentoo, Slackware). Voir [PACKAGING.md](PACKAGING.md).

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

- Manuels utilisateur/développeur (`gtk3dialog/gtk3dialog_1.0.0/MANUEL_*.md`).
- Pages de manuel `gtk3dialog(1)` et référence XML `haplo-dialog-xml(5)`.
- Manuels Texinfo (`gtk3dialog/gtk3dialog_1.0.0/doc/`).
- Site : <https://haplo-dialog.fr>.

---

## Contribuer & licence

Contributions bienvenues, voir [CONTRIBUTING.md](CONTRIBUTING.md) et
[CODE_OF_CONDUCT.md](CODE_OF_CONDUCT.md).

**GPL-2.0-or-later**, voir [LICENSE](LICENSE). Les exemples (`examples/`) sont en CC0.

---

## Développement assisté par IA - dit franchement

gtk3dialog a été écrit avec l'aide d'une IA (Claude, d'Anthropic) —
conception, code, tests, documentation, sous ma relecture et ma responsabilité.
Je le dis ouvertement : l'honnêteté ne se négocie pas. Je crois qu'un outil bien
employé, sans rien masquer, sert la qualité au lieu de lui nuire. Le code est à
moi ; les erreurs aussi.

> *haplo-dialog*
