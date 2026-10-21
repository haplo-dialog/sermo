#!/bin/bash
# SPDX-License-Identifier: GPL-2.0-or-later
# tests/garde_bus_can.sh — haplo-dialog — 2026 — GPL-2.0-or-later
#
# examples/bus-can/ écoute un bus CAN et n'y envoie qu'après approbation. Ce test
# tourne dans un espace réseau isolé (unshare -rn, sans root) avec une VRAIE
# interface virtuelle vcan0 et les vrais candump/cansend :
#   1 — entrées piégées (interface, identifiant, données) : 64, cansend et la porte
#       d'approbation jamais appelés
#   2 — lecture réelle : inventaire exact (ID, dernières données, nombre) ; lignes
#       hostiles glissées dans le journal ignorées
#   3 — envoi : « Autoriser » → la trame exacte passe sur le bus ; refus ou porte en
#       panne → RIEN ; la porte montre identifiant et octets
#   4 — aucune écoute ne survit : --fermer, puis maître tué par SIGKILL
#   5 — journal plafonné (4 Mio) ; XML bien formé ; can-utils absent : 2
# Avec un binaire réel en argument (+ xvfb-run) : 6 — la fenêtre s'ouvre et reste ouverte.
# Rien ne touche au réseau du poste : vcan0 n'existe que dans l'espace isolé.
# Usage : garde_bus_can.sh [/chemin/vers/un-port-a-fenetre]
RACINE=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
APP="$RACINE/examples/bus-can/bus-can.sh"
for t in candump cansend ip python3 timeout unshare; do
    command -v "$t" >/dev/null || { echo "outil manquant : $t (can-utils, iproute2)" >&2; exit 77; }
done
[[ -x "$APP" ]] || { echo "script introuvable ou non exécutable : $APP" >&2; exit 1; }
if [[ -z "${BC_GARDE_ISOLE:-}" ]]; then
    unshare -rn true 2>/dev/null || { echo "espace réseau isolé impossible (unshare -rn) : rien n'est essayé" >&2; exit 77; }
    exec unshare -rn env BC_GARDE_ISOLE=1 bash "$0" "$@"
fi
if ! { ip link add vcan0 type vcan 2>/dev/null && ip link set up vcan0; }; then
    # Le module vcan dépend du noyau de la machine hôte : un exécuteur de CI
    # partagé ne l'a pas, et le conteneur ne peut pas le charger. Là seulement,
    # le banc se déclare NON JOUÉ, en clair, au lieu d'échouer pour rien.
    [ "${CI:-}" = true ] && { echo "NON JOUÉ : module vcan absent du noyau de l'exécuteur de CI (rien n'est vérifié)"; exit 0; }
    echo "vcan indisponible (module vcan ?)" >&2; exit 77
fi

