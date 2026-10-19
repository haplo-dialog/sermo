#!/bin/bash
# SPDX-License-Identifier: CC0-1.0
# Métier   : Administration système et réseau, support, hébergement
# Besoin   : résoudre un nom auprès d'un ou plusieurs résolveurs, comparer leurs
#            réponses (un résolveur qui ment se voit), voir statut, DNSSEC et temps
# Enrobe   : dig (paquet bind9-dnsutils)
# Ports    : tout port à fenêtre (entry, comboboxtext, button, tree, edit)
# Terminal : ncursessermo par SSH : rien de perdu
# Usage    : ./dns-testeur.sh [binaire sermo]
#            Sans fenêtre (mêmes contrôles ; DT_DIR = dossier de session) :
#              ./dns-testeur.sh --interroger NOM TYPE "RÉSOLVEURS" [DÉLAI]
#            RÉSOLVEURS : jusqu'à 6, séparés par des espaces : « système » (celui
#            de la machine), une adresse IPv4/IPv6, ou adresse#port.
#
# Codes de sortie : 0 fait · 2 dig absent · 64 entrée invalide.
# Confidentialité : interroger un résolveur public lui apprend le nom demandé.
# Par défaut seul le résolveur de la machine est interrogé.
# Les réponses sont des données : seules les lignes au format d'un
# enregistrement sont gardées, nettoyées, jamais insérées dans le XML.

export SERMO="${SERMO:-sermo}"
export SERMO_SCRIPT="$0"
TYPES="A AAAA MX TXT NS CNAME SOA CAA SRV PTR"

