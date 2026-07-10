# Risorse

Cartella per icone e risorse dell'applicazione.

`AliveMonitor.xpm`: icona applicativa, formato XPM (array C, incorporata
via `#include "resources/AliveMonitor.xpm"` — nessun file esterno da
distribuire). Già collegata in `view/MainFrame.cpp` a `SetIcon()` del frame
principale (titolo/taskbar) e a `wxAboutDialogInfo::SetIcon()` nella
finestra "Informazioni". Multipiattaforma per natura: XPM funziona sia su
Windows sia su Linux senza differenze.

L'icona del **file** `AliveMonitor.exe` su Windows (quella mostrata da
Esplora risorse prima ancora di avviare il programma, distinta dall'icona di
finestra sopra) è collegata tramite `appicon.ico` (7 risoluzioni, 16..256 px,
generato dallo stesso xpm) e `AliveMonitor.rc` (`IDI_APPICON ICON
"appicon.ico"`), aggiunto alle sorgenti Windows in `CMakeLists.txt`
(`AM_PLATFORM_SOURCES`, ramo `if(WIN32)`) — compilato automaticamente da
`windres` (MinGW) o `rc.exe` (MSVC), nessun passo manuale aggiuntivo.
