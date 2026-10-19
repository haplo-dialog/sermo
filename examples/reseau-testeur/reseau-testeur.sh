#!/bin/bash
# SPDX-License-Identifier: CC0-1.0
# Métier   : Administration système, développement réseau, dépannage
# Besoin   : voir les connexions TCP de la machine, tester un port, envoyer une
#            trame texte ou une requête HTTP, sans droits particuliers
# Enrobe   : ss (sockets), bash /dev/tcp et /dev/udp + timeout (port, trames),
#            ping (latence), curl (HTTP)
# Ports    : tout port à fenêtre (entry, button, tree, edit, frame seulement)
# Terminal : ncursessermo par SSH : rien de perdu
# Usage    : ./reseau-testeur.sh [binaire sermo]
#            En ligne de commande, sans fenêtre (mêmes contrôles) :
#              ./reseau-testeur.sh --tester HÔTE PORT [DÉLAI]
#              ./reseau-testeur.sh --trame HÔTE PORT DÉLAI TEXTE
#              ./reseau-testeur.sh --udp HÔTE PORT DÉLAI TEXTE
#              ./reseau-testeur.sh --ping HÔTE [DÉLAI]
#              ./reseau-testeur.sh --http URL [DÉLAI]
#              ./reseau-testeur.sh --sockets [FILTRE]
#
# Codes de sortie : 0 fait · 2 outil absent · 64 entrée invalide.
# Les champs du dialogue arrivent par l'environnement puis en arguments séparés :
# jamais d'eval, jamais de chaîne de commande. Une réponse reçue est une donnée :
# elle est plafonnée (64 Kio), nettoyée, et affichée depuis un fichier, jamais
# insérée dans le XML.

export SERMO="${SERMO:-sermo}"
export SERMO_SCRIPT="$0"
PLAFOND=65536

