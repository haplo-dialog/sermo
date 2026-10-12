#!/bin/bash
# SPDX-License-Identifier: CC0-1.0
# Boîte    : Système
# Besoin   : choisir l'application qui ouvre les liens web, les PDF, les textes et les images
# Enrobe   : xdg-settings (navigateur) ; xdg-mime (types de fichiers)
# Ports    : gtk3sermo ou qt6sermo conseillés ; ncursessermo possible
# Terminal : rien de perdu
# Usage    : ./09-applications-par-defaut.sh [binaire sermo]
#            SERMO_A_BLANC=0 pour agir pour de vrai (défaut : la commande est affichée)
#            Le choix vaut pour votre compte (~/.config/mimeapps.list), pas pour les autres.

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

INCHANGE="(ne pas changer)"
# Clé du dialogue → type MIME qui sert à lister les candidats.
declare -A TYPE=([web]=x-scheme-handler/https [pdf]=application/pdf [texte]=text/plain [image]=image/png)
TYPES_IMAGE=(image/png image/jpeg image/webp image/gif)

# Une liste déroulante : variable, puis les candidats.
liste() {
	local var=$1 c; shift
	printf '<comboboxtext space-expand="true" space-fill="true"><variable>%s</variable><default>%s</default>\n' "$var" "$(xml "$INCHANGE")"
	printf '  <item>%s</item>\n' "$(xml "$INCHANGE")"
	for c in "$@"; do printf '  <item>%s</item>\n' "$(xml "$c")"; done
	printf '</comboboxtext>\n'
}

# Arguments : des lignes « actuel:clé=application.desktop » et « clé:application.desktop »
# (clé : web, pdf, texte, image).
dialogue_principal() {
	local l web=() pdf=() texte=() image=()
	declare -A actuel=()
	for l in "$@"; do
		case "$l" in
		actuel:*) l=${l#actuel:}; actuel[${l%%=*}]=${l#*=} ;;
		web:*) web+=("${l#web:}") ;;
		pdf:*) pdf+=("${l#pdf:}") ;;
		texte:*) texte+=("${l#texte:}") ;;
		image:*) image+=("${l#image:}") ;;
		esac
	done
	if [ $# -eq 0 ]; then
		actuel=([web]=firefox-esr.desktop [pdf]=org.gnome.Evince.desktop [texte]=org.xfce.mousepad.desktop [image]=org.xfce.ristretto.desktop)
		web=(chromium.desktop firefox-esr.desktop)
		pdf=(atril.desktop org.gnome.Evince.desktop libreoffice-draw.desktop)
		texte=(org.xfce.mousepad.desktop vim.desktop)
		image=(gimp.desktop org.xfce.ristretto.desktop eog.desktop)
	fi
	cat <<XML
<window title="Applications par défaut" default-width="600">
  <vbox>
    <table height-request="120">
      <label>Pour ouvrir|Application actuelle</label>
      <item>Liens web|$(xml "${actuel[web]:-aucune}")</item>
      <item>PDF|$(xml "${actuel[pdf]:-aucune}")</item>
      <item>Textes|$(xml "${actuel[texte]:-aucune}")</item>
      <item>Images|$(xml "${actuel[image]:-aucune}")</item>
      <variable>LIGNE</variable>
    </table>
    <notebook tab-labels="Web|Documents|Images">
      <vbox>
        <hbox><text><label>Navigateur :</label></text>
$(liste NAVIGATEUR "${web[@]}")
        </hbox>
      </vbox>
      <vbox>
        <hbox><text><label>PDF :</label></text>
$(liste PDF "${pdf[@]}")
        </hbox>
        <hbox><text><label>Textes :</label></text>
$(liste TEXTE "${texte[@]}")
        </hbox>
      </vbox>
      <vbox>
        <hbox><text><label>Images :</label></text>
$(liste IMAGE "${image[@]}")
        </hbox>
        <text><label>Pour quels formats d'image :</label></text>
        <hbox>
          <checkbox><label>PNG</label><variable>F_PNG</variable><default>true</default></checkbox>
          <checkbox><label>JPEG</label><variable>F_JPEG</variable><default>true</default></checkbox>
          <checkbox><label>WebP</label><variable>F_WEBP</variable><default>true</default></checkbox>
          <checkbox><label>GIF</label><variable>F_GIF</variable></checkbox>
        </hbox>
      </vbox>
    </notebook>
    <hbox>
      <button cancel></button>
      <button><label>Appliquer</label><action>exit:appliquer</action></button>
    </hbox>
  </vbox>
</window>
XML
}

