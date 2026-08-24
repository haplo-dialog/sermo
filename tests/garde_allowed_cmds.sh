#!/bin/bash
# tests/garde_allowed_cmds.sh — haplo-dialog — 2026 — GPL-2.0-or-later
#
# HAPLO_ALLOWED_CMDS borne les commandes que le programme accepte de lancer.
# Elle est ETEINTE par defaut : ce test verifie les deux moities de la promesse
# — que rien ne change quand elle est absente, et qu'elle mord vraiment, sans
# se laisser contourner, quand elle est posee.
#
# ⚠ On ne cherche le temoin QUE dans stdout. Les avertissements de safe_exec
# recopient la commande entiere sur stderr : chercher dans les deux flux, c'est
# se valider sur son propre bruit — une premiere version de ce test le faisait
# et annoncait un contournement qui n'existait pas.
BIN="${1:?usage: garde_allowed_cmds.sh /chemin/vers/binaire}"
[[ -x "$BIN" ]] || { echo "binaire introuvable : $BIN" >&2; exit 2; }
for t in xvfb-run timeout; do command -v "$t" >/dev/null || { echo "outil manquant : $t" >&2; exit 2; }; done

DIALOG='<window title="acl"><vbox><timer interval="1"><variable>T</variable><action>exit:OK</action></timer><button ok></button></vbox></window>'
export DIALOG
ERRF=$(mktemp); trap 'rm -f "$ERRF"' EXIT
# sortie() : rend UNIQUEMENT stdout ; stderr part dans $ERRF.
sortie() { env "$@" timeout 25 xvfb-run -a "$BIN" --program=DIALOG --do="$CMD" 2>"$ERRF"; }

# 0 — temoin : le banc sait-il seulement voir une commande qui s'execute ?
CMD='/bin/echo TEMOIN-DEFAUT'
grep -q 'TEMOIN-DEFAUT' <<<"$(sortie HAPLO_X=1)" || { echo "ECHEC : sans liste, la commande devrait passer"; exit 1; }

# 1 — liste posee, commande dedans : elle passe
CMD='/bin/echo TEMOIN-AUTORISE'
grep -q 'TEMOIN-AUTORISE' <<<"$(sortie HAPLO_ALLOWED_CMDS=echo,ls)" || { echo "ECHEC : commande listee refusee a tort"; exit 1; }

# 2 — liste posee, commande absente : refusee, et dite
CMD='/bin/echo TEMOIN-INTERDIT'
grep -q 'TEMOIN-INTERDIT' <<<"$(sortie HAPLO_ALLOWED_CMDS=ls,cat)" && { echo "ECHEC : commande hors liste executee"; exit 1; }
grep -qi 'HAPLO_ALLOWED_CMDS' "$ERRF" || { echo "ECHEC : refus sans message explicite"; exit 1; }

# 3 — contournement par shell : « sh -c '...' » passerait la liste en
#     s'appelant sh. Le repli doit tomber des que la liste est posee.
CMD='sh -c "/bin/echo TEMOIN-CONTOURNE"'
grep -q 'TEMOIN-CONTOURNE' <<<"$(sortie HAPLO_ALLOWED_CMDS=echo,sh)" && { echo "ECHEC : le repli shell contourne la liste"; exit 1; }
grep -qi 'repli shell refuse' "$ERRF" || { echo "ECHEC : le repli shell n'a pas ete refuse explicitement"; exit 1; }

# 4 — le chemin absolu ne contourne pas le nom de base
CMD='/bin/echo TEMOIN-CHEMIN'
grep -q 'TEMOIN-CHEMIN' <<<"$(sortie HAPLO_ALLOWED_CMDS=ls)" && { echo "ECHEC : /bin/echo accepte alors que seul ls est liste"; exit 1; }

echo "OK : eteinte par defaut, mordante une fois posee, ni shell ni chemin absolu ne la contournent"