xml() { local s=${1//&/"&amp;"}; s=${s//</"&lt;"}; s=${s//>/"&gt;"}; printf '%s' "${s//\"/"&quot;"}"; }

# ── Contrôles d'entrée : avant tout usage, sur la valeur entière ─────────────
hote_valide() {
	local h=$1 o
	[ -n "$h" ] && [ "${#h}" -le 253 ] || return 1
	case "$h" in -*) return 1 ;; esac
	if [[ "$h" =~ ^[0-9]{1,3}(\.[0-9]{1,3}){3}$ ]]; then
		for o in ${h//./ }; do [ "$((10#$o))" -le 255 ] || return 1; done
		return 0
	fi
	[[ "$h" =~ ^[0-9A-Fa-f:]+$ && "$h" == *:* && "${#h}" -le 39 ]] && return 0
	[[ "$h" =~ ^[A-Za-z0-9]([A-Za-z0-9-]{0,62})(\.[A-Za-z0-9]([A-Za-z0-9-]{0,62}))*\.?$ ]]
}
port_valide()  { [[ "$1" =~ ^[1-9][0-9]{0,4}$ ]] && [ "$1" -le 65535 ]; }
delai_valide() { [[ "$1" =~ ^[1-9][0-9]?$ ]] && [ "$1" -le 60 ]; }
trame_valide() { [ -n "$1" ] && [ "${#1}" -le 4096 ] && [[ "$1" != *$'\n'* && "$1" != *$'\r'* ]]; }
url_valide() {
	local u=$1 reste
	[ "${#u}" -le 2048 ] || return 1
	[[ "$u" =~ ^https?://[^[:space:][:cntrl:]]+$ ]] || return 1
	reste=${u#*://}; reste=${reste%%[/?#]*}
	[[ "$reste" != *@* ]] || return 1          # pas d'identifiants dans l'URL
	reste=${reste%:*}; reste=${reste#[}; reste=${reste%]}
	hote_valide "$reste"
}

refus() { printf 'entrée invalide : %s\n' "$1" | tee "$RT_DIR/resultat" 2>/dev/null; journal "refus : $1"; exit 64; }
journal() {
	[ -n "${RT_DIR:-}" ] || return 0
	printf '%s  %s\n' "$(date +%T)" "$1" >> "$RT_DIR/journal"
	tail -n 20 "$RT_DIR/journal" > "$RT_DIR/journal.n" && mv -f "$RT_DIR/journal.n" "$RT_DIR/journal"
}

# Texte reçu → fichier affichable : 64 Kio au plus, UTF-8 valide, sans
# caractères de contrôle autres que tabulation et fin de ligne.
nettoyer() {   # nettoyer <brut> <sortie>
	local taille
	taille=$(wc -c < "$1")
	head -c "$PLAFOND" -- "$1" | LC_ALL=C tr -d '\000-\010\013\014\016-\037\177' |
		iconv -f UTF-8 -t UTF-8 -c > "$2"
	if [ "$taille" -gt "$PLAFOND" ]; then
		printf '\n[… tronqué : plus de %d octets reçus, %d affichés]\n' "$taille" "$PLAFOND" >> "$2"
	fi
}

# ── Test de port : ouvert · fermé · filtré · délai (et erreur de nom) ────────
tester() {
	local h=$1 p=$2 d=${3:-3} err rc etat
	hote_valide "$h" || refus "hôte « $h »"
	port_valide "$p" || refus "port « $p » (1 à 65535)"
	delai_valide "$d" || refus "délai « $d » (1 à 60 s)"
	err=$(LC_ALL=C timeout "$d" bash -c 'exec 3<>"/dev/tcp/$1/$2"' _ "$h" "$p" 2>&1); rc=$?
	case "$rc:$err" in
	0:*)                                   etat="ouvert" ;;
	124:*)                                 etat="délai : pas de réponse en $d s (filtré sans retour, ou hôte éteint)" ;;
	*[Rr]efused*)                          etat="fermé : la machine répond, rien n'écoute" ;;
	*"No route"*|*nreachable*|*"not permitted"*) etat="filtré : refus réseau (pare-feu ou route absente)" ;;
	*"not known"*|*"resolve"*|*"No address"*|*"Temporary failure"*) etat="nom inconnu : $h ne se résout pas" ;;
	*)                                     etat="erreur : ${err##*: }" ;;
	esac
	[[ "$h" == *:* ]] && h="[$h]"
	printf '%s:%s → %s\n' "$h" "$p" "$etat" | tee "$RT_DIR/resultat"
	journal "test $h:$p → ${etat%% *}"
}

# ── Trame texte : envoyée telle que tapée, réponse lue jusqu'à fermeture ou délai
trame() {
	local h=$1 p=$2 d=$3 t=$4 rc
	hote_valide "$h" || refus "hôte « $h »"
	port_valide "$p" || refus "port « $p »"
	delai_valide "$d" || refus "délai « $d »"
	trame_valide "$t" || refus "trame vide, sur plusieurs lignes ou plus longue que 4096 octets"
	LC_ALL=C timeout "$d" bash -c 'exec 3<>"/dev/tcp/$1/$2" || exit 3; printf "%s\r\n" "$3" >&3; head -c "$4" <&3' \
		_ "$h" "$p" "$t" "$((PLAFOND + 1))" > "$RT_DIR/brut" 2> "$RT_DIR/erreur"; rc=$?
	nettoyer "$RT_DIR/brut" "$RT_DIR/reponse"
	case $rc in
	0)   printf 'trame envoyée à %s:%s, réponse ci-dessous\n' "$h" "$p" ;;
	124) printf 'trame envoyée à %s:%s, délai de %s s atteint (réponse partielle)\n' "$h" "$p" "$d" ;;
	*)   printf 'échec vers %s:%s : %s\n' "$h" "$p" "$(tail -n 1 "$RT_DIR/erreur" | sed 's/.*: //')" ;;
	esac | tee "$RT_DIR/resultat"
	journal "trame $h:$p (rc=$rc)"
}

