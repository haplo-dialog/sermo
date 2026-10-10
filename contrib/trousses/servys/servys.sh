#!/bin/bash
# SPDX-License-Identifier: CC0-1.0
# ============================================================================
# servys — gestionnaire de services, front-end systemctl
#
# PUBLIC        : admin / devops
# POURQUOI LUI  : voir et piloter les services systemd depuis une fenêtre en
#                 local, et depuis la MÊME interface en ncurses via SSH.
# CE QUE SEUL
#   SERMO DONNE : le même front-end en GTK et dans une session terminal — là où
#                 un outil GTK (ou yad/zenity) meurt sans $DISPLAY.
# ENROBE        : systemctl (jamais réimplémenté).
# BACKENDS      : brille en GTK/Qt/… et en ncurses ; aucune dégradation de fond.
# DÉGRADATION
#   NCURSES     : rien de pictural n'est perdu (il n'y en a pas) ; les états
#                 sont des symboles texte. L'élévation passe de pkexec (fenêtre)
#                 à doas/sudo dans le TTY (terminal) — voir run_as_root.
# CIBLE         : système en systemd. Sinon, refus propre au lancement.
# DÉPENDANCES   : sermo (alias gtkdialog accepté), systemctl, et pour agir :
#                 pkexec OU doas OU sudo (selon le contexte).
# ============================================================================

# ── Contrat de module : fiche lisible par le hub (charte §8bis) ──────────────
if [[ "${1:-}" == "--manifest" ]]; then
    cat <<'EOF'
NOM: servys
DESCRIPTION: Gestion des services systemd (start / stop / restart / état)
GROUPE: admin
PUBLIC: admin-devops
BESOIN_ROOT: oui
ENV: les_deux
BACKENDS: tous
EOF
    exit 0
fi

# --check : utilisable sur CETTE machine ? (santé affichee par le hub)
if [[ "${1:-}" == "--check" ]]; then
    if command -v systemctl >/dev/null 2>&1 && [ -d /run/systemd/system ]; then
        exit 0
    fi
    echo "systemd requis (systemctl absent ou systeme non-systemd)"
    exit 1
fi

# ── §2 Refus honnête si pas de systemd ──────────────────────────────────────
if ! command -v systemctl >/dev/null 2>&1 || [ ! -d /run/systemd/system ]; then
    echo "servys requiert un système démarré avec systemd." >&2
    echo "Init détecté : $(ps -p 1 -o comm= 2>/dev/null || echo inconnu)" >&2
    exit 1
fi

SCRIPT_PATH=$(realpath -- "$0")

# ── §4 Garde : le nom d'unité est validé par motif ET par appartenance ───────
# Liste fermée = l'ensemble réel des unités .service, revérifié ici même si la
# sélection vient déjà d'une liste non éditable (charte §4.2 : écrit deux fois).
svc_valide() {
    local s="$1"
    [[ "$s" =~ ^[a-zA-Z0-9@._-]+\.service$ ]] || return 1
    systemctl list-unit-files --type=service --no-legend --no-pager 2>/dev/null \
        | awk '{print $1}' | grep -qxF -- "$s"
}

# ── §5 Élévation adaptative : pkexec en fenêtre, doas/sudo dans un TTY ────────
run_as_root() {
    if [ "$(id -u)" -eq 0 ]; then
        "$@"
    elif [ -n "${DISPLAY:-}${WAYLAND_DISPLAY:-}" ] && command -v pkexec >/dev/null; then
        pkexec "$@"
    elif command -v doas >/dev/null; then
        doas "$@"
    else
        sudo "$@"
    fi
}

# ── §6bis État en symbole TEXTE (jamais une icône) ──────────────────────────
etat_symbole() {
    case "$1" in
        active)                 printf '●  actif' ;;
        inactive)               printf '○  inactif' ;;
        failed)                 printf '✕  échec' ;;
        activating|deactivating) printf '◐  %s' "$1" ;;
        *)                      printf '·  %s' "$1" ;;
    esac
}

# ── §3 --print : producteurs PURS sur stdout, pour alimenter un <input> ──────
if [[ "${1:-}" == "--print" ]]; then
    case "${2:-}" in
        list)
            # Noms d'unités propres, un par ligne (la sélection = un nom net).
            systemctl list-unit-files --type=service --no-legend --no-pager 2>/dev/null \
                | awk '{print $1}' | sort -u
            ;;
        status)
            # $SVC vient de l'environnement (exporté par sermo au clic).
            s="${SVC:-}"
            if [ -z "$s" ]; then
                echo "Sélectionnez un service dans la liste."
            elif ! svc_valide "$s"; then
                echo "Unité non reconnue : $s"
            else
                act=$(systemctl is-active "$s" 2>/dev/null)
                ena=$(systemctl is-enabled "$s" 2>/dev/null)
                printf 'État   : %s\n' "$(etat_symbole "$act")"
                printf 'Au boot: %s\n' "${ena:-inconnu}"
                echo   '----------------------------------------'
                systemctl status --no-pager -n 20 -- "$s" 2>&1
            fi
            ;;
        *) echo "Sortie inconnue : ${2:-}" ;;
    esac
    exit 0
fi

# ── §3 --do : effets de bord ; $SVC lu dans l'environnement ──────────────────
if [[ "${1:-}" == "--do" ]]; then
    action="${2:-}"
    s="${SVC:-}"
    case "$action" in
        start|stop|restart)
            if svc_valide "$s"; then
                run_as_root systemctl "$action" -- "$s"
            fi
            ;;
        *) echo "Action inconnue : $action" >&2; exit 2 ;;
    esac
    exit 0
fi

# ── Lancement : port demandé en 1er argument, sinon détection ────────────────
# Fallback automatique : sans affichage, on prend ncurses s'il est là.
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

# ── §6bis Tronc commun : TEXTE uniquement, aucune image ──────────────────────
export MAIN_DIALOG
MAIN_DIALOG=$(cat <<XMLEOF
<window title="servys — services systemd">
  <vbox>
    <hbox>
      <text><label>Services (.service) :</label></text>
    </hbox>
    <hbox>
      <list>
        <variable>SVC</variable>
        <width>340</width><height>300</height>
        <input>bash "$SCRIPT_PATH" --print list</input>
        <action function="Refresh">STATUT</action>
      </list>
      <edit>
        <variable>STATUT</variable>
        <width>420</width><height>300</height>
        <input>bash "$SCRIPT_PATH" --print status</input>
      </edit>
    </hbox>
    <hbox>
      <button>
        <label>Démarrer</label>
        <action>bash "$SCRIPT_PATH" --do start</action>
        <action function="Refresh">STATUT</action>
      </button>
      <button>
        <label>Arrêter</label>
        <action>bash "$SCRIPT_PATH" --do stop</action>
        <action function="Refresh">STATUT</action>
      </button>
      <button>
        <label>Redémarrer</label>
        <action>bash "$SCRIPT_PATH" --do restart</action>
        <action function="Refresh">STATUT</action>
      </button>
      <button>
        <label>Rafraîchir</label>
        <action function="Refresh">SVC</action>
        <action function="Refresh">STATUT</action>
      </button>
    </hbox>
    <hbox>
      <button cancel></button>
    </hbox>
  </vbox>
</window>
XMLEOF
)

# §4.3 PAS de set -e : sermo renvoie un code non nul à la fermeture normale.
"$DIALOG" --program=MAIN_DIALOG --center
