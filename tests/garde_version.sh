#!/bin/bash
# SPDX-License-Identifier: GPL-2.0-or-later
# garde_version.sh — le dépôt n'a qu'un seul numéro de version : le fichier VERSION.
#
# POURQUOI CE BANC EXISTE
#
# Le numéro de version a longtemps été recopié à la main : quatre CMakeLists le
# portaient en dur, deux config.h écrits à la main demandaient en commentaire de
# le « garder en accord » avec leur CMakeLists, et quatre backends n'en
# déclaraient aucun. Rien ne vérifiait cet accord. Une version oubliée quelque
# part ne se voit pas à la compilation : le binaire sort, il annonce simplement
# un numéro faux — la panne la plus discrète qui soit.
#
# Depuis, VERSION (à la racine) est la seule source : chaque CMakeLists le lit,
# les config.h sont engendrés, et les pages de manuel sont substituées à la
# construction. Ce banc refuse toute copie qui repartirait en vie propre.
#
# Usage : garde_version.sh [racine]   (défaut : la racine du dépôt)

set -u
RACINE="${1:-$(cd "$(dirname "$0")/.." && pwd)}"
[ -d "$RACINE" ] || { echo "usage: $0 [racine]" >&2; exit 2; }
cd "$RACINE" || exit 2

[ -f VERSION ] || { echo "ECHEC : VERSION est absent de $RACINE" >&2; exit 1; }
ATTENDUE=$(tr -d '[:space:]' < VERSION)
[ -n "$ATTENDUE" ] || { echo "ECHEC : VERSION est vide" >&2; exit 1; }

ecarts=0
signale() { printf '  %s\n' "$1"; ecarts=$((ecarts + 1)); }

# 1. Aucun project(... VERSION <littéral> ...) : tous doivent lire ${SERMO_VERSION}.
#    grep -a partout : un CMakeLists en ISO-8859-1 serait pris pour du binaire et
#    sauté sans un mot, et le banc rendrait « 0 trouvé » sur un dépôt fautif.
while IFS= read -r f; do
    [ -n "$f" ] || continue
    if grep -aqE 'project\([^)]*VERSION[[:space:]]+[0-9]+\.[0-9]+' "$f"; then
        signale "$f : project() porte un numéro en dur, il doit lire \${SERMO_VERSION}"
    fi
done <<< "$(git ls-files -- '*/CMakeLists.txt' 'CMakeLists.txt' 2>/dev/null)"

# 2. Le config.h public du cœur porte une valeur de repli : c'est elle que lisent
#    les backends qui n'ont pas de config.h à eux. Elle doit suivre VERSION.
REPLI=$(grep -a '^#define PACKAGE_VERSION' libsermocore/include/config.h 2>/dev/null \
        | sed -E 's/.*"([^"]+)".*/\1/')
[ "$REPLI" = "$ATTENDUE" ] || \
    signale "libsermocore/include/config.h : PACKAGE_VERSION=${REPLI:-<absent>}, attendu $ATTENDUE"

# 3. La version amont du paquet Debian.
DEB=$(sed -n '1s/.*(\([^-]*\).*/\1/p' debian/changelog 2>/dev/null)
[ "$DEB" = "$ATTENDUE" ] || \
    signale "debian/changelog : version amont ${DEB:-<absente>}, attendu $ATTENDUE"

# 4. Aucun config.h écrit à la main dans un backend : ils sont engendrés depuis
#    config.h.in. Un fichier versionné masquerait celui du répertoire de build.
while IFS= read -r f; do
    [ -n "$f" ] || continue
    signale "$f : config.h versionné — il doit être engendré depuis config.h.in"
done <<< "$(git ls-files -- 'sermo-backend-*/src/config.h' 2>/dev/null)"

# 5. Ni page de manuel ni manuel Texinfo ne porte de numéro en dur : tous
#    viennent d'un .in substitué à la construction. Une page recopiée à la main
#    reste juste le jour où on l'écrit, puis ment en silence au bump suivant.
while IFS= read -r f; do
    [ -n "$f" ] || continue
    if grep -aqE '[0-9]+\.[0-9]+\.[0-9]+' "$f"; then
        signale "$f : numéro en dur — ce document doit venir d'un .in substitué"
    fi