xml() { local s=${1//&/"&amp;"}; s=${s//</"&lt;"}; s=${s//>/"&gt;"}; printf '%s' "${s//\"/"&quot;"}"; }

# ── Contrôles d'entrée ──────────────────────────────────────────────────────
nom_valide() {
	local n=$1
	[ -n "$n" ] && [ "${#n}" -le 253 ] || return 1
	case "$n" in -*|.*) return 1 ;; esac
	[[ "$n" =~ ^([A-Za-z0-9_]([A-Za-z0-9_-]{0,61}[A-Za-z0-9_])?\.)*[A-Za-z0-9_]([A-Za-z0-9_-]{0,61}[A-Za-z0-9_])?\.?$ ]]
}
ip_valide() {
	local a=$1 o
	if [[ "$a" =~ ^[0-9]{1,3}(\.[0-9]{1,3}){3}$ ]]; then
		for o in ${a//./ }; do [ "$((10#$o))" -le 255 ] || return 1; done; return 0
	fi
	[[ "$a" =~ ^[0-9A-Fa-f:]+$ && "$a" == *:* && "${#a}" -le 39 ]]
}
resolveur_valide() {   # « système », IP ou IP#port
	local r=$1 a p
	[ "$r" = système ] && return 0
	a=${r%%#*}; p=${r#*#}
	ip_valide "$a" || return 1
	[ "$r" = "$a" ] && return 0
	[[ "$p" =~ ^[1-9][0-9]{0,4}$ ]] && [ "$p" -le 65535 ]
}
delai_valide() { [[ "$1" =~ ^[1-9][0-9]?$ ]] && [ "$1" -le 30 ]; }
dans() { local x; for x in $2; do [ "$1" = "$x" ] && return 0; done; return 1; }
refus() { printf 'entrée invalide : %s\n' "$1" | tee "$DT_DIR/etat" >&2; : > "$DT_DIR/resultats"; exit 64; }

# iconv EN DERNIER : cut -c coupe en octets et pourrait casser un caractère.
nettoyer() { LC_ALL=C tr -d '\000-\010\013-\037\177' | cut -c1-400 | iconv -f UTF-8 -t UTF-8 -c; }

# ── Une interrogation par résolveur ─────────────────────────────────────────
interroger() {
	local nom=$1 type=$2 liste=$3 d=${4:-3} r n=0 args sortie statut ad temps reps empreinte ref="" meme
	local -a resolveurs
	nom_valide "$nom" || refus "nom « $nom »"
	dans "$type" "$TYPES" || refus "type « $type »"
	delai_valide "$d" || refus "délai « $d » (1 à 30 s)"
	read -r -a resolveurs <<<"$liste"
	[ "${#resolveurs[@]}" -ge 1 ] && [ "${#resolveurs[@]}" -le 6 ] || refus "de 1 à 6 résolveurs"
	for r in "${resolveurs[@]}"; do resolveur_valide "$r" || refus "résolveur « $r » (système, IPv4, IPv6, ou adresse#port)"; done
	command -v dig >/dev/null || { echo "dig est absent : sudo apt install bind9-dnsutils" | tee "$DT_DIR/etat"; exit 2; }
	: > "$DT_DIR/resultats"; : > "$DT_DIR/detail"
	for r in "${resolveurs[@]}"; do
		args=(+noall +comments +answer +stats "+time=$d" +tries=1 +adflag)
		if [ "$r" != système ]; then
			args+=("@${r%%#*}")
			[ "$r" != "${r%%#*}" ] && args+=(-p "${r#*#}")
		fi
		sortie=$(LC_ALL=C timeout "$((d + 2))" dig "${args[@]}" -t "$type" -q "$nom" 2>&1 | head -c 65536)
		statut=$(sed -n 's/.*->>HEADER<<-.* status: \([A-Z]*\),.*/\1/p' <<<"$sortie" | head -n 1)
		[ -n "$statut" ] || statut="pas de réponse"
		if grep -qE '^;; flags:[a-z ]* ad[ ;]' <<<"$sortie"; then ad="validé"; else ad="non"; fi
		temps=$(sed -n 's/^;; Query time: \([0-9]*\) msec.*/\1 ms/p' <<<"$sortie" | head -n 1)
		# Enregistrements : « nom TTL classe type données », rien d'autre.
		reps=$(LC_ALL=C awk -v t="$type" 'NF >= 5 && $1 !~ /^;/ && $2 ~ /^[0-9]+$/ && $3 == "IN" { $2 = ""; print }' <<<"$sortie" | nettoyer | head -n 50)
		empreinte=$(awk '{ $1 = ""; print }' <<<"$reps" | sort | md5sum | cut -c1-8)
		n=$((n + 1))
		if [ $n = 1 ]; then ref=$empreinte; meme="(référence)"
		elif [ "$empreinte" = "$ref" ]; then meme="oui"
		else meme="NON, différente"; fi
		printf '%s|%s|%s|%d|%s|%s\n' "$r" "$statut" "$ad" "$( [ -n "$reps" ] && wc -l <<<"$reps" || echo 0)" "${temps:-—}" "$meme" >> "$DT_DIR/resultats"
		{ printf '── %s : %s\n' "$r" "$statut"; [ -n "$reps" ] && printf '%s\n' "$reps"; [ "$statut" = "pas de réponse" ] && tail -n 1 <<<"$sortie" | nettoyer; echo; } >> "$DT_DIR/detail"
	done
	printf '%s %s : %d résolveur(s) interrogé(s)\n' "$nom" "$type" "$n" | tee "$DT_DIR/etat"
	cat "$DT_DIR/resultats"
}

dialogue_principal() {
	local x
	cat <<XML
<window title="Testeur DNS" default-width="760" default-height="560">
  <vbox spacing="6">
    <frame label="Question">
      <vbox>
        <hbox>
          <text><label>Nom :</label></text>
          <entry space-expand="true" space-fill="true"><default>debian.org</default><variable>NOM</variable></entry>
          <comboboxtext><variable>TYPE</variable><default>A</default>
$(for x in $TYPES; do printf '            <item>%s</item>\n' "$x"; done)
          </comboboxtext>
          <text><label>Délai (s) :</label></text>
          <entry width-chars="3"><default>3</default><variable>DELAI</variable></entry>
        </hbox>
        <hbox>
          <text><label>Résolveurs :</label></text>
          <entry space-expand="true" space-fill="true"><default>système</default><variable>RESOLVEURS</variable></entry>
          <button><label>Interroger</label>
            <action>exec "\$SERMO_SCRIPT" --interroger "\$NOM" "\$TYPE" "\$RESOLVEURS" "\$DELAI"</action>
            <action>refresh:ETAT</action><action>refresh:RESULTATS</action><action>refresh:DETAIL</action>
          </button>
        </hbox>
        <text><label>Ajouter un résolveur public (9.9.9.9, 1.1.1.1…) lui apprend le nom demandé.</label></text>
      </vbox>
    </frame>
    <text><variable>ETAT</variable><input>cat "\$DT_DIR/etat"</input></text>
    <tree>
      <label>Résolveur|Statut|DNSSEC|Réponses|Temps|Même réponse</label>
      <variable>RESULTATS</variable>
      <height>140</height>
      <input>cat "\$DT_DIR/resultats"</input>
    </tree>
    <edit editable="false" space-expand="true" space-fill="true"><variable>DETAIL</variable><input file>$(xml "$DT_DIR/detail")</input></edit>
    <hbox><button><label>Fermer</label><action>exit:fermer</action></button></hbox>
  </vbox>
</window>
XML
}

preparer_dossier() {
	if [ -n "${DT_DIR:-}" ] && [ -d "$DT_DIR" ]; then return 0; fi
	DT_DIR=$(mktemp -d "${XDG_RUNTIME_DIR:-${TMPDIR:-/tmp}}/sermo-dns.XXXXXX") || exit 2
	export DT_DIR; DT_PROPRE=1
	: > "$DT_DIR/resultats"; : > "$DT_DIR/detail"; echo "aucune question posée" > "$DT_DIR/etat"
}

usage() { sed -n '/^# Usage/,/^# Codes/p' "$0" | sed '$d;s/^# \{0,1\}//' >&2; exit 64; }

main() {
	umask 077
	local c
	for c in awk iconv md5sum timeout; do
		command -v "$c" >/dev/null || { echo "$c est absent : installez coreutils, gawk ou mawk, libc-bin" >&2; exit 2; }
	done
	case "${1:-}" in
	--interroger)
		preparer_dossier
		[ "${DT_PROPRE:-}" = 1 ] && trap 'rm -rf -- "$DT_DIR"' EXIT
		shift; interroger "${1:-}" "${2:-}" "${3:-}" "${4:-3}"; exit ;;
	--xml) preparer_dossier; [ "${DT_PROPRE:-}" = 1 ] && trap 'rm -rf -- "$DT_DIR"' EXIT; dialogue_principal; exit 0 ;;
	-*)    usage ;;
	esac
	[ -n "${1:-}" ] && SERMO=$1
	command -v "$SERMO" >/dev/null || { echo "sermo introuvable : $SERMO" >&2; exit 2; }
	command -v dig >/dev/null || { echo "dig est absent : sudo apt install bind9-dnsutils" >&2; exit 2; }
	preparer_dossier
	trap 'rm -rf -- "$DT_DIR"' EXIT
	trap 'exit 129' HUP; trap 'exit 130' INT; trap 'exit 143' TERM
	export DIALOGUE; DIALOGUE=$(dialogue_principal)
	"$SERMO" --program=DIALOGUE >/dev/null
	exit 0
}

[ "${BASH_SOURCE[0]}" = "$0" ] && main "$@"
