#!/bin/bash
# SPDX-License-Identifier: CC0-1.0
# Métier   : Bornes, accueil, formation, atelier (consignes sonores), tout script qui joue un son
# Besoin   : lire un son ou une vidéo depuis une fenêtre : lire, pause, reprendre, arrêter,
#            volume, et voir l'état, la position et la durée
# Enrobe   : rien : balises natives <action media> (sermo-cible-media) et <input media>
#            (sermo-source-media), qui pilotent mpv
# Ports    : tous (l'image d'une vidéo s'ouvre dans la fenêtre de mpv : cet exemple
#            n'utilise pas dans= ; voir le manuel pour l'incrustation (gtk3, X11)) ;
#            ncursessermo : le son seul, s'il y a un serveur de son
# Terminal : rien de perdu
# Usage    : ./lecteur-media.sh [FICHIER] [binaire sermo]
#            Sans FICHIER : un son d'essai de 5 s est fabriqué par ffmpeg (440 Hz, doux).
#
# Codes de sortie : 0 fenêtre fermée · 2 sermo, programme d'aide ou ffmpeg absent · 64 fichier refusé.
# La source est écrite dans le XML par ce script : un chemin RÉEL, fichier ordinaire,
# encodé pour l'adresse (« & », « ? », « % » et l'espace ne coupent rien) puis
# échappé pour le XML. Le volume passe par $VOLUME (valeur=$VOLUME), lu par le
# programme d'aide dans l'environnement : jamais relu comme du code.

export SERMO="${SERMO:-sermo}"

xml() { local s=${1//&/"&amp;"}; s=${s//</"&lt;"}; s=${s//>/"&gt;"}; printf '%s' "${s//\"/"&quot;"}"; }
# Encodage « pour cent » d'un chemin pour une valeur d'adresse (le programme d'aide
# décode deux fois : %25 → %, puis la valeur).
encoder() { LC_ALL=C python3 -I -c 'import sys, urllib.parse as u; print(u.quote(u.quote(sys.argv[1], safe="/"), safe="/%"), end="")' "$1"; }

dialogue_principal() {
	local src=$1 nom=$2 a="media://lecteur"
	cat <<XML
<window title="Lecteur — $(xml "$nom")" default-width="460">
  <vbox spacing="8">
    <text><label>$(xml "$nom")</label></text>
    <hbox>
      <button><label>Lire</label><action media>$a?commande=lire&amp;source=$(xml "$src")</action><action>refresh:ETAT</action></button>
      <button><label>Pause</label><action media>$a?commande=pause</action><action>refresh:ETAT</action></button>
      <button><label>Reprendre</label><action media>$a?commande=reprendre</action><action>refresh:ETAT</action></button>
      <button><label>Arrêter</label><action media>$a?commande=arreter</action><action>refresh:ETAT</action></button>
    </hbox>
    <hbox>
      <text><label>Volume</label></text>
      <hscale range-min="0" range-max="100" range-step="5" range-value="60" space-expand="true" space-fill="true">
        <variable>VOLUME</variable>
      </hscale>
      <button><label>Appliquer</label><action media>$a?commande=volume&amp;valeur=\$VOLUME</action></button>
    </hbox>
    <grid columns="2" column-spacing="12">
      <text xalign="0"><label>État</label></text>
      <text xalign="0"><variable>ETAT</variable><input media>$a?donnee=etat</input></text>
      <text xalign="0"><label>Position (s)</label></text>
      <text xalign="0"><variable>POSITION</variable><input media>$a?donnee=position</input></text>
      <text xalign="0"><label>Durée (s)</label></text>
      <text xalign="0"><variable>DUREE</variable><input media>$a?donnee=duree</input></text>
    </grid>
    <timer interval="1" visible="false">
      <variable>HORLOGE</variable>
      <action>refresh:ETAT</action><action>refresh:POSITION</action><action>refresh:DUREE</action>
    </timer>
    <hbox><button><label>Fermer</label><action>exit:fermer</action></button></hbox>
  </vbox>
</window>
XML
}

main() {
	local f=${1:-} reel
	case "$f" in --xml|-*) [ "$f" = --xml ] || { sed -n '/^# Usage/,/^# Codes/p' "$0" | sed '$d;s/^# \{0,1\}//' >&2; exit 64; } ;; esac
	command -v python3 >/dev/null || { echo "python3 est absent" >&2; exit 2; }
	if [ -z "$f" ] || [ "$f" = --xml ]; then
		command -v ffmpeg >/dev/null || { echo "ffmpeg absent : donnez un FICHIER, ou sudo apt install ffmpeg" >&2; exit 2; }
		local d=${XDG_RUNTIME_DIR:-${TMPDIR:-/tmp}}
		f="$d/sermo-lecteur-essai.ogg"
		[ -s "$f" ] || ffmpeg -hide_banner -loglevel error -y -f lavfi -i "sine=frequency=440:duration=5" -af "volume=0.2" "$f" ||
			{ echo "impossible de fabriquer le son d'essai" >&2; exit 2; }
	fi
	reel=$(readlink -f -- "$f") && [ -f "$reel" ] && [ -r "$reel" ] || { echo "fichier refusé : $f (fichier ordinaire lisible)" >&2; exit 64; }
	export DIALOGUE; DIALOGUE=$(dialogue_principal "file://$(encoder "$reel")" "${reel##*/}")
	if [ "${1:-}" = --xml ]; then printf '%s\n' "$DIALOGUE"; exit 0; fi
	[ -n "${2:-}" ] && SERMO=$2
	command -v "$SERMO" >/dev/null || { echo "sermo introuvable : $SERMO" >&2; exit 2; }
	local aide=${SERMO_SOURCE_DIR:-/usr/libexec/sermo}
	[ -x "$aide/sermo-cible-media" ] && [ -x "$aide/sermo-source-media" ] ||
		{ echo "programmes d'aide absents de $aide : sudo apt install sermo-cible-media sermo-source-media mpv" >&2; exit 2; }
	"$SERMO" --program=DIALOGUE >/dev/null
	exit 0
}

[ "${BASH_SOURCE[0]}" = "$0" ] && main "$@"
