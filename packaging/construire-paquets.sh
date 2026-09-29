#!/bin/bash
# SPDX-License-Identifier: GPL-2.0-or-later
# packaging/construire-paquets.sh — les paquets Debian de sermo, construits et
# éprouvés dans une Debian testing VIERGE.
#
# POURQUOI UN CONTENEUR
#
# Un paquet construit sur un poste de travail hérite du poste. Les .deb 2.5.0
# dépendaient de libdrm2-amdgpu, venue d'un dépôt tiers installé sur la machine
# de construction : ininstallables sur une Debian ordinaire. Ici ne compte que ce
# que debian/control déclare.
#
# CE QUE LE SCRIPT FAIT
#  1. archive le COMMIT courant (git archive) : ni fichier non suivi, ni
#     modification en cours n'entre dans le paquet ;
#  2. conteneur jetable : apt-get build-dep, puis dpkg-buildpackage (source et
#     binaires, non signés), par un utilisateur ordinaire — debian/rules y joue
#     les tests unitaires et le banc XML ;
#  3. lintian sur le .changes (sur l'hôte) : aucune erreur admise ;
#  4. conteneur vierge : installation de tous les paquets ; chaque binaire
#     trouve ses bibliothèques (ldd) et répond à --version ; l'alternative
#     « sermo » choisit gtk3sermo, page de manuel et Texinfo compris ; un
#     dialogue tourne en terminal ; puis purge, et plus rien ne reste — ni
#     fichier, ni alternative ;
#  5. si SERMO_PAQUETS_1X désigne un dossier de paquets 1.x : conteneur vierge,
#     installation de la 1.x, puis les paquets 2.x servis par un dépôt apt local
#     et « apt-get upgrade » — comme un utilisateur qui suit un dépôt. Les
#     paquets de transition doivent faire venir les nouveaux backends d'eux-mêmes.
#
# TÉLÉCHARGEMENTS
#
# Les conteneurs téléchargent depuis les miroirs Debian : listes de paquets et
# dépendances (de l'ordre de 550 Mo la première fois, surtout pour construire).
# Deux dossiers réduisent cela :
#  - SERMO_CACHE_APT garde les .deb et les listes d'un conteneur à l'autre ;
#  - le cache apt de l'hôte, monté en lecture seule, fournit les .deb déjà
#    présents sur le poste (même nom, même version) : apt vérifie leur somme
#    avant de s'en servir et ne télécharge que le reste.
#
# Usage : packaging/construire-paquets.sh
# Variables :
#   SERMO_SORTIE      dossier des résultats (défaut : packaging/sortie)
#   SERMO_IMAGE       image de base (défaut : debian:testing)
#   SERMO_CACHE_APT   cache partagé entre conteneurs (défaut : <sortie>/cache-apt)
#   SERMO_CACHE_HOTE  cache apt de l'hôte, lu seulement (défaut :
#                     /var/cache/apt/archives ; vide = ne pas s'en servir)
#   SERMO_PAQUETS_1X  dossier contenant gtk3sermo_1.*.deb, gtksermo_1.*.deb,
#                     gtk4sermo_1.*.deb et qt6sermo_1.*.deb (facultatif)
#   SERMO_SANS_CONSTRUIRE=1  reprendre les paquets déjà dans <sortie>/paquets
#                     (étapes 3 à 5 seulement, pour rejouer une vérification)
# Codes : 0 = tout est vert · 1 = une étape a échoué · 2 = outil manquant

set -uo pipefail
RACINE=$(git rev-parse --show-toplevel 2>/dev/null) || { echo "pas un dépôt git" >&2; exit 2; }
cd "$RACINE" || exit 2
for t in docker git dpkg-parsechangelog lintian xz; do
    command -v "$t" >/dev/null || { echo "outil manquant : $t" >&2; exit 2; }
done
docker info >/dev/null 2>&1 || { echo "docker ne répond pas (service arrêté, ou droits sur la socket)" >&2; exit 2; }

SORTIE="${SERMO_SORTIE:-$RACINE/packaging/sortie}"
IMAGE="${SERMO_IMAGE:-debian:testing}"
CACHE="${SERMO_CACHE_APT:-$SORTIE/cache-apt}"
CACHE_HOTE="${SERMO_CACHE_HOTE-/var/cache/apt/archives}"
SOURCE=$(dpkg-parsechangelog -S Source)
VERSION=$(dpkg-parsechangelog -S Version)
AMONT=${VERSION%-*}
PAQUETS="$SORTIE/paquets"
TMP=$(mktemp -d); trap 'rm -rf "$TMP"' EXIT
mkdir -p "$SORTIE" "$CACHE/archives" "$CACHE/listes"
if [ "${SERMO_SANS_CONSTRUIRE:-0}" = 1 ]; then
    ls "$PAQUETS/${SOURCE}_${VERSION}_"*.changes >/dev/null 2>&1 \
        || { echo "SERMO_SANS_CONSTRUIRE=1 : aucun paquet $VERSION dans $PAQUETS" >&2; exit 2; }
