#!/bin/bash
# SPDX-License-Identifier: GPL-2.0-or-later
# tests/garde_allowed_cmds.sh — haplo-dialog — 2026 — GPL-2.0-or-later
#
# SERMO_ALLOWED_CMDS borne les commandes que le programme accepte de lancer.
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
grep -q 'TEMOIN-DEFAUT' <<<"$(sortie SERMO_X=1)" || { echo "ECHEC : sans liste, la commande devrait passer"; exit 1; }

# 1 — liste posee, commande dedans (nom seul) : elle passe
CMD='echo TEMOIN-AUTORISE'
grep -q 'TEMOIN-AUTORISE' <<<"$(sortie SERMO_ALLOWED_CMDS=echo,ls)" || { echo "ECHEC : commande listee refusee a tort"; exit 1; }

# 2 — liste posee, commande absente : refusee, et dite
CMD='/bin/echo TEMOIN-INTERDIT'
grep -q 'TEMOIN-INTERDIT' <<<"$(sortie SERMO_ALLOWED_CMDS=ls,cat)" && { echo "ECHEC : commande hors liste executee"; exit 1; }
grep -qi 'SERMO_ALLOWED_CMDS' "$ERRF" || { echo "ECHEC : refus sans message explicite"; exit 1; }

# 3 — contournement par shell : « sh -c '...' » passerait la liste en
#     s'appelant sh. Le repli doit tomber des que la liste est posee.
CMD='sh -c "/bin/echo TEMOIN-CONTOURNE"'
grep -q 'TEMOIN-CONTOURNE' <<<"$(sortie SERMO_ALLOWED_CMDS=echo,sh)" && { echo "ECHEC : le repli shell contourne la liste"; exit 1; }
grep -qi 'repli shell refuse' "$ERRF" || { echo "ECHEC : le repli shell n'a pas ete refuse explicitement"; exit 1; }

# 4 — le chemin absolu d'un AUTRE programme ne passe pas
CMD='/bin/echo TEMOIN-CHEMIN'
grep -q 'TEMOIN-CHEMIN' <<<"$(sortie SERMO_ALLOWED_CMDS=ls)" && { echo "ECHEC : /bin/echo accepte alors que seul ls est liste"; exit 1; }

# 5 — un faux programme qui porte un nom liste, pose hors du PATH, ne passe pas.
#     C'est le contournement qu'autorisait l'ancienne comparaison par NOM DE BASE :
#     « /tmp/x/echo » passait des que « echo » etait liste.
FAUX=$(mktemp -d); trap 'rm -f "$ERRF"; rm -rf "$FAUX"' EXIT
printf '#!/bin/sh\necho TEMOIN-FAUX\n' > "$FAUX/echo"; chmod +x "$FAUX/echo"
CMD="$FAUX/echo"
grep -q 'TEMOIN-FAUX' <<<"$(sortie SERMO_ALLOWED_CMDS=echo)" && { echo "ECHEC : un faux echo hors du PATH passe la liste (comparaison par nom de base)"; exit 1; }
#     temoin : sans liste, ce faux programme s'execute bien (le cas 5 mesure donc quelque chose)
grep -q 'TEMOIN-FAUX' <<<"$(sortie SERMO_X=1)" || { echo "ECHEC DU TEMOIN : le faux programme ne s'execute meme pas sans liste"; exit 1; }

# 6 — une entree chemin autorise exactement ce chemin
CMD='/bin/echo TEMOIN-EXACT'
grep -q 'TEMOIN-EXACT' <<<"$(sortie SERMO_ALLOWED_CMDS=/bin/echo)" || { echo "ECHEC : /bin/echo refuse alors qu'il est liste tel quel"; exit 1; }

# 7 — le chemin auquel le PATH resout un nom liste passe, et le nom seul passe
#     quand l'entree est le chemin resolu
RESOLU=$(type -P echo)
CMD="$RESOLU TEMOIN-RESOLU"
grep -q 'TEMOIN-RESOLU' <<<"$(sortie SERMO_ALLOWED_CMDS=echo)" || { echo "ECHEC : $RESOLU (echo resolu par le PATH) refuse alors que echo est liste"; exit 1; }
CMD='echo TEMOIN-NOM'
grep -q 'TEMOIN-NOM' <<<"$(sortie SERMO_ALLOWED_CMDS="$RESOLU")" || { echo "ECHEC : echo refuse alors que $RESOLU est liste"; exit 1; }

# 8 — les noms de la 1.x mordent encore, et le disent
CMD='/bin/echo TEMOIN-ANCIEN'
grep -q 'TEMOIN-ANCIEN' <<<"$(sortie HAPLO_ALLOWED_CMDS=ls)" && { echo "ECHEC : HAPLO_ALLOWED_CMDS (nom 1.x) est ignore"; exit 1; }
grep -q 'HAPLO_ALLOWED_CMDS est l.ancien nom' "$ERRF" || { echo "ECHEC : l'ancien nom HAPLO_ALLOWED_CMDS n'est pas signale"; exit 1; }
CMD='echo TEMOIN-REPLI | cat'
grep -q 'TEMOIN-REPLI' <<<"$(sortie SERMO_X=1)" || { echo "ECHEC DU TEMOIN : sans variable, le repli shell devrait passer"; exit 1; }
grep -q 'TEMOIN-REPLI' <<<"$(sortie HAPLO_NO_SHELL_FALLBACK=1)" && { echo "ECHEC : HAPLO_NO_SHELL_FALLBACK (nom 1.x) est ignore"; exit 1; }
grep -q 'HAPLO_NO_SHELL_FALLBACK est l.ancien nom' "$ERRF" || { echo "ECHEC : l'ancien nom HAPLO_NO_SHELL_FALLBACK n'est pas signale"; exit 1; }

echo "OK : eteinte par defaut, mordante une fois posee ; ni shell, ni chemin d'un autre programme, ni faux homonyme ne la contournent ; noms 1.x encore lus"
