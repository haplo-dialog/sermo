#!/bin/bash
# SPDX-License-Identifier: GPL-2.0-or-later
# tests/garde_reseau_testeur.sh — haplo-dialog — 2026 — GPL-2.0-or-later
#
# examples/reseau-testeur/ lance des outils réseau avec des valeurs tapées dans
# une fenêtre et affiche des réponses venues d'ailleurs. Ce test vérifie :
#   1 — entrées piégées (hôte, port, délai, trame, URL) : code 64, AUCUN outil lancé
#   2 — les quatre états du test de port + nom inconnu (faux timeout) ;
#       ouvert / fermé rejoués pour de vrai sur 127.0.0.1
#   3 — trame réelle vers un faux serveur local : aller-retour, réponse de 1 Mo
#       plafonnée et marquée « tronqué », caractères de contrôle retirés
#   4 — HTTP (faux curl) : arguments exacts, protocoles limités, réponse piégée
#   5 — sockets (faux ss) : 4 colonnes par ligne même avec un nom de processus piégé
#   4b — UDP réel (écho local) : réponse lue, refus ICMP = fermé, silence = « ne permet
#        pas de trancher » ; ping réel sur 127.0.0.1, sortie piégée nettoyée (faux ping)
#   6 — XML du dialogue bien formé ; outil absent : code 2 ; rien ne reste après
# Avec un binaire réel en argument (+ xvfb-run) : 7 — la fenêtre s'ouvre et reste ouverte.
# Usage : garde_reseau_testeur.sh [/chemin/vers/un-port-a-fenetre]
RACINE=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
APP="$RACINE/examples/reseau-testeur/reseau-testeur.sh"
for t in python3 timeout mktemp iconv awk; do
    command -v "$t" >/dev/null || { echo "outil manquant : $t" >&2; exit 77; }
done
[[ -x "$APP" ]] || { echo "script introuvable ou non exécutable : $APP" >&2; exit 1; }

SRV=""; SRV_UDP=""
T=$(mktemp -d); trap 'kill $SRV $SRV_UDP 2>/dev/null; rm -rf "$T"' EXIT
export XDG_RUNTIME_DIR="$T/run"; mkdir -m 700 "$XDG_RUNTIME_DIR"
export APPELS="$T/appels"; : > "$APPELS"
VRAI_TIMEOUT=$(command -v timeout)
mkdir "$T/faux"

# Faux timeout : journalise ; simule un état de connexion si FAKE_CONNECT est posé.
cat > "$T/faux/timeout" <<FAUX
#!/bin/bash
printf 'timeout %s\n' "\$*" >> "\$APPELS"
case "\${FAKE_CONNECT:-}" in
  refuse) echo "_: connect: Connection refused" >&2; exit 1 ;;
  route)  echo "_: connect: No route to host" >&2; exit 1 ;;
  delai)  exit 124 ;;
  nom)    echo "_: line 1: x.invalid: Name or service not known" >&2; exit 1 ;;
esac
exec "$VRAI_TIMEOUT" "\$@"
FAUX
# Faux curl : journalise ses arguments un par ligne ; FAKE_CURL choisit la réponse.
cat > "$T/faux/curl" <<'FAUX'
#!/bin/bash
{ echo "curl"; printf '  [%s]\n' "$@"; } >> "$APPELS"
case "${FAKE_CURL:-}" in
  gros)  printf 'HTTP/1.1 200 OK\r\n\r\n'; head -c 1048576 /dev/zero | tr '\0' 'A' ;;
  piege) printf 'HTTP/1.1 200 OK\r\nX: </edit><button>&amp;\r\n\r\n\033[31m\x01\x02rouge\xff\xfe\n' ;;
  *)     printf 'HTTP/1.1 204 No Content\r\n\r\n' ;;