else
    rm -rf "$PAQUETS"; mkdir -p "$PAQUETS"
fi

etape() { printf '\n── %s\n' "$*"; }
echec() { printf 'ÉCHEC : %s\n' "$*"; exit 1; }

if [ "${SERMO_SANS_CONSTRUIRE:-0}" != 1 ]; then
    etape "0. $SOURCE $VERSION, depuis le commit $(git rev-parse --short HEAD)"
    if [ -n "$(git status --porcelain --untracked-files=no)" ]; then
        echo "⚠️  modifications non commitées : elles ne sont PAS dans les paquets (git archive)."
    fi
    # L'archive amont n'emporte pas debian/ : sinon le paquet source n'apporte
    # « aucun changement » par rapport à elle (lintian : no-debian-changes).
    # debian/ voyage à part et rejoint l'arbre dans le conteneur.
    git archive --format=tar --prefix="$SOURCE-$AMONT/" HEAD -- . ':(exclude)debian' \
        | xz -T0 -9 > "$PAQUETS/${SOURCE}_${AMONT}.orig.tar.xz" || echec "archive du commit"
    git archive --format=tar --prefix="$SOURCE-$AMONT/" HEAD -- debian > "$TMP/debian.tar" \
        || echec "archive de debian/"
fi

# ── scripts joués dans les conteneurs ───────────────────────────────────────
cat > "$TMP/apt.sh" <<'EOF'
set -e
export DEBIAN_FRONTEND=noninteractive
# Cache et listes sont des dossiers de l'hôte : on les lui rend en sortant, quoi
# qu'il arrive (A_RENDRE ajoute d'autres dossiers).
rendre() { chown -R "$UID_HOTE:$GID_HOTE" /var/cache/apt/archives /var/lib/apt/lists ${A_RENDRE:-} 2>/dev/null || true; }
trap rendre EXIT
# L'image vide le cache après chaque installation : on garde les .deb.
rm -f /etc/apt/apt.conf.d/docker-clean
echo 'Binary::apt::APT::Keep-Downloaded-Packages "true";' > /etc/apt/apt.conf.d/10garder
mkdir -p /var/cache/apt/archives/partial /var/lib/apt/lists/partial
chown -R _apt:root /var/cache/apt/archives/partial /var/lib/apt/lists/partial
apt-get update -q
# Copie dans le cache les .deb que l'hôte possède déjà pour ce qu'apt s'apprête à
# télécharger. apt en vérifie la somme ; un fichier différent serait rejeté.
amorcer() {
    [ -d /hote-archives ] || return 0
    local n=0 fichier
    while read -r _ fichier _; do
        if [ -f "/hote-archives/$fichier" ] && [ ! -f "/var/cache/apt/archives/$fichier" ]; then
            cp "/hote-archives/$fichier" /var/cache/apt/archives/ && n=$((n + 1))
        fi
    done < <(apt-get --print-uris -qq -y "$@" 2>/dev/null)
    echo "· $n paquet(s) repris du cache de l'hôte"
}
EOF

cat > "$TMP/construire.sh" <<'EOF'
. /scripts/apt.sh
A_RENDRE=/travail
apt-get install -y -q --no-install-recommends xz-utils
cd /travail
tar xf "${SOURCE}_${AMONT}.orig.tar.xz"
tar xf /scripts/debian.tar
cd "$SOURCE-$AMONT"
amorcer build-dep ./
apt-get build-dep -y -q ./
# Construire en utilisateur ordinaire, comme un démon de construction Debian :
# Rules-Requires-Root: no le permet, et un test qui aurait besoin de root se voit.
useradd -m -o -u "$UID_HOTE" constructeur
chown -R constructeur /travail
runuser -u constructeur -- env HOME=/home/constructeur dpkg-buildpackage -us -uc
cd /travail && rm -rf "$SOURCE-$AMONT"
EOF

