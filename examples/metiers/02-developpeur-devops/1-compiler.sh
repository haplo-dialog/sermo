#!/bin/bash
# SPDX-License-Identifier: CC0-1.0
# Métier   : Développeur / DevOps
# Besoin   : compiler un projet : cible, nombre de tâches, mode, nettoyage éventuel
# Enrobe   : make, cmake --build
# Ports    : gtk3sermo conseillé ; ncursessermo par SSH
# Terminal : rien de perdu
# Usage    : ./1-compiler.sh [binaire sermo]
#            SERMO_A_BLANC=0 pour agir pour de vrai (défaut : la commande est affichée)
#            Le dossier proposé est le dossier courant.

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

# Arguments : dossier du projet, nombre de processeurs.
dialogue_principal() {
	local dossier=${1:-$HOME/src/mon-projet} taches=${2:-4}
	cat <<XML
<window title="Compiler" default-width="520">
  <vbox>
    <frame label="Projet">
      <vbox>
        <hbox><text><label>Dossier :</label></text>
          <entry space-expand="true" space-fill="true"><variable>DOSSIER</variable><default>$(xml "$dossier")</default></entry></hbox>
        <hbox><text><label>Outil :</label></text>
          <comboboxtext space-expand="true" space-fill="true">
            <variable>OUTIL</variable>
            <default>automatique</default>
            <item>automatique</item>
            <item>make</item>
            <item>cmake</item>
          </comboboxtext></hbox>
        <hbox><text><label>Cible :</label></text>
          <entry space-expand="true" space-fill="true"><variable>CIBLE</variable><default>all</default></entry></hbox>
      </vbox>
    </frame>
    <frame label="Réglages">
      <vbox>
        <hbox><text><label>Tâches en parallèle (-j) :</label></text>
          <spinbutton range-min="1" range-max="64" range-step="1"><variable>TACHES</variable><default>$(xml "$taches")</default></spinbutton></hbox>
        <hbox><text><label>Mode (cmake) :</label></text>
          <radiobutton><label>Débogage</label><variable>MODE_DEBUG</variable></radiobutton>
          <radiobutton><label>Optimisé</label><default>true</default><variable>MODE_RELEASE</variable></radiobutton></hbox>
        <checkbox><label>Continuer après une erreur (-k)</label><variable>CONTINUER</variable></checkbox>
        <checkbox><label>Nettoyer avant de compiler (cible clean)</label><variable>NETTOYER</variable></checkbox>
      </vbox>
    </frame>
    <hbox>
      <button cancel></button>
      <button><label>Compiler</label><action>exit:compiler</action></button>
    </hbox>
  </vbox>
</window>
XML
}

dialogue_confirmation() {
	local dossier=${1:-$HOME/src/mon-projet}
	cat <<XML
<window title="Nettoyer d'abord ?">
  <vbox>
    <text><label>Tous les fichiers produits dans $(xml "$dossier") seront effacés,</label></text>
    <text><label>puis tout sera recompilé.</label></text>
    <hbox>
      <button cancel></button>
      <button><label>Nettoyer et compiler</label><action>exit:compiler</action></button>
    </hbox>
  </vbox>
</window>
XML
}

# Outil réel : celui choisi, ou deviné d'après les fichiers du projet.
outil() {
	case "$OUTIL" in
	make|cmake) echo "$OUTIL" ;;
	*)
		if [ -f "$DOSSIER/CMakeLists.txt" ]; then echo cmake
		elif [ -f "$DOSSIER/Makefile" ] || [ -f "$DOSSIER/makefile" ] || [ -f "$DOSSIER/GNUmakefile" ]; then echo make
		fi ;;
	esac
}

compiler() {
	local o; o=$(outil)
	command -v "${o:-make}" >/dev/null || { echo "${o:-make} est absent." >&2; exit 1; }
	case "$o" in
	cmake)
		local type=Release; [ "$MODE_DEBUG" = true ] && type=Debug
		local build="$DOSSIER/build-${type,,}"
		local fin=()
		if [ "$CONTINUER" = true ]; then
			if [ -f "$build/build.ninja" ]; then fin=(-- -k 0); else fin=(-- -k); fi
		fi
		agir cmake -S "$DOSSIER" -B "$build" -DCMAKE_BUILD_TYPE="$type" || exit 1
		[ "$NETTOYER" = true ] && { agir cmake --build "$build" --target clean || exit 1; }
		agir cmake --build "$build" --target "$CIBLE" --parallel "$TACHES" "${fin[@]}"
		;;
	make)
		local opts=(-C "$DOSSIER" -j "$TACHES")
		[ "$CONTINUER" = true ] && opts+=(-k)
		[ "$NETTOYER" = true ] && { agir make -C "$DOSSIER" clean || exit 1; }
		agir make "${opts[@]}" "$CIBLE"
		;;
	*) echo "Ni CMakeLists.txt ni Makefile dans $DOSSIER : choisissez l'outil." >&2; exit 1 ;;
	esac
}

suite() {
	exec >&2
	case "$1" in
	principal)
		[ "$EXIT" = compiler ] || exit 0
		[ -d "$DOSSIER" ] || { echo "Dossier introuvable : $DOSSIER" >&2; exit 1; }
		[[ "$CIBLE" =~ ^[A-Za-z0-9_./+-]+$ && "$CIBLE" != -* ]] || { echo "Cible refusée : $CIBLE" >&2; exit 1; }
		[[ "$TACHES" =~ ^[0-9]+$ && "$TACHES" -ge 1 ]] || { echo "Nombre de tâches illisible : $TACHES" >&2; exit 1; }
		if [ "$NETTOYER" = true ]; then
			export DOSSIER OUTIL CIBLE TACHES MODE_DEBUG MODE_RELEASE CONTINUER NETTOYER
			export DIALOGUE; DIALOGUE=$(dialogue_confirmation "$DOSSIER")
			"$SERMO" --program=DIALOGUE --do='"$SERMO_SCRIPT" --suite confirmation' >/dev/null
		else
			compiler
		fi
		;;
	confirmation)
		[ "$EXIT" = compiler ] || exit 0
		compiler
		;;
	esac
}

main() {
	if [ "${1:-}" = --suite ]; then suite "$2"; exit; fi
	[ -n "${1:-}" ] && SERMO=$1
	command -v make >/dev/null || command -v cmake >/dev/null ||
		{ echo "Ni make ni cmake : installez build-essential ou cmake." >&2; exit 1; }
	local n; n=$(nproc 2>/dev/null || echo 2)
	export DIALOGUE; DIALOGUE=$(dialogue_principal "$PWD" "$n")
	"$SERMO" --program=DIALOGUE --do='"$SERMO_SCRIPT" --suite principal' >/dev/null
}

[ "${BASH_SOURCE[0]}" = "$0" ] && main "$@"
