#!/bin/bash
# SPDX-License-Identifier: CC0-1.0
# Métier   : Automobile, robotique, machines, électronique embarquée
# Besoin   : écouter un bus CAN (trames, identifiants vus) et, sur confirmation
#            explicite, y envoyer UNE trame
# Enrobe   : candump (lecture en fond) et cansend (paquet can-utils) ; la
#            confirmation passe par examples/approbation/approbation.sh
# Ports    : tout port à fenêtre (la confirmation demande un affichage)
# Terminal : lecture seule par SSH (sans affichage, l'envoi est refusé)
# Usage    : ./bus-can.sh [binaire sermo]
#            Sans fenêtre (mêmes contrôles ; BC_DIR = dossier de session) :
#              ./bus-can.sh --ouvrir INTERFACE
#              ./bus-can.sh --inventaire          (ID|données|trames|âge)
#              ./bus-can.sh --envoyer INTERFACE ID DONNÉES
#              ./bus-can.sh --fermer
#            Essai sans matériel : interface virtuelle vcan (voir README).
#
# Codes de sortie : 0 fait · 1 envoi refusé par l'humain · 2 outil absent ·
# 64 entrée invalide. Lecture seule par défaut : RIEN n'est écrit sur le bus sans
# une fenêtre d'approbation qui montre l'identifiant et les octets exacts.
# CAN classique seulement (ID de 11 ou 29 bits, 0 à 8 octets) ; CAN FD : non.

export SERMO="${SERMO:-sermo}"
export SERMO_SCRIPT="$0"
ICI=$(CDPATH= cd -- "$(dirname -- "$(readlink -f -- "$0")")" && pwd)
APPROBATION="${BC_APPROBATION:-$ICI/../approbation/approbation.sh}"
JOURNAL_MAX=4194304     # au-delà, la lecture s'arrête (journal plein)
LIGNES_MAX=500          # identifiants affichés au plus

