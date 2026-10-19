#!/bin/bash
# SPDX-License-Identifier: CC0-1.0
# Métier   : Radioamateur, topographie, maintenance de récepteurs GNSS, embarqué
# Besoin   : voir l'état d'un récepteur GNSS/GPS (fixation, position, qualité,
#            satellites) sans logiciel dédié, et sans rien envoyer hors machine
# Enrobe   : gpspipe (paquet gpsd-clients), qui lit le service gpsd
# Ports    : tout port à fenêtre ; ncursessermo par SSH
# Terminal : rien de perdu
# Usage    : ./gnss-moniteur.sh [binaire sermo]
#            ./gnss-moniteur.sh --demo FICHIER [binaire sermo]   (rejoue un enregistrement
#                                                  de « gpspipe -w », sans gpsd ni récepteur)
#            GNSS_PERIODE : secondes entre deux lectures, 1 à 10 (défaut 2)
#
# Codes de sortie : 0 fermé normalement · 2 gpspipe, python3 ou sermo absent · 64 usage.
# Confidentialité : la position est une donnée personnelle. Rien ne part hors de
# la machine, sauf si l'on clique sur « Ouvrir la carte » (navigateur, OpenStreetMap).
# Le JSON reçu est une donnée : lu par python3 -I, seuls des nombres et une heure
# au format strict en sont gardés, jamais insérés dans le XML.

export SERMO="${SERMO:-sermo}"
export SERMO_SCRIPT="$0"
export GNSS_PERIODE="${GNSS_PERIODE:-2}"

# Lit du JSON gpsd sur l'entrée, garde le dernier TPV et le dernier SKY, écrit
# deux fichiers : etat (CLÉ=valeur, valeurs déjà mises en forme) et sats (lignes
# du <tree>). Toute valeur absente, de mauvais type ou hors bornes devient « — ».
PY_ANALYSE='
import json, math, re, sys
etat_f, sats_f = sys.argv[1], sys.argv[2]
tpv = sky = None
for ligne in sys.stdin:
    if len(ligne) > 65536:
        continue
    try:
        o = json.loads(ligne)
    except ValueError:
        continue
    if not isinstance(o, dict):
        continue
    if o.get("class") == "TPV":
        tpv = o
    elif o.get("class") == "SKY" and isinstance(o.get("satellites"), list):
        sky = o

def nombre(d, cle, mini, maxi):
    v = d.get(cle) if isinstance(d, dict) else None
    if isinstance(v, bool) or not isinstance(v, (int, float)):
        return None
    if not math.isfinite(v) or not (mini <= v <= maxi):
        return None
    return float(v)

def f(v, fmt):
    return "—" if v is None else fmt % v

t = tpv or {}
mode = nombre(t, "mode", 0, 3)
fix = {None: "inconnu", 0: "inconnu", 1: "aucune", 2: "2D", 3: "3D"}[None if mode is None else int(mode)]
lat = nombre(t, "lat", -90, 90)
lon = nombre(t, "lon", -180, 180)
alt = nombre(t, "altHAE", -1000, 100000)
if alt is None:
    alt = nombre(t, "alt", -1000, 100000)
vit = nombre(t, "speed", 0, 20000)
cap = nombre(t, "track", 0, 360)
heure = t.get("time")
if not (isinstance(heure, str) and re.fullmatch(r"\d{4}-\d\d-\d\dT\d\d:\d\d:\d\d(\.\d{1,9})?Z", heure)):
    heure = None
if mode is None or mode < 2:
    lat = lon = vit = cap = None
if mode is None or mode < 3:
    alt = None

CONST = {0: "GPS", 1: "SBAS", 2: "Galileo", 3: "BeiDou", 4: "IMES", 5: "QZSS", 6: "GLONASS", 7: "NavIC"}
lignes, vus, utilises = [], 0, 0
for s in (sky or {}).get("satellites", [])[:64]:
    if not isinstance(s, dict):
        continue
    prn = nombre(s, "PRN", 1, 1000)
    if prn is None:
        continue
    g = nombre(s, "gnssid", 0, 7)
    u = s.get("used") is True
    vus += 1
    utilises += u
    lignes.append("%s|%d|%s|%s|%s|%s" % (
        CONST.get(None if g is None else int(g), "—"), int(prn),
        f(nombre(s, "el", -90, 90), "%.0f°"), f(nombre(s, "az", 0, 360), "%.0f°"),
        f(nombre(s, "ss", 0, 99), "%.0f dB-Hz"), "oui" if u else "non"))

