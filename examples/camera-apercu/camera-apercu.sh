#!/bin/bash
# SPDX-License-Identifier: CC0-1.0
# Métier   : Support, visioconférence, atelier, surveillance de son propre local
# Besoin   : voir l'image d'une webcam ou d'une caméra IP, sur un clic, avec un
#            indicateur « caméra active » — sans rien enregistrer
# Enrobe   : ffplay (paquet ffmpeg) ; curl pour une caméra HTTP(S) à mot de passe ;
#            bwrap (facultatif) pour isoler le lecteur
# Ports    : tout port à fenêtre (l'image s'ouvre dans la fenêtre de ffplay)
# Terminal : sans affichage graphique, l'aperçu est refusé
# Usage    : ./camera-apercu.sh [binaire sermo]
#            Sans fenêtre (CA_DIR = dossier de session) :
#              ./camera-apercu.sh --cameras
#              ./camera-apercu.sh --voir-locale /dev/videoN RÉSOLUTION FORMAT
#              ./camera-apercu.sh --voir-ip ADRESSE [FICHIER_IDENTIFIANTS]
#              ./camera-apercu.sh --etat | --arreter
#            CA_BAC : auto (défaut : bwrap s'il est là), require, off
#
# Codes de sortie : 0 fait · 2 outil absent · 4 caméra occupée · 64 entrée invalide.
# Règles : la caméra ne s'ouvre que sur un geste ; « CAMÉRA
# ACTIVE » est affiché tant qu'elle tourne ; rien n'est enregistré ; une caméra
# occupée par un autre programme est refusée, jamais « volée » ; les identifiants
# d'une caméra IP ne passent JAMAIS en argument (curl les lit sur son entrée).

export SERMO="${SERMO:-sermo}"
export SERMO_SCRIPT="$0"
export CA_BAC="${CA_BAC:-auto}"
export V4L_SYSFS="${V4L_SYSFS:-/sys/class/video4linux}"
RESOLUTIONS="640x480 1280x720 1920x1080 320x240"
FORMATS="mjpeg yuyv422"

