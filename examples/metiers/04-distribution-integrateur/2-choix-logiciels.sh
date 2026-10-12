#!/bin/bash
# SPDX-License-Identifier: CC0-1.0
# Métier   : Intégrateur de distribution
# Besoin   : choisir un profil de poste et des logiciels, voir ce qu'apt installera, puis installer
# Enrobe   : apt-get (simulation -s, puis install)
# Ports    : gtk3sermo conseillé ; ncursessermo par SSH
# Terminal : rien de perdu
# Usage    : ./2-choix-logiciels.sh [binaire sermo]
#            SERMO_A_BLANC=0 pour agir pour de vrai (défaut : la commande est affichée)
#            L'installation demande les droits d'administration : lancer alors en root.

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

dialogue_principal() {
	cat <<XML
<window title="Choix des logiciels" default-width="600">
  <vbox>
    <frame label="Profil du poste">
      <vbox>
        <radiobutton><label>Poste de base : navigateur, visionneuse, éditeur de texte</label><variable>PROFIL_BASE</variable><default>true</default></radiobutton>
        <radiobutton><label>Bureautique : base + LibreOffice, courrier, correcteur français</label><variable>PROFIL_BUREAUTIQUE</variable></radiobutton>
        <radiobutton><label>Développement : base + git, compilateur, meson, débogueur</label><variable>PROFIL_DEV</variable></radiobutton>
        <radiobutton><label>Multimédia : base + VLC, Audacity, GIMP, Inkscape</label><variable>PROFIL_MULTIMEDIA</variable></radiobutton>
      </vbox>
    </frame>
    <frame label="Logiciels en plus">
      <notebook tab-labels="Internet|Documents|Graphisme|Outils">
        <vbox>
          <checkbox><label>Firefox en français (firefox-esr-l10n-fr)</label><variable>L_FIREFOX_FR</variable><default>true</default></checkbox>
          <checkbox><label>Thunderbird, courrier (thunderbird)</label><variable>L_THUNDERBIRD</variable></checkbox>
          <checkbox><label>Transmission, téléchargements BitTorrent (transmission-gtk)</label><variable>L_TRANSMISSION</variable></checkbox>
        </vbox>
        <vbox>
          <checkbox><label>Evince, lecteur PDF (evince)</label><variable>L_EVINCE</variable><default>true</default></checkbox>
          <checkbox><label>Simple Scan, numérisation (simple-scan)</label><variable>L_SCAN</variable></checkbox>
          <checkbox><label>KeePassXC, mots de passe (keepassxc)</label><variable>L_KEEPASSXC</variable></checkbox>
        </vbox>
        <vbox>
          <checkbox><label>GIMP, retouche photo (gimp)</label><variable>L_GIMP</variable></checkbox>
          <checkbox><label>Inkscape, dessin vectoriel (inkscape)</label><variable>L_INKSCAPE</variable></checkbox>
          <checkbox><label>darktable, développement RAW (darktable)</label><variable>L_DARKTABLE</variable></checkbox>
        </vbox>
        <vbox>
          <checkbox><label>GParted, partitions (gparted)</label><variable>L_GPARTED</variable></checkbox>
          <checkbox><label>Timeshift, instantanés du système (timeshift)</label><variable>L_TIMESHIFT</variable></checkbox>
          <checkbox><label>htop, processus (htop)</label><variable>L_HTOP</variable><default>true</default></checkbox>
        </vbox>
      </notebook>
    </frame>
    <checkbox><label>Installer aussi les paquets recommandés (plus complet, plus lourd)</label><variable>RECOMMANDES</variable></checkbox>
    <hbox>
      <button cancel></button>
      <button><label>Voir ce qui sera installé</label><action>exit:preparer</action></button>
    </hbox>
  </vbox>
</window>
XML
}

