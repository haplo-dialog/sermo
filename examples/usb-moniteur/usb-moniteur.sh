#!/bin/bash
# SPDX-License-Identifier: CC0-1.0
# Métier   : Support, atelier, électronique, sécurité du poste
# Besoin   : voir les appareils USB branchés et, en direct, ce qui se branche et se
#            débranche — sans droits, sans rien modifier
# Enrobe   : /sys/bus/usb/devices (lecture) et udevadm monitor (événements du noyau)
# Ports    : tout port à fenêtre (tree, button)
# Terminal : ncursessermo par SSH : rien de perdu
# Usage    : ./usb-moniteur.sh [binaire sermo]
#            Sans fenêtre (UM_DIR = dossier de session) :
#              ./usb-moniteur.sh --inventaire   (bus-port|ID|fabricant|produit|classe)
#              ./usb-moniteur.sh --ouvrir | --evenements | --fermer
#            USB_SYSFS : autre racine que /sys/bus/usb/devices (essais).
#
# Codes de sortie : 0 fait · 2 udevadm absent · 64 usage.
# Chaque lecture dans /sys est bornée à 1 s : un appareil coincé ne fige pas la
# fenêtre. Les textes des appareils (fabricant, produit) sont des données :
# nettoyés, plafonnés, jamais insérés dans le XML. Le numéro de série n'est pas lu.

export SERMO="${SERMO:-sermo}"
export SERMO_SCRIPT="$0"
export USB_SYSFS="${USB_SYSFS:-/sys/bus/usb/devices}"
JOURNAL_MAX=2097152

CLASSES="00:par interface|01:audio|02:communication|03:interface humaine|05:physique|06:image|07:imprimante|08:stockage|09:concentrateur|0a:données CDC|0b:carte à puce|0d:sécurité du contenu|0e:vidéo|0f:santé|10:audio/vidéo|dc:diagnostic|e0:sans fil|ef:divers|fe:spécifique application|ff:spécifique fabricant"

# Une valeur d'attribut, lue en 1 s au plus, nettoyée, 80 caractères.
lire_attr() {
	timeout 1 head -c 256 -- "$1" 2>/dev/null | head -n 1 | LC_ALL=C tr -d '\000-\037\177' | sed 's/|/¦/g' | cut -c1-80 |
		iconv -f UTF-8 -t UTF-8 -c
}

inventaire() {
	local d n v p f pr c cl
	for d in "$USB_SYSFS"/*; do
		n=${d##*/}
		[[ "$n" =~ ^[0-9]+-[0-9]+(\.[0-9]+)*$ ]] || continue      # appareils seulement (pas les interfaces ni les racines)
		v=$(lire_attr "$d/idVendor"); p=$(lire_attr "$d/idProduct")
		[[ "$v" =~ ^[0-9a-f]{4}$ && "$p" =~ ^[0-9a-f]{4}$ ]] || continue
		f=$(lire_attr "$d/manufacturer"); pr=$(lire_attr "$d/product")
		c=$(lire_attr "$d/bDeviceClass")
		cl=$(tr '|' '\n' <<<"$CLASSES" | sed -n "s/^$c://p" 2>/dev/null | head -n 1)
		[[ "$c" =~ ^[0-9a-f]{2}$ ]] || { c="??"; cl=""; }
		printf '%s|%s:%s|%s|%s|%s\n' "$n" "$v" "$p" "${f:-—}" "${pr:-—}" "${cl:-classe $c}"
	done | sort -t'|' -k1,1V | head -n 200
}

# ── Événements : udevadm monitor en fond, gardé (PID exacts, jamais un groupe)
fermer() {
	local g
	g=$(cat "$UM_DIR/gardien" 2>/dev/null)
	if [[ "$g" =~ ^[0-9]+$ ]]; then kill -TERM "$g" 2>/dev/null; fi
	rm -f -- "$UM_DIR/gardien"
}
ouvrir() {
	local n
	fermer
	: > "$UM_DIR/journal"; rm -f -- "$UM_DIR/lecteur"
	bash -c '
		maitre=$1; journal=$2; max=$3
		LC_ALL=C udevadm monitor --kernel --subsystem-match=usb 2>"$journal.err" >> "$journal" &
		lecteur=$!
		echo "$lecteur" > "${journal%/*}/lecteur"
		trap '"'"'kill -TERM "$lecteur" 2>/dev/null; exit 0'"'"' TERM INT HUP
		while kill -0 "$maitre" 2>/dev/null && kill -0 "$lecteur" 2>/dev/null &&
		      [ "$(stat -c %s -- "$journal")" -lt "$max" ]; do sleep 1 & wait $!; done
		kill -TERM "$lecteur" 2>/dev/null
	' _ "${UM_MAITRE:-$PPID}" "$UM_DIR/journal" "$JOURNAL_MAX" </dev/null >/dev/null 2>&1 &
	echo $! > "$UM_DIR/gardien"
	for n in 1 2 3 4 5 6 7 8 9 10; do [ -s "$UM_DIR/lecteur" ] && break; sleep 0.1; done
}

