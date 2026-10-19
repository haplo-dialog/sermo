#!/bin/bash
# SPDX-License-Identifier: CC0-1.0
# Métier   : Administration système, hébergement, support, sécurité
# Besoin   : regarder le certificat TLS d'un serveur (le sien ou un autre) : chaîne,
#            vérification, nom, jours avant expiration, protocole — sans droits
# Enrobe   : openssl s_client et openssl x509
# Ports    : tout port à fenêtre (entry, button, tree, text)
# Terminal : ncursessermo par SSH : rien de perdu
# Usage    : ./certificat-tls.sh [binaire sermo]
#            Sans fenêtre (mêmes contrôles ; CT_DIR = dossier de session) :
#              ./certificat-tls.sh --verifier HÔTE PORT [NOM] [DÉLAI] [AUTORITÉ.pem]
#            NOM : le nom à vérifier (SNI) ; par défaut l'hôte s'il n'est pas une IP.
#            AUTORITÉ.pem : autorité d'une PKI interne, en plus de celles du système.
#            CT_ALERTE : jours avant expiration qui déclenchent l'alerte (défaut 30).
#
# Codes de sortie : 0 fait · 2 openssl absent · 64 entrée invalide.
# Ce que le serveur envoie est une donnée : sujets et émetteurs nettoyés,
# plafonnés, jamais insérés dans le XML. Rien n'est écrit hors du dossier de session.

export SERMO="${SERMO:-sermo}"
export SERMO_SCRIPT="$0"
export CT_ALERTE="${CT_ALERTE:-30}"

