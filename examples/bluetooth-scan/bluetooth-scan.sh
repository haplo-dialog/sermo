#!/bin/bash
# SPDX-License-Identifier: CC0-1.0
# Métier   : Support, objets connectés, atelier, sécurité du poste
# Besoin   : voir les appareils Bluetooth à portée (nom, signal) et ceux déjà connus du
#            poste (appairés, connectés) — sans appairer ni se connecter à rien
# Enrobe   : bluetoothctl (paquet bluez) : « scan on » borné dans le temps, « devices »
# Ports    : tout port à fenêtre (tree, button, text)
# Terminal : ncursessermo par SSH : rien de perdu
# Usage    : ./bluetooth-scan.sh [binaire sermo]
#            ./bluetooth-scan.sh --etat                     (une ligne : adaptateur, service)
#            ./bluetooth-scan.sh --balayer [SECONDES]       (5 à 30, défaut 10)
#            ./bluetooth-scan.sh --liste                    (adresse|nom|signal|connu)
#            BT_SYSFS : autre racine que /sys/class/bluetooth (essais).
#
# Codes de sortie : 0 fait · 2 bluetoothctl absent · 3 pas d'adaptateur ou service
# arrêté · 64 usage. Sans adaptateur, bluetoothctl attend le service sans fin :
# l'exemple le détecte AVANT et ne le lance pas. Chaque appel est borné.
# Confidentialité : les noms des appareils voisins (« Téléphone de … ») sont des
# données personnelles : affichés, jamais envoyés ; le dernier balayage vit dans un
# dossier privé effacé à la fermeture. Noms nettoyés, jamais insérés dans le XML.

export SERMO="${SERMO:-sermo}"
export SERMO_SCRIPT="$0"
export BT_SYSFS="${BT_SYSFS:-/sys/class/bluetooth}"

# 0 si un adaptateur existe ET que bluetoothd tourne ; sinon un message, code 3.
pret() {
	local n
	n=$(find "$BT_SYSFS" -maxdepth 1 -name 'hci[0-9]*' 2>/dev/null | wc -l)
	if [ "$n" = 0 ]; then echo "aucun adaptateur Bluetooth sur cette machine"; return 3; fi
	if ! timeout 3 systemctl is-active --quiet bluetooth 2>/dev/null; then
		echo "service bluetooth arrêté (sudo systemctl start bluetooth)"; return 3
	fi
	return 0
}

etat() {
	local m rc
	command -v bluetoothctl >/dev/null || { echo "bluetoothctl absent (sudo apt install bluez)"; return 2; }
	m=$(pret); rc=$?
	[ $rc = 0 ] && m="adaptateur prêt — « Balayer » cherche les appareils à portée"
	printf '%s\n' "$m"
	return $rc
}

# Balayage borné : « scan on » pendant N s ; on garde la sortie brute (plafonnée).
balayer() {
	local s=${1:-10} rc
	[[ "$s" =~ ^[0-9]{1,2}$ ]] && [ "$s" -ge 5 ] && [ "$s" -le 30 ] || { echo "durée invalide : $s (5 à 30 s)" >&2; return 64; }
	command -v bluetoothctl >/dev/null || { echo "bluetoothctl absent (sudo apt install bluez)" >&2; return 2; }
	pret >/dev/null || { pret >&2; return 3; }
	LC_ALL=C timeout "$((s + 5))" bluetoothctl --timeout "$s" scan on </dev/null 2>/dev/null | head -c 1048576 > "$BS_DIR/balayage.n"
	mv -f -- "$BS_DIR/balayage.n" "$BS_DIR/balayage"
	LC_ALL=C timeout 5 bluetoothctl devices Paired </dev/null 2>/dev/null | head -c 262144 > "$BS_DIR/appaires"
	LC_ALL=C timeout 5 bluetoothctl devices Connected </dev/null 2>/dev/null | head -c 262144 > "$BS_DIR/connectes"
	printf 'balayage de %d s terminé à %s\n' "$s" "$(date +%T)" > "$BS_DIR/etat"
	return 0
}

# Mise en forme : seulement les lignes « Device AA:BB:CC:DD:EE:FF … » et
# « Device … RSSI: -NN » (formats de bluetoothctl) ; tout le reste est ignoré.
PY_LISTE='
import re, sys
d = sys.argv[1]
def lire(n):
    try:
        return open(d + "/" + n, "rb").read().decode("utf-8", "replace")
    except OSError:
        return ""
ANSI = re.compile(r"\x1b\[[0-9;]*[A-Za-z]")
MAC = r"([0-9A-F]{2}(?::[0-9A-F]{2}){5})"
def propre(s, n):
    s = "".join(c if c >= " " and c != "\x7f" else "?" for c in s).replace("|", "¦").strip()
    return s[:n]