cat > "$TMP/installer.sh" <<'EOF'
. /scripts/apt.sh
cd /paquets
debs=$(ls ./*_"$VERSION"_*.deb | grep -v -- -dbgsym_)
amorcer install $debs
apt-get install -y -q $debs
echo "· installés :"
dpkg-query -W -f '  ${Package} ${Version}\n' | grep -E 'sermo|gtkdialog'
[ -z "$(dpkg --audit)" ] || { echo "ÉCHEC : dpkg --audit signale des paquets à moitié installés"; dpkg --audit; exit 1; }
for b in gtk3sermo gtk4sermo qt6sermo fltk1sermo efl1sermo sdl3sermo ncursessermo; do
    if ldd "/usr/bin/$b" | grep -q 'not found'; then
        echo "ÉCHEC : $b, bibliothèque introuvable"; ldd "/usr/bin/$b" | grep 'not found'; exit 1
    fi
    v=$(env QT_QPA_PLATFORM=offscreen SDL_VIDEODRIVER=offscreen ELM_ENGINE=buffer SERMO_NCURSES_BATCH=1 \
        timeout 20 "/usr/bin/$b" --version 2>/dev/null | head -1)
    case "$v" in
        *"$AMONT"*) echo "· $b : $v" ;;
        *) echo "ÉCHEC : $b --version rend « $v »"; exit 1 ;;
    esac
done
[ "$(readlink -f /usr/bin/sermo)" = /usr/bin/gtk3sermo ] || { echo "ÉCHEC : l'alternative sermo ne mène pas à gtk3sermo"; exit 1; }
[ "$(readlink -f /usr/share/man/man1/sermo.1.gz)" = /usr/share/man/man1/gtk3sermo.1.gz ] || { echo "ÉCHEC : la page sermo(1) ne suit pas l'alternative"; exit 1; }
[ "$(readlink -f /usr/share/info/sermo.info.gz)" = /usr/share/info/gtk3sermo.info.gz ] || { echo "ÉCHEC : le manuel Texinfo sermo ne suit pas l'alternative"; exit 1; }
[ "$(readlink -f /usr/bin/gtkdialog)" = /usr/bin/gtk3sermo ] || { echo "ÉCHEC : gtkdialog ne mène pas à gtk3sermo"; exit 1; }
echo "· alternative sermo → gtk3sermo (man et info suivent) ; gtkdialog → gtk3sermo"
dialogue='<window><vbox><entry><variable>E</variable><default>paquet</default></entry><timer visible="false"><variable>T</variable><action>exit:fin</action></timer></vbox></window>'
if ! MAIN_DIALOG="$dialogue" SERMO_NCURSES_BATCH=1 timeout 20 ncursessermo --program=MAIN_DIALOG </dev/null 2>/dev/null | grep -qx 'E="paquet"'; then
    echo "ÉCHEC : le dialogue en terminal n'a pas exporté E=\"paquet\""; exit 1
fi
echo "· dialogue en terminal : E=\"paquet\" exporté"
apt-get purge -y -q $(dpkg-query -W -f '${Package}\n' | grep -E '^(sermo-|gtk3sermo$|gtk4sermo$|qt6sermo$|gtksermo$)')
# Les formes de noms que posent les paquets, et elles seules : « *sermo* »
# attrapait usermod. (Un -regex à alternatives manquait des noms sous find :
# éprouvé sur un arbre factice, 15 noms attendus.)
restes=$(find / -xdev \( -name '*sermo' -o -name '*sermo.*' -o -name 'sermo-*' -o -name '*sermocore*' -o -name 'gtkdialog*' \) \
    -not -path '/proc/*' -not -path '/var/cache/apt/*' -not -path '/var/lib/apt/lists/*' 2>/dev/null || true)
[ -z "$restes" ] || { echo "ÉCHEC : il reste après purge :"; echo "$restes"; exit 1; }
if update-alternatives --query sermo >/dev/null 2>&1; then echo "ÉCHEC : l'alternative sermo survit à la purge"; exit 1; fi
echo "· purge : aucun fichier, aucune alternative ne reste"
EOF

cat > "$TMP/monter.sh" <<'EOF'
. /scripts/apt.sh
cd /anciens
anciens=""
for p in gtk3sermo gtksermo gtk4sermo qt6sermo; do
    f=$(ls -v ./"$p"_1.*.deb 2>/dev/null | tail -1)
    [ -n "$f" ] || { echo "ÉCHEC : aucun $p 1.x dans SERMO_PAQUETS_1X"; exit 1; }
    anciens="$anciens $f"
done
amorcer install $anciens
apt-get install -y -q $anciens
echo "· 1.x installée :"
dpkg-query -W -f '  ${Package} ${Version}\n' | grep -E 'sermo|gtkdialog'
# Les paquets 2.x servis par un dépôt local, comme par un vrai dépôt.
amorcer install apt-utils
apt-get install -y -q --no-install-recommends apt-utils
mkdir -p /depot && cp /paquets/*_"$VERSION"_*.deb /depot/ && rm -f /depot/*-dbgsym_*
( cd /depot && apt-ftparchive packages . > Packages )
echo 'deb [trusted=yes] file:/depot ./' > /etc/apt/sources.list.d/sermo-local.list
apt-get update -q
# « apt upgrade » doit prévoir les quatre paquets 1.x en 2.x ET les nouveaux
# backends, sans rien retenir. Simulé seulement : le jouer pour de bon mettrait
# aussi à jour toute l'image, et téléchargerait pour rien.
plan=$(apt-get -s upgrade --with-new-pkgs)
for p in gtk3sermo gtk4sermo qt6sermo gtksermo sermo-backend-gtk3 sermo-backend-gtk4 sermo-backend-qt6 sermo-gtkdialog; do
    grep -q "^Inst $p " <<<"$plan" || { echo "ÉCHEC : « apt upgrade » ne prévoit pas $p"; grep -E 'sermo|gtkdialog|held back' <<<"$plan"; exit 1; }
done
echo "· « apt upgrade » (simulé) prévoit les quatre paquets 1.x en $VERSION et fait venir sermo-backend-gtk3, -gtk4, -qt6 et sermo-gtkdialog"
amorcer install gtk3sermo gtk4sermo qt6sermo gtksermo
apt-get install -y -q gtk3sermo gtk4sermo qt6sermo gtksermo
echo "· après la mise à jour des quatre paquets 1.x :"
dpkg-query -W -f '  ${Package} ${Version}\n' | grep -E 'sermo|gtkdialog'
[ -z "$(dpkg --audit)" ] || { echo "ÉCHEC : dpkg --audit signale des paquets à moitié installés"; dpkg --audit; exit 1; }
for p in gtk3sermo gtk4sermo qt6sermo gtksermo sermo-backend-gtk3 sermo-backend-gtk4 sermo-backend-qt6 sermo-gtkdialog; do
    v=$(dpkg-query -W -f '${db:Status-Abbrev} ${Version}' "$p" 2>/dev/null || true)
    [ "$v" = "ii  $VERSION" ] || { echo "ÉCHEC : $p est « ${v:-absent} », attendu ii $VERSION"; exit 1; }
done
for b in gtk3sermo gtk4sermo qt6sermo; do
    v=$(env QT_QPA_PLATFORM=offscreen timeout 20 "/usr/bin/$b" --version 2>/dev/null | head -1)
    case "$v" in *"$AMONT"*) ;; *) echo "ÉCHEC : /usr/bin/$b rend « $v »"; exit 1 ;; esac
done
[ "$(readlink -f /usr/bin/gtkdialog)" = /usr/bin/gtk3sermo ] || { echo "ÉCHEC : gtkdialog ne mène pas à gtk3sermo"; exit 1; }
echo "· passage 1.x → $VERSION : les quatre paquets 1.x sont devenus des paquets de transition, ils ont fait venir les nouveaux backends, et les binaires sont ceux de la $VERSION"
# Ce que PACKAGING.md conseille ensuite : marquer les paquets 2.x voulus, retirer
# les paquets de transition — et autoremove ne doit rien emporter de sermo.
apt-get install -y -q sermo-backend-gtk3 sermo-backend-gtk4 sermo-backend-qt6 sermo-gtkdialog
apt-get purge -y -q gtk3sermo gtk4sermo qt6sermo gtksermo
if apt-get -s autoremove | grep -E '^Remv (sermo-|gtk[34]?sermo|qt6sermo)'; then
    echo "ÉCHEC : après le retrait des paquets de transition, autoremove emporterait sermo"; exit 1
fi
v=$(timeout 20 gtk3sermo --version 2>/dev/null | head -1)
case "$v" in *"$AMONT"*) ;; *) echo "ÉCHEC : gtk3sermo absent après le retrait des paquets de transition"; exit 1 ;; esac
echo "· paquets de transition retirés : les backends 2.x restent, autoremove n'emporterait rien de sermo"
EOF

ENV_COMMUN=(-e "SOURCE=$SOURCE" -e "VERSION=$VERSION" -e "AMONT=$AMONT" -e "UID_HOTE=$(id -u)" -e "GID_HOTE=$(id -g)")
MONTAGES=(-v "$TMP":/scripts:ro -v "$CACHE/archives":/var/cache/apt/archives -v "$CACHE/listes":/var/lib/apt/lists)
if [ -n "$CACHE_HOTE" ] && [ -d "$CACHE_HOTE" ]; then
    MONTAGES+=(-v "$(readlink -f "$CACHE_HOTE")":/hote-archives:ro)
fi
# Ce que les journaux retiennent des téléchargements (apt parle anglais dans l'image).
# Les lignes que les scripts des conteneurs écrivent pour le lecteur (« · »,
# paquets listés, échecs), sans le bavardage d'apt.
resume() { grep -a -E '^· |^  (sermo-|gtk3sermo|gtk4sermo|qt6sermo|gtksermo)[^ ]* [0-9]|^ÉCHEC' "$1" | grep -v 'cache de l.hôte'; }
telecharge() { grep -a -E '^(Need to get|Fetched) ' "$1" | sed 's/^/  apt : /'; grep -a '^· .*cache de l.hôte' "$1" | sed 's/^/  /'; }

if [ "${SERMO_SANS_CONSTRUIRE:-0}" = 1 ]; then
    etape "1-2. construction : reprise des paquets de $PAQUETS (SERMO_SANS_CONSTRUIRE=1)"
else
    etape "1-2. construction dans $IMAGE (journal : $SORTIE/construction.log)"
    docker run --rm "${ENV_COMMUN[@]}" "${MONTAGES[@]}" -v "$PAQUETS":/travail \
        "$IMAGE" bash /scripts/construire.sh > "$SORTIE/construction.log" 2>&1
    rc=$?
    telecharge "$SORTIE/construction.log"
    [ $rc -eq 0 ] || { tail -25 "$SORTIE/construction.log"; echec "construction"; }
    grep -a 'Tests unitaires :\|Résultats pour' "$SORTIE/construction.log" | sed 's/\x1b\[[0-9;]*m//g; s#/[^ ]*/construit/#…/#'
fi
ls "$PAQUETS" | sed 's/^/  /'

etape "3. lintian (journal : $SORTIE/lintian.txt)"
lintian --display-info --pedantic "$PAQUETS/${SOURCE}_${VERSION}_"*.changes > "$SORTIE/lintian.txt" 2>&1
if [ -s "$SORTIE/lintian.txt" ]; then cut -c1-2 "$SORTIE/lintian.txt" | sort | uniq -c | sed 's/^/  /'; else echo "  aucune remarque"; fi
[ "$(grep -c '^E:' "$SORTIE/lintian.txt")" -eq 0 ] || { grep '^E:' "$SORTIE/lintian.txt"; echec "lintian signale des erreurs"; }
echo "  lintian : aucune erreur ($(lintian --show-overrides "$PAQUETS/${SOURCE}_${VERSION}_"*.changes 2>/dev/null | grep -c '^O:') exception(s) justifiée(s) dans debian/*.lintian-overrides)"

etape "4. installation, vérification et purge dans un $IMAGE vierge (journal : $SORTIE/installation.log)"
docker run --rm "${ENV_COMMUN[@]}" "${MONTAGES[@]}" -v "$PAQUETS":/paquets:ro \
    "$IMAGE" bash /scripts/installer.sh > "$SORTIE/installation.log" 2>&1
rc=$?
telecharge "$SORTIE/installation.log"
resume "$SORTIE/installation.log"
[ $rc -eq 0 ] || { tail -15 "$SORTIE/installation.log"; echec "installation, vérification ou purge"; }

if [ -n "${SERMO_PAQUETS_1X:-}" ]; then
    etape "5. passage depuis la 1.x (journal : $SORTIE/passage-1x.log)"
    docker run --rm "${ENV_COMMUN[@]}" "${MONTAGES[@]}" -v "$PAQUETS":/paquets:ro \
        -v "$(readlink -f "$SERMO_PAQUETS_1X")":/anciens:ro "$IMAGE" bash /scripts/monter.sh > "$SORTIE/passage-1x.log" 2>&1
    rc=$?
    telecharge "$SORTIE/passage-1x.log"
    resume "$SORTIE/passage-1x.log"
    [ $rc -eq 0 ] || { tail -15 "$SORTIE/passage-1x.log"; echec "passage depuis la 1.x"; }
else
    etape "5. passage depuis la 1.x : non joué (SERMO_PAQUETS_1X non posé)"
fi

printf '\nPaquets %s : construits dans %s, lintian sans erreur, installés et purgés proprement.\n' "$VERSION" "$IMAGE"
