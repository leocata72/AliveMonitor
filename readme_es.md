# AliveMonitor

[Italiano](readme_it.md) · [English](README.md) · [Français](readme_fr.md) · **Español** · [Deutsch](readme_de.md)

Aplicación de escritorio (C++20, wxWidgets 3.2+, CMake, arquitectura MVC) para la adquisición en tiempo real de las tensiones analógicas A0–A5 y el control de las salidas digitales D2–D9 de una placa **Arduino Uno** conectada por USB, con firmware dedicado incluido.
AVISO: ¡CARGA PRIMERO EL FIRMWARE EN EL ARDUINO! Se encuentra dentro de la carpeta de instalación ..\AliveMonitor\arduino\AliveMonitor
Características principales: detección automática del puerto serie (protocolo de enlace `HELLO`/`ARDUINO_UNO`) con reconexión automática; gráfico en tiempo real organizado en 7 pestañas — una combinada con seis curvas (zoom, desplazamiento, autoescala, leyenda, rejilla) más una pestaña por canal con un eje Y dedicado en la magnitud física convertida, ver más abajo — con exportación PNG; calibración lineal por canal (V → magnitud física, ver más abajo); registro CSV continuo al pulsar Iniciar (productor/consumidor, ver más abajo); frecuencia de adquisición ajustable de 1 a 250 Hz incluso durante la adquisición; búfer circular thread-safe de 60 segundos por canal (ya dimensionado para 500 Hz); renderizado desacoplado de la adquisición (10–60 FPS configurables); barra de estado con FPS, paquetes recibidos/perdidos, errores CRC/serie, tiempo de conexión y CPU; interfaz disponible en 5 idiomas (Italiano, English, Français, Español, Deutsch), seleccionable desde Configuración (requiere reinicio).
Aprovecho la ocasión para agradecer a Vincenzo Gentile sus sugerencias y consejos. No dudéis en enviar más.

## Estructura del proyecto

```
AliveMonitor/
├── CMakeLists.txt          Sistema de compilación (Windows y Linux)
├── README.md               Este archivo (inglés, versión de referencia)
├── readme_it.md            Versione italiana
├── readme_fr.md            Version française
├── readme_es.md            Versión en español
├── readme_de.md            Deutsche Version
├── LICENSE                 Licencia MIT
├── include/                Cabeceras compartidas (Version, Language, eventos, IUserActions, CpuMonitor)
├── src/                    main.cpp (wxApp), AppEvents, CpuMonitor
├── i18n/                   Strings.h/.cpp: tabla de traducción de la interfaz
├── model/                  MODEL: BoardState, SerialModel, AnalogDataBuffer,
│                           DigitalOutputState, CommunicationProtocol, RingBuffer,
│                           ChannelCalibration, AppSettings
├── view/                   VIEW: MainFrame, ToolbarPanel, DigitalOutputPanel,
│                           AcquisitionPanel, CalibrationPanel, GraphPanel,
│                           StatusPanel, SettingsDialog, LedIndicator, SplashScreen
├── controller/             CONTROLLER: MainController, SerialController,
│                           GraphController, CommandController, CsvLoggerController
├── serial/                 ISerialPort + implementaciones Win32 y POSIX
├── resources/              Recursos incorporados (iconos, guía HTML en 5 idiomas)
├── installer/              Script de Inno Setup para el setup.exe de Windows
├── docs/                   Architecture.md, Protocol.md, Translations.md, UML/
└── arduino/
    └── AliveMonitor/
        └── AliveMonitor.ino  Firmware completo para Arduino Uno
```

## Compilación — Linux

Requiere GCC 12+ (o Clang 15+), CMake ≥ 3.21 y wxWidgets ≥ 3.2.

**Ubuntu 24.04+ / Debian 12+:**

```bash
sudo apt update
sudo apt install build-essential cmake libwxgtk3.2-dev
cd AliveMonitor
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j$(nproc)
./build/AliveMonitor
```

