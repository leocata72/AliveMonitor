# =============================================================================
#  AliveMonitor - Applicazione di acquisizione dati e controllo Arduino Uno
#  Build system CMake per Windows (MSVC / MinGW) e Linux (GCC / Clang)
# =============================================================================
cmake_minimum_required(VERSION 3.21)

# Policy per selezionare il runtime MSVC (CRT) tramite la proprietà
# MSVC_RUNTIME_LIBRARY: va impostata PRIMA di project().
cmake_policy(SET CMP0091 NEW)

project(AliveMonitor
    VERSION 1.1.0
    DESCRIPTION "Acquisizione dati analogici e controllo uscite digitali di Arduino Uno via porta seriale"
    LANGUAGES CXX)

# --- Standard C++20 obbligatorio ---------------------------------------------
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# --- Tipo di build predefinito ------------------------------------------------
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE "Release" CACHE STRING "Tipo di build (Debug/Release)" FORCE)
endif()

# --- Dipendenze ----------------------------------------------------------------
# wxWidgets >= 3.2 : core (GUI) + base (runtime).
# NB: in wxWidgets 3.2 la vecchia libreria "adv" è confluita in "core":
#     NON va più richiesta come componente.
#
# Vengono supportate ENTRAMBE le modalità di ricerca:
#
#  1) CONFIG mode  -> wxWidgets compilata con CMake dai sorgenti e INSTALLATA:
#         cmake --install <build-di-wx> --prefix C:/library/wx-install
#         cmake -S . -B build -G "MinGW Makefiles" ^
#               -DCMAKE_PREFIX_PATH=C:/library/wx-install
#     (in wx 3.2.x il file wxWidgetsConfig.cmake funziona solo dal
#      prefisso di installazione, non dall'albero di build)
#
#  2) MODULE mode  -> pacchetti di sistema: MSYS2, vcpkg, apt su Linux.
#
find_package(Threads REQUIRED)

# Se è stato eseguito scripts/setup-wxwidgets.{ps1,bat,sh}, wxWidgets è già
# compilata e installata: la aggiungiamo al percorso di ricerca così non
# serve passare -DCMAKE_PREFIX_PATH a mano.
# - Windows (setup-wxwidgets.ps1/.bat): installa in
#   %LOCALAPPDATA%/AliveMonitor/wxWidgets/wx-install-static-<versione>,
#   DELIBERATAMENTE fuori dal repository. Il file di export CMake generato
#   da wxWidgets (wxWidgetsConfig.cmake) calcola il proprio prefisso con una
#   regex sul percorso: se quel percorso contiene caratteri speciali per le
#   regex (es. "+", capita con cartelle come ".../c++/...") la ricerca in
#   CONFIG mode fallisce silenziosamente e si ripiega sul vecchio MODULE
#   mode, che non beneficia del fix --start-group/--end-group più sotto.
#   %LOCALAPPDATA% è un percorso utente stabile e "pulito" per costruzione.
# - Linux/macOS (setup-wxwidgets.sh): installa in third_party/wx-install
#   (build condivisa), alternativa al pacchetto di sistema
#   libwxgtk3.2-dev del README quando non disponibile (es. Ubuntu 22.04).
# Un eventuale -DCMAKE_PREFIX_PATH passato esplicitamente dall'utente resta
# comunque valido (list(APPEND) lo affianca, non lo sostituisce).
set(_am_wx_local_candidates "${CMAKE_CURRENT_SOURCE_DIR}/third_party/wx-install")
if(DEFINED ENV{LOCALAPPDATA})
    file(GLOB _am_wx_appdata_candidates
        "$ENV{LOCALAPPDATA}/AliveMonitor/wxWidgets/wx-install-static-*")
    list(APPEND _am_wx_local_candidates ${_am_wx_appdata_candidates})
endif()
foreach(_am_wx_candidate ${_am_wx_local_candidates})
    if(EXISTS "${_am_wx_candidate}")
        list(APPEND CMAKE_PREFIX_PATH "${_am_wx_candidate}")
        message(STATUS "Trovata wxWidgets locale da setup-wxwidgets: ${_am_wx_candidate}")
    endif()
endforeach()