done <<< "$(git ls-files -- '*.1' '*.5' '*.texi' 2>/dev/null)"

# 6. Le serveur MCP annonce sa version aux clients : elle se lit dans VERSION,
#    jamais écrite en dur. Une valeur figée fait croire à un serveur immobile.
SERVIE=$(printf '%s\n' '{"jsonrpc":"2.0","id":1,"method":"initialize","params":{}}' \
         | python3 sermoman-mcp/server.py 2>/dev/null \
         | sed -n '1s/.*"serverInfo".*"version": *"\([^"]*\)".*/\1/p')
[ "$SERVIE" = "$ATTENDUE" ] || \
    signale "sermoman-mcp : version servie ${SERVIE:-<illisible>}, attendu $ATTENDUE"

# 7. Les documents qui disent quelle version installer, ou laquelle est la
#    version courante, suivent VERSION. La 2.7.5 est sortie avec un README qui
#    faisait encore télécharger et installer la 2.7.4 — la version même qu'elle
#    corrigeait : rien ne reliait ces lignes au fichier VERSION.
#    Chaque motif ne vise QUE la version courante. Les numéros historiques
#    (première version publiée, paquets des captures, mesures) s'écrivent sous
#    d'autres formes et ne sont pas vus. Une nouvelle forme de « version
#    courante » dans ces documents doit être ajoutée ici.
#    Chaque document doit fournir au moins une mention : un document réécrit
#    dont aucun motif ne trouverait plus rien rendrait sinon un vert muet.
V='[0-9]+\.[0-9]+\.[0-9]+'
DOCS_COURANTE=(README.md README.en.md MIGRATION.md MIGRATION.en.md
               PACKAGING.md PACKAGING.en.md VERSIONING.md VERSIONING.en.md
               MANUEL_UTILISATEUR.md MANUEL_UTILISATEUR.en.md
               sermoman-mcp/data/heritage.md sermoman-mcp/data/reference-xml.txt)
MOTIFS_COURANTE=(
    "releases/v$V"                                   # lien vers la release
    "\[\*\*v$V\*\*\]"                                # texte de ce lien
    "generic/sermo/$V"                               # registre de paquets
    "_$V-[0-9]+_"                                    # nom d'un .deb à installer
    "Version \*\*$V\*\*"                             # README : ligne d'état
    "\*\*Version ?:\*\* $V"                          # en-tête du manuel
    "Version courante : \`$V\`"                      # heritage.md
    "\`Version: $V\`"                                # champ de sermocore.pc
    "(une pour tout|one for everything) \($V\)"      # MIGRATION : tableau
    "^\`$V\`\."                                      # VERSIONING : version courante
    "\`$V-[0-9]+\` est la première"                  # VERSIONING : révision Debian,
    "construction Debian de la $V"                   #   phrase française sur 2 lignes
    "\`$V-[0-9]+\` is the first Debian build of $V"  #   et anglaise sur une seule
    "^sermo $V "                                     # pied de la page de manuel rendue
)
ARGS_COURANTE=()
for m in "${MOTIFS_COURANTE[@]}"; do ARGS_COURANTE+=(-e "$m"); done
for f in "${DOCS_COURANTE[@]}"; do
    if [ ! -f "$f" ]; then
        signale "$f : absent — le retirer de DOCS_COURANTE, ou le rétablir"
        continue
    fi
    TROUVES=$(grep -naoE "${ARGS_COURANTE[@]}" "$f")
    if [ -z "$TROUVES" ]; then
        signale "$f : aucune mention de la version courante — motifs à revoir"
        continue
    fi
    while IFS= read -r t; do
        for n in $(printf '%s\n' "${t#*:}" | grep -oE "$V"); do
            [ "$n" = "$ATTENDUE" ] || \
                signale "$f:${t%%:*} : « ${t#*:} » désigne $n, attendu $ATTENDUE"
        done
    done <<< "$TROUVES"
done

if [ "$ecarts" -gt 0 ]; then
    echo "ECHEC : $ecarts écart(s) au fichier VERSION ($ATTENDUE) :" >&2
    exit 1
fi
echo "OK : tout le dépôt s'accorde sur VERSION = $ATTENDUE"