xml() { local s=${1//&/"&amp;"}; s=${s//</"&lt;"}; s=${s//>/"&gt;"}; printf '%s' "${s//\"/"&quot;"}"; }

# ── Contrôles d'entrée ──────────────────────────────────────────────────────
est_ip() {
	local h=$1 o
	if [[ "$h" =~ ^[0-9]{1,3}(\.[0-9]{1,3}){3}$ ]]; then
		for o in ${h//./ }; do [ "$((10#$o))" -le 255 ] || return 1; done; return 0
	fi
	[[ "$h" =~ ^[0-9A-Fa-f:]+$ && "$h" == *:* && "${#h}" -le 39 ]]
}
nom_valide() {
	[ -n "$1" ] && [ "${#1}" -le 253 ] && [[ "$1" =~ ^[A-Za-z0-9]([A-Za-z0-9-]{0,62})(\.[A-Za-z0-9]([A-Za-z0-9-]{0,62}))*\.?$ ]]
}
hote_valide() {
	case "$1" in -*) return 1 ;; esac
	[[ "$1" =~ ^[0-9.]+$ ]] && { est_ip "$1"; return; }   # « 256.1.1.1 » n'est pas un nom
	est_ip "$1" || nom_valide "$1"
}
port_valide()  { [[ "$1" =~ ^[1-9][0-9]{0,4}$ ]] && [ "$1" -le 65535 ]; }
delai_valide() { [[ "$1" =~ ^[1-9][0-9]?$ ]] && [ "$1" -le 30 ]; }
autorite_valide() { [[ "$1" == /* && "$1" != *$'\n'* ]] && [ -f "$1" ] && [ -r "$1" ]; }
refus() { printf 'entrée invalide : %s\n' "$1" | tee "$CT_DIR/etat" >&2; : > "$CT_DIR/chaine"; : > "$CT_DIR/bilan"; exit 64; }

# Texte venu du serveur → une cellule : sans contrôle, sans « | », UTF-8, 200 car.
# sed : tr couperait « ¦ » (2 octets) ; iconv EN DERNIER : cut -c coupe en octets.
cellule() { LC_ALL=C tr -d '\000-\037\177' | sed 's/|/¦/g' | cut -c1-200 | iconv -f UTF-8 -t UTF-8 -c; }

verifier() {
	local h=$1 p=$2 nom=$3 d=${4:-5} ca=${5:-} rc sortie maintenant i f fin s j etat
	local -a args
	hote_valide "$h" || refus "hôte « $h »"
	port_valide "$p" || refus "port « $p »"
	[ -n "$nom" ] || { est_ip "$h" || nom=$h; }
	[ -z "$nom" ] || nom_valide "$nom" || refus "nom à vérifier « $nom »"
	delai_valide "$d" || refus "délai « $d » (1 à 30 s)"
	[ -z "$ca" ] || autorite_valide "$ca" || refus "fichier d'autorité « $ca » (chemin absolu d'un fichier lisible)"
	[[ "$CT_ALERTE" =~ ^[0-9]{1,4}$ ]] || refus "CT_ALERTE « $CT_ALERTE »"
	command -v openssl >/dev/null || { echo "openssl est absent : sudo apt install openssl" | tee "$CT_DIR/etat"; exit 2; }
	args=(s_client -connect "$h:$p" -showcerts)
	[[ "$h" == *:* ]] && args=(s_client -connect "[$h]:$p" -showcerts)
	[ -n "$nom" ] && args+=(-servername "$nom" -verify_hostname "$nom")
	[ -n "$ca" ] && args+=(-CAfile "$ca")
	rm -f -- "$CT_DIR"/c-*.pem
	sortie=$(LC_ALL=C timeout "$d" openssl "${args[@]}" </dev/null 2>&1 | head -c 1048576); rc=$?
	# La chaîne envoyée, un fichier par certificat (10 au plus).
	awk -v dir="$CT_DIR" '/-----BEGIN CERTIFICATE-----/ { n++; if (n > 10) exit; f = sprintf("%s/c-%02d.pem", dir, n) }
		f { print > f } /-----END CERTIFICATE-----/ { close(f); f = "" }' <<<"$sortie"
	: > "$CT_DIR/chaine"; maintenant=$(date +%s); i=0; j=""
	for f in "$CT_DIR"/c-*.pem; do
		[ -e "$f" ] || break
		i=$((i + 1))
		fin=$(LC_ALL=C openssl x509 -in "$f" -noout -enddate 2>/dev/null | sed 's/^notAfter=//')
		s=$(date -d "$fin" +%s 2>/dev/null) || s=""
		[ -n "$s" ] && j=$(( (s - maintenant) / 86400 )) || j="?"
		[ $i = 1 ] && JOURS_FEUILLE=$j
		printf '%d|%s|%s|%s|%s\n' "$i" \
			"$(LC_ALL=C openssl x509 -in "$f" -noout -subject -nameopt RFC2253 2>/dev/null | sed 's/^subject=//' | cellule)" \
			"$(LC_ALL=C openssl x509 -in "$f" -noout -issuer -nameopt RFC2253 2>/dev/null | sed 's/^issuer=//' | cellule)" \
			"$(printf '%s' "$fin" | cellule)" "$j" >> "$CT_DIR/chaine"
	done
	{
		if [ "$i" = 0 ]; then
			printf 'Connexion : échec (%s)\n' "$(grep -m1 -oiE 'Connection refused|Connection timed out|No route to host|Network is unreachable|Name or service not known|wrong version number|unexpected eof[a-z ]*|handshake failure|no peer certificate' <<<"$sortie" || echo 'pas de certificat reçu')"
			[ "$rc" = 124 ] && printf 'Délai de %s s dépassé\n' "$d"
		else
			etat=$(sed -n 's/^ *Verify return code: \([0-9]*\) (\(.*\))$/\1 \2/p' <<<"$sortie" | tail -n 1 | cellule)
			case "$etat" in
			"0 ok") printf 'Vérification : OK (chaîne et nom%s)\n' "$( [ -z "$nom" ] && echo ' — aucun nom vérifié : adresse IP')" ;;
			"")     printf 'Vérification : résultat absent\n' ;;
			*)      printf 'Vérification : ÉCHEC — %s\n' "$etat" ;;
			esac
			[ -n "$nom" ] && printf 'Nom vérifié : %s\n' "$nom"
			case "$JOURS_FEUILLE" in
			\?) printf 'Expiration : date illisible\n' ;;
			-*) printf 'Expiration : EXPIRÉ depuis %d jour(s)\n' "$(( -JOURS_FEUILLE ))" ;;
			*)  if [ "$JOURS_FEUILLE" -le "$CT_ALERTE" ]; then printf 'Expiration : ATTENTION, dans %d jour(s)\n' "$JOURS_FEUILLE"
			    else printf 'Expiration : dans %d jour(s)\n' "$JOURS_FEUILLE"; fi ;;
			esac
			printf 'Protocole : %s\n' "$(sed -n 's/^New, \(TLSv[0-9.]*\), Cipher is \(.*\)$/\1, \2/p' <<<"$sortie" | head -n 1 | cellule)"
			printf 'Chaîne envoyée : %d certificat(s)\n' "$i"
		fi
	} > "$CT_DIR/bilan"
	printf '%s:%s — %s\n' "$h" "$p" "$(head -n 1 "$CT_DIR/bilan")" | tee "$CT_DIR/etat"
	cat "$CT_DIR/bilan"
}

dialogue_principal() {
	cat <<XML
<window title="Certificat TLS" default-width="820" default-height="520">
  <vbox spacing="6">
    <frame label="Serveur">
      <vbox>
        <hbox>
          <text><label>Hôte :</label></text>
          <entry space-expand="true" space-fill="true"><default>debian.org</default><variable>HOTE</variable></entry>
          <text><label>Port :</label></text>
          <entry width-chars="6"><default>443</default><variable>PORT</variable></entry>
          <text><label>Délai (s) :</label></text>
          <entry width-chars="3"><default>5</default><variable>DELAI</variable></entry>
        </hbox>
        <hbox>
          <text><label>Nom à vérifier :</label></text>
          <entry space-expand="true" space-fill="true"><variable>NOM</variable></entry>
          <text><label>Autorité (.pem) :</label></text>
          <entry space-expand="true" space-fill="true"><variable>AUTORITE</variable></entry>
          <button><label>Vérifier</label>
            <action>exec "\$SERMO_SCRIPT" --verifier "\$HOTE" "\$PORT" "\$NOM" "\$DELAI" "\$AUTORITE"</action>
            <action>refresh:ETAT</action><action>refresh:BILAN</action><action>refresh:CHAINE</action>
          </button>
        </hbox>
        <text><label>Nom vide : l'hôte lui-même. Autorité vide : celles du système.</label></text>
      </vbox>
    </frame>
    <text><variable>ETAT</variable><input>cat "\$CT_DIR/etat"</input></text>
    <edit editable="false"><variable>BILAN</variable><height>110</height><input file>$(xml "$CT_DIR/bilan")</input></edit>
    <tree space-expand="true" space-fill="true">
      <label>Rang|Sujet|Émetteur|Fin de validité|Jours</label>
      <variable>CHAINE</variable>
      <input>cat "\$CT_DIR/chaine"</input>
    </tree>
    <hbox><button><label>Fermer</label><action>exit:fermer</action></button></hbox>
  </vbox>
</window>
XML
}

preparer_dossier() {
	if [ -n "${CT_DIR:-}" ] && [ -d "$CT_DIR" ]; then return 0; fi
	CT_DIR=$(mktemp -d "${XDG_RUNTIME_DIR:-${TMPDIR:-/tmp}}/sermo-tls.XXXXXX") || exit 2
	export CT_DIR; CT_PROPRE=1
	: > "$CT_DIR/chaine"; : > "$CT_DIR/bilan"; echo "aucun serveur vérifié" > "$CT_DIR/etat"
}

usage() { sed -n '/^# Usage/,/^# Codes/p' "$0" | sed '$d;s/^# \{0,1\}//' >&2; exit 64; }

main() {
	umask 077
	local c
	for c in awk iconv timeout date; do
		command -v "$c" >/dev/null || { echo "$c est absent : installez coreutils, gawk ou mawk, libc-bin" >&2; exit 2; }
	done
	case "${1:-}" in
	--verifier)
		preparer_dossier
		[ "${CT_PROPRE:-}" = 1 ] && trap 'rm -rf -- "$CT_DIR"' EXIT
		shift; verifier "${1:-}" "${2:-}" "${3:-}" "${4:-5}" "${5:-}"; exit ;;
	--xml) preparer_dossier; [ "${CT_PROPRE:-}" = 1 ] && trap 'rm -rf -- "$CT_DIR"' EXIT; dialogue_principal; exit 0 ;;
	-*)    usage ;;
	esac
	[ -n "${1:-}" ] && SERMO=$1
	command -v "$SERMO" >/dev/null || { echo "sermo introuvable : $SERMO" >&2; exit 2; }
	command -v openssl >/dev/null || { echo "openssl est absent : sudo apt install openssl" >&2; exit 2; }
	preparer_dossier
	trap 'rm -rf -- "$CT_DIR"' EXIT
	trap 'exit 129' HUP; trap 'exit 130' INT; trap 'exit 143' TERM
	export DIALOGUE; DIALOGUE=$(dialogue_principal)
	"$SERMO" --program=DIALOGUE >/dev/null
	exit 0
}

[ "${BASH_SOURCE[0]}" = "$0" ] && main "$@"
