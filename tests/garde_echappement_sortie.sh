#!/bin/bash
# garde_echappement_sortie.sh — la sortie du programme doit rester des DONNÉES.
#
# Le programme écrit sur sa sortie standard des lignes  NOM="valeur"  et
# EXIT="valeur", et la documentation du projet invite à les passer à `eval`.
# Or la valeur ne vient pas forcément de l'auteur du script : elle peut être
# TAPÉE par la personne qui se sert du dialogue, dans une <entry>.
#
# Entre guillemets doubles, le shell POSIX développe QUATRE caractères :
# \  "  $  et l'accent grave. shell_escape_value() n'en échappait que deux, en
# affirmant en commentaire que sa sortie était « safely eval'd », et gtk4sermo
# n'avait pas la fonction du tout. Mesuré le 2026-08-25 : une valeur saisie
# valant $(touch /tmp/preuve) ressortait telle quelle, et l'eval recommandé
# exécutait la commande.
#
# ⚠️ Ce banc OUVRE une vraie fenêtre et CLIQUE. C'est délibéré : la ligne n'est
# rendue qu'à la fermeture du dialogue, et --print-ir ne l'imprime pas. Une
# première version se rabattait sur une chaîne fabriquée quand le binaire ne
# rendait rien — elle se validait donc sur son propre bruit et voyait des échecs
# partout. Il n'y a plus aucun repli : si la ligne n'est pas produite, le banc
# ÉCHOUE au lieu d'inventer.
#
# Usage : garde_echappement_sortie.sh <chemin-du-binaire>

set -u
BIN="${1:-}"
[ -x "$BIN" ] || { echo "usage: $0 <chemin-du-binaire>" >&2; exit 2; }
for t in xvfb-run xdotool timeout; do
	command -v "$t" >/dev/null || { echo "outil manquant : $t" >&2; exit 2; }
done

TMP="$(mktemp -d)"
trap 'rm -rf "$TMP"' EXIT
echec=0
joues=0

# Ouvre le dialogue, clique le bouton du bas, rend la sortie standard.
# GSK_RENDERER=cairo : sans lui GTK 4 échoue sur EGL sous Xvfb et n'ouvre rien.
rendre_ligne() {
	LC_ALL=fr_FR.UTF-8 GSK_RENDERER=cairo \
	xvfb-run -a -s '-screen 0 1024x768x24' timeout 45 sh -c '
		"$1" --file="$2" > "$3" 2>/dev/null &
		pid=$!
		for i in $(seq 1 80); do
			WID=$(xdotool search --onlyvisible --name garde 2>/dev/null | head -1)
			[ -n "$WID" ] && break
			kill -0 $pid 2>/dev/null || break
			sleep 0.2
		done
		[ -z "$WID" ] && { kill $pid 2>/dev/null; exit 1; }
		eval $(xdotool getwindowgeometry --shell $WID)
		xdotool mousemove $((X + WIDTH / 2)) $((Y + HEIGHT - 25)) click 1
		sleep 2
		kill -TERM $pid 2>/dev/null; wait $pid 2>/dev/null
	' _ "$BIN" "$1" "$2" 2>/dev/null
}

essai() {
	nom="$1"; charge="$2"
	temoin="$TMP/temoin_$nom"
	rm -f "$temoin"

	cat > "$TMP/d.xml" <<XML
<window title="garde">
  <vbox>
    <entry><variable>VAL</variable><default>${charge}</default></entry>
    <button><label>ok</label></button>
  </vbox>
</window>
XML

	rendre_ligne "$TMP/d.xml" "$TMP/out"
	ligne="$(grep -a '^VAL=' "$TMP/out" 2>/dev/null | head -1)"

	if [ -z "$ligne" ]; then
		echo "  ✘ $nom : le binaire n'a rendu AUCUNE ligne VAL= — la mesure n'a pas eu lieu" >&2
		echec=$((echec + 1)); return
	fi
	joues=$((joues + 1))

	# On évalue exactement comme la documentation l'a longtemps recommandé.
	( eval "$ligne" ) >/dev/null 2>&1 || true

	if [ -e "$temoin" ]; then
		echo "  ✘ $nom : eval a EXÉCUTÉ la commande — la valeur n'est pas échappée" >&2
		echo "      ligne rendue : $ligne" >&2
		echec=$((echec + 1))
	else
		echo "  ✔ $nom : eval n'exécute rien"
	fi
}

