#!/bin/bash
# SPDX-License-Identifier: GPL-2.0-or-later
# tests/garde_gnss_moniteur.sh — haplo-dialog — 2026 — GPL-2.0-or-later
#
# examples/gnss-moniteur/ affiche ce que gpsd lui envoie. Ce test vérifie, avec un
# faux gpspipe (aucun récepteur, aucun gpsd) :
#   1 — fixation 3D, 2D, aucune, aucun satellite : champs justes, position masquée sans fix
#   2 — JSON piégé (balises, &, guillemets, $(…), NaN, chaînes à la place des nombres,
#       lignes malformées, ligne géante) : rien de piégé ne sort, tout devient « — »
#   3 — « Ouvrir la carte » et « Copier » : URL exacte, et RIEN sans position
#   4 — codes : gpspipe absent 2, période invalide 64, fichier de démo illisible 64
#   5 — XML bien formé ; aucun dossier temporaire ne reste
# Avec un binaire réel en argument (+ xvfb-run) : 6 — la fenêtre de démo s'ouvre et reste ouverte.
# Les faux binaires prouvent la logique, PAS la compatibilité avec un vrai gpsd :
# il faut pour cela un enregistrement réel (voir le README de l'exemple).
# Usage : garde_gnss_moniteur.sh [/chemin/vers/un-port-a-fenetre]
RACINE=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
APP="$RACINE/examples/gnss-moniteur/gnss-moniteur.sh"
DEMO="$RACINE/examples/gnss-moniteur/echantillons/demo.jsonl"
for t in python3 timeout mktemp; do
    command -v "$t" >/dev/null || { echo "outil manquant : $t" >&2; exit 77; }
done
[[ -x "$APP" && -r "$DEMO" ]] || { echo "script ou échantillon introuvable" >&2; exit 1; }

T=$(mktemp -d); trap 'rm -rf "$T"' EXIT
export XDG_RUNTIME_DIR="$T/run"; mkdir -m 700 "$XDG_RUNTIME_DIR"
export APPELS="$T/appels"; : > "$APPELS"
mkdir "$T/faux"
cat > "$T/faux/gpspipe" <<'FAUX'
#!/bin/bash
printf 'gpspipe %s\n' "$*" >> "$APPELS"
cat -- "$FAKE_GPS"
FAUX
cat > "$T/faux/xdg-open" <<'FAUX'
#!/bin/bash
printf 'xdg-open %s\n' "$*" >> "$APPELS"
FAUX
cat > "$T/faux/xclip" <<'FAUX'
#!/bin/bash
printf 'xclip %s <%s>\n' "$*" "$(cat)" >> "$APPELS"
FAUX
chmod +x "$T/faux/"*
export PATH="$T/faux:$PATH"
unset WAYLAND_DISPLAY

echec() { echo "ECHEC : $*"; exit 1; }
export GNSS_CACHE="$T/cache"; mkdir -m 700 "$GNSS_CACHE"
lire() { export FAKE_GPS="$T/ech"; printf '%s\n' "$@" > "$FAKE_GPS"; "$APP" --lire || echec "--lire : code non nul"; }
champ() { "$APP" --champ "$1"; }

# ── 1 — états de fixation ───────────────────────────────────────────────────
SKY='{"class":"SKY","hdop":0.91,"pdop":1.52,"satellites":[{"PRN":5,"gnssid":0,"el":62,"az":145,"ss":41,"used":true},{"PRN":11,"gnssid":2,"el":55,"az":90,"ss":40,"used":true},{"PRN":4,"gnssid":6,"el":40,"az":30,"ss":33,"used":false}]}'
lire "$SKY" '{"class":"TPV","mode":3,"time":"2026-10-18T08:00:12.000Z","lat":48.8363361,"lon":2.3364479,"altHAE":112.45,"speed":2.5,"track":270.4}'
[[ $(champ FIX) == 3D ]] || echec "3D : fixation $(champ FIX)"
[[ $(champ LAT) == 48.836336 && $(champ LON) == 2.336448 ]] || echec "3D : position $(champ LAT),$(champ LON)"
[[ $(champ ALT) == "112.5 m" ]] || echec "3D : altitude $(champ ALT)"
[[ $(champ VITESSE) == "9.0 km/h" ]] || echec "3D : vitesse (m/s → km/h) $(champ VITESSE)"
[[ $(champ CAP) == "270°" && $(champ HEURE) == 2026-10-18T08:00:12.000Z ]] || echec "3D : cap ou heure"
[[ $(champ DOP) == "0.91 / 1.52" ]] || echec "3D : DOP $(champ DOP)"
[[ $(champ SATS) == "3 vus, 2 utilisés" ]] || echec "3D : satellites $(champ SATS)"
diff <(cat "$GNSS_CACHE/sats") <(printf '%s\n' 'GPS|5|62°|145°|41 dB-Hz|oui' 'Galileo|11|55°|90°|40 dB-Hz|oui' 'GLONASS|4|40°|30°|33 dB-Hz|non') >/dev/null \
    || echec "3D : tableau des satellites : $(cat "$GNSS_CACHE/sats")"