esac
FAUX
# Faux ss : sortie de ss -tanpH, dont un nom de processus piégé (| et balises).
cat > "$T/faux/ss" <<'FAUX'
#!/bin/bash
printf 'ss %s\n' "$*" >> "$APPELS"
cat <<'SS'
LISTEN 0      4096      127.0.0.1:631        0.0.0.0:*     users:(("cupsd",pid=812,fd=7))
ESTAB  0      0         192.168.1.5:41234    93.184.216.34:443 users:(("a|b<x>&",pid=99,fd=3))
TIME-WAIT 0   0         127.0.0.1:44718      127.0.0.1:631
SS
FAUX
# Faux ping : journalise ; FAKE_PING=piege rend une sortie hostile, sinon le vrai ping.
VRAI_PING=$(command -v ping)
cat > "$T/faux/ping" <<FAUX
#!/bin/bash
printf 'ping %s\n' "\$*" >> "\$APPELS"
case "\${FAKE_PING:-}" in
  piege) printf '\033[31m<b>&amp; \x01 ping: rien\n' >&2; exit 2 ;;
esac
exec "$VRAI_PING" "\$@"
FAUX
chmod +x "$T/faux/"*
export PATH="$T/faux:$PATH"

echec() { echo "ECHEC : $*"; exit 1; }
rien_lance() { [[ ! -s "$APPELS" ]] || echec "$1 : un outil a été lancé : $(head -n 2 "$APPELS")"; }

# ── 1 — entrées piégées : 64 et rien de lancé ──────────────────────────────
for h in '-oProxyCommand=x' 'a;b' '$(touch '"$T"'/pwn)' '../etc' $'a\nb' '' ' ' 'a b' '`id`' 'x..y' '256.1.1.1'; do
    : > "$APPELS"
    "$APP" --tester "$h" 80 2 >/dev/null 2>&1; rc=$?
    [[ $rc == 64 ]] || echec "hôte « ${h//$'\n'/\\n} » : code $rc au lieu de 64"
    rien_lance "hôte « ${h//$'\n'/\\n} »"
done
for p in 0 65536 080 -1 '1 2' abc ''; do
    : > "$APPELS"; "$APP" --tester 127.0.0.1 "$p" 2 >/dev/null 2>&1
    [[ $? == 64 ]] || echec "port « $p » accepté"; rien_lance "port « $p »"
done
for d in 0 61 -3 1.5 x; do
    : > "$APPELS"; "$APP" --tester 127.0.0.1 80 "$d" >/dev/null 2>&1
    [[ $? == 64 ]] || echec "délai « $d » accepté"; rien_lance "délai « $d »"
done
for t in '' $'a\nb' $'a\rb' "$(head -c 5000 /dev/zero | tr '\0' x)"; do
    : > "$APPELS"; "$APP" --trame 127.0.0.1 80 2 "$t" >/dev/null 2>&1
    [[ $? == 64 ]] || echec "trame piégée acceptée"; rien_lance "trame piégée"
done
for u in 'file:///etc/passwd' 'gopher://x/' 'http://u:p@hote/' 'http://-K/x' 'http://a b/' $'http://a\n/' \
         'ftp://x/' 'http://' "http://$(head -c 2100 /dev/zero | tr '\0' a)" '-K/etc/passwd'; do
    : > "$APPELS"; "$APP" --http "$u" 2 >/dev/null 2>&1
    [[ $? == 64 ]] || echec "URL « ${u:0:40} » acceptée"; rien_lance "URL « ${u:0:40} »"
done
for h in '-f' 'a;b' '$(touch '"$T"'/pwn)' $'a\nb'; do
    : > "$APPELS"; "$APP" --ping "$h" 1 >/dev/null 2>&1
    [[ $? == 64 ]] || echec "ping : hôte « ${h//$'\n'/\\n} » accepté"; rien_lance "ping « $h »"
    : > "$APPELS"; "$APP" --udp "$h" 53 1 x >/dev/null 2>&1
    [[ $? == 64 ]] || echec "udp : hôte « ${h//$'\n'/\\n} » accepté"; rien_lance "udp « $h »"
done
for t in '' $'a\nb'; do
    : > "$APPELS"; "$APP" --udp 127.0.0.1 53 1 "$t" >/dev/null 2>&1
    [[ $? == 64 ]] || echec "udp : datagramme piégé accepté"; rien_lance "udp : datagramme piégé"
done
: > "$APPELS"; "$APP" --udp 127.0.0.1 0 1 x >/dev/null 2>&1; [[ $? == 64 ]] || echec "udp : port 0 accepté"; rien_lance "udp : port 0"
[[ ! -e "$T/pwn" ]] || echec "une commande contenue dans l'hôte a été exécutée"
# Témoin : une entrée valide, elle, lance bien un outil (sinon les vérifications ci-dessus ne prouvent rien).
: > "$APPELS"; "$APP" --tester 127.0.0.1 9 1 >/dev/null 2>&1
[[ -s "$APPELS" ]] || echec "témoin : une entrée valide ne lance rien, le banc est aveugle"

