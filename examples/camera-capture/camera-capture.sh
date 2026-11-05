#!/bin/bash
# SPDX-License-Identifier: CC0-1.0
# Métier   : Support, atelier, inventaire, constat : garder une photo ou quelques
#            secondes de vidéo d'une webcam, en le décidant à chaque fois
# Besoin   : voir la caméra DANS la fenêtre, puis prendre une photo ou
#            enregistrer, après une confirmation qui montre le fichier exact
# Enrobe   : <action media> (paquet sermo-cible-media, mpv) ; la porte
#            examples/approbation pour la confirmation
# Ports    : image dans la fenêtre avec gtk3 et qt6 sous X11 ; ailleurs, dans la
#            fenêtre de mpv (repli)
# Usage    : ./camera-capture.sh [binaire sermo]
#            ./camera-capture.sh --xml           (le XML, sans l'ouvrir)
#            Appelés par les boutons : --photo, --video
#            CC_CAMERA (défaut /dev/video0), CC_DUREE (1 à 600 s, défaut 30),
#            CC_PHOTOS et CC_VIDEOS (dossiers, défaut : Images et Vidéos)
#
# Codes de sortie : 0 fait · 1 refusé ou annulé · 2 outil absent · 64 entrée invalide.
# Règles : la caméra ne s'ouvre que sur un clic ; « CAMÉRA ACTIVE » est écrit
# dans l'image (et « ENREGISTREMENT » pendant un enregistrement) ; rien n'est
# écrit sur le disque sans la confirmation, qui montre le CHEMIN EXACT et la
# durée maximale ; le fichier n'écrase jamais rien et n'est lisible que par vous.

export SERMO="${SERMO:-sermo}"
ICI=$(cd "$(dirname "$0")" && pwd)
CAMERA="${CC_CAMERA:-/dev/video0}"
DUREE="${CC_DUREE:-30}"
AIDE="${SERMO_SOURCE_DIR:-/usr/libexec/sermo}/sermo-cible-media"
PORTE="${CC_APPROBATION:-$ICI/../approbation/approbation.sh}"

xml() { local s=${1//&/"&amp;"}; s=${s//</"&lt;"}; s=${s//>/"&gt;"}; printf '%s' "${s//\"/"&quot;"}"; }
# Encodage « pour cent » pour l'adresse media:// (seuls A-Z a-z 0-9 / . _ - restent).
pourcent() {
	local LC_ALL=C s=$1 i c out=""
	for ((i = 0; i < ${#s}; i++)); do
		c=${s:i:1}
		case "$c" in [A-Za-z0-9/._-]) out+=$c ;; *) out+=$(printf '%%%02X' "'$c") ;; esac
	done
	printf '%s' "$out"
}
dossier_de() { local d; d=$(xdg-user-dir "$1" 2>/dev/null); [ -n "$d" ] && [ "$d" != "$HOME" ] || d="$HOME"; printf '%s' "$d"; }

[[ "$CAMERA" =~ ^/dev/video[0-9]{1,3}$ ]] || { echo "CC_CAMERA : /dev/videoN attendu" >&2; exit 64; }
[[ "$DUREE" =~ ^[0-9]{1,3}$ ]] && [ "$DUREE" -ge 1 ] && [ "$DUREE" -le 600 ] || { echo "CC_DUREE : 1 à 600 secondes" >&2; exit 64; }

# ── Un geste après confirmation : photo ou vidéo ────────────────────────────
capturer() {   # capturer photo|video
	local genre=$1 dossier fichier titre question detail adresse
	[ -x "$AIDE" ] || { echo "sermo-cible-media absent ($AIDE) : paquet sermo-cible-media" >&2; exit 2; }
	[ -x "$PORTE" ] || { echo "porte d'approbation absente ($PORTE)" >&2; exit 2; }
	if [ "$genre" = photo ]; then
		dossier=${CC_PHOTOS:-$(dossier_de PICTURES)}
		fichier="$dossier/sermo-photo-$(date +%Y-%m-%d-%H%M%S).jpg"
		titre="Prendre une photo"
		question="Écrire une photo de la caméra dans ce fichier ?"
		detail=$(printf 'Fichier :\n%s\n\nUne image, lisible par vous seul (mode 600).' "$fichier")
	else
		dossier=${CC_VIDEOS:-$(dossier_de VIDEOS)}
		fichier="$dossier/sermo-video-$(date +%Y-%m-%d-%H%M%S).mkv"
		titre="Enregistrer la caméra"
		question="Enregistrer la vidéo de la caméra dans ce fichier ?"
		detail=$(printf 'Fichier :\n%s\n\nDurée maximale : %s s (bouton « Couper » pour finir avant).\nLisible par vous seul (mode 600).' "$fichier" "$DUREE")
	fi
	SERMO_APPROBATION_BIN="${SERMO_APPROBATION_BIN:-$SERMO}" "$PORTE" --titre "$titre" --question "$question" --detail "$detail"
	case $? in
		0) ;;
		*) echo "annulé : rien n'a été écrit"; exit 1 ;;
	esac
	adresse="media://camera?commande=$genre"
	[ "$genre" = video ] && adresse="media://camera?commande=enregistrer"
	adresse+="&fichier=$(pourcent "$fichier")"
	[ "$genre" = video ] && adresse+="&duree=$DUREE"
	# Le programme d'aide décode l'adresse ; « & » sépare les clés.
	"$AIDE" "${adresse//&/%26}"
}

case "${1:-}" in
	--photo) capturer photo; exit $? ;;
	--video) capturer video; exit $? ;;
esac

# ── La fenêtre ──────────────────────────────────────────────────────────────
MOI=$(xml "$(printf '%q' "$0")")
CAM=$(xml "$CAMERA")
export CAMERA_CAPTURE="<window title=\"Caméra : photo et vidéo\" resizable=\"true\">
<vbox>
  <text><label>La caméra s'affiche ici après « Allumer ». Rien n'est enregistré sans votre confirmation.</label></text>
  <drawingarea width-request=\"640\" height-request=\"480\" embed=\"true\" media=\"camera\"><variable>ECRAN</variable></drawingarea>
  <hbox>
    <button><label>Allumer</label><action media>media://camera?commande=lire&amp;source=v4l2://$CAM&amp;dans=\$ECRAN</action></button>
    <button><label>Photo…</label><action>$MOI --photo</action></button>
    <button><label>Enregistrer ${DUREE} s…</label><action>$MOI --video</action></button>
    <button><label>Couper</label><action media>media://camera?commande=couper</action></button>
    <button><label>Éteindre</label><action media>media://camera?commande=arreter</action></button>
    <button><label>Fermer</label><action media>media://camera?commande=arreter</action><action>EXIT:fermer</action></button>
  </hbox>
</vbox>
</window>"

case "${1:-}" in
	--xml) printf '%s\n' "$CAMERA_CAPTURE"; exit 0 ;;
	-*) echo "usage : $0 [binaire sermo] | --xml" >&2; exit 64 ;;
	"") ;;
	*) SERMO=$1 ;;
esac
command -v "$SERMO" >/dev/null || { echo "binaire sermo introuvable : $SERMO" >&2; exit 2; }
"$SERMO" --program=CAMERA_CAPTURE >/dev/null
