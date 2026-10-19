#!/bin/bash
# SPDX-License-Identifier: GPL-2.0-or-later
# tests/garde_dns_testeur.sh — haplo-dialog — 2026 — GPL-2.0-or-later
#
# examples/dns-testeur/ lance dig avec des valeurs tapées et affiche ce que des
# résolveurs répondent. Ce test lance TROIS petits serveurs DNS locaux (127.0.0.1,
# ports au hasard, aucune requête vers l'extérieur) et le vrai dig :
#   1 — entrées piégées (nom, type, résolveurs, délai) : 64, dig jamais lancé
#   2 — deux serveurs honnêtes (mêmes adresses, ordre inverse) : « oui » ;
#       un serveur menteur : « NON, différente » ; NXDOMAIN et serveur muet dits
#   3 — un serveur hostile (TXT avec balises, &, $(…), caractères de contrôle) :
#       rien d'actif, rien de contrôle, XML du dialogue bien formé
#   4 — dig absent : 2 ; aucun dossier temporaire ne reste
# Avec un binaire réel en argument (+ xvfb-run) : 5 — la fenêtre s'ouvre et reste ouverte.
# Usage : garde_dns_testeur.sh [/chemin/vers/un-port-a-fenetre]
RACINE=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
APP="$RACINE/examples/dns-testeur/dns-testeur.sh"
for t in dig python3 timeout mktemp; do
    command -v "$t" >/dev/null || { echo "outil manquant : $t" >&2; exit 77; }
done
[[ -x "$APP" ]] || { echo "script introuvable ou non exécutable : $APP" >&2; exit 1; }

T=$(mktemp -d); PIDS=()
trap 'for p in "${PIDS[@]}"; do kill "$p" 2>/dev/null; done; rm -rf "$T"' EXIT
export XDG_RUNTIME_DIR="$T/run"; mkdir -m 700 "$XDG_RUNTIME_DIR"
export APPELS="$T/appels"; : > "$APPELS"
VRAI_DIG=$(command -v dig)
mkdir "$T/faux"
# Le faux dig journalise ; avec FAKE_DIG=brut, il rend une sortie au format de dig
# mais avec des caractères de contrôle BRUTS (le vrai dig les échappe en \\027 :
# sans ce mode, le nettoyage du script ne serait jamais mis à l'épreuve).
printf '#!/bin/bash\nprintf "dig %%s\\n" "$*" >> "$APPELS"\n[ "${FAKE_DIG:-}" = brut ] && { cat "$FAKE_DIG_SORTIE"; exit 0; }\nexec %q "$@"\n' "$VRAI_DIG" > "$T/faux/dig"
chmod +x "$T/faux/dig"
export PATH="$T/faux:$PATH"
echec() { echo "ECHEC : $*"; exit 1; }

# Petit serveur DNS : répond à toute question A (ou TXT pour « hostile ») ; NXDOMAIN
# pour un nom qui commence par « absent ». Mode : honnete, inverse, menteur, hostile.
cat > "$T/dns.py" <<'PY'
import socket, struct, sys
mode = sys.argv[1]
s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM); s.bind(("127.0.0.1", 0))
print(s.getsockname()[1], flush=True)
ADRS = {"honnete": ["192.0.2.1", "192.0.2.2"], "inverse": ["192.0.2.2", "192.0.2.1"], "menteur": ["203.0.113.66"]}
while True:
    q, a = s.recvfrom(4096)
    ident = q[:2]; i = 12
    while q[i] != 0: i += 1 + q[i]
    question = q[12:i + 5]; nom = q[13:13 + q[12]]
    if nom.startswith(b"absent"):
        s.sendto(ident + struct.pack(">HHHHH", 0x8183, 1, 0, 0, 0) + question, a); continue
    if mode == "hostile":
        txt = b'<b>&amp;$(touch /tmp/pwn)\x1b[31m\x07`id`'
        rr = [b"\xc0\x0c" + struct.pack(">HHIH", 16, 1, 60, len(txt) + 1) + bytes([len(txt)]) + txt]
    else:
        rr = [b"\xc0\x0c" + struct.pack(">HHIH", 1, 1, 60, 4) + socket.inet_aton(x) for x in ADRS[mode]]
    s.sendto(ident + struct.pack(">HHHHH", 0x8180, 1, len(rr), 0, 0) + question + b"".join(rr), a)
