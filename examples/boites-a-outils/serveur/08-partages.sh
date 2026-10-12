#!/bin/bash
# SPDX-License-Identifier: CC0-1.0
# Boîte    : Serveur
# Besoin   : partager un dossier du serveur, en NFS pour des machines Linux ou en Samba pour des postes Windows et Mac
# Enrobe   : tee /etc/exports.d/<nom>.exports + exportfs -ra ; tee -a /etc/samba/smb.conf + testparm -s + systemctl reload smbd
# Ports    : ncursessermo conseillé (par SSH) ; gtk3sermo sur un poste
# Terminal : rien de perdu (les onglets deviennent des pages, l'interrupteur une case)
# Usage    : ./08-partages.sh [binaire sermo]
#            SERMO_A_BLANC=0 pour agir pour de vrai (défaut : la commande est affichée)
#            Écrire sous /etc demande les droits root : sans eux, les commandes sont affichées.

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

# Sans les droits root, on ne peut qu'afficher les commandes.
exiger_root() {
	if [ "$SERMO_A_BLANC" != 1 ] && [ "$EUID" -ne 0 ]; then
		echo "Écrire sous /etc demande les droits root : relancez ce script en root."
		echo "Voici les commandes qu'il lancerait :"
		SERMO_A_BLANC=1
	fi
}

dialogue_principal() {
	local dossier=${1:-/srv/partages/compta}
	cat <<XML
<window title="Partager un dossier" default-width="580">
  <vbox>
    <hbox><text><label>Nom du partage :</label></text>
      <entry space-expand="true" space-fill="true"><variable>NOM</variable><default>compta</default></entry></hbox>
    <hbox><text><label>Dossier partagé :</label></text>
      <entry space-expand="true" space-fill="true"><variable>DOSSIER</variable><default>$(xml "$dossier")</default></entry></hbox>
    <notebook tab-labels="NFS (Linux)|Samba (Windows, Mac)">
      <vbox>
        <hbox><text><label>Machines permises :</label></text>
          <entry space-expand="true" space-fill="true"><variable>CLIENTS</variable><default>192.168.10.0/24</default></entry></hbox>
        <hbox>
          <radiobutton><label>Lecture et écriture</label><default>true</default><variable>NFS_RW</variable></radiobutton>
          <radiobutton><label>Lecture seule</label><variable>NFS_RO</variable></radiobutton>
        </hbox>
        <checkbox><label>Écriture synchrone (sync : plus sûr, plus lent)</label><default>true</default><variable>NFS_SYNC</variable></checkbox>
        <checkbox><label>root d'un client reste anonyme (root_squash)</label><default>true</default><variable>NFS_SQUASH</variable></checkbox>
        <button><label>Exporter en NFS</label><action>exit:nfs</action></button>
      </vbox>
      <vbox>
        <hbox><text><label>Comptes ou @groupe permis :</label></text>
          <entry space-expand="true" space-fill="true"><variable>SMB_COMPTES</variable><default>@compta</default></entry></hbox>
        <hbox><text><label>Description :</label></text>
          <entry space-expand="true" space-fill="true"><variable>SMB_DESCRIPTION</variable><default>Pièces comptables</default></entry></hbox>
        <switch><label>Lecture seule</label><variable>SMB_LECTURE</variable></switch>
        <checkbox><label>Visible dans le voisinage réseau</label><default>true</default><variable>SMB_VISIBLE</variable></checkbox>
        <button><label>Partager en Samba</label><action>exit:samba</action></button>
      </vbox>
    </notebook>
    <hbox>
      <button cancel></button>
    </hbox>
  </vbox>
</window>
XML
}

