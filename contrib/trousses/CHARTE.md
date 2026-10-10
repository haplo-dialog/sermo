# CHARTE des outils sermo (`contrib/trousses/`)

Tout outil des trousses suit cette charte. Elle n'invente rien : elle
codifie ce que `examples/system-tools/system-tools.sh` fait déjà bien, et
ajoute les deux règles propres aux **vrais outils multi-backend** :
fonctionner aussi en **ncurses**, et exister pour une **raison que seul
sermo donne**.

---

## 0. Le nord — pourquoi cet outil a le droit d'exister

Un outil n'entre dans une trousse que s'il est **un front-end portable
par-dessus une CLI de confiance** (systemctl, journalctl, rsync, ip,
cryptsetup, apt…). On **n'implémente jamais** la logique métier : on
l'enrobe. La seule valeur unique, c'est *le même front-end en fenêtre et
dans une session SSH*. Un outil qui réinvente rsync n'a pas sa place ; un
front-end `systemctl` qui tombe en ncurses à distance, oui.

Si tu ne peux pas écrire en une phrase « ce que cet outil donne et que
yad/zenity ne donnent pas », il ne va pas dans la trousse.

---

## 1. En-tête obligatoire (fiche d'identité + persona)

Les 20 premières lignes de chaque script, en commentaire :

```sh
#!/bin/bash
# SPDX-License-Identifier: CC0-1.0
# ============================================================================
# <NOM DE L'OUTIL> — <une ligne : ce qu'il fait>
#
# PUBLIC        : <admin/devops | distro-rescue | embarqué | …>
# POURQUOI LUI  : <la douleur résolue, en une phrase>
# CE QUE SEUL
#   SERMO DONNE : <le même front-end GTK ET ncurses, précisément ici>
# ENROBE        : <la/les CLI de confiance appelées>
# BACKENDS      : brille en <…> ; dégradé assumé en <…>
# DÉGRADATION
#   NCURSES     : <ce qu'on perd en terminal, et pourquoi c'est acceptable>
# CIBLE         : <distro/env requis ; sinon refus propre au lancement>
# DÉPENDANCES   : sermo (alias gtkdialog accepté), <…>
# ============================================================================
```

La ligne **DÉGRADATION NCURSES** n'est pas décorative : si tu ne sais pas
la remplir, c'est que le fallback n'a pas été pensé. Reviens à la
conception avant d'écrire une ligne de plus.

---

## 2. Refus honnête au lancement

Si un prérequis manque, **refuser proprement en nommant ce qui a été
détecté** — jamais afficher des onglets vides ou faire semblant.

```sh
if [ ! -e /etc/debian_version ]; then
    echo "Outil prévu pour Debian et dérivées (apt, dpkg)." >&2
    echo "Système détecté : $( (. /etc/os-release 2>/dev/null && echo "$PRETTY_NAME") || uname -s )" >&2
    exit 1
fi
```

---

## 3. Le motif `--do` / `--print` (structure imposée)

Une `<action>` lance un **sous-shell** : elle ne peut pas appeler une
fonction du script parent. Le script **se rappelle lui-même**, et les
valeurs des widgets arrivent **par l'environnement** — pas de fichier
intermédiaire, pas de `export -f`.

```sh
SCRIPT_PATH=$(realpath -- "$0")

# --print <nom> [args] : producteurs PURS sur stdout, pour alimenter un <input>
if [[ "${1:-}" == "--print" ]]; then
    case "${2:-}" in
        <nom>) <fonction_qui_affiche> "${3:-}" ;;
        *)     echo "Sortie inconnue : ${2:-}" ;;
    esac
    exit 0
fi

# --do <action> : effets de bord ; les valeurs des widgets sont dans $VAR
if [[ "${1:-}" == "--do" ]]; then
    action="${2:-}"
    case "$action" in
        <action1>) … ;;   # lit $CHAMP1, $SELECTION, etc.
        *)         echo "Action inconnue : $action" >&2; exit 2 ;;
    esac
    exit 0
fi
```

Et côté XML :

```xml
<action>bash "$SCRIPT_PATH" --do <action1></action>
<input>bash "$SCRIPT_PATH" --print <nom> 20</input>
```

---

## 4. Sécurité — non négociable

### 4.1 Liste blanche, comparaison à l'identique

Toute commande ou chemin issu d'un widget passe une garde **à liste
fermée**, comparée caractère pour caractère, **jamais par motif** :