PY
declare -A PORT
for m in honnete inverse menteur hostile; do
    python3 -I "$T/dns.py" $m > "$T/port-$m" & PIDS+=($!)
done
for m in honnete inverse menteur hostile; do
    for _ in $(seq 1 30); do [[ -s "$T/port-$m" ]] && break; timeout 0.1 tail -f /dev/null; done
    PORT[$m]=$(cat "$T/port-$m"); [[ "${PORT[$m]}" =~ ^[0-9]+$ ]] || echec "serveur DNS $m non démarré"
done
MUET=$(python3 -I -c 'import socket; s=socket.socket(socket.AF_INET,socket.SOCK_DGRAM); s.bind(("127.0.0.1",0)); print(s.getsockname()[1])')
export DT_DIR="$T/dt"; mkdir -m 700 "$DT_DIR"

# ── 1 — entrées piégées ─────────────────────────────────────────────────────
for n in '-x' '.debian.org' 'a;b' '$(id)' 'a b' $'a\nb' '' "$(printf 'a%.0s' $(seq 1 254))" 'a..b' '-oProxy=x'; do
    : > "$APPELS"; "$APP" --interroger "$n" A système 1 >/dev/null 2>&1; rc=$?
    [[ $rc == 64 && ! -s "$APPELS" ]] || echec "nom « ${n:0:30} » : code $rc ou dig lancé"
done
for t in ANY AXFR 'A;id' '' '-x'; do
    : > "$APPELS"; "$APP" --interroger debian.org "$t" système 1 >/dev/null 2>&1
    [[ $? == 64 && ! -s "$APPELS" ]] || echec "type « $t » accepté"
done
for r in '' '-x' '1.2.3.4;id' 'resolveur.example' '256.0.0.1' '1.2.3.4#0' '1.2.3.4#99999' '@1.2.3.4' \
         '1.1.1.1 2.2.2.2 3.3.3.3 4.4.4.4 5.5.5.5 6.6.6.6 7.7.7.7'; do
    : > "$APPELS"; "$APP" --interroger debian.org A "$r" 1 >/dev/null 2>&1
    [[ $? == 64 && ! -s "$APPELS" ]] || echec "résolveurs « $r » acceptés"
done
for d in 0 31 x; do
    : > "$APPELS"; "$APP" --interroger debian.org A système "$d" >/dev/null 2>&1
    [[ $? == 64 && ! -s "$APPELS" ]] || echec "délai « $d » accepté"
done
# Témoin : une question valide lance bien dig, avec le nom et le type en arguments séparés.
: > "$APPELS"; "$APP" --interroger essai.test A "127.0.0.1#${PORT[honnete]}" 2 >/dev/null
grep -qE -- "^dig .* @127\.0\.0\.1 -p ${PORT[honnete]} -t A -q essai\.test$" "$APPELS" || echec "témoin : arguments de dig : $(cat "$APPELS")"

# ── 2 — comparaison ─────────────────────────────────────────────────────────
"$APP" --interroger essai.test A "127.0.0.1#${PORT[honnete]} 127.0.0.1#${PORT[inverse]} 127.0.0.1#${PORT[menteur]} 127.0.0.1#$MUET" 1 > "$T/sortie" \
    || echec "interrogation : code non nul"
L() { sed -n "$1p" "$DT_DIR/resultats"; }
[[ $(L 1) == "127.0.0.1#${PORT[honnete]}|NOERROR|non|2|"*"|(référence)" ]] || echec "honnête : $(L 1)"
[[ $(L 2) == "127.0.0.1#${PORT[inverse]}|NOERROR|non|2|"*"|oui" ]] || echec "ordre inverse non reconnu comme identique : $(L 2)"
[[ $(L 3) == "127.0.0.1#${PORT[menteur]}|NOERROR|non|1|"*"|NON, différente" ]] || echec "menteur non signalé : $(L 3)"
[[ $(L 4) == "127.0.0.1#$MUET|pas de réponse|"* ]] || echec "serveur muet mal dit : $(L 4)"
grep -q '203.0.113.66' "$DT_DIR/detail" || echec "détail : réponse du menteur absente"
"$APP" --interroger absent.test A "127.0.0.1#${PORT[honnete]}" 2 >/dev/null
[[ $(L 1) == *"|NXDOMAIN|"*"|0|"* ]] || echec "NXDOMAIN mal dit : $(L 1)"

