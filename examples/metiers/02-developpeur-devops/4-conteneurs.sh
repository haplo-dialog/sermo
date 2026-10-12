#!/bin/bash
# SPDX-License-Identifier: CC0-1.0
# Métier   : Développeur / DevOps
# Besoin   : lister les conteneurs, lire leurs journaux, les démarrer, arrêter ou redémarrer
# Enrobe   : podman (sinon docker) ps, logs, start, stop, restart
# Ports    : gtk3sermo conseillé ; ncursessermo par SSH
# Terminal : rien de perdu
# Usage    : ./4-conteneurs.sh [binaire sermo]
#            SERMO_A_BLANC=0 pour agir pour de vrai (défaut : la commande est affichée)
#            MOTEUR=docker pour forcer docker quand podman est aussi installé.

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

# Arguments : moteur, puis une ligne par conteneur « nom|image|état ».
dialogue_principal() {
	[ $# -gt 0 ] || set -- podman \
		"web|docker.io/library/nginx:1.27|Up 3 hours" \
		"base|docker.io/library/postgres:16|Up 3 hours" \
		"file-attente|docker.io/library/redis:7|Exited (137) 2 days ago"
	local moteur=$1 l; shift
	cat <<XML
<window title="Conteneurs ($(xml "$moteur"))" default-width="620" default-height="440">
  <vbox>
    <tree scrollable="true" space-expand="true" space-fill="true">
      <label>Nom|Image|État</label>
      <variable>CONTENEUR</variable>
XML
	for l in "$@"; do printf '      <item>%s</item>\n' "$(xml "$l")"; done
	cat <<XML
    </tree>
    <frame label="Action sur le conteneur choisi" space-expand="false" space-fill="false">
      <vbox>
        <hbox>
          <radiobutton><label>Journaux</label><default>true</default><variable>A_JOURNAUX</variable></radiobutton>
          <radiobutton><label>Démarrer</label><variable>A_DEMARRER</variable></radiobutton>
          <radiobutton><label>Redémarrer</label><variable>A_REDEMARRER</variable></radiobutton>
          <radiobutton><label>Arrêter</label><variable>A_ARRETER</variable></radiobutton>
        </hbox>
        <hbox><text><label>Dernières lignes de journal :</label></text>
          <spinbutton range-min="10" range-max="10000" range-step="10"><variable>LIGNES</variable><default>200</default></spinbutton>
          <checkbox><label>Avec l'heure (-t)</label><variable>HORODATAGE</variable></checkbox></hbox>
      </vbox>
    </frame>
    <hbox>
      <button cancel></button>
      <button><label>Appliquer</label><action>exit:appliquer</action></button>
    </hbox>
  </vbox>
</window>
XML
}

# Arguments : fichier des journaux, nom du conteneur.
dialogue_journaux() {
	local fichier=${1:-} nom=${2:-web}
	cat <<XML
<window title="Journaux de $(xml "$nom")" default-width="760" default-height="500">
  <vbox>
    <edit editable="false" space-expand="true" space-fill="true">
XML
	if [ -n "$fichier" ]; then
		printf '      <input file>%s</input>\n' "$(xml "$fichier")"
	else
		printf '      <default>%s</default>\n' "10.0.2.100 - - [11/Oct/2026:09:12:03 +0000] GET /sante HTTP/1.1 200 (démonstration)"
	fi
	cat <<XML
    </edit>
    <hbox><button><label>Fermer</label><action>exit:fermer</action></button></hbox>
  </vbox>
</window>
XML
}

dialogue_confirmation() {
	local verbe=${1:-arrêter} nom=${2:-base}
	cat <<XML
<window title="Confirmer">
  <vbox>
    <text><label>Vraiment $(xml "$verbe") le conteneur $(xml "$nom") ?</label></text>
    <text><label>Le service qu'il rend sera interrompu.</label></text>
    <hbox>
      <button cancel></button>
      <button><label>Confirmer</label><action>exit:confirmer</action></button>
    </hbox>
  </vbox>
</window>
XML
}

suite() {
	exec >&2
	case "$1" in
	principal)
		[ "$EXIT" = appliquer ] || exit 0
		[[ "$CONTENEUR" =~ ^[A-Za-z0-9][A-Za-z0-9_.-]*$ ]] || { echo "Aucun conteneur choisi (ou nom refusé)." >&2; exit 1; }
		if [ "$A_JOURNAUX" = true ]; then
			[[ "$LIGNES" =~ ^[0-9]+$ ]] || LIGNES=200
			local opts=(--tail "$LIGNES"); [ "$HORODATAGE" = true ] && opts+=(-t)
			local f; f=$(mktemp "${XDG_RUNTIME_DIR:-$HOME}/sermo-logs-XXXXXX") || exit 1
			# Lecture seule ; le conteneur écrit aussi sur sa sortie d'erreur : on garde les deux.
			"$MOTEUR" logs "${opts[@]}" -- "$CONTENEUR" > "$f" 2>&1
			export DIALOGUE; DIALOGUE=$(dialogue_journaux "$f" "$CONTENEUR")
			"$SERMO" --program=DIALOGUE >/dev/null
			rm -f -- "$f"
		elif [ "$A_DEMARRER" = true ]; then
			agir "$MOTEUR" start -- "$CONTENEUR"
		else
			local verbe=redémarrer; [ "$A_ARRETER" = true ] && verbe=arrêter
			export CONTENEUR A_ARRETER
			export DIALOGUE; DIALOGUE=$(dialogue_confirmation "$verbe" "$CONTENEUR")
			"$SERMO" --program=DIALOGUE --do='"$SERMO_SCRIPT" --suite confirmation' >/dev/null
		fi
		;;
	confirmation)
		[ "$EXIT" = confirmer ] || exit 0
		if [ "$A_ARRETER" = true ]; then
			agir "$MOTEUR" stop -- "$CONTENEUR"
		else
			agir "$MOTEUR" restart -- "$CONTENEUR"
		fi
		;;
	esac
}

main() {
	if [ "${1:-}" = --suite ]; then suite "$2"; exit; fi
	[ -n "${1:-}" ] && SERMO=$1
	if [ -z "${MOTEUR:-}" ]; then
		if command -v podman >/dev/null; then MOTEUR=podman
		elif command -v docker >/dev/null; then MOTEUR=docker
		fi
	fi
	case "${MOTEUR:-}" in
	podman|docker) command -v "$MOTEUR" >/dev/null || { echo "$MOTEUR est absent." >&2; exit 1; } ;;
	*) echo "Ni podman ni docker : installez le paquet podman." >&2; exit 1 ;;
	esac
	export MOTEUR
	local lignes=()
	mapfile -t lignes < <("$MOTEUR" ps -a --format '{{.Names}}|{{.Image}}|{{.Status}}' 2>/dev/null)
	[ ${#lignes[@]} -gt 0 ] || { echo "Aucun conteneur ($MOTEUR ps -a est vide)." >&2; exit 1; }
	export DIALOGUE; DIALOGUE=$(dialogue_principal "$MOTEUR" "${lignes[@]}")
	"$SERMO" --program=DIALOGUE --do='"$SERMO_SCRIPT" --suite principal' >/dev/null
}

[ "${BASH_SOURCE[0]}" = "$0" ] && main "$@"