valider_commun() {
	[[ $NOM =~ ^[a-z0-9][a-z0-9_-]{0,30}$ ]] || { echo "Nom refusé : $NOM (minuscules, chiffres, - et _)"; return 1; }
	[[ $DOSSIER =~ ^/[A-Za-z0-9._/-]+$ && $DOSSIER != *..* ]] || { echo "Dossier refusé : $DOSSIER (chemin absolu, sans espace ni « .. »)"; return 1; }
	# Chemin réel d'un dossier qui existe (« //etc », « /./root », liens suivis), puis
	# comparaison : jamais un dossier système, ni un dossier qui en contient un.
	local reel
	reel=$(realpath -e -- "$DOSSIER" 2>/dev/null) || { echo "Dossier introuvable : $DOSSIER (créez-le d'abord)"; return 1; }
	[ -d "$reel" ] || { echo "Ce n'est pas un dossier : $reel"; return 1; }
	case "$reel" in
	/var/www|/var/www/*) ;;
	/|/bin|/bin/*|/boot|/boot/*|/dev|/dev/*|/etc|/etc/*|/lib|/lib/*|/lib32|/lib32/*|/lib64|/lib64/*|/libx32|/libx32/*|\
	/proc|/proc/*|/root|/root/*|/run|/run/*|/sbin|/sbin/*|/sys|/sys/*|/usr|/usr/*|/var|/var/*|/home|/srv|/opt|/tmp|/tmp/*|/mnt|/media)
		echo "On ne partage pas $DOSSIER (= $reel) : dossier système ou racine d'une arborescence."; return 1 ;;
	esac
	DOSSIER=$reel
	return 0
}

ligne_nfs() {
	local opts=rw
	[ "$NFS_RO" = true ] && opts=ro
	if [ "$NFS_SYNC" = true ]; then opts+=,sync; else opts+=,async; fi
	if [ "$NFS_SQUASH" = true ]; then opts+=,root_squash; else opts+=,no_root_squash; fi
	# Chaque machine porte ses options : « dossier m1(opts) m2(opts) ».
	local c clients ligne=$DOSSIER
	read -r -a clients <<<"$CLIENTS"   # pas de « for c in $CLIENTS » : « *.lan » serait un motif de fichiers
	for c in "${clients[@]}"; do ligne+=" $c($opts,no_subtree_check)"; done
	printf '# Écrit par 08-partages.sh (boîte à outils serveur)\n%s\n' "$ligne"
}

section_samba() {
	printf '\n# Écrit par 08-partages.sh (boîte à outils serveur)\n[%s]\n' "$NOM"
	printf '   path = %s\n' "$DOSSIER"
	[ -n "$SMB_DESCRIPTION" ] && printf '   comment = %s\n' "$SMB_DESCRIPTION"
	if [ "$SMB_LECTURE" = true ]; then printf '   read only = yes\n'; else printf '   read only = no\n'; fi
	if [ "$SMB_VISIBLE" = true ]; then printf '   browseable = yes\n'; else printf '   browseable = no\n'; fi
	printf '   guest ok = no\n   valid users = %s\n' "$SMB_COMPTES"
}

suite() {
	# La sortie standard de sermo part vers /dev/null : écrire sur la sortie d'erreur.
	exec >&2
	case "$1" in
	principal)
		case "$EXIT" in
		nfs)
			valider_commun || exit 1
			# Une adresse, un réseau /NN ou un nom de machine (jokers * permis), séparés par une espace.
			[[ $CLIENTS =~ ^[A-Za-z0-9.*:/-]+(\ [A-Za-z0-9.*:/-]+)*$ ]] || { echo "Machines refusées : $CLIENTS"; exit 1; }
			[ -x /usr/sbin/exportfs ] || { echo "exportfs est absent : installez le paquet nfs-kernel-server."; exit 1; }
			exiger_root
			ligne_nfs | agir tee -- "/etc/exports.d/$NOM.exports" || exit 1
			agir /usr/sbin/exportfs -ra
			;;
		samba)
			valider_commun || exit 1
			[[ $SMB_COMPTES =~ ^@?[a-z_][a-z0-9_.-]*(\ @?[a-z_][a-z0-9_.-]*)*$ ]] ||
				{ echo "Comptes refusés : $SMB_COMPTES (noms ou @groupe séparés par une espace)"; exit 1; }
			case "$SMB_DESCRIPTION" in *[$'\n\r']*|*'['*|*']'*)
				echo "Description refusée : ni saut de ligne ni crochet (smb.conf la lirait comme une section)."; exit 1 ;;
			esac
			[ -r /etc/samba/smb.conf ] || { echo "Samba est absent : installez le paquet samba."; exit 1; }
			grep -qiF "[$NOM]" /etc/samba/smb.conf && { echo "Un partage [$NOM] existe déjà dans smb.conf : rien n'est ajouté."; exit 1; }
			exiger_root
			section_samba | agir tee -a -- /etc/samba/smb.conf || exit 1
			agir testparm -s --suppress-prompt && agir systemctl reload smbd
			;;
		esac
		;;
	esac
}

main() {
	if [ "${1:-}" = --suite ]; then suite "$2"; exit; fi
	[ -n "${1:-}" ] && SERMO=$1
	[ -x /usr/sbin/exportfs ] || [ -r /etc/samba/smb.conf ] ||
		{ echo "Ni NFS ni Samba : installez nfs-kernel-server ou samba." >&2; exit 1; }
	export DIALOGUE; DIALOGUE=$(dialogue_principal)
	"$SERMO" --program=DIALOGUE --do='"$SERMO_SCRIPT" --suite principal' >/dev/null
}

[ "${BASH_SOURCE[0]}" = "$0" ] && main "$@"
