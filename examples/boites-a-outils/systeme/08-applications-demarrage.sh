#!/bin/bash
# SPDX-License-Identifier: CC0-1.0
# Boîte    : Système
# Besoin   : choisir les applications lancées à l'ouverture de session : en désactiver, en réactiver, en ajouter, en retirer
# Enrobe   : fichiers .desktop de ~/.config/autostart (norme XDG Autostart) ; cp, sed, rm
# Ports    : gtk3sermo ou qt6sermo conseillés ; ncursessermo possible
# Terminal : rien de perdu
# Usage    : ./08-applications-demarrage.sh [binaire sermo]
#            SERMO_A_BLANC=0 pour agir pour de vrai (défaut : la commande est affichée)
#            Désactiver une entrée du système (/etc/xdg/autostart) en pose une copie
#            « Hidden=true » dans ~/.config/autostart : le fichier du système n'est pas touché.

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

PERSO=${XDG_CONFIG_HOME:-$HOME/.config}/autostart
SYSTEME=/etc/xdg/autostart

# Arguments : des lignes « fichier.desktop|nom|actif|origine ».
dialogue_principal() {
	local lignes=("$@") l
	[ ${#lignes[@]} -gt 0 ] || lignes=(
		"blueman.desktop|Blueman Applet|oui|système"
		"nm-applet.desktop|Network|oui|système"
		"redshift-gtk.desktop|Redshift|non|personnel"
		"xfce4-clipman-plugin-autostart.desktop|Clipman|oui|système"
		"syncthing-start.desktop|Start Syncthing|oui|personnel")
	cat <<XML
<window title="Applications au démarrage" default-width="660" default-height="520">
  <vbox>
    <tree scrollable="true" height-request="170">
      <label>Fichier|Nom|Active|Origine</label>
      <variable>ENTREE</variable>
XML
	for l in "${lignes[@]}"; do printf '      <item>%s</item>\n' "$(xml "$l")"; done
	cat <<XML
    </tree>
    <frame label="Sur l'entrée choisie">
      <hbox>
        <radiobutton><label>Désactiver</label><default>true</default><variable>A_DESACTIVER</variable></radiobutton>
        <radiobutton><label>Réactiver</label><variable>A_ACTIVER</variable></radiobutton>
        <radiobutton><label>Retirer (entrée personnelle)</label><variable>A_RETIRER</variable></radiobutton>
        <button><label>Appliquer</label><action>exit:modifier</action></button>
      </hbox>
    </frame>
    <expander label="Ajouter une application">
      <vbox>
        <hbox><text><label>Nom :</label></text>
          <entry space-expand="true" space-fill="true"><variable>NOUVEAU_NOM</variable><default>Bloc-notes du matin</default></entry></hbox>
        <hbox><text><label>Commande :</label></text>
          <entry space-expand="true" space-fill="true"><variable>NOUVELLE_COMMANDE</variable><default>$(xml "mousepad $HOME/Documents/notes.txt")</default></entry></hbox>
        <hbox><text><label>Attendre avant de lancer (s) :</label></text>
          <spinbutton range-min="0" range-max="120" range-step="5"><variable>DELAI</variable><default>0</default></spinbutton></hbox>
        <hbox>
          <button><label>Ajouter</label><action>exit:ajouter</action></button>
        </hbox>
      </vbox>
      <variable>AJOUT_OUVERT</variable>
    </expander>
    <hbox>
      <button cancel></button>
    </hbox>
  </vbox>
</window>
XML
}

dialogue_confirmation() {
	local fichier=${1:-$HOME/.config/autostart/redshift-gtk.desktop}
	cat <<XML
<window title="Retirer du démarrage ?">
  <vbox>
    <text><label>Le fichier $(xml "$fichier") sera supprimé.</label></text>
    <text><label>L'application reste installée ; seul son lancement automatique disparaît.</label></text>
    <hbox>
      <button cancel></button>
      <button><label>Retirer</label><action>exit:retirer</action></button>
    </hbox>
  </vbox>
</window>
XML
}

# Valeur d'une clé dans le groupe [Desktop Entry] d'un fichier .desktop.
cle() { awk -F= -v k="$2" '/^\[/ { g = ($0 == "[Desktop Entry]") } g && $1 == k { sub(/^[^=]*=/, ""); print; exit }' "$1" 2>/dev/null; }

# Une ligne par entrée : « fichier|nom|actif|origine ». Le fichier personnel masque celui du système.
entrees() {
	local f base nom actif origine
	declare -A vu
	for f in "$PERSO"/*.desktop "$SYSTEME"/*.desktop; do
		[ -f "$f" ] || continue
		base=${f##*/}
		[ -n "${vu[$base]:-}" ] && continue
		vu[$base]=1
		nom=$(cle "$f" Name); actif=oui
		[ "$(cle "$f" Hidden)" = true ] && actif=non
		[ "$(cle "$f" X-GNOME-Autostart-enabled)" = false ] && actif=non
		origine=système; [ "${f%/*}" = "$PERSO" ] && origine=personnel
		printf '%s|%s|%s|%s\n' "$base" "${nom//|/ }" "$actif" "$origine"
	done
}