dialogue_confirmation() {
	local paquets=${1:-firefox-esr ristretto mousepad htop} resume=${2:-4 paquets nouveaux, 2 dépendances}
	cat <<XML
<window title="Confirmer l'installation" default-width="520" default-height="360">
  <vbox>
    <text><label>Simulation faite : $(xml "$resume").</label></text>
    <text><label>Paquets demandés :</label></text>
    <edit editable="false" wrap-mode="word" height-request="160"><variable>LISTE</variable><default>$(xml "$paquets")</default></edit>
    <text><label>Lancer l'installation ?</label></text>
    <hbox>
      <button cancel></button>
      <button><label>Installer</label><action>exit:installer</action></button>
    </hbox>
  </vbox>
</window>
XML
}

# Paquets retenus, un par ligne : ceux du profil, puis les cases cochées.
paquets() {
	echo firefox-esr; echo ristretto; echo mousepad
	if [ "$PROFIL_BUREAUTIQUE" = true ]; then
		printf '%s\n' libreoffice-writer libreoffice-calc libreoffice-impress libreoffice-l10n-fr hunspell-fr thunderbird-l10n-fr
	elif [ "$PROFIL_DEV" = true ]; then
		printf '%s\n' git build-essential meson pkgconf gdb geany
	elif [ "$PROFIL_MULTIMEDIA" = true ]; then
		printf '%s\n' vlc audacity gimp inkscape
	fi
	[ "$L_FIREFOX_FR" = true ]   && echo firefox-esr-l10n-fr
	[ "$L_THUNDERBIRD" = true ]  && echo thunderbird
	[ "$L_TRANSMISSION" = true ] && echo transmission-gtk
	[ "$L_EVINCE" = true ]       && echo evince
	[ "$L_SCAN" = true ]         && echo simple-scan
	[ "$L_KEEPASSXC" = true ]    && echo keepassxc
	[ "$L_GIMP" = true ]         && echo gimp
	[ "$L_INKSCAPE" = true ]     && echo inkscape
	[ "$L_DARKTABLE" = true ]    && echo darktable
	[ "$L_GPARTED" = true ]      && echo gparted
	[ "$L_TIMESHIFT" = true ]    && echo timeshift
	[ "$L_HTOP" = true ]         && echo htop
}

suite() {
	# La sortie standard de sermo part vers /dev/null : écrire sur la sortie d'erreur.
	exec >&2
	local reco=--no-install-recommends
	case "$1" in
	principal)
		[ "$EXIT" = preparer ] || exit 0
		export PAQUETS; PAQUETS=$(paquets | sort -u | tr '\n' ' ')
		export RECOMMANDES
		[ "$RECOMMANDES" = true ] && reco=--install-recommends
		# La simulation (-s) ne modifie rien et ne demande aucun droit.
		local sortie nouveaux
		# shellcheck disable=SC2086
		sortie=$(apt-get -s "$reco" install $PAQUETS 2>&1)
		if [ $? -ne 0 ]; then
			echo "apt-get refuse la simulation :"; echo "$sortie" | tail -n 5; exit 1
		fi
		nouveaux=$(grep -c '^Inst ' <<< "$sortie")
		export DIALOGUE; DIALOGUE=$(dialogue_confirmation "$PAQUETS" "$nouveaux paquet(s) à installer, dépendances comprises")
		"$SERMO" --program=DIALOGUE --do='"$SERMO_SCRIPT" --suite confirmation' >/dev/null
		;;
	confirmation)
		[ "$EXIT" = installer ] || exit 0
		[ "$RECOMMANDES" = true ] && reco=--install-recommends
		local liste
		read -r -a liste <<< "$PAQUETS"
		[ "$(id -u)" = 0 ] || echo "Droits d'administration nécessaires : lancez en root la commande ci-dessous."
		agir apt-get install -y "$reco" "${liste[@]}"
		;;
	esac
}

main() {
	if [ "${1:-}" = --suite ]; then suite "$2"; exit; fi
	[ -n "${1:-}" ] && SERMO=$1
	command -v apt-get >/dev/null || { echo "apt-get est absent : ce poste n'est pas une Debian." >&2; exit 1; }
	export DIALOGUE; DIALOGUE=$(dialogue_principal)
	"$SERMO" --program=DIALOGUE --do='"$SERMO_SCRIPT" --suite principal' >/dev/null
}

[ "${BASH_SOURCE[0]}" = "$0" ] && main "$@"