k = sky or {}
with open(etat_f, "w", encoding="utf-8") as e:
    e.write("FIX=%s\n" % fix)
    e.write("LAT=%s\n" % f(lat, "%.6f"))
    e.write("LON=%s\n" % f(lon, "%.6f"))
    e.write("ALT=%s\n" % f(alt, "%.1f m"))
    e.write("VITESSE=%s\n" % f(None if vit is None else vit * 3.6, "%.1f km/h"))
    e.write("CAP=%s\n" % f(cap, "%.0f°"))
    e.write("HEURE=%s\n" % (heure or "—"))
    e.write("DOP=%s / %s\n" % (f(nombre(k, "hdop", 0, 99.99), "%.2f"), f(nombre(k, "pdop", 0, 99.99), "%.2f")))
    e.write("SATS=%d vus, %d utilisés\n" % (vus, utilises))
with open(sats_f, "w", encoding="utf-8") as e:
    e.write("".join(l + "\n" for l in lignes))
'

# Une lecture : gpspipe (ou la tranche suivante de l'enregistrement en mode démo)
# → analyse → remplacement atomique des deux fichiers du cache.
lire() {
	local src
	if [ -n "${GNSS_DEMO:-}" ]; then
		# 40 lignes par tour, en boucle sur le fichier : la démo bouge comme un vrai flux.
		local total pos
		total=$(wc -l < "$GNSS_DEMO"); [ "$total" -gt 0 ] || total=1
		pos=$(cat "$GNSS_CACHE/pos" 2>/dev/null); [[ "$pos" =~ ^[0-9]+$ ]] || pos=0
		src=$( { tail -n "+$((pos % total + 1))" -- "$GNSS_DEMO"; cat -- "$GNSS_DEMO"; } | head -n 40)
		echo $(( (pos + 40) % total )) > "$GNSS_CACHE/pos"
	else
		src=$(LC_ALL=C timeout 5 gpspipe -w -n 15 2>/dev/null)
	fi
	if printf '%s\n' "$src" | head -c 1048576 |
		python3 -I -c "$PY_ANALYSE" "$GNSS_CACHE/etat.n" "$GNSS_CACHE/sats.n" 2>/dev/null; then
		[ -n "$src" ] || echo "FIX=pas de réponse de gpsd" > "$GNSS_CACHE/etat.n"
		mv -f "$GNSS_CACHE/sats.n" "$GNSS_CACHE/sats"; mv -f "$GNSS_CACHE/etat.n" "$GNSS_CACHE/etat"
	fi
}

# Le minuteur attend la fin de son action : gpspipe peut attendre plusieurs
# secondes. On lit donc en fond, une lecture à la fois (comme examples/modbus).
lire_en_fond() {
	mkdir -- "$GNSS_CACHE/verrou" 2>/dev/null || return 0
	( trap 'rmdir -- "$GNSS_CACHE/verrou"' EXIT; lire ) </dev/null >/dev/null 2>&1 &
}

champ() {   # champ CLÉ : la valeur du cache, ou « — »
	local v
	[[ "${1:-}" =~ ^[A-Z]{2,8}$ ]] || { echo "—"; return 64; }
	v=$(sed -n "s/^$1=//p" "$GNSS_CACHE/etat" 2>/dev/null | head -n 1)
	printf '%s\n' "${v:-—}"
}

position() {   # « lat,lon » si les deux sont des nombres, rien sinon
	local la lo
	la=$(champ LAT); lo=$(champ LON)
	[[ "$la" =~ ^-?[0-9]{1,2}\.[0-9]{6}$ && "$lo" =~ ^-?[0-9]{1,3}\.[0-9]{6}$ ]] || return 1
	printf '%s,%s\n' "$la" "$lo"
}

copier() {
	local p; p=$(position) || { echo "pas de position à copier" >&2; return 1; }
	if command -v wl-copy >/dev/null && [ -n "${WAYLAND_DISPLAY:-}" ]; then printf '%s' "$p" | wl-copy
	elif command -v xclip >/dev/null; then printf '%s' "$p" | xclip -selection clipboard
	elif command -v xsel >/dev/null; then printf '%s' "$p" | xsel --clipboard --input
	else echo "aucun outil de presse-papiers (wl-copy, xclip, xsel)" >&2; return 2; fi
}