lire "$SKY" '{"class":"TPV","mode":2,"lat":48.8,"lon":2.3,"altHAE":112.4}'
[[ $(champ FIX) == 2D && $(champ LAT) == 48.800000 && $(champ ALT) == "—" ]] || echec "2D : altitude non masquée ou position fausse"
lire "$SKY" '{"class":"TPV","mode":1,"lat":48.8,"lon":2.3}'
[[ $(champ FIX) == aucune && $(champ LAT) == "—" && $(champ LON) == "—" ]] || echec "sans fixation, la position s'affiche quand même"
lire '{"class":"SKY","satellites":[]}' '{"class":"TPV","mode":1}'
[[ $(champ SATS) == "0 vus, 0 utilisés" && ! -s "$GNSS_CACHE/sats" ]] || echec "aucun satellite mal rendu"
lire ''
[[ $(champ FIX) == "pas de réponse de gpsd" ]] || echec "gpsd muet non signalé"

# ── 2 — JSON piégé ──────────────────────────────────────────────────────────
GEANTE=$(python3 -I -c 'print("{\"class\":\"TPV\",\"mode\":3,\"lat\":1,\"lon\":1,\"x\":\"" + "A"*70000 + "\"}")')
lire 'pas du json <b>&' '[1,2,3]' 'null' \
     '{"class":"SKY","hdop":"<b>","pdop":NaN,"satellites":[{"PRN":"5;rm -rf ~","el":"</tree>","az":1e309,"ss":-5,"used":"true"},{"PRN":7,"gnssid":"<x>","el":45,"az":10,"ss":30,"used":1},"chaine",{"PRN":true}]}' \
     '{"class":"TPV","mode":3,"lat":"48<x>","lon":Infinity,"altHAE":"$(touch '"$T"'/pwn)","speed":"&amp;","track":400,"time":"2026-10-18T08:00:00Z<script>"}' \
     "$GEANTE"
for f in "$GNSS_CACHE/etat" "$GNSS_CACHE/sats"; do
    ! grep -qE '[<>&"$`;]|rm |touch' "$f" || echec "une valeur piégée est sortie dans $(basename "$f") : $(cat "$f")"
done
[[ ! -e "$T/pwn" ]] || echec "une commande du JSON a été exécutée"
[[ $(champ LAT) == "—" && $(champ LON) == "—" && $(champ ALT) == "—" && $(champ CAP) == "—" && $(champ HEURE) == "—" ]] \
    || echec "valeurs piégées non ramenées à « — » : $(tr '\n' ' ' < "$GNSS_CACHE/etat")"
[[ $(champ DOP) == "— / —" ]] || echec "DOP piégé : $(champ DOP)"
[[ $(cat "$GNSS_CACHE/sats") == '—|7|45°|10°|30 dB-Hz|non' ]] || echec "satellites piégés : $(cat "$GNSS_CACHE/sats")"
[[ $(champ 'LAT=x;' ) == "—" && $(champ '.*') == "—" ]] || echec "--champ accepte une clé hors liste"