xml() { local s=${1//&/"&amp;"}; s=${s//</"&lt;"}; s=${s//>/"&gt;"}; printf '%s' "${s//\"/"&quot;"}"; }
dans() { local x; for x in $2; do [ "$1" = "$x" ] && return 0; done; return 1; }
refus() { printf 'entrée invalide : %s\n' "$1" | tee "$CA_DIR/etat" >&2; exit 64; }
propre() { LC_ALL=C tr -d '\000-\037\177' | sed 's/|/¦/g' | cut -c1-60 | iconv -f UTF-8 -t UTF-8 -c; }

# ── Liste : lue dans /sys, SANS ouvrir les caméras ; nœuds de métadonnées écartés
cameras() {
	local d n i
	for d in "$V4L_SYSFS"/video*; do
		n=${d##*/}; [[ "$n" =~ ^video[0-9]{1,3}$ ]] || continue
		i=$(timeout 1 head -c 8 -- "$d/index" 2>/dev/null)
		[ "${i:-0}" = 0 ] || continue                  # index ≠ 0 : métadonnées, pas d'image
		printf '/dev/%s|%s\n' "$n" "$(timeout 1 head -c 128 -- "$d/name" 2>/dev/null | head -n 1 | propre)"
	done
}

noeud_valide() {
	local reel
	[[ "$1" =~ ^/dev/video[0-9]{1,3}$ ]] || return 1
	reel=$(readlink -f -- "$1") && [[ "$reel" =~ ^/dev/video[0-9]{1,3}$ ]] && [ -c "$reel" ]
}
url_valide() {   # schéma permis, hôte, port, chemin ; pas d'identifiants, pas d'espace
	[ "${#1}" -le 1024 ] &&
	[[ "$1" =~ ^(rtsps?|https?)://([A-Za-z0-9.-]{1,253}|\[[0-9A-Fa-f:]{2,39}\])(:[1-9][0-9]{0,4})?(/[A-Za-z0-9._~/%?=\&+,;-]*)?$ ]] &&
	[[ "${BASH_REMATCH[2]}" != -* ]]
}

# ── Lecture en fond, gardée : lecteur (ffplay) et, pour HTTP à mot de passe, curl.
# Un gardien les tue si le processus maître disparaît. PID exacts, jamais un groupe.
arreter() {
	local g
	g=$(cat "$CA_DIR/gardien" 2>/dev/null)
	if [[ "$g" =~ ^[0-9]+$ ]]; then kill -TERM "$g" 2>/dev/null; fi
	rm -f -- "$CA_DIR/gardien"
}
# lancer <étiquette> <entrée-curl ou vide> -- commande du lecteur…
lancer() {
	local etiquette=$1 avec_curl=$2 n; shift 3
	arreter
	rm -f -- "$CA_DIR/lecteur" "$CA_DIR/curl" "$CA_DIR/flux"; : > "$CA_DIR/lecteur.err"
	[ -n "$avec_curl" ] && mkfifo -m 600 "$CA_DIR/flux"
	bash -c '
		maitre=$1; dir=$2; avec_curl=$3; shift 3
		if [ -n "$avec_curl" ]; then
			curl -sS -K - -o "$dir/flux" < "$dir/curl.conf" 2>>"$dir/lecteur.err" &
			echo $! > "$dir/curl"
			rm -f -- "$dir/curl.conf"     # lu par curl : plus rien sur disque
			"$@" < "$dir/flux" 2>>"$dir/lecteur.err" &
		else
			"$@" </dev/null 2>>"$dir/lecteur.err" &
		fi
		lecteur=$!
		echo "$lecteur" > "$dir/lecteur"
		finir() { kill -TERM "$lecteur" $(cat "$dir/curl" 2>/dev/null) 2>/dev/null; }
		trap '"'"'finir; exit 0'"'"' TERM INT HUP
		while kill -0 "$maitre" 2>/dev/null && kill -0 "$lecteur" 2>/dev/null; do sleep 1 & wait $!; done
		finir
	' _ "${CA_MAITRE:-$PPID}" "$CA_DIR" "$avec_curl" "$@" </dev/null >/dev/null 2>&1 &
	echo $! > "$CA_DIR/gardien"
	printf '%s' "$etiquette" > "$CA_DIR/source"
	# Trois secondes pour voir si la caméra s'ouvre ou si elle est occupée.
	for n in $(seq 1 30); do
		grep -qi 'busy' "$CA_DIR/lecteur.err" 2>/dev/null && break
		[ -s "$CA_DIR/lecteur" ] && ! kill -0 "$(cat "$CA_DIR/lecteur")" 2>/dev/null && break
		sleep 0.1
	done
	if grep -qi 'busy' "$CA_DIR/lecteur.err"; then
		arreter
		echo "caméra occupée par un autre programme : rien n'a été forcé, réessayez dans quelques secondes" | tee "$CA_DIR/etat"
		exit 4
	fi
	if [ -s "$CA_DIR/lecteur" ] && ! kill -0 "$(cat "$CA_DIR/lecteur")" 2>/dev/null; then
		printf 'échec de l'"'"'aperçu : %s\n' "$(grep -v '^\s*$' "$CA_DIR/lecteur.err" | tail -n 1 | propre)" | tee "$CA_DIR/etat"
		exit 2
	fi
	printf 'aperçu lancé : %s\n' "$etiquette" | tee "$CA_DIR/etat"
}

# Préfixe d'isolation : seul le nœud choisi est visible ; réseau coupé si local.
bac() {   # bac <nœud ou vide> → remplit le tableau BAC
	BAC=()
	case "$CA_BAC" in off) echo "inactif (CA_BAC=off)" > "$CA_DIR/bac"; return 0 ;; auto|require) ;; *) refus "CA_BAC « $CA_BAC »" ;; esac
	if ! command -v bwrap >/dev/null; then
		[ "$CA_BAC" = require ] && { echo "bwrap absent et CA_BAC=require : aperçu refusé" | tee "$CA_DIR/etat"; exit 2; }
		echo "inactif (bwrap absent)" > "$CA_DIR/bac"; return 0
	fi
	BAC=(bwrap --ro-bind / / --dev /dev --tmpfs /tmp --die-with-parent)
	[ -n "$1" ] && BAC+=(--dev-bind "$1" "$1" --unshare-net)
	[ -n "${CA_DIR:-}" ] && BAC+=(--bind "$CA_DIR" "$CA_DIR")
	[ -d /tmp/.X11-unix ] && BAC+=(--ro-bind /tmp/.X11-unix /tmp/.X11-unix)
	[ -n "${XAUTHORITY:-}" ] && [ -e "$XAUTHORITY" ] && BAC+=(--ro-bind "$XAUTHORITY" "$XAUTHORITY")
	echo "actif (bwrap : ${1:-réseau permis} ; écriture : dossier de session seul)" > "$CA_DIR/bac"
}

voir_locale() {
	local n=$1 r=$2 f=$3 nom
	noeud_valide "$n" || refus "caméra « $n » (/dev/videoN existant)"
	dans "$r" "$RESOLUTIONS" || refus "résolution « $r »"
	dans "$f" "$FORMATS" || refus "format « $f »"
	[ -n "${DISPLAY:-}${WAYLAND_DISPLAY:-}" ] || { echo "pas d'affichage graphique : aperçu impossible" | tee "$CA_DIR/etat"; exit 2; }
	command -v ffplay >/dev/null || { echo "ffplay absent : sudo apt install ffmpeg" | tee "$CA_DIR/etat"; exit 2; }
	nom=$(cameras | awk -F'|' -v n="$n" '$1 == n { print $2; exit }'); nom=${nom:-$n}
	bac "$n"
	lancer "$nom ($n, $r, $f)" "" -- "${BAC[@]}" ffplay -hide_banner -loglevel error -nostats -an \
		-window_title "sermo — CAMÉRA ACTIVE — $nom" -f v4l2 -input_format "$f" -video_size "$r" -i "$n"
}

voir_ip() {
	local u=$1 idf=${2:-} ident
	url_valide "$u" || refus "adresse « $u » (rtsp, rtsps, http, https ; sans identifiants dans l'adresse)"
	[ -n "${DISPLAY:-}${WAYLAND_DISPLAY:-}" ] || { echo "pas d'affichage graphique : aperçu impossible" | tee "$CA_DIR/etat"; exit 2; }
	command -v ffplay >/dev/null || { echo "ffplay absent : sudo apt install ffmpeg" | tee "$CA_DIR/etat"; exit 2; }
	bac ""
	if [ -z "$idf" ]; then
		lancer "${u%%\?*}" "" -- "${BAC[@]}" ffplay -hide_banner -loglevel error -nostats -an \
			-window_title "sermo — CAMÉRA ACTIVE — ${u%%\?*}" -protocol_whitelist rtsp,rtsps,rtp,srtp,udp,tcp,tls,http,https -i "$u"
		return
	fi
	# Identifiants : fichier à soi, lisible par soi seul, « utilisateur:mot de passe ».
	[[ "$idf" == /* ]] && [ -f "$idf" ] && [ ! -L "$idf" ] || refus "fichier d'identifiants « $idf » (chemin absolu d'un fichier ordinaire)"
	[ "$(stat -c '%u %a' -- "$idf")" = "$(id -u) 600" ] || refus "fichier d'identifiants : il doit être à vous et en mode 600"
	[[ "$u" == http* ]] || refus "identifiants : seulement pour une caméra HTTP(S) — en RTSP, ffplay les exigerait dans l'adresse, donc en argument"
	command -v curl >/dev/null || { echo "curl absent : sudo apt install curl" | tee "$CA_DIR/etat"; exit 2; }
	ident=$(head -n 1 -- "$idf")
	[[ "$ident" == *:* && "$ident" != *[$'\r\t"\\']* && "${#ident}" -le 256 ]] || refus "fichier d'identifiants : une ligne « utilisateur:mot de passe », sans guillemet"
	# La config de curl (adresse + identifiants) est passée par un fichier 600 lu en
	# entrée standard, puis effacée : rien dans les arguments, ni de curl ni du lecteur.
	printf 'url = "%s"\nuser = "%s"\nproto = "=http,https"\n' "$u" "$ident" > "$CA_DIR/curl.conf"
	lancer "${u%%\?*} (identifiants : fichier)" oui -- "${BAC[@]}" ffplay -hide_banner -loglevel error -nostats -an \
		-window_title "sermo — CAMÉRA ACTIVE — ${u%%\?*}" -i -
}

etat() {
	local l
	l=$(cat "$CA_DIR/lecteur" 2>/dev/null)
	if [[ "$l" =~ ^[0-9]+$ ]] && kill -0 "$l" 2>/dev/null; then
		printf '● CAMÉRA ACTIVE — %s — bac à sable %s\n' "$(cat "$CA_DIR/source")" "$(cat "$CA_DIR/bac" 2>/dev/null)"
	else
		cat "$CA_DIR/etat" 2>/dev/null || echo "aucune caméra ouverte"
	fi
}

dialogue_principal() {
	local x
	cat <<XML
<window title="Aperçu caméra" default-width="680" default-height="440">
  <vbox spacing="6">
    <frame label="Caméra de cette machine (liste lue sans l'ouvrir)">
      <vbox>
        <tree>
          <label>Nœud|Nom</label>
          <variable>CAMERA</variable>
          <height>110</height>
          <input>exec "\$SERMO_SCRIPT" --cameras</input>
        </tree>
        <hbox>
          <comboboxtext><variable>RESOLUTION</variable><default>640x480</default>
$(for x in $RESOLUTIONS; do printf '            <item>%s</item>\n' "$x"; done)
          </comboboxtext>
          <comboboxtext><variable>FORMAT</variable><default>mjpeg</default>
$(for x in $FORMATS; do printf '            <item>%s</item>\n' "$x"; done)
          </comboboxtext>
          <button><label>Voir</label>
            <action>exec "\$SERMO_SCRIPT" --voir-locale "\$CAMERA" "\$RESOLUTION" "\$FORMAT"</action><action>refresh:ETAT</action>
          </button>
        </hbox>
      </vbox>
    </frame>
    <frame label="Caméra IP (rtsp, http…) — identifiants : chemin d'un fichier 600, jamais dans l'adresse">
      <hbox>
        <entry space-expand="true" space-fill="true"><variable>ADRESSE</variable></entry>
        <entry><variable>IDENTIFIANTS</variable></entry>
        <button><label>Voir</label>
          <action>exec "\$SERMO_SCRIPT" --voir-ip "\$ADRESSE" "\$IDENTIFIANTS"</action><action>refresh:ETAT</action>
        </button>
      </hbox>
    </frame>
    <text use-markup="false"><variable>ETAT</variable><input>exec "\$SERMO_SCRIPT" --etat</input></text>
    <text><label>Rien n'est enregistré. Une caméra occupée par un autre programme est refusée, jamais forcée.</label></text>
    <timer interval="1" visible="false">
      <variable>HORLOGE</variable>
      <action>refresh:ETAT</action>
    </timer>
    <hbox>
      <button><label>Arrêter l'aperçu</label><action>exec "\$SERMO_SCRIPT" --arreter</action><action>refresh:ETAT</action></button>
      <button><label>Fermer</label><action>exit:fermer</action></button>
    </hbox>
  </vbox>
</window>
XML
}

preparer_dossier() {
	if [ -n "${CA_DIR:-}" ] && [ -d "$CA_DIR" ]; then return 0; fi
	CA_DIR=$(mktemp -d "${XDG_RUNTIME_DIR:-${TMPDIR:-/tmp}}/sermo-camera.XXXXXX") || exit 2
	export CA_DIR; CA_PROPRE=1
	echo "aucune caméra ouverte" > "$CA_DIR/etat"
}

usage() { sed -n '/^# Usage/,/^# Codes/p' "$0" | sed '$d;s/^# \{0,1\}//' >&2; exit 64; }

main() {
	umask 077
	case "${1:-}" in
	--cameras) cameras; exit 0 ;;
	--voir-locale|--voir-ip|--etat|--arreter)
		preparer_dossier
		[ "${CA_PROPRE:-}" = 1 ] && [[ "$1" != --voir-* ]] && trap 'rm -rf -- "$CA_DIR"' EXIT
		case "$1" in
		--voir-locale) voir_locale "${2:-}" "${3:-640x480}" "${4:-mjpeg}" ;;
		--voir-ip)     voir_ip "${2:-}" "${3:-}" ;;
		--etat)        etat ;;
		--arreter)     arreter; echo "aperçu arrêté" > "$CA_DIR/etat" ;;
		esac
		exit ;;
	--xml) dialogue_principal; exit 0 ;;
	-*)    usage ;;
	esac
	[ -n "${1:-}" ] && SERMO=$1
	command -v "$SERMO" >/dev/null || { echo "sermo introuvable : $SERMO" >&2; exit 2; }
	preparer_dossier
	export CA_MAITRE=$$
	trap 'arreter; rm -rf -- "$CA_DIR"' EXIT
	trap 'exit 129' HUP; trap 'exit 130' INT; trap 'exit 143' TERM
	export DIALOGUE; DIALOGUE=$(dialogue_principal)
	"$SERMO" --program=DIALOGUE >/dev/null
	exit 0
}

[ "${BASH_SOURCE[0]}" = "$0" ] && main "$@"
