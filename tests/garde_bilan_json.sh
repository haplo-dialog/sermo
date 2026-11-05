#!/bin/bash
# SPDX-License-Identifier: GPL-2.0-or-later
# garde_bilan_json.sh — ci/bilan-json.sh lit-il les bons chiffres dans les journaux ?
#
# Des journaux FABRIQUÉS, aux chiffres connus, dans un bac ; le JSON produit doit
# les rendre tels quels. Puis sabotage : un chiffre changé dans un journal doit
# changer dans le JSON (sinon l'analyseur ne lit pas ce journal), et un verdict
# ÉCHEC doit être compté. Codes : 0 = OK · 1 = écart · 77 = outil manquant.
set -u
RACINE=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
command -v python3 >/dev/null || { echo "IGNORÉ : python3 manquant" >&2; exit 77; }
mkdir -p "${TMPDIR:-$RACINE/_build}" 2>/dev/null   # un clone neuf n'a pas _build/ (vu le 2026-11-05 sur bancs-sur-paquets)
BAC=$(mktemp -d "${TMPDIR:-$RACINE/_build}/garde_bilan_json.XXXXXX") || exit 77
trap 'rm -rf "$BAC"' EXIT
J="$BAC/journaux/bancs"; mkdir -p "$J"
ecarts=0
ko() { echo "  ✘ $*"; ecarts=$((ecarts + 1)); }
ok() { echo "  ✔ $*"; }

# ── Journaux fabriqués : gtk3 complet, ncurses, html, qt6 en échec ────────────
printf '────  Résultats pour /x/gtk3sermo : 55 PASS | 0 FAIL | 0 SKIP\nSUCCÈS — tous les tests passent\n' > "$J/gtk3_xml.log"
printf 'OK : 8 garanties de durcissement mesurees sur le binaire\n' > "$J/gtk3_durcissement.log"
printf '  ✔ fichier chargé, dialogue fermé par son gestionnaire (rc=0)\ngarde_glade : OK — conforme pour gtk3sermo.\n' > "$J/gtk3_glade.log"
printf '  ✔ --version annonce VTE\ngarde_terminal : OK — conforme pour gtk3sermo.\n' > "$J/gtk3_terminal.log"
printf '  ✔ une valeur ordinaire traverse intacte\ngarde_echappement_sortie : OK — 4 cas, aucune exécution.\n' > "$J/gtk3_echappement.log"
printf '  ✔ tree\ngarde_clic_widgets : OK — 17 widgets, aucun plantage.\n' > "$J/gtk3_clic_widgets.log"
printf '  wifi-scan        (non joué : nmcli absent)\n  usb-moniteur     (non joué : udevadm absent)\n  ================================================\n  72 exemples : 72 OK · 0 CRASH · 0 SYNTAX · 0 sans fenêtre\n  ================================================\n' > "$J/gtk3_exemples_reels.log"
printf '────  Résultats pour /x/ncursessermo : 55 PASS | 0 FAIL | 0 SKIP\n' > "$J/ncurses_xml.log"
printf 'garde_ncurses_mot_de_passe : OK — saisie masquée, valeur exportée.\n' > "$J/ncurses_mot_de_passe.log"
printf '  ✔ option refusée : code 1 et message\ngarde_glade : OK — conforme pour ncursessermo.\n' > "$J/ncurses_glade.log"
printf '  ✔ terminal non promis ici ; le dialogue tourne et se ferme (rc=0)\ngarde_terminal : OK\n' > "$J/html_terminal.log"
printf '────  Résultats pour /x/qt6sermo : 54 PASS | 1 FAIL | 0 SKIP\nÉCHEC\n' > "$J/qt6_xml.log"
printf 'gtk3       ✅ 78 cas · 78 au vert · 0 en écart · 0 bloqué(s).\nqt6        ❌ 78 cas · 77 au vert · 1 en écart · 0 bloqué(s).\nncurses    ✅ 78 cas · 78 au vert · 0 en écart · 0 bloqué(s).\nBackends joués : 3 · à parité : 2 · en écart : 1 · absents : 0\n' > "$J/comportement__tous_les_ports_.log"
{
    printf 'gtk3 xml\tOK\t0\tSUCCÈS — tous les tests passent\n'
    printf 'gtk3 durcissement\tOK\t0\tOK : 8 garanties\n'
    printf 'gtk3 glade\tOK\t0\tgarde_glade : OK\n'
    printf 'gtk3 terminal\tOK\t0\tgarde_terminal : OK\n'
    printf 'gtk3 échappement\tOK\t0\tgarde_echappement_sortie : OK\n'
    printf 'gtk3 clic widgets\tOK\t0\tgarde_clic_widgets : OK\n'
    printf 'gtk3 exemples réels\tOK\t0\t72 exemples : 72 OK\n'
    printf 'gtk3 identité\tOK\t0\tgarde_identite_port : OK\n'
    printf 'ncurses xml\tOK\t0\tSUCCÈS\n'
    printf 'ncurses mot de passe\tOK\t0\tOK\n'
    printf 'ncurses glade\tOK\t0\tgarde_glade : OK\n'
    printf 'qt6 xml\tÉCHEC\t1\tÉCHEC\n'
    printf 'comportement (tous les ports)\tÉCHEC\t1\tBackends joués : 3\n'
    printf 'sdl3\tÉCHEC\t-\tbinaire absent : /x/sdl3sermo\n'
} > "$J/verdicts.tsv"

