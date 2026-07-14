# AliveMonitor

[Italiano](readme_it.md) · **English** · [Français](readme_fr.md) · [Español](readme_es.md) · [Deutsch](readme_de.md)

Desktop application (C++20, wxWidgets 3.2+, CMake, MVC architecture) for real-time acquisition of the analog voltages A0–A5 and control of the digital outputs D2–D9 of an **Arduino Uno** board connected via USB, with a dedicated firmware sketch included.
NOTICE: FLASH THE FIRMWARE ONTO THE ARDUINO FIRST!! It is located inside the installation folder ..\AliveMonitor\arduino\AliveMonitor
Main features: automatic serial port detection (`HELLO`/`ARDUINO_UNO` handshake) with automatic reconnection; real-time graph organized into 7 tabs — one combined tab with six curves (zoom, pan, autoscale, legend, grid) plus one tab per channel with a dedicated Y axis in the converted physical quantity, see below — with PNG export; per-channel linear calibration (V → physical quantity, see below); continuous CSV recording on Start (producer/consumer, see below); acquisition rate adjustable from 1 to 250 Hz even while running; thread-safe 60-second ring buffer per channel (already sized for 500 Hz); rendering decoupled from acquisition (10–60 configurable FPS); status bar with FPS, received/lost packets, CRC/serial errors, connection time and CPU usage; interface available in 5 languages (Italiano, English, Français, Español, Deutsch), selectable from Settings (requires a restart).
I would also like to take this opportunity to thank Vincenzo Gentile for his suggestions and advice. Feel free to send more.

## Project structure

```
AliveMonitor/
├── CMakeLists.txt          Build system (Windows and Linux)
├── README.md               This file (English, reference version)
├── readme_it.md            Versione italiana
├── readme_fr.md            Version française
├── readme_es.md            Versión en español
├── readme_de.md            Deutsche Version
├── LICENSE                 MIT license
├── include/                Shared headers (Version, Language, events, IUserActions, CpuMonitor)
├── src/                    main.cpp (wxApp), AppEvents, CpuMonitor
├── i18n/                   Strings.h/.cpp: interface translation table
├── model/                  MODEL: BoardState, SerialModel, AnalogDataBuffer,
│                           DigitalOutputState, CommunicationProtocol, RingBuffer,
│                           ChannelCalibration, AppSettings
├── view/                   VIEW: MainFrame, ToolbarPanel, DigitalOutputPanel,
│                           AcquisitionPanel, CalibrationPanel, GraphPanel,
│                           StatusPanel, SettingsDialog, LedIndicator, SplashScreen
├── controller/             CONTROLLER: MainController, SerialController,
│                           GraphController, CommandController, CsvLoggerController
├── serial/                 ISerialPort + Win32 and POSIX implementations
├── resources/              Embedded resources (icons, HTML guide in 5 languages)
├── installer/              Inno Setup script for the Windows setup.exe
├── docs/                   Architecture.md, Protocol.md, Translations.md, UML/
└── arduino/
    └── AliveMonitor/
        └── AliveMonitor.ino  Complete firmware for Arduino Uno
```

## Building — Linux

Requires GCC 12+ (or Clang 15+), CMake ≥ 3.21 and wxWidgets ≥ 3.2.

**Ubuntu 24.04+ / Debian 12+:**

```bash
sudo apt update
sudo apt install build-essential cmake libwxgtk3.2-dev
cd AliveMonitor
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j$(nproc)
./build/AliveMonitor
```

