#!/bin/bash
# SPDX-License-Identifier: CC0-1.0
# Boîte    : Administrateur
# Besoin   : formater un disque ou une partition avec un système de fichiers et une étiquette, sans se tromper de périphérique
# Enrobe   : lsblk et findmnt (lecture) ; mkfs.ext4, mkfs.xfs, mkfs.btrfs, mkfs.vfat, mkfs.exfat
# Ports    : gtk3sermo conseillé ; ncursessermo par SSH
# Terminal : rien de perdu
# Usage    : ./06-formatage.sh [binaire sermo]
#            SERMO_A_BLANC=0 pour agir pour de vrai (défaut : la commande est affichée)
#            DESTRUCTIF : tout le contenu du périphérique est perdu. Il faut retaper son nom
#            pour confirmer. Formater demande les droits root : sans eux, le script le dit
#            et affiche la commande. Un périphérique monté est refusé.

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

# Arguments : une ligne par périphérique NON monté, « périphérique taille type actuel modèle ».
dialogue_principal() {
	local lignes=("$@") l
	[ ${#lignes[@]} -gt 0 ] || lignes=(
		"/dev/sdb 58,6G disque (Clé USB 64 Go)"
		"/dev/sdb1 58,6G exfat CLE-USB"
		"/dev/sdc1 1,8T ext4 ancien-nas")
	cat <<XML
<window title="Formater un périphérique" default-width="560" default-height="520">
  <vbox>
    <frame label="Périphérique (seuls les non montés sont proposés)">
      <list scrollable="true" height-request="120" space-expand="true" space-fill="true"><variable>CHOIX</variable>
XML
	for l in "${lignes[@]}"; do printf '        <item>%s</item>\n' "$(xml "$l")"; done
	cat <<XML
      </list>
    </frame>
    <frame label="Système de fichiers">
      <vbox>
        <hbox>
          <radiobutton><label>ext4</label><default>true</default><variable>FS_EXT4</variable></radiobutton>
          <radiobutton><label>xfs</label><variable>FS_XFS</variable></radiobutton>
          <radiobutton><label>btrfs</label><variable>FS_BTRFS</variable></radiobutton>
        </hbox>
        <hbox>
          <radiobutton><label>vfat (FAT32, tout appareil)</label><variable>FS_VFAT</variable></radiobutton>
          <radiobutton><label>exfat (gros fichiers, Windows et macOS)</label><variable>FS_EXFAT</variable></radiobutton>
        </hbox>
      </vbox>
    </frame>
    <hbox><text><label>Étiquette (lettres, chiffres, - et _) :</label></text>
      <entry space-expand="true" space-fill="true"><variable>ETIQUETTE</variable><default>DONNEES</default></entry></hbox>
    <checkbox><label>Écraser une signature déjà présente (-f pour xfs et btrfs)</label><variable>FORCER</variable></checkbox>
    <hbox>
      <button cancel></button>
      <button><label>Formater…</label><action>exit:formater</action></button>
    </hbox>
    <statusbar><variable>AVERTISSEMENT</variable><default>Formater efface tout le contenu du périphérique choisi.</default></statusbar>
  </vbox>
</window>
XML
}

# Arguments : périphérique, système de fichiers, ce que lsblk en dit.
dialogue_confirmation() {
	local dev=${1:-/dev/sdb1} fs=${2:-ext4} detail=${3:-58,6G exfat CLE-USB}
	cat <<XML
<window title="Effacer $(xml "$dev") ?" default-width="520">
  <vbox>
    <text><label>Vous allez formater $(xml "$dev") en $(xml "$fs").</label></text>
    <text><label>Aujourd'hui : $(xml "$detail"). TOUT son contenu sera perdu.</label></text>
    <text><label>Pour confirmer, retapez le nom exact du périphérique :</label></text>
    <entry><variable>RETAPE</variable></entry>
    <hbox>
      <button cancel></button>
      <button><label>Effacer et formater</label><action>exit:effacer</action></button>
    </hbox>
  </vbox>
</window>
XML
}

fs_choisi() {
	if [ "$FS_XFS" = true ]; then echo xfs
	elif [ "$FS_BTRFS" = true ]; then echo btrfs
	elif [ "$FS_VFAT" = true ]; then echo vfat
	elif [ "$FS_EXFAT" = true ]; then echo exfat
	else echo ext4
	fi
}

# Commande mkfs, un argument par ligne.
commande_mkfs() {
	case "$FS" in
	ext4)  echo mkfs.ext4; [ -n "$ETIQUETTE" ] && { echo -L; echo "$ETIQUETTE"; } ;;
	xfs)   echo mkfs.xfs; [ "$FORCER" = true ] && echo -f; [ -n "$ETIQUETTE" ] && { echo -L; echo "$ETIQUETTE"; } ;;
	btrfs) echo mkfs.btrfs; [ "$FORCER" = true ] && echo -f; [ -n "$ETIQUETTE" ] && { echo -L; echo "$ETIQUETTE"; } ;;
	vfat)  echo mkfs.vfat; echo -F; echo 32; [ -n "$ETIQUETTE" ] && printf '%s\n' -n "${ETIQUETTE^^}" ;;
	exfat) echo mkfs.exfat; [ -n "$ETIQUETTE" ] && { echo -L; echo "$ETIQUETTE"; } ;;
	esac
	echo --; echo "$DEV"
}

