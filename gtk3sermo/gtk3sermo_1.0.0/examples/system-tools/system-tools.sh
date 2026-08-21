#!/bin/bash
# ============================================================================
# Outils Système, exemple complet pour gtk3sermo
#
# Version   : 4.0
# Interface : gtk3sermo 1.0.0 (alias gtkdialog accepté)
# Cible     : Debian et dérivées UNIQUEMENT (apt, dpkg, journalctl)
# Licence   : CC0-1.0, domaine public, comme le reste de examples/
#
# Ce que cet exemple démontre, au-delà des cinq repères visuels :
#   - une fenêtre qui affiche de VRAIES données système, relevées au lancement ;
#   - le motif « --do » : le script se rappelle lui-même pour exécuter une
#     action, et les valeurs des widgets arrivent par l'environnement. C'est la
#     façon propre de programmer avec gtk3sermo, sans « export -f » ;
#   - six onglets, une barre de progression liée à la mémoire réelle, des
#     icônes prises dans le thème de l'utilisateur, un champ de recherche.
#
# SÉCURITÉ, ce qui est réellement fait :
#   - toute élévation passe par pkexec (PolicyKit). Aucun sudo dans une
#     interface graphique, nulle part ;
#   - l'aide en ligne n'accepte qu'une commande figurant dans une liste
#     blanche, comparée à l'identique (pas de motif) ;
#   - l'onglet Fichiers ne travaille que sur une liste FERMÉE de chemins,
#     revérifiée avant toute ouverture, y compris en root ;
#   - « set -e » et « trap ERR » sont volontairement absents : gtk3sermo
#     renvoie un code non nul à la fermeture normale de la fenêtre.
#
# Dépendances : gtk3sermo, pkexec, et un terminal + un éditeur graphique
#               (détectés automatiquement parmi ceux présents).
# ============================================================================

# ── Debian seulement, et on le dit franchement ──────────────────────────────
# Cet exemple appelle apt, dpkg, journalctl et deborphan. Ailleurs il
# afficherait des onglets vides ou des erreurs : autant refuser proprement.
if [ ! -e /etc/debian_version ]; then
    echo "Cet exemple est prévu pour Debian et ses dérivées (apt, dpkg)." >&2
    echo "Système détecté : $( (. /etc/os-release 2>/dev/null && echo "$PRETTY_NAME") || uname -s )" >&2
    exit 1
fi

SCRIPT_PATH=$(realpath -- "$0")
SCRIPT_DIR="$(dirname "$SCRIPT_PATH")"

# ── Logo du système ────────────────────────────────────────────────────────
# On affiche le logo de Debian, pas celui d'un projet : c'est l'OS que cette
# fenêtre décrit. Le fichier est fourni par debconf, présent sur toute Debian.
# On ne copie rien dans /tmp (l'ancienne version le faisait, ce qui ouvrait
# une fenêtre de tir sur un nom prévisible) et on ne distribue aucun logo.
export LOGO_XML
if [ -r /usr/share/pixmaps/debian-logo.png ]; then
    LOGO_XML='<image file="/usr/share/pixmaps/debian-logo.png" width="48" height="48"></image>'
else
    # Repli sur le thème d'icônes, puis sur une icône générique toujours là.
    LOGO_XML='<image icon-name="debian-logo" icon-size="dialog"></image>'
fi

# ── Terminal et éditeur : on prend ce qui est installé ──────────────────────
# L'ancienne version exigeait xfce4-terminal et mousepad : sur une Debian avec
# un autre bureau, la moitié des boutons ne faisaient rien. On détecte.
TERMINAL=""
for _t in x-terminal-emulator xfce4-terminal gnome-terminal konsole mate-terminal \
          lxterminal xterm; do
    command -v "$_t" >/dev/null 2>&1 && { TERMINAL="$_t"; break; }
done
unset _t

EDITEUR=""
for _e in mousepad gedit kate pluma gnome-text-editor xed leafpad; do
    command -v "$_e" >/dev/null 2>&1 && { EDITEUR="$_e"; break; }
done
unset _e
export TERMINAL EDITEUR

# Affiche un fichier texte : éditeur graphique si présent, sinon terminal.
afficher() {
    if [ -n "$EDITEUR" ]; then
        "$EDITEUR" "$1" &
    elif [ -n "$TERMINAL" ]; then
        "$TERMINAL" -e "less $1" &
    else
        echo "Aucun éditeur ni terminal graphique trouvé." >&2
    fi
}

# Lance une commande dans un terminal et laisse la fenêtre ouverte à la fin.
dans_terminal() {
    [ -n "$TERMINAL" ] || { echo "Aucun terminal graphique trouvé." >&2; return 1; }
    ST_CMD="$1" "$TERMINAL" -e "bash -c 'eval \"\$ST_CMD\"; echo; echo \"[Entrée pour fermer]\"; read -r _'" &
}