**Distributions without wxWidgets 3.2 in the official repositories** (e.g. Ubuntu 22.04, which only has 3.0; or Slackware, which doesn't offer it at all — only wxGTK3 3.0.5 via SlackBuilds.org): instead of building wxWidgets by hand you can use the automatic script, which doesn't depend on any package manager (it only needs `cmake`, `gcc`, `g++`, `make`, `curl` — on Slackware `cmake` needs to be added, everything else is included in a "full" installation):

```bash
bash scripts/setup-wxwidgets.sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j$(nproc)
```

Downloads, builds (shared build) and installs wxWidgets 3.2.11 into `third_party/wx-install`, automatically detected by `CMakeLists.txt` — no need to pass `-DCMAKE_PREFIX_PATH`. Idempotent (`--force` to redo it), options with `--help`.

**Serial port permissions:** add the user to the `dialout` group (`sudo usermod -aG dialout $USER`, then log back in).

### Distributing the executable to another Linux machine (without installing wxGTK)

On Linux, a truly "zero dependency" build like on Windows isn't realistic
(wxGTK relies on GTK, which loads themes/icons from the system at runtime
regardless of whether it's linked statically). The practical alternative is
a **portable bundle**: a folder with the executable and all its non-system
shared libraries, ready to be copied and run on another machine (same
architecture) without installing anything:

```bash
bash scripts/setup-wxwidgets.sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j$(nproc)
bash scripts/package-linux.sh
```

Creates `dist/AliveMonitor-linux-x64/` with the executable plus a `lib/`
folder containing wxGTK, GTK, glib, pango, cairo, X11, etc. (detected via
`ldd`). Base libraries (glibc, libpthread, ld-linux...) are deliberately
excluded: they are tied to the target machine's kernel/libc, and bundling
them would break portability rather than help it. This works thanks to a
relative RPATH (`$ORIGIN/lib`) already embedded in the executable by
`CMakeLists.txt`: just copy the entire `dist/AliveMonitor-linux-x64/`
folder and run `./AliveMonitor` — no `LD_LIBRARY_PATH` to set (`run.sh` is
included as a safety net). The script itself checks with `ldd` that no
dependencies are missing before declaring itself done.

Limitation: the target machine needs a compatible libc (same distro/family,
or a version equal to or newer than the build machine's) and the same
architecture. Full cross-distro portability (bundling glibc as well) would
require a format like AppImage — not included here, but the
`package-linux.sh` bundle covers most real-world cases (same distro or
family, e.g. Slackware to Slackware, Ubuntu to Debian).

## Building — Windows

### Option 0 (recommended): automatic script, static build with no DLLs

You first need a **MinGW-w64** compiler installed (e.g. via
[MSYS2](https://www.msys2.org/) or [WinLibs](https://winlibs.com/)). Then,
with no intermediate steps:

```bat
cd AliveMonitor
scripts\setup-wxwidgets.bat
cmake -S . -B build -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Release
cmake --build build -j8
build\AliveMonitor.exe
```

The script downloads, builds and installs a static wxWidgets 3.2.11 into
`%LOCALAPPDATA%\AliveMonitor\wxWidgets` — deliberately **outside** the
project: if the repository path contains characters special to regular
expressions (e.g. a folder named `c++`), the `wxWidgetsConfig.cmake` file
generated by wxWidgets isn't found correctly by CMake. `%LOCALAPPDATA%`
avoids the problem regardless of where the project is located, and is
automatically detected by `CMakeLists.txt` anyway. It looks for the MinGW
compiler on its own (in `PATH`, then in the most common installation
folders): if it finds it in exactly one place it proceeds without asking
anything. If it can't find it, or finds more than one installed, it stops
and asks you to specify it explicitly:

```bat
scripts\setup-wxwidgets.bat -MinGwBin C:\path\to\mingw64\bin
```

`setup-wxwidgets.bat` is a wrapper that calls `setup-wxwidgets.ps1`
(forwarding all arguments); if you prefer you can call the PowerShell
script directly: `powershell -ExecutionPolicy Bypass -File scripts\setup-wxwidgets.ps1 [-MinGwBin ...]`.
The script is idempotent (skips rebuilding if already done; `-Force` to
redo it).
The resulting executable doesn't depend on any third-party DLL (verified
with `objdump -p AliveMonitor.exe`), only on Windows system DLLs: it can be
copied and distributed on its own.

**Cleanup:** the compiled wxWidgets (source + build + install, a few
hundred MB) stays in `%LOCALAPPDATA%\AliveMonitor`, shared across any
clones of the project. To free up space or force a full rebuild, delete it
with:

```bat
rmdir /s /q "%LOCALAPPDATA%\AliveMonitor"
```

(this doesn't touch the project or AliveMonitor's `build\` folder: only
wxWidgets will need to be rebuilt, with `scripts\setup-wxwidgets.bat`, if
needed.)

Options A/B/C below remain valid for those who prefer to manage wxWidgets
manually (e.g. shared build/DLLs, or a wxWidgets already installed
elsewhere).

### Option A: wxWidgets built from source with MinGW (CONFIG mode)

If wxWidgets was built with CMake from source, for example:

```bat
cd C:\library\wxWidgets
mkdir build_gcc && cd build_gcc
cmake -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Release -DwxBUILD_SHARED=ON -DwxBUILD_SAMPLES=OFF -DwxBUILD_TESTS=OFF ..
cmake --build . -j8
```

it then **needs to be installed** (in wx 3.2.x the `wxWidgetsConfig.cmake` file only works from the install prefix, not from the build tree):

```bat
cmake --install C:\library\wxWidgets\build_gcc --prefix C:\library\wx-install
```

then build AliveMonitor pointing at the prefix:

```bat
cd AliveMonitor
cmake -S . -B build -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Release -DCMAKE_PREFIX_PATH=C:/library/wx-install
cmake --build build -j8
build\AliveMonitor.exe
```

Notes for this mode:
- with `wxBUILD_SHARED=ON` the wxWidgets DLLs are **automatically copied** next to the executable during the build (disable with `-DAM_COPY_WX_DLLS=OFF`, useful for static builds);
- the MinGW DLLs (`libstdc++-6.dll`, `libgcc_s_seh-1.dll`, `libwinpthread-1.dll`) are also needed at runtime: the project links them **statically by default** (`AM_STATIC_RUNTIME=ON`, see below), so normally there's no need to copy them or touch `PATH`;
- **use the same MinGW compiler** that wxWidgets was built with (same toolchain in `PATH`) — a toolchain mismatch is the most common cause of the *"entry point not found"* error at startup.

### Fully static build (no DLLs to distribute)

To get an `AliveMonitor.exe` that doesn't depend on any third-party DLL (only Windows system DLLs), besides the compiler runtime you also need to build **wxWidgets in static mode**:

```bat
cd C:\library\wxWidgets
mkdir build_gcc_static && cd build_gcc_static
cmake -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Release -DwxBUILD_SHARED=OFF -DwxBUILD_SAMPLES=OFF -DwxBUILD_TESTS=OFF ..
cmake --build . -j8
cmake --install . --prefix C:\library\wx-install-static

cd AliveMonitor
cmake -S . -B build-static -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Release -DCMAKE_PREFIX_PATH=C:/library/wx-install-static
cmake --build build-static -j8
build-static\AliveMonitor.exe
```

- `AM_STATIC_RUNTIME` (default **ON**) adds `-static-libgcc -static-libstdc++ -static`, embedding libstdc++/libgcc/winpthread into the exe. Disable with `-DAM_STATIC_RUNTIME=OFF` if you'd rather distribute those DLLs separately.
- With static wx (`wxBUILD_SHARED=OFF`) the DLL copy step (`AM_COPY_WX_DLLS`) turns off automatically: there's nothing to copy.
- On MSVC the same goal is achieved with `AM_STATIC_RUNTIME=ON` (CRT `/MT`, automatic) plus a wxWidgets/vcpkg built with a static triplet, e.g. `vcpkg install wxwidgets:x64-windows-static`.
- The static executable is larger (the runtime and wx are embedded) but copies and distributes as a single file, with no risk of a DLL version mismatch.

### Option B: MSYS2/MinGW with precompiled packages

1. Install [MSYS2](https://www.msys2.org/), open the *MSYS2 MINGW64* terminal and run:

```bash
pacman -S --needed mingw-w64-x86_64-toolchain mingw-w64-x86_64-cmake mingw-w64-x86_64-wxwidgets3.2-msw
cd /c/path/to/AliveMonitor
cmake -S . -B build -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Release
cmake --build build -j
./build/AliveMonitor.exe
```

### Option C: Visual Studio 2022 + vcpkg

```bat
vcpkg install wxwidgets:x64-windows
cd AliveMonitor
cmake -S . -B build -DCMAKE_TOOLCHAIN_FILE=%VCPKG_ROOT%\scripts\buildsystems\vcpkg.cmake
cmake --build build --config Release
build\Release\AliveMonitor.exe
```

The Arduino Uno's COM port driver is installed automatically by Windows 10/11 (for CH340-based clones, install the manufacturer's driver).

### Building the Windows installer (setup.exe)

To distribute AliveMonitor to an end user without making them install a compiler or wxWidgets, you can generate a self-contained installer with [Inno Setup](https://jrsoftware.org/isinfo.php) (free, only needs to be installed on the machine that *creates* the setup, not the one that *receives* it):

```bat
scripts\setup-wxwidgets.bat
cmake -S . -B build-static -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Release
cmake --build build-static -j8

powershell -ExecutionPolicy Bypass -File scripts\package-windows.ps1
```

The `package-windows.ps1` wrapper verifies that `build-static\AliveMonitor.exe` exists (the **fully static build** is required, with no DLLs to depend on: see above), automatically derives the version from `include/Version.h`, locates `ISCC.exe` (in `PATH` or in Inno Setup's default install folders) and produces `dist\AliveMonitor-Setup-<version>.exe` — icon, Start menu entry, optional desktop shortcut, license shown during installation, uninstaller listed in "Apps & features". Besides the executable, the installer also copies the `AliveMonitor.ino` sketch (`arduino\AliveMonitor\` folder, also reachable from the Start menu): anyone installing from this setup.exe, without having cloned the repository, still needs to be able to flash the firmware onto the board.

The Inno Setup script (`installer\AliveMonitor.iss`) packages the **already-built executable**, not the source (which stays in the GitHub repository): the resulting `dist\AliveMonitor-Setup-*.exe` file must NOT be committed — it's a build artifact (the `dist/` folder is already in `.gitignore`), to be distributed separately, for example as an attachment on a GitHub Release.

## Arduino firmware

1. Open `arduino/AliveMonitor/AliveMonitor.ino` with the Arduino IDE (≥ 1.8) or `arduino-cli`.
2. Select the **Arduino Uno** board and the correct port.
3. Upload the sketch. Done: the PC application will find the board on its own.

With `arduino-cli`:

```bash
arduino-cli compile --fqbn arduino:avr:uno arduino/AliveMonitor/AliveMonitor.ino
arduino-cli upload  --fqbn arduino:avr:uno -p /dev/ttyACM0 arduino/AliveMonitor/AliveMonitor.ino
```

## Usage

On startup the program automatically scans the serial ports and connects to the board that responds `ARDUINO_UNO` (green LED in the top-left corner). *Start* begins streaming at the rate set with the SpinCtrl or the slider (1–250 Hz, adjustable on the fly); *Pause* suspends it while keeping the data; *Stop* stops it and clears the buffer. The D2–D9 buttons control the outputs: the LED next to each button reflects the **actual state** confirmed by the firmware. In the graph: mouse wheel = zoom on time, Ctrl+wheel = zoom on voltage, drag = pan, *Follow* re-attaches the scrolling window, *Reset* restores the default view (last 60 s, 0–5 V). If the board is disconnected the application reports it and automatically resumes searching, restoring the outputs and streaming upon reconnection.

### Continuous CSV recording

Pressing *Start* from a stopped state (not from *Pause*) brings up a window
to choose where to save the file, with a ready-made suggested name
(`acquisizione_YYYY-MM-DD_HH-MM-SS.csv`); cancelling the window also cancels
starting the acquisition. A "Record CSV" checkbox below Start/Pause/Stop
(checked by default) lets you skip the dialog and the recording entirely: if
unchecked, Start begins immediately without asking or writing any file.
While recording is active, every ADC sample received is written to file in
real time, without ever blocking the serial thread: this is implemented
with a **producer/consumer** pattern (`CsvLoggerController`) — the serial
thread enqueues samples into a thread-safe queue (`push()`, non-blocking), a
third dedicated thread pulls them off and writes them to disk. An LED
(green = writing, red = stopped) and a label with the file's full path,
centered in the graph's toolbar, show the recording status. *Pause* doesn't
interrupt the file (it stays the same, with possible "gaps" during pause
periods); *Stop* closes the recording, waiting for the consumer to finish
writing all samples already queued, so no data is lost either on manual stop
or when closing the application.

### Channel calibration (V → physical quantity)

Below the acquisition panel there is a grid (`CalibrationPanel`) with one
row per channel (A0..A5) and four editable columns: coefficient `a`, offset
`b`, unit of measurement and description. For each transducer connected
with a linear law G = a·V + b (e.g. a 0–5 V → 0–100 °C temperature sensor
would have a=20, b=0, unit "°C"), enter the values in the corresponding
channel's row: the effect is immediate on the graph's legend, which shows
the converted value in real time next to the channel name (e.g. "A0: 23.4
°C"). Unconfigured channels stay at the identity (a=1, b=0, unit "V"), i.e.
they simply show the voltage. The description is a free-text label (e.g.
"Oven temperature"): if filled in it becomes the title of that channel's
dedicated tab in the graph (see below), otherwise the tab keeps the
channel's name ("A0".."A5"). Calibration is only valid for the current
session (it is not saved to disk); if a CSV recording is active, the file
includes — alongside the six `A#_V` columns in Volts — six additional
`A#_conv[unit]` columns with the already-converted value, using the
calibration set at the moment Start was pressed (a change made while a
recording is already running doesn't alter the header or the rows already
written in that session).

### 7-tab graph (independent axes per channel)

The graph is a `wxNotebook` with 7 tabs. The first, "All", is the usual
combined graph: the six curves share a single Y axis in Volts, with
checkboxes to hide/show individual channels and the CSV recording status
(LED + file path). The other six tabs (A0..A5) each show a single channel,
with the curve and Y axis in the quantity *already converted* according to
its calibration (or in Volts if uncalibrated) — a simple way to have, in
practice, "multiple axes" when the channels measure quantities of different
nature and scale, without having to overlay incompatible units on the same
axis. The title of each of these six tabs is the description set in the
calibration grid ("description" column), or the channel's name if not
filled in; it updates in real time on every grid edit, with no restart
needed. Every single-channel tab has Auto Y enabled by default (since the
range of the converted quantity isn't known in advance) and its own
Autoscale/Follow/Reset/PNG buttons independent of the other tabs; PNG
export always saves the currently displayed tab. The time window (in
Settings) applies to all tabs together, while the combined tab's continuous
Auto Y in the Settings panel only affects the combined tab — the six
per-channel tabs remain independent.

### Interface language

From the Tools &gt; Settings menu you can choose the interface language
among Italiano, English, Français, Español and Deutsch. The choice is
saved to `%APPDATA%\AliveMonitor\settings.ini` (Windows) or
`~/.config/AliveMonitor/settings.ini` (Linux) and requires restarting the
application to take effect: already-built widgets are not re-translated on
the fly. The complete translation table is in `docs/Translations.md`.

## Documentation

The full description of the architecture, design choices and extension suggestions is in `docs/Architecture.md`; the serial protocol specification in `docs/Protocol.md`; the interface translation table in `docs/Translations.md`; the UML diagrams (class and sequence, PlantUML + Mermaid) in `docs/UML/`; the step-by-step guide for building with the CMake GUI (static wxWidgets included) in `docs/CMakeGUI.md`.

## In-app guide

From the Help &gt; Guide menu (or F1), a quick usage guide opens in the default browser, in the interface's current language, embedded in the executable like the other resources (including the icon): no external file to distribute alongside the binary.

## License

Distributed under the **MIT** license: free to use, modify and redistribute, including commercially, provided "as is" with no warranty and no liability for the authors for any damages arising from the use of the software. Full text in [`LICENSE`](LICENSE).
