#!/bin/bash
# SPDX-License-Identifier: CC0-1.0
# Boîte    : Administrateur
# Besoin   : ajouter un dépôt apt au format deb822, ou en désactiver un, en voyant le fichier avant de l'écrire
# Enrobe   : fichiers /etc/apt/sources.list.d/*.sources (install -m 0644) ; apt update ensuite
# Ports    : gtk3sermo conseillé ; ncursessermo par SSH
# Terminal : rien de perdu
# Usage    : ./04-depots-apt.sh [binaire sermo]
#            SERMO_A_BLANC=0 pour agir pour de vrai (défaut : la commande est affichée)
#            Écrire sous /etc/apt demande les droits root : sans eux, le script le dit et
#            affiche la commande. La clé du dépôt (Signed-By) doit déjà être en place.

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

DOSSIER_SOURCES=/etc/apt/sources.list.d

# Arguments : les fichiers .sources présents (nom seul).
dialogue_principal() {
	local fichiers=("$@") f
	[ ${#fichiers[@]} -gt 0 ] || fichiers=(debian.sources mozilla.sources vscodium.sources)
	cat <<XML
<window title="Dépôts apt" default-width="600" default-height="520">
  <vbox>
    <hbox>
      <radiobutton><label>Ajouter un dépôt</label><default>true</default><variable>A_AJOUTER</variable></radiobutton>
      <radiobutton><label>Désactiver un dépôt existant</label><variable>A_DESACTIVER</variable></radiobutton>
    </hbox>
    <frame label="Nouveau dépôt">
      <vbox>
        <hbox><text><label>Nom du fichier (sans .sources) :</label></text>
          <entry space-expand="true" space-fill="true"><variable>NOM</variable><default>exemple-logiciel</default></entry></hbox>
        <hbox><text><label>Adresse (URIs) :</label></text>
          <entry space-expand="true" space-fill="true"><variable>URI</variable><default>https://depot.exemple.org/debian</default></entry></hbox>
        <hbox><text><label>Version (Suites) :</label></text>
          <comboboxtext><variable>SUITE</variable><default>stable</default>
            <item>stable</item><item>testing</item><item>unstable</item><item>trixie</item><item>forky</item>
          </comboboxtext></hbox>
        <hbox><text><label>Composants :</label></text>
          <checkbox><label>main</label><default>true</default><variable>C_MAIN</variable></checkbox>
          <checkbox><label>contrib</label><variable>C_CONTRIB</variable></checkbox>
          <checkbox><label>non-free</label><variable>C_NONFREE</variable></checkbox>
          <checkbox><label>non-free-firmware</label><variable>C_FIRMWARE</variable></checkbox></hbox>
        <hbox><text><label>Clé (Signed-By) :</label></text>
          <entry space-expand="true" space-fill="true"><variable>CLE</variable><default>/usr/share/keyrings/exemple-logiciel.gpg</default></entry></hbox>
        <checkbox><label>Aussi les sources (Types: deb deb-src)</label><variable>DEB_SRC</variable></checkbox>
      </vbox>
    </frame>
    <frame label="Dépôt à désactiver (Enabled: no)">
      <list scrollable="true" height-request="90"><variable>EXISTANT</variable>
XML
	for f in "${fichiers[@]}"; do printf '        <item>%s</item>\n' "$(xml "$f")"; done
	cat <<XML
      </list>
    </frame>
    <hbox>
      <button cancel></button>
      <button><label>Voir le fichier</label><action>exit:voir</action></button>
    </hbox>
  </vbox>
</window>
XML
}

# Arguments : chemin du fichier, contenu qui sera écrit.
dialogue_apercu() {
	local cible=${1:-/etc/apt/sources.list.d/exemple-logiciel.sources}
	local contenu=${2:-"Types: deb
URIs: https://depot.exemple.org/debian
Suites: stable
Components: main
Signed-By: /usr/share/keyrings/exemple-logiciel.gpg"}
	cat <<XML
<window title="Fichier à écrire" default-width="600" default-height="380">
  <vbox>
    <text><label>Fichier : $(xml "$cible")</label></text>
    <edit editable="false" space-expand="true" space-fill="true"><variable>APERCU</variable><default>$(xml "$contenu")</default></edit>
    <checkbox><label>Lancer apt update ensuite</label><default>true</default><variable>MAJ</variable></checkbox>
    <hbox>
      <button cancel></button>
      <button><label>Écrire le fichier</label><action>exit:ecrire</action></button>
    </hbox>
  </vbox>
</window>
XML
}

# Contenu deb822 d'un nouveau dépôt, d'après les champs du dialogue.
contenu_nouveau() {
	local comp=()
	[ "$C_MAIN" = true ] && comp+=(main)
	[ "$C_CONTRIB" = true ] && comp+=(contrib)
	[ "$C_NONFREE" = true ] && comp+=(non-free)
	[ "$C_FIRMWARE" = true ] && comp+=(non-free-firmware)
	printf 'Types: deb%s\n' "$([ "$DEB_SRC" = true ] && echo ' deb-src')"
	printf 'URIs: %s\nSuites: %s\nComponents: %s\nSigned-By: %s\n' "$URI" "$SUITE" "${comp[*]}" "$CLE"
}

# Contenu d'un fichier existant, chaque paragraphe passé à « Enabled: no ».
contenu_desactive() {
	awk 'BEGIN { p = 0 }
		/^[[:space:]]*$/ { if (p && !e) print "Enabled: no"; p = 0; e = 0; print; next }
		/^Enabled:/ { print "Enabled: no"; e = 1; p = 1; next }
		{ p = 1; print }
		END { if (p && !e) print "Enabled: no" }' "$1"
}

suite() {
	# La sortie standard de sermo part vers /dev/null : écrire sur la sortie d'erreur.
	exec >&2
	case "$1" in
	principal)
		[ "$EXIT" = voir ] || exit 0
		if [ "$A_DESACTIVER" = true ]; then
			[[ $EXISTANT =~ ^[A-Za-z0-9_.-]+\.sources$ ]] || { echo "Choisissez d'abord un fichier dans la liste." >&2; exit 1; }
			CIBLE=$DOSSIER_SOURCES/$EXISTANT
			[ -f "$CIBLE" ] || { echo "Fichier introuvable : $CIBLE" >&2; exit 1; }
			CONTENU=$(contenu_desactive "$CIBLE")
		else
			[[ $NOM =~ ^[a-z0-9][a-z0-9_-]{0,40}$ ]] || { echo "Nom refusé (minuscules, chiffres, - et _) : $NOM" >&2; exit 1; }
			[[ $URI =~ ^https?://[A-Za-z0-9.-]+(:[0-9]+)?(/[A-Za-z0-9._~/-]*)?$ ]] || { echo "Adresse refusée : $URI" >&2; exit 1; }
			case "$SUITE" in stable|testing|unstable|trixie|forky) ;; *) echo "Version refusée : $SUITE" >&2; exit 1 ;; esac
			[ "$C_MAIN$C_CONTRIB$C_NONFREE$C_FIRMWARE" != falsefalsefalsefalse ] || { echo "Cochez au moins un composant." >&2; exit 1; }
			[[ $CLE =~ ^/[A-Za-z0-9._/-]+\.(gpg|asc|pgp)$ ]] || { echo "Chemin de clé refusé : $CLE" >&2; exit 1; }
			[ -f "$CLE" ] || echo "Attention : la clé $CLE n'existe pas encore ; apt update échouera sans elle."
			CIBLE=$DOSSIER_SOURCES/$NOM.sources
			[ -e "$CIBLE" ] && { echo "Ce fichier existe déjà : $CIBLE" >&2; exit 1; }
			[ "${URI#http://}" != "$URI" ] && echo "Attention : adresse en http ; la signature reste vérifiée par la clé."
			CONTENU=$(contenu_nouveau)
		fi
		export CIBLE CONTENU
		export DIALOGUE; DIALOGUE=$(dialogue_apercu "$CIBLE" "$CONTENU")
		"$SERMO" --program=DIALOGUE --do='"$SERMO_SCRIPT" --suite apercu' >/dev/null
		;;
	apercu)
		[ "$EXIT" = ecrire ] || exit 0
		[[ $CIBLE =~ ^/etc/apt/sources\.list\.d/[A-Za-z0-9_.-]+\.sources$ ]] || exit 1
		# Le contenu écrit est celui calculé avant l'aperçu, pas le texte du widget.
		agir install -m 0644 -T /dev/stdin "$CIBLE" <<<"$CONTENU" || exit 1
		[ "$MAJ" = true ] && agir apt update
		;;
	esac
}

main() {
	if [ "${1:-}" = --suite ]; then suite "$2"; exit; fi
	[ -n "${1:-}" ] && SERMO=$1
	command -v apt >/dev/null || { echo "apt est absent : ce script vise Debian et ses dérivées." >&2; exit 1; }
	mapfile -t fichiers < <(find "$DOSSIER_SOURCES" -maxdepth 1 -name '*.sources' -printf '%f\n' 2>/dev/null | sort)
	[ ${#fichiers[@]} -gt 0 ] || fichiers=("(aucun fichier .sources)")
	export DIALOGUE; DIALOGUE=$(dialogue_principal "${fichiers[@]}")
	"$SERMO" --program=DIALOGUE --do='"$SERMO_SCRIPT" --suite principal' >/dev/null
}

[ "${BASH_SOURCE[0]}" = "$0" ] && main "$@"
