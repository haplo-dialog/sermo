#!/bin/bash
# SPDX-License-Identifier: GPL-2.0-or-later
# tests/garde_certificat_tls.sh — haplo-dialog — 2026 — GPL-2.0-or-later
#
# examples/certificat-tls/ lance openssl avec des valeurs tapées et affiche ce
# qu'envoie un serveur. Ce test fabrique une autorité d'essai et des serveurs
# openssl s_server locaux (127.0.0.1, ports au hasard, rien vers l'extérieur) :
#   1 — entrées piégées : 64, openssl jamais lancé
#   2 — bilans : valide (OK, jours, chaîne de 2), expiré, expiration proche
#       (ATTENTION), mauvais nom, autosigné, serveur muet, adresse IP sans nom
#   3 — sujet piégé (balises, &, |, caractères de contrôle) : tableau à 5 colonnes,
#       rien de contrôle ; XML du dialogue bien formé
#   4 — openssl absent : 2 ; aucun dossier temporaire ne reste
# Avec un binaire réel en argument (+ xvfb-run) : 5 — la fenêtre s'ouvre et reste ouverte.
# Usage : garde_certificat_tls.sh [/chemin/vers/un-port-a-fenetre]
RACINE=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
APP="$RACINE/examples/certificat-tls/certificat-tls.sh"
BIN_REEL="${1:-}"      # gardé : plus bas, « set -- » réécrit les arguments
for t in openssl python3 timeout mktemp; do
    command -v "$t" >/dev/null || { echo "outil manquant : $t" >&2; exit 77; }
done
[[ -x "$APP" ]] || { echo "script introuvable ou non exécutable : $APP" >&2; exit 1; }

T=$(mktemp -d); PIDS=()
trap 'for p in "${PIDS[@]}"; do kill "$p" 2>/dev/null; done; rm -rf "$T"' EXIT
export XDG_RUNTIME_DIR="$T/run"; mkdir -m 700 "$XDG_RUNTIME_DIR"
export APPELS="$T/appels"; : > "$APPELS"
VRAI_OPENSSL=$(command -v openssl)
mkdir "$T/faux" "$T/pki"
printf '#!/bin/bash\nprintf "openssl %%s\\n" "$*" >> "$APPELS"\nexec %q "$@"\n' "$VRAI_OPENSSL" > "$T/faux/openssl"
chmod +x "$T/faux/openssl"
echec() { echo "ECHEC : $*"; exit 1; }

# ── PKI d'essai ─────────────────────────────────────────────────────────────
K="$T/pki"; O="$VRAI_OPENSSL"
$O req -x509 -newkey rsa:2048 -nodes -days 30 -subj "/CN=Autorite essai" -keyout "$K/ac.cle" -out "$K/ac.pem" 2>/dev/null || exit 2
feuille() {  # feuille <nom> <sujet> <SAN> <options de durée…>
    local n=$1 sujet=$2 san=$3; shift 3
    $O req -newkey rsa:2048 -nodes -subj "$sujet" -keyout "$K/$n.cle" -out "$K/$n.csr" 2>/dev/null || exit 2
    printf 'subjectAltName=%s\n' "$san" > "$K/$n.ext"
    $O x509 -req -in "$K/$n.csr" -CA "$K/ac.pem" -CAkey "$K/ac.cle" -CAcreateserial "$@" -extfile "$K/$n.ext" -out "$K/$n.crt" 2>/dev/null || exit 2
    cat "$K/$n.crt" "$K/ac.pem" > "$K/$n.chaine"
}
feuille valide "/CN=essai.test" "DNS:essai.test,IP:127.0.0.1" -days 200
feuille expire "/CN=essai.test" "DNS:essai.test" -not_before 20200101000000Z -not_after 20200201000000Z
feuille proche "/CN=essai.test" "DNS:essai.test" -days 10
feuille piege "/CN=<b>&amp; a|b\$(id)" "DNS:essai.test" -days 200
$O req -x509 -newkey rsa:2048 -nodes -days 200 -subj "/CN=essai.test" -addext "subjectAltName=DNS:essai.test" \
    -keyout "$K/auto.cle" -out "$K/auto.crt" 2>/dev/null && cp "$K/auto.crt" "$K/auto.chaine" || exit 2
