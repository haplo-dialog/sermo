#!/bin/bash
# SPDX-License-Identifier: CC0-1.0
# Métier   : Électronique, embarqué, maintenance d'automates et d'appareils de mesure
# Besoin   : écouter un port série (USB-série, carte, appareil) et, sur confirmation
#            explicite, y envoyer une ligne de texte
# Enrobe   : socat (lecture en fond, envoi) ; la confirmation passe par
#            examples/approbation/approbation.sh
# Ports    : tout port à fenêtre (la confirmation demande un affichage)
# Terminal : lecture seule par SSH (sans affichage, l'envoi est refusé)
# Usage    : ./liaison-serie.sh [binaire sermo]
#            Sans fenêtre (mêmes contrôles) :
#              ./liaison-serie.sh --ouvrir PÉRIPHÉRIQUE DÉBIT FORMAT
#              ./liaison-serie.sh --envoyer PÉRIPHÉRIQUE DÉBIT FORMAT FIN TEXTE
#              ./liaison-serie.sh --fermer
#            LS_DIR : dossier de session à réutiliser entre deux appels
#
# Codes de sortie : 0 fait · 1 envoi refusé par l'humain · 2 outil absent ·
# 64 entrée invalide. Lecture seule par défaut : RIEN n'est écrit sur la ligne sans
# une fenêtre d'approbation qui montre la trame exacte, octet par octet.
# Ce qui arrive sur la ligne est une donnée : plafonné, nettoyé, affiché depuis un
# fichier, jamais inséré dans le XML.

export SERMO="${SERMO:-sermo}"
export SERMO_SCRIPT="$0"
ICI=$(CDPATH= cd -- "$(dirname -- "$(readlink -f -- "$0")")" && pwd)
APPROBATION="${LS_APPROBATION:-$ICI/../approbation/approbation.sh}"
JOURNAL_MAX=1048576     # au-delà, la lecture s'arrête (journal plein)
VUE_MAX=8192            # ce que la fenêtre montre : la fin du journal
DEBITS="1200 2400 4800 9600 19200 38400 57600 115200 230400 460800 921600"
FORMATS="8N1 8E1 8O1 8N2 7E1 7O1"
FINS="CRLF LF CR aucune"

# ── Contrôles d'entrée ──────────────────────────────────────────────────────
xml() { local s=${1//&/"&amp;"}; s=${s//</"&lt;"}; s=${s//>/"&gt;"}; printf '%s' "${s//\"/"&quot;"}"; }
dans() { local x; for x in $2; do [ "$1" = "$x" ] && return 0; done; return 1; }
periph_valide() {
	local p=$1 reel
	[[ "$p" =~ ^/dev/(tty(S|USB|ACM|AMA|XRUSB)[0-9]{1,3}|pts/[0-9]{1,4}|serial/by-(id|path)/[A-Za-z0-9._:+-]{1,200})$ ]] || return 1
	reel=$(readlink -f -- "$p") || return 1
	[[ "$reel" =~ ^/dev/(tty[A-Z]+[0-9]{1,3}|pts/[0-9]{1,4})$ ]] && [ -c "$reel" ] && [ -r "$reel" ] && [ -w "$reel" ]
}
options_socat() {   # options_socat DÉBIT FORMAT → options de la ligne (sans le chemin)
	local b=$1 f=$2 cs par odd stop
	cs=cs${f:0:1}
	case ${f:1:1} in N) par=0; odd=0 ;; E) par=1; odd=0 ;; O) par=1; odd=1 ;; esac
	[ "${f:2:1}" = 2 ] && stop=1 || stop=0
	printf 'b%s,%s,parenb=%s,parodd=%s,cstopb=%s,raw,echo=0,noctty' "$b" "$cs" "$par" "$odd" "$stop"
}
refus() { printf 'entrée invalide : %s\n' "$1" | tee "$LS_DIR/etat" >&2; exit 64; }
controler() {   # controler PÉRIPH DÉBIT FORMAT
	periph_valide "$1" || refus "périphérique « $1 » (tty série existant, lisible et inscriptible : groupe dialout)"
	dans "$2" "$DEBITS" || refus "débit « $2 »"
	dans "$3" "$FORMATS" || refus "format « $3 »"
}