**Distribuciones sin wxWidgets 3.2 en los repositorios oficiales** (p. ej. Ubuntu 22.04, que solo tiene la 3.0; o Slackware, que no la ofrece en absoluto — solo wxGTK3 3.0.5 vía SlackBuilds.org): en lugar de compilar wxWidgets a mano se puede usar el script automático, que no depende de ningún gestor de paquetes (solo necesita `cmake`, `gcc`, `g++`, `make`, `curl` — en Slackware hay que añadir `cmake`, el resto está incluido en una instalación "full"):

```bash
bash scripts/setup-wxwidgets.sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j$(nproc)
```

Descarga, compila (build compartida) e instala wxWidgets 3.2.11 en `third_party/wx-install`, detectada automáticamente por `CMakeLists.txt` — no hace falta pasar `-DCMAKE_PREFIX_PATH`. Idempotente (`--force` para rehacerlo), opciones con `--help`.

**Permisos del puerto serie:** añade el usuario al grupo `dialout` (`sudo usermod -aG dialout $USER`, y vuelve a iniciar sesión).

### Distribuir el ejecutable a otra máquina Linux (sin instalar wxGTK)

En Linux, una build realmente "sin dependencias" como en Windows no es
realista (wxGTK se apoya en GTK, que en tiempo de ejecución carga temas/
iconos del sistema de todos modos, aunque esté enlazada estáticamente). La
alternativa práctica es un **paquete portable**: una carpeta con el
ejecutable y todas sus bibliotecas compartidas no del sistema, lista para
copiarse y ejecutarse en otra máquina (misma arquitectura) sin instalar
nada:

```bash
bash scripts/setup-wxwidgets.sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j$(nproc)
bash scripts/package-linux.sh
```

Crea `dist/AliveMonitor-linux-x64/` con el ejecutable más una carpeta
`lib/` que contiene wxGTK, GTK, glib, pango, cairo, X11, etc. (detectadas
con `ldd`). Las bibliotecas base (glibc, libpthread, ld-linux...) se
excluyen deliberadamente: están ligadas al kernel/a la libc de la máquina
de destino, incluirlas rompería la portabilidad en lugar de ayudarla.
Funciona gracias a un RPATH relativo (`$ORIGIN/lib`) ya incorporado en el
ejecutable por `CMakeLists.txt`: basta con copiar toda la carpeta
`dist/AliveMonitor-linux-x64/` y ejecutar `./AliveMonitor` — sin necesidad
de establecer `LD_LIBRARY_PATH` (`run.sh` incluido como red de seguridad).
El propio script comprueba con `ldd` que no falte ninguna dependencia antes
de darse por concluido.

Limitación: la máquina de destino debe tener una libc compatible (misma
distribución/familia, o una versión igual o más reciente que la de la
máquina de compilación) y la misma arquitectura. Para una portabilidad
completa entre distribuciones (incluyendo también glibc) haría falta un
formato como AppImage — no incluido aquí, pero el paquete de
`package-linux.sh` cubre la mayoría de los casos reales (misma
distribución o familia, p. ej. de Slackware a Slackware, de Ubuntu a
Debian).

## Compilación — Windows

### Opción 0 (recomendada): script automático, build estática sin DLL

