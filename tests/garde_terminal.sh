#!/bin/bash
# SPDX-License-Identifier: GPL-2.0-or-later
# tests/garde_terminal.sh — <terminal> ouvre un vrai terminal sur gtk3sermo et
# gtk4sermo ; ailleurs, un dialogue qui en contient un tourne quand même.
#
# POURQUOI CETTE GARDE EXISTE
#
# De la 2.0 à la 2.7.0, <terminal> n'existait plus nulle part, sans un message :
# le -DHAVE_VTE ne vivait que dans les anciens Makefile.am, et CMake ne l'a
# jamais repris (gtk4 le forçait même à 0). Les paquets 1.x dépendaient de
# libvte ; les 2.x, non. Aucun banc ne touchait ce widget. Le manuel et le
# README de gtk3 promettaient pourtant le terminal.
#
# Sur gtk3/gtk4, la garde vérifie ce qui fait un terminal :
#  - --version annonce VTE (le cœur a été compilé avec) ;
#  - la variable du widget rend le PID de l'enfant (un nombre, pas du vide) ;
#  - le shell de l'enfant tourne vraiment dans le terminal : la ligne que
#    <input> lui envoie est exécutée et laisse un fichier témoin.
# Ailleurs, le widget n'est pas promis : la garde vérifie seulement que le
# dialogue qui en contient un s'ouvre et se ferme normalement.
#
# Usage : garde_terminal.sh <chemin-du-binaire>
# Codes : 0 = conforme · 1 = écart · 2 = usage/outil

set -u
BIN="${1:-}"
[[ -x "$BIN" ]] || { echo "usage: $0 <chemin-du-binaire>" >&2; exit 2; }
BIN="$(readlink -f "$BIN")"
for t in timeout xvfb-run; do
    command -v "$t" >/dev/null || { echo "outil manquant : $t" >&2; exit 2; }
done

TMP="$(mktemp -d)"; trap 'rm -rf "$TMP"' EXIT
echecs=0
ok() { echo "  ✔ $1"; }
ko() { echo "  ✘ $1"; echecs=$((echecs + 1)); }

# L'<input> d'un terminal est TAPÉ dans le shell de l'enfant : la commande
# imprime la ligne à taper, le shell l'exécute.
dialogue="<window><vbox><terminal><variable>TRM</variable><input>printf '%s\\n' 'echo terminal-ok > $TMP/temoin'</input></terminal><timer milliseconds=\"true\" interval=\"2500\" visible=\"false\"><variable>_T</variable><action>exit:fin</action></timer></vbox></window>"

echo "garde_terminal : $BIN"
case "$(basename "$BIN")" in
    gtk3sermo|gtk4sermo)
        if "$BIN" --version 2>/dev/null | grep -q 'support for:.*VTE'; then ok "--version annonce VTE"
        else ko "--version n'annonce pas VTE : $("$BIN" --version 2>/dev/null | grep 'support for' )"; fi

        sortie="$(MAIN_DIALOG="$dialogue" GSK_RENDERER=cairo timeout 25 xvfb-run -a "$BIN" --program=MAIN_DIALOG </dev/null 2>"$TMP/err")"; rc=$?
        if [[ $rc -eq 0 ]] && grep -qx 'EXIT="fin"' <<<"$sortie"; then ok "dialogue fermé par sa minuterie (rc=0)"
        else ko "rc=$rc, $(grep '^EXIT=' <<<"$sortie") — $(grep -v 'libEGL\|xapp\|safe_popen' "$TMP/err" | head -2 | tr '\n' ' ')"; fi
        if grep -qE '^TRM="[1-9][0-9]*"$' <<<"$sortie"; then ok "la variable rend le PID de l'enfant ($(grep '^TRM=' <<<"$sortie"))"
        else ko "PID attendu, obtenu : $(grep '^TRM=' <<<"$sortie")"; fi
        if [[ "$(cat "$TMP/temoin" 2>/dev/null)" = terminal-ok ]]; then ok "le shell du terminal a exécuté la ligne tapée (fichier témoin écrit)"
        else ko "fichier témoin absent : le terminal n'a rien exécuté"; fi
        ;;
    *)
        sortie="$(MAIN_DIALOG="$dialogue" QT_QPA_PLATFORM=offscreen SDL_VIDEODRIVER=offscreen ELM_ENGINE=buffer \
            SERMO_NCURSES_BATCH=1 GSK_RENDERER=cairo timeout 25 xvfb-run -a "$BIN" --program=MAIN_DIALOG </dev/null 2>"$TMP/err")"; rc=$?
        if [[ $rc -eq 0 ]] && grep -qx 'EXIT="fin"' <<<"$sortie"; then ok "terminal non promis ici ; le dialogue qui en contient un tourne et se ferme (rc=0)"
        else ko "rc=$rc, $(grep '^EXIT=' <<<"$sortie") — $(head -2 "$TMP/err" | tr '\n' ' ')"; fi
        ;;
esac

if [[ $echecs -gt 0 ]]; then
    echo "ÉCHEC : $echecs écart(s)."
    exit 1
fi
echo "garde_terminal : OK — conforme pour $(basename "$BIN")."