# ── Lecture en fond ─────────────────────────────────────────────────────────
# socat écrit dans le journal. Un gardien le surveille et le tue si le processus
# maître disparaît, même par SIGKILL : aucune écoute ne survit à la fenêtre. On tue
# des PID exacts, jamais un groupe (le groupe est celui de sermo).
fermer() {
	local g
	g=$(cat "$LS_DIR/gardien" 2>/dev/null)
	if [[ "$g" =~ ^[0-9]+$ ]]; then kill -TERM "$g" 2>/dev/null; fi
	rm -f -- "$LS_DIR/gardien"
	echo "liaison fermée" > "$LS_DIR/etat"
}
ouvrir() {
	local p=$1 b=$2 f=$3 opts i
	controler "$p" "$b" "$f"
	fermer
	opts=$(options_socat "$b" "$f")
	: > "$LS_DIR/journal"; rm -f -- "$LS_DIR/lecteur"
	bash -c '
		maitre=$1; periph=$2; opts=$3; journal=$4; max=$5
		socat -u "FILE:$periph,$opts" STDOUT 2>"$journal.err" >> "$journal" &
		lecteur=$!
		echo "$lecteur" > "${journal%/*}/lecteur"
		trap '"'"'kill -TERM "$lecteur" 2>/dev/null; exit 0'"'"' TERM INT HUP
		# Plafond vérifié chaque seconde : dépassement possible, au plus une seconde de débit.
		while kill -0 "$maitre" 2>/dev/null && kill -0 "$lecteur" 2>/dev/null &&
		      [ "$(stat -c %s -- "$journal")" -lt "$max" ]; do sleep 1 & wait $!; done
		kill -TERM "$lecteur" 2>/dev/null
	' _ "${LS_MAITRE:-$PPID}" "$p" "$opts" "$LS_DIR/journal" "$JOURNAL_MAX" </dev/null >/dev/null 2>&1 &
	echo $! > "$LS_DIR/gardien"
	for i in 1 2 3 4 5 6 7 8 9 10; do [ -s "$LS_DIR/lecteur" ] && break; sleep 0.1; done
	printf 'écoute de %s à %s bauds, %s\n' "$p" "$b" "$f" | tee "$LS_DIR/etat"
}

# ── Ce que montre la fenêtre : la fin du journal, en texte ou en hexadécimal
affiche() {
	local hexa=$1 taille
	taille=$(wc -c < "$LS_DIR/journal" 2>/dev/null || echo 0)
	if [ "$hexa" = true ]; then
		tail -c "$VUE_MAX" -- "$LS_DIR/journal" | od -An -tx1 -v -w16 > "$LS_DIR/vue.n"
	else
		tail -c "$VUE_MAX" -- "$LS_DIR/journal" | LC_ALL=C tr -d '\000-\010\013-\037\177' |
			iconv -f UTF-8 -t UTF-8 -c > "$LS_DIR/vue.n"
	fi
	[ "$taille" -ge "$JOURNAL_MAX" ] && printf '\n[journal plein (%d octets) : lecture arrêtée, rouvrez la liaison]\n' "$taille" >> "$LS_DIR/vue.n"
	mv -f -- "$LS_DIR/vue.n" "$LS_DIR/vue"
}

# ── Envoi : seulement après une approbation qui montre la trame exacte ──────
envoyer() {
	local p=$1 b=$2 f=$3 fin=$4 t=$5 suffixe octets
	controler "$p" "$b" "$f"
	dans "$fin" "$FINS" || refus "fin de ligne « $fin »"
	[ -n "$t" ] && [ "${#t}" -le 1024 ] && [[ "$t" != *[$'\n\r']* ]] || refus "trame vide, sur plusieurs lignes ou plus longue que 1024 caractères"
	case $fin in CRLF) suffixe=$'\r\n' ;; LF) suffixe=$'\n' ;; CR) suffixe=$'\r' ;; aucune) suffixe= ;; esac
	printf '%s%s' "$t" "$suffixe" > "$LS_DIR/trame"
	octets=$(wc -c < "$LS_DIR/trame")
	{
		printf 'Vers : %s (%s bauds, %s)\nTexte : %s\nFin de ligne : %s\n\nOctets envoyés (%d) :\n' "$p" "$b" "$f" "$t" "$fin" "$octets"
		od -An -tx1 -v -w16 "$LS_DIR/trame"
	} > "$LS_DIR/detail"
	[ -x "$APPROBATION" ] || { echo "porte d'approbation introuvable : $APPROBATION" | tee "$LS_DIR/etat" >&2; exit 2; }
	if ! "$APPROBATION" --titre "Envoi sur une liaison série" \
	        --question "Écrire ces octets sur la ligne ? Un appareil branché peut réagir." \
	        --detail-fichier "$LS_DIR/detail" >/dev/null 2>&1; then
		echo "envoi refusé : rien n'a été écrit" | tee "$LS_DIR/etat"
		return 1
	fi
	if socat -u "OPEN:$LS_DIR/trame" "FILE:$p,$(options_socat "$b" "$f")" 2>"$LS_DIR/envoi.err"; then
		printf '%d octets envoyés à %s\n' "$octets" "$p" | tee "$LS_DIR/etat"
	else
		printf 'échec de l'"'"'envoi : %s\n' "$(tail -n 1 "$LS_DIR/envoi.err")" | tee "$LS_DIR/etat"
		return 2
	fi
}

