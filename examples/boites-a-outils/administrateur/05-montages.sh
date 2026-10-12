#!/bin/bash
# SPDX-License-Identifier: CC0-1.0
# Boîte    : Administrateur
# Besoin   : monter un disque à demeure : ligne fstab par UUID, vérifiée à blanc avant de monter
# Enrobe   : lsblk (lecture) ; /etc/fstab (ajout) ; findmnt --verify ; mount --fake -a ; mount
# Ports    : gtk3sermo conseillé ; ncursessermo par SSH
# Terminal : rien de perdu
# Usage    : ./05-montages.sh [binaire sermo]
#            SERMO_A_BLANC=0 pour agir pour de vrai (défaut : la commande est affichée)
#            Écrire /etc/fstab et monter demandent les droits root : sans eux, le script le dit
#            et affiche la commande. Une ligne fausse dans fstab peut bloquer le démarrage :
#            l'option nofail est cochée par défaut.

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

# Arguments : une ligne par partition, « périphérique|UUID|type|taille|étiquette|monté sur ».
dialogue_principal() {
	local lignes=("$@") l
	[ ${#lignes[@]} -gt 0 ] || lignes=(
		"/dev/sda1|3f2c9a1e-7b4d-4e8a-9c21-5d6e7f8a9b0c|ext4|931,5G|archives|"
		"/dev/sdb1|6A1F-2C3D|exfat|58,6G|CLE-USB|"
		"/dev/nvme0n1p2|0b1d2e3f-4a5b-6c7d-8e9f-a0b1c2d3e4f5|ext4|475,9G||/")
	cat <<XML
<window title="Monter un disque" default-width="700" default-height="540">
  <vbox>
    <text><label>Partitions vues par lsblk. Choisissez celle à monter.</label></text>
    <table scrollable="true" height-request="140" space-expand="true" space-fill="true">
      <label>Périphérique|UUID|Type|Taille|Étiquette|Monté sur</label>
      <variable>PART</variable>
XML
	for l in "${lignes[@]}"; do printf '      <item>%s</item>\n' "$(xml "$l")"; done
	cat <<XML
    </table>
    <frame label="Ligne fstab">
      <vbox>
        <hbox><text><label>Point de montage :</label></text>
          <entry space-expand="true" space-fill="true"><variable>POINT</variable><default>/srv/archives</default></entry></hbox>
        <hbox><text><label>Type :</label></text>
          <comboboxtext><variable>TYPE</variable><default>auto</default>
            <item>auto</item><item>ext4</item><item>xfs</item><item>btrfs</item><item>vfat</item><item>exfat</item><item>ntfs3</item>
          </comboboxtext></hbox>
        <hbox>
          <checkbox><label>nofail (démarrer même si le disque manque)</label><default>true</default><variable>NOFAIL</variable></checkbox>
          <checkbox><label>noatime</label><default>true</default><variable>NOATIME</variable></checkbox>
          <checkbox><label>lecture seule (ro)</label><variable>LECTURE_SEULE</variable></checkbox>
        </hbox>
        <hbox><text><label>Vérification au démarrage (fsck, 0 = jamais, 2 = après la racine) :</label></text>
          <spinbutton range-min="0" range-max="2" range-step="1"><variable>PASSE</variable><default>2</default></spinbutton></hbox>
      </vbox>
    </frame>
    <hbox>
      <button cancel></button>
      <button><label>Préparer la ligne</label><action>exit:preparer</action></button>
    </hbox>
  </vbox>
</window>
XML
}

# Arguments : la ligne fstab, le compte rendu du test à blanc.
dialogue_confirmation() {
	local ligne=${1:-UUID=3f2c9a1e-7b4d-4e8a-9c21-5d6e7f8a9b0c /srv/archives auto defaults,nofail,noatime 0 2}
	local test=${2:-"Success, no errors or warnings detected"}
	cat <<XML
<window title="Ajouter à /etc/fstab ?" default-width="640">
  <vbox>
    <text><label>Ligne ajoutée à la fin de /etc/fstab :</label></text>
    <entry editable="false"><variable>LIGNE_VUE</variable><default>$(xml "$ligne")</default></entry>
    <frame label="Test de la table (findmnt --verify, sans rien monter)">
      <text><label>$(xml "$test")</label></text>
    </frame>
    <checkbox><label>Monter tout de suite après l'ajout</label><default>true</default><variable>MONTER</variable></checkbox>
    <hbox>
      <button cancel></button>
      <button><label>Ajouter la ligne</label><action>exit:ajouter</action></button>
    </hbox>
  </vbox>
</window>
XML
}

# Points de montage du système (et leurs sous-dossiers) : refusés.
# Le chemin comparé est déjà résolu (realpath -m) : ni « // », ni « .. », ni lien.
point_systeme() {
	case "$1" in
	/|/bin|/bin/*|/boot|/boot/*|/dev|/dev/*|/etc|/etc/*|/lib|/lib/*|/lib32|/lib32/*|/lib64|/lib64/*|/libx32|/libx32/*|\
	/proc|/proc/*|/root|/root/*|/run|/run/*|/sbin|/sbin/*|/sys|/sys/*|/usr|/usr/*|/var|/var/*|\
	/home|/media|/opt|/tmp|/srv|/mnt) return 0 ;;
	esac
	return 1
}

# Résout POINT (realpath -m : il peut ne pas exister encore) puis le contrôle.
verifier_point() {
	[[ $POINT =~ ^/[A-Za-z0-9._/-]+$ ]] || { echo "Point de montage refusé : $POINT" >&2; exit 1; }
	local vrai
	vrai=$(realpath -m -- "$POINT") && [[ $vrai =~ ^/[A-Za-z0-9._/-]+$ ]] || { echo "Point de montage refusé : $POINT" >&2; exit 1; }
	point_systeme "$vrai" && { echo "Point de montage système refusé : $POINT (c'est $vrai). Prenez un dossier sous /srv ou /mnt." >&2; exit 1; }
	[ -e "$vrai" ] && [ ! -d "$vrai" ] && { echo "Ce n'est pas un dossier : $vrai" >&2; exit 1; }
	POINT=$vrai
}

suite() {
	# La sortie standard de sermo part vers /dev/null : écrire sur la sortie d'erreur.
	exec >&2
	case "$1" in
	principal)
		[ "$EXIT" = preparer ] || exit 0
		[[ $PART =~ ^/dev/[A-Za-z0-9/_-]+$ && $PART != *..* ]] || { echo "Choisissez d'abord une partition dans la liste." >&2; exit 1; }
		uuid=$(lsblk -dno UUID -- "$PART" 2>/dev/null)
		[[ $uuid =~ ^[A-Fa-f0-9-]{8,36}$ ]] || { echo "Pas d'UUID lisible pour $PART (non formatée ?)." >&2; exit 1; }
		verifier_point
		case "$TYPE" in auto|ext4|xfs|btrfs|vfat|exfat|ntfs3) ;; *) echo "Type refusé : $TYPE" >&2; exit 1 ;; esac
		[[ $PASSE =~ ^[0-2]$ ]] || PASSE=2
		case "$TYPE" in vfat|exfat|ntfs3) PASSE=0 ;; esac
		grep -qE "^[[:space:]]*UUID=$uuid[[:space:]]" /etc/fstab && { echo "Ce disque est déjà dans /etc/fstab." >&2; exit 1; }
		opts=defaults
		[ "$NOFAIL" = true ] && opts+=,nofail
		[ "$NOATIME" = true ] && opts+=,noatime
		[ "$LECTURE_SEULE" = true ] && opts+=,ro
		LIGNE="UUID=$uuid $POINT $TYPE $opts 0 $PASSE"
		# Test à blanc : la table actuelle plus la nouvelle ligne, vérifiée sans rien monter.
		# Le fichier d'essai va dans le cache de la personne, pas dans /tmp.
		essai_dir=${XDG_CACHE_HOME:-$HOME/.cache}; mkdir -p -- "$essai_dir"
		essai=$(mktemp -p "$essai_dir" fstab-essai.XXXXXX) || exit 1
		{ cat /etc/fstab; echo "$LIGNE"; } > "$essai"
		test=$(LC_ALL=C findmnt --verify --tab-file "$essai" 2>&1); rc=$?
		rm -f -- "$essai"
		# Bloquant : une table que findmnt refuse peut empêcher le démarrage.
		# Seule exception : le point de montage pas encore créé (mkdir le crée
		# juste avant le montage d'essai). Un type qui ne correspond pas au
		# disque bloque aussi.
		[ -e "$POINT" ] && absent=0 || absent=1
		bloquants=$(awk -v p="$POINT" -v absent="$absent" '
			/^[0-9]+ parse error/ { if ($1 + 0 > 0) { n++; print } next }
			/^[^ ]/ { cible = $0; next }
			/\[E\]/ { if (absent && cible == p && /unreachable/) next; n++; print cible ": " $0 }
			/\[W\]/ && cible == p && /does not match/ { n++; print cible ": " $0 }
			END { exit n > 0 }' <<<"$test")
		if [ $? -ne 0 ] || { [ "$rc" -ne 0 ] && [ -z "$bloquants" ] && [ "$absent" = 0 ]; }; then
			echo "findmnt --verify refuse la table avec la nouvelle ligne ; rien n'est écrit :" >&2
			echo "${bloquants:-$test}" >&2; exit 1
		fi
		test=$(head -n 1 <<<"$test")
		[ "$absent" = 1 ] && test+=" ; $POINT sera créé avant le montage d'essai"
		export PART POINT LIGNE
		export DIALOGUE; DIALOGUE=$(dialogue_confirmation "$LIGNE" "${test:-aucun compte rendu}")
		"$SERMO" --program=DIALOGUE --do='"$SERMO_SCRIPT" --suite confirmation' >/dev/null
		;;
	confirmation)
		[ "$EXIT" = ajouter ] || exit 0
		[[ $LIGNE =~ ^UUID=[A-Fa-f0-9-]+\ /[A-Za-z0-9._/-]+\ [a-z0-9]+\ [a-z,]+\ 0\ [0-2]$ ]] || exit 1
		verifier_point
		[ "$(cut -d' ' -f2 <<<"$LIGNE")" = "$POINT" ] || exit 1
		agir cp --preserve=all -- /etc/fstab /etc/fstab.avant-montage || exit 1
		agir tee --append -- /etc/fstab <<<"$LIGNE" || exit 1
		agir mkdir -p -- "$POINT"
		agir systemctl daemon-reload
		# Rejoue toute la table sans rien monter (--fake). En cas d'échec, la
		# copie d'avant est remise : la ligne fautive ne reste pas dans fstab.
		if ! agir mount --fake --all --verbose; then
			echo "Le montage d'essai échoue : /etc/fstab est remis comme avant."
			agir cp --preserve=all -- /etc/fstab.avant-montage /etc/fstab
			agir systemctl daemon-reload
			exit 1
		fi
		[ "$MONTER" = true ] && agir mount -- "$POINT"
		;;
	esac
}

main() {
	if [ "${1:-}" = --suite ]; then suite "$2"; exit; fi
	[ -n "${1:-}" ] && SERMO=$1
	command -v lsblk >/dev/null && command -v findmnt >/dev/null || { echo "lsblk ou findmnt est absent : installez le paquet util-linux." >&2; exit 1; }
	mapfile -t lignes < <(lsblk -rpno NAME,UUID,FSTYPE,SIZE,LABEL,MOUNTPOINT,TYPE | awk -F'[ ]' '$7 == "part" || $7 == "lvm" || $7 == "crypt" { printf "%s|%s|%s|%s|%s|%s\n", $1, $2, $3, $4, $5, $6 }')
	export DIALOGUE; DIALOGUE=$(dialogue_principal "${lignes[@]}")
	"$SERMO" --program=DIALOGUE --do='"$SERMO_SCRIPT" --suite principal' >/dev/null
}

[ "${BASH_SOURCE[0]}" = "$0" ] && main "$@"