# ── 2 — états du test de port ──────────────────────────────────────────────
etat() { FAKE_CONNECT="$1" "$APP" --tester "${2:-127.0.0.1}" 9 2 2>/dev/null; }
[[ $(etat refuse) == *"→ fermé"* ]] || echec "refus de connexion ≠ fermé"
[[ $(etat route)  == *"→ filtré"* ]] || echec "pas de route ≠ filtré"
[[ $(etat delai)  == *"→ délai"* ]]  || echec "délai écoulé ≠ délai"
[[ $(etat nom x.invalid) == *"→ nom inconnu"* ]] || echec "nom non résolu ≠ nom inconnu"
[[ $(etat refuse ::1) == "[::1]:9 → fermé"* ]] || echec "IPv6 mal affiché"

# Un vrai serveur local : écoute, accepte, répond 1 Mo plus des caractères de contrôle, ferme.
cat > "$T/serveur.py" <<'PY'
import socket, sys
s = socket.socket(); s.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
s.bind(("127.0.0.1", 0)); s.listen(4)
print(s.getsockname()[1], flush=True)
while True:
    c, _ = s.accept()
    try:
        ligne = c.recv(8192)
        if ligne:                      # le test de port ouvre puis ferme sans rien dire
            open(sys.argv[1], "wb").write(ligne)
            c.sendall(b"ECHO " + ligne.strip() + b"\x00\x07\x1b[31m\n" + b"B" * 1048576)
    except OSError:
        pass                           # le client coupe après 64 Kio : attendu
    c.close()
PY
python3 -I "$T/serveur.py" "$T/recu" > "$T/port" & SRV=$!
for _ in 1 2 3 4 5 6 7 8 9 10; do [[ -s "$T/port" ]] && break; "$VRAI_TIMEOUT" 0.3 tail -f /dev/null; done
PORT=$(cat "$T/port"); [[ "$PORT" =~ ^[0-9]+$ ]] || echec "serveur de test non démarré"
[[ $("$APP" --tester 127.0.0.1 "$PORT" 2) == *"→ ouvert"* ]] || echec "port réellement ouvert ≠ ouvert"
# Port fermé réel : on prend un port libre (lié puis relâché).
LIBRE=$(python3 -I -c 'import socket; s=socket.socket(); s.bind(("127.0.0.1",0)); print(s.getsockname()[1])')
[[ $("$APP" --tester 127.0.0.1 "$LIBRE" 2) == *"→ fermé"* ]] || echec "port réellement fermé ≠ fermé"

# ── 3 — trame réelle ───────────────────────────────────────────────────────
export RT_DIR="$T/rt"; mkdir -m 700 "$RT_DIR"; : > "$RT_DIR/journal"
TRAME='bonjour $(touch '"$T"'/pwn3) </edit>&'
"$APP" --trame 127.0.0.1 "$PORT" 5 "$TRAME" > "$T/sortie" 2>&1 || echec "trame : code non nul"
[[ $(cat "$T/recu") == "$TRAME"$'\r' ]] || echec "trame reçue par le serveur ≠ trame tapée"
[[ ! -e "$T/pwn3" ]] || echec "la trame a été exécutée"
head -n 1 "$RT_DIR/reponse" | grep -qF "ECHO $TRAME" || echec "réponse du serveur absente"
corps() { echo $(( $(wc -c < "$1") - $(tail -n 1 "$1" | wc -c) - 1 )); }   # sans la ligne « tronqué »
taille=$(corps "$RT_DIR/reponse")
(( taille > 60000 && taille <= 65536 )) || echec "réponse de 1 Mo non plafonnée à 64 Kio ($taille octets)"
grep -q 'tronqué' "$RT_DIR/reponse" || echec "troncature non signalée"
! LC_ALL=C grep -q $'[\x01-\x08\x1b\x7f]' "$RT_DIR/reponse" || echec "caractères de contrôle restés dans la réponse"
grep -q "trame 127.0.0.1:$PORT" "$RT_DIR/journal" || echec "trame absente du journal"
unset RT_DIR

