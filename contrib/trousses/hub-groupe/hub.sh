#!/bin/bash
# SPDX-License-Identifier: CC0-1.0
# ============================================================================
# hub — conteneur forme A, groupé « système / admin / user » (esprit PowerToys)
#
# Découvre les modules de ./modules/ (chacun répond à --manifest et --check),
# les range PAR GROUPE en sections, affiche une pastille de SANTÉ par module
# (● prêt / ✕ prérequis manquant), et lance le choisi dans son propre
# processus. L'entreprise peut piloter l'exposition via ./groupes.conf.
#
# Tronc commun TEXTE (charte §6bis) : liste + edit + boutons. Aucune image,
# aucun onglet — tout se rend en ncurses.
#
# UN TTY = UN SEUL TUI (charte §9) : module en fenêtre (graphique) ou au
# premier plan dans le TTY (terminal), puis retour au hub.
# ============================================================================

SCRIPT_PATH=$(realpath -- "$0")
HUBDIR="$(dirname "$SCRIPT_PATH")"
MODDIR="$HUBDIR/modules"
CONF="$HUBDIR/groupes.conf"

_champ() { sed -n "s/^$1: *//p" | head -1; }

# ── Politique d'entreprise (facultative) ─────────────────────────────────────
# ORDRE: <groupes espacés>     -> ordre d'affichage des sections
# ACTIFS: <noms espacés>       -> si présent, SEULS ces modules sont montrés
ORDRE_DEFAUT="système admin user autres"
GROUP_ORDER="$ORDRE_DEFAUT"
ACTIFS=""
if [ -f "$CONF" ]; then
    _o=$(sed -n 's/^ORDRE: *//p'  "$CONF" | head -1); [ -n "$_o" ] && GROUP_ORDER="$_o autres"
    ACTIFS=$(sed -n 's/^ACTIFS: *//p' "$CONF" | head -1)
fi

# Un module est-il exposé par la politique ?
_module_actif() {
    [ -z "$ACTIFS" ] && return 0
    local n; for n in $ACTIFS; do [ "$n" = "$1" ] && return 0; done
    return 1
}

# ── Santé d'un module : pastille texte ───────────────────────────────────────
_pastille() {
    if bash "$1" --check >/dev/null 2>&1; then printf '●  prêt'
    else printf '✕  %s' "$(bash "$1" --check 2>&1 | head -1)"; fi
}

