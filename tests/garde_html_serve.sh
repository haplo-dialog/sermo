#!/bin/bash
# SPDX-License-Identifier: GPL-2.0-or-later
# garde_html_serve.sh — htmlsermo en mode serve ne rend que les variables du
# dialogue, et un vrai formulaire renvoie chaque champ.
#
# POURQUOI CE BANC EXISTE (trouvé le 2026-10-18 en écrivant le tutoriel du site)
#
#   1. Le micro-serveur imprimait, et posait par setenv(), CHAQUE paire reçue.
#      Celui qui remplit la page peut ajouter un champ avec les outils du
#      navigateur : PATH=…, LD_PRELOAD=…, IFS=… ressortaient en NOM="valeur" et
#      un script qui fait `eval "$(htmlsermo …)"` les reprenait. C'est
#      précisément l'utilisateur dont sermo protège le script (SECURITY.md).
#   2. Sept widgets (comboboxtext, password, searchentry, switch, togglebutton,
#      list, et la forme <variable> enfant de list) sortaient sans attribut
#      name : un navigateur n'envoie pas un champ sans nom, la valeur choisie
#      était perdue. Et la sortie ne contenait QUE ce qui était reçu, là où
#      gtk3sermo rend toutes les variables du dialogue.
#
# Le banc sert un dialogue, vérifie que chaque champ de la page porte son nom,
# poste le formulaire avec deux champs pirates (PATH, INTRUS), et exige :
# les valeurs envoyées ; une variable non envoyée rendue avec sa valeur de
# départ ; aucun nom inconnu du dialogue ; EXIT du bouton.
#
# Usage : garde_html_serve.sh <htmlsermo>
# Codes : 0 = tout tient · 1 = un défaut · 2 = outil ou binaire manquant

set -u
BIN="${1:?usage: garde_html_serve.sh /chemin/vers/htmlsermo}"
[[ -x "$BIN" ]] || { echo "binaire introuvable : $BIN" >&2; exit 2; }
command -v curl >/dev/null || { echo "outil manquant : curl" >&2; exit 2; }
T=$(mktemp -d "${TMPDIR:-/tmp}/garde-html-serve.XXXXXX"); trap 'kill "$p" 2>/dev/null; rm -rf "$T"' EXIT
echecs=0
ko() { echo "ECHEC : $*"; echecs=$((echecs + 1)); }

cat >"$T/d.xml" <<'XML'
<window title="serve"><vbox>
<entry><variable>NOM</variable></entry>
<password><variable>SECRET</variable></password>
<searchentry><variable>CHERCHE</variable></searchentry>
<comboboxtext><variable>LANGUE</variable><item>Français</item><item>English</item></comboboxtext>
<list><variable>CHOIX</variable><item>un</item><item>deux</item></list>
<switch><variable>ACTIF</variable></switch>
<togglebutton><label>t</label><variable>BASCULE</variable></togglebutton>
<entry><variable>RESTE</variable><default>depart</default></entry>
<button ok></button>
</vbox></window>
XML

SERMO_HTML_MODE=serve SERMO_HTML_TIMEOUT=20 "$BIN" --file="$T/d.xml" >"$T/sortie" 2>"$T/err" &
p=$!
url=
for _ in $(seq 1 100); do
    url=$(sed -n 's|^htmlsermo: ouvrez \(http://127\.0\.0\.1:[0-9]*/[0-9a-f]*\).*|\1|p' "$T/err" | head -1)
    [ -n "$url" ] && break
    sleep 0.1
done
[ -n "$url" ] || { echo "ECHEC : aucune adresse annoncée ($(head -2 "$T/err"))"; exit 1; }

curl -s --max-time 5 "$url" >"$T/page.html"
for v in NOM SECRET CHERCHE LANGUE CHOIX ACTIF BASCULE RESTE; do
    grep -q "name=\"$v\"" "$T/page.html" || ko "la page n'a pas de champ name=\"$v\" : le navigateur ne l'enverrait pas"
done

curl -s --max-time 5 -X POST "$url" \
    --data-urlencode "NOM=Jean" --data-urlencode "SECRET=s3cr\$t" --data-urlencode "CHERCHE=x" \
    --data-urlencode "LANGUE=English" --data-urlencode "CHOIX=deux" --data-urlencode "ACTIF=true" \
    --data-urlencode "BASCULE=true" --data-urlencode "PATH=/tmp/piege" --data-urlencode "INTRUS=x" \
    --data-urlencode "EXIT=OK" >/dev/null
wait "$p"; p=

attendu() {   # attendu NOM VALEUR_ÉCHAPPÉE
    grep -qFx "$1=\"$2\"" "$T/sortie" || ko "$1 attendu « $2 » ; sortie : $(tr '\n' ' ' <"$T/sortie")"
}
attendu NOM Jean
attendu SECRET 's3cr\$t'
attendu LANGUE English
attendu CHOIX deux
attendu ACTIF true
attendu BASCULE true
attendu RESTE depart
attendu EXIT OK
for v in PATH INTRUS; do
    grep -q "^$v=" "$T/sortie" && ko "$v, absent du dialogue, ressort dans la sortie (eval le reprendrait)"
done
# TÉMOIN : la sortie n'est pas vide (un banc qui ne lit rien ne prouve rien).
[ -s "$T/sortie" ] || ko "sortie vide"

if [ "$echecs" -gt 0 ]; then echo "garde_html_serve : $echecs défaut(s)"; exit 1; fi
echo "OK : mode serve — chaque champ nommé, valeurs reçues rendues, valeur de départ gardée, PATH et INTRUS ignorés"