suite() {
	# La sortie standard de sermo part vers /dev/null : écrire sur la sortie d'erreur.
	exec >&2
	case "$1" in
	principal)
		[ "$EXIT" = formater ] || exit 0
		DEV=${CHOIX%% *}
		[[ $DEV =~ ^/dev/[A-Za-z0-9/_-]+$ && $DEV != *..* ]] || { echo "Choisissez d'abord un périphérique dans la liste." >&2; exit 1; }
		[ -b "$DEV" ] || { echo "Ce n'est pas un périphérique bloc : $DEV" >&2; exit 1; }
		# Refus si lui-même ou l'une de ses partitions est monté (ou sert de swap).
		if lsblk -nro MOUNTPOINTS -- "$DEV" 2>/dev/null | grep -q .; then
			echo "$DEV (ou une de ses partitions) est monté : démontez-le d'abord." >&2; exit 1
		fi
		FS=$(fs_choisi)
		max=16; case "$FS" in xfs) max=12 ;; vfat) max=11 ;; exfat) max=15 ;; esac
		[[ $ETIQUETTE =~ ^[A-Za-z0-9_-]*$ ]] && [ ${#ETIQUETTE} -le $max ] || { echo "Étiquette refusée pour $FS (au plus $max caractères : lettres, chiffres, - et _)." >&2; exit 1; }
		command -v "mkfs.$FS" >/dev/null || PATH=$PATH:/usr/sbin:/sbin
		command -v "mkfs.$FS" >/dev/null || { echo "mkfs.$FS est absent (paquets e2fsprogs, xfsprogs, btrfs-progs, dosfstools, exfatprogs)." >&2; exit 1; }
		detail=$(lsblk -dno SIZE,FSTYPE,LABEL,MODEL -- "$DEV" 2>/dev/null | tr -s ' ')
		export DEV FS ETIQUETTE FORCER
		export DIALOGUE; DIALOGUE=$(dialogue_confirmation "$DEV" "$FS" "${detail:-inconnu}")
		"$SERMO" --program=DIALOGUE --do='"$SERMO_SCRIPT" --suite confirmation' >/dev/null
		;;
	confirmation)
		[ "$EXIT" = effacer ] || exit 0
		[ "$RETAPE" = "$DEV" ] || { echo "Nom retapé différent ($RETAPE ≠ $DEV) : rien n'est fait." >&2; exit 1; }
		lsblk -nro MOUNTPOINTS -- "$DEV" 2>/dev/null | grep -q . && { echo "$DEV est monté entre-temps : rien n'est fait." >&2; exit 1; }
		mapfile -t cmd < <(commande_mkfs)
		agir "${cmd[@]}"
		;;
	esac
}

main() {
	if [ "${1:-}" = --suite ]; then suite "$2"; exit; fi
	[ -n "${1:-}" ] && SERMO=$1
	command -v lsblk >/dev/null || { echo "lsblk est absent : installez le paquet util-linux." >&2; exit 1; }
	# Disques et partitions sans point de montage (ni pour eux ni pour leurs enfants).
	mapfile -t lignes < <(lsblk -rpno NAME,TYPE,SIZE,FSTYPE,LABEL | awk -F'[ ]' '$2 == "disk" || $2 == "part" { print $1 " " $3 " " ($4 == "" ? "vide" : $4) " " $5 }' |
		while read -r dev reste; do lsblk -nro MOUNTPOINTS -- "$dev" 2>/dev/null | grep -q . || echo "$dev $reste"; done)
	[ ${#lignes[@]} -gt 0 ] || { echo "Aucun périphérique non monté : rien à formater."; exit 0; }
	export DIALOGUE; DIALOGUE=$(dialogue_principal "${lignes[@]}")
	"$SERMO" --program=DIALOGUE --do='"$SERMO_SCRIPT" --suite principal' >/dev/null
}

[ "${BASH_SOURCE[0]}" = "$0" ] && main "$@"
