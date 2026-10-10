#!/bin/bash
# SPDX-License-Identifier: CC0-1.0
# ============================================================================
# info — infos système en lecture seule, front-end uname/uptime/free/df
#
# PUBLIC        : admin-devops
# POURQUOI LUI  : un coup d'œil système, en fenêtre comme en terminal SSH.
# CE QUE SEUL
#   SERMO DONNE : la même vue en GTK et en ncurses.
# ENROBE        : uname, uptime, free, df (lecture seule, aucune élévation).
# BACKENDS      : tous.
# DÉGRADATION
#   NCURSES     : aucune — c'est du texte, rien de pictural.
# CIBLE         : tout Linux.
# DÉPENDANCES   : sermo (alias gtkdialog accepté).
# ============================================================================

# ── Contrat de module (charte §8bis) ────────────────────────────────────────
if [[ "${1:-}" == "--manifest" ]]; then
    cat <<'EOF'
NOM: info
DESCRIPTION: Infos système (noyau, charge, mémoire, disque) — lecture seule
GROUPE: user
PUBLIC: user
BESOIN_ROOT: non
ENV: les_deux
BACKENDS: tous
EOF
    exit 0
fi

# --check : coreutils de base, disponible partout
if [[ "${1:-}" == "--check" ]]; then
    command -v uname >/dev/null 2>&1 && exit 0
    echo "coreutils requis"; exit 1
fi

SCRIPT_PATH=$(realpath -- "$0")

# ── --print : producteur pur (aucune entrée utilisateur -> aucune garde) ─────
if [[ "${1:-}" == "--print" ]]; then
    case "${2:-}" in
        info)
            echo "=== Noyau ==="    ; uname -a
            echo ; echo "=== Charge / uptime ===" ; uptime
            echo ; echo "=== Mémoire ==="  ; free -h
            echo ; echo "=== Disques ===" ; df -h
            ;;
        *) echo "Sortie inconnue : ${2:-}" ;;
    esac
    exit 0
fi

# ── Lancement (détection + fallback automatique, cf. charte) ─────────────────
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

export MAIN_DIALOG
MAIN_DIALOG=$(cat <<XMLEOF
<window title="info — système">
  <vbox>
    <edit>
      <variable>INFOS</variable>
      <width>560</width><height>340</height>
      <input>bash "$SCRIPT_PATH" --print info</input>
    </edit>
    <hbox>
      <button>
        <label>Rafraîchir</label>
        <action function="Refresh">INFOS</action>
      </button>
      <button cancel></button>
    </hbox>
  </vbox>
</window>
XMLEOF
)

"$DIALOG" --program=MAIN_DIALOG --center
