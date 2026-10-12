#!/bin/bash
# SPDX-License-Identifier: CC0-1.0
# Métier   : Sécurité informatique
# Besoin   : préparer une analyse réseau (cible, type, ports, rythme) et relire la commande avant de la lancer
# Enrobe   : nmap
# Ports    : gtk3sermo conseillé ; ncursessermo par SSH
# Terminal : rien de perdu
# Usage    : ./1-perimetre-analyse.sh [binaire sermo]
#            SERMO_A_BLANC=0 pour agir pour de vrai (défaut : la commande est affichée)
#            N'analysez que des réseaux pour lesquels vous avez une autorisation écrite.

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

dialogue_principal() {
	local cible=${1:-192.168.1.0/24}
	cat <<XML
<window title="Analyse de périmètre" default-width="560">
  <vbox>
    <text><label>N'analysez que des réseaux pour lesquels vous avez une autorisation écrite.</label></text>
    <frame label="Cible">
      <vbox>
        <hbox><text><label>Adresse, plage ou nom :</label></text>
          <entry space-expand="true" space-fill="true"><variable>CIBLE</variable><default>$(xml "$cible")</default></entry></hbox>
        <hbox><text><label>Adresses à exclure (séparées par des virgules) :</label></text>
          <entry space-expand="true" space-fill="true"><variable>EXCLURE</variable><default>192.168.1.1</default></entry></hbox>
      </vbox>
    </frame>
    <frame label="Type d'analyse">
      <vbox>
        <radiobutton><label>Découverte des machines seulement (-sn)</label><variable>TYPE_DECOUVERTE</variable><default>true</default></radiobutton>
        <radiobutton><label>Ports TCP ouverts, connexion complète (-sT)</label><variable>TYPE_PORTS</variable></radiobutton>
        <radiobutton><label>Ports TCP et versions des services (-sT -sV)</label><variable>TYPE_VERSIONS</variable></radiobutton>
      </vbox>
    </frame>
    <frame label="Réglages">
      <vbox>
        <hbox><text><label>Ports analysés :</label></text>
          <comboboxtext><variable>PORTS</variable><default>les 100 plus courants</default>
            <item>les 100 plus courants</item>
            <item>les 1000 plus courants</item>
            <item>tous (1-65535)</item>
          </comboboxtext></hbox>
        <hbox><text><label>Rythme (0 discret, 5 agressif) :</label></text>
          <spinbutton range-min="0" range-max="5" range-step="1"><variable>RYTHME</variable><default>3</default></spinbutton></hbox>
        <checkbox><label>Ne pas tester la présence avant (-Pn)</label><variable>SANS_PING</variable></checkbox>
        <checkbox><label>Enregistrer le résultat (-oN)</label><variable>ENREGISTRER</variable><default>true</default></checkbox>
      </vbox>
    </frame>
    <hbox>
      <button cancel></button>
      <button><label>Préparer l'analyse</label><action>exit:analyser</action></button>
    </hbox>
  </vbox>
</window>
XML
}

# Une cible nmap ne contient que lettres, chiffres, points, deux-points, tirets,
# barres obliques et virgules ; jamais de tiret en tête (pris pour une option).
cible_valide() { [[ $1 =~ ^[A-Za-z0-9.:/,-]+$ && $1 != -* ]]; }

# Arguments nmap choisis, un par ligne.
options() {
	if [ "$TYPE_DECOUVERTE" = true ]; then
		echo -sn
	else
		echo -sT
		[ "$TYPE_VERSIONS" = true ] && echo -sV
		case "$PORTS" in
		"les 1000 plus courants") echo --top-ports; echo 1000 ;;
		"tous (1-65535)")         echo -p-; ;;
		*)                        echo --top-ports; echo 100 ;;
		esac
	fi
	echo "-T$RYTHME"
	[ "$SANS_PING" = true ] && echo -Pn
	[ -n "$EXCLURE" ] && { echo --exclude; echo "$EXCLURE"; }
	[ "$ENREGISTRER" = true ] && { echo -oN; echo "$HOME/analyse-$(date +%Y%m%d-%H%M).txt"; }
}

suite() {
	# La sortie standard de sermo part vers /dev/null : écrire sur la sortie d'erreur.
	exec >&2
	case "$1" in
	principal)
		[ "$EXIT" = analyser ] || exit 0
		cible_valide "$CIBLE" || { echo "Cible refusée : $CIBLE" >&2; exit 1; }
		[ -z "$EXCLURE" ] || cible_valide "$EXCLURE" || { echo "Exclusion refusée : $EXCLURE" >&2; exit 1; }
		[[ $RYTHME =~ ^[0-5]$ ]] || RYTHME=3
		mapfile -t opts < <(options)
		agir nmap "${opts[@]}" -- "$CIBLE"
		;;
	esac
}

main() {
	if [ "${1:-}" = --suite ]; then suite "$2"; exit; fi
	[ -n "${1:-}" ] && SERMO=$1
	command -v nmap >/dev/null || { echo "nmap est absent : installez le paquet nmap." >&2; exit 1; }
	export DIALOGUE; DIALOGUE=$(dialogue_principal)
	"$SERMO" --program=DIALOGUE --do='"$SERMO_SCRIPT" --suite principal' >/dev/null
}

[ "${BASH_SOURCE[0]}" = "$0" ] && main "$@"
