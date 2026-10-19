#!/bin/bash
# SPDX-License-Identifier: GPL-2.0-or-later
# tests/garde_heure_ntp.sh — haplo-dialog — 2026 — GPL-2.0-or-later
#
# examples/heure-ntp/ lit timedatectl et en tire un bilan. Ce test utilise un FAUX
# timedatectl (FAKE_TD choisit la situation) :
#   1 — synchronisée : valeurs justes, bilan OK
#   2 — écart trop grand (s, ms, us) : ATTENTION avec la valeur ; seuil HN_ECART_MAX
#   3 — non synchronisée ; NTP désactivé (prioritaire) ; horloge matérielle en heure locale
#   4 — chrony (pas de timesync-status) : bilan qui le dit
#   5 — réponses piégées (|, balises, $(…), contrôles, formats faux) : « ? » ou « — »,
#       2 colonnes, rien de piégé ; HN_ECART_MAX invalide : 64 ; timedatectl absent : 2
# Avec un binaire réel en argument (+ xvfb-run) : 6 — la fenêtre s'ouvre et reste ouverte.
# Usage : garde_heure_ntp.sh [/chemin/vers/un-port-a-fenetre]
RACINE=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
APP="$RACINE/examples/heure-ntp/heure-ntp.sh"
BIN_REEL="${1:-}"
for t in timeout mktemp python3; do
    command -v "$t" >/dev/null || { echo "outil manquant : $t" >&2; exit 77; }
done
[[ -x "$APP" ]] || { echo "script introuvable ou non exécutable : $APP" >&2; exit 1; }
T=$(mktemp -d); trap 'rm -rf "$T"' EXIT
mkdir "$T/faux"
cat > "$T/faux/timedatectl" <<'FAUX'
#!/bin/bash
m=${FAKE_TD:-ok}
case "$1" in
show)
    ntp=yes; sy=yes; loc=no; tz=Europe/Paris; rtc="Sun 2026-10-18 14:30:18 UTC"
    case $m in nonsync) sy=no ;; ntpoff) ntp=no; sy=no ;; rtclocal) loc=yes ;;
               piege) tz='Europe/Paris|<b>$(id)'; rtc=$'Sun 2026-10-18 \x1b[31m'; ntp='peut-être' ;; esac
    printf 'Timezone=%s\nLocalRTC=%s\nCanNTP=yes\nNTP=%s\nNTPSynchronized=%s\nRTCTimeUSec=%s\n' "$tz" "$loc" "$ntp" "$sy" "$rtc" ;;
timesync-status)
    [ "$m" = chrony ] && { echo "Failed to query server: Could not activate remote peer" >&2; exit 1; }
    off=-27.881ms; srv="79.143.250.33 (2.debian.pool.ntp.org)"; st=1; iv="34min 8s (min: 32s; max 34min 8s)"
    case $m in ecart_s) off=+1.250s ;; ecart_ms) off=-750.5ms ;; ecart_us) off=900us ;;
               piege) off='12ms|x'; srv='1.2.3.4 (<b>&amp;|$(id))'; st='1;id'; iv=$'34min | x\x07' ;; esac
    printf '       Server: %s\nPoll interval: %s\n      Stratum: %s\n       Offset: %s\n        Delay: 39.663ms\n       Jitter: 38.704ms\n' "$srv" "$iv" "$st" "$off" ;;
esac
FAUX
chmod +x "$T/faux/timedatectl"
export PATH="$T/faux:$PATH"
echec() { echo "ECHEC : $*"; exit 1; }
v() { grep -m1 "^$1|" "$T/e" | cut -d'|' -f2-; }
run() { FAKE_TD=$1 "$APP" --etat > "$T/e" || echec "$1 : code non nul"; }

# ── 1 ───────────────────────────────────────────────────────────────────────
run ok
[[ $(v Fuseau) == Europe/Paris && $(v "NTP activé") == oui && $(v Synchronisée) == oui ]] || echec "ok : $(tr '\n' ' ' < "$T/e")"
[[ $(v Serveur) == "2.debian.pool.ntp.org (79.143.250.33)" && $(v Strate) == 1 && $(v "Écart mesuré") == -27.881ms ]] || echec "ok : serveur/strate/écart"
[[ $(v "Horloge matérielle") == "Sun 2026-10-18 14:30:18 UTC" ]] || echec "ok : RTC $(v "Horloge matérielle")"
[[ $(v Bilan) == "OK : horloge synchronisée" ]] || echec "ok : bilan $(v Bilan)"
[[ $(wc -l < "$T/e") == 13 ]] || echec "ok : 13 lignes attendues"

