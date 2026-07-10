<#
.SYNOPSIS
    Genera il setup.exe Windows di AliveMonitor con Inno Setup.

.DESCRIPTION
    Impacchetta l'ESEGUIBILE GIA' COMPILATO (non i sorgenti, che restano su
    GitHub) in un installer autonomo:
      1. verifica che esista una build già compilata in -BuildDir
      2. ricava la versione da include/Version.h (unico punto di verità,
         nessun numero duplicato a mano nello script Inno Setup)
      3. individua ISCC.exe (Inno Setup Compiler), nel PATH o nelle
         posizioni di installazione più comuni
      4. lo invoca su installer\AliveMonitor.iss, che produce
         dist\AliveMonitor-Setup-<versione>.exe

    Presuppone una build COMPLETAMENTE STATICA (vedi README.md, sezione
    "Build completamente statica"): un solo AliveMonitor.exe senza DLL di
    terze parti da cui dipendere. Una build condivisa (con DLL accanto)
    funzionerebbe comunque a runtime, ma l'installer così com'è copia solo
    l'eseguibile: andrebbe esteso ad hoc per includere anche le DLL.

.PARAMETER BuildDir
    Cartella della build già compilata, relativa alla radice del repository
    (default: build-static, coerente con le istruzioni del README per la
    build completamente statica).

.PARAMETER IsccPath
    Percorso di ISCC.exe. Se omesso, lo script lo cerca nel PATH e nelle
    cartelle di installazione predefinite di Inno Setup.

.EXAMPLE
    .\scripts\package-windows.ps1

.EXAMPLE
    .\scripts\package-windows.ps1 -BuildDir build -IsccPath "C:\Tools\InnoSetup\ISCC.exe"
#>

[CmdletBinding()]
param(
    [string]$BuildDir = "build-static",
    [string]$IsccPath = ""
)

$ErrorActionPreference = "Stop"

function Write-Step($msg) {
    Write-Host ""
    Write-Host "==> $msg" -ForegroundColor Cyan
}

function Invoke-Checked($exe, $cmdArgs) {
    # NB: il parametro non si chiama $args di proposito, vedi il commento
    # identico in scripts/setup-wxwidgets.ps1 — è una variabile automatica
    # riservata di PowerShell e un parametro con quel nome non riceverebbe
    # il valore passato dal chiamante.
    Write-Host "    $exe $($cmdArgs -join ' ')" -ForegroundColor DarkGray
    & $exe @cmdArgs
    if ($LASTEXITCODE -ne 0) {
        throw "Comando fallito (exit $LASTEXITCODE): $exe $($cmdArgs -join ' ')"
    }
}

$RepoRoot = (Resolve-Path (Join-Path $PSScriptRoot "..")).Path

# --- 1) Eseguibile già compilato --------------------------------------------
$ExePath = Join-Path $RepoRoot "$BuildDir\AliveMonitor.exe"
if (-not (Test-Path $ExePath)) {
    throw "Non trovo un eseguibile compilato in $ExePath.`n" +
          "Compila prima la build statica (vedi README.md):`n" +
          "    cmake -S . -B $BuildDir -G `"MinGW Makefiles`" -DCMAKE_BUILD_TYPE=Release -DCMAKE_PREFIX_PATH=C:/library/wx-install-static`n" +
          "    cmake --build $BuildDir -j8"
}
Write-Host "Eseguibile: $ExePath"

# --- 2) Versione da include/Version.h ---------------------------------------
$VersionHeader = Join-Path $RepoRoot "include\Version.h"
$VersionMatch = Select-String -Path $VersionHeader -Pattern 'kAppVersion\s*=\s*"([^"]+)"' | Select-Object -First 1
if ($null -eq $VersionMatch) {
    throw "Non riesco a leggere kAppVersion da $VersionHeader."
}
$Version = $VersionMatch.Matches[0].Groups[1].Value
Write-Host "Versione: $Version"

# --- 3) Individua ISCC.exe ---------------------------------------------------
if ($IsccPath -eq "") {
    $cmd = Get-Command "ISCC.exe" -ErrorAction SilentlyContinue
    if ($cmd) {
        $IsccPath = $cmd.Source
    } else {
        # NB: "${env:ProgramFiles(x86)}" con le graffe, non "$env:ProgramFiles(x86)":
        # l'interpolazione $env:NOME si ferma al primo carattere non valido in un
        # nome di variabile, quindi senza graffe "(x86)" verrebbe letto come testo
        # letterale invece che come parte del nome della variabile d'ambiente.
        #
        # L'installazione "solo per l'utente corrente" di Inno Setup (senza
        # diritti di amministratore) non va in Program Files ma in
        # %LOCALAPPDATA%\Programs, come molti altri installer moderni:
        # va cercata anche lì, non solo nelle due posizioni "per tutti gli utenti".
        $candidates = @(
            "${env:ProgramFiles(x86)}\Inno Setup 6\ISCC.exe",
            "$env:ProgramFiles\Inno Setup 6\ISCC.exe",
            "$env:LOCALAPPDATA\Programs\Inno Setup 6\ISCC.exe"
        ) | Where-Object { Test-Path $_ }
        if ($candidates.Count -gt 0) {
            $IsccPath = $candidates[0]
        } else {
            throw "ISCC.exe (Inno Setup Compiler) non trovato nel PATH né nelle posizioni predefinite.`n" +
                  "Installa Inno Setup (gratuito): https://jrsoftware.org/isinfo.php`n" +
                  "oppure indica il percorso con -IsccPath C:\percorso\ISCC.exe"
        }
    }
}

# Validazione esplicita: se -IsccPath è stato passato a mano (o un candidato
# rilevato automaticamente si rivela comunque sbagliato), un percorso
# inesistente non deve arrivare silenziosamente a "& $IsccPath ..." — lì
# fallirebbe con un CommandNotFoundException criptico e poco riconducibile
# alla causa reale (è già successo: un percorso con spazi non racchiuso fra
# virgolette, es. "...Inno Setup 6\ISCC.exe" passato SENZA virgolette dalla
# riga di comando, viene troncato dal parsing della shell prima ancora di
# arrivare qui).
if (-not (Test-Path $IsccPath)) {
    throw "Il percorso di ISCC.exe non esiste: '$IsccPath'.`n" +
          "Se l'hai passato con -IsccPath e il percorso contiene spazi (es. `"Inno Setup 6`"), racchiudilo fra virgolette:`n" +
          "    -IsccPath `"C:\percorso con spazi\ISCC.exe`""
}
Write-Host "Inno Setup Compiler: $IsccPath"

# --- 4) Compila l'installer ---------------------------------------------------
Write-Step "Genero il setup.exe (versione $Version)"
$IssPath = Join-Path $RepoRoot "installer\AliveMonitor.iss"
Invoke-Checked $IsccPath @(
    "/DMyAppVersion=$Version",
    "/DSourceExe=$ExePath",
    $IssPath
)

$OutputExe = Join-Path $RepoRoot "dist\AliveMonitor-Setup-$Version.exe"
if (-not (Test-Path $OutputExe)) {
    throw "ISCC ha terminato senza errori ma non trovo l'output atteso: $OutputExe"
}

Write-Host ""
Write-Host "Pronto: $OutputExe" -ForegroundColor Green