```sh
readonly ALLOWED_CMDS=( systemctl journalctl systemd-analyze … )

cmd_allowed() {
    local cmd="$1" c
    [[ "$cmd" =~ ^[a-zA-Z][a-zA-Z0-9_-]*$ ]] || { echo "ERREUR: commande invalide"; return 1; }
    for c in "${ALLOWED_CMDS[@]}"; do [[ "$cmd" == "$c" ]] && return 0; done
    echo "ERREUR: commande non autorisée : '$cmd'"; return 1
}
```

Pour les chemins/arguments libres : rejeter les métacaractères
(`; | & \` $ < > ( ) { }`) **avant** tout usage.

### 4.2 Garde écrite deux fois

Même quand le widget est une **liste fermée non éditable**, la garde est
**revérifiée en shell**, à l'identique. Le jour où quelqu'un remplace le
widget par un champ libre, la garde tient encore. C'est l'intérêt du
doublon : on l'écrit exprès.

### 4.3 Pas de piège

- `set -e` et `trap ERR` **interdits** : sermo renvoie un code non nul à la
  fermeture normale de la fenêtre.
- Aucun fichier temporaire à nom prévisible : `mktemp` systématique.

---

## 5. Élévation — LA règle propre au fallback

`system-tools.sh` dit « jamais de sudo dans une GUI, toujours pkexec ».
Vrai **en fenêtre**. Mais **pkexec échoue dans un terminal SSH** : pas
d'agent PolicyKit, pas de `$DISPLAY`. Un vrai outil fallback doit donc
**choisir sa méthode d'élévation selon le contexte de rendu** :

```sh
run_as_root() {
    if [ "$(id -u)" -eq 0 ]; then
        "$@"
    elif [ -n "${DISPLAY:-}${WAYLAND_DISPLAY:-}" ] && command -v pkexec >/dev/null; then
        # Contexte graphique : PolicyKit, jamais sudo-en-GUI
        pkexec env DISPLAY="$DISPLAY" XAUTHORITY="$XAUTHORITY" "$@"
    elif command -v doas >/dev/null; then
        doas "$@"          # terminal : l'invite se fait dans le TTY, c'est sûr
    else
        sudo "$@"          # idem : le tabou « sudo-GUI » ne vaut QUE pour la GUI
    fi
}
```

Le tabou « pas de sudo en graphique » ne s'applique pas à un vrai
terminal : là, l'invite de mot de passe se fait honnêtement dans le TTY.
**Documente ce choix dans l'en-tête** (ligne DÉGRADATION NCURSES).

---

## 6. Détection, jamais codage en dur

