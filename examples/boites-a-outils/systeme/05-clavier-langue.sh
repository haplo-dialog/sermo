#!/bin/bash
# SPDX-License-Identifier: CC0-1.0
# Boîte    : Système
# Besoin   : changer la disposition du clavier (AZERTY, BÉPO, QWERTY…), sa variante et ses options, et la langue du poste
# Enrobe   : setxkbmap (la session en cours) ; localectl (tout le poste, droits d'administration)
# Ports    : gtk3sermo ou qt6sermo conseillés
# Terminal : sans objet pour setxkbmap (session X) ; la partie localectl marche aussi par SSH
# Usage    : ./05-clavier-langue.sh [binaire sermo]
#            SERMO_A_BLANC=0 pour agir pour de vrai (défaut : la commande est affichée)
#            « Tout le poste » demande les droits : le script affiche alors la commande.
#            Sous Wayland, setxkbmap ne règle rien : passer par les réglages du compositeur.

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

# Options XKB proposées : « option|libellé ».
OPTIONS_XKB=(
	"compose:ralt|Touche Compose sur Alt Gr droite"
	"caps:escape|Verr. Maj devient Échap"
	"ctrl:nocaps|Verr. Maj devient Ctrl"
	"grp:alt_shift_toggle|Alt+Maj passe à la disposition suivante"
	"terminate:ctrl_alt_bksp|Ctrl+Alt+Retour arrière ferme la session X")