ports_detectes() {
	local d premier=""
	for d in /dev/serial/by-id/* /dev/ttyUSB* /dev/ttyACM*; do
		[ -e "$d" ] && { premier=$d; break; }
	done
	printf '%s' "${premier:-/dev/ttyUSB0}"
}

dialogue_principal() {
	local x
	cat <<XML
<window title="Liaison série" default-width="720" default-height="560">
  <vbox spacing="6">
    <frame label="Ligne">
      <hbox>
        <entry space-expand="true" space-fill="true"><default>$(xml "$(ports_detectes)")</default><variable>PERIPH</variable></entry>
        <comboboxtext><variable>DEBIT</variable><default>9600</default>
$(for x in $DEBITS; do printf '          <item>%s</item>\n' "$x"; done)
        </comboboxtext>
        <comboboxtext><variable>FORMAT</variable><default>8N1</default>
$(for x in $FORMATS; do printf '          <item>%s</item>\n' "$x"; done)
        </comboboxtext>
        <button><label>Écouter</label>
          <action>exec "\$SERMO_SCRIPT" --ouvrir "\$PERIPH" "\$DEBIT" "\$FORMAT"</action><action>refresh:ETAT</action>
        </button>
        <button><label>Arrêter</label><action>exec "\$SERMO_SCRIPT" --fermer</action><action>refresh:ETAT</action></button>
      </hbox>
    </frame>
    <text><variable>ETAT</variable><input>cat "\$LS_DIR/etat"</input></text>
    <frame label="Reçu (fin du journal)">
      <vbox>
        <checkbox><label>Hexadécimal</label><variable>HEXA</variable></checkbox>
        <edit editable="false" space-expand="true" space-fill="true"><variable>VUE</variable><input file>$(xml "$LS_DIR/vue")</input></edit>
      </vbox>
    </frame>
    <frame label="Envoyer (une fenêtre de confirmation montre les octets exacts)">
      <hbox>
        <entry space-expand="true" space-fill="true"><variable>TRAME</variable></entry>
        <comboboxtext><variable>FIN</variable><default>CRLF</default>
$(for x in $FINS; do printf '          <item>%s</item>\n' "$x"; done)
        </comboboxtext>
        <button><label>Envoyer…</label>
          <action>exec "\$SERMO_SCRIPT" --envoyer "\$PERIPH" "\$DEBIT" "\$FORMAT" "\$FIN" "\$TRAME"</action><action>refresh:ETAT</action>
        </button>
      </hbox>
    </frame>
    <timer interval="1" visible="false">
      <variable>HORLOGE</variable>
      <action>exec "\$SERMO_SCRIPT" --affiche "\$HEXA"</action>
      <action>refresh:VUE</action>
    </timer>
    <hbox><button><label>Fermer</label><action>exit:fermer</action></button></hbox>
  </vbox>
</window>
XML
}

preparer_dossier() {
	if [ -n "${LS_DIR:-}" ] && [ -d "$LS_DIR" ]; then return 0; fi
	umask 077
	LS_DIR=$(mktemp -d "${XDG_RUNTIME_DIR:-${TMPDIR:-/tmp}}/sermo-serie.XXXXXX") || exit 2
	export LS_DIR; LS_PROPRE=1
	: > "$LS_DIR/journal"; : > "$LS_DIR/vue"; echo "lecture seule — aucune liaison ouverte" > "$LS_DIR/etat"
}

usage() { sed -n '/^# Usage/,/^# Codes/p' "$0" | sed '$d;s/^# \{0,1\}//' >&2; exit 64; }

main() {
	umask 077
	local c
	for c in socat od iconv tail; do
		command -v "$c" >/dev/null || { echo "$c est absent : sudo apt install socat coreutils libc-bin" >&2; exit 2; }
	done
	case "${1:-}" in
	--ouvrir|--envoyer|--fermer|--affiche)
		preparer_dossier
		[ "${LS_PROPRE:-}" = 1 ] && [ "$1" != --ouvrir ] && trap 'rm -rf -- "$LS_DIR"' EXIT
		case "$1" in
		--ouvrir)  ouvrir "${2:-}" "${3:-}" "${4:-}" ;;
		--envoyer) envoyer "${2:-}" "${3:-}" "${4:-}" "${5:-}" "${6:-}" ;;
		--fermer)  fermer ;;
		--affiche) affiche "${2:-false}" ;;
		esac
		exit ;;
	--xml) preparer_dossier; [ "${LS_PROPRE:-}" = 1 ] && trap 'rm -rf -- "$LS_DIR"' EXIT; dialogue_principal; exit 0 ;;
	-*)    usage ;;
	esac
	[ -n "${1:-}" ] && SERMO=$1
	command -v "$SERMO" >/dev/null || { echo "sermo introuvable : $SERMO" >&2; exit 2; }
	preparer_dossier
	export LS_MAITRE=$$
	trap 'fermer; rm -rf -- "$LS_DIR"' EXIT
	trap 'exit 129' HUP; trap 'exit 130' INT; trap 'exit 143' TERM
	export DIALOGUE; DIALOGUE=$(dialogue_principal)
	"$SERMO" --program=DIALOGUE >/dev/null
	exit 0
}

[ "${BASH_SOURCE[0]}" = "$0" ] && main "$@"