export LC_ALL=C
export DISPLAY="${DISPLAY:-:0}"
export XAUTHORITY="${XAUTHORITY:-$HOME/.Xauthority}"

# ── Liste blanche de commandes autorisées pour l'aide inline ────────────────

readonly ALLOWED_CMDS=(
    apt apt-get apt-cache aptitude
    cat cp df du dmesg find free grep
    htop id ifconfig ip journalctl
    last less lsb_release lsblk lsmod lspci lsusb lshw
    mount netstat ping ps ss
    systemctl top traceroute uname uptime w who
)

cmd_allowed() {
    local cmd="$1"
    # Accepter uniquement un nom de commande simple (pas de métacaractères)
    if [[ ! "$cmd" =~ ^[a-zA-Z][a-zA-Z0-9_-]*$ ]]; then
        echo "ERREUR : commande invalide (caractères non autorisés : '$cmd')"
        return 1
    fi
    local c
    for c in "${ALLOWED_CMDS[@]}"; do
        [[ "$cmd" == "$c" ]] && return 0
    done
    echo "ERREUR : commande non autorisée : '$cmd'"
    echo "Commandes autorisées : ${ALLOWED_CMDS[*]}"
    return 1
}

path_valid() {
    local p="$1"
    if [[ -z "$p" ]]; then
        echo "ERREUR : chemin vide"; return 1
    fi
    # Interdire les métacaractères shell dans les chemins de fichiers
    if [[ "$p" =~ [';|&`$<>(){}'] ]]; then
        echo "ERREUR : chemin invalide (caractères dangereux détectés)"; return 1
    fi
    return 0
}

# ── Lecture du tampon noyau ─────────────────────────────────────────────────
# Sur Debian, kernel.dmesg_restrict vaut 1 par defaut : « dmesg » echoue pour
# un utilisateur ordinaire, et l'ancienne version affichait un cadre vide.
# journalctl -k donne la meme chose aux membres du groupe adm ou
# systemd-journal. On essaie l'un, puis l'autre, puis on explique.
noyau_lignes() {
    local n="${1:-20}"
    if dmesg --color=never 2>/dev/null | tail -n "$n" | grep -q .; then
        dmesg --color=never 2>/dev/null | tail -n "$n"
    elif journalctl -k -n "$n" --no-pager --no-hostname 2>/dev/null | grep -q .; then
        journalctl -k -n "$n" --no-pager --no-hostname 2>/dev/null
    else
        echo "Le tampon du noyau n'est pas lisible par cet utilisateur."
        echo "Debian protege dmesg par defaut (kernel.dmesg_restrict = 1)."
        echo "Deux facons d'y avoir acces :"
        echo "  - appartenir au groupe adm ou systemd-journal (journalctl -k) ;"
        echo "  - ou lancer cet outil en root."
    fi
}

# Sortie sur stdout, pour alimenter un <input> de gtk3sermo.
if [[ "${1:-}" == "--print" ]]; then
    case "${2:-}" in
        noyau) noyau_lignes "${3:-20}" ;;
        *)     echo "Sortie inconnue : ${2:-}" ;;
    esac
    exit 0
fi

# ── Élévation de privilèges ─────────────────────────────────────────────────

run_as_root() {
    if [ "$(id -u)" -eq 0 ]; then
        "$@"
    else
        pkexec env DISPLAY="$DISPLAY" XAUTHORITY="$XAUTHORITY" "$@"
    fi
}

# ── Liste FERMÉE des fichiers que l'onglet Fichiers accepte ─────────────────
# La liste déroulante n'est pas éditable : l'utilisateur ne peut choisir que
# ces chemins. On revérifie quand même ici, à l'identique. Un jour où
# quelqu'un remplacera le widget par un champ libre, cette garde tiendra
# encore, et c'est tout l'intérêt de l'écrire deux fois.
readonly FICHIERS_AUTORISES=(
    /etc/fstab
    /etc/apt/sources.list
    /etc/network/interfaces
    /etc/systemd/system.conf
    /etc/ssh/sshd_config
    /etc/X11/xorg.conf
    "$HOME/.bashrc"
    /root/.bashrc
)

fichier_autorise() {
    local f="$1" c
    for c in "${FICHIERS_AUTORISES[@]}"; do
        [[ "$f" == "$c" ]] && return 0
    done
    echo "ERREUR : ce chemin ne fait pas partie de la liste autorisee : '$f'"
    return 1
}

# Édition d'un fichier système. On n'ouvre un éditeur graphique en root que
# pour un chemin de la liste, jamais pour une saisie libre.
editer_root() {
    local f="$1"
    fichier_autorise "$f" || return 1
    [ -n "$EDITEUR" ] || { echo "Aucun editeur graphique trouve." >&2; return 1; }
    run_as_root "$EDITEUR" "$f"
}