declare -A PORT
for n in valide expire proche piege auto; do
    PORT[$n]=$(python3 -I -c 'import socket; s=socket.socket(); s.bind(("127.0.0.1",0)); print(s.getsockname()[1])')
    $O s_server -accept "127.0.0.1:${PORT[$n]}" -cert "$K/$n.crt" $([ $n = auto ] || echo -cert_chain "$K/ac.pem") -key "$K/$n.cle" -quiet </dev/null >/dev/null 2>&1 & PIDS+=($!)
done
for n in valide expire proche piege auto; do
    for _ in $(seq 1 50); do (exec 3<>"/dev/tcp/127.0.0.1/${PORT[$n]}") 2>/dev/null && break; timeout 0.1 tail -f /dev/null; done
done
MUET=$(python3 -I -c 'import socket; s=socket.socket(); s.bind(("127.0.0.1",0)); print(s.getsockname()[1])')
export PATH="$T/faux:$PATH" CT_DIR="$T/ct"; mkdir -m 700 "$CT_DIR"
V() { "$APP" --verifier 127.0.0.1 "${PORT[$1]}" "$2" 3 "${3-$K/ac.pem}"; }

# ── 1 — entrées piégées ─────────────────────────────────────────────────────
for args in "-x 443" "'a;b' 443" "'\$(id)' 443" "'' 443" "h.test 0" "h.test 65536" "h.test x" \
            "h.test 443 -oX" "h.test 443 'a b'" "h.test 443 '' 0" "h.test 443 '' 31" \
            "h.test 443 '' 3 relatif.pem" "h.test 443 '' 3 /nexiste.pem" "h.test 443 '' 3 /tmp" "256.1.1.1 443"; do
    eval "set -- $args"; : > "$APPELS"
    "$APP" --verifier "$@" >/dev/null 2>&1; rc=$?
    [[ $rc == 64 && ! -s "$APPELS" ]] || echec "entrée « $args » : code $rc ou openssl lancé"
done
CT_ALERTE=abc "$APP" --verifier h.test 443 >/dev/null 2>&1; [[ $? == 64 ]] || echec "CT_ALERTE invalide accepté"
# Témoin : une entrée valide lance bien openssl, arguments séparés.
: > "$APPELS"; V valide essai.test >/dev/null
grep -qF -- "openssl s_client -connect 127.0.0.1:${PORT[valide]} -showcerts -servername essai.test -verify_hostname essai.test -CAfile $K/ac.pem" "$APPELS" \
    || echec "témoin : arguments d'openssl : $(head -n 1 "$APPELS")"

# ── 2 — bilans ──────────────────────────────────────────────────────────────
b() { grep -m1 "^$1" "$CT_DIR/bilan"; }
V valide essai.test >/dev/null
[[ $(b Vérification) == "Vérification : OK (chaîne et nom)" ]] || echec "valide : $(b Vérification)"
[[ $(b Expiration) =~ ^"Expiration : dans "(199|200)" jour(s)"$ ]] || echec "valide : $(b Expiration)"
[[ $(b Chaîne) == "Chaîne envoyée : 2 certificat(s)" ]] || echec "valide : $(b Chaîne)"
[[ $(b Protocole) =~ ^"Protocole : TLSv1."[23]", " ]] || echec "valide : $(b Protocole)"
[[ $(sed -n 1p "$CT_DIR/chaine") == "1|CN=essai.test|CN=Autorite essai|"*"|"* ]] || echec "chaîne, rang 1 : $(sed -n 1p "$CT_DIR/chaine")"
V expire essai.test >/dev/null
[[ $(b Vérification) == *"certificate has expired"* ]] || echec "expiré : $(b Vérification)"
[[ $(b Expiration) =~ ^"Expiration : EXPIRÉ depuis "[0-9]+" jour(s)"$ ]] || echec "expiré : $(b Expiration)"
V proche essai.test >/dev/null
[[ $(b Vérification) == "Vérification : OK (chaîne et nom)" ]] || echec "proche : $(b Vérification)"
[[ $(b Expiration) =~ ^"Expiration : ATTENTION, dans "(9|10)" jour(s)"$ ]] || echec "proche : alerte absente : $(b Expiration)"
CT_ALERTE=5 V proche essai.test >/dev/null
[[ $(b Expiration) != *ATTENTION* ]] || echec "CT_ALERTE=5 : alerte à 10 jours quand même"
V valide autre.test >/dev/null
[[ $(b Vérification) == *"hostname mismatch"* ]] || echec "mauvais nom non signalé : $(b Vérification)"
V auto essai.test >/dev/null
[[ $(b Vérification) == *"self-signed certificate"* ]] || echec "autosigné non signalé : $(b Vérification)"
V valide essai.test "" >/dev/null
[[ $(b Vérification) == *"ÉCHEC"* ]] || echec "sans l'autorité d'essai, la chaîne passe quand même : $(b Vérification)"
"$APP" --verifier 127.0.0.1 "${PORT[valide]}" "" 3 "$K/ac.pem" >/dev/null
[[ $(b Vérification) == *"aucun nom vérifié"* ]] || echec "IP sans nom : la limite n'est pas dite : $(b Vérification)"
"$APP" --verifier 127.0.0.1 "$MUET" "" 2 >/dev/null
[[ $(b Connexion) == "Connexion : échec (Connection refused)" ]] || echec "serveur muet : $(cat "$CT_DIR/bilan")"
[[ ! -s "$CT_DIR/chaine" ]] || echec "serveur muet : une chaîne reste affichée"

