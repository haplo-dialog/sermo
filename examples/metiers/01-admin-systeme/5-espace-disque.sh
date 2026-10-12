#!/bin/bash
# SPDX-License-Identifier: CC0-1.0
# Métier   : Administrateur système
# Besoin   : voir où part l'espace disque et nettoyer les caches, avec confirmation
# Enrobe   : df, du, apt-get clean, journalctl --vacuum-time
# Ports    : gtk3sermo conseillé ; ncursessermo par SSH
# Terminal : rien de perdu
# Usage    : ./5-espace-disque.sh [binaire sermo]
#            SERMO_A_BLANC=0 pour agir pour de vrai (défaut : la commande est affichée)
#            En vrai, apt-get clean et le journal demandent les droits root : le
#            script affiche alors la commande à lancer en root.

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

# Comme agir, mais pour une commande qui exige root.
agir_root() {
	if [ "$SERMO_A_BLANC" != 1 ] && [ "$EUID" != 0 ]; then
		printf 'Droits root nécessaires. À lancer en root :'; printf ' %q' "$@"; printf '\n'
		return 1
	fi
	agir "$@"
}

# Arguments : taille du cache apt, du journal, des vignettes, dossier personnel,
# puis une ligne par système de fichiers « source|taille|utilisé|libre|occupé|point ».
dialogue_principal() {
	# Sans argument : valeurs de démonstration.
	[ $# -gt 0 ] || set -- 1,2G 640M 150M /home/utilisateur \
		"/dev/nvme0n1p2|468G|301G|144G|68%|/" \
		"/dev/nvme0n1p1|511M|6,1M|505M|2%|/boot/efi" \
		"/dev/sda1|916G|802G|68G|93%|/media/donnees"
	local apt=$1 journal=$2 vignettes=$3 maison=$4 l
	shift 4
	cat <<XML
<window title="Espace disque" default-width="660" default-height="600">
  <vbox>
    <frame label="Systèmes de fichiers">
      <vbox>
        <table scrollable="true" space-expand="true" space-fill="true">
          <label>Périphérique|Taille|Utilisé|Libre|Occupé|Monté sur</label>
XML
	for l in "$@"; do printf '          <item>%s</item>\n' "$(xml "$l")"; done
	cat <<XML
        </table>
      </vbox>
    </frame>
    <frame label="Chercher ce qui prend de la place" space-expand="false" space-fill="false">
      <hbox>
        <entry space-expand="true" space-fill="true"><variable>DOSSIER</variable><default>$(xml "$maison")</default></entry>
        <text><label>Profondeur :</label></text>
        <spinbutton range-min="1" range-max="3" range-step="1"><variable>PROFONDEUR</variable><default>1</default></spinbutton>
        <button><label>Analyser</label><action>exit:analyser</action></button>
      </hbox>
    </frame>
    <frame label="Nettoyage" space-expand="false" space-fill="false">
      <vbox>
        <checkbox><label>Vider le cache des paquets téléchargés (apt-get clean) : $(xml "$apt")</label><variable>N_APT</variable><default>true</default></checkbox>
        <checkbox><label>Réduire le journal système ($(xml "$journal") aujourd'hui)</label><variable>N_JOURNAL</variable><default>true</default></checkbox>
        <hbox><text><label>Garder les journaux des derniers jours :</label></text>
          <hscale range-min="1" range-max="90" range-step="1" draw-value="true" space-expand="true" space-fill="true"><variable>JOURS</variable><default>30</default></hscale></hbox>
        <checkbox><label>Effacer les vignettes d'images (~/.cache/thumbnails) : $(xml "$vignettes")</label><variable>N_VIGNETTES</variable></checkbox>
      </vbox>
    </frame>
    <hbox>
      <button cancel></button>
      <button><label>Nettoyer…</label><action>exit:nettoyer</action></button>
    </hbox>
  </vbox>
</window>
XML
}

# Arguments : dossier analysé, puis une ligne par sous-dossier « chemin|taille ».
dialogue_analyse() {
	[ $# -gt 0 ] || set -- /home/utilisateur \
		"/home/utilisateur|58G" "/home/utilisateur/Vidéos|31G" "/home/utilisateur/.cache|9,4G" \
		"/home/utilisateur/Documents|6,2G" "/home/utilisateur/Téléchargements|4,8G"
	local dossier=$1 l
	shift
	cat <<XML
<window title="Analyse de $(xml "$dossier")" default-width="560" default-height="440">
  <vbox>
    <text><label>Les plus gros dossiers (sans franchir les autres disques) :</label></text>
    <tree scrollable="true" space-expand="true" space-fill="true">
      <label>Dossier|Taille</label>
      <variable>LOURD</variable>
XML
	for l in "$@"; do printf '      <item>%s</item>\n' "$(xml "$l")"; done
	cat <<XML
    </tree>
    <hbox>
      <button><label>Fermer</label><action>exit:fermer</action></button>
      <button><label>Analyser le dossier choisi</label><action>exit:descendre</action></button>
    </hbox>
  </vbox>
</window>
XML
}

dialogue_confirmation() {
	local resume=${1:-apt-get clean, journal ramené à 30 jours}
	cat <<XML
<window title="Confirmer le nettoyage">
  <vbox>
    <text><label>Ces fichiers seront supprimés définitivement :</label></text>
    <text><label>$(xml "$resume")</label></text>
    <hbox>
      <button cancel></button>
      <button><label>Supprimer</label><action>exit:supprimer</action></button>
    </hbox>
  </vbox>
</window>
XML
}

# Taille lisible d'un dossier, ou « ? » s'il est illisible ou absent.
taille() { local t; t=$(du -sh -- "$1" 2>/dev/null | cut -f1); printf '%s' "${t:-?}"; }

analyser() {
	local dossier=$1 profondeur=$2 lignes=() t c
	[ -d "$dossier" ] || { echo "Dossier introuvable : $dossier" >&2; exit 1; }
	[[ "$profondeur" =~ ^[1-3]$ ]] || profondeur=1
	echo "du -xh --max-depth=$profondeur -- $dossier" >&2
	while IFS=$'\t' read -r t c; do
		lignes+=("${c//|//}|$t")
	done < <(du -xh --max-depth="$profondeur" -- "$dossier" 2>/dev/null | sort -rh | head -n 40)
	export PROFONDEUR
	export DIALOGUE; DIALOGUE=$(dialogue_analyse "$dossier" "${lignes[@]}")
	"$SERMO" --program=DIALOGUE --do='"$SERMO_SCRIPT" --suite analyse' >/dev/null
}

suite() {
	exec >&2
	case "$1" in
	principal)
		case "$EXIT" in
		analyser) analyser "$DOSSIER" "$PROFONDEUR" ;;
		nettoyer)
			local quoi=()
			[ "$N_APT" = true ] && quoi+=("cache apt")
			[ "$N_JOURNAL" = true ] && quoi+=("journaux de plus de $JOURS jours")
			[ "$N_VIGNETTES" = true ] && quoi+=("vignettes d'images")
			[ ${#quoi[@]} -gt 0 ] || { echo "Rien de coché." >&2; exit 0; }
			[[ "$JOURS" =~ ^[0-9]+$ ]] || { echo "Nombre de jours illisible : $JOURS" >&2; exit 1; }
			export N_APT N_JOURNAL N_VIGNETTES JOURS
			local IFS=,
			export DIALOGUE; DIALOGUE=$(dialogue_confirmation "${quoi[*]}")
			"$SERMO" --program=DIALOGUE --do='"$SERMO_SCRIPT" --suite confirmation' >/dev/null
			;;
		esac
		;;
	analyse)
		[ "$EXIT" = descendre ] || exit 0
		[ -n "$LOURD" ] || { echo "Aucun dossier choisi." >&2; exit 1; }
		analyser "$LOURD" "$PROFONDEUR"
		;;
	confirmation)
		[ "$EXIT" = supprimer ] || exit 0
		if [ "$N_APT" = true ]; then
			if command -v apt-get >/dev/null; then agir_root apt-get clean
			else echo "apt-get absent : cache apt ignoré." >&2; fi
		fi
		[ "$N_JOURNAL" = true ] && agir_root journalctl --vacuum-time="${JOURS}d"
		[ "$N_VIGNETTES" = true ] && agir rm -rf -- "$HOME/.cache/thumbnails"
		;;
	esac
}

main() {
	if [ "${1:-}" = --suite ]; then suite "$2"; exit; fi
	[ -n "${1:-}" ] && SERMO=$1
	local c
	for c in df du sort journalctl; do
		command -v "$c" >/dev/null || { echo "$c est absent : impossible de mesurer l'espace." >&2; exit 1; }
	done
	local journal lignes=() src tai uti dis pc pt
	journal=$(journalctl --disk-usage 2>/dev/null | grep -oE '[0-9.,]+[KMGT]' | head -n1)
	while read -r src tai uti dis pc pt; do
		lignes+=("$src|$tai|$uti|$dis|$pc|$pt")
	done < <(df -h --output=source,size,used,avail,pcent,target -x tmpfs -x devtmpfs -x squashfs -x overlay -x efivarfs 2>/dev/null | tail -n +2)
	export DIALOGUE; DIALOGUE=$(dialogue_principal "$(taille /var/cache/apt/archives)" "${journal:-?}" \
		"$(taille "$HOME/.cache/thumbnails")" "$HOME" "${lignes[@]}")
	"$SERMO" --program=DIALOGUE --do='"$SERMO_SCRIPT" --suite principal' >/dev/null
}

[ "${BASH_SOURCE[0]}" = "$0" ] && main "$@"
