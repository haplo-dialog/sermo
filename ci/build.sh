#!/bin/sh
# build.sh — Script de build universel sermo
# sermo 1.1.0 — GPL-2.0-or-later
#
# Usage :
#   ./ci/build.sh                        # build gtk3sermo (port par défaut)
#   ./ci/build.sh all                    # build les deux ports
#   ./ci/build.sh gtk4sermo              # build un seul port
#   ./ci/build.sh gtk3sermo --test       # build + tests
#   ./ci/build.sh gtk3sermo --clean      # nettoyage

set -e

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
ROOT_DIR="$(dirname "$SCRIPT_DIR")"
BUILD_DIR="${ROOT_DIR}/_build"
INSTALL_DIR="${BUILD_DIR}/install"
JOBS="${JOBS:-$(nproc 2>/dev/null || echo 4)}"
RUN_TESTS=0
CLEAN=0

# ── Couleurs (uniquement sur un terminal interactif) ─────────────────────────
if [ -t 1 ]; then
    RED='\033[0;31m'; GREEN='\033[0;32m'; YELLOW='\033[1;33m'
    BLUE='\033[0;34m'; NC='\033[0m'
else
    RED=''; GREEN=''; YELLOW=''; BLUE=''; NC=''
fi
ok()   { printf "${GREEN}✅ %s${NC}\n" "$*"; }
warn() { printf "${YELLOW}⚠  %s${NC}\n" "$*"; }
err()  { printf "${RED}❌ %s${NC}\n" "$*" >&2; exit 1; }
info() { printf "${BLUE}── %s${NC}\n" "$*"; }

# ── Parse arguments ───────────────────────────────────────────────────────────
PORTS=""
for arg in "$@"; do
    case "$arg" in
        --test)  RUN_TESTS=1 ;;
        --clean) CLEAN=1 ;;
        all)     PORTS="gtk3sermo gtk4sermo" ;;
        gtk3sermo|gtk4sermo) PORTS="$PORTS $arg" ;;
        *) err "Argument inconnu : $arg (attendu : all, gtk3sermo, gtk4sermo, --test, --clean)" ;;
    esac
done

# Par défaut : gtk3sermo
[ -z "$PORTS" ] && PORTS="gtk3sermo"

# ── Répertoires source ────────────────────────────────────────────────────────
src_dir() {
    case "$1" in
        gtk3sermo)  echo "${ROOT_DIR}/gtk3sermo/gtk3sermo_1.1.1" ;;
        gtk4sermo)  echo "${ROOT_DIR}/gtk4sermo/gtk4sermo_1.1.1" ;;
    esac
}

build_system() {
    case "$1" in
        gtk3sermo|gtk4sermo) echo "autotools" ;;
    esac
}

# ── Build autotools ───────────────────────────────────────────────────────────
build_autotools() {
    port="$1"; src="$(src_dir $port)"
    bdir="${BUILD_DIR}/${port}"

    info "Building ${port} (autotools) → ${bdir}"
    mkdir -p "$bdir"

    cd "$src"
    [ -f configure ] || autoreconf -fi

    ./configure \
        --prefix="${INSTALL_DIR}" \
        CFLAGS="-O2 -Wall -Wextra -Werror=format-security -D_FORTIFY_SOURCE=3 \
                -D_POSIX_C_SOURCE=200809L -fstack-protector-strong \
                -fstack-clash-protection -fcf-protection=full -fPIE" \
        LDFLAGS="-pie -Wl,-z,relro -Wl,-z,now -Wl,-z,noexecstack"

    make -j"${JOBS}"
    ok "${port} compilé"

    if [ "$RUN_TESTS" = "1" ]; then
        info "Tests unitaires ${port} (make check)"
        chklog="${BUILD_DIR}/${port}-check.log"
        make check > "$chklog" 2>&1 || warn "Tests ${port} : échec non bloquant"
        # Ne jamais annoncer « testé » quand rien n'a tourné : make check
        # traverse les sous-répertoires et rend 0 même sans une seule épreuve.
        total=$(sed -n 's/^# TOTAL: *\([0-9]*\).*/\1/p' "$chklog" | head -1)
        if [ -z "$total" ] || [ "$total" = "0" ]; then
            warn "${port} : AUCUN test unitaire câblé dans l'autotools (journal : $chklog)"
            warn "         les épreuves de ce port sont à la racine : tests/ (lancées par le pipeline)"
        else
            fails=$(sed -n 's/^# FAIL: *\([0-9]*\).*/\1/p' "$chklog" | head -1)
            ok "${port} : ${total} test(s) unitaire(s), ${fails:-0} échec(s)"
        fi
    fi

    make install
    ok "${port} installé dans ${INSTALL_DIR}"
}

# ── Build CMake ───────────────────────────────────────────────────────────────
build_cmake() {
    port="$1"; src="$(src_dir $port)"
    bdir="${BUILD_DIR}/${port}"

    info "Building ${port} (CMake) → ${bdir}"

    cmake -S "$src" -B "$bdir" \
        -DCMAKE_BUILD_TYPE=Release \
        -DCMAKE_INSTALL_PREFIX="${INSTALL_DIR}" \
        -DCMAKE_EXPORT_COMPILE_COMMANDS=ON

    cmake --build "$bdir" -j"${JOBS}"
    ok "${port} compilé"

    if [ "$RUN_TESTS" = "1" ]; then
        info "CTest ${port}"
        (cd "$bdir" && ctest --output-on-failure -j"${JOBS}") || \
            warn "CTest ${port} : échec non bloquant"
    fi

    cmake --install "$bdir"
    ok "${port} installé dans ${INSTALL_DIR}"
}

# ── Clean ─────────────────────────────────────────────────────────────────────
if [ "$CLEAN" = "1" ]; then
    info "Nettoyage ${BUILD_DIR}"
    rm -rf "${BUILD_DIR}"
    ok "Nettoyage terminé"
    exit 0
fi

# ── Build ─────────────────────────────────────────────────────────────────────
mkdir -p "${BUILD_DIR}" "${INSTALL_DIR}"

FAILED=""
for port in $PORTS; do
    printf "\n"
    info "=== ${port} ==="
    sys="$(build_system $port)"
    if [ "$sys" = "autotools" ]; then
        build_autotools "$port" || { FAILED="$FAILED $port"; warn "${port} ÉCHEC"; }
    else
        build_cmake "$port" || { FAILED="$FAILED $port"; warn "${port} ÉCHEC"; }
    fi
done

printf "\n"
if [ -n "$FAILED" ]; then
    err "Ports en échec :${FAILED}"
else
    ok "Tous les ports compilés avec succès"
    ok "Binaires dans : ${INSTALL_DIR}/bin/"
    ls "${INSTALL_DIR}/bin/" 2>/dev/null | sed 's/^/  /'
fi
