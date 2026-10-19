#!/bin/bash
# SPDX-License-Identifier: CC0-1.0
# ============================================================================
# approbation-hook.sh — hook PreToolUse de Claude Code : chaque action risquée
# de l'agent passe par une fenêtre sermo, que lui ne peut pas cliquer.
#
# Branchement (settings.json des réglages de Claude Code, du projet ou global) :
#
#   {
#     "hooks": {
#       "PreToolUse": [
#         { "matcher": "Bash",
#           "hooks": [ { "type": "command",
#                        "command": "/chemin/vers/sermo/examples/approbation/approbation-hook.sh" } ] }
#       ]
#     }
#   }
#
# Mettre "Bash|Write|Edit" au matcher pour porter aussi les écritures.
#
# Ce que fait le hook, par réponse :
#   Autoriser     → permissionDecision "allow"  (l'utilisateur a cliqué lui-même)
#   Refuser/fermé → permissionDecision "deny"   (l'agent en est informé)
#   pas d'affichage, sermo en erreur, délai → "ask" : l'invite normale de Claude
#                   Code prend le relais. Jamais d'accord par défaut.
#   Commande banale (ls, pwd, git status…, sans aucun caractère de shell)
#                 → aucun avis : les réglages habituels s'appliquent, sans fenêtre.
#
# Le texte de la commande n'est jamais interprété : il va dans un fichier que
# la fenêtre affiche (voir approbation.sh).
# Variables : APPROBATION_DELAI (secondes, défaut 120), APPROBATION_SCRIPT.
# ============================================================================

set -u
ICI=$(CDPATH= cd -- "$(dirname -- "$(readlink -f -- "$0")")" && pwd)
APPROBATION="${APPROBATION_SCRIPT:-$ICI/approbation.sh}"

decision() {   # decision <allow|deny|ask> <raison fixe, sans guillemet>
    printf '{"hookSpecificOutput":{"hookEventName":"PreToolUse","permissionDecision":"%s","permissionDecisionReason":"%s"}}\n' "$1" "$2"
}

tmp=$(mktemp -d "${TMPDIR:-/tmp}/approbation-hook.XXXXXX") || exit 0
trap 'rm -rf "$tmp"' EXIT

# Lit le JSON de Claude Code sur l'entrée, écrit le détail à montrer, et rend :
#   silent (rien à demander) · gate (demander) · ask (illisible : invite normale)
PY='
import json, re, sys
try:
    d = json.load(sys.stdin)
    ti = d.get("tool_input") or {}
    outil = str(d.get("tool_name", "?"))
    cwd = str(d.get("cwd", ""))
except Exception:
    print("ask"); sys.exit(0)
SURES = re.compile(r"(?:ls|pwd|date|whoami|git status|git diff|git log)(?: [A-Za-z0-9_./=:@%+,-]+)*")
cmd = ti.get("command") if isinstance(ti, dict) else None
if outil == "Bash" and isinstance(cmd, str) and SURES.fullmatch(cmd):
    print("silent"); sys.exit(0)
if outil == "Bash" and isinstance(cmd, str):
    corps = cmd
elif isinstance(ti, dict) and isinstance(ti.get("file_path"), str):
    extrait = str(ti.get("content", ti.get("new_string", "")))[:3000]
    corps = ti["file_path"] + "\n\n" + extrait
else:
    corps = json.dumps(ti, ensure_ascii=False, indent=2)
texte = "Outil : %s\nDossier : %s\n\n%s\n" % (outil, cwd, corps)
if len(texte) > 20000:
    texte = texte[:20000] + "\n[… tronqué : %d caractères en tout]\n" % len(texte)
with open(sys.argv[1], "w", encoding="utf-8", errors="replace") as f:
    f.write(texte.encode("utf-8", "replace").decode("utf-8"))
print("gate")
'
verdict=$(python3 -I -c "$PY" "$tmp/detail.txt" 2>/dev/null)

case "$verdict" in
    silent) exit 0 ;;
    gate)   ;;
    *)      decision ask "Hook d approbation: entree illisible, invite normale"; exit 0 ;;
esac

"$APPROBATION" \
    --titre "Approbation requise" \
    --question "Un agent IA demande l'autorisation d'exécuter ceci :" \
    --delai "${APPROBATION_DELAI:-120}" \
    --detail-fichier "$tmp/detail.txt"
case $? in
    0) decision allow "Autorise par l utilisateur dans la fenetre sermo" ;;
    1) decision deny  "Refuse par l utilisateur dans la fenetre sermo. Ne pas relancer la meme action sans lui en parler." ;;
    *) decision ask   "Fenetre d approbation indisponible ou sans reponse: invite normale" ;;
esac
exit 0
