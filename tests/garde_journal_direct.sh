#!/bin/bash
# SPDX-License-Identifier: GPL-2.0-or-later
# tests/garde_journal_direct.sh — haplo-dialog — 2026 — GPL-2.0-or-later
#
# examples/journal-direct/ suit journalctl en fond et met en forme ce qu'il lit.
# Ce test utilise un FAUX journalctl (il rend un enregistrement JSON fabriqué puis
# reste ouvert, comme « -f ») — le vrai journal du poste n'est ni lu ni montré :
#   1 — entrées piégées (source, gravité, unité, filtre) : 64, journalctl jamais lancé ;
#       arguments exacts pour une entrée valide
#   2 — mise en forme : heure, gravité, source, message ; message binaire décodé ;
#       balises, |, caractères de contrôle neutralisés ; entrée géante ignorée ;
#       lignes non JSON ignorées ; filtre ; plus récent en haut
#   3 — aucun suivi ne survit : --fermer, puis maître tué par SIGKILL
#   4 — journal plafonné (8 Mio) ; XML bien formé ; outil absent : 2
# Avec un binaire réel en argument (+ xvfb-run) : 5 — la fenêtre s'ouvre et reste ouverte.
# Usage : garde_journal_direct.sh [/chemin/vers/un-port-a-fenetre]
RACINE=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
APP="$RACINE/examples/journal-direct/journal-direct.sh"
BIN_REEL="${1:-}"
for t in python3 timeout mktemp; do
    command -v "$t" >/dev/null || { echo "outil manquant : $t" >&2; exit 77; }
done
[[ -x "$APP" ]] || { echo "script introuvable ou non exécutable : $APP" >&2; exit 1; }

T=$(mktemp -d)
nettoyer() { "$APP" --fermer >/dev/null 2>&1; rm -rf "$T"; }
trap nettoyer EXIT
export XDG_RUNTIME_DIR="$T/run"; mkdir -m 700 "$XDG_RUNTIME_DIR"
export APPELS="$T/appels"; : > "$APPELS"
mkdir "$T/faux"
# Faux journalctl : journalise ses arguments, rend $FAUX_JOURNAL, puis attend comme « -f ».
cat > "$T/faux/journalctl" <<'FAUX'
#!/bin/bash
{ echo "journalctl"; printf '  [%s]\n' "$@"; } >> "$APPELS"
cat -- "$FAUX_JOURNAL"
exec sleep 300
FAUX
chmod +x "$T/faux/journalctl"
export PATH="$T/faux:$PATH" FAUX_JOURNAL="$T/enregistrement.json"
export JD_DIR="$T/jd"; mkdir -m 700 "$JD_DIR"; : > "$JD_DIR/journal"
export JD_MAITRE=$$
echec() { echo "ECHEC : $*"; exit 1; }
attendre() { local i; for i in $(seq 1 30); do eval "$1" && return 0; timeout 0.1 tail -f /dev/null; done; return 1; }

python3 -I - "$FAUX_JOURNAL" <<'PY'
import json, sys
t0 = 1792332000 * 10**6
L = [
 {"__REALTIME_TIMESTAMP": str(t0), "PRIORITY": "3", "SYSLOG_IDENTIFIER": "sshd", "MESSAGE": "Connexion refusée pour root"},
 {"__REALTIME_TIMESTAMP": str(t0 + 10**6), "PRIORITY": "4", "_COMM": "cupsd", "MESSAGE": [104, 105, 0, 27, 255, 33]},
 {"__REALTIME_TIMESTAMP": str(t0 + 2 * 10**6), "PRIORITY": "2", "SYSLOG_IDENTIFIER": "<b>pirate|", "MESSAGE": "a|b <i>&amp; $(id)\x1b[31m\x07fin"},
 {"__REALTIME_TIMESTAMP": "pas un nombre", "PRIORITY": "99", "MESSAGE": None},
 {"__REALTIME_TIMESTAMP": str(t0 + 4 * 10**6), "PRIORITY": "6", "SYSLOG_IDENTIFIER": "x", "MESSAGE": "A" * 70000},
 {"__REALTIME_TIMESTAMP": str(t0 + 5 * 10**6), "PRIORITY": "6", "SYSLOG_IDENTIFIER": "kernel", "MESSAGE": "B" * 1000},
]
with open(sys.argv[1], "w") as f:
    for e in L:
        f.write(json.dumps(e) + "\n")
    f.write("pas du json\n[1,2]\nnull\n")
