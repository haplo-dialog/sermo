#!/bin/bash
# SPDX-License-Identifier: GPL-2.0-or-later
# tests/garde_liaison_serie.sh — haplo-dialog — 2026 — GPL-2.0-or-later
#
# examples/liaison-serie/ écoute un port série et n'y écrit qu'après approbation.
# Ce test utilise une VRAIE paire de ports virtuels socat (aucun matériel) :
#   1 — entrées piégées (périphérique, débit, format, fin, trame) : 64, socat jamais lancé
#   2 — lecture réelle : ce qui entre d'un côté s'affiche, en texte nettoyé et en hexadécimal
#   3 — envoi : « Autoriser » → les octets exacts arrivent ; refus ou porte en panne → RIEN
#       n'arrive ; la fenêtre d'approbation montre la trame octet par octet
#   4 — aucune écoute ne survit : --fermer, puis maître tué par SIGKILL
#   5 — journal plafonné (1 Mio) avec message ; XML bien formé ; codes 2
# Avec un binaire réel en argument (+ xvfb-run) : 6 — la fenêtre s'ouvre et reste ouverte.
# Usage : garde_liaison_serie.sh [/chemin/vers/un-port-a-fenetre]
RACINE=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
APP="$RACINE/examples/liaison-serie/liaison-serie.sh"
BIN_REEL="${1:-}"      # gardé : plus bas, « set -- » réécrit les arguments
for t in socat python3 timeout mktemp od; do
    command -v "$t" >/dev/null || { echo "outil manquant : $t" >&2; exit 77; }
done
[[ -x "$APP" ]] || { echo "script introuvable ou non exécutable : $APP" >&2; exit 1; }