# ── Gestionnaire d'actions gtkdialog (pattern --do) ─────────────────────────
#
# Quand gtkdialog exécute une <action>, il définit les variables de widgets
# comme variables d'environnement dans le sous-shell. Ce script est rappelé
# avec --do ACTION, et les variables ($KERNEL_SEARCH, $CMD_INPUT, etc.)
# sont directement disponibles via l'environnement.

handle_action() {
    local action="$1"
    local tmp
    tmp=$(mktemp /tmp/systools_XXXXXX.txt)

    case "$action" in

        # ── Onglet Système ──────────────────────────────────────────────────

        info-sys)
            {   echo "=== Distribution ==="
                lsb_release -a 2>/dev/null
                echo
                echo "=== Noyau ==="
                uname -a
                echo
                echo "=== Mémoire ==="
                free -h
                echo
                echo "=== Charge système ==="
                uptime
                echo
                echo "=== Disques ==="
                df -h
            } > "$tmp"
            afficher "$tmp" ;;

        terminal)
            test -n "$TERMINAL" && "$TERMINAL" &
            rm -f "$tmp" ;;

        terminal-root)
            run_as_root "$TERMINAL"
            rm -f "$tmp" ;;

        processes)
            dans_terminal "htop"
            rm -f "$tmp" ;;

        services)
            systemctl list-unit-files --type=service --no-pager 2>/dev/null \
                > "$tmp"
            afficher "$tmp" ;;

        search-dmesg)
            local term="${KERNEL_SEARCH:-}"
            if [ -n "$term" ]; then
                noyau_lignes 100000 | grep -i "$term" > "$tmp" 2>/dev/null \
                    || echo "Aucun resultat pour : $term" > "$tmp"
            else
                noyau_lignes 100 > "$tmp"
            fi
            afficher "$tmp" ;;

        dmesg-full)
            noyau_lignes 100000 > "$tmp"
            afficher "$tmp" ;;

        cmd-help)
            local cmd="${CMD_INPUT:-}"
            if cmd_allowed "$cmd" > "$tmp" 2>&1; then
                "$cmd" --help >> "$tmp" 2>&1 || true
            fi
            afficher "$tmp" ;;

        cmd-man)
            local cmd="${CMD_INPUT:-}"
            local errtmp
            errtmp=$(mktemp /tmp/systools_XXXXXX.txt)
            if cmd_allowed "$cmd" > "$errtmp" 2>&1; then
                dans_terminal "man $cmd"
                rm -f "$errtmp"
            else
                cat "$errtmp" > "$tmp"
                afficher "$tmp"
                rm -f "$errtmp"
            fi
            rm -f "$tmp" ;;

        # ── Onglet Matériel ─────────────────────────────────────────────────

        hardware-info)
            {   echo "=== lshw (résumé) ==="
                lshw -short 2>/dev/null || echo "lshw non disponible"
                echo
                echo "=== Bus PCI ==="
                lspci 2>/dev/null || echo "lspci non disponible"
                echo
                echo "=== Périphériques USB ==="
                lsusb 2>/dev/null || echo "lsusb non disponible"
                echo
                echo "=== Stockage ==="
                lsblk
            } > "$tmp"
            afficher "$tmp" ;;

        bios-info)
            {   echo "=== BIOS/UEFI ==="
                dmidecode -t bios 2>/dev/null \
                    || echo "dmidecode non disponible (droits root requis)"
                echo
                echo "=== Système ==="
                dmidecode -t system 2>/dev/null
            } > "$tmp"
            afficher "$tmp" ;;

        lsmod)
            lsmod > "$tmp" 2>/dev/null
            afficher "$tmp" ;;

        lsblk)
            lsblk --output NAME,SIZE,TYPE,MOUNTPOINT,FSTYPE > "$tmp" 2>/dev/null
            afficher "$tmp" ;;

        # ── Onglet Réseau ───────────────────────────────────────────────────

        network-info)
            {   echo "=== Interfaces réseau ==="
                ip addr show
                echo
                echo "=== Routes ==="
                ip route show
                echo
                echo "=== Résolution DNS ==="
                cat /etc/resolv.conf 2>/dev/null
                echo
                echo "=== Connexions actives (ss) ==="
                ss -tuln 2>/dev/null
            } > "$tmp"
            afficher "$tmp" ;;

        ss)
            ss -tuln > "$tmp" 2>/dev/null
            afficher "$tmp" ;;

        ping-test)
            dans_terminal "ping -c 4 8.8.8.8"
            rm -f "$tmp" ;;

        # ── Onglet Fichiers ─────────────────────────────────────────────────

        open-file)
            # editer_root refuse tout chemin hors de la liste fermee.
            local f="${SELECTED_FILE:-}"
            if path_valid "$f" > "$tmp" 2>&1; then
                if [ -e "$f" ]; then
                    editer_root "$f"
                    rm -f "$tmp"
                else
                    echo "Fichier introuvable : $f" > "$tmp"
                    afficher "$tmp"
                fi
            else
                afficher "$tmp"
            fi ;;

        view-file)
            # Meme garde que pour l'edition : la liste fermee d'abord. Afficher
            # est moins grave qu'editer, mais un exemple qui montre deux regles
            # differentes pour deux boutons voisins enseigne la mauvaise chose.
            local f="${SELECTED_FILE:-}"
            if fichier_autorise "$f" > "$tmp" 2>&1 && path_valid "$f" >> "$tmp" 2>&1; then
                if [ -r "$f" ]; then
                    afficher "$f"
                    rm -f "$tmp"
                else
                    echo "Fichier illisible ou inexistant : $f" > "$tmp"
                    afficher "$tmp"
                fi
            else
                afficher "$tmp"
            fi ;;

        # ── Onglet Journaux ─────────────────────────────────────────────────

        journal-search)
            local term="${JOURNAL_SEARCH:-}"
            local lines="${JOURNAL_LINES:-200}"
            if [ -n "$term" ]; then
                journalctl -n "$lines" --no-pager --no-hostname 2>/dev/null \
                    | cat | grep -i "$term" > "$tmp" \
                    || echo "Aucun résultat pour : $term" > "$tmp"
            else
                journalctl -n "$lines" --no-pager --no-hostname 2>/dev/null | cat > "$tmp"
            fi
            afficher "$tmp" ;;

        journal-errors)
            journalctl -p 3 -n 300 --no-pager --no-hostname 2>/dev/null | cat > "$tmp"
            afficher "$tmp" ;;

        journal-boot)
            journalctl -b --no-pager --no-hostname 2>/dev/null | cat > "$tmp"
            afficher "$tmp" ;;

        # ── Onglet Paquets ──────────────────────────────────────────────────

        pkg-update)
            dans_terminal "pkexec apt update && pkexec apt full-upgrade -y"
            rm -f "$tmp" ;;

        pkg-list)
            dpkg -l 2>/dev/null | grep "^ii" > "$tmp"
            afficher "$tmp" ;;

        pkg-orphans)
            {   deborphan 2>/dev/null \
                    || apt-get autoremove --dry-run 2>/dev/null \
                    || echo "deborphan non disponible"
            } > "$tmp"
            afficher "$tmp" ;;

        # ── À propos ────────────────────────────────────────────────────────

        about)
            cat > "$tmp" <<'ABOUT'