# Écrit une entrée personnelle (appelée par agir : affichée à blanc).
ecrire_entree() {
	local fichier=$1 nom=$2 commande=$3 delai=$4
	{
		echo "[Desktop Entry]"
		echo "Type=Application"
		echo "Name=$nom"
		echo "Exec=$commande"
		echo "X-GNOME-Autostart-Delay=$delai"
		echo "Hidden=false"
	} > "$fichier"
}

nom_fichier_valide() { [[ $1 =~ ^[A-Za-z0-9][A-Za-z0-9._-]*\.desktop$ ]]; }

suite() {
	# La sortie standard de sermo part vers /dev/null : écrire sur la sortie d'erreur.
	exec >&2
	case "$1" in
	principal)
		case "$EXIT" in
		modifier)
			local base=${ENTREE%%|*}
			nom_fichier_valide "$base" || { echo "Aucune entrée choisie, ou nom refusé : $base" >&2; exit 1; }
			local perso="$PERSO/$base" sys="$SYSTEME/$base"
			[ -f "$perso" ] || [ -f "$sys" ] || { echo "Entrée introuvable : $base" >&2; exit 1; }
			if [ "$A_DESACTIVER" = true ]; then
				[ -f "$perso" ] || { agir mkdir -p -- "$PERSO"; agir cp -- "$sys" "$perso"; }
				agir sed -i -e '/^Hidden=/d' -e '/^\[Desktop Entry\]$/a Hidden=true' -- "$perso"
			elif [ "$A_ACTIVER" = true ]; then
				[ -f "$perso" ] || { echo "Déjà active : l'entrée vient du système et n'a pas de copie personnelle."; exit 0; }
				agir sed -i -e '/^Hidden=/d' -e '/^X-GNOME-Autostart-enabled=/d' -- "$perso"
			elif [ "$A_RETIRER" = true ]; then
				[ -f "$perso" ] || { echo "Seule une entrée personnelle se retire ; celle-ci vient du système : désactivez-la." >&2; exit 1; }
				export ENTREE
				export DIALOGUE; DIALOGUE=$(dialogue_confirmation "$perso")
				"$SERMO" --program=DIALOGUE --do='"$SERMO_SCRIPT" --suite retrait' >/dev/null
			fi
			;;
		ajouter)
			[[ $NOUVEAU_NOM =~ ^[[:alnum:]][[:alnum:]\ ._\'-]{0,59}$ ]] || { echo "Nom refusé (lettres, chiffres, espace, . _ ' -) : $NOUVEAU_NOM" >&2; exit 1; }
			# Exec= a ses propres règles d'échappement : on n'accepte que des mots simples.
			[[ $NOUVELLE_COMMANDE =~ ^[A-Za-z0-9/._+][A-Za-z0-9\ /._+:=,-]{0,199}$ ]] ||
				{ echo "Commande refusée (pas de guillemets, \$, %, ;, | ni &) : $NOUVELLE_COMMANDE" >&2; exit 1; }
			command -v -- "${NOUVELLE_COMMANDE%% *}" >/dev/null || { echo "Programme introuvable : ${NOUVELLE_COMMANDE%% *}" >&2; exit 1; }
			[[ $DELAI =~ ^[0-9]{1,3}$ ]] && [ "$DELAI" -le 120 ] || { echo "Délai hors bornes : $DELAI" >&2; exit 1; }
			local base nom=${NOUVEAU_NOM,,}
			nom=${nom// /-}; nom=${nom//[^a-z0-9._-]/}
			base="perso-${nom:-application}.desktop"
			nom_fichier_valide "$base" || { echo "Nom de fichier refusé : $base" >&2; exit 1; }
			[ -e "$PERSO/$base" ] && { echo "Existe déjà : $PERSO/$base" >&2; exit 1; }
			agir mkdir -p -- "$PERSO"
			agir ecrire_entree "$PERSO/$base" "$NOUVEAU_NOM" "$NOUVELLE_COMMANDE" "$((10#$DELAI))"
			;;
		*) exit 0 ;;
		esac
		;;
	retrait)
		[ "$EXIT" = retirer ] || exit 0
		local base=${ENTREE%%|*}
		nom_fichier_valide "$base" && [ -f "$PERSO/$base" ] || { echo "Entrée personnelle introuvable : $base" >&2; exit 1; }
		agir rm -- "$PERSO/$base"
		;;
	esac
}

main() {
	if [ "${1:-}" = --suite ]; then suite "$2"; exit; fi
	[ -n "${1:-}" ] && SERMO=$1
	local lignes=()
	mapfile -t lignes < <(entrees)
	[ ${#lignes[@]} -gt 0 ] || echo "Aucune entrée de démarrage trouvée : la liste montre un exemple." >&2
	export DIALOGUE; DIALOGUE=$(dialogue_principal "${lignes[@]}")
	"$SERMO" --program=DIALOGUE --do='"$SERMO_SCRIPT" --suite principal' >/dev/null
}

[ "${BASH_SOURCE[0]}" = "$0" ] && main "$@"
