#!/bin/bash
# SPDX-License-Identifier: GPL-2.0-or-later
# tests/garde_glade.sh — --glade-xml charge un fichier GtkBuilder sur gtk3sermo et
# gtk4sermo ; les autres ports le refusent en le disant.
#
# POURQUOI CETTE GARDE EXISTE
#
# Jusqu'à la 2.6.8, --glade-xml supposait libglade, qui n'existe ni pour GTK 3 ni
# pour GTK 4 : l'option était compilée « ignorée » sur les sept ports, et tout
# script qui s'en servait avortait — pendant que les pages de manuel promettaient
# le chargement. Le code GtkBuilder existait, jamais compilé, et portait trois
# défauts : un gestionnaire sur deux jamais branché, des variables nommées d'après
# le TYPE du widget, un avortement sur fichier illisible.
#
# Sur gtk3/gtk4, un petit fichier vérifie ce qui fait un dialogue Glade :
#  - la fenêtre est trouvée par son identifiant (MAIN_WINDOW par défaut) ;
#  - chaque widget devient une variable nommée d'après son identifiant ;
#  - un gestionnaire de « realize » REMPLIT le widget (c'est son <input>) ;
#  - un gestionnaire ordinaire (« map » de la fenêtre) exécute une action (exit:) ;
#  - un fichier absent ou une fenêtre inconnue rendent le code 1 et un message,
#    jamais un avortement.
# Ailleurs : l'option est refusée (code 1, message), et aucun dialogue ne tourne.
#
# Usage : garde_glade.sh <chemin-du-binaire>
# Codes : 0 = conforme · 1 = écart · 2 = usage/outil

set -u
BIN="${1:-}"
[[ -x "$BIN" ]] || { echo "usage: $0 <chemin-du-binaire>" >&2; exit 2; }
BIN="$(readlink -f "$BIN")"
for t in timeout xvfb-run; do
    command -v "$t" >/dev/null || { echo "outil manquant : $t" >&2; exit 2; }
done

TMP="$(mktemp -d)"; trap 'rm -rf "$TMP"' EXIT
# Fichier valable en GTK 3 comme en GTK 4 : GtkWindow, GtkBox, GtkEntry.
cat > "$TMP/essai.ui" <<'EOF'
<?xml version="1.0" encoding="UTF-8"?>
<interface>
  <object class="GtkWindow" id="MAIN_WINDOW">
    <property name="title">garde glade</property>
    <signal name="map" handler="exit:carte"/>
    <child>
      <object class="GtkBox" id="boite">
        <property name="visible">True</property>
        <property name="orientation">vertical</property>
        <child>
          <object class="GtkEntry" id="nom">
            <property name="visible">True</property>
            <signal name="realize" handler="echo glade-ok"/>
          </object>
        </child>
      </object>
    </child>
  </object>
</interface>
EOF

echecs=0
ok() { echo "  ✔ $1"; }
ko() { echo "  ✘ $1"; echecs=$((echecs + 1)); }

echo "garde_glade : $BIN"
case "$(basename "$BIN")" in
    gtk3sermo|gtk4sermo)
        glade() { ( cd "$TMP" && GSK_RENDERER=cairo timeout 20 xvfb-run -a "$BIN" "$@" </dev/null 2>"$TMP/err" ); }

        sortie="$(glade --glade-xml=essai.ui)"; rc=$?
        if [[ $rc -eq 0 ]]; then ok "fichier chargé, dialogue fermé par son gestionnaire (rc=0)"
        else ko "rc=$rc au lieu de 0 — $(grep -v 'libEGL\|xapp' "$TMP/err" | head -2 | tr '\n' ' ')"; fi
        if grep -qx 'EXIT="carte"' <<<"$sortie"; then ok "gestionnaire de signal exécuté (map → exit:carte)"
        else ko "EXIT=\"carte\" attendu, obtenu : $(grep '^EXIT=' <<<"$sortie")"; fi
        if grep -qx 'nom="glade-ok"' <<<"$sortie"; then ok "realize remplit le widget, variable nommée par son identifiant"
        else ko "nom=\"glade-ok\" attendu, obtenu : $(grep '^nom=' <<<"$sortie")"; fi

        glade --glade-xml=absent.ui >/dev/null; rc=$?
        if [[ $rc -eq 1 ]] && grep -q 'absent.ui' "$TMP/err"; then ok "fichier absent : code 1 et message"
        else ko "fichier absent : rc=$rc (1 attendu, 134 = avortement) — $(grep -v 'libEGL' "$TMP/err" | head -1)"; fi

        glade --program=inconnue --glade-xml=essai.ui >/dev/null; rc=$?
        if [[ $rc -eq 1 ]] && grep -q "inconnue" "$TMP/err"; then ok "fenêtre inconnue : code 1 et message"
        else ko "fenêtre inconnue : rc=$rc (1 attendu) — $(grep -v 'libEGL' "$TMP/err" | head -1)"; fi
        ;;
    *)
        ( cd "$TMP" && QT_QPA_PLATFORM=offscreen SDL_VIDEODRIVER=offscreen ELM_ENGINE=buffer \
            SERMO_NCURSES_BATCH=1 timeout 20 "$BIN" --glade-xml=essai.ui </dev/null >"$TMP/out" 2>"$TMP/err" ); rc=$?
        if [[ $rc -eq 1 ]] && grep -q 'gtk3sermo et gtk4sermo' "$TMP/err"; then ok "option refusée : code 1 et message"
        else ko "refus attendu (code 1, message) : rc=$rc — $(head -1 "$TMP/err")"; fi
        if ! grep -q '^EXIT=' "$TMP/out"; then ok "aucun dialogue lancé à la place"
        else ko "un dialogue a tourné malgré le refus"; fi
        ;;
esac

if [[ $echecs -gt 0 ]]; then
    echo "ÉCHEC : $echecs écart(s)."
    exit 1
fi
echo "garde_glade : OK — conforme pour $(basename "$BIN")."