# ── 4 — HTTP ───────────────────────────────────────────────────────────────
export RT_DIR="$T/rt"
: > "$APPELS"
[[ $("$APP" --http 'http://127.0.0.1:8080/a?b=c' 7) == "HTTP : HTTP/1.1 204"* ]] || echec "HTTP : statut non affiché"
diff <(sed -n '2,$p' "$APPELS") <(printf '  [%s]\n' -sS -i -m 7 --proto =http,https --proto-redir =http,https -- 'http://127.0.0.1:8080/a?b=c') >/dev/null \
    || echec "HTTP : arguments de curl inattendus : $(tr '\n' ' ' < "$APPELS")"
FAKE_CURL=gros "$APP" --http http://127.0.0.1/ 3 >/dev/null 2>&1
taille=$(corps "$RT_DIR/reponse"); (( taille <= 65536 )) || echec "HTTP : 1 Mo non plafonné à 64 Kio ($taille)"
grep -q 'tronqué' "$RT_DIR/reponse" || echec "HTTP : troncature non signalée"
FAKE_CURL=piege "$APP" --http https://[::1]:8443/ 3 >/dev/null 2>&1 || echec "HTTP IPv6 entre crochets refusé"
iconv -f UTF-8 -t UTF-8 "$RT_DIR/reponse" >/dev/null 2>&1 || echec "HTTP : réponse non UTF-8 laissée telle quelle"
! LC_ALL=C grep -q $'[\x01\x02\x1b]' "$RT_DIR/reponse" || echec "HTTP : caractères de contrôle restés"
unset RT_DIR

# ── 4b — UDP et ping ───────────────────────────────────────────────────────
export RT_DIR="$T/rt"
cat > "$T/echo-udp.py" <<'PY2'
import socket
s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM); s.bind(("127.0.0.1", 0))
print(s.getsockname()[1], flush=True)
while True:
    d, a = s.recvfrom(9000)
    s.sendto(b"ECHO " + d.strip() + b" \x1b[31m\x07<b>&", a)
PY2
python3 -I "$T/echo-udp.py" > "$T/port-udp" & SRV_UDP=$!
for _ in 1 2 3 4 5 6 7 8 9 10; do [[ -s "$T/port-udp" ]] && break; "$VRAI_TIMEOUT" 0.3 tail -f /dev/null; done
PU=$(cat "$T/port-udp"); [[ "$PU" =~ ^[0-9]+$ ]] || echec "serveur UDP de test non démarré"
[[ $("$APP" --udp 127.0.0.1 "$PU" 2 'bonjour $(id)') == "UDP 127.0.0.1:$PU → réponse reçue"* ]] || echec "UDP : écho non reçu : $(cat "$RT_DIR/resultat")"
head -n 1 "$RT_DIR/reponse" | grep -qF 'ECHO bonjour $(id) [31m<b>&' || echec "UDP : réponse mal rendue : $(od -c "$RT_DIR/reponse" | head -2)"
! LC_ALL=C grep -q $'[\x01-\x08\x1b]' "$RT_DIR/reponse" || echec "UDP : caractères de contrôle restés"
kill $SRV_UDP 2>/dev/null
LIBRE_U=$(python3 -I -c 'import socket; s=socket.socket(socket.AF_INET,socket.SOCK_DGRAM); s.bind(("127.0.0.1",0)); print(s.getsockname()[1])')
[[ $("$APP" --udp 127.0.0.1 "$LIBRE_U" 2 x) == *"→ fermé"* ]] || echec "UDP : port fermé (ICMP) ≠ fermé : $(cat "$RT_DIR/resultat")"
[[ $(FAKE_CONNECT=delai "$APP" --udp 127.0.0.1 "$LIBRE_U" 2 x) == *"ne permet pas de trancher"* ]] || echec "UDP : silence présenté comme un verdict"
if "$VRAI_TIMEOUT" 5 ping -c 1 -W 1 127.0.0.1 >/dev/null 2>&1; then
    [[ $("$APP" --ping 127.0.0.1 2) =~ ^"ping 127.0.0.1 : 4 packets transmitted, 4 received, 0% packet loss ; temps min/moy/max "[0-9.]+/[0-9.]+/[0-9.]+" ms"$ ]] \
        || echec "ping réel : $(cat "$RT_DIR/resultat")"