Outils Systeme, exemple complet pour gtk3sermo
===============================================
Version   : 4.0
Interface : gtk3sermo 1.0.0 (l'alias gtkdialog fonctionne aussi)
Cible     : Debian et derivees uniquement (apt, dpkg, journalctl)
Licence   : CC0-1.0, domaine public

Ce que cet exemple montre :
  Une fenetre qui affiche de VRAIES donnees systeme, relevees au lancement,
  et six onglets qui font un vrai travail d'administration.

Le motif « --do » :
  gtk3sermo execute une <action>, celle-ci rappelle ce meme script avec
  --do <action>, et les valeurs des widgets arrivent par l'environnement.
  Pas besoin d'exporter des fonctions bash, pas de fichier intermediaire.
  C'est la facon propre d'ecrire un programme gtk3sermo un peu serieux.

Widgets mis en avant :
  <searchentry>   champ de recherche avec sa loupe
  <comboboxtext>  liste deroulante non editable
  <image>         icones prises dans le theme de l'utilisateur
  <progressbar>   liee a la memoire reellement utilisee
  <edit>          zone remplie par la sortie d'une commande

Ce qui est reellement fait cote securite :
  - toute elevation passe par pkexec (PolicyKit). Aucun sudo graphique,
    nulle part, y compris pour la mise a jour des paquets ;
  - l'aide en ligne n'accepte qu'une commande de la liste blanche,
    comparee a l'identique, jamais par motif ;
  - l'onglet Fichiers ne travaille que sur une liste FERMEE de chemins,
    revérifiee avant chaque ouverture, y compris en root ;
  - aucun fichier n'est depose dans /tmp sous un nom previsible ;
  - set -e et trap ERR sont absents : gtk3sermo renvoie un code non nul
    a la fermeture normale de la fenetre.

Ce qu'il ne fait pas :
  Il ne fonctionne pas hors de Debian, et il le dit au lancement plutot
  que d'afficher des onglets vides.

Onglets :
  Systeme   terminal, htop, dmesg, recherche noyau, aide sur une commande
  Materiel  lshw, dmidecode, lsmod, lsblk
  Reseau    ip, ss, ping
  Fichiers  affichage et edition de fichiers systeme (root via pkexec)
  Journaux  journalctl et dmesg avec filtre
  Paquets   mise a jour, liste des paquets, paquets inutiles
ABOUT
            afficher "$tmp" ;;

        *)
            echo "Action inconnue : $action" > "$tmp"
            afficher "$tmp" ;;
    esac
}