xml() { local s=${1//&/"&amp;"}; s=${s//</"&lt;"}; s=${s//>/"&gt;"}; printf '%s' "${s//\"/"&quot;"}"; }

# ── Contrôles d'entrée ──────────────────────────────────────────────────────
iface_valide() {
	[[ "$1" =~ ^v?can[0-9]{1,2}$ ]] || return 1
	LC_ALL=C ip -o link show dev "$1" 2>/dev/null | grep -q 'link/can'
}
id_valide() {   # 3 chiffres hex ≤ 7FF (11 bits) ou 8 chiffres hex ≤ 1FFFFFFF (29 bits)
	if [[ "$1" =~ ^[0-9A-Fa-f]{3}$ ]]; then [ "$((16#$1))" -le $((16#7FF)) ]
	elif [[ "$1" =~ ^[0-9A-Fa-f]{8}$ ]]; then [ "$((16#$1))" -le $((16#1FFFFFFF)) ]
	else return 1; fi
}
donnees_valides() { [[ "$1" =~ ^([0-9A-Fa-f]{2}){0,8}$ ]]; }
refus() { printf 'entrée invalide : %s\n' "$1" | tee "$BC_DIR/etat" >&2; exit 64; }

# ── Lecture en fond : candump écrit le journal ; un gardien le tue quand le
# processus maître disparaît (même par SIGKILL), ou quand le journal est plein.
# PID exacts, jamais un groupe (le groupe est celui de sermo).
fermer() {
	local g
	g=$(cat "$BC_DIR/gardien" 2>/dev/null)
	if [[ "$g" =~ ^[0-9]+$ ]]; then kill -TERM "$g" 2>/dev/null; fi
	rm -f -- "$BC_DIR/gardien"
	echo "écoute arrêtée" > "$BC_DIR/etat"
}
ouvrir() {
	local i=$1 n
	iface_valide "$i" || refus "interface « $i » (can0, vcan0… existante et de type CAN)"
	fermer
	: > "$BC_DIR/journal"; rm -f -- "$BC_DIR/lecteur"
	bash -c '
		maitre=$1; iface=$2; journal=$3; max=$4
		candump -L "$iface" 2>"$journal.err" >> "$journal" &
		lecteur=$!
		echo "$lecteur" > "${journal%/*}/lecteur"
		trap '"'"'kill -TERM "$lecteur" 2>/dev/null; exit 0'"'"' TERM INT HUP
		# Plafond vérifié chaque seconde : dépassement possible, au plus une seconde de trafic.
		while kill -0 "$maitre" 2>/dev/null && kill -0 "$lecteur" 2>/dev/null &&
		      [ "$(stat -c %s -- "$journal")" -lt "$max" ]; do sleep 1 & wait $!; done
		kill -TERM "$lecteur" 2>/dev/null
	' _ "${BC_MAITRE:-$PPID}" "$i" "$BC_DIR/journal" "$JOURNAL_MAX" </dev/null >/dev/null 2>&1 &
	echo $! > "$BC_DIR/gardien"
	for n in 1 2 3 4 5 6 7 8 9 10; do [ -s "$BC_DIR/lecteur" ] && break; sleep 0.1; done
	printf 'écoute de %s\n' "$i" | tee "$BC_DIR/etat"
}

# ── Inventaire : une ligne par identifiant, depuis le journal (lignes candump -L
# seulement : « (horodatage) interface ID#DONNÉES » ; toute autre ligne est ignorée).
inventaire() {
	local maintenant taille
	maintenant=$(date +%s.%N)
	LC_ALL=C awk -v t="$maintenant" '
		$1 ~ /^\([0-9]+\.[0-9]+\)$/ && $3 ~ /^([0-7][0-9A-F][0-9A-F]|[01][0-9A-F][0-9A-F][0-9A-F][0-9A-F][0-9A-F][0-9A-F][0-9A-F])#([0-9A-F][0-9A-F]){0,8}$/ {
			split($3, p, "#"); id = p[1]
			if (!(id in n)) ordre[++k] = id
			n[id]++; d[id] = p[2]; h[id] = substr($1, 2, length($1) - 2)
		}
		END {
			for (i = 1; i <= k; i++) {
				id = ordre[i]; x = d[id]; o = ""
				for (j = 1; j <= length(x); j += 2) o = o (j > 1 ? " " : "") substr(x, j, 2)
				printf "%s|%s|%d|%.1f s\n", id, (o == "" ? "(vide)" : o), n[id], t - h[id]
			}
		}' "$BC_DIR/journal" 2>/dev/null | sort -t'|' -k1,1 | head -n "$LIGNES_MAX"
	taille=$(wc -c < "$BC_DIR/journal" 2>/dev/null || echo 0)
	[ "$taille" -ge "$JOURNAL_MAX" ] && printf 'JOURNAL PLEIN|lecture arrêtée|%d octets|rouvrez\n' "$taille"
	return 0
}

# ── Envoi : seulement après une approbation qui montre la trame exacte ──────
envoyer() {
	local i=$1 id=$2 dn=$3 trame o
	iface_valide "$i" || refus "interface « $i »"
	id_valide "$id" || refus "identifiant « $id » (3 chiffres hex jusqu'à 7FF, ou 8 jusqu'à 1FFFFFFF)"
	donnees_valides "$dn" || refus "données « $dn » (0 à 8 octets en hexadécimal, sans espace)"
	id=${id^^}; dn=${dn^^}
	trame="$id#$dn"
	o=$(sed 's/../& /g; s/ $//' <<<"$dn")
	printf 'Bus : %s\nIdentifiant : %s (%s bits)\nOctets (%d) : %s\nTrame cansend : %s\n\nSur un bus réel, un calculateur peut réagir à cette trame.\n' \
		"$i" "$id" "$( [ ${#id} = 3 ] && echo 11 || echo 29)" "$(( ${#dn} / 2 ))" "${o:-(aucun)}" "$trame" > "$BC_DIR/detail"
	[ -x "$APPROBATION" ] || { echo "porte d'approbation introuvable : $APPROBATION" | tee "$BC_DIR/etat" >&2; exit 2; }
	if ! "$APPROBATION" --titre "Envoi sur un bus CAN" \
	        --question "Envoyer cette trame sur le bus ?" \
	        --detail-fichier "$BC_DIR/detail" >/dev/null 2>&1; then
		echo "envoi refusé : rien n'a été envoyé" | tee "$BC_DIR/etat"
		return 1
	fi
	if cansend "$i" "$trame" 2>"$BC_DIR/envoi.err"; then
		printf 'trame %s envoyée sur %s\n' "$trame" "$i" | tee "$BC_DIR/etat"
	else
		printf 'échec de l'"'"'envoi : %s\n' "$(tail -n 1 "$BC_DIR/envoi.err")" | tee "$BC_DIR/etat"
		return 2
	fi
}

interface_detectee() {
	local i
	for i in $(LC_ALL=C ip -o link show 2>/dev/null | awk -F': ' '/link\/can/ {print $2}'); do
		[[ "$i" =~ ^v?can[0-9]{1,2}$ ]] && { printf '%s' "$i"; return; }
	done
	printf 'can0'
}

dialogue_principal() {
	cat <<XML
<window title="Bus CAN" default-width="680" default-height="560">
  <vbox spacing="6">
    <frame label="Bus">
      <hbox>
        <text><label>Interface :</label></text>
        <entry space-expand="true" space-fill="true"><default>$(xml "$(interface_detectee)")</default><variable>IFACE</variable></entry>
        <button><label>Écouter</label>
          <action>exec "\$SERMO_SCRIPT" --ouvrir "\$IFACE"</action><action>refresh:ETAT</action>
        </button>
        <button><label>Arrêter</label><action>exec "\$SERMO_SCRIPT" --fermer</action><action>refresh:ETAT</action></button>
      </hbox>
    </frame>
    <text><variable>ETAT</variable><input>cat "\$BC_DIR/etat"</input></text>
    <frame label="Identifiants vus">
      <tree space-expand="true" space-fill="true">
        <label>ID|Dernières données|Trames|Il y a</label>
        <variable>INVENTAIRE</variable>
        <height>240</height>
        <input>exec "\$SERMO_SCRIPT" --inventaire</input>
      </tree>
    </frame>
    <frame label="Envoyer UNE trame (une fenêtre de confirmation montre l'identifiant et les octets)">
      <hbox>
        <text><label>ID :</label></text>
        <entry width-chars="9"><variable>ID</variable></entry>
        <text><label>Données (hex) :</label></text>
        <entry space-expand="true" space-fill="true"><variable>DONNEES</variable></entry>
        <button><label>Envoyer…</label>
          <action>exec "\$SERMO_SCRIPT" --envoyer "\$IFACE" "\$ID" "\$DONNEES"</action><action>refresh:ETAT</action>
        </button>
      </hbox>
    </frame>
    <timer interval="1" visible="false">
      <variable>HORLOGE</variable>
      <action>refresh:INVENTAIRE</action>
    </timer>
    <hbox><button><label>Fermer</label><action>exit:fermer</action></button></hbox>
  </vbox>
</window>
XML
}

preparer_dossier() {
	if [ -n "${BC_DIR:-}" ] && [ -d "$BC_DIR" ]; then return 0; fi
	BC_DIR=$(mktemp -d "${XDG_RUNTIME_DIR:-${TMPDIR:-/tmp}}/sermo-can.XXXXXX") || exit 2
	export BC_DIR; BC_PROPRE=1
	: > "$BC_DIR/journal"; echo "lecture seule — aucune écoute" > "$BC_DIR/etat"
}

usage() { sed -n '/^# Usage/,/^# Codes/p' "$0" | sed '$d;s/^# \{0,1\}//' >&2; exit 64; }

main() {
	local c
	umask 077
	for c in candump cansend ip awk; do
		command -v "$c" >/dev/null || { echo "$c est absent : sudo apt install can-utils iproute2" >&2; exit 2; }
	done
	case "${1:-}" in
	--ouvrir|--envoyer|--fermer|--inventaire)
		preparer_dossier
		[ "${BC_PROPRE:-}" = 1 ] && [ "$1" != --ouvrir ] && trap 'rm -rf -- "$BC_DIR"' EXIT
		case "$1" in
		--ouvrir)     ouvrir "${2:-}" ;;
		--envoyer)    envoyer "${2:-}" "${3:-}" "${4:-}" ;;
		--fermer)     fermer ;;
		--inventaire) inventaire ;;
		esac
		exit ;;
	--xml) preparer_dossier; [ "${BC_PROPRE:-}" = 1 ] && trap 'rm -rf -- "$BC_DIR"' EXIT; dialogue_principal; exit 0 ;;
	-*)    usage ;;
	esac
	[ -n "${1:-}" ] && SERMO=$1
	command -v "$SERMO" >/dev/null || { echo "sermo introuvable : $SERMO" >&2; exit 2; }
	preparer_dossier
	export BC_MAITRE=$$
	trap 'fermer; rm -rf -- "$BC_DIR"' EXIT
	trap 'exit 129' HUP; trap 'exit 130' INT; trap 'exit 143' TERM
	export DIALOGUE; DIALOGUE=$(dialogue_principal)
	"$SERMO" --program=DIALOGUE >/dev/null
	exit 0
}

[ "${BASH_SOURCE[0]}" = "$0" ] && main "$@"
