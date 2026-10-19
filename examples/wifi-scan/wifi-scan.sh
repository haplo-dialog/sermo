#!/bin/bash
# SPDX-License-Identifier: CC0-1.0
# Métier   : Support, installation réseau, dépannage à domicile ou en entreprise
# Besoin   : voir les réseaux Wi-Fi à portée (signal, canal, bande, sécurité), repérer
#            le canal encombré ou le réseau mal protégé — sans rien changer
# Enrobe   : nmcli device wifi list (NetworkManager) ; lecture seule
# Ports    : tout port à fenêtre (tree, button, text)
# Terminal : ncursessermo par SSH : rien de perdu
# Usage    : ./wifi-scan.sh [binaire sermo]
#            ./wifi-scan.sh --liste [relancer]   (utilisé|SSID|BSSID|canal|bande|signal|sécurité)
#            ./wifi-scan.sh --canaux             (bande|canal|réseaux|signal max)
#
# Codes de sortie : 0 fait · 2 nmcli absent · 64 usage.
# Confidentialité : les noms des réseaux voisins sont des données personnelles ; rien
# n'est enregistré ni envoyé. Les SSID sont des données arbitraires (32 octets
# quelconques) : nettoyés, « | » remplacé, jamais insérés dans le XML.
# Rien n'est modifié : aucune connexion, aucun mot de passe demandé ni lu.

export SERMO="${SERMO:-sermo}"
export SERMO_SCRIPT="$0"

# nmcli -t échappe « : » et « \ » par « \ » ; python3 -I découpe, contrôle chaque
# champ, et rend des lignes sûres pour le <tree>.
PY_LISTE='
import sys
mode = sys.argv[1]
def champs(ligne):
    out, cur, i = [], "", 0
    while i < len(ligne):
        c = ligne[i]
        if c == "\\" and i + 1 < len(ligne):
            cur += ligne[i + 1]; i += 2; continue
        if c == ":":
            out.append(cur); cur = ""
        else:
            cur += c
        i += 1
    out.append(cur)
    return out
def propre(s, n):
    s = "".join(ch if ch >= " " and ch != "\x7f" else "?" for ch in s).replace("|", "¦").strip()
    return s[:n]
reseaux = []
for brut in sys.stdin.buffer.read().split(b"\n")[:500]:
    ligne = brut.decode("utf-8", "replace")
    f = champs(ligne)
    if len(f) != 7:
        continue
    utilise, bssid, ssid, chan, freq, sig, secu = f
    if not (len(bssid) == 17 and all(c in "0123456789ABCDEFabcdef:" for c in bssid)):
        continue
    if not (chan.isdigit() and sig.isdigit() and int(sig) <= 100):
        continue
    mhz = freq.split()[0] if freq.split() else ""
    if not mhz.isdigit():
        continue
    m = int(mhz)
    bande = "2,4 GHz" if m < 3000 else "5 GHz" if m < 5935 else "6 GHz"
    s = propre(secu, 40) or "aucune"
    if s in ("--", ""):
        s = "aucune"
    alerte = " ⚠ ouvert" if s == "aucune" else " ⚠ faible" if ("WEP" in s or (s.startswith("WPA1") and "WPA2" not in s)) else ""
    reseaux.append(("oui" if utilise.strip() == "*" else "", propre(ssid, 32) or "(masqué)", bssid.upper(), int(chan), bande, int(sig), s + alerte))
reseaux.sort(key=lambda r: -r[5])
if mode == "liste":
    for r in reseaux:
        print("%s|%s|%s|%d|%s|%d %%|%s" % r)
else:
    canaux = {}
    for r in reseaux:
        k = (r[4], r[3]); n, mx = canaux.get(k, (0, 0)); canaux[k] = (n + 1, max(mx, r[5]))
    for (b, c), (n, mx) in sorted(canaux.items(), key=lambda x: (-x[1][0], x[0])):
        print("%s|%d|%d|%d %%" % (b, c, n, mx))
'

liste() {   # liste [relancer|cache] [liste|canaux]
	local rescan=no
	[ "${1:-cache}" = relancer ] && rescan=yes
	command -v nmcli >/dev/null || { echo "—|nmcli absent (paquet network-manager)|—|0|—|0 %|—"; exit 2; }
	LC_ALL=C timeout 20 nmcli -t -f IN-USE,BSSID,SSID,CHAN,FREQ,SIGNAL,SECURITY device wifi list --rescan "$rescan" 2>/dev/null |
		head -c 1048576 | python3 -I -c "$PY_LISTE" "${2:-liste}"
}

dialogue_principal() {
	cat <<XML
<window title="Réseaux Wi-Fi à portée" default-width="900" default-height="600">
  <vbox spacing="6">
    <tree space-expand="true" space-fill="true">
      <label>Utilisé|SSID|BSSID|Canal|Bande|Signal|Sécurité</label>
      <variable>RESEAUX</variable>
      <height>320</height>
      <input>exec "\$SERMO_SCRIPT" --liste</input>
    </tree>
    <frame label="Occupation des canaux (réseaux vus par canal)">
      <tree>
        <label>Bande|Canal|Réseaux|Signal max</label>
        <variable>CANAUX</variable>
        <height>150</height>
        <input>exec "\$SERMO_SCRIPT" --canaux</input>
      </tree>
    </frame>
    <text><label>Lecture seule : aucune connexion, aucun mot de passe. Les noms des réseaux voisins ne sont ni gardés ni envoyés.</label></text>
    <hbox>
      <button><label>Relancer la recherche</label>
        <action>exec "\$SERMO_SCRIPT" --liste relancer >/dev/null</action>
        <action>refresh:RESEAUX</action><action>refresh:CANAUX</action>
      </button>
      <button><label>Fermer</label><action>exit:fermer</action></button>
    </hbox>
    <timer interval="15" visible="false">
      <variable>HORLOGE</variable>
      <action>refresh:RESEAUX</action><action>refresh:CANAUX</action>
    </timer>
  </vbox>
</window>
XML
}

usage() { sed -n '/^# Usage/,/^# Codes/p' "$0" | sed '$d;s/^# \{0,1\}//' >&2; exit 64; }

main() {
	command -v python3 >/dev/null || { echo "python3 est absent" >&2; exit 2; }
	case "${1:-}" in
	--liste)  case "${2:-}" in ''|relancer) liste "${2:-cache}" liste; exit ;; *) usage ;; esac ;;
	--canaux) liste cache canaux; exit ;;
	--xml)    dialogue_principal; exit 0 ;;
	-*)       usage ;;
	esac
	[ -n "${1:-}" ] && SERMO=$1
	command -v "$SERMO" >/dev/null || { echo "sermo introuvable : $SERMO" >&2; exit 2; }
	command -v nmcli >/dev/null || { echo "nmcli est absent : sudo apt install network-manager" >&2; exit 2; }
	export DIALOGUE; DIALOGUE=$(dialogue_principal)
	"$SERMO" --program=DIALOGUE >/dev/null
	exit 0
}

[ "${BASH_SOURCE[0]}" = "$0" ] && main "$@"
