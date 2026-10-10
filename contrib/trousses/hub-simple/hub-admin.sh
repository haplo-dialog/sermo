#!/bin/bash
# SPDX-License-Identifier: CC0-1.0
# ============================================================================
# hub-admin — conteneur forme A (lanceur) pour la trousse admin-devops
#
# Découvre les modules de ./modules/ (chacun répond à --manifest), les liste,
# et lance celui qu'on choisit DANS SON PROPRE PROCESSUS. Un module qui plante
# ne tue pas le hub ; chaque module garde ses propres gardes de sécurité.
#
# UN TTY = UN SEUL TUI (charte §9) : en session graphique, le module s'ouvre en
# fenêtre à part et le hub se rouvre ; en terminal, le hub cède le TTY au module
# (au premier plan), puis revient à la liste quand le module se ferme.
#
# DÉPENDANCES : sermo (alias gtkdialog accepté). Aucun privilège : le hub ne
#               fait que lister et lancer.
# ============================================================================

SCRIPT_PATH=$(realpath -- "$0")
MODDIR="$(dirname "$SCRIPT_PATH")/modules"

# ── Lecture d'un champ de manifest, proprement ───────────────────────────────
_champ() { sed -n "s/^$1: *//p" | head -1; }

# ── Résolution NOM -> chemin, sur l'ensemble FERMÉ re-scanné (sécurité) ───────
# On ne lance JAMAIS un chemin dérivé du texte : seulement un .sh trouvé sur le
# disque dont le manifest NOM correspond exactement au choix.
module_path_for() {
    local want="$1" f nom
    for f in "$MODDIR"/*.sh; do
        [ -e "$f" ] || continue
        nom=$(bash "$f" --manifest 2>/dev/null | _champ NOM)
        [ "$nom" = "$want" ] && { printf '%s' "$f"; return 0; }
    done
    return 1
}

# ── --print pour les widgets du sélecteur ────────────────────────────────────
if [[ "${1:-}" == "--print" ]]; then
    case "${2:-}" in
        list)
            for f in "$MODDIR"/*.sh; do
                [ -e "$f" ] || continue
                bash "$f" --manifest 2>/dev/null | _champ NOM
            done | sort -u
            ;;
        desc)
            want="${MODULE:-}"
            if [ -z "$want" ]; then
                echo "Choisissez un module dans la liste."
            else
                for f in "$MODDIR"/*.sh; do
                    [ -e "$f" ] || continue
                    man=$(bash "$f" --manifest 2>/dev/null)
                    if [ "$(printf '%s' "$man" | _champ NOM)" = "$want" ]; then
                        printf 'Module : %s\n' "$(printf '%s' "$man" | _champ NOM)"
                        printf 'Public : %s\n' "$(printf '%s' "$man" | _champ PUBLIC)"
                        printf 'Backends: %s\n' "$(printf '%s' "$man" | _champ BACKENDS)"
                        echo '----------------------------------------'
                        printf '%s\n' "$(printf '%s' "$man" | _champ DESCRIPTION)"
                        exit 0
                    fi
                done
                echo "Module inconnu : $want"
            fi
            ;;
    esac
    exit 0
fi

# ── Détection du binaire (même logique de fallback que les modules) ──────────
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

# ── Le sélecteur : une fenêtre qui se ferme en renvoyant le choix ────────────
# Tronc commun TEXTE (charte §6bis). "Lancer" positionne EXIT=lancer et sort ;
# gtkdialog imprime alors MODULE="..." et EXIT="..." sur stdout, qu'on relit.
choisir() {
    export MAIN_DIALOG
    MAIN_DIALOG=$(cat <<XMLEOF
<window title="Hub admin — modules">
  <vbox>
    <text><label>Modules disponibles :</label></text>
    <hbox>
      <list>
        <variable>MODULE</variable>
        <width>280</width><height>260</height>
        <input>bash "$SCRIPT_PATH" --print list</input>
        <action function="Refresh">DESC</action>
      </list>
      <edit>
        <variable>DESC</variable>
        <width>420</width><height>260</height>
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
    # stdout capturé = le dump des variables ; l'UI s'affiche sur le terminal.
    "$DIALOG" --program=MAIN_DIALOG --center
}

# ── Boucle extérieure : choisir -> lancer -> revenir (charte §9) ─────────────
while : ; do
    sortie=$(choisir)
    exitv=$(printf '%s\n' "$sortie"  | sed -n 's/^EXIT="\(.*\)"$/\1/p'   | tail -1)
    module=$(printf '%s\n' "$sortie" | sed -n 's/^MODULE="\(.*\)"$/\1/p' | tail -1)

    [ "$exitv" = "lancer" ] || break            # Quitter / Annuler / fermeture
    [ -n "$module" ] || continue

    path=$(module_path_for "$module") || { echo "Module inconnu : $module" >&2; continue; }

    if [ -n "${DISPLAY:-}${WAYLAND_DISPLAY:-}" ]; then
        bash "$path" &                          # graphique : fenêtre à part
    else
        bash "$path"                            # terminal : même TTY, au 1er plan
    fi
done