# Seules les lignes au format strict « KERNEL[s.us] action /devices/… (usb) »
# d'un APPAREIL (pas d'une interface « :1.0 ») sont gardées ; plus récent en haut.
evenements() {
	LC_ALL=C awk '
		$1 ~ /^KERNEL\[[0-9]+\.[0-9]+\]$/ && $2 ~ /^(add|remove|bind|unbind|change)$/ && $4 == "(usb)" &&
		$3 ~ /^\/devices\/[A-Za-z0-9.:\/_-]+$/ {
			n = split($3, p, "/"); dev = p[n]
			if (dev !~ /^[0-9]+-[0-9]+(\.[0-9]+)*$/) next
			act = ($2 == "add") ? "branché" : ($2 == "remove") ? "débranché" : $2
			t = substr($1, 8, length($1) - 8)
			print t "|" act "|" dev
		}' "$UM_DIR/journal" 2>/dev/null | tail -n 200 | tac
	[ "$(stat -c %s -- "$UM_DIR/journal" 2>/dev/null || echo 0)" -ge "$JOURNAL_MAX" ] && echo "—|journal plein|rouvrez"
	return 0
}

dialogue_principal() {
	cat <<XML
<window title="Appareils USB" default-width="840" default-height="620">
  <vbox spacing="6">
    <frame label="Branchés maintenant (lecture de /sys, sans droits)">
      <vbox>
        <tree>
          <label>Bus-port|ID|Fabricant|Produit|Classe</label>
          <variable>APPAREILS</variable>
          <height>260</height>
          <input>exec "\$SERMO_SCRIPT" --inventaire</input>
        </tree>
        <hbox><button><label>Actualiser</label><action>refresh:APPAREILS</action></button></hbox>
      </vbox>
    </frame>
    <frame label="Événements depuis l'ouverture (temps du noyau, en secondes)">
      <tree>
        <label>Instant|Événement|Bus-port</label>
        <variable>EVENEMENTS</variable>
        <height>180</height>
        <input>exec "\$SERMO_SCRIPT" --evenements</input>
      </tree>
    </frame>
    <timer interval="2" visible="false">
      <variable>HORLOGE</variable>
      <action>refresh:EVENEMENTS</action>
    </timer>
    <!-- Inventaire toutes les 10 s seulement : un appareil coincé coûte jusqu'à 1 s
         par attribut lu, et la fenêtre attend la fin de la lecture. -->
    <timer interval="10" visible="false">
      <variable>HORLOGE_INVENTAIRE</variable>
      <action>refresh:APPAREILS</action>
    </timer>
    <hbox><button><label>Fermer</label><action>exit:fermer</action></button></hbox>
  </vbox>
</window>
XML
}

preparer_dossier() {
	if [ -n "${UM_DIR:-}" ] && [ -d "$UM_DIR" ]; then return 0; fi
	UM_DIR=$(mktemp -d "${XDG_RUNTIME_DIR:-${TMPDIR:-/tmp}}/sermo-usb.XXXXXX") || exit 2
	export UM_DIR; UM_PROPRE=1
	: > "$UM_DIR/journal"
}

usage() { sed -n '/^# Usage/,/^# Codes/p' "$0" | sed '$d;s/^# \{0,1\}//' >&2; exit 64; }

main() {
	umask 077
	case "${1:-}" in
	--inventaire) inventaire; exit 0 ;;
	--ouvrir|--evenements|--fermer)
		command -v udevadm >/dev/null || { echo "udevadm est absent (paquet udev)" >&2; exit 2; }
		preparer_dossier
		[ "${UM_PROPRE:-}" = 1 ] && [ "$1" != --ouvrir ] && trap 'rm -rf -- "$UM_DIR"' EXIT
		case "$1" in --ouvrir) ouvrir ;; --evenements) evenements ;; --fermer) fermer ;; esac
		exit ;;
	--xml) dialogue_principal; exit 0 ;;
	-*)    usage ;;
	esac
	[ -n "${1:-}" ] && SERMO=$1
	command -v "$SERMO" >/dev/null || { echo "sermo introuvable : $SERMO" >&2; exit 2; }
	command -v udevadm >/dev/null || { echo "udevadm est absent (paquet udev)" >&2; exit 2; }
	preparer_dossier
	export UM_MAITRE=$$
	trap 'fermer; rm -rf -- "$UM_DIR"' EXIT
	trap 'exit 129' HUP; trap 'exit 130' INT; trap 'exit 143' TERM
	ouvrir
	export DIALOGUE; DIALOGUE=$(dialogue_principal)
	"$SERMO" --program=DIALOGUE >/dev/null
	exit 0
}

[ "${BASH_SOURCE[0]}" = "$0" ] && main "$@"