# ── Résolution NOM -> chemin, ensemble FERMÉ re-scanné (sécurité) ─────────────
module_path_for() {
    local want="$1" f nom
    for f in "$MODDIR"/*.sh; do
        [ -e "$f" ] || continue
        nom=$(bash "$f" --manifest 2>/dev/null | _champ NOM)
        [ "$nom" = "$want" ] && { printf '%s' "$f"; return 0; }
    done
    return 1
}

# ── --print pour les widgets ─────────────────────────────────────────────────
if [[ "${1:-}" == "--print" ]]; then
    case "${2:-}" in
      list)
        # Pour chaque groupe dans l'ordre : un en-tête, puis ses modules.
        # Chaque ligne module commence par le NOM (1er mot) : le résolveur
        # prend le 1er token ; les en-têtes commencent par ── -> ignorés.
        for g in $GROUP_ORDER; do
            section_vide=1; buf=""
            for f in "$MODDIR"/*.sh; do
                [ -e "$f" ] || continue
                man=$(bash "$f" --manifest 2>/dev/null)
                nom=$(printf '%s' "$man" | _champ NOM)
                grp=$(printf '%s' "$man" | _champ GROUPE); grp="${grp:-autres}"
                [ "$grp" = "$g" ] || continue
                _module_actif "$nom" || continue
                section_vide=0
                root=$(printf '%s' "$man" | _champ BESOIN_ROOT)
                tag=""; [ "$root" = "oui" ] && tag=" (root)"
                buf="$buf$(printf '%-16s %s%s\n' "$nom" "$(_pastille "$f")" "$tag")
"
            done
            # N'afficher une section que si elle existe dans l'ordre ; vide = note.
            printf '──  %s  ──\n' "$g"
            if [ "$section_vide" -eq 1 ]; then printf '   (aucun module)\n'; else printf '%s' "$buf"; fi
        done
        ;;
      desc)
        want=$(printf '%s' "${MODULE:-}" | awk '{print $1}')
        if [ -z "$want" ] || [ "$want" = "──" ] || [ "$want" = "(aucun" ]; then
            echo "Choisissez un module (pas un titre de section)."
        else
            f=$(module_path_for "$want")
            if [ -z "$f" ]; then echo "Module inconnu : $want"
            else
                man=$(bash "$f" --manifest 2>/dev/null)
                printf 'Module  : %s\n' "$(printf '%s' "$man" | _champ NOM)"
                printf 'Groupe  : %s\n' "$(printf '%s' "$man" | _champ GROUPE)"
                printf 'Root    : %s\n' "$(printf '%s' "$man" | _champ BESOIN_ROOT)"
                printf 'Env     : %s\n' "$(printf '%s' "$man" | _champ ENV)"
                printf 'Santé   : %s\n' "$(_pastille "$f")"
                # Avertissement de privilège, non bloquant.
                if [ "$(printf '%s' "$man" | _champ BESOIN_ROOT)" = "oui" ] \
                   && [ "$(id -u)" -ne 0 ] && ! sudo -n true 2>/dev/null; then
                    echo '⚠  droits root non acquis : une élévation sera demandée au lancement.'
                fi
                echo '----------------------------------------'
                printf '%s\n' "$(printf '%s' "$man" | _champ DESCRIPTION)"
            fi
        fi
        ;;
    esac
    exit 0
fi

# ── Détection du binaire (fallback automatique) ──────────────────────────────
DIALOG_DEMANDE="${1:-}"
if [ -n "$DIALOG_DEMANDE" ]; then
    DIALOG="$DIALOG_DEMANDE"
elif [ -z "${DISPLAY:-}${WAYLAND_DISPLAY:-}" ] && command -v ncursessermo >/dev/null; then
    DIALOG=ncursessermo
else
    for b in sermo gtk3sermo gtk4sermo qt6sermo fltk1sermo efl1sermo sdl3sermo lvglsermo ncursessermo gtkdialog; do
        command -v "$b" >/dev/null 2>&1 && { DIALOG="$b"; break; }
    done
fi
[ -n "${DIALOG:-}" ] || { echo "Aucun binaire sermo/gtkdialog trouvé." >&2; exit 1; }
[ -d "$MODDIR" ] || { echo "Dossier modules/ absent à côté du hub." >&2; exit 1; }

choisir() {
    export MAIN_DIALOG
    MAIN_DIALOG=$(cat <<XMLEOF
<window title="Hub — boîte à outils">
  <vbox>
    <text><label>Outils (par groupe) :</label></text>
    <hbox>
      <list>
        <variable>MODULE</variable>
        <width>320</width><height>300</height>
        <input>bash "$SCRIPT_PATH" --print list</input>
        <action function="Refresh">DESC</action>
      </list>
      <edit>
        <variable>DESC</variable>
        <width>420</width><height>300</height>
        <input>bash "$SCRIPT_PATH" --print desc</input>
      </edit>
    </hbox>
    <hbox>
      <button><label>Lancer</label><action>exit:lancer</action></button>
      <button><label>Rafraîchir</label><action function="Refresh">MODULE</action><action function="Refresh">DESC</action></button>
      <button><label>Quitter</label><action>exit:quit</action></button>
    </hbox>
  </vbox>
</window>
XMLEOF
)
    "$DIALOG" --program=MAIN_DIALOG --center
}

while : ; do
    sortie=$(choisir)
    exitv=$(printf '%s\n' "$sortie"  | sed -n 's/^EXIT="\(.*\)"$/\1/p'   | tail -1)
    module=$(printf '%s\n' "$sortie" | sed -n 's/^MODULE="\(.*\)"$/\1/p' | tail -1)
    [ "$exitv" = "lancer" ] || break
    want=$(printf '%s' "$module" | awk '{print $1}')
    { [ -z "$want" ] || [ "$want" = "──" ] || [ "$want" = "(aucun" ]; } && continue
    path=$(module_path_for "$want") || { echo "Module inconnu : $want" >&2; continue; }
    if [ -n "${DISPLAY:-}${WAYLAND_DISPLAY:-}" ]; then bash "$path" & else bash "$path"; fi
done
