#!/bin/bash
# SPDX-License-Identifier: CC0-1.0
# Métier   : Administration système, support, développement de services
# Besoin   : suivre le journal en direct (système ou session), filtré par gravité,
#            par unité et par texte, sans terminal
# Enrobe   : journalctl -f -o json (systemd) ; mise en forme par python3 -I
# Ports    : tout port à fenêtre (comboboxtext, entry, button, tree)
# Terminal : ncursessermo par SSH : rien de perdu
# Usage    : ./journal-direct.sh [binaire sermo]
#            Sans fenêtre (mêmes contrôles ; JD_DIR = dossier de session) :
#              ./journal-direct.sh --ouvrir système|session GRAVITÉ [UNITÉ]
#              ./journal-direct.sh --vue [FILTRE]    (heure|gravité|source|message)
#              ./journal-direct.sh --fermer
#            GRAVITÉ : urgence alerte critique erreur avertissement notice info débogage
#
# Codes de sortie : 0 fait · 2 journalctl ou python3 absent · 64 entrée invalide.
# Le journal « système » demande d'être dans le groupe systemd-journal ou adm
# (sinon journalctl ne montre que vos propres messages) : aucune élévation ici.
# Les messages sont des données : nettoyés, plafonnés, jamais insérés dans le XML,
# gardés seulement dans un dossier privé effacé à la fermeture.

export SERMO="${SERMO:-sermo}"
export SERMO_SCRIPT="$0"
GRAVITES="urgence alerte critique erreur avertissement notice info débogage"
JOURNAL_MAX=8388608     # au-delà, la lecture s'arrête (journal plein)

xml() { local s=${1//&/"&amp;"}; s=${s//</"&lt;"}; s=${s//>/"&gt;"}; printf '%s' "${s//\"/"&quot;"}"; }
rang() { local i=0 g; for g in $GRAVITES; do [ "$g" = "$1" ] && { echo $i; return 0; }; i=$((i + 1)); done; return 1; }
refus() { printf 'entrée invalide : %s\n' "$1" | tee "$JD_DIR/etat" >&2; exit 64; }

# ── Lecture en fond, gardée (même schéma que liaison-serie et bus-can) ──────
fermer() {
	local g
	g=$(cat "$JD_DIR/gardien" 2>/dev/null)
	if [[ "$g" =~ ^[0-9]+$ ]]; then kill -TERM "$g" 2>/dev/null; fi
	rm -f -- "$JD_DIR/gardien"
	echo "suivi arrêté" > "$JD_DIR/etat"
}
ouvrir() {
	local source=$1 grav=$2 unite=${3:-} p n
	local -a args=(-f -n 100 -o json --no-pager)
	case "$source" in
	système) ;;
	session) args+=(--user) ;;
	*) refus "source « $source » (système ou session)" ;;
	esac
	p=$(rang "$grav") || refus "gravité « $grav »"
	args+=(-p "$p")
	if [ -n "$unite" ]; then
		[[ "$unite" =~ ^[A-Za-z0-9][A-Za-z0-9@._:-]{0,127}$ ]] || refus "unité « $unite » (nom d'unité systemd)"
		args+=(-u "$unite")
	fi
	fermer
	: > "$JD_DIR/journal"; rm -f -- "$JD_DIR/lecteur"
	bash -c '
		maitre=$1; journal=$2; max=$3; shift 3
		LC_ALL=C journalctl "$@" 2>"$journal.err" >> "$journal" &
		lecteur=$!
		echo "$lecteur" > "${journal%/*}/lecteur"
		trap '"'"'kill -TERM "$lecteur" 2>/dev/null; exit 0'"'"' TERM INT HUP
		# Plafond vérifié chaque seconde : dépassement possible, au plus une seconde de messages.
		while kill -0 "$maitre" 2>/dev/null && kill -0 "$lecteur" 2>/dev/null &&
		      [ "$(stat -c %s -- "$journal")" -lt "$max" ]; do sleep 1 & wait $!; done
		kill -TERM "$lecteur" 2>/dev/null
	' _ "${JD_MAITRE:-$PPID}" "$JD_DIR/journal" "$JOURNAL_MAX" "${args[@]}" </dev/null >/dev/null 2>&1 &
	echo $! > "$JD_DIR/gardien"
	for n in 1 2 3 4 5 6 7 8 9 10; do [ -s "$JD_DIR/lecteur" ] && break; sleep 0.1; done
	printf 'suivi du journal %s, gravité %s ou pire%s\n' "$source" "$grav" "${unite:+, unité $unite}" | tee "$JD_DIR/etat"
}

# ── Vue : les 300 derniers messages qui passent le filtre, du plus récent au plus ancien
PY_VUE='
import json, sys, time
journal, filtre, plein = sys.argv[1], sys.argv[2].lower(), sys.argv[3] == "1"
NOMS = ["urgence", "alerte", "critique", "erreur", "avert.", "notice", "info", "débogage"]
def propre(v, n):
    if isinstance(v, list):                       # MESSAGE binaire : tableau d octets
        v = bytes(x for x in v if isinstance(x, int) and 0 <= x < 256).decode("utf-8", "replace")
    if not isinstance(v, str):
        return "—"
    v = "".join(c if c >= " " and c != "\x7f" else " " for c in v).replace("|", "¦").strip()
    return v[:n] + ("…" if len(v) > n else "")
lignes = []
try:
    f = open(journal, "rb")
except OSError:
    sys.exit(0)