# ── 3 — sujet piégé ─────────────────────────────────────────────────────────
V piege essai.test >/dev/null
[[ $(awk -F'|' '{print NF}' "$CT_DIR/chaine" | sort -u) == 5 ]] || echec "sujet piégé : le tableau n'a plus 5 colonnes : $(cat "$CT_DIR/chaine")"
grep -qF 'CN=\<b\>&amp\; a¦b$(id)' "$CT_DIR/chaine" || echec "sujet piégé non affiché tel quel : $(sed -n 1p "$CT_DIR/chaine")"
! LC_ALL=C grep -q $'[\x01-\x08\x1b]' "$CT_DIR/chaine" "$CT_DIR/bilan" || echec "caractères de contrôle restés"
"$APP" --xml > "$T/xml" || echec "--xml : code non nul"
sed 's/<input file>/<input file="1">/g' "$T/xml" | python3 -I -c 'import sys,xml.dom.minidom as m; m.parseString(sys.stdin.read())' || echec "XML mal formé"

# ── 4 — codes, ménage ───────────────────────────────────────────────────────
mkdir "$T/maigre"
for c in bash awk iconv timeout date cat mktemp rm sed tee head tr cut grep; do ln -s "$(PATH=${PATH#*:} command -v $c)" "$T/maigre/$c"; done
env PATH="$T/maigre" "$APP" --verifier h.test 443 >/dev/null 2>&1; [[ $? == 2 ]] || echec "openssl absent : code ≠ 2"
( unset CT_DIR; "$APP" --xml >/dev/null; "$APP" --verifier -x 443 >/dev/null 2>&1 )
[[ -z "$(find "$XDG_RUNTIME_DIR" -name 'sermo-tls.*')" ]] || echec "un dossier temporaire survit"

# ── 5 — binaire réel (facultatif) ──────────────────────────────────────────
if [[ -n "$BIN_REEL" ]]; then
    BIN="$BIN_REEL"
    [[ -x "$BIN" ]] && command -v xvfb-run >/dev/null || { echo "binaire ou xvfb-run absent : étape 5 non faite" >&2; exit 77; }
    ( unset CT_DIR; timeout -k 3 10 xvfb-run -a "$APP" "$BIN" ) > "$T/gui" 2>&1
    rc=$?
    [[ $rc == 124 ]] || echec "binaire réel : la fenêtre ne reste pas ouverte (rc=$rc) : $(tail -n 3 "$T/gui")"
    ! grep -qiE 'syntax error|parse error|unknown tag|erreur de syntaxe' "$T/gui" || echec "binaire réel : XML refusé"
    suite=" ; fenêtre réelle ouverte"
fi
echo "OK : entrées piégées refusées sans openssl, bilans justes (valide, expiré, proche, mauvais nom, autosigné, muet, IP), sujet piégé neutralisé${suite:-}"
