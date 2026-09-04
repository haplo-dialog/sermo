#!/bin/sh
# minuterie.sh — le CONTRAT de <timer>, que le banc de valeurs ne voit pas :
# il compare des variables, or une minuterie ne produit que du TEMPS.
#
# Contrat de l'oracle (gtk3sermo, widget_timer.c) :
#   - interval passe par atoi : SECONDES ENTIÈRES, interval="1.9" vaut 1 ;
#   - milliseconds est un BOOLÉEN : levé, interval compte en millisecondes.
# Deux régressions vécues (2026-09-04, binaires installés) : milliseconds lu
# comme une VALEUR (strtod("true")=0 → départ immédiat), et décimales honorées
# (le même script ne durait pas pareil selon le port).
#
# Méthode COMPARATIVE : on mesure des paires sur le MÊME binaire et on borne
# leurs ÉCARTS — le coût de démarrage s'annule, le banc tient sur machine lente.
set -u
BIN="${1:?usage: minuterie.sh /chemin/du/binaire}"
[ -x "$BIN" ] || { echo "binaire introuvable : $BIN" >&2; exit 2; }
command -v xvfb-run >/dev/null || { echo "outil manquant : xvfb-run" >&2; exit 2; }

duree() { # $1 = XML ; imprime la durée en ms
    _d0=$(date +%s%N)
    MAIN_DIALOG="$1" xvfb-run -a "$BIN" --program=MAIN_DIALOG >/dev/null 2>&1
    _d1=$(date +%s%N)
    echo $(( (_d1 - _d0) / 1000000 ))
}
gabarit() { printf '<window title="t"><vbox><text><label>x</label></text><timer %s><action>EXIT:ok</action></timer></vbox></window>' "$1"; }

echec=0
# 1) milliseconds booléen : 400 ms doit finir NETTEMENT avant 1 s (défaut).
d_ms=$(duree "$(gabarit 'interval="400" milliseconds="true"')")
d_1s=$(duree "$(gabarit 'interval="1"')")
# ⚠️ Deux bornes, pas une : la régression vécue (strtod("true")=0) partait
# IMMÉDIATEMENT — elle passait haut la main un simple « nettement avant 1 s ».
# La durée mesurée contient démarrage + minuterie : elle ne peut qu'EXCÉDER
# les 400 ms demandées — une durée sous 350 ms prouve que le délai est ignoré.
if [ "$d_ms" -lt 350 ]; then
    echo "FAIL  minuterie partie trop tôt : 400 ms demandées, fini en $d_ms ms"; echec=1
elif [ $(( d_1s - d_ms )) -lt 200 ]; then
    echo "FAIL  milliseconds ignoré ou mal lu : 400ms=$d_ms, 1s=$d_1s (écart < 200 ms)"; echec=1
else
    echo "ok    milliseconds booléen : 400 ≤ $d_ms ms < 1s=$d_1s"
fi
# 2) troncature atoi : interval="1.9" doit durer comme interval="1", pas 900 ms de plus.
d_19=$(duree "$(gabarit 'interval="1.9"')")
d_1b=$(duree "$(gabarit 'interval="1"')")
ecart=$(( d_19 - d_1b )); [ "$ecart" -lt 0 ] && ecart=$(( -ecart ))
if [ "$ecart" -gt 500 ]; then
    echo "FAIL  interval décimal non tronqué comme l'oracle : 1.9=$d_19 vs 1=$d_1b (écart $ecart ms)"; echec=1
else
    echo "ok    troncature atoi : 1.9=$d_19 ≈ 1=$d_1b (écart $ecart ms)"
fi
[ "$echec" -eq 0 ] && echo "SUCCES — contrat <timer> conforme à l'oracle" || echo "ECHEC — contrat <timer> violé"
exit "$echec"