# ── 2 ───────────────────────────────────────────────────────────────────────
run ecart_s;  [[ $(v Bilan) == "ATTENTION : écart de 1250 ms (toléré : 500 ms)" ]] || echec "écart en s : $(v Bilan)"
run ecart_ms; [[ $(v Bilan) == "ATTENTION : écart de 750 ms (toléré : 500 ms)" ]] || echec "écart en ms : $(v Bilan)"
run ecart_us; [[ $(v Bilan) == "OK : horloge synchronisée" ]] || echec "écart en µs : $(v Bilan)"
HN_ECART_MAX=20 FAKE_TD=ok "$APP" --etat > "$T/e"; [[ $(v Bilan) == "ATTENTION : écart de 27 ms (toléré : 20 ms)" ]] || echec "seuil HN_ECART_MAX : $(v Bilan)"

# ── 3 ───────────────────────────────────────────────────────────────────────
run nonsync;  [[ $(v Bilan) == "ATTENTION : l'horloge n'est pas synchronisée" ]] || echec "non synchronisée : $(v Bilan)"
run ntpoff;   [[ $(v Bilan) == "ATTENTION : NTP n'est pas activé"* ]] || echec "NTP désactivé : $(v Bilan)"
run rtclocal; [[ $(v "Horloge matérielle") == *"en heure LOCALE"* ]] || echec "RTC locale non signalée"

# ── 4 ───────────────────────────────────────────────────────────────────────
run chrony
[[ $(v Bilan) == "Synchronisée (détails : seulement avec systemd-timesyncd"* ]] || echec "chrony : $(v Bilan)"
[[ $(v Serveur) == "—" && $(v "Écart mesuré") == "—" ]] || echec "chrony : détails inventés : $(v Serveur) / $(v "Écart mesuré")"

# ── 5 ───────────────────────────────────────────────────────────────────────
run piege
[[ $(awk -F'|' '{print NF}' "$T/e" | sort -u) == 2 ]] || echec "piège : le tableau n'a plus 2 colonnes : $(cat "$T/e")"
! grep -qE '<b>|\$\(|id\)|x$' "$T/e" || echec "piège : une valeur piégée est sortie : $(cat "$T/e")"
! LC_ALL=C grep -q $'[\x01-\x1f\x7f]' "$T/e" || echec "piège : caractères de contrôle"
[[ $(v Fuseau) == "?" && $(v "NTP activé") == "?" && $(v "Horloge matérielle") == "?" ]] || echec "piège : show non rejeté : $(head -n 6 "$T/e" | tr '\n' ' ')"
[[ $(v Serveur) == "— (1.2.3.4)" && $(v Strate) == "—" && $(v "Écart mesuré") == "—" && $(v "Intervalle d'interrogation") == "—" ]] \
    || echec "piège : timesync-status non rejeté : $(sed -n '7,12p' "$T/e" | tr '\n' ' ')"
[[ $(v Bilan) == "ATTENTION : NTP n'est pas activé"* ]] || echec "piège : NTP « peut-être » ne doit pas valoir oui"
HN_ECART_MAX=abc "$APP" --etat >/dev/null 2>&1; [[ $? == 64 ]] || echec "HN_ECART_MAX invalide accepté"
mkdir "$T/maigre"; for c in bash sed date awk cut tr head timeout cat; do ln -s "$(PATH=${PATH#*:} command -v $c)" "$T/maigre/$c"; done
env PATH="$T/maigre" "$APP" --etat >/dev/null 2>&1; [[ $? == 2 ]] || echec "timedatectl absent : code ≠ 2"
"$APP" --xml | python3 -I -c 'import sys,xml.dom.minidom as m; m.parseString(sys.stdin.read())' || echec "XML mal formé"

# ── 6 — binaire réel (facultatif) ──────────────────────────────────────────
if [[ -n "$BIN_REEL" ]]; then
    [[ -x "$BIN_REEL" ]] && command -v xvfb-run >/dev/null || { echo "binaire ou xvfb-run absent : étape 6 non faite" >&2; exit 77; }
    timeout -k 3 10 xvfb-run -a "$APP" "$BIN_REEL" > "$T/gui" 2>&1
    rc=$?
    [[ $rc == 124 ]] || echec "binaire réel : la fenêtre ne reste pas ouverte (rc=$rc) : $(tail -n 3 "$T/gui")"
    ! grep -qiE 'syntax error|parse error|unknown tag|erreur de syntaxe' "$T/gui" || echec "binaire réel : XML refusé"
    suite=" ; fenêtre réelle ouverte"
fi
echo "OK : bilans justes (OK, écart en s/ms/µs, non synchronisée, NTP désactivé, RTC locale, chrony), réponses piégées rejetées${suite:-}"
