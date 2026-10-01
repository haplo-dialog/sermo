#!/bin/bash
# SPDX-License-Identifier: GPL-2.0-or-later
# ci/bancs.sh — rejoue TOUS les bancs sur l'arbre construit par ci/construire.sh.
#
# Même script pour la CI et pour chacun : un banc qui ne tourne qu'en CI finit
# par mentir sans que personne le voie, un banc qui ne tourne que chez soi aussi.
#
# Usage : ci/bancs.sh [port …]      (défaut : les sept)
# Variables :
#   SERMO_JOURNAUX           dossier des journaux (défaut : _journaux/)
#   SERMO_BANC_PUBLICATION=1 contrôle AVANT PUBLICATION, chez le mainteneur :
#                            motifs privés exigés, identité de chaque commit,
#                            tout l'historique de HEAD. Sans elle (la CI, les
#                            contributeurs) : l'arbre seul, identité non
#                            contrôlée — une contribution garde le nom de son
#                            auteur.
#   SERMO_BANC_DELAI         secondes avant d'arrêter un banc figé (défaut : 1800)
# Codes : 0 = tout est vert · 1 = au moins un banc rouge, figé ou qui n'a rien vérifié

set -uo pipefail
RACINE=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
# Les bancs graphiques appellent xvfb-run : tests/outils/xvfb-run passe devant le
# vrai (écran choisi et attendu par Xvfb lui-même, éteint et attendu à la fin) —
# voir l'en-tête du lanceur, et le 2026-09-30 dans tests/run_examples.sh.
export PATH="$RACINE/tests/outils:$PATH"
cd "$RACINE" || exit 1
J="${SERMO_JOURNAUX:-$RACINE/_journaux}/bancs"
DELAI="${SERMO_BANC_DELAI:-1800}"   # secondes par banc : un banc figé ne doit pas manger le job
mkdir -p "$J"
# Mêmes conditions qu'en CI : aucun affichage. Les gardes graphiques lancent
# leur propre Xvfb ; sans ça, sur un poste de travail, des fenêtres s'ouvriraient
# sur l'écran de la session et la mesure ne serait plus celle de la CI.
unset DISPLAY WAYLAND_DISPLAY
TOUS=(gtk3 gtk4 qt6 fltk1 efl1 sdl3 ncurses)
PORTS=("$@"); PARTIEL=1; [ ${#PORTS[@]} -gt 0 ] || { PORTS=("${TOUS[@]}"); PARTIEL=0; }
echecs=0; joues=0

ligne() {  # ligne <état> <nom> <texte> : colonnes alignées en caractères, pas en octets
    local larg; larg=$(printf '%s' "$2" | LC_ALL=C.UTF-8 wc -m)
    printf '%s %s%*s %s\n' "$1" "$2" $((larg < 40 ? 40 - larg : 0)) '' "$3"
}

banc() {  # banc <nom> <commande…> : journal complet, une ligne de verdict
    local nom="$1"; shift
    local journal="$J/$(printf '%s' "$nom" | LC_ALL=C.UTF-8 iconv -f UTF-8 -t ASCII//TRANSLIT | tr -c 'A-Za-z0-9.-' '_').log"
    timeout --kill-after=10 "$DELAI" "$@" > "$journal" 2>&1
    local rc=$?
    local fin; fin=$(grep -a -v '^[[:space:]]*$' "$journal" | tail -1 | sed 's/\x1b\[[0-9;]*m//g' | cut -c1-80)
    joues=$((joues + 1))
    if [ "$rc" -eq 0 ]; then
        ligne 'OK    ' "$nom" "$fin"
    elif [ "$rc" -eq 77 ]; then
        # 77 = « rien n'a été vérifié ». Ici tout est censé être là : c'est un échec.
        ligne 'IGNORÉ' "$nom" "compté en échec — $fin"
        echecs=$((echecs + 1))
    elif [ "$rc" -eq 124 ] || [ "$rc" -eq 137 ]; then
        ligne 'ÉCHEC ' "$nom" "figé, arrêté après ${DELAI}s"
        echecs=$((echecs + 1))
    else
        ligne 'ÉCHEC ' "$nom" "rc=$rc — $fin"
        echecs=$((echecs + 1))
    fi
}

echo "Ports : ${PORTS[*]} · langue : ${LC_ALL:-${LANG:-C}} · délai par banc : ${DELAI}s"
echo "── Sources et cœur"
banc "tests unitaires du cœur" bash tests/run_unit_tests.sh
banc "version unique" bash tests/garde_version.sh
banc "liens de langue" bash tests/garde_liens_langue.sh
for d in libsermocore contract tests examples sermo-backend-*; do
    banc "spdx $d" bash tests/garde_spdx.sh "$d"
done
for d in libsermocore/src libsermocore/src-gtk4 libsermocore/include contract sermo-backend-*/src; do
    banc "fonctions interdites $d" bash tests/garde_fonctions_interdites.sh "$d"
done
for p in gtk3 gtk4; do
    banc "thread progressbar $p" bash tests/garde_progressbar_thread.sh "sermo-backend-$p/src/widget_progressbar.c"
done
banc "en-têtes de poste" bash tests/garde_pas_de_poste.sh "$RACINE"
banc "lectures d'<input> plafonnées" bash tests/garde_lecture_input.sh "$RACINE"
if [ "${SERMO_BANC_PUBLICATION:-0}" = 1 ]; then
    banc "façade avant publication (arbre)" bash tests/verifie-facade.sh --strict
    banc "façade avant publication (historique)" bash tests/verifie-facade.sh --strict --historique HEAD
else
    banc "façade (arbre)" bash tests/verifie-facade.sh --sans-identite
fi

declare -A VARIANTES=()
for p in "${PORTS[@]}"; do
    case "$p" in gtk3) VARIANTES[_build_gtk]=1 ;; gtk4) VARIANTES[_build_gtk4]=1 ;; *) VARIANTES[_build]=1 ;; esac