# ── Datagramme UDP : envoyé tel que tapé ; une réponse, un refus ICMP, ou rien.
# UDP n'a pas de connexion : un silence ne prouve RIEN (port fermé sans ICMP,
# filtré, ou service qui ne répond pas à ce texte). L'écran le dit.
udp() {
	local h=$1 p=$2 d=$3 t=$4 rc err
	hote_valide "$h" || refus "hôte « $h »"
	port_valide "$p" || refus "port « $p »"
	delai_valide "$d" || refus "délai « $d »"
	trame_valide "$t" || refus "datagramme vide, sur plusieurs lignes ou plus long que 4096 octets"
	LC_ALL=C timeout "$d" bash -c 'exec 3<>"/dev/udp/$1/$2" || exit 3; printf "%s\n" "$3" >&3; dd bs=65536 count=1 status=none <&3' \
		_ "$h" "$p" "$t" > "$RT_DIR/brut" 2> "$RT_DIR/erreur"; rc=$?   # un read() = un datagramme
	err=$(cat "$RT_DIR/erreur")
	nettoyer "$RT_DIR/brut" "$RT_DIR/reponse"
	[[ "$h" == *:* ]] && h="[$h]"
	if [ -s "$RT_DIR/brut" ]; then
		printf 'UDP %s:%s → réponse reçue (%d octets), ci-dessous\n' "$h" "$p" "$(wc -c < "$RT_DIR/brut")"
	elif [[ "$err" == *[Rr]efused* ]]; then
		printf 'UDP %s:%s → fermé : la machine a répondu « port injoignable » (ICMP)\n' "$h" "$p"
	elif [ "$rc" -eq 124 ]; then
		printf 'UDP %s:%s → pas de réponse en %s s : fermé sans ICMP, filtré, ou service muet (UDP ne permet pas de trancher)\n' "$h" "$p" "$d"
	else
		printf 'UDP %s:%s → erreur : %s\n' "$h" "$p" "${err##*: }"
	fi | tee "$RT_DIR/resultat"
	journal "udp $h:$p (rc=$rc)"
}

# ── Ping : 4 échos ICMP, pertes et temps ────────────────────────────────────
ping_hote() {
	local h=$1 d=${2:-3} rc
	hote_valide "$h" || refus "hôte « $h »"
	delai_valide "$d" || refus "délai « $d »"
	command -v ping >/dev/null || { echo "ping est absent : sudo apt install iputils-ping" | tee "$RT_DIR/resultat"; exit 2; }
	LC_ALL=C timeout "$((d + 5))" ping -n -c 4 -i 0.5 -W "$d" -- "$h" > "$RT_DIR/brut" 2> "$RT_DIR/erreur"; rc=$?
	cat "$RT_DIR/erreur" >> "$RT_DIR/brut"
	nettoyer "$RT_DIR/brut" "$RT_DIR/reponse"
	{
		if grep -q 'packets transmitted' "$RT_DIR/reponse"; then
			printf 'ping %s : %s' "$h" "$(grep 'packets transmitted' "$RT_DIR/reponse" | head -n 1 | sed 's/, time .*//')"
			grep -q '^rtt' "$RT_DIR/reponse" && printf ' ; temps min/moy/max %s' "$(sed -n 's|^rtt [^=]*= \([0-9.]*\)/\([0-9.]*\)/\([0-9.]*\)/.*|\1/\2/\3 ms|p' "$RT_DIR/reponse")"
			printf '\n'
		else
			printf 'ping %s : échec (%s)\n' "$h" "$(grep -v '^$' "$RT_DIR/reponse" | tail -n 1 | sed 's/^ping: //')"   # tiré de la sortie NETTOYÉE
		fi
	} | tee "$RT_DIR/resultat"
	journal "ping $h (rc=$rc)"
}