T=$(mktemp -d)
nettoyer() { [[ -n "${PAIRE:-}" ]] && kill "$PAIRE" 2>/dev/null; "$APP" --fermer >/dev/null 2>&1; rm -rf "$T"; }
trap nettoyer EXIT
export XDG_RUNTIME_DIR="$T/run"; mkdir -m 700 "$XDG_RUNTIME_DIR"
export APPELS="$T/appels"; : > "$APPELS"
VRAI_SOCAT=$(command -v socat)
mkdir "$T/faux"
# socat journalisé : on sait s'il a été lancé, et avec quoi.
printf '#!/bin/bash\nprintf "socat %%s\\n" "$*" >> "$APPELS"\nexec %q "$@"\n' "$VRAI_SOCAT" > "$T/faux/socat"
# Fausse porte d'approbation : garde ce qu'elle a montré, répond FAKE_RC.
cat > "$T/faux/approbation" <<'FAUX'
#!/bin/bash
while [ $# -gt 0 ]; do [ "$1" = --detail-fichier ] && cat -- "$2" > "$APPELS.detail"; shift; done
exit "${FAKE_RC:-1}"
FAUX
chmod +x "$T/faux/"*
export PATH="$T/faux:$PATH" LS_APPROBATION="$T/faux/approbation"
export LS_DIR="$T/ls"; mkdir -m 700 "$LS_DIR"; : > "$LS_DIR/journal"
export LS_MAITRE=$$

echec() { echo "ECHEC : $*"; exit 1; }
attendre() { local i; for i in $(seq 1 30); do eval "$1" && return 0; timeout 0.1 tail -f /dev/null; done; return 1; }

"$VRAI_SOCAT" pty,raw,echo=0,link="$T/a" pty,raw,echo=0,link="$T/b" 2>/dev/null & PAIRE=$!
attendre '[[ -e "$T/a" && -e "$T/b" ]]' || echec "paire socat non créée"
A=$(readlink -f "$T/a"); B=$(readlink -f "$T/b")

# ── 1 — entrées piégées ─────────────────────────────────────────────────────
for p in /etc/passwd /dev/null /dev/sda /dev/tty '/dev/ttyUSB0;id' '-x' '/dev/pts/../sda' "$T/a" '/dev/pts/9999' \
         '/dev/serial/by-id/$(id)' $'/dev/pts/1\n' ''; do
    : > "$APPELS"; "$APP" --ouvrir "$p" 9600 8N1 >/dev/null 2>&1; rc=$?
    [[ $rc == 64 ]] || echec "périphérique « ${p//$'\n'/\\n} » : code $rc au lieu de 64"
    [[ ! -s "$APPELS" ]] || echec "périphérique « $p » : socat lancé quand même"
done
for args in "9601 8N1" "9600 9N1" "9600 8X1" "'' 8N1" "9600 ''"; do
    eval "set -- $args"; : > "$APPELS"
    "$APP" --ouvrir "$A" "$1" "$2" >/dev/null 2>&1; [[ $? == 64 && ! -s "$APPELS" ]] || echec "débit/format « $args » accepté"
done
for args in "CRLF ''" "XX bonjour" "CRLF $'a\nb'" "CRLF $'a\rb'" "CRLF $(printf 'x%.0s' $(seq 1 1025))"; do
    eval "set -- $args"; : > "$APPELS"; rm -f "$APPELS.detail"
    FAKE_RC=0 "$APP" --envoyer "$A" 9600 8N1 "$1" "$2" >/dev/null 2>&1
    [[ $? == 64 && ! -s "$APPELS" && ! -e "$APPELS.detail" ]] || echec "envoi piégé « ${args:0:30} » accepté"
done
# Témoin : un vrai pty, lui, passe le contrôle (sinon tout ce qui précède ne prouve rien).
: > "$APPELS"; "$APP" --ouvrir "$A" 9600 8N1 >/dev/null || echec "témoin : $A refusé"
[[ -s "$APPELS" ]] || echec "témoin : socat non lancé pour un vrai pty"

# ── 2 — lecture réelle ──────────────────────────────────────────────────────
attendre '[[ -s "$LS_DIR/lecteur" ]] && kill -0 "$(cat "$LS_DIR/lecteur")" 2>/dev/null' || echec "lecteur non démarré"
timeout 0.5 tail -f /dev/null
printf 'bonjour <b>&\033[31m\x01\r\n' > "$B"
attendre '[[ $(wc -c < "$LS_DIR/journal") -ge 18 ]]' || echec "rien de lu sur la ligne ($(wc -c < "$LS_DIR/journal") octets)"
"$APP" --affiche false
[[ $(head -n 1 "$LS_DIR/vue") == 'bonjour <b>&[31m' ]] || echec "vue texte : « $(head -n 1 "$LS_DIR/vue" | od -c | head -2) »"
"$APP" --affiche true
[[ $(tr -s ' ' < "$LS_DIR/vue" | head -n 1) == ' 62 6f 6e 6a 6f 75 72 20 3c 62 3e 26 1b 5b 33 31' ]] || echec "vue hexa : $(head -n 1 "$LS_DIR/vue")"
LECTEUR=$(cat "$LS_DIR/lecteur")

# ── 3 — envoi ───────────────────────────────────────────────────────────────
recevoir() { : > "$T/recu"; timeout 2 cat "$B" > "$T/recu" 2>/dev/null & RECV=$!; timeout 0.3 tail -f /dev/null; }
recevoir
FAKE_RC=0 "$APP" --envoyer "$A" 9600 8N1 CRLF 'AT+CMD=$(id);1' >/dev/null || echec "envoi autorisé : code non nul"
wait $RECV
[[ $(od -An -tx1 "$T/recu" | tr -d ' \n') == $(printf 'AT+CMD=$(id);1\r\n' | od -An -tx1 | tr -d ' \n') ]] \
    || echec "octets reçus ≠ octets demandés : $(od -c "$T/recu" | head -2)"
grep -qF 'Texte : AT+CMD=$(id);1' "$APPELS.detail" || echec "la fenêtre d'approbation ne montre pas le texte"
grep -qF '41 54 2b 43 4d 44 3d 24 28 69 64 29 3b 31 0d 0a' "$APPELS.detail" || echec "la fenêtre d'approbation ne montre pas les octets exacts"
for rc in 1 2 3; do
    recevoir
    FAKE_RC=$rc "$APP" --envoyer "$A" 9600 8N1 LF 'NE-PAS-ENVOYER' >/dev/null 2>&1; r=$?
    wait $RECV
    [[ ! -s "$T/recu" ]] || echec "approbation $rc : des octets sont partis quand même"
    [[ $r == 1 ]] || echec "approbation $rc : code $r au lieu de 1"
done
recevoir
LS_APPROBATION="$T/nexiste" "$APP" --envoyer "$A" 9600 8N1 LF 'NON' >/dev/null 2>&1; r=$?; wait $RECV
[[ $r == 2 && ! -s "$T/recu" ]] || echec "porte d'approbation absente : code $r ou envoi quand même"

# ── 4 — aucune écoute ne survit ────────────────────────────────────────────
kill -0 "$LECTEUR" 2>/dev/null || echec "le lecteur est mort trop tôt"
"$APP" --fermer >/dev/null
attendre '! kill -0 "$LECTEUR" 2>/dev/null' || echec "--fermer : socat toujours vivant"
bash -c 'export LS_MAITRE=$$; "$1" --ouvrir "$2" 9600 8N1 >/dev/null; exec sleep 60' _ "$APP" "$A" & MAITRE=$!; disown
attendre '[[ -s "$LS_DIR/lecteur" ]] && [[ $(cat "$LS_DIR/lecteur") != "$LECTEUR" ]]' || echec "second lecteur non démarré"
L2=$(cat "$LS_DIR/lecteur"); kill -0 "$L2" 2>/dev/null || echec "second lecteur absent"
kill -9 "$MAITRE"
attendre '! kill -0 "$L2" 2>/dev/null' || echec "maître tué par SIGKILL : socat orphelin ($L2)"

# ── 5 — plafond, XML, codes ────────────────────────────────────────────────
"$APP" --ouvrir "$A" 115200 8N1 >/dev/null
attendre '[[ -s "$LS_DIR/lecteur" ]] && kill -0 "$(cat "$LS_DIR/lecteur")" 2>/dev/null' || echec "lecteur (plafond) non démarré"
L3=$(cat "$LS_DIR/lecteur")
timeout 0.3 tail -f /dev/null
head -c 1300000 /dev/zero | tr '\0' 'Z' > "$B" &
attendre '! kill -0 "$L3" 2>/dev/null' || echec "plafond : la lecture continue au-delà de 1 Mio"
"$APP" --affiche false
grep -q 'journal plein' "$LS_DIR/vue" || echec "plafond : pas de message « journal plein »"
(( $(head -n 1 "$LS_DIR/vue" | wc -c) <= 8193 )) || echec "la vue dépasse 8 Kio"
"$APP" --fermer >/dev/null

"$APP" --xml > "$T/xml" || echec "--xml : code non nul"
sed 's/<input file>/<input file="1">/g' "$T/xml" | python3 -I -c 'import sys,xml.dom.minidom as m; d=m.parseString(sys.stdin.read()); assert len(d.getElementsByTagName("comboboxtext"))==3' \
    || echec "XML du dialogue mal formé"
mkdir "$T/maigre"
for c in bash od iconv tail cat mktemp rm sed dirname readlink; do ln -s "$(PATH=${PATH#*:} command -v $c)" "$T/maigre/$c"; done
env PATH="$T/maigre" "$APP" --fermer >/dev/null 2>&1; [[ $? == 2 ]] || echec "socat absent : code ≠ 2"
unset LS_DIR
"$APP" --xml >/dev/null
restes=$(find "$XDG_RUNTIME_DIR" -name 'sermo-serie.*' | head -n 1)
[[ -z "$restes" ]] || echec "un dossier temporaire survit : $restes"

# ── 6 — binaire réel (facultatif) ──────────────────────────────────────────
if [[ -n "$BIN_REEL" ]]; then
    BIN="$BIN_REEL"
    [[ -x "$BIN" ]] && command -v xvfb-run >/dev/null || { echo "binaire ou xvfb-run absent : étape 6 non faite" >&2; exit 77; }
    timeout -k 3 10 xvfb-run -a "$APP" "$BIN" > "$T/gui" 2>&1
    rc=$?
    [[ $rc == 124 ]] || echec "binaire réel : la fenêtre ne reste pas ouverte (rc=$rc) : $(tail -n 3 "$T/gui")"
    ! grep -qiE 'syntax error|parse error|unknown tag|erreur de syntaxe' "$T/gui" || echec "binaire réel : XML refusé"
    suite=" ; fenêtre réelle ouverte"
fi
echo "OK : entrées piégées refusées sans socat, lecture réelle texte/hexa, envoi seulement après « Autoriser » et octet pour octet, pas d'orphelin même après SIGKILL, journal plafonné${suite:-}"
