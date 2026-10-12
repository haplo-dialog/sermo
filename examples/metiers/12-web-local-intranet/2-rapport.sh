#!/bin/bash
# SPDX-License-Identifier: CC0-1.0
# Métier   : Web local / intranet
# Besoin   : publier sur l'intranet une page de rapport (erreurs du journal et occupation des disques) sur une période choisie
# Enrobe   : journalctl et df (lecture) ; htmlsermo en mode export pour écrire la page
# Ports    : gtk3sermo (ou tout port) pour les réglages ; htmlsermo en mode export pour la page
# Terminal : rien de perdu
# Usage    : ./2-rapport.sh [binaire sermo]
#            SERMO_A_BLANC=0 pour agir pour de vrai (défaut : la commande est affichée)
#            HTMLSERMO=chemin pour un htmlsermo hors du PATH.

export SERMO="${SERMO:-sermo}"
export SERMO_A_BLANC="${SERMO_A_BLANC:-1}"
# Le script lui-même, pour --do : le shell de sermo lit la variable, sans
# citation à refaire (printf %q écrit $'…' en locale C, que dash lit de travers).
export SERMO_SCRIPT="$0"
export HTMLSERMO="${HTMLSERMO:-htmlsermo}"

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
	local debut=${1:-2026-10-04}
	cat <<XML
<window title="Rapport pour l'intranet" default-width="520">
  <vbox>
    <hbox><text><label>Titre :</label></text>
      <entry space-expand="true" space-fill="true"><variable>TITRE</variable><default>Rapport hebdomadaire du serveur</default></entry></hbox>
    <frame label="Période : depuis le">
      <calendar><variable>DEBUT</variable><default>$(xml "$debut")</default></calendar>
    </frame>
    <frame label="Contenu">
      <vbox>
        <hbox><text><label>Gravité minimale :</label></text>
          <comboboxtext><variable>PRIORITE</variable><default>err</default>
            <item>crit</item><item>err</item><item>warning</item>
          </comboboxtext>
          <text><label>Lignes au plus :</label></text>
          <spinbutton range-min="10" range-max="500" range-step="10"><variable>MAX</variable><default>50</default></spinbutton></hbox>
        <checkbox><label>Occupation des disques</label><variable>DISQUES</variable><default>true</default></checkbox>
      </vbox>
    </frame>
    <hbox><text><label>Page à écrire :</label></text>
      <entry space-expand="true" space-fill="true"><variable>PAGE</variable><default>/var/www/intranet/rapport.html</default></entry></hbox>
    <hbox>
      <button cancel></button>
      <button><label>Produire la page</label><action>exit:produire</action></button>
    </hbox>
  </vbox>
</window>
XML
}

# La page elle-même : arguments titre, période, puis lignes
# « montage|taille|utilisé|% ».
dialogue_rapport() {
	local titre=${1:-Rapport hebdomadaire du serveur} periode=${2:-du 2026-10-04 au 2026-10-11, gravité err et plus}
	# Le journal (plusieurs lignes) entre par <default>, échappé par xml : jamais
	# collé dans une commande <input>, où une ligne de journal deviendrait du shell.
	shift 2 2>/dev/null || shift $#
	local disques=("$@")
	[ ${#disques[@]} -gt 0 ] || disques=("/|29G|11G|39%" "/var|98G|71G|76%")
	cat <<XML
<window title="$(xml "$titre")">
  <vbox>
    <text><label>Période : $(xml "$periode")</label></text>
    <frame label="Erreurs du journal système">
      <edit editable="false" height-request="160"><variable>JOURNAL</variable><default>$(xml "${TEXTE_JOURNAL:-}")</default></edit>
    </frame>
    <frame label="Occupation des disques">
      <table height-request="120">
        <label>Montage|Taille|Utilisé|Occupation</label>
        <variable>DISQUE</variable>
$(for l in "${disques[@]}"; do printf '        <item>%s</item>\n' "$(xml "$l")"; done)
      </table>
    </frame>
    <hbox><button ok></button></hbox>
  </vbox>
</window>
XML
}

suite() {
	# La sortie standard de sermo part vers /dev/null : écrire sur la sortie d'erreur.
	exec >&2
	case "$1" in
	principal)
		[ "$EXIT" = produire ] || exit 0
		[[ $DEBUT =~ ^[0-9]{4}-[0-9]{2}-[0-9]{2}$ ]] || { echo "Date refusée : $DEBUT" >&2; exit 1; }
		case "$PRIORITE" in crit|err|warning) ;; *) PRIORITE=err ;; esac
		[[ $MAX =~ ^[0-9]+$ ]] || MAX=50
		[[ $PAGE = /*.html ]] || { echo "Donnez un chemin absolu en .html : $PAGE" >&2; exit 1; }
		# Lecture seule : toujours lancée. Hors du groupe adm, seul votre journal est lisible.
		journal=$(journalctl --no-pager --quiet --since "$DEBUT" --priority "$PRIORITE" --lines "$MAX" 2>&1)
		disques=()
		[ "$DISQUES" = true ] && mapfile -t disques < <(df -h -x tmpfs -x devtmpfs -x efivarfs \
			--output=target,size,used,pcent 2>/dev/null | awk 'NR > 1 { print $1 "|" $2 "|" $3 "|" $4 }')
		export TEXTE_JOURNAL=${journal:-(aucune entrée)}
		export RAPPORT; RAPPORT=$(dialogue_rapport "$TITRE" \
			"du $DEBUT au $(date +%F), gravité $PRIORITE et plus" "${disques[@]}")
		# La page est écrite par htmlsermo en mode export : page figée, aucun script.
		agir env SERMO_HTML_MODE=export SERMO_HTML_OUT="$PAGE" "$HTMLSERMO" --program=RAPPORT
		;;
	esac
}

main() {
	if [ "${1:-}" = --suite ]; then suite "$2"; exit; fi
	[ -n "${1:-}" ] && SERMO=$1
	for c in journalctl df "$HTMLSERMO"; do
		command -v "$c" >/dev/null || { echo "$c est absent : installez systemd, coreutils ou sermo-backend-html." >&2; exit 1; }
	done
	export DIALOGUE; DIALOGUE=$(dialogue_principal "$(date -d '-7 days' +%F)")
	"$SERMO" --program=DIALOGUE --do='"$SERMO_SCRIPT" --suite principal' >/dev/null
}

[ "${BASH_SOURCE[0]}" = "$0" ] && main "$@"