# ── 3 — carte et presse-papiers ─────────────────────────────────────────────
lire "$SKY" '{"class":"TPV","mode":3,"lat":-33.856784,"lon":151.215297,"altHAE":10}'
: > "$APPELS"; "$APP" --carte; sleep 0.3
grep -qxF 'xdg-open https://www.openstreetmap.org/?mlat=-33.856784&mlon=151.215297#map=16/-33.856784/151.215297' "$APPELS" \
    || echec "carte : URL inattendue : $(cat "$APPELS")"
: > "$APPELS"; "$APP" --copier
grep -qxF 'xclip -selection clipboard <-33.856784,151.215297>' "$APPELS" || echec "copier : $(cat "$APPELS")"
lire "$SKY" '{"class":"TPV","mode":1}'
: > "$APPELS"; "$APP" --carte 2>/dev/null; "$APP" --copier 2>/dev/null; sleep 0.3
[[ ! -s "$APPELS" ]] || echec "sans position, la carte ou le presse-papiers a été appelé : $(cat "$APPELS")"
unset GNSS_CACHE

# ── 4 — codes de sortie ─────────────────────────────────────────────────────
mkdir "$T/maigre"
for c in bash python3 sed head mktemp cat rm readlink dirname; do ln -s "$(PATH=${PATH#*:} command -v $c)" "$T/maigre/$c"; done
env PATH="$T/maigre" "$APP" >/dev/null 2>&1; [[ $? == 2 ]] || echec "gpspipe absent : code ≠ 2"
env PATH="$T/maigre" "$APP" --demo "$DEMO" /nexiste/pas >/dev/null 2>&1; [[ $? == 2 ]] || echec "sermo absent en démo : code ≠ 2"
for p in 0 11 x 1.5 -2; do
    GNSS_PERIODE=$p "$APP" --xml >/dev/null 2>&1; [[ $? == 64 ]] || echec "période « $p » acceptée"
done
"$APP" --demo "$T/nexiste" >/dev/null 2>&1; [[ $? == 64 ]] || echec "démo illisible : code ≠ 64"
"$APP" --demo "$T" >/dev/null 2>&1; [[ $? == 64 ]] || echec "démo = dossier : code ≠ 64"
"$APP" --inconnu >/dev/null 2>&1; [[ $? == 64 ]] || echec "option inconnue : code ≠ 64"

# ── 5 — XML et ménage ───────────────────────────────────────────────────────
"$APP" --xml > "$T/xml" || echec "--xml : code non nul"
python3 -I -c 'import sys,xml.dom.minidom as m; d=m.parse(sys.argv[1]); assert len(d.getElementsByTagName("tree"))==1; assert any(b.getElementsByTagName("label")[0].firstChild.data=="Copier la position" for b in d.getElementsByTagName("button"))' "$T/xml" \
    || echec "XML du dialogue mal formé, ou bouton Copier absent alors que xclip est là"
env PATH="$T/maigre" "$APP" --xml | grep -q 'Copier la position' && echec "bouton Copier présent sans outil de presse-papiers"
restes=$(find "$XDG_RUNTIME_DIR" -name 'sermo-gnss.*' | head -n 1)
[[ -z "$restes" ]] || echec "un dossier temporaire survit : $restes"

# ── 6 — binaire réel (facultatif) ──────────────────────────────────────────
if [[ -n "${1:-}" ]]; then
    BIN="$1"
    [[ -x "$BIN" ]] && command -v xvfb-run >/dev/null || { echo "binaire ou xvfb-run absent : étape 6 non faite" >&2; exit 77; }
    PATH=${PATH#*:} GNSS_PERIODE=1 timeout -k 3 10 xvfb-run -a "$APP" --demo "$DEMO" "$BIN" > "$T/gui" 2>&1
    rc=$?
    [[ $rc == 124 ]] || echec "binaire réel : la fenêtre ne reste pas ouverte (rc=$rc) : $(tail -n 3 "$T/gui")"
    ! grep -qiE 'syntax error|parse error|unknown tag|erreur de syntaxe' "$T/gui" || echec "binaire réel : XML refusé"
    suite=" ; fenêtre réelle ouverte (démo)"
fi
echo "OK : fixations 3D/2D/aucune, JSON piégé neutralisé, carte et copie seulement avec une position, codes 2/64${suite:-}"
