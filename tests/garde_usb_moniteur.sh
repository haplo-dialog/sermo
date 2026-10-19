#!/bin/bash
# SPDX-License-Identifier: GPL-2.0-or-later
# tests/garde_usb_moniteur.sh — haplo-dialog — 2026 — GPL-2.0-or-later
#
# examples/usb-moniteur/ lit /sys et écoute udevadm. Ce test utilise un FAUX /sys
# (USB_SYSFS) et un FAUX udevadm : le matériel du poste n'est ni lu ni montré.
#   1 — inventaire : appareils seulement (ni racines ni interfaces), ID invalide écarté,
#       texte piégé neutralisé, appareil COINCÉ (lecture qui bloque) rendu « — » en
#       quelques secondes au lieu de figer, numéro de série jamais lu
#   2 — événements : format strict, appareils seulement, plus récent en haut
#   3 — aucune écoute ne survit : --fermer, puis maître tué par SIGKILL ; plafond 2 Mio
#   4 — XML bien formé ; udevadm absent : 2
# Avec un binaire réel en argument (+ xvfb-run) : 5 — la fenêtre s'ouvre et reste ouverte.
# Usage : garde_usb_moniteur.sh [/chemin/vers/un-port-a-fenetre]
RACINE=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
APP="$RACINE/examples/usb-moniteur/usb-moniteur.sh"
BIN_REEL="${1:-}"
for t in python3 timeout mktemp mkfifo; do
    command -v "$t" >/dev/null || { echo "outil manquant : $t" >&2; exit 77; }
done
[[ -x "$APP" ]] || { echo "script introuvable ou non exécutable : $APP" >&2; exit 1; }

T=$(mktemp -d)
nettoyer() { "$APP" --fermer >/dev/null 2>&1; rm -rf "$T"; }
trap nettoyer EXIT
export XDG_RUNTIME_DIR="$T/run"; mkdir -m 700 "$XDG_RUNTIME_DIR"
export APPELS="$T/appels"; : > "$APPELS"
echec() { echo "ECHEC : $*"; exit 1; }
attendre() { local i; for i in $(seq 1 30); do eval "$1" && return 0; timeout 0.1 tail -f /dev/null; done; return 1; }

# ── Faux /sys ───────────────────────────────────────────────────────────────
S="$T/sys"; mkdir -p "$S"
appareil() {  # appareil <nom> <vendor> <product> <fabricant> <produit> <classe>
    mkdir -p "$S/$1"; printf '%s\n' "$2" > "$S/$1/idVendor"; printf '%s\n' "$3" > "$S/$1/idProduct"
    [ -n "$4" ] && printf '%s\n' "$4" > "$S/$1/manufacturer"; printf '%s\n' "$5" > "$S/$1/product"
    printf '%s\n' "$6" > "$S/$1/bDeviceClass"; printf 'SECRET-SERIE\n' > "$S/$1/serial"
}
appareil usb1 1d6b 0002 Linux "xHCI Host Controller" 09
appareil 1-2 046d c077 Logitech "USB Optical Mouse" 00
appareil 1-2:1.0 046d c077 Logitech "interface" 03
appareil 1-3.1 0763 410c M-Audio "AIR 192 6" ef
appareil 1-4 0b05 19af "$(printf 'Pirate <b>&amp;|\033[31m\x07')" "$(printf 'x%.0s' $(seq 1 300))" ff
appareil 1-5 zzzz 0001 Faux "ID invalide" 00
appareil 2-1 0af0 6971 "" "" 08
rm -f "$S/2-1/product"; mkfifo "$S/2-1/product"              # appareil coincé : la lecture bloque
export USB_SYSFS="$S"

# ── Faux udevadm : journalise, rend des lignes (valides, piégées), puis attend.
mkdir "$T/faux"
cat > "$T/faux/udevadm" <<'FAUX'
#!/bin/bash
printf 'udevadm %s\n' "$*" >> "$APPELS"
cat <<'L'
monitor will print the received events for:
KERNEL - the kernel uevent

KERNEL[100.000001] add      /devices/pci0000:00/0000:00:14.0/usb1/1-2 (usb)
KERNEL[100.000002] add      /devices/pci0000:00/0000:00:14.0/usb1/1-2/1-2:1.0 (usb)
KERNEL[100.000003] bind     /devices/pci0000:00/0000:00:14.0/usb1/1-2 (usb)
KERNEL[205.500000] remove   /devices/pci0000:00/0000:00:14.0/usb1/1-3.1 (usb)
KERNEL[206.000000] add      /devices/<b>/1-9 (usb)
KERNEL[207.000000] explose  /devices/usb1/1-7 (usb)
KERNEL[x] add /devices/usb1/1-8 (usb)
UDEV  [208.000000] add      /devices/usb1/1-6 (usb)
L
exec sleep 300
FAUX
chmod +x "$T/faux/udevadm"
export PATH="$T/faux:$PATH"
export UM_DIR="$T/um"; mkdir -m 700 "$UM_DIR"; : > "$UM_DIR/journal"
export UM_MAITRE=$$