carte() {   # seulement sur clic : c'est le seul moment où la position quitte la machine
	local p la lo; p=$(position) || { echo "pas de position" >&2; return 1; }
	la=${p%,*}; lo=${p#*,}
	command -v xdg-open >/dev/null || { echo "xdg-open absent" >&2; return 2; }
	xdg-open "https://www.openstreetmap.org/?mlat=$la&mlon=$lo#map=16/$la/$lo" >/dev/null 2>&1 &
}

presse_papiers() { command -v wl-copy >/dev/null || command -v xclip >/dev/null || command -v xsel >/dev/null; }

dialogue_principal() {
	local ligne cle lib
	cat <<XML
<window title="Moniteur GNSS$( [ -n "${GNSS_DEMO:-}" ] && printf ' (démo)')" default-width="620" default-height="560">
  <vbox spacing="6">
    <frame label="Position">
      <grid columns="2" row-spacing="4" column-spacing="12">
$(for ligne in FIX:Fixation LAT:Latitude LON:Longitude ALT:Altitude VITESSE:Vitesse CAP:Cap HEURE:"Heure UTC" DOP:"HDOP / PDOP" SATS:Satellites; do
	cle=${ligne%%:*}; lib=${ligne#*:}
	printf '        <text xalign="0"><label>%s</label></text>\n' "$lib"
	printf '        <text xalign="0"><variable>V_%s</variable><input>exec "$SERMO_SCRIPT" --champ %s</input></text>\n' "$cle" "$cle"
done)
      </grid>
    </frame>
    <frame label="Satellites">
      <tree space-expand="true" space-fill="true">
        <label>Constellation|PRN|Élévation|Azimut|Signal|Utilisé</label>
        <variable>SATELLITES</variable>
        <height>200</height>
        <input>cat "\$GNSS_CACHE/sats"</input>
      </tree>
    </frame>
    <timer interval="$GNSS_PERIODE" visible="false">
      <variable>HORLOGE</variable>
      <action>exec "\$SERMO_SCRIPT" --lire-en-fond</action>
$(for cle in FIX LAT LON ALT VITESSE CAP HEURE DOP SATS; do printf '      <action>refresh:V_%s</action>\n' "$cle"; done)
      <action>refresh:SATELLITES</action>
    </timer>
    <text><label>La position reste sur cette machine. « Ouvrir la carte » l'envoie à OpenStreetMap, seulement quand vous cliquez.</label></text>
    <hbox>
$(presse_papiers && printf '      <button><label>Copier la position</label><action>exec "$SERMO_SCRIPT" --copier</action></button>\n')
      <button><label>Ouvrir la carte</label><action>exec "\$SERMO_SCRIPT" --carte</action></button>
      <button><label>Fermer</label><action>exit:fermer</action></button>
    </hbox>
  </vbox>
</window>
XML
}

usage() { sed -n '/^# Usage/,/^# Codes/p' "$0" | sed '$d;s/^# \{0,1\}//' >&2; exit 64; }

main() {
	case "${1:-}" in
	--lire)         lire; exit ;;
	--lire-en-fond) lire_en_fond; exit ;;
	--champ)        champ "${2:-}"; exit ;;
	--copier)       copier; exit ;;
	--carte)        carte; exit ;;
	--demo)         [ -n "${2:-}" ] || usage
	                [ -f "$2" ] && [ -r "$2" ] || { echo "fichier de démo illisible : $2" >&2; exit 64; }
	                GNSS_DEMO=$(readlink -f -- "$2"); export GNSS_DEMO; shift 2 ;;
	--xml)          ;;
	-*)             usage ;;
	esac
	[[ "$GNSS_PERIODE" =~ ^([1-9]|10)$ ]] || { echo "GNSS_PERIODE invalide (1 à 10 s) : $GNSS_PERIODE" >&2; exit 64; }
	command -v python3 >/dev/null || { echo "python3 est absent : sudo apt install python3" >&2; exit 2; }
	if [ -z "${GNSS_DEMO:-}" ] && [ "${1:-}" != --xml ]; then
		command -v gpspipe >/dev/null || {
			echo "gpspipe est absent : sudo apt install gpsd gpsd-clients (ou essayez --demo FICHIER)" >&2; exit 2; }
	fi
	local dossier=${XDG_RUNTIME_DIR:-${TMPDIR:-/tmp}}
	umask 077
	export GNSS_CACHE
	GNSS_CACHE=$(mktemp -d "$dossier/sermo-gnss.XXXXXX") || exit 2
	trap 'rm -rf -- "$GNSS_CACHE"' EXIT
	trap 'exit 129' HUP; trap 'exit 130' INT; trap 'exit 143' TERM
	: > "$GNSS_CACHE/sats"
	if [ "${1:-}" = --xml ]; then dialogue_principal; exit 0; fi
	[ -n "${1:-}" ] && SERMO=$1
	command -v "$SERMO" >/dev/null || { echo "sermo introuvable : $SERMO" >&2; exit 2; }
	lire
	export DIALOGUE; DIALOGUE=$(dialogue_principal)
	"$SERMO" --program=DIALOGUE >/dev/null
	exit 0
}

[ "${BASH_SOURCE[0]}" = "$0" ] && main "$@"
