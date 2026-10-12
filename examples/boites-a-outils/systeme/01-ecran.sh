#!/bin/bash
# SPDX-License-Identifier: CC0-1.0
# Boîte    : Système
# Besoin   : régler un écran (résolution, fréquence, échelle, place par rapport à un autre écran) et la lisibilité (police, DPI, couleur de fond)
# Enrobe   : xrandr ; xfconf-query (police et DPI sous XFCE) ; xsetroot (couleur de fond sans gestionnaire de bureau)
# Ports    : gtk3sermo ou qt6sermo conseillés
# Terminal : sans objet : xrandr règle une session X, pas un terminal
# Usage    : ./01-ecran.sh [binaire sermo]
#            SERMO_A_BLANC=0 pour agir pour de vrai (défaut : la commande est affichée)
#            Sous Wayland, xrandr ne règle rien : passer par l'outil du compositeur
#            (réglages d'affichage du bureau, wlr-randr, kscreen-doctor…).

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

# Arguments : type de session, puis des lignes « sortie|état|mode|fréquence »
# et des lignes « mode:LxH » (les modes proposés).
dialogue_principal() {
	local session=${1:-x11} l
	shift || true
	local sorties=() modes=()
	for l in "$@"; do
		case "$l" in mode:*) modes+=("${l#mode:}") ;; *) sorties+=("$l") ;; esac
	done
	[ ${#sorties[@]} -gt 0 ] || sorties=(
		"HDMI-1|branché, principal|1920x1080|60.00"
		"eDP-1|branché|1366x768|60.00"
		"DP-1|débranché||")
	[ ${#modes[@]} -gt 0 ] || modes=(1920x1080 1680x1050 1600x900 1366x768 1280x1024 1280x720 1024x768)
	cat <<XML
<window title="Écran" default-width="640">
  <vbox>
    <notebook tab-labels="Disposition|Lisibilité">
      <vbox>
        <tree scrollable="true" height-request="120">
          <label>Sortie|État|Mode|Fréquence (Hz)</label>
          <variable>SORTIE</variable>
XML
	for l in "${sorties[@]}"; do printf '          <item>%s</item>\n' "$(xml "$l")"; done
	cat <<XML
        </tree>
        <frame label="Réglage de la sortie choisie">
          <vbox>
            <hbox><text><label>Résolution :</label></text>
              <comboboxtext><variable>MODE</variable><default>$(xml "${modes[0]}")</default>
XML
	for l in "${modes[@]}"; do printf '                <item>%s</item>\n' "$(xml "$l")"; done
	cat <<XML
              </comboboxtext></hbox>
            <hbox><text><label>Fréquence :</label></text>
              <comboboxtext><variable>FREQUENCE</variable><default>auto</default>
                <item>auto</item><item>50</item><item>60</item><item>75</item><item>100</item><item>120</item><item>144</item><item>165</item>
              </comboboxtext></hbox>
            <hbox><text><label>Échelle (%) :</label></text>
              <hscale range-min="100" range-max="200" range-step="25" draw-value="true" space-expand="true" space-fill="true"><variable>ECHELLE</variable><default>100</default></hscale></hbox>
          </vbox>
        </frame>
        <frame label="Place par rapport à un autre écran">
          <vbox>
            <hbox>
              <radiobutton><label>Ne pas bouger</label><default>true</default><variable>P_RIEN</variable></radiobutton>
              <radiobutton><label>À droite de</label><variable>P_DROITE</variable></radiobutton>
              <radiobutton><label>À gauche de</label><variable>P_GAUCHE</variable></radiobutton>
              <radiobutton><label>Au-dessus de</label><variable>P_DESSUS</variable></radiobutton>
              <radiobutton><label>En miroir de</label><variable>P_MIROIR</variable></radiobutton>
            </hbox>
            <hbox><text><label>Autre écran (nom de sortie) :</label></text>
              <entry><variable>REPERE</variable><default>eDP-1</default></entry></hbox>
          </vbox>
        </frame>
      </vbox>
      <vbox>
        <frame label="Police de l'interface (XFCE)">
          <hbox><text><label>Police :</label></text>
            <fontbutton><variable>POLICE</variable><default>Sans 10</default></fontbutton>
            <checkbox><label>Appliquer la police</label><variable>CHANGER_POLICE</variable></checkbox></hbox>
        </frame>
        <frame label="Taille des textes (XFCE, DPI)">
          <hbox><text><label>DPI :</label></text>
            <spinbutton range-min="72" range-max="192" range-step="12"><variable>DPI</variable><default>96</default></spinbutton>
            <checkbox><label>Appliquer le DPI</label><variable>CHANGER_DPI</variable></checkbox></hbox>
        </frame>
        <frame label="Couleur de fond (session sans gestionnaire de bureau)">
          <hbox><text><label>Couleur :</label></text>
            <colorbutton><variable>FOND</variable><default>#336699</default></colorbutton>
            <checkbox><label>Appliquer la couleur</label><variable>CHANGER_FOND</variable></checkbox></hbox>
        </frame>
      </vbox>
    </notebook>
    <statusbar><variable>STATUT</variable><default>Session : $(xml "$session"). Sous Wayland, ces réglages passent par l'outil du compositeur.</default></statusbar>
    <hbox>
      <button cancel></button>
      <button><label>Appliquer</label><action>exit:appliquer</action></button>
    </hbox>
  </vbox>
</window>
XML
}

# Argument : les commandes, une par argument, telles qu'elles seront lancées.
dialogue_confirmation() {
	local l
	[ $# -gt 0 ] || set -- "xrandr --output HDMI-1 --mode 1920x1080 --rate 60 --scale 1x1"
	cat <<XML
<window title="Confirmer le réglage de l'écran">
  <vbox>
    <text><label>Ces commandes vont être lancées :</label></text>
XML
	for l in "$@"; do printf '    <text><label>%s</label></text>\n' "$(xml "$l")"; done
	cat <<XML
    <text><label>Si l'écran devient noir, attendez : rien n'est enregistré, une nouvelle session revient au réglage d'avant.</label></text>
    <hbox>
      <button cancel></button>
      <button><label>Appliquer</label><action>exit:confirmer</action></button>
    </hbox>
  </vbox>
</window>
XML
}

# Sorties connues de xrandr, une par ligne.
sorties_connues() { xrandr --query 2>/dev/null | awk '$2 == "connected" || $2 == "disconnected" { print $1 }'; }

# Les commandes à lancer, une par ligne, champs séparés par une tabulation.
commandes() {
	local sortie=${SORTIE%%|*} echelle
	if [ -n "$sortie" ]; then
		local cmd=(xrandr --output "$sortie" --mode "$MODE")
		[ "$FREQUENCE" = auto ] || cmd+=(--rate "$FREQUENCE")
		case "$ECHELLE" in 100) echelle=1 ;; 125) echelle=1.25 ;; 150) echelle=1.5 ;; 175) echelle=1.75 ;; 200) echelle=2 ;; esac
		cmd+=(--scale "${echelle}x${echelle}")
		[ "$P_DROITE" = true ] && cmd+=(--right-of "$REPERE")
		[ "$P_GAUCHE" = true ] && cmd+=(--left-of "$REPERE")
		[ "$P_DESSUS" = true ] && cmd+=(--above "$REPERE")
		[ "$P_MIROIR" = true ] && cmd+=(--same-as "$REPERE")
		(IFS=$'\t'; echo "${cmd[*]}")
	fi
	[ "$CHANGER_POLICE" = true ] && printf '%s\t' xfconf-query -c xsettings -p /Gtk/FontName -s "$POLICE" && echo
	# Xft/DPI (xsettings de XFCE) : des points par pouce ; -1 = automatique.
	[ "$CHANGER_DPI" = true ] && printf '%s\t' xfconf-query -c xsettings -p /Xft/DPI -n -t int -s "$DPI" && echo
	[ "$CHANGER_FOND" = true ] && printf '%s\t' xsetroot -solid "$FOND" && echo
	return 0
}