# ── Données dynamiques (évaluées une seule fois au lancement) ───────────────

get_sysinfo() {
    DISTRO=$(lsb_release -ds 2>/dev/null || uname -s)
    KERNEL=$(uname -r)
    HOSTNAME=$(hostname)
    UPTIME=$(uptime -p 2>/dev/null | sed 's/^up //' || uptime | cut -d, -f1)
    CPU=$(grep "model name" /proc/cpuinfo 2>/dev/null \
              | head -1 | cut -d: -f2 | sed 's/^ *//' \
              | cut -c1-50)
    RAM_USED=$(free -h | awk '/^Mem:/{print $3}')
    RAM_TOTAL=$(free -h | awk '/^Mem:/{print $2}')
    RAM_PCT=$(free | awk '/^Mem:/{printf "%.0f", $3/$2*100}')
    DISK_FREE=$(df -h / 2>/dev/null | awk 'NR==2{print $4}')
    DISK_USED=$(df -h / 2>/dev/null | awk 'NR==2{print $5}')
    NET_IF=$(ip -brief addr show 2>/dev/null \
              | grep -v "^lo " | awk '$2=="UP"{print $1}' \
              | tr '\n' ' ' | sed 's/ $//')
    NET_IF="${NET_IF:-aucune}"
}

# ── Construction de l'interface gtkdialog ───────────────────────────────────