Terminal et éditeur : détectés parmi ceux installés
(`x-terminal-emulator`, `xterm`, `foot`… ; `sensible-editor`, `nano`…),
avec repli terminal (`nano`/`vi`) pour le contexte ncurses. Logo et nom du
système : lus sur la machine (`/etc/os-release`, thème d'icônes), **jamais**
un nom ou une image de projet copiée.

---

## 6bis. Pas d'images dans le tronc commun

`<image>`, `<pixmap>`, `icon-name` : **interdits** dans la partie de
l'interface partagée par tous les backends. ncurses ne les rend pas — mais
le problème n'est pas l'absence, c'est que **la boîte de l'image décale la
mise en page** : le même XML ne s'aligne plus pareil en fenêtre et en
terminal, et la parité casse là où on la croyait acquise.

Règle :

- Le **tronc commun** (ce que voient tous les backends) est en **texte, labels,
  cadres** — rien de pictural.
- Un logo ou une icône décorative, si on y tient, est **optionnel et
  injecté uniquement quand il y a un affichage**, et placé pour que son
  absence **ne bouge aucun autre widget** (une colonne/entête à part, jamais
  au milieu du flux) :

  ```sh
  LOGO_XML=""
  if [ -n "${DISPLAY:-}${WAYLAND_DISPLAY:-}" ]; then
      LOGO_XML='<image icon-name="…" icon-size="dialog"></image>'
  fi
  # … puis dans le XML, à un emplacement neutre :  $LOGO_XML
  ```

- Les statuts (actif/arrêté, OK/erreur) se disent en **mots ou symboles
  texte** (`●`/`○`, `[OK]`/`[!]`), jamais par une icône colorée qu'un
  terminal ne peut pas montrer.

Le test de parité (§7) attrape ce genre d'écart : si une image traîne dans
le flux, le rendu ncurses diverge et le cas passe au rouge.

## 7. Preuve de fallback — test de parité obligatoire

Chaque outil livre **au moins un cas de parité** dans
`tests/comportement/cas/`, sur le modèle du kit REFRESH : un dialogue
déterministe (pas de `date`, pas de hasard), un timer invisible qui
déclenche l'action clé, un `.attendu` = sortie de `gtk3sermo`. Le cas doit
**passer au vert sur `ncursessermo`**. Tant qu'il est rouge, l'outil n'est
pas « multi-backend », il est « GTK avec un README optimiste ».

---

## 8bis. Contrat de module (pour être branché dans un hub)

Un outil devient un **module** branchable dans un hub (forme A : lanceur)
en exposant, en plus de tout ce qui précède, un point d'entrée :

```sh
# --manifest : fiche lisible par le hub, format "CLE: valeur", une par ligne.
if [[ "${1:-}" == "--manifest" ]]; then
    cat <<'EOF'
NOM: servys
DESCRIPTION: Gestion des services systemd (start/stop/restart)
PUBLIC: admin-devops
BACKENDS: tous
EOF
    exit 0
fi
```

Champs de manifest (les premiers obligatoires, les suivants pour un hub groupé) :

- `NOM`, `DESCRIPTION`, `BACKENDS` — de base.
- `GROUPE` — `système` | `admin` | `user` | `autres` : la section du hub.
  L'axe est le **privilège/public**, pas le thème : `user` = sans root,
  `admin` = élévation, `système` = profond/secours.
- `BESOIN_ROOT` — `oui`/`non` : le hub en tire une pastille `(root)` et un
  avertissement de privilège non bloquant.
- `ENV` — `desktop` | `tty` | `les_deux` : où le module a un sens.

Et un second point d'entrée, la **santé** :

```sh
# --check : code 0 si l'outil est utilisable sur CETTE machine, sinon 1 + raison.
if [[ "${1:-}" == "--check" ]]; then
    command -v <cli_requise> >/dev/null 2>&1 && exit 0
    echo "<ce qui manque>"; exit 1
fi
```

Le hub affiche `●  prêt` ou `✕  <raison>` **avant** le lancement : sur un
parc hétérogène, on voit d'un coup ce qui marche sur la machine courante.

Règles :

- `NOM` est **unique** dans un dossier `modules/` et ne contient ni `<`, `>`,
  `&`, ni retour-ligne : le hub le réinjecte dans son propre XML, donc le
  module doit le garder propre (le hub ré-échappe quand même — défense en
  profondeur).
- Le module reste **autonome** : lancé par le hub comme **son propre
  processus** (`bash module.sh [port]`), il garde ses propres gardes. Le hub
  ne lui fait jamais confiance pour la sécurité.
- **Forme B (onglets) seulement** — le module expose aussi `--xml` (le
  fragment de widgets de son onglet) avec **toutes ses variables préfixées**
  par son nom (`SERVYS_SVC`, jamais `SVC`) : un seul processus, un seul
  espace de noms, les collisions sont à la charge du module.

Le hub, lui, ne fait jamais confiance au texte d'un manifest : il ne lance
qu'un **chemin découvert sur le disque** (jamais un chemin dérivé du texte),
sur un **ensemble fermé** re-scanné au moment du lancement.

## 9. Un TTY, un seul TUI à la fois

Rappel de réalité pour les hubs en terminal : deux interfaces ncurses ne
peuvent pas partager le même TTY. Un hub forme A doit donc **détecter le
contexte** au lancement d'un module :

- **graphique** (`$DISPLAY`/`$WAYLAND_DISPLAY`) → module en tâche de fond
  (`&`), le hub persiste, plusieurs fenêtres coexistent ;
- **terminal** → le hub **cède le TTY** (`exec`) au module ; au retour, on
  est revenu au shell (ou le hub se relance, au choix).

## 8. Squelette de départ

```sh
#!/bin/bash
# SPDX-License-Identifier: CC0-1.0
# <en-tête §1>
<refus honnête §2>
SCRIPT_PATH=$(realpath -- "$0")
<gardes §4.1>
<run_as_root §5>
<--print §3>
<--do §3>
# ── Construction du dialogue ────────────────────────────────────────────
export MAIN_DIALOG
MAIN_DIALOG=$(cat <<'XMLEOF'
<window>…</window>
XMLEOF
)
exec sermo --program=MAIN_DIALOG   # ou gtkdialog, alias accepté
```

---

## Checklist avant d'ajouter un outil à une trousse

- [ ] En-tête §1 complet, **DÉGRADATION NCURSES** remplie
- [ ] Enrobe une CLI de confiance, ne la réimplémente pas (§0)
- [ ] Refus propre si prérequis absent (§2)
- [ ] Tout `--do`/`--print`, valeurs par l'environnement (§3)
- [ ] Gardes liste blanche + doublon shell (§4)
- [ ] `run_as_root` adaptatif GUI/terminal (§5)
- [ ] Terminal/éditeur détectés, repli TTY (§6)
- [ ] **Aucune image dans le tronc commun** ; statuts en texte (§6bis)
- [ ] Un cas de parité vert sur ncurses (§7)
