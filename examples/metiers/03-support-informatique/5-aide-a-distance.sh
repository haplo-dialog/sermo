#!/bin/bash
# SPDX-License-Identifier: CC0-1.0
# Métier   : Support informatique
# Besoin   : ouvrir une aide à distance : choix de l'outil, consignes à lire, accord de la personne
# Enrobe   : rustdesk, x11vnc (par tunnel SSH), tmate, timeout
# Ports    : gtk3sermo conseillé (la personne aidée est devant l'écran)
# Terminal : tmate seul a du sens par SSH ; RustDesk et x11vnc partagent l'écran graphique
# Usage    : ./5-aide-a-distance.sh [binaire sermo]
#            SERMO_A_BLANC=0 pour agir pour de vrai (défaut : la commande est affichée)

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

# Arguments : présence de rustdesk, x11vnc, tmate (« installé » ou « absent »).
dialogue_principal() {
	local rd=${1:-installé} vnc=${2:-absent} tm=${3:-installé}
	cat <<XML
<window title="Aide à distance" default-width="540">
  <vbox>
    <frame label="Qui">
      <vbox>
        <hbox><text><label>Personne aidée :</label></text>
          <entry space-expand="true" space-fill="true"><variable>PERSONNE</variable><default>Camille Dupont</default></entry></hbox>
        <hbox><text><label>Technicien :</label></text>
          <entry space-expand="true" space-fill="true"><variable>TECHNICIEN</variable><default>Support informatique</default></entry></hbox>
        <hbox><text><label>Joint par :</label></text>
          <comboboxtext space-expand="true" space-fill="true">
            <variable>CANAL</variable>
            <default>Téléphone</default>
            <item>Téléphone</item>
            <item>Courriel</item>
            <item>Messagerie interne</item>
          </comboboxtext></hbox>
      </vbox>
    </frame>
    <frame label="Outil">
      <vbox>
        <radiobutton><label>RustDesk : écran partagé ($(xml "$rd"))</label><default>true</default><variable>O_RUSTDESK</variable></radiobutton>
        <radiobutton><label>x11vnc par tunnel SSH : écran partagé ($(xml "$vnc"))</label><variable>O_VNC</variable></radiobutton>
        <radiobutton><label>tmate : terminal partagé ($(xml "$tm"))</label><variable>O_TMATE</variable></radiobutton>
        <radiobutton><label>Aucun : guider la personne à la voix</label><variable>O_VOIX</variable></radiobutton>
      </vbox>
    </frame>
    <frame label="Limites">
      <vbox>
        <hbox><text><label>Durée maximale (minutes) :</label></text>
          <spinbutton range-min="5" range-max="240" range-step="5"><variable>DUREE</variable><default>30</default></spinbutton></hbox>
        <checkbox><label>Vue seule : le technicien regarde, sans clavier ni souris (x11vnc)</label><variable>VUE_SEULE</variable></checkbox>
      </vbox>
    </frame>
    <hbox>
      <button cancel></button>
      <button><label>Préparer les consignes</label><action>exit:preparer</action></button>
    </hbox>
  </vbox>
</window>
XML
}

# Arguments : fichier des consignes, personne, technicien, durée.
dialogue_accord() {
	local fichier=${1:-} personne=${2:-Camille Dupont} technicien=${3:-Support informatique} duree=${4:-30}
	cat <<XML
<window title="Aide à distance : accord" default-width="620" default-height="480">
  <vbox>
    <text><label>Consignes pour $(xml "$personne") :</label></text>
    <edit editable="false" space-expand="true" space-fill="true">
XML
	if [ -n "$fichier" ]; then
		printf '      <input file>%s</input>\n' "$(xml "$fichier")"
	else
		printf '      <default>%s</default>\n' "1. Ouvrez RustDesk depuis le menu Internet. (démonstration)"
	fi
	cat <<XML
    </edit>
    <frame label="Question à la personne aidée" space-expand="false" space-fill="false">
      <text><label>$(xml "$personne"), acceptez-vous que $(xml "$technicien") accède à ce poste pendant $(xml "$duree") minutes au plus ?</label></text>
    </frame>
    <hbox>
      <button><label>Je refuse</label><action>exit:refuser</action></button>
      <button><label>J'accepte</label><action>exit:accepter</action></button>
    </hbox>
  </vbox>
</window>
XML
}