set(AM_WX_CONFIG_MODE FALSE)
find_package(wxWidgets 3.2 QUIET CONFIG COMPONENTS core base)
if(wxWidgets_FOUND)
    set(AM_WX_CONFIG_MODE TRUE)
    set(AM_WX_LIBRARIES wx::core wx::base)
    message(STATUS "wxWidgets ${wxWidgets_VERSION} trovata (CONFIG mode): ${wxWidgets_DIR}")
else()
    find_package(wxWidgets 3.2 REQUIRED COMPONENTS core base)
    include(${wxWidgets_USE_FILE})
    set(AM_WX_LIBRARIES ${wxWidgets_LIBRARIES})
    message(STATUS "wxWidgets trovata (MODULE mode)")
endif()

# wx statica (wxBUILD_SHARED=OFF) o condivisa? Serve per decidere sotto se
# raggruppare le librerie nel link (le .a statiche di wx hanno dipendenze
# incrociate fra loro: core <-> base <-> png/zlib/regex) e se copiare le DLL.
set(AM_WX_IS_SHARED FALSE)
if(AM_WX_CONFIG_MODE)
    get_target_property(_am_wx_core_type wx::core TYPE)
    if(_am_wx_core_type STREQUAL "SHARED_LIBRARY")
        set(AM_WX_IS_SHARED TRUE)
    endif()
endif()

# --- Sorgenti comuni a tutte le piattaforme -------------------------------------
set(AM_COMMON_SOURCES
    src/main.cpp
    src/AppEvents.cpp
    src/CpuMonitor.cpp

    model/AnalogDataBuffer.cpp
    model/AppSettings.cpp
    model/BoardState.cpp
    model/CommunicationProtocol.cpp
    model/DigitalOutputState.cpp
    model/SerialModel.cpp

    i18n/Strings.cpp

    controller/CommandController.cpp
    controller/CsvLoggerController.cpp
    controller/GraphController.cpp
    controller/MainController.cpp
    controller/SerialController.cpp

    view/AcquisitionPanel.cpp
    view/CalibrationPanel.cpp
    view/DigitalOutputPanel.cpp
    view/GraphPanel.cpp
    view/LanguageSelectDialog.cpp
    view/LedIndicator.cpp
    view/MainFrame.cpp
    view/SettingsDialog.cpp
    view/SplashScreen.cpp
    view/StatusPanel.cpp
    view/ToolbarPanel.cpp
)

# --- Sorgenti specifiche per piattaforma (implementazione porta seriale) --------
if(WIN32)
    # AliveMonitor.rc associa resources/appicon.ico all'eseguibile
    # (icona del file .exe, distinta da SetIcon() nel MainFrame).
    set(AM_PLATFORM_SOURCES serial/SerialPortWindows.cpp resources/AliveMonitor.rc)
else()
    set(AM_PLATFORM_SOURCES serial/SerialPortPosix.cpp)
endif()

# WIN32 come secondo argomento: su Windows crea una GUI app (niente console);
# su Linux il flag viene ignorato.
add_executable(AliveMonitor WIN32
    ${AM_COMMON_SOURCES}
    ${AM_PLATFORM_SOURCES})

target_include_directories(AliveMonitor PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/include)

# NB: niente RPATH custom qui. L'RPATH automatico che CMake imposta di
# default nell'albero di build (puntando alle librerie di wx effettivamente
# usate in fase di link) è quello che fa funzionare "./build/AliveMonitor"
# subito dopo la compilazione, in sviluppo: sovrascriverlo con un RPATH
# relativo tipo "$ORIGIN/lib" romperebbe proprio questo caso d'uso (è successo
# durante lo sviluppo di questo progetto). Il bundle portabile per la
# distribuzione (scripts/package-linux.sh) usa invece un wrapper run.sh con
# LD_LIBRARY_PATH, indipendente dall'RPATH incorporato nell'eseguibile.