# ── Requête HTTP(S) : en-têtes et début du corps ─────────────────────────────
http() {
	local u=$1 d=${2:-10} rc
	url_valide "$u" || refus "URL « $u » (http:// ou https://, sans identifiants)"
	delai_valide "$d" || refus "délai « $d »"
	command -v curl >/dev/null || { echo "curl est absent : sudo apt install curl" | tee "$RT_DIR/resultat"; exit 2; }
	curl -sS -i -m "$d" --proto '=http,https' --proto-redir '=http,https' -- "$u" 2> "$RT_DIR/erreur" |
		head -c "$((PLAFOND + 1))" > "$RT_DIR/brut"; rc=${PIPESTATUS[0]}
	nettoyer "$RT_DIR/brut" "$RT_DIR/reponse"
	if [ "$rc" -eq 0 ] || [ "$rc" -eq 23 ]; then   # 23 : sortie coupée par le plafond
		printf 'HTTP : %s\n' "$(head -n 1 "$RT_DIR/reponse" | tr -d '\r')"
	else
		printf 'HTTP : échec (curl %s) %s\n' "$rc" "$(tail -n 1 "$RT_DIR/erreur")"
	fi | tee "$RT_DIR/resultat"
	journal "http ${u:0:80} (curl $rc)"
}

