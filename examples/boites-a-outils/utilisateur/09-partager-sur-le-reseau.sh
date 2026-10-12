#!/bin/bash
# SPDX-License-Identifier: CC0-1.0
# Boîte    : Utilisateur
# Besoin   : prêter un fichier à un appareil de la maison (téléphone, autre ordinateur) pendant quelques minutes, sans clé USB ni compte en ligne
# Enrobe   : python3 -m http.server, dans un dossier dédié qui ne contient QUE la copie du fichier, arrêté par timeout
# Ports    : gtk3sermo conseillé ; qt6sermo ; htmlsermo
# Terminal : rien de perdu
# Usage    : ./09-partager-sur-le-reseau.sh [binaire sermo]
#            SERMO_A_BLANC=0 pour partager pour de vrai (défaut : la commande est affichée)
#            Le partage s'arrête tout seul à la fin de la durée, ou quand on ferme la fenêtre.
#            Toute personne du même réseau (Wi-Fi) qui connaît l'adresse peut télécharger le fichier.

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

# Arguments : fichier proposé, adresse de l'ordinateur sur le réseau.
dialogue_principal() {
	local fichier=${1:-$HOME/Images/photo-anniversaire.jpg} ip=${2:-192.168.1.20}
	cat <<XML
<window title="Prêter un fichier sur le réseau" default-width="540">
  <vbox>
    <frame label="Quel fichier">
      <entry><variable>FICHIER</variable><default>$(xml "$fichier")</default></entry>
    </frame>
    <frame label="Pendant combien de temps">
      <hbox>
        <radiobutton><label>5 minutes</label><variable>D5</variable></radiobutton>
        <radiobutton><label>15 minutes</label><variable>D15</variable><default>true</default></radiobutton>
        <radiobutton><label>1 heure</label><variable>D60</variable></radiobutton>
      </hbox>
    </frame>
    <expander label="Réglages">
      <vbox>
        <hbox><text><label>Numéro de porte (port) :</label></text>
          <spinbutton range-min="8000" range-max="8999" range-step="1"><variable>PORT</variable><default>8080</default></spinbutton></hbox>
        <text><label>Adresse de cet ordinateur : $(xml "$ip")</label></text>
      </vbox>
      <variable>REGLAGES</variable>
    </expander>
    <text><label>Attention : tout appareil qui peut joindre l'adresse $(xml "$ip") et qui connaît le lien pourra le télécharger.</label></text>
    <hbox>
      <button cancel></button>
      <button><label>Partager</label><action>exit:partager</action></button>
    </hbox>
  </vbox>
</window>
XML
}