# Con wx STATICA le sue librerie hanno dipendenze incrociate fra loro
# (core <-> base <-> png/zlib/regex/...): con GNU ld (MinGW) senza gruppo il
# link è un passaggio unico e può fallire con "undefined reference" a metodi
# interni di wx (es. wxWindow::MSWDestroyWindow) se una .a che li definisce
# viene scansionata prima che serva. --start-group/--end-group fa scansionare
# ripetutamente il gruppo finché non restano riferimenti irrisolti.
if(NOT MSVC AND AM_WX_CONFIG_MODE AND NOT AM_WX_IS_SHARED)
    target_link_libraries(AliveMonitor PRIVATE
        -Wl,--start-group ${AM_WX_LIBRARIES} -Wl,--end-group
        Threads::Threads)
else()
    target_link_libraries(AliveMonitor PRIVATE
        ${AM_WX_LIBRARIES}
        Threads::Threads)
endif()

# --- Linking statico del runtime del compilatore (Windows/MinGW) ----------------
# Elimina la dipendenza a runtime da libstdc++-6.dll / libgcc_s_seh-1.dll /
# libwinpthread-1.dll, che sono le DLL NON copiate automaticamente (vedi sotto)
# e la causa più comune dell'errore "impossibile trovare il punto di ingresso":
# un mismatch fra la libstdc++ trovata a runtime nel PATH e quella con cui è
# stata compilata wxWidgets. Con AM_STATIC_RUNTIME=ON queste librerie vengono
# incorporate direttamente nell'eseguibile, quindi il problema non si pone più.
option(AM_STATIC_RUNTIME
    "Linka staticamente il runtime del compilatore (MinGW: libstdc++/libgcc/winpthread; MSVC: CRT /MT)" ON)

if(WIN32 AND MINGW AND AM_STATIC_RUNTIME)
    target_link_options(AliveMonitor PRIVATE
        -static-libgcc -static-libstdc++ -static)
    message(STATUS "AM_STATIC_RUNTIME: runtime MinGW (libstdc++/libgcc/winpthread) linkato staticamente")
endif()

if(MSVC AND AM_STATIC_RUNTIME)
    set_property(TARGET AliveMonitor PROPERTY
        MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
    message(STATUS "AM_STATIC_RUNTIME: CRT MSVC linkato staticamente (/MT); richiede una wxWidgets compilata con lo stesso runtime (vcpkg: triplet x64-windows-static)")
endif()

# --- Copia automatica delle DLL di wxWidgets (Windows, solo se wx è condivisa) ---
# Se wxWidgets è compilata come DLL (wxBUILD_SHARED=ON, il default nelle guide
# del README) l'eseguibile ha comunque bisogno delle sue DLL a runtime: questo
# passo le copia accanto all'eseguibile. Se invece wx è stata (ri)compilata in
# modalità statica (-DwxBUILD_SHARED=OFF) non c'è nulla da copiare e
# l'eseguibile risultante non dipende più da nessuna DLL di terze parti (solo
# dalle DLL di sistema di Windows). Disattivabile con -DAM_COPY_WX_DLLS=OFF.
option(AM_COPY_WX_DLLS
    "Copia le DLL di wxWidgets accanto all'eseguibile (build condivise)" ON)

if(WIN32 AND AM_WX_CONFIG_MODE AND AM_WX_IS_SHARED AND AM_COPY_WX_DLLS)
    add_custom_command(TARGET AliveMonitor POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
                $<TARGET_RUNTIME_DLLS:AliveMonitor>
                $<TARGET_FILE_DIR:AliveMonitor>
        COMMAND_EXPAND_LISTS
        COMMENT "Copia delle DLL di wxWidgets accanto all'eseguibile")
elseif(WIN32 AND AM_WX_CONFIG_MODE AND NOT AM_WX_IS_SHARED)
    message(STATUS "wxWidgets è statica: eseguibile standalone, nessuna DLL di terze parti da copiare")
endif()

# --- Warning severi: il codice deve compilare pulito -----------------------------
if(MSVC)
    target_compile_options(AliveMonitor PRIVATE /W4 /permissive- /utf-8)
    target_compile_definitions(AliveMonitor PRIVATE
        _CRT_SECURE_NO_WARNINGS NOMINMAX WIN32_LEAN_AND_MEAN)
else()
    target_compile_options(AliveMonitor PRIVATE -Wall -Wextra -Wpedantic)
endif()

# --- Installazione (opzionale) ----------------------------------------------------
install(TARGETS AliveMonitor RUNTIME DESTINATION bin)
