<#
.SYNOPSIS
    Scarica, compila e installa wxWidgets (build statica) per AliveMonitor.

.DESCRIPTION
    Automatizza i passi manuali necessari a preparare una wxWidgets statica
    compatibile con la toolchain MinGW usata per compilare AliveMonitor:
      1. scarica il sorgente di wxWidgets (release ufficiale GitHub)
      2. lo estrae in una cache utente FUORI dal repository
      3. lo configura con CMake (wxBUILD_SHARED=OFF, niente esempi/test)
      4. lo compila
      5. lo installa in quella stessa cache (wx-install-static-<versione>)

    Perché fuori dal repository e non in third_party/: il file di export
    CMake generato da wxWidgets (wxWidgetsConfig.cmake) calcola il proprio
    prefisso con un'espressione regolare sul percorso di installazione. Se
    quel percorso contiene caratteri speciali per le regex (es. "+" — capita
    con cartelle come "c++"), la ricerca in CONFIG mode fallisce in modo
    silenzioso e CMake ripiega sul vecchio MODULE mode, che non beneficia
    delle protezioni per il link statico già previste nel CMakeLists.txt
    (--start-group/--end-group), riproducendo errori "undefined reference".
    Installare in %LOCALAPPDATA%, un percorso utente stabile e "pulito",
    evita il problema a prescindere da dove si trovi il repository, e ha il
    vantaggio di essere condiviso fra eventuali cloni multipli del progetto.

    Il CMakeLists.txt di AliveMonitor rileva automaticamente questa cache,
    quindi dopo aver eseguito questo script basta lanciare la normale build
    del progetto senza passare -DCMAKE_PREFIX_PATH a mano.

.PARAMETER WxVersion
    Versione di wxWidgets da compilare (default: 3.2.11, la stable attuale).

.PARAMETER MinGwBin
    Cartella bin del MinGW da usare (es. C:\devTools\mingw64\bin). Se omessa,
    lo script usa gcc/g++/cmake già presenti nel PATH.

.PARAMETER Jobs
    Numero di job paralleli per la compilazione (default: numero di core logici).

.PARAMETER InstallRoot
    Cartella radice dove scaricare/compilare/installare wxWidgets (default:
    %LOCALAPPDATA%\AliveMonitor\wxWidgets). Evita percorsi con caratteri
    speciali per le regex (+ ( ) [ ] $ ^ . * ?).

.PARAMETER Force
    Ricompila da zero anche se una installazione è già presente.

.EXAMPLE
    .\scripts\setup-wxwidgets.ps1

.EXAMPLE
    .\scripts\setup-wxwidgets.ps1 -MinGwBin C:\devTools\mingw64\bin -Force
#>

[CmdletBinding()]
param(
    [string]$WxVersion = "3.2.11",
    [string]$MinGwBin = "",
    [int]$Jobs = [Environment]::ProcessorCount,
    [string]$InstallRoot = (Join-Path $env:LOCALAPPDATA "AliveMonitor\wxWidgets"),
    [switch]$Force
)

$ErrorActionPreference = "Stop"

function Write-Step($msg) {
    Write-Host ""
    Write-Host "==> $msg" -ForegroundColor Cyan
}

function Invoke-Checked($exe, $cmdArgs) {
    # NB: il parametro NON deve chiamarsi $args - è una variabile automatica
    # riservata di PowerShell (rappresenta gli argomenti non associati della
    # funzione/script corrente): un parametro con quel nome non riceve il
    # valore passato dal chiamante, "& $exe @args" finiva per invocare cmake
    # SENZA argomenti (che stampa l'usage ed esce comunque con codice 0,
    # mascherando il problema).
    Write-Host "    $exe $($cmdArgs -join ' ')" -ForegroundColor DarkGray
    & $exe @cmdArgs
    if ($LASTEXITCODE -ne 0) {
        throw "Comando fallito (exit $LASTEXITCODE): $exe $($cmdArgs -join ' ')"
    }
}

# --- Percorsi ------------------------------------------------------------
# Deliberatamente FUORI dal repository (vedi .DESCRIPTION): un percorso
# stabile in %LOCALAPPDATA% evita che caratteri come "+" nel path del
# progetto rompano la ricerca CONFIG mode di wxWidgetsConfig.cmake.
if ($InstallRoot -match '[+()\[\]$^*?]') {
    Write-Host "Attenzione: -InstallRoot '$InstallRoot' contiene caratteri speciali per le regex; potrebbe far fallire il rilevamento CONFIG mode di wxWidgets." -ForegroundColor Yellow
}
$SourceDir    = Join-Path $InstallRoot "wxWidgets-$WxVersion"
$BuildDir     = Join-Path $SourceDir   "build_gcc_static"
$InstallDir   = Join-Path $InstallRoot "wx-install-static-$WxVersion"
$DownloadsDir = Join-Path $InstallRoot "downloads"
$ZipPath      = Join-Path $DownloadsDir "wxWidgets-$WxVersion.zip"
$MarkerFile   = Join-Path $InstallDir  ".setup-complete"

