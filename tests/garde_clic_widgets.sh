#!/bin/bash
# garde_clic_widgets.sh — les widgets interactifs doivent survivre à un CLIC.
#
# Pourquoi ce banc existe. tests/run_examples.sh ouvre les exemples et vérifie
# qu'une fenêtre apparaît — mais il ne clique jamais. Le 2026-08-25, cliquer un
# <switch> tuait les DEUX ports par SIGSEGV, 6 fois sur 6, pendant que les 55 et
# 58 exemples étaient comptés OK et que le pipeline restait vert.
#
# La cause était un signal dont l'arité ne correspond pas au rappel branché :
# GObject::notify passe (objet, pspec, data), le rappel n'attendait que
# (widget, Attr), et le GParamSpec était déréférencé comme un jeu d'attributs.
# Exactement la faute de GtkListBox::row-selected, corrigée trois jours plus tôt.
# Ce genre de défaut ne se voit qu'en INTERAGISSANT : ouvrir ne suffit pas.
#
# Le banc ouvre un dialogue minimal par widget, clique deux fois au même endroit
# (basculer puis rebasculer), et vérifie que le processus est toujours vivant.
# Un widget qui n'ouvre pas de fenêtre est un ÉCHEC, pas un cas ignoré : sinon
# une régression qui empêche l'affichage passerait pour un succès.
#
# Usage : garde_clic_widgets.sh <chemin-du-binaire>

set -u
BIN="${1:-}"
[ -x "$BIN" ] || { echo "usage: $0 <chemin-du-binaire>" >&2; exit 2; }
for t in xvfb-run xdotool timeout; do
	command -v "$t" >/dev/null || { echo "outil manquant : $t" >&2; exit 2; }
done

TMP="$(mktemp -d)"; trap 'rm -rf "$TMP"' EXIT
echec=0; joues=0

# GSK_RENDERER=cairo : sans lui GTK 4 échoue sur EGL sous Xvfb et n'ouvre rien.
clic() {
	LC_ALL=fr_FR.UTF-8 GSK_RENDERER=cairo \
	xvfb-run -a -s '-screen 0 800x600x24' timeout 45 sh -c '
		"$1" --file="$2" >/dev/null 2>&1 &
		pid=$!
		for i in $(seq 1 60); do
			WID=$(xdotool search --onlyvisible --name CLIC 2>/dev/null | head -1)
			[ -n "$WID" ] && break
			kill -0 $pid 2>/dev/null || break
			sleep 0.2
		done
		[ -z "$WID" ] && { kill $pid 2>/dev/null; echo NOWIN; exit 0; }
		eval $(xdotool getwindowgeometry --shell $WID)
		xdotool mousemove $((X + 40)) $((Y + 22)) click 1; sleep 1
		xdotool mousemove $((X + 40)) $((Y + 22)) click 1; sleep 1.5
		if kill -0 $pid 2>/dev/null; then
			kill $pid 2>/dev/null; wait $pid 2>/dev/null; echo VIVANT
		else
			wait $pid 2>/dev/null; echo "MORT rc=$?"
		fi' _ "$BIN" "$1" 2>/dev/null
}

essai() {
	nom="$1"; corps="$2"
	printf '<window title="CLIC"><vbox>%s</vbox></window>\n' "$corps" > "$TMP/w.xml"
	r="$(clic "$TMP/w.xml")"
	joues=$((joues + 1))
	case "$r" in
		VIVANT)  echo "  ✔ $nom" ;;
		NOWIN)   echo "  ✘ $nom : aucune fenêtre — la mesure n'a pas eu lieu" >&2; echec=$((echec + 1)) ;;
		*)       echo "  ✘ $nom : le processus est mort au clic ($r)" >&2; echec=$((echec + 1)) ;;
	esac
}

echo "garde_clic_widgets : $BIN"

essai switch       '<switch><variable>V</variable></switch>'
essai checkbox     '<checkbox><label>x</label><variable>V</variable></checkbox>'
essai togglebutton '<togglebutton><label>x</label><variable>V</variable></togglebutton>'
essai radiobutton  '<radiobutton><label>a</label><variable>R1</variable></radiobutton><radiobutton><label>b</label><variable>R2</variable></radiobutton>'
essai button       '<button><label>x</label><action>echo x</action></button>'
essai spinbutton   '<spinbutton><variable>V</variable></spinbutton>'
essai hscale       '<hscale><variable>V</variable></hscale>'
essai vscale       '<vscale><variable>V</variable></vscale>'
essai comboboxtext '<comboboxtext><variable>C</variable><item>un</item><item>deux</item></comboboxtext>'
essai entry        '<entry><variable>V</variable></entry>'
essai calendar     '<calendar><variable>V</variable></calendar>'
essai colorbutton  '<colorbutton><variable>V</variable></colorbutton>'
essai fontbutton   '<fontbutton><variable>V</variable></fontbutton>'
essai searchentry  '<searchentry><variable>V</variable></searchentry>'
essai notebook     '<notebook labels="a|b"><vbox><text><label>1</label></text></vbox><vbox><text><label>2</label></text></vbox></notebook>'
essai list         '<list><variable>L</variable><item>un</item><item>deux</item></list>'
essai tree         '<tree><variable>T</variable><label>col</label><item>un</item></tree>'

[ "$joues" -eq 0 ] && { echo "ÉCHEC — aucun widget joué." >&2; exit 1; }

if [ "$echec" -gt 0 ]; then
	cat >&2 <<'MSG'

ÉCHEC — un widget ne survit pas au clic.

La cause la plus fréquente est un signal dont l'ARITÉ ne correspond pas au
rappel branché : GObject::notify passe (objet, pspec, data) et
GtkListBox::row-selected passe (box, row, data), un argument de plus que les
signaux sans paramètre. Brancher dessus un rappel en (widget, Attr) fait
recevoir le mauvais pointeur, qui est ensuite déréférencé.

Le remède est un adaptateur de signature, comme hp_switch_notify_active()
dans widget_switch.c ou hp_list_row_selected() dans widget_list.c.
MSG
	exit 1
fi

echo "garde_clic_widgets : OK — $joues widgets, aucun plantage."
exit 0
