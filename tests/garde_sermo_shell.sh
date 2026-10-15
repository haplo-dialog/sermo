#!/bin/bash
# SPDX-License-Identifier: GPL-2.0-or-later
# tests/garde_sermo_shell.sh — haplo-dialog — 2026 — GPL-2.0-or-later
#
# SERMO_SHELL choisit le shell du repli « -c » (commande qui contient de la
# syntaxe shell). /bin/sh était écrit en dur dans safe_exec.c ; un système qui
# range son shell ailleurs (Termux, Windows) ne pouvait rien lancer. Ce test
# vérifie, sur les deux chemins du repli (safe_system par --do, safe_popen par
# un <input>), que le shell désigné est bien celui qui tourne, qu'une valeur
# douteuse est refusée et dite, et que rien ne change sans la variable.
BIN="${1:?usage: garde_sermo_shell.sh /chemin/vers/binaire}"
[[ -x "$BIN" ]] || { echo "binaire introuvable : $BIN" >&2; exit 2; }
BIN=$(realpath -- "$BIN")   # l'étape 2 change de dossier
for t in xvfb-run timeout; do command -v "$t" >/dev/null || { echo "outil manquant : $t" >&2; exit 2; }; done

# Le faux shell est un programme qu'on EXÉCUTE : jamais sous /tmp (une
# exécution depuis un dossier temporaire est une alerte de sécurité réelle sur
# un poste surveillé). Bac dans le cache de l'utilisateur.
RACINE="${XDG_CACHE_HOME:-$HOME/.cache}/sermo-bancs"
mkdir -p "$RACINE" || { echo "impossible de créer $RACINE" >&2; exit 2; }
BAC=$(mktemp -d "$RACINE/sermo-shell.XXXXXX") || exit 2
trap 'rm -rf "$BAC"' EXIT

TRACE="$BAC/trace"
cat >"$BAC/faux-sh" <<EOF
#!/bin/sh
# Témoin : note qu'il a été appelé, puis laisse le vrai shell travailler.
printf 'FAUX-SHELL %s\n' "\$*" >>"$TRACE"
exec /bin/sh "\$@"
EOF
chmod 700 "$BAC/faux-sh"

DIALOG='<window title="essai-shell"><vbox><entry><default>Ada</default><variable>NOM</variable></entry><entry><input>echo aaa | tr a b</input><variable>TUBE</variable></entry><timer interval="1"><variable>T</variable><action>exit:OK</action></timer><button ok></button></vbox></window>'
export DIALOG

lancer() {   # lancer [VAR=valeur…] : sortie standard ; stderr dans $BAC/err
    env "$@" timeout 25 xvfb-run -a "$BIN" --program=DIALOG --do='echo "SALUT $NOM"' 2>"$BAC/err"
}

# 1 — SERMO_SHELL désigne le témoin : les deux chemins du repli passent par lui
: >"$TRACE"
out=$(lancer SERMO_SHELL="$BAC/faux-sh")
grep -q '^SALUT Ada$' <<<"$out" || {
    echo "ECHEC : avec SERMO_SHELL, --do n'a pas donné « SALUT Ada ». Sortie :"
    sed 's/^/    /' <<<"$out"; exit 1; }
grep -q '^TUBE="bbb"$' <<<"$out" || {
    echo "ECHEC : avec SERMO_SHELL, l'<input> à tube n'a pas donné TUBE=\"bbb\". Sortie :"
    sed 's/^/    /' <<<"$out"; exit 1; }
grep -q 'FAUX-SHELL -c echo "SALUT' "$TRACE" || {
    echo "ECHEC : --do n'est pas passé par SERMO_SHELL (safe_system). Trace :"
    sed 's/^/    /' "$TRACE"; exit 1; }
grep -q 'FAUX-SHELL -c echo aaa | tr a b' "$TRACE" || {
    echo "ECHEC : l'<input> à tube n'est pas passé par SERMO_SHELL (safe_popen). Trace :"
    sed 's/^/    /' "$TRACE"; exit 1; }

# 2 — un chemin relatif est refusé, dit, et le shell par défaut prend le relais
: >"$TRACE"
out=$(cd "$BAC" && lancer SERMO_SHELL=faux-sh)
grep -q 'SERMO_SHELL ignorée' "$BAC/err" || {
    echo "ECHEC : SERMO_SHELL relative acceptée sans un mot. Erreurs :"
    sed 's/^/    /' "$BAC/err"; exit 1; }
[[ ! -s "$TRACE" ]] || {
    echo "ECHEC : SERMO_SHELL relative a quand même été lancée. Trace :"
    sed 's/^/    /' "$TRACE"; exit 1; }
grep -q '^SALUT Ada$' <<<"$out" || {
    echo "ECHEC : après le refus de SERMO_SHELL, le shell par défaut n'a pas pris le relais."; exit 1; }

# 3 — sans la variable : le témoin n'est jamais appelé, le résultat ne change pas
: >"$TRACE"
out=$(env -u SERMO_SHELL timeout 25 xvfb-run -a "$BIN" --program=DIALOG --do='echo "SALUT $NOM"' 2>/dev/null)
[[ ! -s "$TRACE" ]] || { echo "ECHEC : témoin appelé sans SERMO_SHELL."; exit 1; }
grep -q '^SALUT Ada$' <<<"$out" && grep -q '^TUBE="bbb"$' <<<"$out" || {
    echo "ECHEC : sans SERMO_SHELL, le résultat a changé. Sortie :"
    sed 's/^/    /' <<<"$out"; exit 1; }

echo "OK : SERMO_SHELL suivie par --do et par <input>, refusée si relative, sans effet absente"
