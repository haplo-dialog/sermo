#!/bin/bash
# SPDX-License-Identifier: CC0-1.0
# Métier   : Enseignement, formation (surveillance d'épreuve)
# Besoin   : chronométrer une épreuve (durée, tiers-temps, alerte avant la fin) et noter début et fin au procès-verbal
# Enrobe   : date (temps restant, lu par <timer>) ; tee -a (procès-verbal CSV)
# Ports    : gtk3sermo conseillé (vidéoprojecteur) ; qt6sermo ; htmlsermo ne joue pas les minuteurs
# Terminal : le temps restant se met à jour ; la barre devient un pourcentage
# Usage    : ./4-minuteur-examen.sh [binaire sermo]
#            SERMO_A_BLANC=0 pour agir pour de vrai (défaut : la commande est affichée)

export SERMO="${SERMO:-sermo}"
export SERMO_A_BLANC="${SERMO_A_BLANC:-1}"
# Le script lui-même, pour --do : le shell de sermo lit la variable, sans
# citation à refaire (printf %q écrit $'…' en locale C, que dash lit de travers).
export SERMO_SCRIPT="$0"

# Échappe une valeur avant de l'insérer dans le XML.
# (« \& » : depuis bash 5.2, un « & » nu dans le remplacement recopie le motif.)
xml() { local s=${1//&/"&amp;"}; s=${s//</"&lt;"}; s=${s//>/"&gt;"}; printf '%s' "${s//\"/"&quot;"}"; }

# Lance une commande, ou l'affiche seulement en mode à blanc.
agir() {
	if [ "$SERMO_A_BLANC" = 1 ]; then
		printf 'à blanc :'; printf ' %q' "$@"; printf '\n'
	else
		"$@"
	fi
}

# Une ligne CSV : champs entre guillemets (guillemets doublés), séparés par « ; ».
ligne_csv() {
	local sep="" c
	for c in "$@"; do c=${c//$'\n'/ }; printf '%s"%s"' "$sep" "${c//\"/\"\"}"; sep=";"; done
	printf '\n'
}

dialogue_principal() {
	cat <<XML
<window title="Minuteur d'examen" default-width="500">
  <vbox>
    <frame label="Épreuve">
      <vbox>
        <hbox><text><label>Intitulé :</label></text>
          <entry space-expand="true" space-fill="true"><variable>EPREUVE</variable><default>Mathématiques, devoir commun</default></entry></hbox>
        <hbox><text><label>Salle :</label></text>
          <entry space-expand="true" space-fill="true"><variable>SALLE</variable><default>B204</default></entry></hbox>
        <hbox><text><label>Durée (minutes) :</label></text>
          <spinbutton range-min="5" range-max="360" range-step="5"><variable>DUREE</variable><default>120</default></spinbutton></hbox>
        <checkbox><label>Tiers-temps (durée majorée d'un tiers)</label><variable>TIERS_TEMPS</variable></checkbox>
      </vbox>
    </frame>
    <frame label="Alerte avant la fin (minutes)">
      <vbox>
        <hscale range-min="0" range-max="30" range-step="5" draw-value="true">
          <variable>ALERTE</variable><default>10</default>
        </hscale>
      </vbox>
    </frame>
    <checkbox><label>Noter début et fin au procès-verbal</label><variable>PV</variable><default>true</default></checkbox>
    <hbox>
      <button cancel></button>
      <button><label>Distribuer les sujets et démarrer</label><action>exit:demarrer</action></button>
    </hbox>
  </vbox>
</window>
XML
}

# Écran du minuteur. $1 épreuve, $2 début (secondes depuis 1970), $3 durée (s),
# $4 heure de fin « 10 h 30 », $5 alerte (s). Les <input> ne font que lire l'horloge.
# Balises Pango et <input> sont entre guillemets : sermo y garde « < » et « & » tels
# quels (il ne décode pas « &lt; »). Pango, lui, décode les « &amp; » posés par xml.
dialogue_minuteur() {
	local epreuve=${1:-Mathématiques, devoir commun} debut=${2:-1791792000} duree=${3:-7200} fin=${4:-10 h 00} alerte=${5:-600}
	cat <<XML
<window title="Épreuve en cours" default-width="520">
  <vbox>
    <text use-markup="true"><label>"<big><b>$(xml "$epreuve")</b></big>"</label></text>
    <text><label>Fin de l'épreuve à $(xml "$fin")</label></text>
    <text use-markup="true">
      <variable>RESTE</variable>
      <input>"r=\$(( $debut + $duree - \$(date +%s) )); [ \$r -lt 0 ] && r=0; m=\$(( r / 60 ))
if [ \$r -gt 0 ] && [ \$r -le $alerte ]; then printf '<big><b>Plus que %d min</b></big>' \$(( (r + 59) / 60 ))
elif [ \$r -eq 0 ]; then printf '<big><b>Posez les stylos</b></big>'
else printf '<big>Reste %d h %02d</big>' \$((m / 60)) \$((m % 60)); fi"</input>
    </text>
    <progressbar>
      <label>Temps écoulé</label>
      <input>"while :; do p=\$(( (\$(date +%s) - $debut) * 100 / $duree )); [ \$p -gt 100 ] && p=100
echo \$p; [ \$p -ge 100 ] && break; sleep 15; done"</input>
    </progressbar>
    <timer interval="10" visible="false"><action>refresh:RESTE</action></timer>
    <hbox>
      <button><label>Clore l'épreuve</label><action>exit:clore</action></button>
    </hbox>
  </vbox>
</window>
XML
}

suite() {
	# La sortie standard de sermo part vers /dev/null (les lignes NOM=…) et
	# l'étape --do en hérite : écrire sur la sortie d'erreur, qui reste visible.
	exec >&2
	case "$1" in
	principal)
		[ "$EXIT" = demarrer ] || exit 0
		duree=$(( ${DUREE%.*} * 60 ))
		[ "$TIERS_TEMPS" = true ] && duree=$(( duree * 4 / 3 ))
		debut=$(date +%s)
		fin=$(date -d "@$((debut + duree))" '+%H h %M')
		export EPREUVE SALLE PV DEBUT=$debut FIN_PREVUE=$fin DUREE_S=$duree
		export DIALOGUE; DIALOGUE=$(dialogue_minuteur "$EPREUVE" "$debut" "$duree" "$fin" "$(( ${ALERTE%.*} * 60 ))")
		"$SERMO" --program=DIALOGUE --do='"$SERMO_SCRIPT" --suite minuteur' >/dev/null
		;;
	minuteur)
		# Fermer la fenêtre ou la clore : dans les deux cas l'épreuve est finie.
		maintenant=$(date +%s)
		echo "Épreuve « $EPREUVE » : $(( (maintenant - DEBUT) / 60 )) min écoulées (fin prévue à $FIN_PREVUE)."
		[ "$PV" = true ] || exit 0
		f="$HOME/Documents/Examens/proces-verbal.csv"
		contenu=""
		[ -s "$f" ] || contenu=$(ligne_csv Épreuve Salle Début "Fin prévue" "Fin réelle" "Durée accordée (min)")$'\n'
		contenu+=$(ligne_csv "$EPREUVE" "$SALLE" "$(date -d "@$DEBUT" '+%F %H:%M')" "$FIN_PREVUE" "$(date -d "@$maintenant" '+%H h %M')" "$((DUREE_S / 60))")
		[ -d "${f%/*}" ] || agir mkdir -p -- "${f%/*}"
		printf 'Ajout au procès-verbal %s :\n' "$f"
		# À blanc, tee ne lit rien : les lignes sont montrées ici. Pour de vrai, tee les recopie.
		[ "$SERMO_A_BLANC" = 1 ] && printf '%s\n' "$contenu"
		printf '%s\n' "$contenu" | agir tee -a -- "$f"
		;;
	esac
}

main() {
	if [ "${1:-}" = --suite ]; then suite "$2"; exit; fi
	[ -n "${1:-}" ] && SERMO=$1
	for c in date tee; do
		command -v "$c" >/dev/null || { echo "$c est absent : installez le paquet coreutils." >&2; exit 1; }
	done
	export DIALOGUE; DIALOGUE=$(dialogue_principal)
	"$SERMO" --program=DIALOGUE --do='"$SERMO_SCRIPT" --suite principal' >/dev/null
}

[ "${BASH_SOURCE[0]}" = "$0" ] && main "$@"
