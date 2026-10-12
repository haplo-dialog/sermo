#!/bin/bash
# SPDX-License-Identifier: CC0-1.0
# Boîte    : Administrateur
# Besoin   : charger ou décharger un module du noyau, ou le mettre sur liste noire pour les prochains démarrages
# Enrobe   : /proc/modules et modinfo (lecture) ; modprobe ; modprobe -r ; fichier /etc/modprobe.d/*.conf ; update-initramfs
# Ports    : gtk3sermo conseillé ; ncursessermo par SSH
# Terminal : rien de perdu
# Usage    : ./07-modules-noyau.sh [binaire sermo]
#            SERMO_A_BLANC=0 pour agir pour de vrai (défaut : la commande est affichée)
#            Toutes les actions demandent les droits root : sans eux, le script le dit et
#            affiche la commande. Toute liste noire et tout déchargement se confirment ;
#            les modules du disque racine (pilotes, système de fichiers) sont refusés.

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

# Arguments : une ligne par module chargé, « nom|taille|utilisé par ».
dialogue_principal() {
	local lignes=("$@") l
	[ ${#lignes[@]} -gt 0 ] || lignes=(
		"btusb|77824|0"
		"pcspkr|12288|0"
		"snd_hda_intel|61440|3"
		"uvcvideo|135168|0"
		"usb_storage|86016|1 uas")
	cat <<XML
<window title="Modules du noyau" default-width="600" default-height="540">
  <vbox>
    <frame label="Modules chargés">
      <tree scrollable="true" height-request="150" space-expand="true" space-fill="true">
        <label>Module|Taille (octets)|Utilisé par</label>
        <variable>CHARGE</variable>
XML
	for l in "${lignes[@]}"; do printf '        <item>%s</item>\n' "$(xml "$l")"; done
	cat <<XML
      </tree>
    </frame>
    <hbox><text><label>Module visé (vide = celui choisi dans la liste) :</label></text>
      <entry space-expand="true" space-fill="true"><variable>MODULE</variable><default>pcspkr</default></entry></hbox>
    <frame label="Action">
      <vbox>
        <radiobutton><label>Charger maintenant (modprobe)</label><variable>A_CHARGER</variable></radiobutton>
        <radiobutton><label>Décharger maintenant (modprobe -r)</label><variable>A_DECHARGER</variable></radiobutton>
        <radiobutton><label>Mettre sur liste noire (prochains démarrages)</label><default>true</default><variable>A_LISTE_NOIRE</variable></radiobutton>
      </vbox>
    </frame>
    <expander label="Réglages fins">
      <vbox>
        <hbox><text><label>Paramètres au chargement (ex. index=1) :</label></text>
          <entry space-expand="true" space-fill="true"><variable>PARAMS</variable></entry></hbox>
        <switch><label>Liste noire stricte : refuser aussi un chargement demandé par un autre module (install … /bin/false)</label><variable>STRICTE</variable></switch>
        <togglebutton><label>Reconstruire l'initramfs après la liste noire (la liste noire joue alors dès le démarrage)</label><variable>INITRAMFS</variable></togglebutton>
      </vbox>
      <variable>REGLAGES_OUVERTS</variable>
    </expander>
    <hbox>
      <button cancel></button>
      <button><label>Appliquer</label><action>exit:appliquer</action></button>
    </hbox>
  </vbox>
</window>
XML
}

# Arguments : module, action (décharger, liste noire), ce que l'action fera.
dialogue_confirmation() {
	local module=${1:-pcspkr} action=${2:-mettre sur liste noire} detail=${3:-"Fichier /etc/modprobe.d/blacklist-pcspkr.conf : blacklist pcspkr"}
	cat <<XML
<window title="Confirmer : $(xml "$action") $(xml "$module")" default-width="560">
  <vbox>
    <text><label>Vous allez $(xml "$action") le module $(xml "$module").</label></text>
    <text><label>$(xml "$detail")</label></text>
    <text><label>Un pilote de disque, de clavier ou de réseau mis hors jeu peut couper l'accès à la machine (et SSH avec).</label></text>
    <hbox>
      <button cancel></button>
      <button><label>Confirmer</label><action>exit:confirmer</action></button>
    </hbox>
  </vbox>
</window>
XML
}

# Modules dont dépend le disque racine : pilotes de toute la chaîne (disque,
# contrôleur, bus), système de fichiers, chiffrement et LVM. Un par ligne,
# écrits avec « _ » comme dans /proc/modules.
modules_racine() {
	local src d drv l
	src=$(findmnt -no SOURCE / 2>/dev/null)
	findmnt -no FSTYPE / 2>/dev/null
	[ -b "$src" ] || return 0
	for d in $(lsblk -spnro NAME -- "$src" 2>/dev/null); do
		udevadm info -a -n "$d" 2>/dev/null | sed -n 's/^ *DRIVERS=="\(.\+\)"$/\1/p'
	done | sort -u | while read -r drv; do
		# Nom du pilote → nom du module (sd → sd_mod) ; un pilote intégré n'a pas de module.
		for l in /sys/bus/*/drivers/"$drv"/module; do
			[ -e "$l" ] && basename "$(readlink -f "$l")"
		done
	done
	lsblk -spnro TYPE -- "$src" 2>/dev/null | while read -r t; do
		case "$t" in crypt) echo dm_crypt; echo dm_mod ;; lvm|raid*) echo dm_mod ;; esac
	done
}

liste_noire() {
	local f=/etc/modprobe.d/blacklist-$MODULE.conf contenu
	contenu="# Écrit par 07-modules-noyau.sh
blacklist $MODULE"
	[ "$STRICTE" = true ] && contenu+="
install $MODULE /bin/false"
	echo "Contenu de $f :"; echo "$contenu"
	agir install -m 0644 -T /dev/stdin "$f" <<<"$contenu" || return 1
	[ "$INITRAMFS" = true ] && agir update-initramfs -u
	echo "Effet au prochain démarrage. Pour annuler : supprimer $f."
}

suite() {
	# La sortie standard de sermo part vers /dev/null : écrire sur la sortie d'erreur.
	exec >&2
	PATH=$PATH:/usr/sbin:/sbin   # modinfo et modprobe sont souvent sous /usr/sbin
	case "$1" in
	principal)
		[ "$EXIT" = appliquer ] || exit 0
		[ -n "$MODULE" ] || MODULE=$CHARGE
		[[ $MODULE =~ ^[A-Za-z0-9_-]{1,56}$ ]] || { echo "Nom de module refusé : $MODULE" >&2; exit 1; }
		modinfo -- "$MODULE" >/dev/null 2>&1 || { echo "Module inconnu de ce noyau : $MODULE" >&2; exit 1; }
		if [ "$A_CHARGER" = true ]; then
			read -ra params <<<"$PARAMS"
			for p in "${params[@]}"; do
				[[ $p =~ ^[A-Za-z0-9_]+=[A-Za-z0-9_.,:-]+$ ]] || { echo "Paramètre refusé : $p" >&2; exit 1; }
			done
			agir modprobe -- "$MODULE" "${params[@]}"
		else
			m=${MODULE//-/_}
			if modules_racine | sort -u | grep -qxF -- "$m"; then
				echo "Refusé : $MODULE sert au disque racine ; sans lui la machine ne démarre plus." >&2; exit 1
			fi
			if [ "$A_DECHARGER" = true ]; then
				ligne=$(awk -v m="$m" '$1 == m { print $3 "|" $4 }' /proc/modules)
				[ -n "$ligne" ] || { echo "$MODULE n'est pas chargé." >&2; exit 1; }
				ACTION=decharger
				detail="Utilisé $(cut -d'|' -f1 <<<"$ligne") fois, par : $(tr ',' ' ' <<<"${ligne#*|}"). Le matériel qu'il pilote s'arrête aussitôt."
				libelle=décharger
			else
				ACTION=liste-noire
				detail="Fichier /etc/modprobe.d/blacklist-$MODULE.conf : blacklist $MODULE$([ "$STRICTE" = true ] && echo " ; install $MODULE /bin/false")$([ "$INITRAMFS" = true ] && echo " ; puis update-initramfs -u")."
				libelle="mettre sur liste noire"
			fi
			export MODULE ACTION STRICTE INITRAMFS
			export DIALOGUE; DIALOGUE=$(dialogue_confirmation "$MODULE" "$libelle" "$detail")
			"$SERMO" --program=DIALOGUE --do='"$SERMO_SCRIPT" --suite confirmation' >/dev/null
		fi
		;;
	confirmation)
		[ "$EXIT" = confirmer ] || exit 0
		[[ $MODULE =~ ^[A-Za-z0-9_-]{1,56}$ ]] || exit 1
		case "$ACTION" in
		decharger)   agir modprobe --remove -- "$MODULE" ;;
		liste-noire) liste_noire ;;
		esac
		;;
	esac
}

main() {
	if [ "${1:-}" = --suite ]; then suite "$2"; exit; fi
	[ -n "${1:-}" ] && SERMO=$1
	command -v modprobe >/dev/null || PATH=$PATH:/usr/sbin:/sbin
	command -v modprobe >/dev/null && command -v modinfo >/dev/null || { echo "modprobe ou modinfo est absent : installez le paquet kmod." >&2; exit 1; }
	mapfile -t lignes < <(awk '{ u = $4; gsub(/,$/, "", u); if (u == "-") u = ""; print $1 "|" $2 "|" $3 (u == "" ? "" : " " u) }' /proc/modules | sort)
	export DIALOGUE; DIALOGUE=$(dialogue_principal "${lignes[@]}")
	"$SERMO" --program=DIALOGUE --do='"$SERMO_SCRIPT" --suite principal' >/dev/null
}

[ "${BASH_SOURCE[0]}" = "$0" ] && main "$@"
