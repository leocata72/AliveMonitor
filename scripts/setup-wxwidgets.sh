#!/usr/bin/env bash
# Scarica, compila e installa wxWidgets (build condivisa) per AliveMonitor
# su Linux/macOS. Utile soprattutto su distribuzioni senza wxWidgets 3.2 nei
# repository (es. Ubuntu 22.04, che ha solo la 3.0): in alternativa, dove
# disponibile, usare il pacchetto di sistema (vedi README, "libwxgtk3.2-dev").
#
# Uso:
#   bash scripts/setup-wxwidgets.sh [--wx-version X.Y.Z] [--jobs N] [--force]
set -euo pipefail

WX_VERSION="3.2.11"
JOBS="$(nproc 2>/dev/null || sysctl -n hw.ncpu 2>/dev/null || echo 4)"
FORCE=0

usage() {
    cat <<EOF
Uso: $0 [--wx-version X.Y.Z] [--jobs N] [--force]

Scarica, compila e installa wxWidgets (build condivisa, Release) in
third_party/, rilevata poi in automatico dal CMakeLists.txt di AliveMonitor.
EOF
}

while [[ $# -gt 0 ]]; do
    case "$1" in
        --wx-version) WX_VERSION="$2"; shift 2 ;;
        --jobs)       JOBS="$2"; shift 2 ;;
        --force)      FORCE=1; shift ;;
        -h|--help)    usage; exit 0 ;;
        *) echo "Opzione sconosciuta: $1" >&2; usage; exit 1 ;;
    esac
done

REPO_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
THIRD_PARTY="$REPO_ROOT/third_party"
SOURCE_DIR="$THIRD_PARTY/wxWidgets-$WX_VERSION"
BUILD_DIR="$SOURCE_DIR/build_gcc_shared"
INSTALL_DIR="$THIRD_PARTY/wx-install"
DOWNLOADS_DIR="$THIRD_PARTY/downloads"
TARBALL="$DOWNLOADS_DIR/wxWidgets-$WX_VERSION.tar.bz2"
MARKER="$INSTALL_DIR/.setup-complete"

step() { echo; echo -e "\033[36m==> $*\033[0m"; }

for tool in cmake g++ gcc make curl; do
    command -v "$tool" >/dev/null 2>&1 || { echo "Errore: '$tool' non trovato nel PATH." >&2; exit 1; }
done

if [[ -f "$MARKER" && "$FORCE" -eq 0 ]]; then
    echo "wxWidgets $WX_VERSION già installata in $INSTALL_DIR (usa --force per ricompilare)."
    exit 0
fi

if [[ "$FORCE" -eq 1 ]]; then
    step "Force: rimuovo build/install precedenti"
    rm -rf "$BUILD_DIR" "$INSTALL_DIR"
fi

mkdir -p "$THIRD_PARTY" "$DOWNLOADS_DIR"

if [[ ! -f "$TARBALL" ]]; then
    URL="https://github.com/wxWidgets/wxWidgets/releases/download/v$WX_VERSION/wxWidgets-$WX_VERSION.tar.bz2"
    step "Scarico wxWidgets $WX_VERSION da $URL"
    curl -fL "$URL" -o "$TARBALL"
else
    echo "Tarball già scaricato: $TARBALL"
fi

if [[ ! -d "$SOURCE_DIR" ]]; then
    step "Estraggo in $SOURCE_DIR"
    mkdir -p "$SOURCE_DIR"
    tar -xjf "$TARBALL" -C "$SOURCE_DIR" --strip-components=1
else
    echo "Sorgente già estratto: $SOURCE_DIR"
fi

step "Configuro wxWidgets (condivisa, Release)"
cmake -S "$SOURCE_DIR" -B "$BUILD_DIR" \
    -DCMAKE_BUILD_TYPE=Release \
    -DwxBUILD_SHARED=ON \
    -DwxBUILD_SAMPLES=OFF \
    -DwxBUILD_TESTS=OFF \
    -DwxBUILD_DEMOS=OFF

step "Compilo wxWidgets ($JOBS job paralleli, può richiedere diversi minuti)"
cmake --build "$BUILD_DIR" -j "$JOBS"

step "Installo in $INSTALL_DIR"
cmake --install "$BUILD_DIR" --prefix "$INSTALL_DIR"

touch "$MARKER"

echo
echo -e "\033[32mwxWidgets $WX_VERSION pronta in: $INSTALL_DIR\033[0m"
echo "Ora puoi compilare AliveMonitor normalmente:"
echo "    cmake -S . -B build -DCMAKE_BUILD_TYPE=Release"
echo "    cmake --build build -j$JOBS"
echo
echo "Nota: è una build CONDIVISA (.so): a runtime servirà avere le librerie"
echo "raggiungibili (stessa cartella third_party/, non spostare solo l'exe),"
echo "oppure esportare LD_LIBRARY_PATH=$INSTALL_DIR/lib."