PY

# ── 1 — entrées piégées ─────────────────────────────────────────────────────
for args in "tout erreur" "'' erreur" "système grave" "système ''" "système 3" "session erreur -x" \
            "système erreur 'a;b'" "système erreur '\$(id)'" "système erreur 'a b'" "système erreur $'a\nb'"; do
    eval "set -- $args"; : > "$APPELS"
    "$APP" --ouvrir "$@" >/dev/null 2>&1; rc=$?
    [[ $rc == 64 && ! -s "$APPELS" ]] || echec "entrée « $args » : code $rc ou journalctl lancé"
done
"$APP" --vue "$(printf 'x%.0s' $(seq 1 201))" >/dev/null 2>&1; [[ $? == 64 ]] || echec "filtre de 201 caractères accepté"
"$APP" --vue $'a\nb' >/dev/null 2>&1; [[ $? == 64 ]] || echec "filtre sur deux lignes accepté"
: > "$APPELS"; "$APP" --ouvrir session notice ssh.service >/dev/null || echec "témoin : entrée valide refusée"
attendre '[[ -s "$JD_DIR/lecteur" ]] && grep -q "\[ssh.service\]" "$APPELS"' || echec "témoin : journalctl non lancé"
diff <(sed -n '2,$p' "$APPELS") <(printf '  [%s]\n' -f -n 100 -o json --no-pager --user -p 5 -u ssh.service) >/dev/null \
    || echec "témoin : arguments de journalctl : $(tr '\n' ' ' < "$APPELS")"
LECTEUR=$(cat "$JD_DIR/lecteur")

# ── 2 — mise en forme ───────────────────────────────────────────────────────
attendre '[[ $(wc -l < "$JD_DIR/journal") -ge 9 ]]' || echec "rien de lu"
"$APP" --vue > "$T/vue" || echec "--vue : code non nul"
[[ $(wc -l < "$T/vue") == 5 ]] || echec "vue : 5 lignes attendues (entrée géante et lignes non JSON ignorées) : $(cat "$T/vue")"
[[ $(awk -F'|' '{print NF}' "$T/vue" | sort -u) == 4 ]] || echec "vue : une ligne n'a pas 4 colonnes : $(cat "$T/vue")"
[[ $(sed -n 1p "$T/vue") == *"|info|kernel|"*"…" ]] || echec "le plus récent n'est pas en haut, ou message long non coupé : $(sed -n 1p "$T/vue" | cut -c1-80)"
(( $(sed -n 1p "$T/vue" | cut -d'|' -f4 | wc -m) <= 302 )) || echec "message non plafonné à 300 caractères"
grep -qE '\|critique\|<b>pirate¦\|a¦b <i>&amp; \$\(id\) \[31m fin$' "$T/vue" || echec "message piégé mal rendu : $(grep pirate "$T/vue")"
grep -qE '\|avert\.\|cupsd\|hi  .*!$' "$T/vue" || echec "message binaire mal décodé : $(grep cupsd "$T/vue")"
grep -qE '^—\|—\|—\|—$' "$T/vue" || echec "entrée sans champs valides mal rendue"
grep -qE '\|erreur\|sshd\|Connexion refusée pour root$' "$T/vue" || echec "message ordinaire mal rendu"
! LC_ALL=C grep -q $'[\x01-\x08\x0b-\x1f\x7f]' "$T/vue" || echec "caractères de contrôle restés"
[[ $("$APP" --vue "CONNEXION" | wc -l) == 1 ]] || echec "filtre (casse ignorée) : $("$APP" --vue "CONNEXION")"
[[ $("$APP" --vue "rien de tel" | wc -l) == 0 ]] || echec "filtre sans résultat"

