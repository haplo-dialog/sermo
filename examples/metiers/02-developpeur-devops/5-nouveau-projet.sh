#!/bin/bash
# SPDX-License-Identifier: CC0-1.0
# Métier   : Développeur / DevOps
# Besoin   : créer l'arborescence d'un nouveau projet (langage, licence, dépôt git)
# Enrobe   : mkdir, install, cp, git init
# Ports    : gtk3sermo conseillé ; ncursessermo par SSH
# Terminal : rien de perdu
# Usage    : ./5-nouveau-projet.sh [binaire sermo]
#            SERMO_A_BLANC=0 pour agir pour de vrai (défaut : la commande est affichée)

export SERMO="${SERMO:-sermo}"
export SERMO_A_BLANC="${SERMO_A_BLANC:-1}"
# Le script lui-même, pour --do : le shell de sermo lit la variable, sans
# citation à refaire (printf %q écrit $'…' en locale C, que dash lit de travers).
export SERMO_SCRIPT="$0"

# Échappe une valeur avant de l'insérer dans le XML.
xml() { local s=${1//&/"&amp;"}; s=${s//</"&lt;"}; s=${s//>/"&gt;"}; printf '%s' "${s//\"/"&quot;"}"; }

# Lance une commande, ou l'affiche seulement en mode à blanc.
agir() {
	if [ "$SERMO_A_BLANC" = 1 ]; then
		printf 'à blanc :'; printf ' %q' "$@"; printf '\n'
	else
		"$@"
	fi
}

# Arguments : auteur (git config user.name), dossier parent proposé.
dialogue_principal() {
	local auteur=${1:-Camille Dupont} parent=${2:-$HOME/projets}
	cat <<XML
<window title="Nouveau projet" default-width="500">
  <vbox>
    <frame label="Projet">
      <vbox>
        <hbox><text><label>Nom :</label></text>
          <entry space-expand="true" space-fill="true"><variable>NOM</variable><default>mon-outil</default></entry></hbox>
        <hbox><text><label>Dans le dossier :</label></text>
          <entry space-expand="true" space-fill="true"><variable>PARENT</variable><default>$(xml "$parent")</default></entry></hbox>
        <hbox><text><label>Auteur :</label></text>
          <entry space-expand="true" space-fill="true"><variable>AUTEUR</variable><default>$(xml "$auteur")</default></entry></hbox>
        <hbox><text><label>Langage :</label></text>
          <comboboxtext space-expand="true" space-fill="true">
            <variable>LANGAGE</variable>
            <default>C</default>
            <item>C</item>
            <item>Python</item>
            <item>Shell</item>
            <item>Rust</item>
          </comboboxtext></hbox>
      </vbox>
    </frame>
    <frame label="Licence">
      <vbox>
        <radiobutton><label>GPL-3.0-or-later (copyleft)</label><default>true</default><variable>LIC_GPL</variable></radiobutton>
        <radiobutton><label>MIT (permissive, courte)</label><variable>LIC_MIT</variable></radiobutton>
        <radiobutton><label>Apache-2.0 (permissive, clause de brevets)</label><variable>LIC_APACHE</variable></radiobutton>
        <radiobutton><label>CC0-1.0 (domaine public)</label><variable>LIC_CC0</variable></radiobutton>
      </vbox>
    </frame>
    <frame label="Options">
      <vbox>
        <checkbox><label>Créer un dépôt git (git init)</label><default>true</default><variable>GIT</variable></checkbox>
        <checkbox><label>Dossier tests/</label><default>true</default><variable>TESTS</variable></checkbox>
        <checkbox><label>Dossier docs/</label><variable>DOCS</variable></checkbox>
      </vbox>
    </frame>
    <hbox>
      <button cancel></button>
      <button><label>Créer le projet</label><action>exit:creer</action></button>
    </hbox>
  </vbox>
</window>
XML
}

# Écrit un fichier (contenu en argument) : passe par agir comme le reste.
ecrire() {
	local mode=$1 fichier=$2 contenu=$3
	agir install -D -m "$mode" /dev/stdin "$fichier" <<<"$contenu" || return 1
	# À blanc, on montre aussi le contenu qui serait écrit.
	[ "$SERMO_A_BLANC" = 1 ] && sed 's/^/    | /' <<<"$contenu"
	return 0
}

licence_mit() {
	cat <<EOF
MIT License

Copyright (c) $(date +%Y) $AUTEUR

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
EOF
}

creer() {
	local racine=$1 spdx=$2 annee; annee=$(date +%Y)
	local ent="SPDX-License-Identifier: $spdx"
	agir mkdir -p -- "$racine" || exit 1
	[ "$TESTS" = true ] && agir mkdir -p -- "$racine/tests"
	[ "$DOCS" = true ] && agir mkdir -p -- "$racine/docs"
	case "$LANGAGE" in
	C)
		ecrire 644 "$racine/src/main.c" "// $ent
// Copyright (c) $annee $AUTEUR
#include <stdio.h>

int main(void)
{
	puts(\"$NOM\");
	return 0;
}"
		ecrire 644 "$racine/Makefile" "# $ent
CFLAGS ?= -O2 -g -Wall -Wextra

$NOM: src/main.c
	\$(CC) \$(CFLAGS) -o \$@ \$<

clean:
	rm -f $NOM

.PHONY: clean"
		;;
	Python)
		local mod=${NOM//-/_}
		ecrire 644 "$racine/src/$mod/__init__.py" "# $ent
\"\"\"$NOM.\"\"\"
__version__ = \"0.1.0\""
		ecrire 644 "$racine/pyproject.toml" "[project]
name = \"$NOM\"
version = \"0.1.0\"
license = \"$spdx\"
authors = [{ name = \"$AUTEUR\" }]

[build-system]
requires = [\"setuptools>=69\"]
build-backend = \"setuptools.build_meta\""
		;;
	Shell)
		ecrire 755 "$racine/bin/$NOM" "#!/bin/bash
# $ent
# Copyright (c) $annee $AUTEUR
echo \"$NOM\""
		;;
	Rust)
		ecrire 644 "$racine/Cargo.toml" "[package]
name = \"$NOM\"
version = \"0.1.0\"
edition = \"2021\"
license = \"$spdx\"
authors = [\"$AUTEUR\"]"
		ecrire 644 "$racine/src/main.rs" "// $ent
fn main() {
    println!(\"$NOM\");
}"
		;;
	esac
	ecrire 644 "$racine/README.md" "# $NOM

Licence : $spdx. Auteur : $AUTEUR."
	# Le texte complet de la licence : celui que Debian fournit, sinon un renvoi SPDX.
	local commun=""
	case "$spdx" in
	GPL-3.0-or-later) commun=/usr/share/common-licenses/GPL-3 ;;
	Apache-2.0) commun=/usr/share/common-licenses/Apache-2.0 ;;
	CC0-1.0) commun=/usr/share/common-licenses/CC0-1.0 ;;
	esac
	if [ "$spdx" = MIT ]; then
		ecrire 644 "$racine/LICENSE" "$(licence_mit)"
	elif [ -r "$commun" ]; then
		agir cp -- "$commun" "$racine/LICENSE"
	else
		ecrire 644 "$racine/LICENSE" "$spdx : texte complet sur https://spdx.org/licenses/$spdx.html"
	fi
	[ "$GIT" = true ] && agir git init --quiet -- "$racine"
}

suite() {
	exec >&2
	case "$1" in
	principal)
		[ "$EXIT" = creer ] || exit 0
		[[ "$NOM" =~ ^[A-Za-z0-9][A-Za-z0-9._-]*$ ]] || { echo "Nom refusé : lettres, chiffres, . _ -." >&2; exit 1; }
		[[ "$AUTEUR" != *[\"\\$\`]* && -n "$AUTEUR" ]] || { echo "Auteur vide ou avec des caractères refusés." >&2; exit 1; }
		[ -n "$PARENT" ] || { echo "Dossier parent vide." >&2; exit 1; }
		case "$PARENT" in /*) ;; *) PARENT="$PWD/$PARENT" ;; esac
		local racine="${PARENT%/}/$NOM"
		[ -e "$racine" ] && { echo "Existe déjà : $racine" >&2; exit 1; }
		local spdx=GPL-3.0-or-later
		[ "$LIC_MIT" = true ] && spdx=MIT
		[ "$LIC_APACHE" = true ] && spdx=Apache-2.0
		[ "$LIC_CC0" = true ] && spdx=CC0-1.0
		[ "$GIT" = true ] && ! command -v git >/dev/null && { echo "git est absent : décochez git ou installez-le." >&2; exit 1; }
		creer "$racine" "$spdx"
		;;
	esac
}

main() {
	if [ "${1:-}" = --suite ]; then suite "$2"; exit; fi
	[ -n "${1:-}" ] && SERMO=$1
	command -v install >/dev/null || { echo "install est absent : installez coreutils." >&2; exit 1; }
	local auteur=""
	command -v git >/dev/null && auteur=$(git config user.name 2>/dev/null)
	export DIALOGUE; DIALOGUE=$(dialogue_principal "${auteur:-$USER}" "$HOME/projets")
	"$SERMO" --program=DIALOGUE --do='"$SERMO_SCRIPT" --suite principal' >/dev/null
}

[ "${BASH_SOURCE[0]}" = "$0" ] && main "$@"