T=$(mktemp -d)
ESPION=""
nettoyer() { [[ -n "$ESPION" ]] && kill "$ESPION" 2>/dev/null; "$APP" --fermer >/dev/null 2>&1; rm -rf "$T"; }
trap nettoyer EXIT
export XDG_RUNTIME_DIR="$T/run"; mkdir -m 700 "$XDG_RUNTIME_DIR"
export APPELS="$T/appels"; : > "$APPELS"
VRAI_CANSEND=$(command -v cansend)
mkdir "$T/faux"
printf '#!/bin/bash\nprintf "cansend %%s\\n" "$*" >> "$APPELS"\nexec %q "$@"\n' "$VRAI_CANSEND" > "$T/faux/cansend"
cat > "$T/faux/approbation" <<'FAUX'
#!/bin/bash
echo "approbation" >> "$APPELS"
while [ $# -gt 0 ]; do [ "$1" = --detail-fichier ] && cat -- "$2" > "$APPELS.detail"; shift; done
exit "${FAKE_RC:-1}"
FAUX
chmod +x "$T/faux/"*
export PATH="$T/faux:$PATH" BC_APPROBATION="$T/faux/approbation"
export BC_DIR="$T/bc"; mkdir -m 700 "$BC_DIR"; : > "$BC_DIR/journal"
export BC_MAITRE=$$
echec() { echo "ECHEC : $*"; exit 1; }
attendre() { local i; for i in $(seq 1 30); do eval "$1" && return 0; timeout 0.1 tail -f /dev/null; done; return 1; }

# ── 1 — entrées piégées ─────────────────────────────────────────────────────
for i in eth0 lo 'vcan0;id' '-x' can999 '' 'vcan0 ' 'vcan1' $'vcan0\n'; do
    : > "$APPELS"; FAKE_RC=0 "$APP" --envoyer "$i" 123 00 >/dev/null 2>&1; rc=$?
    [[ $rc == 64 && ! -s "$APPELS" ]] || echec "interface « ${i//$'\n'/\\n} » : code $rc ou outil appelé"
done
for id in 800 20000000 12 GG1 7FF0 '' '-1' '1 2' 0x123 '123;id'; do
    : > "$APPELS"; FAKE_RC=0 "$APP" --envoyer vcan0 "$id" 00 >/dev/null 2>&1; rc=$?
    [[ $rc == 64 && ! -s "$APPELS" ]] || echec "identifiant « $id » : code $rc ou outil appelé"
done
for dn in 0 123 001122334455667788 zz '00 11' '$(id)' 00, ; do
    : > "$APPELS"; FAKE_RC=0 "$APP" --envoyer vcan0 123 "$dn" >/dev/null 2>&1; rc=$?
    [[ $rc == 64 && ! -s "$APPELS" ]] || echec "données « $dn » : code $rc ou outil appelé"
done
"$APP" --ouvrir eth0 >/dev/null 2>&1; [[ $? == 64 ]] || echec "écoute d'une interface non CAN acceptée"
# Témoin : une trame valide passe bien la porte puis cansend.
: > "$APPELS"; FAKE_RC=0 "$APP" --envoyer vcan0 7ff 00ff >/dev/null || echec "témoin : trame valide refusée"
[[ $(cat "$APPELS") == $'approbation\ncansend vcan0 7FF#00FF' ]] || echec "témoin : appels inattendus : $(cat "$APPELS")"

# ── 2 — lecture réelle ──────────────────────────────────────────────────────
"$APP" --ouvrir vcan0 >/dev/null || echec "--ouvrir vcan0 a échoué"
attendre '[[ -s "$BC_DIR/lecteur" ]] && kill -0 "$(cat "$BC_DIR/lecteur")" 2>/dev/null' || echec "candump non démarré"
timeout 0.3 tail -f /dev/null
cansend vcan0 123#DEADBEEF; cansend vcan0 123#0102; cansend vcan0 1ABCDEF0#
attendre '[[ $(wc -l < "$BC_DIR/journal") -ge 3 ]]' || echec "rien de lu sur le bus"
printf '%s\n' '(1.0) vcan0 <b>#&amp;' 'pas une trame' '(2.0) vcan0 123#ZZ' '(3.0) vcan0 999#00' >> "$BC_DIR/journal"
"$APP" --inventaire | cut -d'|' -f1-3 > "$T/inv"
diff "$T/inv" <(printf '%s\n' '123|01 02|2' '1ABCDEF0|(vide)|1') >/dev/null || echec "inventaire faux : $(tr '\n' ' ' < "$T/inv")"
! grep -qE '[<>&]' "$T/inv" || echec "une ligne hostile du journal est passée dans l'inventaire"
LECTEUR=$(cat "$BC_DIR/lecteur")

# ── 3 — envoi ───────────────────────────────────────────────────────────────
candump -L vcan0 > "$T/bus" 2>/dev/null & ESPION=$!
timeout 0.3 tail -f /dev/null
FAKE_RC=0 "$APP" --envoyer vcan0 1abcdef0 DEADBEEF00112233 >/dev/null || echec "envoi autorisé : code non nul"
grep -qF 'Identifiant : 1ABCDEF0 (29 bits)' "$APPELS.detail" || echec "la porte ne montre pas l'identifiant"
grep -qF 'Octets (8) : DE AD BE EF 00 11 22 33' "$APPELS.detail" || echec "la porte ne montre pas les octets exacts"
for rc in 1 2 3; do FAKE_RC=$rc "$APP" --envoyer vcan0 456 AA >/dev/null 2>&1; [[ $? == 1 ]] || echec "approbation $rc : code ≠ 1"; done
BC_APPROBATION="$T/nexiste" "$APP" --envoyer vcan0 456 BB >/dev/null 2>&1; [[ $? == 2 ]] || echec "porte absente : code ≠ 2"
timeout 0.5 tail -f /dev/null
kill "$ESPION"; ESPION=""
grep -qE ' vcan0 1ABCDEF0#DEADBEEF00112233$' "$T/bus" || echec "la trame autorisée n'est pas passée sur le bus : $(cat "$T/bus")"
! grep -qE '#AA$|#BB$' "$T/bus" || echec "une trame REFUSÉE est passée sur le bus"

# ── 4 — aucune écoute ne survit ────────────────────────────────────────────
"$APP" --fermer >/dev/null
attendre '! kill -0 "$LECTEUR" 2>/dev/null' || echec "--fermer : candump toujours vivant"
bash -c 'export BC_MAITRE=$$; "$1" --ouvrir vcan0 >/dev/null; exec sleep 60' _ "$APP" & MAITRE=$!; disown
attendre '[[ -s "$BC_DIR/lecteur" && $(cat "$BC_DIR/lecteur") != "$LECTEUR" ]]' || echec "second candump non démarré"
L2=$(cat "$BC_DIR/lecteur")
kill -9 "$MAITRE"
attendre '! kill -0 "$L2" 2>/dev/null' || echec "maître tué par SIGKILL : candump orphelin ($L2)"

# ── 5 — plafond, XML, codes ────────────────────────────────────────────────
"$APP" --ouvrir vcan0 >/dev/null
attendre '[[ -s "$BC_DIR/lecteur" ]] && kill -0 "$(cat "$BC_DIR/lecteur")" 2>/dev/null' || echec "candump (plafond) non démarré"
L3=$(cat "$BC_DIR/lecteur")
head -c 4300000 /dev/zero | tr '\0' 'x' >> "$BC_DIR/journal"
attendre '! kill -0 "$L3" 2>/dev/null' || echec "plafond : la lecture continue au-delà de 4 Mio"
"$APP" --inventaire | grep -q '^JOURNAL PLEIN|' || echec "plafond : pas d'avertissement « journal plein »"
"$APP" --fermer >/dev/null
"$APP" --xml > "$T/xml" || echec "--xml : code non nul"
python3 -I -c 'import sys,xml.dom.minidom as m; d=m.parse(sys.argv[1]); assert len(d.getElementsByTagName("tree"))==1' "$T/xml" || echec "XML mal formé"
grep -q '<default>vcan0</default>' "$T/xml" || echec "l'interface vcan0 n'est pas proposée par défaut"
mkdir "$T/maigre"
for c in bash ip awk cat mktemp rm sed dirname readlink; do ln -s "$(PATH=${PATH#*:} command -v $c)" "$T/maigre/$c"; done
env PATH="$T/maigre" "$APP" --fermer >/dev/null 2>&1; [[ $? == 2 ]] || echec "can-utils absent : code ≠ 2"
( unset BC_DIR; "$APP" --xml >/dev/null )
[[ -z "$(find "$XDG_RUNTIME_DIR" -name 'sermo-can.*')" ]] || echec "un dossier temporaire survit"

# ── 6 — binaire réel (facultatif) ──────────────────────────────────────────
if [[ -n "${1:-}" ]]; then
    BIN="$1"
    [[ -x "$BIN" ]] && command -v xvfb-run >/dev/null || { echo "binaire ou xvfb-run absent : étape 6 non faite" >&2; exit 77; }
    ( unset BC_DIR; timeout -k 3 10 xvfb-run -a "$APP" "$BIN" ) > "$T/gui" 2>&1
    rc=$?
    [[ $rc == 124 ]] || echec "binaire réel : la fenêtre ne reste pas ouverte (rc=$rc) : $(tail -n 3 "$T/gui")"
    ! grep -qiE 'syntax error|parse error|unknown tag|erreur de syntaxe' "$T/gui" || echec "binaire réel : XML refusé"
    suite=" ; fenêtre réelle ouverte"
fi
echo "OK : entrées piégées refusées sans cansend ni porte, inventaire exact sur un vrai vcan0, envoi seulement après « Autoriser » et octet pour octet, pas d'orphelin même après SIGKILL, journal plafonné${suite:-}"