# Applications qui déclarent un type MIME (caches des dossiers d'applications).
candidats() {
	local d
	for d in "${XDG_DATA_HOME:-$HOME/.local/share}/applications" /usr/local/share/applications /usr/share/applications; do
		[ -f "$d/mimeinfo.cache" ] && sed -n "s|^$1=||p" "$d/mimeinfo.cache"
	done | tr ';' '\n' | grep -E '^[A-Za-z0-9][A-Za-z0-9._-]*\.desktop$' | sort -u
}

# Vérifie un choix : soit « ne pas changer », soit un candidat réel pour ce type.
choix_valide() { [ "$2" = "$INCHANGE" ] || grep -qxF -- "$2" < <(candidats "$1"); }

suite() {
	# La sortie standard de sermo part vers /dev/null : écrire sur la sortie d'erreur.
	exec >&2
	case "$1" in
	principal)
		[ "$EXIT" = appliquer ] || exit 0
		choix_valide "${TYPE[web]}" "$NAVIGATEUR" || { echo "Navigateur refusé : $NAVIGATEUR" >&2; exit 1; }
		choix_valide "${TYPE[pdf]}" "$PDF" || { echo "Lecteur PDF refusé : $PDF" >&2; exit 1; }
		choix_valide "${TYPE[texte]}" "$TEXTE" || { echo "Éditeur refusé : $TEXTE" >&2; exit 1; }
		choix_valide "${TYPE[image]}" "$IMAGE" || { echo "Visionneuse refusée : $IMAGE" >&2; exit 1; }
		local rien=1 t
		[ "$NAVIGATEUR" != "$INCHANGE" ] && { agir xdg-settings set default-web-browser "$NAVIGATEUR"; rien=0; }
		[ "$PDF" != "$INCHANGE" ] && { agir xdg-mime default "$PDF" application/pdf; rien=0; }
		[ "$TEXTE" != "$INCHANGE" ] && { agir xdg-mime default "$TEXTE" text/plain; rien=0; }
		if [ "$IMAGE" != "$INCHANGE" ]; then
			local formats=()
			[ "$F_PNG" = true ] && formats+=(image/png)
			[ "$F_JPEG" = true ] && formats+=(image/jpeg)
			[ "$F_WEBP" = true ] && formats+=(image/webp)
			[ "$F_GIF" = true ] && formats+=(image/gif)
			[ ${#formats[@]} -gt 0 ] || { echo "Aucun format d'image coché." >&2; exit 1; }
			for t in "${formats[@]}"; do agir xdg-mime default "$IMAGE" "$t"; done
			rien=0
		fi
		if [ "$rien" = 1 ]; then echo "Rien ne change."; fi
		;;
	esac
}

main() {
	if [ "${1:-}" = --suite ]; then suite "$2"; exit; fi
	[ -n "${1:-}" ] && SERMO=$1
	command -v xdg-mime >/dev/null && command -v xdg-settings >/dev/null ||
		{ echo "xdg-mime ou xdg-settings est absent : installez le paquet xdg-utils." >&2; exit 1; }
	local args=() c k
	args+=("actuel:web=$(xdg-settings get default-web-browser 2>/dev/null)")
	args+=("actuel:pdf=$(xdg-mime query default application/pdf 2>/dev/null)")
	args+=("actuel:texte=$(xdg-mime query default text/plain 2>/dev/null)")
	args+=("actuel:image=$(xdg-mime query default image/png 2>/dev/null)")
	for k in web pdf texte image; do
		while read -r c; do args+=("$k:$c"); done < <(candidats "${TYPE[$k]}")
	done
	export DIALOGUE; DIALOGUE=$(dialogue_principal "${args[@]}")
	"$SERMO" --program=DIALOGUE --do='"$SERMO_SCRIPT" --suite principal' >/dev/null
}

[ "${BASH_SOURCE[0]}" = "$0" ] && main "$@"
