# Compilare con CMake GUI (Windows)

Guida passo-passo per chi preferisce **CMake GUI** invece della riga di
comando. Copre sia la compilazione di wxWidgets (statica, una tantum) sia la
configurazione di AliveMonitor. In alternativa a questa guida si può usare
`scripts\setup-wxwidgets.ps1` (vedi README, "Opzione 0") che automatizza la
Parte A.

## Prerequisiti

- **CMake** (cmake-gui incluso nell'installer standard).
- Una toolchain **MinGW-w64** (es. `C:\devTools\mingw64\bin`), con `gcc.exe`,
  `g++.exe`, `mingw32-make.exe`.
- Sorgente di **wxWidgets 3.2.11** scaricato ed estratto (zip "Windows
  source" da https://www.wxwidgets.org/downloads/), ad es. in
  `C:\devTools\wxWidgets-3.2.11`.

Non serve impostare la variabile d'ambiente `WXWIN`: serve solo ai vecchi
build system `nmake`/`makefile.vc`, non a CMake.

## Parte A — Compilare wxWidgets (build statica)

1. Apri **CMake GUI**.
2. *"Where is the source code"*: `C:/devTools/wxWidgets-3.2.11`
3. *"Where to build the binaries"*: `C:/devTools/wxWidgets-3.2.11/build_gcc_static`
   (cartella nuova, dedicata — non riusare build precedenti).
4. **Configure** → generator **"MinGW Makefiles"** → *"Specify native
   compilers"* → punta a `gcc.exe`/`g++.exe` della tua toolchain MinGW →
   **Finish**. La prima scansione produce righe rosse: è normale.
5. Nel campo di ricerca imposta:
   - `wxBUILD_SHARED` → **deseleziona** (OFF) — build statica
   - `wxBUILD_SAMPLES` → OFF
   - `wxBUILD_TESTS` → OFF
   - `wxBUILD_DEMOS` → OFF (se presente)
   - `CMAKE_BUILD_TYPE` → `Release`
6. **Configure** di nuovo (ripeti finché non restano righe rosse), poi
   **Generate**.
7. Compila da terminale (CMake GUI non compila da sé):
   ```bat
   cd C:\devTools\wxWidgets-3.2.11\build_gcc_static
   C:\devTools\mingw64\bin\mingw32-make.exe -j8
   ```
8. Installa in un prefisso pulito e stabile:
   ```bat
   cmake --install . --prefix C:/devTools/wx-install-static
   ```
   `cmake --install` crea la cartella se non esiste e vi copia header (`include/`)
   e librerie statiche (`lib/`, incluso `lib/cmake/wxWidgets-3.2/wxWidgetsConfig.cmake`,
   il file che permette ad AliveMonitor di trovare wx in **CONFIG mode**).

## Parte B — Configurare AliveMonitor

1. Apri **CMake GUI**.
2. *"Where is the source code"*: `C:/Users/leona/Documents/c++/AliveMonitor`
3. *"Where to build the binaries"*: `.../AliveMonitor/build`. Se esiste già
   da una configurazione precedente, **File → Delete Cache** prima di
   procedere (una cache vecchia con percorsi wx diversi causa errori
   fuorvianti).
4. **Configure** → generator **"MinGW Makefiles"** → stessa toolchain MinGW
   usata per wx → **Finish**.
5. Se il progetto non trova wx da solo (non hai usato `third_party/`, vedi
   README), aggiungi manualmente la variabile: **Add Entry** → nome
   `CMAKE_PREFIX_PATH`, tipo `PATH`, valore `C:/devTools/wx-install-static`.
6. **Configure**. Nell'output in basso cerca:
   ```
   wxWidgets 3.2.11 trovata (CONFIG mode): C:/devTools/wx-install-static/lib/cmake/wxWidgets-3.2
   AM_STATIC_RUNTIME: runtime MinGW (libstdc++/libgcc/winpthread) linkato staticamente
   wxWidgets è statica: eseguibile standalone, nessuna DLL di terze parti da copiare
   ```
   Se compare **CONFIG mode**, tutto è a posto: la build sarà completamente
   statica (nessuna DLL da distribuire) e il link userà automaticamente
   `--start-group`/`--end-group` (necessario con wx statica su MinGW/GNU ld).
7. **Generate**, poi compila:
   ```bat
   cd C:\Users\leona\Documents\c++\AliveMonitor\build
   C:\devTools\mingw64\bin\mingw32-make.exe -j8
   ```

## Verifica finale (consigliata)

Controlla che l'eseguibile non dipenda da DLL esterne al sistema Windows:

```bat
C:\devTools\mingw64\bin\objdump.exe -p build\AliveMonitor.exe | findstr "DLL Name"
```

Devono comparire solo DLL di sistema (`KERNEL32.dll`, `USER32.dll`,
`GDI32.dll`, `COMCTL32.dll`, `ADVAPI32.dll`, `SHELL32.dll`, `ole32.dll`,
`msvcrt.dll`, ecc.). **Nessuna** `libstdc++-6.dll`, `libgcc_s_seh-1.dll`,
`libwinpthread-1.dll`, `wxbase*.dll` o `wxmsw*.dll` deve apparire: se è così,
l'exe è distribuibile da solo.

## Problemi comuni

- **`wxWidgets_DIR-NOTFOUND`** nella cache: CMake non ha trovato la CONFIG di
  wx ed è caduto in *MODULE mode* (vecchio `FindwxWidgets.cmake`). Di solito
  significa che `CMAKE_PREFIX_PATH`/l'`-DwxWidgets_ROOT_DIR` punta all'albero
  sorgente/build di wx invece che al prefisso creato con `cmake --install`
  (Parte A, step 8). Correggi il percorso e riconfigura da cache pulita.
- **`undefined reference` a metodi `wxWindow::MSW...`** in fase di link: con
  wx statica su MinGW le librerie hanno dipendenze incrociate; il
  `CMakeLists.txt` del progetto già le raggruppa con `--start-group`/
  `--end-group` quando rileva CONFIG mode + wx statica, quindi non dovrebbe
  più capitare. Se ricompare, verifica di essere ripartiti da una cache
  pulita (`File → Delete Cache`) dopo aver cambiato `CMAKE_PREFIX_PATH`.
- **Errore "impossibile trovare il punto di ingresso" all'avvio**: mismatch
  fra la toolchain MinGW che ha compilato wx e quella usata per
  AliveMonitor (o `libstdc++-6.dll` sbagliata nel `PATH`). Con
  `AM_STATIC_RUNTIME=ON` (default) il runtime è incorporato nell'exe e il
  problema non si presenta più; verificalo con `objdump` come sopra.
