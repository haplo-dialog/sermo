#!/bin/bash
# SPDX-License-Identifier: CC0-1.0
# Boîte    : Administrateur
# Besoin   : régler le propriétaire, le mode et une ACL d'un dossier partagé, en confirmant si c'est récursif
# Enrobe   : chown ; chmod ; setfacl (paquet acl) ; getfacl pour l'état actuel
# Ports    : gtk3sermo conseillé ; ncursessermo par SSH
# Terminal : rien de perdu
# Usage    : ./02-droits-fichiers.sh [binaire sermo] [dossier]
#            SERMO_A_BLANC=0 pour agir pour de vrai (défaut : la commande est affichée)
#            Changer de propriétaire demande les droits root : sans eux, le script le dit
#            et affiche la commande.

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

# Arguments : dossier, propriétaire, groupe, puis l'ACL actuelle (une ligne par entrée).
dialogue_principal() {
	local dossier=${1:-/srv/partage/compta} proprio=${2:-root} groupe=${3:-compta}
	shift 3 2>/dev/null || set --
	local acl=("$@") l
	[ ${#acl[@]} -gt 0 ] || acl=("user::rwx" "group::rwx" "group:direction:r-x" "mask::rwx" "other::---")
	cat <<XML
<window title="Droits d'un dossier" default-width="580" default-height="500">
  <vbox>
    <hbox><text><label>Dossier :</label></text>
      <entry space-expand="true" space-fill="true"><variable>DOSSIER</variable><default>$(xml "$dossier")</default></entry></hbox>
    <notebook tab-labels="Propriétaire|Mode|ACL" space-expand="true" space-fill="true">
      <vbox>
        <checkbox><label>Changer le propriétaire et le groupe (chown)</label><variable>F_CHOWN</variable></checkbox>
        <hbox><text><label>Propriétaire :</label></text>
          <entry><variable>PROPRIO</variable><default>$(xml "$proprio")</default></entry></hbox>
        <hbox><text><label>Groupe :</label></text>
          <entry><variable>GROUPE</variable><default>$(xml "$groupe")</default></entry></hbox>
      </vbox>
      <vbox>
        <checkbox><label>Changer le mode (chmod)</label><default>true</default><variable>F_CHMOD</variable></checkbox>
        <hbox><text><label>Propriétaire :</label></text>
          <checkbox><label>lire</label><default>true</default><variable>U_R</variable></checkbox>
          <checkbox><label>écrire</label><default>true</default><variable>U_W</variable></checkbox>
          <checkbox><label>entrer</label><default>true</default><variable>U_X</variable></checkbox></hbox>
        <hbox><text><label>Groupe :</label></text>
          <checkbox><label>lire</label><default>true</default><variable>G_R</variable></checkbox>
          <checkbox><label>écrire</label><default>true</default><variable>G_W</variable></checkbox>
          <checkbox><label>entrer</label><default>true</default><variable>G_X</variable></checkbox></hbox>
        <hbox><text><label>Les autres :</label></text>
          <checkbox><label>lire</label><variable>O_R</variable></checkbox>
          <checkbox><label>écrire</label><variable>O_W</variable></checkbox>
          <checkbox><label>entrer</label><variable>O_X</variable></checkbox></hbox>
        <checkbox><label>Les nouveaux fichiers prennent le groupe du dossier (setgid)</label><default>true</default><variable>SETGID</variable></checkbox>
      </vbox>
      <vbox>
        <text><label>ACL actuelle :</label></text>
        <list scrollable="true" height-request="110">
XML
	for l in "${acl[@]}"; do printf '          <item>%s</item>\n' "$(xml "$l")"; done
	cat <<XML
        </list>
        <checkbox><label>Ajouter une entrée d'ACL (setfacl -m)</label><variable>F_ACL</variable></checkbox>
        <hbox>
          <comboboxtext><variable>ACL_TYPE</variable><default>group</default>
            <item>group</item><item>user</item>
          </comboboxtext>
          <entry space-expand="true" space-fill="true"><variable>ACL_NOM</variable><default>direction</default></entry>
          <comboboxtext><variable>ACL_DROITS</variable><default>r-x</default>
            <item>r-x</item><item>rwx</item><item>r--</item><item>---</item>
          </comboboxtext>
        </hbox>
        <checkbox><label>Aussi par défaut pour ce qui sera créé dedans (-d)</label><default>true</default><variable>ACL_DEFAUT</variable></checkbox>
      </vbox>
    </notebook>
    <hbox>
      <switch><label>Récursif (tout le contenu du dossier)</label><variable>RECURSIF</variable></switch>
    </hbox>
    <hbox>
      <button cancel></button>
      <button><label>Appliquer</label><action>exit:appliquer</action></button>
    </hbox>
  </vbox>
</window>
XML
}

dialogue_confirmation() {
	local dossier=${1:-/srv/partage/compta} nombre=${2:-1 284}
	cat <<XML
<window title="Appliquer à tout le contenu ?">
  <vbox>
    <text><label>Le changement touchera $(xml "$nombre") fichiers et dossiers sous $(xml "$dossier").</label></text>
    <text><label>Il n'y a pas d'annulation : les anciens droits ne sont pas gardés.</label></text>
    <hbox>
      <button cancel></button>
      <button><label>Appliquer à tout</label><action>exit:recursif</action></button>
    </hbox>
  </vbox>
</window>
XML
}

nom_valide() { [[ $1 =~ ^[a-z_][a-z0-9_-]{0,31}$ ]]; }

# Mode octal à partir des cases : setgid, propriétaire, groupe, autres.
mode_octal() {
	local u=0 g=0 o=0 s=0
	[ "$U_R" = true ] && u=$((u + 4)); [ "$U_W" = true ] && u=$((u + 2)); [ "$U_X" = true ] && u=$((u + 1))
	[ "$G_R" = true ] && g=$((g + 4)); [ "$G_W" = true ] && g=$((g + 2)); [ "$G_X" = true ] && g=$((g + 1))
	[ "$O_R" = true ] && o=$((o + 4)); [ "$O_W" = true ] && o=$((o + 2)); [ "$O_X" = true ] && o=$((o + 1))
	[ "$SETGID" = true ] && s=2
	# Cinq chiffres : un 0 de tête explicite retire aussi un setgid non voulu.
	echo "0$s$u$g$o"
}

# Mode symbolique pour le récursif : u=rwX,g=rX,o= (sans setgid, posé à part).
mode_symbolique() {
	local u= g= o=
	[ "$U_R" = true ] && u+=r; [ "$U_W" = true ] && u+=w; [ "$U_X" = true ] && u+=X
	[ "$G_R" = true ] && g+=r; [ "$G_W" = true ] && g+=w; [ "$G_X" = true ] && g+=X
	[ "$O_R" = true ] && o+=r; [ "$O_W" = true ] && o+=w; [ "$O_X" = true ] && o+=X
	echo "u=$u,g=$g,o=$o"
}

appliquer() {
	local r=() ra=()
	[ "$RECURSIF" = true ] && { r=(--recursive); ra=(-R); }
	if [ "$F_CHOWN" = true ]; then
		agir chown "${r[@]}" -- "$PROPRIO:$GROUPE" "$DOSSIER"
	fi
	if [ "$F_CHMOD" = true ]; then
		# Récursif, le setgid ne doit toucher que les dossiers : on le pose à part.
		if [ "$RECURSIF" = true ]; then
			# X (majuscule) : « entrer » ne rend exécutables que les dossiers,
			# et les fichiers qui l'étaient déjà.
			agir chmod --recursive -- "$(mode_symbolique)" "$DOSSIER"
			[ "$SETGID" = true ] && agir find "$DOSSIER" -type d -exec chmod g+s -- {} +
		else
			agir chmod -- "$(mode_octal)" "$DOSSIER"
		fi
	fi
	if [ "$F_ACL" = true ]; then
		agir setfacl "${ra[@]}" -m "$ACL_TYPE:$ACL_NOM:$ACL_DROITS" -- "$DOSSIER"
		[ "$ACL_DEFAUT" = true ] && agir setfacl "${ra[@]}" -d -m "$ACL_TYPE:$ACL_NOM:$ACL_DROITS" -- "$DOSSIER"
	fi
	return 0
}

# Dossiers du système (et leurs sous-dossiers) : jamais touchés par cet outil.
# Le chemin comparé est déjà résolu (realpath) : ni « // », ni « .. », ni lien.
chemin_systeme() {
	case "$1" in
	/var/www|/var/www/*) return 1 ;;
	/|/bin|/bin/*|/boot|/boot/*|/dev|/dev/*|/etc|/etc/*|/lib|/lib/*|/lib32|/lib32/*|/lib64|/lib64/*|/libx32|/libx32/*|\
	/proc|/proc/*|/root|/root/*|/run|/run/*|/sbin|/sbin/*|/sys|/sys/*|/usr|/usr/*|/var|/var/*|\
	/home|/srv|/mnt|/media|/opt|/tmp) return 0 ;;
	esac
	return 1
}

# Résout DOSSIER (realpath -e) puis le refuse s'il est dans le système.
verifier_dossier() {
	[[ $DOSSIER = /* && $DOSSIER != *[[:cntrl:]]* ]] || { echo "Chemin absolu attendu : $DOSSIER" >&2; exit 1; }
	local vrai
	vrai=$(realpath -e -- "$DOSSIER" 2>/dev/null) || { echo "Dossier introuvable : $DOSSIER" >&2; exit 1; }
	[ -d "$vrai" ] || { echo "Ce n'est pas un dossier : $DOSSIER" >&2; exit 1; }
	chemin_systeme "$vrai" && { echo "Dossier système refusé : $DOSSIER (c'est $vrai)" >&2; exit 1; }
	DOSSIER=$vrai
}

suite() {
	# La sortie standard de sermo part vers /dev/null : écrire sur la sortie d'erreur.
	exec >&2
	case "$1" in
	principal)
		[ "$EXIT" = appliquer ] || exit 0
		verifier_dossier
		if [ "$F_CHOWN" = true ]; then
			nom_valide "$PROPRIO" && getent passwd -- "$PROPRIO" >/dev/null || { echo "Propriétaire inconnu : $PROPRIO" >&2; exit 1; }
			nom_valide "$GROUPE" && getent group -- "$GROUPE" >/dev/null || { echo "Groupe inconnu : $GROUPE" >&2; exit 1; }
		fi
		if [ "$F_ACL" = true ]; then
			case "$ACL_TYPE" in user) getent passwd -- "$ACL_NOM" >/dev/null ;; group) getent group -- "$ACL_NOM" >/dev/null ;; *) false ;; esac &&
				nom_valide "$ACL_NOM" || { echo "Entrée d'ACL refusée : $ACL_TYPE:$ACL_NOM" >&2; exit 1; }
			case "$ACL_DROITS" in r-x|rwx|r--|---) ;; *) echo "Droits d'ACL refusés : $ACL_DROITS" >&2; exit 1 ;; esac
		fi
		if [ "$RECURSIF" = true ]; then
			nombre=$(find "$DOSSIER" 2>/dev/null | wc -l)
			export DOSSIER PROPRIO GROUPE F_CHOWN F_CHMOD F_ACL U_R U_W U_X G_R G_W G_X O_R O_W O_X SETGID ACL_TYPE ACL_NOM ACL_DROITS ACL_DEFAUT RECURSIF
			export DIALOGUE; DIALOGUE=$(dialogue_confirmation "$DOSSIER" "$nombre")
			"$SERMO" --program=DIALOGUE --do='"$SERMO_SCRIPT" --suite confirmation' >/dev/null
		else
			appliquer
		fi
		;;
	confirmation)
		[ "$EXIT" = recursif ] || exit 0
		verifier_dossier
		appliquer
		;;
	esac
}

main() {
	if [ "${1:-}" = --suite ]; then suite "$2"; exit; fi
	[ -n "${1:-}" ] && SERMO=$1
	command -v setfacl >/dev/null || { echo "setfacl est absent : installez le paquet acl." >&2; exit 1; }
	local dossier=${2:-$HOME}
	mapfile -t acl < <(getfacl --omit-header --absolute-names -- "$dossier" 2>/dev/null | grep -v '^$')
	export DIALOGUE; DIALOGUE=$(dialogue_principal "$dossier" "$(stat -c %U -- "$dossier")" "$(stat -c %G -- "$dossier")" "${acl[@]}")
	"$SERMO" --program=DIALOGUE --do='"$SERMO_SCRIPT" --suite principal' >/dev/null
}

[ "${BASH_SOURCE[0]}" = "$0" ] && main "$@"
