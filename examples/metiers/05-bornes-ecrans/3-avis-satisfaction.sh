#!/bin/bash
# SPDX-License-Identifier: CC0-1.0
# Métier   : Bornes et écrans d'accueil
# Besoin   : recueillir l'avis des visiteurs (note de 1 à 5 et commentaire) sur une borne
# Enrobe   : tee -a (avis en CSV, séparateur « ; »)
# Ports    : lvglsermo conseillé (écran tactile) ; gtk3sermo sur un poste
# Terminal : rien de perdu
# Usage    : ./3-avis-satisfaction.sh [binaire sermo]
#            SERMO_A_BLANC=0 pour agir pour de vrai (défaut : la commande est affichée)
#            BORNE_DONNEES : dossier des avis (défaut ~/borne) ; BORNE_BOUCLE=1 : formulaire en boucle

export SERMO="${SERMO:-sermo}"
export SERMO_A_BLANC="${SERMO_A_BLANC:-1}"
# Le script lui-même, pour --do : le shell de sermo lit la variable, sans
# citation à refaire (printf %q écrit $'…' en locale C, que dash lit de travers).
export SERMO_SCRIPT="$0"
export BORNE_DONNEES="${BORNE_DONNEES:-$HOME/borne}"
AVIS="$BORNE_DONNEES/avis.csv"
ENTETE='date;heure;service;note;commentaire;courriel_pour_reponse'

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

# Un champ CSV : entre guillemets, guillemets doublés, sans saut de ligne ; une
# valeur qui commence par = + - @ est neutralisée (elle serait une formule pour un tableur).
csv() {
	local s=${1//$'\n'/ }; s=${s//$'\r'/}
	case $s in [=+@-]*) s="'$s" ;; esac
	printf '"%s"' "${s//\"/\"\"}"
}

# Ajoute une ligne au fichier des avis (et l'en-tête si le fichier n'existe pas encore).
ajouter() {
	local f=$1 l=$2
	echo "Ligne : $l"
	[ -e "$f" ] || l=$ENTETE$'\n'$l
	agir mkdir -p -- "$(dirname -- "$f")"
	printf '%s\n' "$l" | agir tee -a -- "$f"
}

dialogue_principal() {
	cat <<XML
<window title="Votre avis" default-width="600">
  <vbox>
    <text><label>Votre avis nous aide à mieux vous accueillir. Il est anonyme.</label></text>
    <hbox>
      <text><label>Service rencontré :</label></text>
      <comboboxtext space-expand="true" space-fill="true"><variable>SERVICE</variable><default>Accueil</default>
        <item>Accueil</item><item>Caisse</item><item>Service après-vente</item><item>Conseil</item><item>Autre</item>
      </comboboxtext>
    </hbox>
    <frame label="Votre note : 1 = pas du tout satisfait, 5 = très satisfait">
      <hscale range-min="1" range-max="5" range-step="1" draw-value="true"><variable>NOTE</variable><default>4</default></hscale>
    </frame>
    <frame label="Un commentaire ? (facultatif)">
      <edit wrap-mode="word" height-request="110"><variable>COMMENTAIRE</variable></edit>
    </frame>
    <checkbox><label>Je souhaite une réponse (mon adresse ne sert qu'à cela)</label><variable>REPONSE</variable></checkbox>
    <hbox><text><label>Courriel :</label></text>
      <entry space-expand="true" space-fill="true"><variable>COURRIEL</variable></entry></hbox>
    <hbox>
      <button cancel></button>
      <button><label>Envoyer mon avis</label><action>exit:envoyer</action></button>
    </hbox>
  </vbox>
</window>
XML
}

dialogue_merci() {
	cat <<XML
<window title="Merci">
  <vbox>
    <text><label>Merci pour votre avis.</label></text>
    <timer visible="false" interval="6"><action>exit:fin</action></timer>
    <hbox>
      <button ok></button>
    </hbox>
  </vbox>
</window>
XML
}

suite() {
	# La sortie standard de sermo part vers /dev/null : écrire sur la sortie d'erreur.
	exec >&2
	case "$1" in
	principal)
		[ "$EXIT" = envoyer ] || exit 0
		[[ "$NOTE" =~ ^[1-5]$ ]] || { echo "Note illisible : $NOTE"; exit 1; }
		local courriel=""
		if [ "$REPONSE" = true ]; then
			[[ "$COURRIEL" =~ ^[^@\ ]+@[^@\ ]+\.[^@\ ]+$ ]] && courriel=$COURRIEL ||
				echo "Adresse de courriel illisible : l'avis est gardé sans elle."
		fi
		# Le commentaire est borné : une borne publique ne doit pas remplir le disque.
		local ligne
		ligne="$(date +%F);$(date +%H:%M);$(csv "$SERVICE");$NOTE;$(csv "${COMMENTAIRE:0:2000}");$(csv "$courriel")"
		ajouter "$AVIS" "$ligne"
		export DIALOGUE; DIALOGUE=$(dialogue_merci)
		"$SERMO" --program=DIALOGUE >/dev/null
		;;
	esac
}

main() {
	if [ "${1:-}" = --suite ]; then suite "$2"; exit; fi
	[ -n "${1:-}" ] && SERMO=$1
	for c in tee date; do
		command -v "$c" >/dev/null || { echo "$c est absent : installez coreutils." >&2; exit 1; }
	done
	while :; do
		export DIALOGUE; DIALOGUE=$(dialogue_principal)
		"$SERMO" --program=DIALOGUE --do='"$SERMO_SCRIPT" --suite principal' >/dev/null
		[ "${BORNE_BOUCLE:-0}" = 1 ] || break
	done
}

[ "${BASH_SOURCE[0]}" = "$0" ] && main "$@"
