#!/bin/bash
# SPDX-License-Identifier: CC0-1.0
# Métier   : Web local / intranet
# Besoin   : recueillir une demande de matériel dans un navigateur et l'ajouter au registre des demandes (CSV)
# Enrobe   : htmlsermo en mode serve (page sur 127.0.0.1) ; ajout d'une ligne au registre CSV
# Ports    : htmlsermo conseillé (SERMO_HTML_MODE=serve, posé par défaut) ; tout autre port en fenêtre
# Terminal : rien de perdu
# Usage    : ./1-demande-materiel.sh [binaire sermo]   (par exemple : ./1-demande-materiel.sh htmlsermo)
#            SERMO_A_BLANC=0 pour agir pour de vrai (défaut : la commande est affichée)
#            DEMANDES=fichier.csv pour changer de registre (défaut : ~/intranet/demandes-materiel.csv)
#            htmlsermo affiche l'adresse à ouvrir (http://127.0.0.1:…/jeton) sur la sortie d'erreur.

export SERMO="${SERMO:-sermo}"
export SERMO_A_BLANC="${SERMO_A_BLANC:-1}"
# Le script lui-même, pour --do : le shell de sermo lit la variable, sans
# citation à refaire (printf %q écrit $'…' en locale C, que dash lit de travers).
export SERMO_SCRIPT="$0"
export SERMO_HTML_MODE="${SERMO_HTML_MODE:-serve}"
export DEMANDES="${DEMANDES:-$HOME/intranet/demandes-materiel.csv}"

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
	local date=${1:-2026-10-20}
	cat <<XML
<window title="Demande de matériel" default-width="560">
  <vbox>
    <frame label="Demandeur">
      <vbox>
        <hbox><text><label>Nom et prénom :</label></text>
          <entry space-expand="true" space-fill="true"><variable>NOM</variable><default>Camille Martin</default></entry></hbox>
        <hbox><text><label>Service :</label></text>
          <comboboxtext><variable>SERVICE</variable><default>Comptabilité</default>
            <item>Accueil</item><item>Comptabilité</item><item>Direction</item>
            <item>Informatique</item><item>Ressources humaines</item><item>Technique</item>
          </comboboxtext></hbox>
      </vbox>
    </frame>
    <frame label="Matériel">
      <vbox>
        <hbox><text><label>Article :</label></text>
          <comboboxtext><variable>ARTICLE</variable><default>Écran 24 pouces</default>
            <item>Ordinateur portable</item><item>Écran 24 pouces</item><item>Clavier et souris</item>
            <item>Casque avec micro</item><item>Station d'accueil</item><item>Autre (préciser)</item>
          </comboboxtext>
          <text><label>Quantité :</label></text>
          <spinbutton range-min="1" range-max="20" range-step="1"><variable>QUANTITE</variable><default>1</default></spinbutton></hbox>
        <hbox>
          <radiobutton><label>Normal</label><variable>NORMAL</variable><default>true</default></radiobutton>
          <radiobutton><label>Urgent (poste bloqué)</label><variable>URGENT</variable></radiobutton>
        </hbox>
        <text><label>Date souhaitée :</label></text>
        <calendar><variable>DATE_SOUHAITEE</variable><default>$(xml "$date")</default></calendar>
        <text><label>Justification :</label></text>
        <edit height-request="70"><variable>JUSTIFICATION</variable><default>Second écran pour la clôture des comptes.</default></edit>
      </vbox>
    </frame>
    <hbox>
      <button cancel></button>
      <button><label>Envoyer la demande</label><action>exit:envoyer</action></button>
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
		[ "$EXIT" = envoyer ] || exit 0
		[ -n "$NOM" ] || { echo "Le nom est obligatoire." >&2; exit 1; }
		[[ $QUANTITE =~ ^[0-9]+$ ]] || { echo "Quantité refusée : $QUANTITE" >&2; exit 1; }
		[[ $DATE_SOUHAITEE =~ ^[0-9]{4}-[0-9]{2}-[0-9]{2}$ ]] || { echo "Date refusée : $DATE_SOUHAITEE" >&2; exit 1; }
		urgence=normal; [ "$URGENT" = true ] && urgence=urgent
		ligne="$(csv "$(date '+%F %T')");$(csv "$NOM");$(csv "$SERVICE");$(csv "$ARTICLE");$QUANTITE;$urgence;$DATE_SOUHAITEE;$(csv "$JUSTIFICATION")"
		entete='horodatage;nom;service;article;quantite;urgence;date_souhaitee;justification'
		# Le registre reçoit la ligne (et l'en-tête s'il est neuf).
		agir sh -c 'mkdir -p -- "$(dirname -- "$1")" && { [ -s "$1" ] || printf "%s\n" "$2"; printf "%s\n" "$3"; } >> "$1"' \
			demande "$DEMANDES" "$entete" "$ligne"
		if [ "$SERMO_A_BLANC" != 1 ]; then echo "Demande enregistrée pour $NOM : $QUANTITE × $ARTICLE ($urgence)."; fi
		;;
	esac
}

main() {
	if [ "${1:-}" = --suite ]; then suite "$2"; exit; fi
	[ -n "${1:-}" ] && SERMO=$1
	command -v "$SERMO" >/dev/null || { echo "$SERMO est absent : installez sermo (htmlsermo pour le mode navigateur)." >&2; exit 1; }
	export DIALOGUE; DIALOGUE=$(dialogue_principal "$(date -d '+7 days' +%F)")
	"$SERMO" --program=DIALOGUE --do='"$SERMO_SCRIPT" --suite principal' >/dev/null
}

[ "${BASH_SOURCE[0]}" = "$0" ] && main "$@"