create_dialog() {
    get_sysinfo

    # Logo : LOGO_XML est calculé au niveau global (variable non-locale)
    # LOGO_FILE et LOGO_XML sont définis au démarrage du script, avant tout fork.

    export MAIN_DIALOG
    MAIN_DIALOG=$(cat <<XMLEOF
<window title="Outils Systeme : $HOSTNAME" window-position="1"
        resizable="true" default-width="1020" default-height="700">
  <vbox spacing="4" space-expand="true" space-fill="true">

    <!-- ═══════════════════════════════════════════════════════════════════
         EN-TETE : logo gauche + infos systeme + ressources
         ═══════════════════════════════════════════════════════════════════ -->
    <frame label="Systeme" space-expand="false" space-fill="false">
      <hbox spacing="10" homogeneous="false">

        <!-- Logo du systeme, colonne gauche fixe -->
        <vbox spacing="2" space-expand="false" space-fill="false">
          $LOGO_XML
          <text justify="center"><label>Debian</label></text>
        </vbox>

        <!-- Colonne infos systeme + ressources -->
        <vbox spacing="4" space-expand="true" space-fill="true">
          <hbox spacing="6" space-expand="false" space-fill="false">
            <image icon-name="computer" icon-size="menu"></image>
            <text><label>$DISTRO</label></text>
          </hbox>
          <hbox spacing="6" space-expand="false" space-fill="false">
            <image icon-name="applications-system" icon-size="menu"></image>
            <text><label>Noyau : $KERNEL</label></text>
          </hbox>
          <hbox spacing="6" space-expand="false" space-fill="false">
            <image icon-name="appointment-soon" icon-size="menu"></image>
            <text><label>Uptime : $UPTIME</label></text>
          </hbox>
          <hbox spacing="6" space-expand="false" space-fill="false">
            <image icon-name="cpu" icon-size="menu"></image>
            <text><label>CPU : $CPU</label></text>
          </hbox>
          <hbox spacing="6" space-expand="false" space-fill="false">
            <image icon-name="network-wired" icon-size="menu"></image>
            <text><label>Reseau : $NET_IF</label></text>
          </hbox>
          <hseparator></hseparator>
          <hbox spacing="6" space-expand="false" space-fill="false">
            <image icon-name="media-flash" icon-size="menu"></image>
            <text><label>RAM : $RAM_USED / $RAM_TOTAL</label></text>
          </hbox>
          <progressbar space-expand="true" space-fill="true">
            <variable>RAM_BAR</variable>
            <input>echo "$RAM_PCT"</input>
          </progressbar>
          <hbox spacing="6" space-expand="false" space-fill="false">
            <image icon-name="drive-harddisk" icon-size="menu"></image>
            <text><label>/ : $DISK_USED, $DISK_FREE libres</label></text>
          </hbox>
        </vbox>

      </hbox>
    </frame>

    <hseparator></hseparator>

    <!-- ═══════════════════════════════════════════════════════════════════
         NOTEBOOK, 6 onglets
         ═══════════════════════════════════════════════════════════════════ -->
    <notebook tab-pos="top" space-expand="true" space-fill="true"
              tab-labels="Systeme|Materiel|Reseau|Fichiers|Journaux|Paquets">

      <!-- ══════════════════════════════════════════════════════════════════
           Onglet 1 : Systeme
           ══════════════════════════════════════════════════════════════════ -->
      <vbox label="Systeme" space-expand="true" space-fill="true">
        <hbox spacing="4" space-expand="true" space-fill="true">

          <frame label="Actions rapides" space-expand="false" space-fill="false">
            <vbox spacing="2" width-request="200" homogeneous="true">
              <button space-expand="true" space-fill="true" image-position="right" homogeneous="true">
                <input file icon="utilities-terminal"></input>
                <label>Terminal</label>
                <action>bash "$SCRIPT_PATH" --do terminal</action>
              </button>
              <button space-expand="true" space-fill="true" image-position="right" homogeneous="true">
                <input file icon="dialog-password"></input>
                <label>Terminal root</label>
                <action>bash "$SCRIPT_PATH" --do terminal-root</action>
              </button>
              <button space-expand="true" space-fill="true" image-position="right" homogeneous="true">
                <input file icon="system-run"></input>
                <label>Processus (htop)</label>
                <action>bash "$SCRIPT_PATH" --do processes</action>
              </button>
              <button space-expand="true" space-fill="true" image-position="right" homogeneous="true">
                <input file icon="preferences-system"></input>
                <label>Infos systeme</label>
                <action>bash "$SCRIPT_PATH" --do info-sys</action>
              </button>
              <button space-expand="true" space-fill="true" image-position="right" homogeneous="true">
                <input file icon="applications-system"></input>
                <label>Services systemd</label>
                <action>bash "$SCRIPT_PATH" --do services</action>
              </button>
            </vbox>
          </frame>

          <vbox spacing="4" space-expand="true" space-fill="true">
            <frame label="Recherche dans dmesg" space-expand="false" space-fill="false">
              <vbox>
                <text><label>Filtre (vide = 100 dernieres lignes) :</label></text>
                <searchentry>
                  <variable>KERNEL_SEARCH</variable>
                  <default>error</default>
                </searchentry>
                <hbox>
                  <button>
                    <label>Rechercher</label>
                    <action>bash "$SCRIPT_PATH" --do search-dmesg</action>
                  </button>
                  <button>
                    <label>dmesg complet</label>
                    <action>bash "$SCRIPT_PATH" --do dmesg-full</action>
                  </button>
                </hbox>
              </vbox>
            </frame>
            <frame label="Aide sur une commande" space-expand="false" space-fill="false">
              <vbox>
                <text><label>Commande (liste blanche) :</label></text>
                <entry>
                  <variable>CMD_INPUT</variable>
                  <default>ls</default>
                </entry>
                <hbox>
                  <button>
                    <label>--help</label>
                    <action>bash "$SCRIPT_PATH" --do cmd-help</action>
                  </button>
                  <button>
                    <label>man</label>
                    <action>bash "$SCRIPT_PATH" --do cmd-man</action>
                  </button>
                </hbox>
              </vbox>
            </frame>
            <frame label="dmesg (20 dernieres lignes)" space-expand="true" space-fill="true">
              <edit space-expand="true" space-fill="true">
                <input>bash "$SCRIPT_PATH" --print noyau 20</input>
              </edit>
            </frame>
          </vbox>

        </hbox>
      </vbox>

      <!-- ══════════════════════════════════════════════════════════════════
           Onglet 2 : Materiel
           ══════════════════════════════════════════════════════════════════ -->
      <vbox label="Materiel" space-expand="true" space-fill="true">
        <hbox spacing="4" space-expand="true" space-fill="true">
          <frame label="Actions materiel" space-expand="false" space-fill="false">
            <vbox spacing="2" width-request="200" homogeneous="true">
              <button space-expand="true" space-fill="true" image-position="right" homogeneous="true">
                <input file icon="computer"></input>
                <label>Materiel complet</label>
                <action>bash "$SCRIPT_PATH" --do hardware-info</action>
              </button>
              <button space-expand="true" space-fill="true" image-position="right" homogeneous="true">
                <input file icon="preferences-desktop"></input>
                <label>BIOS / UEFI</label>
                <action>bash "$SCRIPT_PATH" --do bios-info</action>
              </button>
              <button space-expand="true" space-fill="true" image-position="right" homogeneous="true">
                <input file icon="application-x-executable"></input>
                <label>Modules noyau</label>
                <action>bash "$SCRIPT_PATH" --do lsmod</action>
              </button>
              <button space-expand="true" space-fill="true" image-position="right" homogeneous="true">
                <input file icon="drive-harddisk"></input>
                <label>Blocs (lsblk)</label>
                <action>bash "$SCRIPT_PATH" --do lsblk</action>
              </button>
            </vbox>
          </frame>
          <frame label="Vue rapide : lsblk" space-expand="true" space-fill="true">
            <edit space-expand="true" space-fill="true">
              <input>lsblk --output NAME,SIZE,TYPE,MOUNTPOINT --color=never 2>/dev/null | cat</input>
            </edit>
          </frame>
        </hbox>
      </vbox>

      <!-- ══════════════════════════════════════════════════════════════════
           Onglet 3 : Reseau
           ══════════════════════════════════════════════════════════════════ -->
      <vbox label="Reseau" space-expand="true" space-fill="true">
        <hbox spacing="4" space-expand="true" space-fill="true">
          <frame label="Actions reseau" space-expand="false" space-fill="false">
            <vbox spacing="2" width-request="200" homogeneous="true">
              <button space-expand="true" space-fill="true" image-position="right" homogeneous="true">
                <input file icon="network-wired"></input>
                <label>Infos completes</label>
                <action>bash "$SCRIPT_PATH" --do network-info</action>
              </button>
              <button space-expand="true" space-fill="true" image-position="right" homogeneous="true">
                <input file icon="network-transmit-receive"></input>
                <label>Connexions (ss)</label>
                <action>bash "$SCRIPT_PATH" --do ss</action>
              </button>
              <button space-expand="true" space-fill="true" image-position="right" homogeneous="true">
                <input file icon="network-idle"></input>
                <label>Test ping</label>
                <action>bash "$SCRIPT_PATH" --do ping-test</action>
              </button>
            </vbox>
          </frame>
          <frame label="Interfaces actives" space-expand="true" space-fill="true">
            <edit space-expand="true" space-fill="true">
              <input>ip -brief -color=never addr show 2>/dev/null | cat</input>
            </edit>
          </frame>
        </hbox>
      </vbox>

      <!-- ══════════════════════════════════════════════════════════════════
           Onglet 4 : Fichiers
           ══════════════════════════════════════════════════════════════════ -->
      <vbox label="Fichiers" space-expand="true" space-fill="true">
        <hbox spacing="6" space-expand="false" space-fill="false">
          <image icon-name="dialog-warning" icon-size="menu"></image>
          <text><label>Attention : l edition de fichiers systeme necessite les droits root (via pkexec).</label></text>
        </hbox>
        <hseparator></hseparator>
        <hbox spacing="4" space-expand="true" space-fill="true">
          <frame label="Actions fichiers" space-expand="false" space-fill="false">
            <vbox spacing="2" width-request="200" homogeneous="true">
              <comboboxtext space-expand="true" space-fill="true">
                <variable>SELECTED_FILE</variable>
                <item>/etc/fstab</item>
                <item>/etc/apt/sources.list</item>
                <item>/etc/network/interfaces</item>
                <item>/etc/systemd/system.conf</item>
                <item>/etc/ssh/sshd_config</item>
                <item>/etc/X11/xorg.conf</item>
                <item>$HOME/.bashrc</item>
                <item>/root/.bashrc</item>
              </comboboxtext>
              <button space-expand="true" space-fill="true" image-position="right" homogeneous="true">
                <input file icon="document-edit"></input>
                <label>Editer (root)</label>
                <action>bash "$SCRIPT_PATH" --do open-file</action>
              </button>
              <button space-expand="true" space-fill="true" image-position="right" homogeneous="true">
                <input file icon="document-open"></input>
                <label>Afficher</label>
                <action>bash "$SCRIPT_PATH" --do view-file</action>
              </button>
            </vbox>
          </frame>
          <frame label="Espace disque (df -h)" space-expand="true" space-fill="true">
            <edit space-expand="true" space-fill="true">
              <input>df -h 2>/dev/null</input>
            </edit>
          </frame>
        </hbox>
      </vbox>

      <!-- ══════════════════════════════════════════════════════════════════
           Onglet 5 : Journaux
           ══════════════════════════════════════════════════════════════════ -->
      <vbox label="Journaux" space-expand="true" space-fill="true">
        <hbox spacing="4" space-expand="true" space-fill="true">
          <frame label="Actions journaux" space-expand="false" space-fill="false">
            <vbox spacing="2" width-request="200" homogeneous="true">
              <button space-expand="true" space-fill="true" image-position="right" homogeneous="true">
                <input file icon="dialog-error"></input>
                <label>Erreurs (prio 3)</label>
                <action>bash "$SCRIPT_PATH" --do journal-errors</action>
              </button>
              <button space-expand="true" space-fill="true" image-position="right" homogeneous="true">
                <input file icon="system-reboot"></input>
                <label>Journal demarrage</label>
                <action>bash "$SCRIPT_PATH" --do journal-boot</action>
              </button>
              <button space-expand="true" space-fill="true" image-position="right" homogeneous="true">
                <input file icon="text-x-generic"></input>
                <label>dmesg complet</label>
                <action>bash "$SCRIPT_PATH" --do dmesg-full</action>
              </button>
              <hseparator></hseparator>
              <searchentry>
                <variable>JOURNAL_SEARCH</variable>
              </searchentry>
              <comboboxtext>
                <variable>JOURNAL_LINES</variable>
                <item>100</item>
                <item>200</item>
                <item>500</item>
                <item>1000</item>
              </comboboxtext>
              <button space-expand="true" space-fill="true" image-position="right" homogeneous="true">
                <input file icon="edit-find"></input>
                <label>Filtrer</label>
                <action>bash "$SCRIPT_PATH" --do journal-search</action>
              </button>
            </vbox>
          </frame>
          <frame label="Dernieres erreurs journald" space-expand="true" space-fill="true">
            <edit space-expand="true" space-fill="true">
              <input>journalctl -p 3 -n 30 --no-pager --no-hostname 2>/dev/null | cat</input>
            </edit>
          </frame>
        </hbox>
      </vbox>

      <!-- ══════════════════════════════════════════════════════════════════
           Onglet 6 : Paquets
           ══════════════════════════════════════════════════════════════════ -->
      <vbox label="Paquets" space-expand="true" space-fill="true">
        <hbox spacing="4" space-expand="true" space-fill="true">
          <frame label="Actions paquets" space-expand="false" space-fill="false">
            <vbox spacing="2" width-request="200" homogeneous="true">
              <button space-expand="true" space-fill="true" image-position="right" homogeneous="true">
                <input file icon="system-software-update"></input>
                <label>Mettre a jour</label>
                <action>bash "$SCRIPT_PATH" --do pkg-update</action>
              </button>
              <button space-expand="true" space-fill="true" image-position="right" homogeneous="true">
                <input file icon="package-x-generic"></input>
                <label>Paquets installes</label>
                <action>bash "$SCRIPT_PATH" --do pkg-list</action>
              </button>
              <button space-expand="true" space-fill="true" image-position="right" homogeneous="true">
                <input file icon="edit-delete"></input>
                <label>Paquets inutiles</label>
                <action>bash "$SCRIPT_PATH" --do pkg-orphans</action>
              </button>
            </vbox>
          </frame>
          <frame label="Dernieres installations (dpkg.log)" space-expand="true" space-fill="true">
            <edit space-expand="true" space-fill="true">
              <input>grep " install " /var/log/dpkg.log 2>/dev/null | tail -40 | cat</input>
            </edit>
          </frame>
        </hbox>
      </vbox>

    </notebook>

    <!-- Pied de page : boutons fixes, texte central expansible -->
    <hbox spacing="6" space-expand="false" space-fill="false">
      <button space-expand="false" space-fill="false">
        <label>A propos</label>
        <action>bash "$SCRIPT_PATH" --do about</action>
      </button>
      <text space-expand="true" space-fill="true">
        <label>Exemple gtk3sermo 1.0.0</label>
      </text>
      <button space-expand="false" space-fill="false">
        <label>Quitter</label>
        <action>exit:0</action>
      </button>
    </hbox>

  </vbox>
</window>
XMLEOF
)
}

# ── Vérification des dépendances ────────────────────────────────────────────

check_deps() {
    local missing=()
    command -v gtk3sermo &>/dev/null || command -v gtkdialog &>/dev/null \
        || missing+=("gtk3sermo")
    command -v pkexec &>/dev/null || missing+=("pkexec")
    [ -n "$TERMINAL" ] || missing+=("un terminal graphique")
    [ -n "$EDITEUR" ]  || missing+=("un editeur graphique")
    if [ ${#missing[@]} -gt 0 ]; then
        echo "AVERTISSEMENT : dépendances manquantes : ${missing[*]}" >&2
    fi
}

# ── Point d'entrée principal ─────────────────────────────────────────────────

# Mode action gtkdialog : le script est rappelé par lui-même avec --do ACTION
# Les variables de widgets sont disponibles via l'environnement (export par gtkdialog)
if [[ "${1:-}" == "--do" && -n "${2:-}" ]]; then
    handle_action "$2"
    exit 0
fi

check_deps
create_dialog

# NE PAS utiliser set -e ici : gtk3sermo renvoie un code non-zéro normal
# à la fermeture de la fenêtre, ce qui déclencherait un arrêt prématuré.
gtk3sermo --program=MAIN_DIALOG --center
