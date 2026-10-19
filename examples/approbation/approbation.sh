#!/bin/bash
# SPDX-License-Identifier: CC0-1.0
# ============================================================================
# approbation.sh — une porte d'approbation humaine, en une fenêtre sermo
#
# Un programme (un agent IA, un script, un hook) veut faire quelque chose de
# risqué. Il appelle ce script : une vraie fenêtre demande « Autoriser ? », et
# le code de retour dit ce que l'humain a répondu. La fenêtre vit dans un
# autre processus que le demandeur : il ne peut pas la cliquer à la place de
# l'utilisateur.
#
# Usage :
#   approbation.sh [--titre T] [--question Q] [--delai S]
#                  (--detail TEXTE | --detail-fichier FICHIER)
#
# Code de retour :
#   0  autorisé       l'utilisateur a cliqué sur « Autoriser » (et rien d'autre)
#   1  refusé         « Refuser », ou fenêtre fermée
#   2  indisponible   pas d'affichage, pas de port graphique, sermo en erreur
#   3  délai écoulé   personne n'a répondu dans --delai secondes (défaut 120)
#   64 usage
# Seul 0 vaut accord. Tout le reste est un non : à l'appelant de décider de
# ce qu'il fait des cas 2 et 3 (le hook fourni les renvoie à l'invite normale).
#
# SÉCURITÉ, ce qui est réellement fait :
#   - le texte venu du demandeur (--detail, --detail-fichier) n'est JAMAIS mis
#     dans le XML : il est copié dans un fichier temporaire (mode 600) que la
#     fenêtre affiche par <input file>. Aucun échappement à rater, aucune
#     injection de balise possible ;
#   - --titre et --question vont dans le XML, échappés ; ils doivent pourtant
#     être écrits par l'appelant, pas par l'agent qu'on surveille ;
#   - la réponse est lue ligne à ligne (EXIT="…"), jamais passée à eval ;
#   - fail-closed : sans réponse claire, le code n'est pas 0.
# Variable : SERMO_APPROBATION_BIN force le binaire (essais).
# ============================================================================

titre="Approbation requise"
question="Un programme demande votre autorisation pour :"
delai=120
detail=""
detail_fichier=""

usage() { sed -n '/^# Usage/,/^# Code de retour/p' "$0" | sed '$d;s/^# \{0,1\}//' >&2; exit 64; }

while [ $# -gt 0 ]; do
    case "$1" in
        --titre)          [ $# -ge 2 ] || usage; titre="$2"; shift 2 ;;
        --question)       [ $# -ge 2 ] || usage; question="$2"; shift 2 ;;
        --delai)          [ $# -ge 2 ] || usage; delai="$2"; shift 2 ;;
        --detail)         [ $# -ge 2 ] || usage; detail="$2"; shift 2 ;;
        --detail-fichier) [ $# -ge 2 ] || usage; detail_fichier="$2"; shift 2 ;;
        -h|--help)        usage ;;
        *)                usage ;;
    esac
done
case "$delai" in ''|*[!0-9]*) echo "approbation: --delai doit être un nombre de secondes" >&2; exit 64 ;; esac
[ "$delai" -ge 1 ] || { echo "approbation: --delai doit être >= 1" >&2; exit 64; }
if [ -z "$detail" ] && [ -z "$detail_fichier" ]; then usage; fi
if [ -n "$detail_fichier" ] && [ ! -r "$detail_fichier" ]; then
    echo "approbation: fichier illisible : $detail_fichier" >&2; exit 64
fi

# ── Quel binaire ? Un port à fenêtre : ncurses n'a rien à faire dans un hook ──
choisir_binaire() {
    if [ -n "${SERMO_APPROBATION_BIN:-}" ]; then
        BIN=$(command -v -- "$SERMO_APPROBATION_BIN") || return 1
        return 0
    fi
    [ -n "${DISPLAY:-}${WAYLAND_DISPLAY:-}" ] || return 1
    local d
    for d in gtk3sermo gtk4sermo qt6sermo fltk1sermo efl1sermo sdl3sermo sermo; do
        BIN=$(command -v -- "$d") && return 0
    done
    return 1
}
if ! choisir_binaire; then
    echo "approbation: pas d'affichage ou pas de port sermo à fenêtre" >&2
    exit 2
fi

xml_echapper() {
    local s="$1"
    # Remplacements entre guillemets : bash >= 5.2 lit un « & » nu comme « le texte trouvé ».
    s=${s//&/"&amp;"}; s=${s//</"&lt;"}; s=${s//>/"&gt;"}; s=${s//\"/"&quot;"}
    printf '%s' "$s"
}

# ── Le texte du demandeur va dans un fichier à nous, jamais dans le XML ──────
umask 077
tmp=$(mktemp -d "${TMPDIR:-/tmp}/approbation.XXXXXX") || exit 2
trap 'rm -rf "$tmp"' EXIT
fichier="$tmp/detail.txt"
if [ -n "$detail_fichier" ]; then
    head -c 65536 -- "$detail_fichier" > "$fichier"
else
    printf '%s\n' "$detail" | head -c 65536 > "$fichier"
fi

T=$(xml_echapper "$titre"); Q=$(xml_echapper "$question")
export APPROBATION_DIALOG
APPROBATION_DIALOG=$(cat <<XMLEOF
<window title="$T" window-position="1" resizable="true" default-width="700" default-height="460">
  <vbox spacing="6">
    <text><label>$Q</label></text>
    <frame label="Détail">
      <edit editable="false" space-expand="true" space-fill="true">
        <input file>$fichier</input>
      </edit>
    </frame>
    <text><label>Rien n'est exécuté tant que vous n'avez pas répondu. Fermer la fenêtre revient à refuser.</label></text>
    <hbox spacing="6">
      <button space-expand="true" space-fill="true">
        <label>Refuser</label>
        <action>exit:refuser</action>
      </button>
      <button space-expand="true" space-fill="true">
        <label>Autoriser</label>
        <action>exit:autoriser</action>
      </button>
    </hbox>
  </vbox>
</window>
XMLEOF
)

# NE PAS faire « set -e » : sermo renvoie un code non nul à la fermeture normale.
sortie="$tmp/sortie.txt"
timeout --kill-after=3 "$delai" "$BIN" --program=APPROBATION_DIALOG --center > "$sortie" 2>/dev/null
rc=$?
if [ "$rc" -eq 124 ] || [ "$rc" -eq 137 ]; then
    echo "approbation: délai écoulé sans réponse" >&2
    exit 3
fi

reponse=$(sed -n 's/^EXIT="\(.*\)"$/\1/p' "$sortie" | tail -n 1)
case "$reponse" in
    autoriser) exit 0 ;;
    "")        echo "approbation: sermo n'a rendu aucune réponse (rc=$rc)" >&2; exit 2 ;;
    *)         exit 1 ;;     # « refuser », « abort » (fenêtre fermée), tout le reste
esac