valider() {
	local sortie=${SORTIE%%|*}
	if [ -n "$sortie" ]; then
		[ "${XDG_SESSION_TYPE:-}" = wayland ] && { echo "Session Wayland : xrandr n'y règle rien. Utilisez l'outil d'affichage du compositeur." >&2; return 1; }
		grep -qxF -- "$sortie" < <(sorties_connues) || { echo "Sortie inconnue : $sortie" >&2; return 1; }
		[[ $MODE =~ ^[0-9]{3,5}x[0-9]{3,5}$ ]] || { echo "Résolution refusée : $MODE" >&2; return 1; }
		[[ $FREQUENCE =~ ^(auto|50|60|75|100|120|144|165)$ ]] || { echo "Fréquence refusée : $FREQUENCE" >&2; return 1; }
		[[ $ECHELLE =~ ^(100|125|150|175|200)$ ]] || { echo "Échelle refusée : $ECHELLE" >&2; return 1; }
		if [ "$P_RIEN" != true ]; then
			grep -qxF -- "$REPERE" < <(sorties_connues) || { echo "Autre écran inconnu : $REPERE" >&2; return 1; }
			[ "$REPERE" != "$sortie" ] || { echo "Un écran ne se place pas par rapport à lui-même." >&2; return 1; }
		fi
	fi
	if [ "$CHANGER_POLICE" = true ]; then
		[[ $POLICE =~ ^[[:alnum:]\ ._,-]+\ [0-9]{1,2}$ ]] || { echo "Police refusée : $POLICE" >&2; return 1; }
	fi
	if [ "$CHANGER_DPI" = true ]; then
		[[ $DPI =~ ^[0-9]+$ ]] && [ "$DPI" -ge 72 ] && [ "$DPI" -le 192 ] || { echo "DPI hors bornes : $DPI" >&2; return 1; }
	fi
	if [ "$CHANGER_FOND" = true ]; then
		[[ $FOND =~ ^#[0-9A-Fa-f]{6}$ ]] || { echo "Couleur refusée (attendu #RRVVBB) : $FOND" >&2; return 1; }
	fi
	return 0
}

suite() {
	# La sortie standard de sermo part vers /dev/null : écrire sur la sortie d'erreur.
	exec >&2
	case "$1" in
	principal)
		[ "$EXIT" = appliquer ] || exit 0
		valider || exit 1
		local lignes=()
		mapfile -t lignes < <(commandes | tr '\t' ' ' | sed 's/ $//')
		[ ${#lignes[@]} -gt 0 ] || { echo "Rien à changer : choisissez une sortie ou cochez un réglage." >&2; exit 0; }
		export SORTIE MODE FREQUENCE ECHELLE P_RIEN P_DROITE P_GAUCHE P_DESSUS P_MIROIR REPERE
		export POLICE CHANGER_POLICE DPI CHANGER_DPI FOND CHANGER_FOND
		export DIALOGUE; DIALOGUE=$(dialogue_confirmation "${lignes[@]}")
		"$SERMO" --program=DIALOGUE --do='"$SERMO_SCRIPT" --suite confirmation' >/dev/null
		;;
	confirmation)
		[ "$EXIT" = confirmer ] || exit 0
		# On revalide : l'environnement vient d'un autre processus.
		valider || exit 1
		local ligne cmd
		while IFS= read -r ligne; do
			IFS=$'\t' read -r -a cmd <<<"$ligne"
			[ ${#cmd[@]} -gt 0 ] && agir "${cmd[@]}"
		done < <(commandes)
		;;
	esac
}

main() {
	if [ "${1:-}" = --suite ]; then suite "$2"; exit; fi
	[ -n "${1:-}" ] && SERMO=$1
	command -v xrandr >/dev/null || { echo "xrandr est absent : installez le paquet x11-xserver-utils." >&2; exit 1; }
	local session=${XDG_SESSION_TYPE:-inconnue} lignes=() nom etat reste mode freq
	[ "$session" = wayland ] && echo "Session Wayland : xrandr ne règle rien ici ; utilisez l'outil d'affichage du compositeur." >&2
	# « HDMI-1 connected primary 1920x1080+0+0 … » puis les modes, « * » = mode courant.
	while read -r nom etat reste; do
		mode=""; freq=""
		if [ "$etat" = connected ]; then
			read -r mode freq < <(xrandr --query 2>/dev/null |
				awk -v s="$nom" '$1 == s { dans = 1; next } /^[^ ]/ { dans = 0 }
					dans { for (i = 2; i <= NF; i++) if ($i ~ /\*/) { f = $i; gsub(/[*+]/, "", f); print $1, f; exit } }')
			etat=branché; [[ $reste == primary* ]] && etat="branché, principal"
		else
			etat=débranché
		fi
		lignes+=("$nom|$etat|$mode|$freq")
	done < <(xrandr --query 2>/dev/null | awk '$2 == "connected" || $2 == "disconnected"')
	while read -r mode; do lignes+=("mode:$mode"); done < <(xrandr --query 2>/dev/null |
		awk '/^ +[0-9]+x[0-9]+ / && !vu[$1]++ { print $1 }' | head -20)
	export DIALOGUE; DIALOGUE=$(dialogue_principal "$session" "${lignes[@]}")
	"$SERMO" --program=DIALOGUE --do='"$SERMO_SCRIPT" --suite principal' >/dev/null
}

[ "${BASH_SOURCE[0]}" = "$0" ] && main "$@"
