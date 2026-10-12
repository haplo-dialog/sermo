#!/bin/bash
# SPDX-License-Identifier: CC0-1.0
# Boîte    : Utilisateur
# Besoin   : se faire un rappel qui s'affichera sur le bureau à la date et à l'heure choisies ; voir et annuler ses rappels
# Enrobe   : at, atq, atrm ; notify-send pour la bulle à l'heure dite
# Ports    : gtk3sermo conseillé ; qt6sermo ; htmlsermo
# Terminal : en ncurses, la bulle s'affiche quand même sur le bureau s'il y en a un
# Usage    : ./04-rappel.sh [binaire sermo]
#            SERMO_A_BLANC=0 pour agir pour de vrai (défaut : la commande est affichée)
#            Le service atd doit tourner (paquet at) ; l'installer demande les droits root.

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

# Arguments : le jour proposé, puis une ligne par rappel déjà prévu « numéro|quand ».
dialogue_principal() {
	[ $# -gt 0 ] || set -- 2026-10-12 "7|Mon Oct 12 09:30:00 2026" "8|Thu Oct 15 18:00:00 2026"
	local jour=$1 l
	shift
	cat <<XML
<window title="Me faire un rappel" default-width="560">
  <vbox>
    <frame label="Nouveau rappel">
      <vbox>
        <hbox><text><label>Me rappeler de :</label></text>
          <entry space-expand="true" space-fill="true"><variable>MESSAGE</variable><default>Appeler le cabinet médical</default></entry></hbox>
        <hbox>
          <calendar><variable>JOUR</variable><default>$(xml "$jour")</default></calendar>
          <vbox>
            <hbox><text><label>Heure :</label></text>
              <spinbutton range-min="0" range-max="23" range-step="1"><variable>HEURE</variable><default>9</default></spinbutton></hbox>
            <hbox><text><label>Minutes :</label></text>
              <spinbutton range-min="0" range-max="55" range-step="5"><variable>MINUTE</variable><default>30</default></spinbutton></hbox>
            <hbox><text><label>Importance :</label></text>
              <comboboxtext><variable>IMPORTANCE</variable><default>Normale</default>
                <item>Discrète</item><item>Normale</item><item>Urgente</item>
              </comboboxtext></hbox>
          </vbox>
        </hbox>
        <hbox><button><label>Créer le rappel</label><action>exit:creer</action></button></hbox>
      </vbox>
    </frame>
    <frame label="Mes rappels déjà prévus">
      <vbox>
        <tree scrollable="true" height-request="110">
          <label>N°|Quand</label>
          <variable>RAPPEL</variable>
XML
	for l in "$@"; do printf '          <item>%s</item>\n' "$(xml "$l")"; done
	cat <<XML
        </tree>
        <hbox><button><label>Annuler le rappel choisi</label><action>exit:annuler</action></button></hbox>
      </vbox>
    </frame>
    <hbox>
      <button cancel></button>
    </hbox>
  </vbox>
</window>
XML
}

dialogue_annulation() {
	local numero=${1:-7} quand=${2:-Mon Oct 12 09:30:00 2026}
	cat <<XML
<window title="Annuler ce rappel ?">
  <vbox>
    <text><label>Le rappel n° $(xml "$numero"), prévu le $(xml "$quand"), sera supprimé.</label></text>
    <text><label>Il ne s'affichera pas.</label></text>
    <hbox>
      <button cancel></button>
      <button><label>Supprimer le rappel</label><action>exit:supprimer</action></button>
    </hbox>
  </vbox>
</window>
XML
}

# Les rappels prévus, « numéro|quand », lus dans atq.
prevus() {
	local n reste quand
	while IFS=$'\t' read -r n reste; do
		quand=${reste% * *}
		printf '%s|%s\n' "$n" "$quand"
	done < <(atq 2>/dev/null | sort -n)
}

suite() {
	# La sortie standard de sermo part vers /dev/null : écrire sur la sortie d'erreur.
	exec >&2
	case "$1" in
	principal)
		case "$EXIT" in
		creer)
			# Aucun caractère de contrôle (saut de ligne, tabulation, échappement…).
			[[ $MESSAGE =~ [[:cntrl:]] ]] && { echo "Le message contient un caractère invisible : retapez-le." >&2; exit 1; }
			[ -n "$MESSAGE" ] || { echo "Le message est vide." >&2; exit 1; }
			[ ${#MESSAGE} -le 200 ] || { echo "Message trop long (200 caractères au plus)." >&2; exit 1; }
			[[ $JOUR =~ ^([0-9]{4})-([0-9]{2})-([0-9]{2})$ ]] || { echo "Date illisible : $JOUR" >&2; exit 1; }
			# Lu tout de suite : le test suivant écrase BASH_REMATCH.
			local jour=${BASH_REMATCH[1]}${BASH_REMATCH[2]}${BASH_REMATCH[3]}
			[[ $HEURE =~ ^[0-9]{1,2}$ ]] && [ "$HEURE" -le 23 ] || { echo "Heure illisible : $HEURE" >&2; exit 1; }
			[[ $MINUTE =~ ^[0-9]{1,2}$ ]] && [ "$MINUTE" -le 59 ] || { echo "Minutes illisibles : $MINUTE" >&2; exit 1; }
			local urgence=normal
			case "$IMPORTANCE" in Discrète) urgence=low ;; Urgente) urgence=critical ;; esac
			# at -t AAAAMMJJhhmm. La tâche est lue par /bin/sh (dash) : elle est
			# écrite telle quelle, SANS aucune donnée dedans. Le message et
			# l'urgence voyagent par l'environnement, que at garde pour la tâche.
			local quand
			quand=$(printf '%s%02d%02d' "$jour" "$((10#$HEURE))" "$((10#$MINUTE))")
			# shellcheck disable=SC2016
			local tache='notify-send --urgency="$RAPPEL_URGENCE" --app-name=Rappel -- Rappel "$RAPPEL_MSG"'
			echo "Tâche confiée à at : $tache"
			agir env RAPPEL_MSG="$MESSAGE" RAPPEL_URGENCE="$urgence" at -t "$quand" <<<"$tache"
			;;
		annuler)
			[[ $RAPPEL =~ ^[0-9]+$ ]] || { echo "Choisissez d'abord un rappel dans la liste." >&2; exit 1; }
			local l quand=""
			while IFS= read -r l; do [ "${l%%|*}" = "$RAPPEL" ] && quand=${l#*|}; done < <(prevus)
			[ -n "$quand" ] || { echo "Rappel n° $RAPPEL introuvable." >&2; exit 1; }
			export RAPPEL
			export DIALOGUE; DIALOGUE=$(dialogue_annulation "$RAPPEL" "$quand")
			"$SERMO" --program=DIALOGUE --do='"$SERMO_SCRIPT" --suite annulation' >/dev/null
			;;
		esac
		;;
	annulation)
		[ "$EXIT" = supprimer ] || exit 0
		[[ $RAPPEL =~ ^[0-9]+$ ]] || exit 1
		agir atrm -- "$RAPPEL"
		;;
	esac
}

main() {
	if [ "${1:-}" = --suite ]; then suite "$2"; exit; fi
	[ -n "${1:-}" ] && SERMO=$1
	local c
	for c in at atq atrm; do
		command -v "$c" >/dev/null || { echo "$c est absent. Il faut le paquet at ; l'installer demande les droits root : apt install at" >&2; exit 1; }
	done
	command -v notify-send >/dev/null || { echo "notify-send est absent : il faut le paquet libnotify-bin." >&2; exit 1; }
	local lignes jour
	mapfile -t lignes < <(prevus)
	jour=$(date -d tomorrow +%F)
	export DIALOGUE; DIALOGUE=$(dialogue_principal "$jour" "${lignes[@]}")
	"$SERMO" --program=DIALOGUE --do='"$SERMO_SCRIPT" --suite principal' >/dev/null
}

[ "${BASH_SOURCE[0]}" = "$0" ] && main "$@"