# ── 3 — aucun suivi ne survit ──────────────────────────────────────────────
"$APP" --fermer >/dev/null
attendre '! kill -0 "$LECTEUR" 2>/dev/null' || echec "--fermer : journalctl toujours vivant"
bash -c 'export JD_MAITRE=$$; "$1" --ouvrir système erreur >/dev/null; exec sleep 60' _ "$APP" & MAITRE=$!; disown
attendre '[[ -s "$JD_DIR/lecteur" && $(cat "$JD_DIR/lecteur") != "$LECTEUR" ]]' || echec "second suivi non démarré"
L2=$(cat "$JD_DIR/lecteur")
kill -9 "$MAITRE"
attendre '! kill -0 "$L2" 2>/dev/null' || echec "maître tué par SIGKILL : journalctl orphelin ($L2)"

# ── 4 — plafond, XML, codes ────────────────────────────────────────────────
"$APP" --ouvrir système erreur >/dev/null
attendre '[[ -s "$JD_DIR/lecteur" ]] && kill -0 "$(cat "$JD_DIR/lecteur")" 2>/dev/null' || echec "suivi (plafond) non démarré"
L3=$(cat "$JD_DIR/lecteur")
head -c 8400000 /dev/zero | tr '\0' 'x' >> "$JD_DIR/journal"
attendre '! kill -0 "$L3" 2>/dev/null' || echec "plafond : la lecture continue au-delà de 8 Mio"
"$APP" --vue | grep -q '^—|—|journal plein|' || echec "plafond : pas de message « journal plein »"
"$APP" --fermer >/dev/null
"$APP" --xml > "$T/xml" || echec "--xml : code non nul"
python3 -I -c 'import sys,xml.dom.minidom as m; d=m.parse(sys.argv[1]); assert len(d.getElementsByTagName("comboboxtext"))==2' "$T/xml" || echec "XML mal formé"
mkdir "$T/maigre"
for c in bash cat mktemp rm sed dirname readlink stat; do ln -s "$(PATH=${PATH#*:} command -v $c)" "$T/maigre/$c"; done
env PATH="$T/maigre" "$APP" --fermer >/dev/null 2>&1; [[ $? == 2 ]] || echec "journalctl/python3 absents : code ≠ 2"
( unset JD_DIR; "$APP" --xml >/dev/null )
[[ -z "$(find "$XDG_RUNTIME_DIR" -name 'sermo-journal.*')" ]] || echec "un dossier temporaire survit"

# ── 5 — binaire réel (facultatif) ──────────────────────────────────────────
if [[ -n "$BIN_REEL" ]]; then
    [[ -x "$BIN_REEL" ]] && command -v xvfb-run >/dev/null || { echo "binaire ou xvfb-run absent : étape 5 non faite" >&2; exit 77; }
    ( unset JD_DIR; timeout -k 3 10 xvfb-run -a "$APP" "$BIN_REEL" ) > "$T/gui" 2>&1
    rc=$?
    [[ $rc == 124 ]] || echec "binaire réel : la fenêtre ne reste pas ouverte (rc=$rc) : $(tail -n 3 "$T/gui")"
    ! grep -qiE 'syntax error|parse error|unknown tag|erreur de syntaxe' "$T/gui" || echec "binaire réel : XML refusé"
    suite=" ; fenêtre réelle ouverte"
fi
echo "OK : entrées piégées refusées sans journalctl, messages (binaires, piégés, géants) mis en forme sans risque, filtre, pas d'orphelin même après SIGKILL, journal plafonné${suite:-}"