else
    echo "  note : ping refusé sans droits sur ce poste (net.ipv4.ping_group_range) : ping réel non essayé"
fi
[[ $(FAKE_PING=piege "$APP" --ping 127.0.0.1 1) == "ping 127.0.0.1 : échec ("* ]] || echec "ping : échec mal dit"
! LC_ALL=C grep -q $'[\x01-\x08\x1b]' "$RT_DIR/reponse" "$RT_DIR/resultat" || echec "ping : caractères de contrôle restés"
unset RT_DIR

# ── 5 — sockets ────────────────────────────────────────────────────────────
"$APP" --sockets > "$T/sk" || echec "sockets : code non nul"
[[ $(wc -l < "$T/sk") == 3 ]] || echec "sockets : nombre de lignes"
awk -F'|' 'NF != 4 { exit 1 }' "$T/sk" || echec "sockets : une ligne n'a pas 4 colonnes (nom piégé) : $(cat "$T/sk")"
grep -q '^LISTEN|127.0.0.1:631|0.0.0.0:\*|cupsd$' "$T/sk" || echec "sockets : ligne cupsd mal lue"
[[ $("$APP" --sockets cups | wc -l) == 1 ]] || echec "sockets : filtre"
[[ $("$APP" --sockets -v | wc -l) == 0 ]] || echec "sockets : un filtre commençant par - est pris pour une option"

# ── 6 — XML, outil absent, rien qui reste ──────────────────────────────────
"$APP" --xml > "$T/xml" || echec "--xml : code non nul"
sed 's/<input file>/<input file="1">/g' "$T/xml" > "$T/xml-strict"
python3 -I -c 'import sys,xml.dom.minidom as m; d=m.parse(sys.argv[1]); assert d.getElementsByTagName("tree")' "$T/xml-strict" \
    || echec "XML du dialogue mal formé"
mkdir "$T/maigre"
for c in bash timeout head iconv awk tee wc tr grep sed date mktemp cat rm mv tail dirname; do
    ln -s "$(PATH=${PATH#*:} command -v $c)" "$T/maigre/$c"
done
env PATH="$T/maigre" "$APP" --http http://127.0.0.1/ 2 >/dev/null 2>&1; [[ $? == 2 ]] || echec "curl absent : code ≠ 2"
env PATH="$T/maigre" "$APP" >/dev/null 2>&1; [[ $? == 2 ]] || echec "ss absent : code ≠ 2"
ln -s "$(PATH=${PATH#*:} command -v dd)" "$T/maigre/dd"
env PATH="$T/maigre" "$APP" --ping 127.0.0.1 1 >/dev/null 2>&1; [[ $? == 2 ]] || echec "ping absent : code ≠ 2"
rm -f "$T/maigre/iconv"
env PATH="$T/maigre" "$APP" --tester 127.0.0.1 9 1 >/dev/null 2>&1; [[ $? == 2 ]] || echec "iconv absent : code ≠ 2"
restes=$(find "$XDG_RUNTIME_DIR" -name 'sermo-reseau.*' | head -n 1)
[[ -z "$restes" ]] || echec "un dossier temporaire survit : $restes"

# ── 7 — binaire réel (facultatif) ─────────────────────────────────────────
if [[ -n "${1:-}" ]]; then
    BIN="$1"
    [[ -x "$BIN" ]] && command -v xvfb-run >/dev/null || { echo "binaire ou xvfb-run absent : étape 7 non faite" >&2; exit 77; }
    PATH=${PATH#*:} "$VRAI_TIMEOUT" -k 3 10 xvfb-run -a "$APP" "$BIN" > "$T/gui" 2>&1
    rc=$?
    [[ $rc == 124 ]] || echec "binaire réel : la fenêtre ne reste pas ouverte (rc=$rc) : $(tail -n 3 "$T/gui")"
    ! grep -qiE 'erreur de syntaxe|syntax error|parse error|unknown tag' "$T/gui" || echec "binaire réel : XML refusé : $(grep -iE 'syntax|parse|unknown' "$T/gui" | head -2)"
    suite=" ; fenêtre réelle ouverte"
fi
echo "OK : entrées piégées refusées sans rien lancer, 4 états du port, trames TCP/UDP et HTTP plafonnées, ping, sockets en 4 colonnes${suite:-}"