# Arguments : adresse à taper sur l'autre appareil, durée en minutes.
dialogue_en_cours() {
	local adresse=${1:-http://192.168.1.20:8080/photo-anniversaire.jpg} duree=${2:-15}
	cat <<XML
<window title="Partage en cours" default-width="520">
  <vbox>
    <text><label>Sur l'autre appareil (relié au même réseau), ouvrez le navigateur et tapez :</label></text>
    <entry editable="false"><variable>ADRESSE</variable><default>$(xml "$adresse")</default></entry>
    <statusbar><variable>DUREE</variable><default>Le partage s'arrêtera seul dans $(xml "$duree") minutes.</default></statusbar>
    <hbox>
      <button><label>Arrêter le partage</label><action>exit:arreter</action></button>
    </hbox>
  </vbox>
</window>
XML
}

dialogue_adresse_publique() {
	local ip=${1:-203.0.113.7}
	cat <<XML
<window title="Adresse qui n'est pas celle de la maison">
  <vbox>
    <text><label>L'adresse de cet ordinateur est $(xml "$ip").</label></text>
    <text><label>Ce n'est pas une adresse de réseau domestique : ce peut être un VPN ou Internet.</label></text>
    <text><label>Des inconnus pourraient alors télécharger le fichier. Partager quand même ?</label></text>
    <hbox>
      <button cancel></button>
      <button><label>Partager quand même</label><action>exit:confirmer</action></button>
    </hbox>
  </vbox>
</window>
XML
}

# Vrai si l'adresse est privée (RFC 1918) ou locale au lien (169.254/16).
adresse_privee() {
	local a b
	IFS=. read -r a b _ _ <<< "$1"
	[ "$a" = 10 ] && return 0
	[ "$a" = 172 ] && [ "$b" -ge 16 ] && [ "$b" -le 31 ] && return 0
	[ "$a" = 192 ] && [ "$b" = 168 ] && return 0
	[ "$a" = 169 ] && [ "$b" = 254 ] && return 0
	return 1
}

# Adresse IPv4 de l'ordinateur sur le réseau local (lecture seule).
mon_ip() {
	ip -4 -o route get 1.1.1.1 2>/dev/null | sed -n 's/.* src \([0-9.]*\).*/\1/p'
}

suite() {
	# La sortie standard de sermo part vers /dev/null : écrire sur la sortie d'erreur.
	exec >&2
	case "$1" in
	principal)
		[ "$EXIT" = partager ] || exit 0
		[ -f "$FICHIER" ] || { echo "Fichier introuvable : $FICHIER" >&2; exit 1; }
		[[ $PORT =~ ^[0-9]+$ ]] && [ "$PORT" -ge 8000 ] && [ "$PORT" -le 8999 ] || { echo "Port refusé : $PORT (8000 à 8999)." >&2; exit 1; }
		# dossier et pid restent globaux : le piège EXIT les lit après la fin de suite.
		local minutes=15 ip nom adresse
		dossier="" pid=""
		[ "$D5" = true ] && minutes=5
		[ "$D60" = true ] && minutes=60
		ip=$(mon_ip)
		[[ $ip =~ ^[0-9]{1,3}\.[0-9]{1,3}\.[0-9]{1,3}\.[0-9]{1,3}$ ]] || { echo "Pas de réseau : cet ordinateur n'a pas d'adresse locale." >&2; exit 1; }
		if ! adresse_privee "$ip"; then
			# Réponse lue sur la sortie de sermo : seule la ligne EXIT compte.
			local reponse
			export DIALOGUE; DIALOGUE=$(dialogue_adresse_publique "$ip")
			reponse=$("$SERMO" --program=DIALOGUE | grep '^EXIT=')
			[ "$reponse" = 'EXIT="confirmer"' ] || exit 0
		fi
		nom=$(basename -- "$FICHIER")
		# Un dossier dédié, dans le cache de l'utilisateur, qui ne contient que la copie.
		local cache=${XDG_CACHE_HOME:-$HOME/.cache}
		if [ "$SERMO_A_BLANC" = 1 ]; then
			dossier=$cache/partage-XXXXXX
			agir mkdir -p -- "$cache"
			agir mktemp -d "$dossier"
		else
			mkdir -p -- "$cache" && dossier=$(mktemp -d "$cache/partage-XXXXXX") || exit 1
			# Script tué ou fenêtre fermée : le serveur s'arrête et la copie s'efface.
			trap 'trap - EXIT; [ -n "$pid" ] && kill "$pid" 2>/dev/null; rm -rf -- "$dossier"' EXIT
			trap 'exit 1' INT TERM HUP
		fi
		agir cp -- "$FICHIER" "$dossier/$nom" || exit 1
		if [ "$SERMO_A_BLANC" = 1 ]; then
			agir timeout "${minutes}m" python3 -I -m http.server "$PORT" --bind "$ip" --directory "$dossier"
		else
			# -I : aucun module n'est importé du dossier courant.
			timeout "${minutes}m" python3 -I -m http.server "$PORT" --bind "$ip" --directory "$dossier" >/dev/null 2>&1 &
			pid=$!
		fi
		# Le nom du fichier dans l'adresse : espaces et signes codés pour le navigateur.
		adresse="http://$ip:$PORT/$(python3 -I -c 'import sys, urllib.parse; print(urllib.parse.quote(sys.argv[1]))' "$nom")"
		if [ "$SERMO_A_BLANC" = 1 ]; then
			echo "Adresse qui serait donnée : $adresse"
			agir rm -r -- "$dossier"
		else
			sleep 1
			kill -0 "$pid" 2>/dev/null || { echo "Le partage n'a pas démarré (port $PORT déjà pris ?)." >&2; exit 1; }
			export DIALOGUE; DIALOGUE=$(dialogue_en_cours "$adresse" "$minutes")
			# Sans --do : quand la fenêtre se ferme, le partage s'arrête.
			"$SERMO" --program=DIALOGUE >/dev/null
			# La suite (arrêt du serveur, copie effacée) est faite par le piège EXIT.
		fi
		;;
	esac
}

main() {
	if [ "${1:-}" = --suite ]; then suite "$2"; exit; fi
	[ -n "${1:-}" ] && SERMO=$1
	local c
	for c in python3 timeout ip mktemp; do
		command -v "$c" >/dev/null || { echo "$c est absent : il faut les paquets python3, coreutils et iproute2." >&2; exit 1; }
	done
	local ip
	ip=$(mon_ip)
	export DIALOGUE; DIALOGUE=$(dialogue_principal "$HOME/Images/" "${ip:-pas de réseau}")
	"$SERMO" --program=DIALOGUE --do='"$SERMO_SCRIPT" --suite principal' >/dev/null
}

[ "${BASH_SOURCE[0]}" = "$0" ] && main "$@"