sortie=$(bash "$RACINE/ci/bilan-json.sh" "$J" abc1234) || { echo "ECHEC : ci/bilan-json.sh rc=$?"; exit 1; }
case "$sortie" in "BILAN-JSON: {"*) ok "une ligne BILAN-JSON: sur la sortie" ;; *) ko "pas de ligne BILAN-JSON: ($sortie)" ;; esac
[ -f "$BAC/journaux/bilan.json" ] && ok "bilan.json écrit à côté des journaux" || ko "bilan.json absent"

lit() { python3 -I -c '
import json, sys
d = json.load(open(sys.argv[1]))
for ch in sys.argv[2:]:
    v = d
    for k in ch.split("."): v = v[k] if not k.isdigit() else v[int(k)]
    print(v)
' "$BAC/journaux/bilan.json" "$@" 2>/dev/null; }
attendu() { # attendu <chemin.json> <valeur>
    local vu; vu=$(lit "$1")
    [ "$vu" = "$2" ] && ok "$1 = $2" || ko "$1 : vu « $vu », attendu « $2 »"
}
attendu commit abc1234
attendu bancs.joues 13               # 14 lignes, dont « binaire absent » (rc « - ») qui n'est pas un banc joué
attendu bancs.echecs 3
attendu ports.gtk3.xml.verts 55
attendu ports.gtk3.xml.rouges 0
attendu ports.gtk3.comportement.cas 78
attendu ports.gtk3.comportement.verts 78
attendu ports.gtk3.durcissement.garanties 8
attendu ports.gtk3.glade.etat chargé
attendu ports.gtk3.terminal.etat VTE
attendu ports.gtk3.echappement.cas 4
attendu ports.gtk3.clic.widgets 17
attendu ports.gtk3.exemples.joues 72
attendu ports.gtk3.exemples.ok 72
attendu ports.gtk3.exemples.non_joues.0 "wifi-scan (non joué : nmcli absent)"
attendu ports.gtk3.autres.identité OK
attendu ports.ncurses.glade.etat refusé
attendu ports.ncurses.mot_de_passe.verdict OK
attendu ports.html.terminal.etat "non promis"
attendu ports.qt6.xml.verdict ÉCHEC
attendu ports.qt6.xml.rouges 1
attendu ports.qt6.comportement.ecarts 1
attendu ports.qt6.comportement.verdict ÉCHEC
attendu ports.sdl3.xml.verdict absent
attendu ports.gtk4.comportement.verdict absent

# ── ci/bilan-md.sh : le tableau du bilan, depuis ce JSON ─────────────────────
MD="$BAC/BILAN.md"
printf '# Bilan de santé — essai\n\n## 2.0 — mesuré jadis\n\nTexte du mainteneur, à garder.\n' > "$MD"
bash "$RACINE/ci/bilan-md.sh" "$BAC/journaux/bilan.json" "$MD" >/dev/null || ko "bilan-md.sh rc=$? au premier passage"
grep -q '^\[//\]: # (bilan:debut)$' "$MD" && grep -q '^\[//\]: # (bilan:fin)$' "$MD" && ok "repères posés" || ko "repères absents"
grep -q '^| gtk3 (étalon) | ✅ 55/55 | ✅ 78/78 | ✅ 8/8 | ✅ chargé | ✅ VTE | ✅ 4/4 | ✅ 17/17 | ✅ 72/72 ; 2 non joué(s) faute d'\''outil (nmcli, udevadm) |$' "$MD" \
    && ok "ligne gtk3 du tableau" || { ko "ligne gtk3 du tableau"; grep '^| gtk3' "$MD"; }
grep -q '^| qt6 | ❌ 54/55 | ❌ 77/78 | — | — | — | — | — | — |$' "$MD" && ok "ligne qt6 : rouge, 54/55 et 77/78" || { ko "ligne qt6"; grep '^| qt6' "$MD"; }
grep -q '^| ncurses | ✅ 55/55 | ✅ 78/78 | — | ✅ refusé | — · mot de passe ✅ | — | — | — |$' "$MD" && ok "ligne ncurses : mot de passe" || { ko "ligne ncurses"; grep '^| ncurses' "$MD"; }
grep -q '^| sdl3 ' "$MD" && ko "sdl3 sans journal apparaît" || ok "sdl3 sans journal : pas de ligne"
grep -q 'bash ci/bancs.sh          # 13 bancs : 10 verts, 3 en échec' "$MD" && ok "compte des bancs" || ko "compte des bancs"
grep -q 'Texte du mainteneur, à garder.' "$MD" && grep -q '^## 2.0 — mesuré jadis$' "$MD" && ok "le texte du mainteneur reste" || ko "texte du mainteneur perdu"
avant=$(md5sum < "$MD"); bash "$RACINE/ci/bilan-md.sh" "$BAC/journaux/bilan.json" "$MD" | grep -q 'déjà à jour' && [ "$avant" = "$(md5sum < "$MD")" ] \
    && ok "second passage : identique, rien réécrit" || ko "second passage : fichier changé ou pas « déjà à jour »"
[ "$(grep -c 'bilan:debut' "$MD")" -eq 1 ] && ok "un seul repère de début" || ko "repères dupliqués"

# ── Sabotage : un chiffre changé dans un journal doit changer dans le JSON ────
sed -i 's/55 PASS/54 PASS/' "$J/gtk3_xml.log"
bash "$RACINE/ci/bilan-json.sh" "$J" abc1234 >/dev/null
attendu ports.gtk3.xml.verts 54
sed -i 's/78 cas · 78 au vert · 0 en écart/78 cas · 70 au vert · 8 en écart/' "$J/comportement__tous_les_ports_.log"
bash "$RACINE/ci/bilan-json.sh" "$J" abc1234 >/dev/null
attendu ports.gtk3.comportement.ecarts 8
attendu ports.ncurses.comportement.ecarts 8   # même chaîne sur la ligne ncurses : lue aussi
# Sans verdicts.tsv : les chiffres restent, les verdicts disent qu'ils ne sont pas enregistrés.
rm "$J/verdicts.tsv"
bash "$RACINE/ci/bilan-json.sh" "$J" abc1234 >/dev/null
attendu ports.gtk3.xml.verts 54
attendu ports.gtk3.xml.verdict "non enregistré"
# Sans journaux du tout : code 1.
bash "$RACINE/ci/bilan-json.sh" "$BAC/nulle-part" abc1234 >/dev/null 2>&1; rc=$?
[ "$rc" -eq 1 ] && ok "dossier absent : code 1" || ko "dossier absent : code $rc, attendu 1"

[ "$ecarts" -eq 0 ] && { echo "garde_bilan_json : OK — chiffres lus et sabotage vu"; exit 0; }
echo "garde_bilan_json : $ecarts écart(s)"; exit 1