# ── Sockets TCP, une ligne par socket pour le <tree> : état|locale|distante|processus
sockets() {
	local filtre=${1:-}
	command -v ss >/dev/null || { echo "ss absent (paquet iproute2)|—|—|—"; return 2; }
	LC_ALL=C ss -tanpH 2>/dev/null | awk '{
		proc = "—"
		if (match($0, /users:\(\("[^"]*"/)) proc = substr($0, RSTART + 9, RLENGTH - 10)
		gsub(/\|/, "¦", proc)
		printf "%s|%s|%s|%s\n", $1, $4, $5, proc
	}' | { if [ -n "$filtre" ]; then grep -iF -- "$filtre"; else cat; fi; } | head -n 500
	return 0
}

dialogue_principal() {
	cat <<XML
<window title="Testeur réseau" default-width="760" default-height="640">
  <vbox spacing="6">
    <frame label="Connexions TCP de cette machine">
      <vbox>
        <hbox>
          <text><label>Filtre :</label></text>
          <entry space-expand="true" space-fill="true"><variable>FILTRE</variable></entry>
          <button><label>Actualiser</label><action>refresh:SOCKETS</action></button>
        </hbox>
        <tree space-expand="true" space-fill="true">
          <label>État|Adresse locale|Adresse distante|Processus</label>
          <variable>SOCKETS</variable>
          <height>160</height>
          <input>exec "\$SERMO_SCRIPT" --sockets "\$FILTRE"</input>
        </tree>
      </vbox>
    </frame>
    <frame label="Port, ping, trames TCP/UDP, HTTP">
      <vbox>
        <hbox>
          <text><label>Hôte :</label></text>
          <entry space-expand="true" space-fill="true"><default>127.0.0.1</default><variable>HOTE</variable></entry>
          <text><label>Port :</label></text>
          <entry width-chars="7"><default>80</default><variable>PORT</variable></entry>
          <text><label>Délai (s) :</label></text>
          <entry width-chars="4"><default>3</default><variable>DELAI</variable></entry>
          <button><label>Tester le port</label>
            <action>exec "\$SERMO_SCRIPT" --tester "\$HOTE" "\$PORT" "\$DELAI"</action>
            <action>refresh:RESULTAT</action><action>refresh:JOURNAL</action>
          </button>
          <button><label>Ping</label>
            <action>exec "\$SERMO_SCRIPT" --ping "\$HOTE" "\$DELAI"</action>
            <action>refresh:RESULTAT</action><action>refresh:REPONSE</action><action>refresh:JOURNAL</action>
          </button>
        </hbox>
        <hbox>
          <text><label>Trame :</label></text>
          <entry space-expand="true" space-fill="true"><variable>TRAME</variable></entry>
          <button><label>Envoyer (TCP)</label>
            <action>exec "\$SERMO_SCRIPT" --trame "\$HOTE" "\$PORT" "\$DELAI" "\$TRAME"</action>
            <action>refresh:RESULTAT</action><action>refresh:REPONSE</action><action>refresh:JOURNAL</action>
          </button>
          <button><label>Envoyer (UDP)</label>
            <action>exec "\$SERMO_SCRIPT" --udp "\$HOTE" "\$PORT" "\$DELAI" "\$TRAME"</action>
            <action>refresh:RESULTAT</action><action>refresh:REPONSE</action><action>refresh:JOURNAL</action>
          </button>
        </hbox>
        <hbox>
          <text><label>URL :</label></text>
          <entry space-expand="true" space-fill="true"><default>http://127.0.0.1/</default><variable>URL</variable></entry>
          <button><label>Requête HTTP</label>
            <action>exec "\$SERMO_SCRIPT" --http "\$URL" "\$DELAI"</action>
            <action>refresh:RESULTAT</action><action>refresh:REPONSE</action><action>refresh:JOURNAL</action>
          </button>
        </hbox>
        <text><variable>RESULTAT</variable><input>cat "\$RT_DIR/resultat"</input></text>
        <edit editable="false" space-expand="true" space-fill="true"><variable>REPONSE</variable><input file>$(xml "$RT_DIR/reponse")</input></edit>
      </vbox>
    </frame>
    <frame label="Journal de la session (effacé à la fermeture)">
      <edit editable="false"><variable>JOURNAL</variable><height>90</height><input file>$(xml "$RT_DIR/journal")</input></edit>
    </frame>
    <hbox>
      <button><label>Fermer</label><action>exit:fermer</action></button>
    </hbox>
  </vbox>
</window>
XML
}

preparer_dossier() {
	[ -n "${RT_DIR:-}" ] && [ -d "$RT_DIR" ] && return 0
	local base=${XDG_RUNTIME_DIR:-${TMPDIR:-/tmp}}
	umask 077
	RT_DIR=$(mktemp -d "$base/sermo-reseau.XXXXXX") || exit 2
	export RT_DIR
	RT_PROPRE=1
	: > "$RT_DIR/resultat"; : > "$RT_DIR/reponse"; : > "$RT_DIR/journal"
}

main() {
	local c
	for c in bash timeout head iconv awk dd; do
		command -v "$c" >/dev/null || { echo "$c est absent : installez coreutils, gawk ou mawk, libc-bin." >&2; exit 2; }
	done
	case "${1:-}" in
	--tester|--trame|--udp|--ping|--http|--sockets)
		preparer_dossier
		[ "${RT_PROPRE:-}" = 1 ] && trap 'rm -rf -- "$RT_DIR"' EXIT
		case "$1" in
		--tester)  shift; tester "${1:-}" "${2:-}" "${3:-3}" ;;
		--trame)   shift; trame "${1:-}" "${2:-}" "${3:-3}" "${4:-}" ;;
		--udp)     shift; udp "${1:-}" "${2:-}" "${3:-3}" "${4:-}" ;;
		--ping)    shift; ping_hote "${1:-}" "${2:-3}" ;;
		--http)    shift; http "${1:-}" "${2:-10}" ;;
		--sockets) shift; sockets "${1:-}" ;;
		esac
		exit ;;
	--xml) preparer_dossier; [ "${RT_PROPRE:-}" = 1 ] && trap 'rm -rf -- "$RT_DIR"' EXIT; dialogue_principal; exit 0 ;;
	-*)    sed -n '/^# Usage/,/^# Codes/p' "$0" | sed '$d;s/^# \{0,1\}//' >&2; exit 64 ;;
	esac
	[ -n "${1:-}" ] && SERMO=$1
	command -v ss >/dev/null || { echo "ss est absent : sudo apt install iproute2" >&2; exit 2; }
	command -v "$SERMO" >/dev/null || { echo "sermo introuvable : $SERMO" >&2; exit 2; }
	preparer_dossier
	trap 'rm -rf -- "$RT_DIR"' EXIT
	trap 'exit 129' HUP; trap 'exit 130' INT; trap 'exit 143' TERM
	export DIALOGUE; DIALOGUE=$(dialogue_principal)
	"$SERMO" --program=DIALOGUE >/dev/null
	exit 0
}

[ "${BASH_SOURCE[0]}" = "$0" ] && main "$@"
