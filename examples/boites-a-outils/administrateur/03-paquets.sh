#!/bin/bash
# SPDX-License-Identifier: CC0-1.0
# Boîte    : Administrateur
# Besoin   : chercher un paquet Debian, puis l'installer ou le retirer ; la purge (configuration effacée) se confirme
# Enrobe   : apt-cache search (lecture) ; apt install / apt remove / apt purge
# Ports    : gtk3sermo conseillé ; ncursessermo par SSH
# Terminal : rien de perdu
# Usage    : ./03-paquets.sh [binaire sermo]
#            SERMO_A_BLANC=0 pour agir pour de vrai (défaut : la commande est affichée)
#            Installer ou retirer demande les droits root : sans eux, le script le dit et
#            affiche la commande. apt pose lui-même ses questions dans le terminal.

export SERMO="${SERMO:-sermo}"
export SERMO_A_BLANC="${SERMO_A_BLANC:-1}"
# Le script lui-même, pour --do : le shell de sermo lit la variable, sans
# citation à refaire (printf %q écrit $'…' en locale C, que dash lit de travers).
export SERMO_SCRIPT="$0"

# Échappe une valeur avant de l'insérer dans le XML.
xml() { local s=${1//&/"&amp;"}; s=${s//</"&lt;"}; s=${s//>/"&gt;"}; printf '%s' "${s//\"/"&quot;"}"; }

# Lance une commande, ou l'affiche seulement en mode à blanc.
# Sans les droits root, ne lance rien : affiche la commande à lancer en root.
agir() {
	if [ "$SERMO_A_BLANC" = 1 ]; then
		printf 'à blanc :'; printf ' %q' "$@"; printf '\n'
	elif [ "$EUID" -ne 0 ]; then
		printf 'Droits root nécessaires. Commande à lancer en root :'; printf ' %q' "$@"; printf '\n'
		return 1
	else
		"$@"
	fi
}

dialogue_recherche() {
	local terme=${1:-pdf viewer}
	cat <<XML
<window title="Paquets : chercher" default-width="480">
  <vbox>
    <hbox><text><label>Chercher :</label></text>
      <entry space-expand="true" space-fill="true"><variable>TERME</variable><default>$(xml "$terme")</default>
        <action signal="activate">exit:chercher</action></entry></hbox>
    <checkbox><label>Chercher dans le nom seulement (--names-only)</label><variable>NOMS_SEULS</variable></checkbox>
    <hbox><text><label>Résultats au plus :</label></text>
      <spinbutton range-min="10" range-max="200" range-step="10"><variable>MAX</variable><default>50</default></spinbutton></hbox>
    <hbox>
      <button cancel></button>
      <button><label>Chercher</label><action>exit:chercher</action></button>
    </hbox>
  </vbox>
</window>
XML
}

# Arguments : une ligne par paquet trouvé, « nom|installé|description ».
dialogue_resultats() {
	local lignes=("$@") l
	[ ${#lignes[@]} -gt 0 ] || lignes=(
		"atril|non|MATE document viewer"
		"evince|oui|Document (PostScript, PDF) viewer"
		"mupdf|non|lightweight PDF viewer"
		"qpdfview|non|tabbed document viewer"
		"zathura-pdf-poppler|non|PDF support for zathura")
	cat <<XML
<window title="Paquets : résultats" default-width="640" default-height="480">
  <vbox>
    <tree scrollable="true" space-expand="true" space-fill="true" exported-column="0">
      <label>Paquet|Installé|Description</label>
      <variable>PAQUET</variable>
XML
	for l in "${lignes[@]}"; do printf '      <item>%s</item>\n' "$(xml "$l")"; done
	cat <<XML
    </tree>
    <frame label="Que faire du paquet choisi ?">
      <hbox>
        <radiobutton><label>Installer</label><default>true</default><variable>A_INSTALLER</variable></radiobutton>
        <radiobutton><label>Retirer</label><variable>A_RETIRER</variable></radiobutton>
        <radiobutton><label>Purger (retirer et effacer sa configuration)</label><variable>A_PURGER</variable></radiobutton>
      </hbox>
    </frame>
    <expander label="Options d'apt">
      <vbox>
        <checkbox><label>Sans les paquets recommandés (--no-install-recommends)</label><variable>SANS_RECO</variable></checkbox>
        <checkbox><label>Simuler seulement (--simulate), même en mode réel</label><variable>SIMULER</variable></checkbox>
      </vbox>
      <variable>OPTIONS_OUVERTES</variable>
    </expander>
    <hbox>
      <button cancel></button>
      <button><label>Appliquer</label><action>exit:appliquer</action></button>
    </hbox>
  </vbox>
</window>
XML
}

dialogue_purge() {
	local paquet=${1:-evince}
	cat <<XML
<window title="Purger le paquet ?">
  <vbox>
    <text><label>Le paquet $(xml "$paquet") sera retiré, avec ses fichiers de configuration sous /etc.</label></text>
    <text><label>Les réglages faits à la main dans ces fichiers seront perdus.</label></text>
    <hbox>
      <button cancel></button>
      <button><label>Purger</label><action>exit:purger</action></button>
    </hbox>
  </vbox>
</window>
XML
}

# Commande apt pour l'action choisie, un argument par ligne.
commande_apt() {
	echo apt
	if [ "$A_PURGER" = true ]; then echo purge
	elif [ "$A_RETIRER" = true ]; then echo remove
	else echo install; [ "$SANS_RECO" = true ] && echo --no-install-recommends
	fi
	[ "$SIMULER" = true ] && echo --simulate
	echo --; echo "$PAQUET"
}

suite() {
	# La sortie standard de sermo part vers /dev/null : écrire sur la sortie d'erreur.
	exec >&2
	case "$1" in
	recherche)
		[ "$EXIT" = chercher ] || exit 0
		[[ $TERME =~ ^[[:alnum:][:space:]+.:-]{2,60}$ ]] || { echo "Recherche refusée (lettres, chiffres, + . : - ; 2 à 60 caractères)." >&2; exit 1; }
		[[ $MAX =~ ^[0-9]+$ ]] && [ "$MAX" -ge 10 ] && [ "$MAX" -le 200 ] || MAX=50
		opts=(); [ "$NOMS_SEULS" = true ] && opts=(--names-only)
		read -ra mots <<<"$TERME"
		mapfile -t trouves < <(apt-cache search "${opts[@]}" -- "${mots[@]}" | head -n "$MAX")
		[ ${#trouves[@]} -gt 0 ] || { echo "Aucun paquet ne correspond à : $TERME"; exit 0; }
		mapfile -t installes < <(dpkg-query -W -f='${db:Status-Abbrev}|${Package}\n' 2>/dev/null | awk -F'|' '$1 ~ /^ii/ {print $2}')
		declare -A est
		for p in "${installes[@]}"; do est[$p]=1; done
		lignes=()
		for t in "${trouves[@]}"; do
			nom=${t%% - *}; desc=${t#* - }
			lignes+=("$nom|$([ -n "${est[$nom]:-}" ] && echo oui || echo non)|$desc")
		done
		export DIALOGUE; DIALOGUE=$(dialogue_resultats "${lignes[@]}")
		"$SERMO" --program=DIALOGUE --do='"$SERMO_SCRIPT" --suite resultats' >/dev/null
		;;
	resultats)
		[ "$EXIT" = appliquer ] || exit 0
		[[ $PAQUET =~ ^[a-z0-9][a-z0-9+.-]+(:[a-z0-9-]+)?$ ]] || { echo "Choisissez d'abord un paquet dans la liste." >&2; exit 1; }
		if [ "$A_PURGER" = true ]; then
			export PAQUET A_PURGER A_RETIRER SANS_RECO SIMULER
			export DIALOGUE; DIALOGUE=$(dialogue_purge "$PAQUET")
			"$SERMO" --program=DIALOGUE --do='"$SERMO_SCRIPT" --suite purge' >/dev/null
			exit
		fi
		mapfile -t cmd < <(commande_apt)
		agir "${cmd[@]}"
		;;
	purge)
		[ "$EXIT" = purger ] || exit 0
		mapfile -t cmd < <(commande_apt)
		agir "${cmd[@]}"
		;;
	esac
}

main() {
	if [ "${1:-}" = --suite ]; then suite "$2"; exit; fi
	[ -n "${1:-}" ] && SERMO=$1
	command -v apt >/dev/null || { echo "apt est absent : ce script vise Debian et ses dérivées." >&2; exit 1; }
	export DIALOGUE; DIALOGUE=$(dialogue_recherche)
	"$SERMO" --program=DIALOGUE --do='"$SERMO_SCRIPT" --suite recherche' >/dev/null
}

[ "${BASH_SOURCE[0]}" = "$0" ] && main "$@"