echo "garde_echappement_sortie : $BIN"

essai dollar   "\$(touch $TMP/temoin_dollar)"
essai accolade "\${IFS}\$(touch $TMP/temoin_accolade)"
essai backtick "\`touch $TMP/temoin_backtick\`"

# ── La même chose sur la ligne EXIT=, dont la valeur est l'étiquette du bouton
rm -f "$TMP/temoin_exit"
cat > "$TMP/e.xml" <<XML
<window title="garde">
  <vbox>
    <entry><variable>V</variable></entry>
    <button><label>\$(touch $TMP/temoin_exit)</label></button>
  </vbox>
</window>
XML
rendre_ligne "$TMP/e.xml" "$TMP/oute"
ligne="$(grep -a '^EXIT=' "$TMP/oute" 2>/dev/null | head -1)"
if [ -z "$ligne" ]; then
	echo "  ✘ EXIT : aucune ligne EXIT= rendue — la mesure n'a pas eu lieu" >&2
	echec=$((echec + 1))
else
	joues=$((joues + 1))
	( eval "$ligne" ) >/dev/null 2>&1 || true
	if [ -e "$TMP/temoin_exit" ]; then
		echo "  ✘ EXIT : eval a EXÉCUTÉ l'étiquette du bouton" >&2
		echo "      ligne rendue : $ligne" >&2
		echec=$((echec + 1))
	else
		echo "  ✔ EXIT : eval n'exécute rien"
	fi
fi

# ── Témoin de sensibilité ───────────────────────────────────────────────────
# Sans lui, un banc qui n'exécuterait plus jamais rien serait vert même s'il
# avait cessé de mesurer. On vérifie qu'une ligne NON échappée, elle, déclenche
# bien la détection.
rm -f "$TMP/temoin_sensibilite"
( eval "VAL=\"\$(touch $TMP/temoin_sensibilite)\"" ) >/dev/null 2>&1 || true
if [ ! -e "$TMP/temoin_sensibilite" ]; then
	echo "ÉCHEC — le témoin de sensibilité n'a pas été créé : le banc ne mesure rien." >&2
	exit 1
fi
echo "  ✔ témoin de sensibilité : une ligne NON échappée déclenche bien la détection"

# ── La valeur doit rester intacte, pas seulement inoffensive ────────────────
cat > "$TMP/p.xml" <<'XML'
<window title="garde">
  <vbox>
    <entry><variable>VAL</variable><default>Ada Lovelace</default></entry>
    <button><label>ok</label></button>
  </vbox>
</window>
XML
rendre_ligne "$TMP/p.xml" "$TMP/outp"
ligne="$(grep -a '^VAL=' "$TMP/outp" 2>/dev/null | head -1)"
if [ -z "$ligne" ]; then
	echo "  ✘ valeur ordinaire : aucune ligne rendue" >&2
	echec=$((echec + 1))
else
	obtenu="$( eval "$ligne"; printf '%s' "$VAL" )"
	if [ "$obtenu" = "Ada Lovelace" ]; then
		echo "  ✔ une valeur ordinaire traverse intacte"
	else
		echo "  ✘ une valeur ordinaire est abîmée : « $obtenu » au lieu de « Ada Lovelace »" >&2
		echec=$((echec + 1))
	fi
fi

[ "$joues" -eq 0 ] && { echo "ÉCHEC — aucun cas joué." >&2; exit 1; }

if [ "$echec" -gt 0 ]; then
	cat >&2 <<'MSG'

ÉCHEC — la sortie du programme peut être exécutée par eval.

Entre guillemets doubles, le shell développe \  "  $  et l'accent grave.
shell_escape_value() (src/variables.c) doit échapper LES QUATRE, et être
appelée sur CHAQUE ligne rendue : celle des variables (variables.c) et celle
d'EXIT (actions.c).

Rappel : la voie sûre pour l'appelant reste --do, qui passe les valeurs par
l'environnement sans jamais les relire comme du code.
MSG
	exit 1
fi

echo "garde_echappement_sortie : OK — $joues cas, aucune exécution."
exit 0