# ── 3 — serveur hostile ─────────────────────────────────────────────────────
"$APP" --interroger essai.test TXT "127.0.0.1#${PORT[hostile]}" 2 >/dev/null || echec "hostile : code non nul"
grep -qF '<b>&amp;$(touch /tmp/pwn)' "$DT_DIR/detail" || echec "hostile : le texte reçu n'est pas affiché tel quel : $(cat "$DT_DIR/detail")"
! LC_ALL=C grep -q $'[\x01-\x08\x1b]' "$DT_DIR/detail" "$DT_DIR/resultats" || echec "hostile : caractères de contrôle restés"
[[ $(awk -F'|' '{print NF}' "$DT_DIR/resultats") == 6 ]] || echec "hostile : le tableau n'a plus 6 colonnes"
printf ';; ->>HEADER<<- opcode: QUERY, status: NOERROR, id: 1\n;; flags: qr rd ra; QUERY: 1, ANSWER: 1\nessai.test.\t60\tIN\tTXT\t"\033[31mrouge\x07\x01<i>"\n;; Query time: 3 msec\n' > "$T/brut"
FAKE_DIG=brut FAKE_DIG_SORTIE="$T/brut" "$APP" --interroger essai.test TXT système 1 >/dev/null || echec "sortie brute : code non nul"
grep -qF '[31mrouge' "$DT_DIR/detail" || echec "sortie brute : réponse absente du détail"
! LC_ALL=C grep -q $'[\x01-\x08\x1b]' "$DT_DIR/detail" || echec "sortie brute : caractères de contrôle restés dans le détail"
"$APP" --xml > "$T/xml" || echec "--xml : code non nul"
sed 's/<input file>/<input file="1">/g' "$T/xml" | python3 -I -c 'import sys,xml.dom.minidom as m; m.parseString(sys.stdin.read())' || echec "XML mal formé"

# ── 4 — codes, ménage ───────────────────────────────────────────────────────
mkdir "$T/maigre"
for c in bash awk iconv md5sum timeout cat mktemp rm sed tee head tr cut sort wc; do ln -s "$(PATH=${PATH#*:} command -v $c)" "$T/maigre/$c"; done
env PATH="$T/maigre" "$APP" --interroger debian.org A système 1 >/dev/null 2>&1; [[ $? == 2 ]] || echec "dig absent : code ≠ 2"
( unset DT_DIR; "$APP" --xml >/dev/null; "$APP" --interroger debian.org ANY système 1 >/dev/null 2>&1 )
[[ -z "$(find "$XDG_RUNTIME_DIR" -name 'sermo-dns.*')" ]] || echec "un dossier temporaire survit"

# ── 5 — binaire réel (facultatif) ──────────────────────────────────────────
if [[ -n "${1:-}" ]]; then
    BIN="$1"
    [[ -x "$BIN" ]] && command -v xvfb-run >/dev/null || { echo "binaire ou xvfb-run absent : étape 5 non faite" >&2; exit 77; }
    ( unset DT_DIR; timeout -k 3 10 xvfb-run -a "$APP" "$BIN" ) > "$T/gui" 2>&1
    rc=$?
    [[ $rc == 124 ]] || echec "binaire réel : la fenêtre ne reste pas ouverte (rc=$rc) : $(tail -n 3 "$T/gui")"
    ! grep -qiE 'syntax error|parse error|unknown tag|erreur de syntaxe' "$T/gui" || echec "binaire réel : XML refusé"
    suite=" ; fenêtre réelle ouverte"
fi
echo "OK : entrées piégées refusées sans dig, menteur et muet signalés, ordre ignoré, réponse hostile neutralisée${suite:-}"