# ── 1 — inventaire ──────────────────────────────────────────────────────────
debut=$(date +%s)
"$APP" --inventaire > "$T/inv" || echec "--inventaire : code non nul"
duree=$(( $(date +%s) - debut ))
(( duree <= 6 )) || echec "appareil coincé : l'inventaire a pris $duree s"
cut -d'|' -f1 "$T/inv" | tr '\n' ' ' | grep -qx '1-2 1-3.1 1-4 2-1 ' || echec "appareils listés : $(cut -d'|' -f1 "$T/inv" | tr '\n' ' ')"
grep -qx '1-2|046d:c077|Logitech|USB Optical Mouse|par interface' "$T/inv" || echec "souris mal rendue : $(grep '^1-2|' "$T/inv")"
grep -qx '1-3.1|0763:410c|M-Audio|AIR 192 6|divers' "$T/inv" || echec "carte son mal rendue"
grep -qx '2-1|0af0:6971|—|—|stockage' "$T/inv" || echec "appareil coincé mal rendu : $(grep '^2-1|' "$T/inv")"
L4=$(grep '^1-4|' "$T/inv")
[[ "$L4" == '1-4|0b05:19af|Pirate <b>&amp;¦[31m|'*'|spécifique fabricant' ]] || echec "texte piégé mal rendu : $L4"
(( $(cut -d'|' -f4 <<<"$L4" | wc -m) <= 81 )) || echec "produit non plafonné à 80 caractères"
[[ $(awk -F'|' '{print NF}' "$T/inv" | sort -u) == 5 ]] || echec "une ligne n'a pas 5 colonnes"
! LC_ALL=C grep -q $'[\x01-\x1f\x7f]' "$T/inv" || echec "caractères de contrôle restés"
! grep -q SECRET-SERIE "$T/inv" || echec "le numéro de série est affiché"

# ── 2 — événements ──────────────────────────────────────────────────────────
"$APP" --ouvrir || echec "--ouvrir : code non nul"
attendre '[[ -s "$UM_DIR/lecteur" ]] && [[ $(wc -l < "$UM_DIR/journal") -ge 10 ]]' || echec "udevadm non lancé ou rien lu"
grep -qx 'udevadm monitor --kernel --subsystem-match=usb' "$APPELS" || echec "arguments d'udevadm : $(cat "$APPELS")"
"$APP" --evenements > "$T/ev"
diff "$T/ev" <(printf '%s\n' '205.500000|débranché|1-3.1' '100.000003|bind|1-2' '100.000001|branché|1-2') >/dev/null \
    || echec "événements : $(tr '\n' ' ' < "$T/ev")"
LECTEUR=$(cat "$UM_DIR/lecteur")

# ── 3 — survie, plafond ─────────────────────────────────────────────────────
"$APP" --fermer
attendre '! kill -0 "$LECTEUR" 2>/dev/null' || echec "--fermer : udevadm toujours vivant"
bash -c 'export UM_MAITRE=$$; "$1" --ouvrir; exec sleep 60' _ "$APP" & MAITRE=$!; disown
attendre '[[ -s "$UM_DIR/lecteur" && $(cat "$UM_DIR/lecteur") != "$LECTEUR" ]]' || echec "second suivi non démarré"
L2=$(cat "$UM_DIR/lecteur")
kill -9 "$MAITRE"
attendre '! kill -0 "$L2" 2>/dev/null' || echec "maître tué par SIGKILL : udevadm orphelin ($L2)"
"$APP" --ouvrir
attendre '[[ -s "$UM_DIR/lecteur" ]] && kill -0 "$(cat "$UM_DIR/lecteur")" 2>/dev/null' || echec "suivi (plafond) non démarré"
L3=$(cat "$UM_DIR/lecteur")
head -c 2200000 /dev/zero | tr '\0' 'x' >> "$UM_DIR/journal"
attendre '! kill -0 "$L3" 2>/dev/null' || echec "plafond : la lecture continue au-delà de 2 Mio"
"$APP" --evenements | grep -q '^—|journal plein|' || echec "plafond : pas de message « journal plein »"
"$APP" --fermer

# ── 4 — XML, codes ──────────────────────────────────────────────────────────
"$APP" --xml | python3 -I -c 'import sys,xml.dom.minidom as m; d=m.parseString(sys.stdin.read()); assert len(d.getElementsByTagName("timer"))==2' \
    || echec "XML mal formé"
mkdir "$T/maigre"
for c in bash cat mktemp rm sed dirname readlink; do ln -s "$(PATH=${PATH#*:} command -v $c)" "$T/maigre/$c"; done
env PATH="$T/maigre" "$APP" --fermer >/dev/null 2>&1; [[ $? == 2 ]] || echec "udevadm absent : code ≠ 2"
"$APP" --inconnu >/dev/null 2>&1; [[ $? == 64 ]] || echec "option inconnue : code ≠ 64"
( unset UM_DIR; "$APP" --evenements >/dev/null )
[[ -z "$(find "$XDG_RUNTIME_DIR" -name 'sermo-usb.*')" ]] || echec "un dossier temporaire survit"

# ── 5 — binaire réel (facultatif) ──────────────────────────────────────────
if [[ -n "$BIN_REEL" ]]; then
    [[ -x "$BIN_REEL" ]] && command -v xvfb-run >/dev/null || { echo "binaire ou xvfb-run absent : étape 5 non faite" >&2; exit 77; }
    ( unset UM_DIR; timeout -k 3 12 xvfb-run -a "$APP" "$BIN_REEL" ) > "$T/gui" 2>&1
    rc=$?
    [[ $rc == 124 ]] || echec "binaire réel : la fenêtre ne reste pas ouverte (rc=$rc) : $(tail -n 3 "$T/gui")"
    ! grep -qiE 'syntax error|parse error|unknown tag|erreur de syntaxe' "$T/gui" || echec "binaire réel : XML refusé"
    suite=" ; fenêtre réelle ouverte"
fi
echo "OK : inventaire (appareils seulement, texte piégé neutralisé, appareil coincé sans blocage, série jamais lue), événements stricts, pas d'orphelin même après SIGKILL, journal plafonné${suite:-}"