# Arguments : disposition, variante, options (séparées par des virgules), langue,
# puis la liste des langues proposées.
dialogue_principal() {
	local disposition=${1:-fr} variante=${2-latin9} options=${3-compose:ralt} langue=${4:-fr_FR.UTF-8} l i=0
	shift 4 2>/dev/null || set --
	local langues=("$@")
	[ ${#langues[@]} -gt 0 ] || langues=(de_DE.UTF-8 en_GB.UTF-8 en_US.UTF-8 es_ES.UTF-8 fr_BE.UTF-8 fr_CA.UTF-8 fr_CH.UTF-8 fr_FR.UTF-8)
	cat <<XML
<window title="Clavier et langue" default-width="560">
  <vbox>
    <frame label="Disposition">
      <vbox>
        <hbox><text><label>Disposition :</label></text>
          <comboboxtext><variable>DISPOSITION</variable><default>$(xml "$disposition")</default>
            <item>fr</item><item>be</item><item>ch</item><item>ca</item><item>us</item><item>gb</item><item>de</item><item>es</item><item>it</item>
          </comboboxtext>
          <text><label>Variante (vide = standard) :</label></text>
          <entry><variable>VARIANTE</variable><default>$(xml "$variante")</default></entry></hbox>
        <text><label>Exemples de variantes pour fr : latin9, oss, bepo, afnor, azerty, nodeadkeys, mac.</label></text>
      </vbox>
    </frame>
    <expander label="Options du clavier" expanded="true">
      <vbox>
XML
	for l in "${OPTIONS_XKB[@]}"; do
		i=$((i + 1))
		local coche=false
		[[ ",$options," == *",${l%%|*},"* ]] && coche=true
		printf '        <checkbox><label>%s</label><variable>OPT_%d</variable><default>%s</default></checkbox>\n' \
			"$(xml "${l#*|}")" "$i" "$coche"
	done
	cat <<XML
      </vbox>
      <variable>OPTIONS_OUVERTES</variable>
    </expander>
    <frame label="Langue du poste (menus, dates, nombres)">
      <hbox><text><label>Langue :</label></text>
        <comboboxtext><variable>LANGUE</variable><default>$(xml "$langue")</default>
XML
	for l in "${langues[@]}"; do printf '          <item>%s</item>\n' "$(xml "$l")"; done
	cat <<XML
        </comboboxtext></hbox>
    </frame>
    <frame label="Portée">
      <vbox>
        <radiobutton><label>Cette session seulement (clavier ; la langue n'est pas touchée)</label><default>true</default><variable>PORTEE_SESSION</variable></radiobutton>
        <radiobutton><label>Tout le poste, durablement (clavier et langue ; droits d'administration)</label><variable>PORTEE_POSTE</variable></radiobutton>
      </vbox>
    </frame>
    <hbox><text><label>Essayez ici après coup :</label></text>
      <entry space-expand="true" space-fill="true"><variable>ESSAI</variable><default>àéèùç €@#</default></entry></hbox>
    <hbox>
      <button cancel></button>
      <button><label>Appliquer</label><action>exit:appliquer</action></button>
    </hbox>
  </vbox>
</window>
XML
}

# Options cochées, séparées par des virgules.
options_cochees() {
	local i=0 l v liste=()
	for l in "${OPTIONS_XKB[@]}"; do
		i=$((i + 1)); v="OPT_$i"
		[ "${!v:-}" = true ] && liste+=("${l%%|*}")
	done
	(IFS=,; echo "${liste[*]}")
}

suite() {
	# La sortie standard de sermo part vers /dev/null : écrire sur la sortie d'erreur.
	exec >&2
	case "$1" in
	principal)
		[ "$EXIT" = appliquer ] || exit 0
		[[ $DISPOSITION =~ ^(fr|be|ch|ca|us|gb|de|es|it)$ ]] || { echo "Disposition refusée : $DISPOSITION" >&2; exit 1; }
		if [ -n "$VARIANTE" ]; then
			[[ $VARIANTE =~ ^[a-z0-9_]+$ ]] || { echo "Variante refusée : $VARIANTE" >&2; exit 1; }
			if command -v localectl >/dev/null; then
				grep -qxF -- "$VARIANTE" < <(localectl list-x11-keymap-variants "$DISPOSITION" 2>/dev/null) ||
					{ echo "Variante « $VARIANTE » inconnue pour la disposition $DISPOSITION." >&2; exit 1; }
			fi
		fi
		local opts; opts=$(options_cochees)
		if [ "$PORTEE_POSTE" = true ]; then
			[[ $LANGUE =~ ^[A-Za-z]{2,3}_[A-Z]{2}(\.UTF-8)?(@[a-z]+)?$ ]] || { echo "Langue refusée : $LANGUE" >&2; exit 1; }
			echo "À lancer en administrateur (root) :"
			printf 'localectl set-x11-keymap %q pc105 %q %q\n' "$DISPOSITION" "$VARIANTE" "$opts"
			printf 'localectl set-locale %q\n' "LANG=$LANGUE"
			echo "La langue prend effet à la prochaine ouverture de session."
		else
			[ "${XDG_SESSION_TYPE:-}" = wayland ] && echo "Session Wayland : setxkbmap ne change rien ici ; passez par les réglages du compositeur."
			# « -option "" » efface d'abord les options en place.
			local cmd=(setxkbmap -layout "$DISPOSITION")
			[ -n "$VARIANTE" ] && cmd+=(-variant "$VARIANTE")
			cmd+=(-option "")
			[ -n "$opts" ] && cmd+=(-option "$opts")
			agir "${cmd[@]}"
		fi
		;;
	esac
}

main() {
	if [ "${1:-}" = --suite ]; then suite "$2"; exit; fi
	[ -n "${1:-}" ] && SERMO=$1
	command -v setxkbmap >/dev/null || { echo "setxkbmap est absent : installez le paquet x11-xkb-utils." >&2; exit 1; }
	local disposition variante options langue langues=()
	# setxkbmap -query : « layout:     fr », « variant:    latin9 », « options:    compose:ralt ».
	disposition=$(setxkbmap -query 2>/dev/null | awk '$1 == "layout:" { print $2 }')
	variante=$(setxkbmap -query 2>/dev/null | awk '$1 == "variant:" { print $2 }')
	options=$(setxkbmap -query 2>/dev/null | awk '$1 == "options:" { print $2 }')
	disposition=${disposition%%,*}; variante=${variante%%,*}
	langue=${LANG:-C.UTF-8}
	command -v localectl >/dev/null && mapfile -t langues < <(localectl list-locales 2>/dev/null)
	export DIALOGUE; DIALOGUE=$(dialogue_principal "${disposition:-fr}" "$variante" "$options" "$langue" "${langues[@]}")
	"$SERMO" --program=DIALOGUE --do='"$SERMO_SCRIPT" --suite principal' >/dev/null
}

[ "${BASH_SOURCE[0]}" = "$0" ] && main "$@"
