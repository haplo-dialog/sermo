#!/bin/bash
# SPDX-License-Identifier: GPL-2.0-or-later
# ci/construire.sh — construit le cœur (les variantes utiles) et les backends.
#
# La CI (.gitlab-ci.yml) appelle ce script tel quel : ce qu'elle vérifie,
# n'importe qui peut le rejouer chez soi, sans rien d'autre qu'une Debian
# testing et les paquets de ci/dependances.txt.
#
# Usage : ci/construire.sh [port …]        (défaut : les neuf)
#         ports : gtk3 gtk4 qt6 fltk1 efl1 sdl3 lvgl ncurses html
# Journaux : $SERMO_JOURNAUX (défaut : _journaux/), un fichier par cible.
# Codes : 0 = tout est construit · 1 = au moins une cible a échoué

set -uo pipefail
# Messages des outils en anglais : le compte des avertissements cherche
# « warning: », et bison les écrit « avertissement : » sous une locale française.
export LC_ALL=C
RACINE=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
J="${SERMO_JOURNAUX:-$RACINE/_journaux}"
PORTS=("$@"); [ ${#PORTS[@]} -gt 0 ] || PORTS=(gtk3 gtk4 qt6 fltk1 efl1 sdl3 lvgl ncurses html)
N=$(nproc 2>/dev/null || echo 2)
mkdir -p "$J"
unset PKG_CONFIG_PATH
echo "Outils : $(cc --version | head -1) · $(cmake --version | head -1) · $(flex --version) · $(bison --version | head -1)"
echecs=0

variante() {  # port → variante du cœur
    case "$1" in gtk3) echo glib ;; gtk4) echo gtk4 ;; *) echo neutre ;; esac
}
dossiers() {  # variante → « dossier_build dossier_install options »
    case "$1" in
        neutre) echo "_build _install" ;;
        glib)   echo "_build_gtk _install_gtk -DSERMOCORE_GLIB=ON" ;;
        gtk4)   echo "_build_gtk4 _install_gtk4 -DSERMOCORE_GTK4=ON" ;;
    esac
}

declare -A FAITES=()
for p in "${PORTS[@]}"; do
    v=$(variante "$p")
    [ -n "${FAITES[$v]:-}" ] && continue
    read -r b i opts <<< "$(dossiers "$v")"
    journal="$J/coeur-$v.log"
    if { cmake -S "$RACINE/libsermocore" -B "$RACINE/libsermocore/$b" $opts -DCMAKE_INSTALL_PREFIX="$RACINE/libsermocore/$i" \
         && cmake --build "$RACINE/libsermocore/$b" -j"$N" \
         && cmake --install "$RACINE/libsermocore/$b"; } > "$journal" 2>&1; then
        printf 'OK     cœur %-7s (%s avertissement(s))\n' "$v" "$(grep -c 'warning:' "$journal")"
        FAITES[$v]=ok
    else
        printf 'ÉCHEC  cœur %-7s — voir %s\n' "$v" "$journal"; tail -20 "$journal"
        FAITES[$v]=ko; echecs=$((echecs + 1))
    fi
done

for p in "${PORTS[@]}"; do
    v=$(variante "$p")
    [ "${FAITES[$v]}" = ok ] || { printf 'SAUTÉ  %-8s (cœur %s en échec)\n' "$p" "$v"; echecs=$((echecs + 1)); continue; }
    read -r b i opts <<< "$(dossiers "$v")"
    journal="$J/backend-$p.log"
    if { PKG_CONFIG_PATH="$RACINE/libsermocore/$i/lib/pkgconfig" cmake -S "$RACINE/sermo-backend-$p" -B "$RACINE/sermo-backend-$p/_build" \
         && PKG_CONFIG_PATH="$RACINE/libsermocore/$i/lib/pkgconfig" cmake --build "$RACINE/sermo-backend-$p/_build" -j"$N"; } > "$journal" 2>&1 \
       && [ -x "$RACINE/sermo-backend-$p/_build/${p}sermo" ]; then
        printf 'OK     %-8s (%s avertissement(s)) — %s\n' "$p" "$(grep -c 'warning:' "$journal")" \
               "$(env -u DISPLAY QT_QPA_PLATFORM=offscreen SDL_VIDEODRIVER=offscreen ELM_ENGINE=buffer SERMO_NCURSES_BATCH=1 SERMO_HTML_OUT=/dev/null \
                  timeout 10 "$RACINE/sermo-backend-$p/_build/${p}sermo" --version 2>/dev/null | grep -m1 version)"
    else
        printf 'ÉCHEC  %-8s — voir %s\n' "$p" "$journal"; tail -20 "$journal"
        echecs=$((echecs + 1))
    fi
done

# Le serveur MCP sermoia (C, GLib) : construit avec les ports, joué par ci/bancs.sh.
journal="$J/sermoia-mcp.log"
if { cmake -S "$RACINE/sermoia-mcp" -B "$RACINE/sermoia-mcp/_build" \
     && cmake --build "$RACINE/sermoia-mcp/_build" -j"$N"; } > "$journal" 2>&1 \
   && [ -x "$RACINE/sermoia-mcp/_build/sermoia-mcp" ]; then
    printf 'OK     %-8s (%s avertissement(s))\n' sermoia "$(grep -c 'warning:' "$journal")"
else
    printf 'ÉCHEC  %-8s — voir %s\n' sermoia "$journal"; tail -20 "$journal"
    echecs=$((echecs + 1))
fi

# lvgl léger (paquet sermo-backend-lvgl-leger) : même port, sans ThorVG/Lottie.
case " ${PORTS[*]} " in *" lvgl "*)
    journal="$J/backend-lvgl-leger.log"
    if { PKG_CONFIG_PATH="$RACINE/libsermocore/_install/lib/pkgconfig" cmake -S "$RACINE/sermo-backend-lvgl" -B "$RACINE/sermo-backend-lvgl/_build_leger" -DSERMO_LVGL_LEGER=ON \
         && PKG_CONFIG_PATH="$RACINE/libsermocore/_install/lib/pkgconfig" cmake --build "$RACINE/sermo-backend-lvgl/_build_leger" -j"$N"; } > "$journal" 2>&1 \
       && [ -x "$RACINE/sermo-backend-lvgl/_build_leger/lvglsermo" ]; then
        printf 'OK     %-8s (%s avertissement(s))\n' lvgl-leg "$(grep -c 'warning:' "$journal")"
    else
        printf 'ÉCHEC  %-8s — voir %s\n' lvgl-leg "$journal"; tail -20 "$journal"
        echecs=$((echecs + 1))
    fi
    ;;
esac

# Les programmes d'aide de <input modbus> / <input opcua> (sermo_source.h).
journal="$J/sermo-source.log"
if { cmake -S "$RACINE/sermo-source" -B "$RACINE/sermo-source/_build" -DSERMO_SOURCE_EXIGER=ON \
     && cmake --build "$RACINE/sermo-source/_build" -j"$N"; } > "$journal" 2>&1 \
   && [ -x "$RACINE/sermo-source/_build/sermo-source-modbus" ] && [ -x "$RACINE/sermo-source/_build/sermo-source-opcua" ]; then
    printf 'OK     %-8s (%s avertissement(s))\n' source "$(grep -c 'warning:' "$journal")"
else
    printf 'ÉCHEC  %-8s — voir %s\n' source "$journal"; tail -20 "$journal"
    echecs=$((echecs + 1))
fi

[ "$echecs" -eq 0 ] || { echo "ÉCHEC : $echecs cible(s)."; exit 1; }
echo "Construction : tout est construit."
