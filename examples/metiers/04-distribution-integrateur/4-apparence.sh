#!/bin/bash
# SPDX-License-Identifier: CC0-1.0
# Métier   : Intégrateur de distribution
# Besoin   : régler l'apparence du bureau : thème, icônes, police, taille et échelle des textes
# Enrobe   : xfconf-query (XFCE), sinon gsettings
# Ports    : gtk3sermo conseillé
# Terminal : sans objet en ncurses (réglage d'une session graphique)
# Usage    : ./4-apparence.sh [binaire sermo]
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

# Une <item> par ligne non vide.
items() {
	local l
	while IFS= read -r l; do [ -n "$l" ] && printf '<item>%s</item>' "$(xml "$l")"; done <<< "$1"
}

# Arguments : thèmes (un par ligne), icônes (un par ligne), thème, icônes, police, taille, DPI actuels.
dialogue_principal() {
	local themes=${1:-$'Adwaita\nAdwaita-dark\nGreybird\nGreybird-dark'}
	local icones=${2:-$'Adwaita\nelementary-xfce\nelementary-xfce-dark'}
	local theme=${3:-Greybird} icone=${4:-elementary-xfce} police=${5:-Sans} taille=${6:-10} dpi=${7:-96}
	cat <<XML
<window title="Apparence du bureau" default-width="520">
  <vbox>
    <frame label="Thème">
      <grid columns="2" row-spacing="4" column-spacing="8">
        <text><label>Fenêtres et boutons :</label></text>
        <comboboxtext space-expand="true" space-fill="true"><variable>THEME</variable><default>$(xml "$theme")</default>$(items "$themes")</comboboxtext>
        <text><label>Icônes :</label></text>
        <comboboxtext space-expand="true" space-fill="true"><variable>ICONES</variable><default>$(xml "$icone")</default>$(items "$icones")</comboboxtext>
      </grid>
    </frame>
    <frame label="Textes">
      <vbox>
        <hbox>
          <text><label>Police :</label></text>
          <comboboxtext space-expand="true" space-fill="true"><variable>POLICE</variable><default>$(xml "$police")</default>
            <item>Sans</item><item>Serif</item><item>DejaVu Sans</item><item>Noto Sans</item><item>Cantarell</item><item>Liberation Sans</item>
          </comboboxtext>
          <text><label>Taille :</label></text>
          <spinbutton range-min="6" range-max="24" range-step="1"><variable>TAILLE</variable><default>$(xml "$taille")</default></spinbutton>
        </hbox>
        <text><label>Échelle des textes, en points par pouce (96 = 100 %) :</label></text>
        <hscale range-min="72" range-max="192" range-step="1" draw-value="true"><variable>DPI</variable><default>$(xml "$dpi")</default></hscale>
        <checkbox><label>Lisser les caractères (anticrénelage)</label><variable>LISSAGE</variable><default>true</default></checkbox>
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

# Dossiers de thèmes qui contiennent un dossier donné (gtk-3.0 pour les thèmes, index.theme pour les icônes).
lister() {
	local d
	for d in "$1"/*/"$2"; do [ -e "$d" ] && basename "$(dirname "$d")"; done | sort -u
}

suite() {
	# La sortie standard de sermo part vers /dev/null : écrire sur la sortie d'erreur.
	exec >&2
	case "$1" in
	principal)
		[ "$EXIT" = appliquer ] || exit 0
		[[ "$TAILLE" =~ ^[0-9]+$ && "$DPI" =~ ^[0-9]+$ ]] || { echo "Taille ou échelle illisible."; exit 1; }
		local lisse=0; [ "$LISSAGE" = true ] && lisse=1
		if command -v xfconf-query >/dev/null; then
			agir xfconf-query -c xsettings -p /Net/ThemeName -s "$THEME"
			agir xfconf-query -c xsettings -p /Net/IconThemeName -s "$ICONES"
			agir xfconf-query -c xsettings -p /Gtk/FontName -s "$POLICE $TAILLE"
			agir xfconf-query -c xsettings -p /Xft/DPI -n -t int -s "$DPI"
			agir xfconf-query -c xsettings -p /Xft/Antialias -n -t int -s "$lisse"
		else
			local facteur
			facteur=$(awk -v d="$DPI" 'BEGIN { printf "%.2f", d / 96 }')
			agir gsettings set org.gnome.desktop.interface gtk-theme "$THEME"
			agir gsettings set org.gnome.desktop.interface icon-theme "$ICONES"
			agir gsettings set org.gnome.desktop.interface font-name "$POLICE $TAILLE"
			agir gsettings set org.gnome.desktop.interface text-scaling-factor "$facteur"
			if [ "$lisse" = 1 ]; then
				agir gsettings set org.gnome.desktop.interface font-antialiasing grayscale
			else
				agir gsettings set org.gnome.desktop.interface font-antialiasing none
			fi
		fi
		;;
	esac
}

main() {
	if [ "${1:-}" = --suite ]; then suite "$2"; exit; fi
	[ -n "${1:-}" ] && SERMO=$1
	command -v xfconf-query >/dev/null || command -v gsettings >/dev/null ||
		{ echo "Ni xfconf-query ni gsettings : installez xfconf (XFCE) ou libglib2.0-bin." >&2; exit 1; }
	local themes icones theme="" icone="" police="" taille="" dpi=""
	themes=$(lister /usr/share/themes gtk-3.0)
	icones=$(lister /usr/share/icons index.theme)
	# Valeurs actuelles, en lecture seule.
	if command -v xfconf-query >/dev/null; then
		theme=$(xfconf-query -c xsettings -p /Net/ThemeName 2>/dev/null)
		icone=$(xfconf-query -c xsettings -p /Net/IconThemeName 2>/dev/null)
		police=$(xfconf-query -c xsettings -p /Gtk/FontName 2>/dev/null)
		dpi=$(xfconf-query -c xsettings -p /Xft/DPI 2>/dev/null)
	else
		theme=$(gsettings get org.gnome.desktop.interface gtk-theme 2>/dev/null | tr -d "'")
		icone=$(gsettings get org.gnome.desktop.interface icon-theme 2>/dev/null | tr -d "'")
		police=$(gsettings get org.gnome.desktop.interface font-name 2>/dev/null | tr -d "'")
	fi
	# « Sans 10 » : la taille est le dernier mot.
	[[ "$police" =~ ^(.*)\ ([0-9]+)$ ]] && { police=${BASH_REMATCH[1]}; taille=${BASH_REMATCH[2]}; }
	[[ "$dpi" =~ ^[0-9]+$ ]] && [ "$dpi" -ge 72 ] || dpi=96
	export DIALOGUE; DIALOGUE=$(dialogue_principal "$themes" "$icones" "$theme" "$icone" "$police" "$taille" "$dpi")
	"$SERMO" --program=DIALOGUE --do='"$SERMO_SCRIPT" --suite principal' >/dev/null
}

[ "${BASH_SOURCE[0]}" = "$0" ] && main "$@"
