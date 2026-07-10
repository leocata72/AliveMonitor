#!/usr/bin/env bash
# Crea una cartella autosufficiente con AliveMonitor già compilato + tutte
# le librerie condivise "non di sistema" da cui dipende (wxGTK, GTK, glib,
# pango, cairo, X11, ...), così si può copiare su un'altra macchina Linux
# (stessa architettura) e lanciarlo senza installare wxGTK né nient'altro.
#
# Le librerie "di base" (glibc, libpthread, ld-linux, ...) NON vengono
# incluse deliberatamente: sono legate a doppio filo al kernel/alla libc
# della macchina di destinazione, bundlarle romperebbe la portabilità
# invece di aiutarla (approccio standard per bundle Linux "leggeri", a
# differenza di un vero AppImage che va oltre includendo anche quelle).
#
# Il bundle si lancia con ./run.sh (imposta LD_LIBRARY_PATH sulla cartella
# lib/ locale): l'eseguibile stesso NON ha un RPATH relativo incorporato
# (l'RPATH di sviluppo di CMake punta alla wx della macchina di build, non a
# lib/), quindi ./AliveMonitor lanciato direttamente dalla cartella
# impacchettata fallisce con "cannot open shared object file" a meno di
# passare da run.sh o da patchelf (usato se disponibile, vedi sotto).
#
# Uso:
#   bash scripts/package-linux.sh [--build-dir build] [--out dist/AliveMonitor-linux-x64]
set -euo pipefail

BUILD_DIR="build"
OUT_DIR="dist/AliveMonitor-linux-x64"

usage() {
    cat <<EOF
Uso: $0 [--build-dir DIR] [--out DIR]

Compila prima il progetto (cmake --build), poi impacchetta l'eseguibile e le
sue librerie condivise non di sistema in una cartella autosufficiente.
EOF
}

while [[ $# -gt 0 ]]; do
    case "$1" in
        --build-dir) BUILD_DIR="$2"; shift 2 ;;
        --out)       OUT_DIR="$2"; shift 2 ;;
        -h|--help)   usage; exit 0 ;;
        *) echo "Opzione sconosciuta: $1" >&2; usage; exit 1 ;;
    esac
done

REPO_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
EXE="$REPO_ROOT/$BUILD_DIR/AliveMonitor"
OUT="$REPO_ROOT/$OUT_DIR"

if [[ ! -x "$EXE" ]]; then
    echo "Errore: non trovo un eseguibile compilato in $EXE" >&2
    echo "Compila prima il progetto:" >&2
    echo "    cmake -S . -B $BUILD_DIR -DCMAKE_BUILD_TYPE=Release" >&2
    echo "    cmake --build $BUILD_DIR -j\$(nproc)" >&2
    exit 1
fi

command -v ldd >/dev/null 2>&1 || { echo "Errore: 'ldd' non trovato nel PATH." >&2; exit 1; }

# Se wxWidgets è stata compilata da scripts/setup-wxwidgets.sh, vive in
# third_party/wx-install/lib: NON è un percorso di sistema, quindi senza
# includerlo nel LD_LIBRARY_PATH qui, ldd non riuscirebbe a risolverla
# (la segnerebbe "not found") e verrebbe scartata invece che bundlata.
WX_LOCAL_LIB="$REPO_ROOT/third_party/wx-install/lib"
if [[ -d "$WX_LOCAL_LIB" ]]; then
    export LD_LIBRARY_PATH="$WX_LOCAL_LIB:${LD_LIBRARY_PATH:-}"
    echo "==> Uso anche $WX_LOCAL_LIB per risolvere le dipendenze (wx locale da setup-wxwidgets.sh)"
fi

echo "==> Preparo $OUT"
rm -rf "$OUT"
mkdir -p "$OUT/lib"
cp "$EXE" "$OUT/AliveMonitor"

# Librerie "di base" da NON bundlare (vedi commento in testa al file).
BASE_LIBS_REGEX='^(linux-vdso\.so|ld-linux|libc\.so|libm\.so|libdl\.so|libpthread\.so|librt\.so|libresolv\.so|libnsl\.so|libutil\.so)'

echo "==> Analizzo le dipendenze con ldd"
LDD_OUTPUT="$(ldd "$EXE")"

NOT_FOUND="$(echo "$LDD_OUTPUT" | grep -i "not found" || true)"
if [[ -n "$NOT_FOUND" ]]; then
    echo "Errore: ldd non riesce a risolvere queste librerie:" >&2
    echo "$NOT_FOUND" >&2
    echo "(se è wxGTK: hai eseguito 'bash scripts/setup-wxwidgets.sh' e ricompilato dopo? altrimenti installa la dipendenza di sistema mancante)" >&2
    exit 1
fi

mapfile -t LIB_PATHS < <(
    echo "$LDD_OUTPUT" \
        | awk '{ if ($3 ~ /^\//) print $3; else if ($1 ~ /^\//) print $1 }' \
        | sort -u
)

COUNT=0
for lib in "${LIB_PATHS[@]}"; do
    [[ -f "$lib" ]] || continue
    name="$(basename "$lib")"
    if [[ "$name" =~ $BASE_LIBS_REGEX ]]; then
        continue
    fi
    cp -L "$lib" "$OUT/lib/$name"
    COUNT=$((COUNT + 1))
done
echo "    $COUNT librerie copiate in $OUT/lib"

# Lancio: run.sh imposta LD_LIBRARY_PATH sulla cartella lib/ locale, sempre
# affidabile. In più, se patchelf è disponibile, incorporiamo anche un RPATH
# relativo nell'eseguibile bundlato così funziona pure "./AliveMonitor"
# lanciato direttamente (comodo, ma non indispensabile: run.sh basta sempre).
cat > "$OUT/run.sh" <<'EOF'
#!/usr/bin/env bash
DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
export LD_LIBRARY_PATH="$DIR/lib:${LD_LIBRARY_PATH:-}"
exec "$DIR/AliveMonitor" "$@"
EOF
chmod +x "$OUT/run.sh" "$OUT/AliveMonitor"

DIRECT_RUN_OK=0
if command -v patchelf >/dev/null 2>&1; then
    patchelf --set-rpath '$ORIGIN/lib' "$OUT/AliveMonitor" && DIRECT_RUN_OK=1
    echo "==> patchelf trovato: RPATH relativo incorporato, ./AliveMonitor funziona anche diretto"
else
    echo "==> patchelf non trovato: usa ./run.sh per lanciare il bundle (./AliveMonitor diretto non troverebbe le librerie)"
fi

echo
echo "==> Verifico che non manchino dipendenze nel bundle"
MISSING="$(cd "$OUT" && LD_LIBRARY_PATH="lib" ldd AliveMonitor | grep -i "not found" || true)"
if [[ -n "$MISSING" ]]; then
    echo "ATTENZIONE: dipendenze non risolte nel bundle:" >&2
    echo "$MISSING" >&2
    exit 1
fi
echo "    OK: tutte le dipendenze sono risolte dentro la cartella."

echo
echo "Pronto: $OUT"
echo "Copia l'intera cartella su un'altra macchina Linux (stessa architettura, es. x86_64) ed esegui:"
if [[ "$DIRECT_RUN_OK" -eq 1 ]]; then
    echo "    ./AliveMonitor    (oppure ./run.sh, equivalente)"
else
    echo "    ./run.sh"
fi
