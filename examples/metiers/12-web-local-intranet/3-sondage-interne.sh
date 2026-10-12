#!/bin/bash
# SPDX-License-Identifier: CC0-1.0
# Métier   : Web local / intranet
# Besoin   : faire remplir un sondage interne anonyme dans un navigateur et cumuler les réponses dans un CSV
# Enrobe   : htmlsermo en mode serve (page sur 127.0.0.1) ; ajout d'une ligne au fichier des réponses
# Ports    : htmlsermo conseillé (SERMO_HTML_MODE=serve, posé par défaut) ; tout autre port en fenêtre
# Terminal : rien de perdu
# Usage    : ./3-sondage-interne.sh [binaire sermo]   (par exemple : ./3-sondage-interne.sh htmlsermo)
#            SERMO_A_BLANC=0 pour agir pour de vrai (défaut : la commande est affichée)
#            REPONSES=fichier.csv pour changer de fichier (défaut : ~/intranet/sondage-teletravail.csv)

export SERMO="${SERMO:-sermo}"
export SERMO_A_BLANC="${SERMO_A_BLANC:-1}"
# Le script lui-même, pour --do : le shell de sermo lit la variable, sans
# citation à refaire (printf %q écrit $'…' en locale C, que dash lit de travers).
export SERMO_SCRIPT="$0"
export SERMO_HTML_MODE="${SERMO_HTML_MODE:-serve}"
export REPONSES="${REPONSES:-$HOME/intranet/sondage-teletravail.csv}"

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
	local sujet=${1:-Organisation du télétravail}
	cat <<XML
<window title="Sondage interne" default-width="560">
  <vbox>
    <text><label>Sondage anonyme : $(xml "$sujet"). Aucun nom n'est enregistré.</label></text>
    <frame label="1. Votre service">
      <comboboxtext><variable>SERVICE</variable><default>Technique</default>
        <item>Accueil</item><item>Comptabilité</item><item>Direction</item>
        <item>Informatique</item><item>Ressources humaines</item><item>Technique</item>
      </comboboxtext>
    </frame>
    <frame label="2. Satisfaction globale (0 à 10)">
      <hscale range-min="0" range-max="10" range-step="1" draw-value="true"><variable>NOTE</variable><default>7</default></hscale>
    </frame>
    <frame label="3. Jours de télétravail souhaités par semaine">
      <hbox>
        <radiobutton><label>Aucun</label><variable>JOURS_0</variable></radiobutton>
        <radiobutton><label>Un</label><variable>JOURS_1</variable></radiobutton>
        <radiobutton><label>Deux</label><variable>JOURS_2</variable><default>true</default></radiobutton>
        <radiobutton><label>Trois ou plus</label><variable>JOURS_3</variable></radiobutton>
      </hbox>
    </frame>
    <frame label="4. Ce qui vous manque (plusieurs choix)">
      <vbox>
        <checkbox><label>Un second écran à domicile</label><variable>BESOIN_ECRAN</variable></checkbox>
        <checkbox><label>Un meilleur accès au réseau interne (VPN)</label><variable>BESOIN_VPN</variable><default>true</default></checkbox>
        <checkbox><label>Des réunions mieux équipées (son, caméra)</label><variable>BESOIN_REUNION</variable></checkbox>
      </vbox>
    </frame>
    <frame label="5. Remarque libre">
      <edit height-request="70"><variable>REMARQUE</variable><default></default></edit>
    </frame>
    <hbox>
      <button cancel></button>
      <button><label>Envoyer mes réponses</label><action>exit:repondre</action></button>
    </hbox>
  </vbox>
</window>
XML
}

# Un champ CSV : guillemets doublés, sauts de ligne remplacés par des espaces.
csv() { local s=${1//$'\r'/}; s=${s//$'\n'/ }; printf '"%s"' "${s//\"/\"\"}"; }

suite() {
	# La sortie standard de sermo part vers /dev/null : écrire sur la sortie d'erreur.
	exec >&2
	case "$1" in
	principal)
		[ "$EXIT" = repondre ] || exit 0
		[[ $NOTE =~ ^([0-9]|10)$ ]] || { echo "Note refusée : $NOTE" >&2; exit 1; }
		jours=2
		for j in 0 1 2 3; do v=JOURS_$j; [ "${!v}" = true ] && jours=$j; done
		b() { [ "$1" = true ] && echo 1 || echo 0; }
		# Pas d'heure précise : la date seule, pour ne pas désanonymiser par l'horodatage.
		ligne="$(date +%F);$(csv "$SERVICE");$NOTE;$jours;$(b "$BESOIN_ECRAN");$(b "$BESOIN_VPN");$(b "$BESOIN_REUNION");$(csv "$REMARQUE")"
		entete='date;service;note;jours_teletravail;besoin_ecran;besoin_vpn;besoin_reunion;remarque'
		agir sh -c 'mkdir -p -- "$(dirname -- "$1")" && { [ -s "$1" ] || printf "%s\n" "$2"; printf "%s\n" "$3"; } >> "$1"' \
			sondage "$REPONSES" "$entete" "$ligne"
		if [ "$SERMO_A_BLANC" != 1 ]; then echo "Merci : réponse enregistrée."; fi
		;;
	esac
}

main() {
	if [ "${1:-}" = --suite ]; then suite "$2"; exit; fi
	[ -n "${1:-}" ] && SERMO=$1
	command -v "$SERMO" >/dev/null || { echo "$SERMO est absent : installez sermo (htmlsermo pour le mode navigateur)." >&2; exit 1; }
	export DIALOGUE; DIALOGUE=$(dialogue_principal)
	"$SERMO" --program=DIALOGUE --do='"$SERMO_SCRIPT" --suite principal' >/dev/null
}

[ "${BASH_SOURCE[0]}" = "$0" ] && main "$@"