consignes() {
	local hote; hote=$(hostname)
	echo "Session d'aide à distance — $(date '+%Y-%m-%d %H:%M'), contact par $CANAL."
	echo "Durée maximale : $DUREE minutes. Vous pouvez arrêter à tout moment."
	echo
	if [ "$O_RUSTDESK" = true ]; then
		cat <<EOF
Pour la personne aidée :
1. Après votre accord, RustDesk s'ouvre.
2. Lisez au technicien l'« ID » et le « mot de passe à usage unique » affichés.
3. Quand une demande de connexion apparaît, vérifiez le nom puis acceptez.
4. Pour finir : fermez la fenêtre RustDesk.
EOF
	elif [ "$O_VNC" = true ]; then
		cat <<EOF
Pour la personne aidée :
1. Après votre accord, rien d'autre à faire : l'écran est partagé.
2. Le partage s'arrête quand le technicien se déconnecte, ou au bout de $DUREE minutes.

Pour le technicien (le partage n'écoute que sur ce poste, on y entre par SSH) :
  ssh -L 5900:localhost:5900 $USER@$hote
  puis, dans un autre terminal : vncviewer localhost:5900
EOF
	elif [ "$O_TMATE" = true ]; then
		cat <<EOF
Pour la personne aidée :
1. Après votre accord, un terminal tmate s'ouvre.
2. Lisez au technicien la ligne qui commence par « ssh session: ».
3. Pour finir : tapez exit dans ce terminal.
EOF
	else
		cat <<EOF
Pas d'accès au poste : le technicien vous guide pas à pas.
1. Décrivez ce que vous voyez à l'écran, mot pour mot.
2. Ne tapez jamais de mot de passe à voix haute.
EOF
	fi
}

suite() {
	exec >&2
	case "$1" in
	principal)
		[ "$EXIT" = preparer ] || exit 0
		[[ "$DUREE" =~ ^[0-9]+$ && "$DUREE" -ge 1 ]] || { echo "Durée illisible : $DUREE" >&2; exit 1; }
		local outil=""
		[ "$O_RUSTDESK" = true ] && outil=rustdesk
		[ "$O_VNC" = true ] && outil=x11vnc
		[ "$O_TMATE" = true ] && outil=tmate
		if [ -n "$outil" ]; then
			command -v "$outil" >/dev/null || { echo "$outil est absent : installez-le ou choisissez un autre outil." >&2; exit 1; }
		fi
		local f; f=$(mktemp "${XDG_RUNTIME_DIR:-$HOME}/sermo-aide-XXXXXX") || exit 1
		consignes > "$f"
		export O_RUSTDESK O_VNC O_TMATE O_VOIX DUREE VUE_SEULE PERSONNE TECHNICIEN
		export DIALOGUE; DIALOGUE=$(dialogue_accord "$f" "$PERSONNE" "$TECHNICIEN" "$DUREE")
		"$SERMO" --program=DIALOGUE --do='"$SERMO_SCRIPT" --suite accord' >/dev/null
		rm -f -- "$f"
		;;
	accord)
		if [ "$EXIT" != accepter ]; then
			echo "Accès refusé par $PERSONNE : rien n'est ouvert." >&2; exit 0
		fi
		# timeout borne la session : à l'échéance, l'outil est arrêté.
		if [ "$O_RUSTDESK" = true ]; then
			agir timeout "${DUREE}m" rustdesk
		elif [ "$O_VNC" = true ]; then
			local opts=(-display "${DISPLAY:-:0}" -localhost -once -noxdamage)
			[ "$VUE_SEULE" = true ] && opts+=(-viewonly)
			agir timeout "${DUREE}m" x11vnc "${opts[@]}"
		elif [ "$O_TMATE" = true ]; then
			agir timeout "${DUREE}m" tmate -F
		else
			echo "Aide à la voix : aucun accès ouvert."
		fi
		;;
	esac
}

main() {
	if [ "${1:-}" = --suite ]; then suite "$2"; exit; fi
	[ -n "${1:-}" ] && SERMO=$1
	command -v timeout >/dev/null || { echo "timeout est absent : installez coreutils." >&2; exit 1; }
	local etat=() o
	for o in rustdesk x11vnc tmate; do
		if command -v "$o" >/dev/null; then etat+=(installé); else etat+=(absent); fi
	done
	export DIALOGUE; DIALOGUE=$(dialogue_principal "${etat[@]}")
	"$SERMO" --program=DIALOGUE --do='"$SERMO_SCRIPT" --suite principal' >/dev/null
}

[ "${BASH_SOURCE[0]}" = "$0" ] && main "$@"