for brut in f:
    if len(brut) > 65536:
        continue
    try:
        e = json.loads(brut)
    except ValueError:
        continue
    if not isinstance(e, dict):
        continue
    try:
        t = time.strftime("%H:%M:%S", time.localtime(int(e.get("__REALTIME_TIMESTAMP", "0")) / 1e6))
    except (ValueError, TypeError, OverflowError):
        t = "—"
    p = e.get("PRIORITY")
    g = NOMS[int(p)] if isinstance(p, str) and p.isdigit() and int(p) < 8 else "—"
    src = propre(e.get("SYSLOG_IDENTIFIER") or e.get("_COMM") or "—", 40)
    msg = propre(e.get("MESSAGE"), 300)
    ligne = "%s|%s|%s|%s" % (t, g, src, msg)
    if filtre and filtre not in ligne.lower():
        continue
    lignes.append(ligne)
if plein:
    print("—|—|journal plein|lecture arrêtée : rouvrez le suivi")
for l in reversed(lignes[-300:]):
    print(l)
'
vue() {
	local filtre=${1:-} plein=0
	[ "${#filtre}" -le 200 ] && [[ "$filtre" != *[$'\n\r']* ]] || { echo "—|—|filtre|filtre refusé (une ligne, 200 caractères)"; return 64; }
	[ "$(stat -c %s -- "$JD_DIR/journal" 2>/dev/null || echo 0)" -ge "$JOURNAL_MAX" ] && plein=1
	python3 -I -c "$PY_VUE" "$JD_DIR/journal" "$filtre" "$plein"
}

dialogue_principal() {
	local g
	cat <<XML
<window title="Journal en direct" default-width="900" default-height="600">
  <vbox spacing="6">
    <frame label="Suivi" space-expand="false" space-fill="false">
      <hbox>
        <comboboxtext><variable>SOURCE</variable><default>système</default><item>système</item><item>session</item></comboboxtext>
        <comboboxtext><variable>GRAVITE</variable><default>avertissement</default>
$(for g in $GRAVITES; do printf '          <item>%s</item>\n' "$g"; done)
        </comboboxtext>
        <text><label>Unité :</label></text>
        <entry><variable>UNITE</variable></entry>
        <button><label>Suivre</label>
          <action>exec "\$SERMO_SCRIPT" --ouvrir "\$SOURCE" "\$GRAVITE" "\$UNITE"</action><action>refresh:ETAT</action>
        </button>
        <button><label>Arrêter</label><action>exec "\$SERMO_SCRIPT" --fermer</action><action>refresh:ETAT</action></button>
      </hbox>
    </frame>
    <hbox space-expand="false" space-fill="false">
      <text><variable>ETAT</variable><input>cat "\$JD_DIR/etat"</input></text>
      <text><label>   Filtre :</label></text>
      <entry space-expand="true" space-fill="true"><variable>FILTRE</variable></entry>
    </hbox>
    <tree space-expand="true" space-fill="true">
      <label>Heure|Gravité|Source|Message</label>
      <variable>MESSAGES</variable>
      <height>400</height>
      <input>exec "\$SERMO_SCRIPT" --vue "\$FILTRE"</input>
    </tree>
    <timer interval="2" visible="false">
      <variable>HORLOGE</variable>
      <action>refresh:MESSAGES</action>
    </timer>
    <hbox><button><label>Fermer</label><action>exit:fermer</action></button></hbox>
  </vbox>
</window>
XML
}

preparer_dossier() {
	if [ -n "${JD_DIR:-}" ] && [ -d "$JD_DIR" ]; then return 0; fi
	JD_DIR=$(mktemp -d "${XDG_RUNTIME_DIR:-${TMPDIR:-/tmp}}/sermo-journal.XXXXXX") || exit 2
	export JD_DIR; JD_PROPRE=1
	: > "$JD_DIR/journal"; echo "aucun suivi" > "$JD_DIR/etat"
}

usage() { sed -n '/^# Usage/,/^# Codes/p' "$0" | sed '$d;s/^# \{0,1\}//' >&2; exit 64; }

main() {
	umask 077
	local c
	for c in journalctl python3; do
		command -v "$c" >/dev/null || { echo "$c est absent (systemd, python3)" >&2; exit 2; }
	done
	case "${1:-}" in
	--ouvrir|--fermer|--vue)
		preparer_dossier
		[ "${JD_PROPRE:-}" = 1 ] && [ "$1" != --ouvrir ] && trap 'rm -rf -- "$JD_DIR"' EXIT
		case "$1" in
		--ouvrir) ouvrir "${2:-}" "${3:-}" "${4:-}" ;;
		--fermer) fermer ;;
		--vue)    vue "${2:-}" ;;
		esac
		exit ;;
	--xml) preparer_dossier; [ "${JD_PROPRE:-}" = 1 ] && trap 'rm -rf -- "$JD_DIR"' EXIT; dialogue_principal; exit 0 ;;
	-*)    usage ;;
	esac
	[ -n "${1:-}" ] && SERMO=$1
	command -v "$SERMO" >/dev/null || { echo "sermo introuvable : $SERMO" >&2; exit 2; }
	preparer_dossier
	export JD_MAITRE=$$
	trap 'fermer; rm -rf -- "$JD_DIR"' EXIT
	trap 'exit 129' HUP; trap 'exit 130' INT; trap 'exit 143' TERM
	export DIALOGUE; DIALOGUE=$(dialogue_principal)
	"$SERMO" --program=DIALOGUE >/dev/null
	exit 0
}

[ "${BASH_SOURCE[0]}" = "$0" ] && main "$@"