# --- Toolchain ---------------------------------------------------------------
# Un utente comune lancia lo script così com'è, senza diagnosticare prima il
# PATH a mano: se -MinGwBin non è indicato esplicitamente, proviamo prima il
# PATH e poi le cartelle di installazione MinGW-w64 più comuni. Se troviamo
# più di un candidato NON scegliamo a caso (è proprio un mismatch fra
# toolchain diverse la causa degli errori più subdoli con wx+MinGW): in quel
# caso chiediamo di scegliere esplicitamente con -MinGwBin.
if ($MinGwBin -ne "") {
    if (-not (Test-Path $MinGwBin)) {
        throw "MinGwBin non trovata: $MinGwBin"
    }
    $env:Path = "$MinGwBin;$env:Path"
    Write-Host "Uso toolchain MinGW indicata: $MinGwBin"
}
elseif (-not (Get-Command g++ -ErrorAction SilentlyContinue)) {
    $candidates = @(
        "C:\msys64\mingw64\bin",
        "C:\mingw64\bin",
        "C:\devTools\mingw64\bin",
        "C:\TDM-GCC-64\bin",
        "$env:LOCALAPPDATA\Programs\mingw64\bin"
    ) | Where-Object { Test-Path (Join-Path $_ "g++.exe") }

    if ($candidates.Count -eq 1) {
        $MinGwBin = $candidates[0]
        $env:Path = "$MinGwBin;$env:Path"
        Write-Host "g++ non era nel PATH: trovata automaticamente una toolchain MinGW in $MinGwBin" -ForegroundColor Yellow
        Write-Host "(per usarne un'altra: -MinGwBin C:\percorso\mingw64\bin)"
    }
    elseif ($candidates.Count -gt 1) {
        throw "Trovate più installazioni MinGW: $($candidates -join ', '). " +
              "Specifica quale usare con -MinGwBin C:\percorso\mingw64\bin (deve essere la STESSA che userai per compilare AliveMonitor)."
    }
    else {
        throw "Nessuna toolchain MinGW-w64 trovata nel PATH né nelle posizioni comuni. Installa MinGW-w64 (es. https://www.msys2.org/) e rilancia con -MinGwBin C:\percorso\mingw64\bin."
    }
}

foreach ($tool in @("cmake", "g++", "gcc")) {
    if (-not (Get-Command $tool -ErrorAction SilentlyContinue)) {
        throw "'$tool' non trovato nel PATH. Passa -MinGWBin C:\percorso\mingw64\bin oppure aggiungilo al PATH."
    }
}

# --- Skip se già installata -------------------------------------------------
if ((Test-Path $MarkerFile) -and -not $Force) {
    Write-Host "wxWidgets $WxVersion già installata in $InstallDir (usa -Force per ricompilare)." -ForegroundColor Green
    exit 0
}

if ($Force) {
    Write-Step "Force: rimuovo build/install precedenti"
    Remove-Item -Recurse -Force $BuildDir   -ErrorAction SilentlyContinue
    Remove-Item -Recurse -Force $InstallDir -ErrorAction SilentlyContinue
}

New-Item -ItemType Directory -Force -Path $InstallRoot, $DownloadsDir | Out-Null

# --- Download ----------------------------------------------------------------
if (-not (Test-Path $ZipPath)) {
    $url = "https://github.com/wxWidgets/wxWidgets/releases/download/v$WxVersion/wxWidgets-$WxVersion.zip"
    Write-Step "Scarico wxWidgets $WxVersion da $url"
    Invoke-WebRequest -Uri $url -OutFile $ZipPath -UseBasicParsing
} else {
    Write-Host "Zip già scaricato: $ZipPath"
}

# --- Estrazione ----------------------------------------------------------------
if (-not (Test-Path $SourceDir)) {
    Write-Step "Estraggo in $SourceDir"
    New-Item -ItemType Directory -Force -Path $SourceDir | Out-Null
    Expand-Archive -Path $ZipPath -DestinationPath $SourceDir -Force
} else {
    Write-Host "Sorgente già estratto: $SourceDir"
}

# --- Configure -----------------------------------------------------------------
Write-Step "Configuro wxWidgets (statica, Release)"
Invoke-Checked "cmake" @(
    "-S", $SourceDir,
    "-B", $BuildDir,
    "-G", "MinGW Makefiles",
    "-DCMAKE_BUILD_TYPE=Release",
    "-DwxBUILD_SHARED=OFF",
    "-DwxBUILD_SAMPLES=OFF",
    "-DwxBUILD_TESTS=OFF",
    "-DwxBUILD_DEMOS=OFF"
)

# --- Build -----------------------------------------------------------------
Write-Step "Compilo wxWidgets ($Jobs job paralleli, puo' richiedere diversi minuti)"
Invoke-Checked "cmake" @("--build", $BuildDir, "-j", "$Jobs")

# --- Install -----------------------------------------------------------------
Write-Step "Installo in $InstallDir"
Invoke-Checked "cmake" @("--install", $BuildDir, "--prefix", $InstallDir)

New-Item -ItemType File -Force -Path $MarkerFile | Out-Null

Write-Host ""
Write-Host "wxWidgets $WxVersion pronta in: $InstallDir" -ForegroundColor Green
if ($InstallRoot -eq (Join-Path $env:LOCALAPPDATA "AliveMonitor\wxWidgets")) {
    Write-Host "Ora puoi compilare AliveMonitor normalmente (CMakeLists.txt rileva questo percorso in automatico):"
    Write-Host "    cmake -S . -B build -G `"MinGW Makefiles`" -DCMAKE_BUILD_TYPE=Release"
} else {
    Write-Host "-InstallRoot personalizzato: il CMakeLists.txt NON lo rileva da solo, indicalo esplicitamente:"
    Write-Host "    cmake -S . -B build -G `"MinGW Makefiles`" -DCMAKE_BUILD_TYPE=Release -DCMAKE_PREFIX_PATH=`"$InstallDir`""
}
Write-Host "    cmake --build build -j$Jobs"