done
for v in "${!VARIANTES[@]}"; do
    banc "fortify cœur $v" bash tests/garde_fortify_coeur.sh "libsermocore/$v/libsermocore.a"
done

for p in "${PORTS[@]}"; do
    echo "── $p"
    b="$RACINE/sermo-backend-$p/_build/${p}sermo"
    if [ ! -x "$b" ]; then
        ligne 'ÉCHEC ' "$p" "binaire absent : $b"; echecs=$((echecs + 1)); continue
    fi
    banc "$p xml" env TIMEOUT=10 bash tests/xml/run_tests.sh "$b"
    banc "$p identité" bash tests/garde_identite_port.sh "$b"
    # Taille de fenêtre : tous les ports graphiques. ncurses est un terminal,
    # il n'a pas de fenêtre à dimensionner. qt6 en était écarté le temps que
    # son --render-png cesse d'ignorer la taille demandée — c'est réglé.
    case "$p" in
        ncurses) ;;
        *) banc "$p taille de fenêtre" bash tests/garde_taille_fenetre.sh "$b" ;;
    esac
    banc "$p calendrier" bash tests/garde_calendrier_date_du_jour.sh "$b"
    banc "$p durcissement" bash tests/garde_durcissement.sh "$b"
    banc "$p allowed_cmds" bash tests/garde_allowed_cmds.sh "$b"
    banc "$p source sans fin" bash tests/garde_input_sans_fin.sh "$b"
    banc "$p option --do" bash tests/garde_option_do.sh "$b"
    banc "$p maxwidgets" bash tests/garde_maxwidgets.sh "$b"
    banc "$p include" bash tests/garde_include.sh "$b"
    banc "$p glade" bash tests/garde_glade.sh "$b"
    banc "$p terminal" bash tests/garde_terminal.sh "$b"
    [ "$p" = gtk3 ] && banc "$p layer-shell" bash tests/garde_layer_shell.sh "$b"
    if [ "$p" = ncurses ]; then
        banc "$p mot de passe" bash tests/garde_ncurses_mot_de_passe.sh "$b"
    else
        banc "$p échappement" bash tests/garde_echappement_sortie.sh "$b"
        banc "$p clic widgets" bash tests/garde_clic_widgets.sh "$b"
        banc "$p exemples réels" bash tests/run_examples.sh "$b"
    fi
done

echo "── Transverses"
if [ "$PARTIEL" -eq 1 ]; then
    banc "comportement (ports construits)" env SERMO_PORTS_OPTIONNELS=1 bash tests/comportement/run_all.sh
else
    banc "comportement (sept ports)" bash tests/comportement/run_all.sh
fi
banc "sermoman-mcp : vérité" env SERMO_SRC="$RACINE" bash sermoman-mcp/tests/verifie-verite.sh
banc "sermoman-mcp : exemples" env SERMO_SRC="$RACINE" SERMO_PORTS_REQUIS="${PORTS[*]/%/sermo}" bash sermoman-mcp/tests/verifie-exemples.sh

echo
echo "Bancs : $joues joué(s), $echecs en échec. Journaux : $J"
[ "$echecs" -eq 0 ]