noms, rssi, ordre = {}, {}, []
for ligne in ANSI.sub("", lire("balayage")).split("\n")[:20000]:
    ligne = ligne.replace("\r", "")
    m = re.search(r"\b(?:NEW|CHG)\]\s+Device\s+" + MAC + r"\s+RSSI:\s+(?:0x[0-9a-fA-F]{1,8}\s+\()?(-?\d{1,3})\)?(?:\s|$)", ligne)
    if m:
        a, v = m.group(1), int(m.group(2))
        if -127 <= v <= 20:
            rssi[a] = v
            if a not in ordre: ordre.append(a)
        continue
    m = re.search(r"\bNEW\]\s+Device\s+" + MAC + r"(?:\s+(.*))?$", ligne)
    if m:
        a = m.group(1)
        if a not in ordre: ordre.append(a)
        nom = (m.group(2) or "").strip()
        if nom and nom.replace("-", ":") != a:
            noms[a] = propre(nom, 40)
def macs(n):
    vus = set()
    for a, nom in re.findall(r"^Device\s+" + MAC + r"(?:[ \t]+([^\r\n]*))?$", ANSI.sub("", lire(n)), re.M):
        vus.add(a)
        if a not in noms and nom.strip() and nom.strip().replace("-", ":") != a:
            noms[a] = propre(nom, 40)          # nom connu du poste (appareil hors de portée)
    return vus
app, con = macs("appaires"), macs("connectes")
for a in sorted(app | con):
    if a not in ordre: ordre.append(a)
lignes = []
for a in ordre[:300]:
    connu = "connecté" if a in con else "appairé" if a in app else ""
    lignes.append((a, noms.get(a, "(sans nom)"), rssi.get(a), connu))
lignes.sort(key=lambda r: (r[2] is None, -(r[2] if r[2] is not None else 0)))
for a, n, r, c in lignes:
    print("%s|%s|%s|%s" % (a, n, ("%d dBm" % r) if r is not None else "—", c))
'
liste() { python3 -I -c "$PY_LISTE" "$BS_DIR"; }

dialogue_principal() {
	cat <<XML
<window title="Appareils Bluetooth à portée" default-width="760" default-height="520">
  <vbox spacing="6">
    <text><variable>ETAT</variable><input>exec "\$SERMO_SCRIPT" --etat-session</input></text>
    <tree space-expand="true" space-fill="true">
      <label>Adresse|Nom|Signal|Connu du poste</label>
      <variable>APPAREILS</variable>
      <height>360</height>
      <input>exec "\$SERMO_SCRIPT" --liste</input>
    </tree>
    <text><label>Lecture seule : rien n'est appairé ni connecté. Les noms des appareils voisins ne sont ni gardés ni envoyés.</label></text>
    <hbox>
      <button><label>Balayer 10 s</label>
        <action>exec "\$SERMO_SCRIPT" --balayer 10</action>
        <action>refresh:ETAT</action><action>refresh:APPAREILS</action>
      </button>
      <button><label>Fermer</label><action>exit:fermer</action></button>
    </hbox>
  </vbox>
</window>
XML
}

preparer_dossier() {
	if [ -n "${BS_DIR:-}" ] && [ -d "$BS_DIR" ]; then return 0; fi
	BS_DIR=$(mktemp -d "${XDG_RUNTIME_DIR:-${TMPDIR:-/tmp}}/sermo-bt.XXXXXX") || exit 2
	export BS_DIR; BS_PROPRE=1
	: > "$BS_DIR/balayage"; : > "$BS_DIR/appaires"; : > "$BS_DIR/connectes"; : > "$BS_DIR/etat"
}

usage() { sed -n '/^# Usage/,/^# Codes/p' "$0" | sed '$d;s/^# \{0,1\}//' >&2; exit 64; }

main() {
	umask 077
	command -v python3 >/dev/null || { echo "python3 est absent" >&2; exit 2; }
	case "${1:-}" in
	--etat) etat; exit ;;
	--etat-session) [ -s "${BS_DIR:-/nonexistent}/etat" ] && cat "$BS_DIR/etat" || etat; exit 0 ;;
	--balayer|--liste)
		preparer_dossier
		[ "${BS_PROPRE:-}" = 1 ] && trap 'rm -rf -- "$BS_DIR"' EXIT
		case "$1" in --balayer) balayer "${2:-10}" ;; --liste) liste ;; esac
		exit ;;
	--xml) dialogue_principal; exit 0 ;;
	-*)    usage ;;
	esac
	[ -n "${1:-}" ] && SERMO=$1
	command -v "$SERMO" >/dev/null || { echo "sermo introuvable : $SERMO" >&2; exit 2; }
	preparer_dossier
	trap 'rm -rf -- "$BS_DIR"' EXIT
	trap 'exit 129' HUP; trap 'exit 130' INT; trap 'exit 143' TERM
	export DIALOGUE; DIALOGUE=$(dialogue_principal)
	"$SERMO" --program=DIALOGUE >/dev/null
	exit 0
}

[ "${BASH_SOURCE[0]}" = "$0" ] && main "$@"