Primero hace falta un compilador **MinGW-w64** instalado (p. ej. mediante
[MSYS2](https://www.msys2.org/) o [WinLibs](https://winlibs.com/)). Luego,
sin pasos intermedios:

```bat
cd AliveMonitor
scripts\setup-wxwidgets.bat
cmake -S . -B build -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Release
cmake --build build -j8
build\AliveMonitor.exe
```

El script descarga, compila e instala una wxWidgets 3.2.11 estática en
`%LOCALAPPDATA%\AliveMonitor\wxWidgets` — deliberadamente **fuera** del
proyecto: si la ruta del repositorio contiene caracteres especiales para
expresiones regulares (p. ej. una carpeta llamada `c++`), el archivo
`wxWidgetsConfig.cmake` generado por wxWidgets no es encontrado
correctamente por CMake. `%LOCALAPPDATA%` evita el problema
independientemente de dónde esté el proyecto, y de todos modos es
detectada automáticamente por `CMakeLists.txt`. Busca el compilador MinGW
por sí mismo (en el `PATH`, luego en las carpetas de instalación más
comunes): si lo encuentra en un único lugar, continúa sin preguntar nada.
Si no lo encuentra, o encuentra más de uno instalado, se detiene y pide
indicarlo explícitamente:

```bat
scripts\setup-wxwidgets.bat -MinGwBin C:\ruta\a\mingw64\bin
```

`setup-wxwidgets.bat` es un wrapper que llama a `setup-wxwidgets.ps1`
(reenviando todos los argumentos); si prefieres, puedes llamar
directamente al script de PowerShell:
`powershell -ExecutionPolicy Bypass -File scripts\setup-wxwidgets.ps1 [-MinGwBin ...]`.
El script es idempotente (omite la recompilación si ya se hizo; `-Force`
para rehacerla).
El ejecutable resultante no depende de ninguna DLL de terceros (verificado
con `objdump -p AliveMonitor.exe`), solo de las DLL del sistema de
Windows: se puede copiar y distribuir solo.

**Limpieza:** wxWidgets compilada (fuente + build + instalación, unos
cientos de MB) permanece en `%LOCALAPPDATA%\AliveMonitor`, compartida
entre posibles clones del proyecto. Para liberar espacio o forzar una
recompilación completa, bórrala con:

```bat
rmdir /s /q "%LOCALAPPDATA%\AliveMonitor"
```

(no afecta al proyecto ni a la carpeta `build\` de AliveMonitor: solo
wxWidgets deberá recompilarse, con `scripts\setup-wxwidgets.bat`, si es
necesario.)

Las opciones A/B/C de abajo siguen siendo válidas para quien prefiera
gestionar wxWidgets manualmente (p. ej. build compartida/DLL, o una
wxWidgets ya instalada en otro sitio).

### Opción A: wxWidgets compilada desde el código fuente con MinGW (modo CONFIG)

Si wxWidgets se compiló con CMake desde el código fuente, por ejemplo:

```bat
cd C:\library\wxWidgets
mkdir build_gcc && cd build_gcc
cmake -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Release -DwxBUILD_SHARED=ON -DwxBUILD_SAMPLES=OFF -DwxBUILD_TESTS=OFF ..
cmake --build . -j8
```

luego es **necesario instalarla** (en wx 3.2.x el archivo `wxWidgetsConfig.cmake` solo funciona desde el prefijo de instalación, no desde el árbol de compilación):

```bat
cmake --install C:\library\wxWidgets\build_gcc --prefix C:\library\wx-install
```

y a continuación compilar AliveMonitor indicando el prefijo:

```bat
cd AliveMonitor
cmake -S . -B build -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Release -DCMAKE_PREFIX_PATH=C:/library/wx-install
cmake --build build -j8
build\AliveMonitor.exe
```

Notas para este modo:
- con `wxBUILD_SHARED=ON` las DLL de wxWidgets se **copian automáticamente** junto al ejecutable durante la compilación (desactivable con `-DAM_COPY_WX_DLLS=OFF`, a usar para builds estáticas);
- también hacen falta en tiempo de ejecución las DLL de MinGW (`libstdc++-6.dll`, `libgcc_s_seh-1.dll`, `libwinpthread-1.dll`): el proyecto las enlaza **estáticamente por defecto** (`AM_STATIC_RUNTIME=ON`, ver abajo), así que normalmente no hace falta copiarlas ni tocar el `PATH`;
- **usa el mismo compilador MinGW** con el que se compiló wxWidgets (misma toolchain en el `PATH`) — un desajuste de toolchain es la causa más común del error *"no se puede encontrar el punto de entrada"* al iniciar.

### Build totalmente estática (ninguna DLL que distribuir)

Para obtener un `AliveMonitor.exe` que no dependa de ninguna DLL de terceros (solo de las DLL del sistema de Windows), además del runtime del compilador hace falta compilar también **wxWidgets en modo estático**:

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

- `AM_STATIC_RUNTIME` (por defecto **ON**) añade `-static-libgcc -static-libstdc++ -static`, incorporando libstdc++/libgcc/winpthread en el exe. Desactivable con `-DAM_STATIC_RUNTIME=OFF` si prefieres distribuir esas DLL por separado.
- Con wx estática (`wxBUILD_SHARED=OFF`) el paso de copia de DLL (`AM_COPY_WX_DLLS`) se desactiva automáticamente: no hay nada que copiar.
- En MSVC el mismo objetivo se logra con `AM_STATIC_RUNTIME=ON` (CRT `/MT`, automático) más una wxWidgets/vcpkg compilada con triplete estático, p. ej. `vcpkg install wxwidgets:x64-windows-static`.
- El ejecutable estático es más grande (el runtime y wx están incorporados), pero se copia y distribuye como un único archivo, sin riesgo de desajuste de versión entre las DLL.

### Opción B: MSYS2/MinGW con paquetes precompilados

1. Instala [MSYS2](https://www.msys2.org/), abre la terminal *MSYS2 MINGW64* y ejecuta:

```bash
pacman -S --needed mingw-w64-x86_64-toolchain mingw-w64-x86_64-cmake mingw-w64-x86_64-wxwidgets3.2-msw
cd /c/ruta/a/AliveMonitor
cmake -S . -B build -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Release
cmake --build build -j
./build/AliveMonitor.exe
```

### Opción C: Visual Studio 2022 + vcpkg

```bat
vcpkg install wxwidgets:x64-windows
cd AliveMonitor
cmake -S . -B build -DCMAKE_TOOLCHAIN_FILE=%VCPKG_ROOT%\scripts\buildsystems\vcpkg.cmake
cmake --build build --config Release
build\Release\AliveMonitor.exe
```

El controlador del puerto COM del Arduino Uno lo instala automáticamente Windows 10/11 (para los clones con CH340, instala el controlador del fabricante).

### Crear el instalador de Windows (setup.exe)

Para distribuir AliveMonitor a un usuario final sin que tenga que instalar un compilador ni wxWidgets, se puede generar un instalador autónomo con [Inno Setup](https://jrsoftware.org/isinfo.php) (gratuito, solo hay que instalarlo en la máquina que *crea* el setup, no en la que lo *recibe*):

```bat
scripts\setup-wxwidgets.bat
cmake -S . -B build-static -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Release
cmake --build build-static -j8

powershell -ExecutionPolicy Bypass -File scripts\package-windows.ps1
```

El wrapper `package-windows.ps1` verifica que `build-static\AliveMonitor.exe` exista (hace falta la **build totalmente estática**, sin DLL de las que depender: ver arriba), obtiene automáticamente la versión de `include/Version.h`, localiza `ISCC.exe` (en el `PATH` o en las carpetas de instalación predeterminadas de Inno Setup) y produce `dist\AliveMonitor-Setup-<versión>.exe` — icono, entrada en el menú Inicio, acceso directo en el escritorio opcional, licencia mostrada durante la instalación, desinstalador en "Aplicaciones y características". Además del ejecutable, el instalador también copia el sketch `AliveMonitor.ino` (carpeta `arduino\AliveMonitor\`, también accesible desde el menú Inicio): quien instale desde este setup.exe, sin haber clonado el repositorio, aun así debe poder cargar el firmware en la placa.

El script de Inno Setup (`installer\AliveMonitor.iss`) empaqueta el **ejecutable ya compilado**, no el código fuente (que permanece en el repositorio de GitHub): el archivo `dist\AliveMonitor-Setup-*.exe` producido NO debe subirse al repositorio — es un artefacto de compilación (la carpeta `dist/` ya está en `.gitignore`), para distribuir por separado, por ejemplo como adjunto de una GitHub Release.

## Firmware de Arduino

1. Abre `arduino/AliveMonitor/AliveMonitor.ino` con el IDE de Arduino (≥ 1.8) o `arduino-cli`.
2. Selecciona la placa **Arduino Uno** y el puerto correcto.
3. Carga el sketch. Listo: la aplicación de PC encontrará la placa por sí sola.

Con `arduino-cli`:

```bash
arduino-cli compile --fqbn arduino:avr:uno arduino/AliveMonitor/AliveMonitor.ino
arduino-cli upload  --fqbn arduino:avr:uno -p /dev/ttyACM0 arduino/AliveMonitor/AliveMonitor.ino
```

## Uso

Al iniciarse, el programa explora automáticamente los puertos serie y se conecta a la placa que responde `ARDUINO_UNO` (LED verde en la esquina superior izquierda). *Iniciar* comienza el streaming a la frecuencia configurada con el SpinCtrl o el deslizador (1–250 Hz, modificable sobre la marcha); *Pausa* lo suspende conservando los datos; *Detener* lo para y vacía el búfer. Los botones D2–D9 controlan las salidas: el LED junto a cada botón refleja el **estado real** confirmado por el firmware. En el gráfico: rueda del ratón = zoom en el tiempo, Ctrl+rueda = zoom en tensión, arrastrar = desplazar, *Seguir* vuelve a enganchar la ventana desplazable, *Restablecer* restaura la vista predeterminada (últimos 60 s, 0–5 V). Si la placa se desconecta, la aplicación lo notifica y reanuda automáticamente la búsqueda, restableciendo al reconectarse las salidas y el streaming.

### Registro CSV continuo

Al pulsar *Iniciar* desde un estado detenido (no desde *Pausa*) aparece una
ventana para elegir dónde guardar el archivo, con un nombre propuesto ya
preparado (`acquisizione_AAAA-MM-DD_HH-MM-SS.csv`); cancelar la ventana
también cancela el inicio de la adquisición. Una casilla "Registrar CSV"
bajo Iniciar/Pausa/Detener (marcada por defecto) permite saltarse por
completo el diálogo y el registro: si está desmarcada, Iniciar arranca de
inmediato sin preguntar ni escribir ningún archivo. Con el registro
activo, cada muestra ADC recibida se escribe en el archivo en tiempo real,
sin bloquear nunca el hilo serie: está implementado con un patrón
**productor/consumidor** (`CsvLoggerController`) — el hilo serie encola
las muestras en una cola thread-safe (`push()`, no bloqueante), un tercer
hilo dedicado las recoge y las escribe en disco. Un LED (verde =
escribiendo, rojo = detenido) y una etiqueta con la ruta completa del
archivo, centrados en la barra del gráfico, muestran el estado del
registro en curso. *Pausa* no interrumpe el archivo (sigue siendo el
mismo, con posibles "huecos" en los tiempos de pausa); *Detener* cierra el
registro esperando a que el consumidor termine de escribir todas las
muestras ya en cola, de modo que no se pierde ningún dato ni al detenerse
manualmente ni al cerrar la aplicación.

### Calibración de canales (V → magnitud física)

Debajo del panel de adquisición hay una rejilla (`CalibrationPanel`) con
una fila por canal (A0..A5) y cuatro columnas editables: coeficiente `a`,
offset `b`, unidad de medida y descripción. Para cada transductor
conectado con ley lineal G = a·V + b (p. ej. un sensor de temperatura 0–5
V → 0–100 °C tendría a=20, b=0, unidad "°C"), introduce los valores en la
fila del canal correspondiente: el efecto es inmediato en la leyenda del
gráfico, que junto al nombre del canal muestra también el valor convertido
en tiempo real (p. ej. "A0: 23.4 °C"). Los canales sin configurar
permanecen en la identidad (a=1, b=0, unidad "V"), es decir, muestran
simplemente la tensión. La descripción es una etiqueta libre (p. ej.
"Temperatura horno"): si se rellena, pasa a ser el título de la pestaña
dedicada a ese canal en el gráfico (ver más abajo), de lo contrario la
pestaña conserva el nombre del canal ("A0".."A5"). La calibración solo es
válida para la sesión actual (no se guarda en disco); si hay un registro
CSV activo, el archivo incluye — junto a las seis columnas `A#_V` en
Voltios — seis columnas adicionales `A#_conv[unidad]` con el valor ya
convertido, usando la calibración establecida en el momento en que se
pulsó Iniciar (un cambio realizado con un registro ya en marcha no altera
ni la cabecera ni las filas ya escritas en esa sesión).

### Gráfico de 7 pestañas (ejes independientes por canal)

El gráfico es un `wxNotebook` con 7 pestañas. La primera, "Todos", es el
gráfico combinado de siempre: las seis curvas comparten un único eje Y en
Voltios, con las casillas para ocultar/mostrar los canales individuales y
el estado del registro CSV (LED + ruta del archivo). Las otras seis
pestañas (A0..A5) muestran cada una un solo canal, con la curva y el eje Y
en la magnitud *ya convertida* según su calibración (o en Voltios si no
está calibrado) — una forma sencilla de tener en la práctica "ejes
múltiples" cuando los canales miden magnitudes de naturaleza y escala
distintas, sin tener que superponerlas en un único eje en Voltios. El
título de cada una de estas seis pestañas es la descripción establecida en
la rejilla de calibración (columna "descripción"), o el nombre del canal
si no está rellena; cambia en tiempo real con cada modificación de la
rejilla, sin necesidad de reiniciar. Cada pestaña de canal único tiene
Auto Y activado por defecto (ya que el rango de la magnitud convertida no
se conoce de antemano) y sus propios botones Autoescala/Seguir/Restablecer/
PNG independientes de las demás pestañas; la exportación PNG guarda
siempre la pestaña mostrada actualmente. La ventana temporal (en
Configuración) se aplica a las 7 pestañas juntas, mientras que la
autoescala Y continua del panel de Configuración solo afecta a la pestaña
combinada — las seis pestañas por canal permanecen independientes.

### Idioma de la interfaz

Desde el menú Herramientas &gt; Configuración se puede elegir el idioma de
la interfaz entre Italiano, English, Français, Español y Deutsch. La
elección se guarda en `%APPDATA%\AliveMonitor\settings.ini` (Windows) o
`~/.config/AliveMonitor/settings.ini` (Linux) y requiere reiniciar la
aplicación para surtir efecto: los widgets ya construidos no se vuelven a
traducir en caliente. La tabla completa de traducciones está en
`docs/Translations.md`.

## Documentación

La descripción completa de la arquitectura, las decisiones de diseño y las sugerencias de extensión está en `docs/Architecture.md`; la especificación del protocolo serie en `docs/Protocol.md`; la tabla de traducciones de la interfaz en `docs/Translations.md`; los diagramas UML (de clases y de secuencia, PlantUML + Mermaid) en `docs/UML/`; la guía paso a paso para compilar con CMake GUI (wxWidgets estática incluida) en `docs/CMakeGUI.md`.

## Guía integrada

Desde el menú Ayuda &gt; Guía (o F1) se abre en el navegador predeterminado una guía rápida de uso de la aplicación, en el idioma actual de la interfaz, incorporada en el ejecutable como los demás recursos (icono incluido): ningún archivo externo que distribuir junto con el binario.

## Licencia

Distribuido bajo licencia **MIT**: libre de usar, modificar y redistribuir, incluso con fines comerciales, proporcionado "tal cual" sin ninguna garantía ni responsabilidad de los autores por los daños que pudieran derivarse del uso del software. Texto completo en [`LICENSE`](LICENSE).
