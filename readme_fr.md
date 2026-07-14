# AliveMonitor

[Italiano](readme_it.md) · [English](README.md) · **Français** · [Español](readme_es.md) · [Deutsch](readme_de.md)

Application de bureau (C++20, wxWidgets 3.2+, CMake, architecture MVC) pour l'acquisition en temps réel des tensions analogiques A0–A5 et le contrôle des sorties numériques D2–D9 d'une carte **Arduino Uno** connectée en USB, avec un firmware dédié inclus.
AVIS : CHARGEZ D'ABORD LE FIRMWARE SUR L'ARDUINO !! Il se trouve dans le dossier d'installation ..\AliveMonitor\arduino\AliveMonitor
Fonctionnalités principales : détection automatique du port série (négociation `HELLO`/`ARDUINO_UNO`) avec reconnexion automatique ; graphique en temps réel organisé en 7 onglets — un onglet combiné à six courbes (zoom, déplacement, auto-échelle, légende, grille) plus un onglet par canal avec un axe Y dédié dans la grandeur physique convertie, voir plus bas — avec export PNG ; étalonnage linéaire par canal (V → grandeur physique, voir plus bas) ; enregistrement CSV continu au démarrage (producteur/consommateur, voir plus bas) ; fréquence d'acquisition réglable de 1 à 250 Hz même en cours d'acquisition ; tampon circulaire thread-safe de 60 secondes par canal (déjà dimensionné pour 500 Hz) ; rendu découplé de l'acquisition (10–60 FPS configurables) ; barre d'état avec FPS, paquets reçus/perdus, erreurs CRC/série, temps de connexion et CPU ; interface disponible en 5 langues (Italiano, English, Français, Español, Deutsch), sélectionnable depuis les Paramètres (redémarrage requis).
J'en profite pour remercier Vincenzo Gentile pour ses suggestions et ses conseils. N'hésitez pas à en envoyer d'autres.

## Structure du projet

```
AliveMonitor/
├── CMakeLists.txt          Système de build (Windows et Linux)
├── README.md               Ce fichier (anglais, version de référence)
├── readme_it.md            Versione italiana
├── readme_fr.md            Version française
├── readme_es.md            Versión en español
├── readme_de.md            Deutsche Version
├── LICENSE                 Licence MIT
├── include/                En-têtes partagés (Version, Language, événements, IUserActions, CpuMonitor)
├── src/                    main.cpp (wxApp), AppEvents, CpuMonitor
├── i18n/                   Strings.h/.cpp : table de traduction de l'interface
├── model/                  MODEL : BoardState, SerialModel, AnalogDataBuffer,
│                           DigitalOutputState, CommunicationProtocol, RingBuffer,
│                           ChannelCalibration, AppSettings
├── view/                   VIEW : MainFrame, ToolbarPanel, DigitalOutputPanel,
│                           AcquisitionPanel, CalibrationPanel, GraphPanel,
│                           StatusPanel, SettingsDialog, LedIndicator, SplashScreen
├── controller/             CONTROLLER : MainController, SerialController,
│                           GraphController, CommandController, CsvLoggerController
├── serial/                 ISerialPort + implémentations Win32 et POSIX
├── resources/              Ressources incorporées (icônes, guide HTML en 5 langues)
├── installer/              Script Inno Setup pour le setup.exe Windows
├── docs/                   Architecture.md, Protocol.md, Translations.md, UML/
└── arduino/
    └── AliveMonitor/
        └── AliveMonitor.ino  Firmware complet pour Arduino Uno
```

## Compilation — Linux

Nécessite GCC 12+ (ou Clang 15+), CMake ≥ 3.21 et wxWidgets ≥ 3.2.

**Ubuntu 24.04+ / Debian 12+ :**

```bash
sudo apt update
sudo apt install build-essential cmake libwxgtk3.2-dev
cd AliveMonitor
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j$(nproc)
./build/AliveMonitor
```

**Distributions sans wxWidgets 3.2 dans les dépôts officiels** (par ex. Ubuntu 22.04, qui n'a que la 3.0 ; ou Slackware, qui ne l'offre pas du tout — seulement wxGTK3 3.0.5 via SlackBuilds.org) : au lieu de compiler wxWidgets à la main, on peut utiliser le script automatique, qui ne dépend d'aucun gestionnaire de paquets (il faut seulement `cmake`, `gcc`, `g++`, `make`, `curl` — sur Slackware `cmake` doit être ajouté, le reste est inclus dans une installation "full") :

```bash
bash scripts/setup-wxwidgets.sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j$(nproc)
```

Télécharge, compile (build partagée) et installe wxWidgets 3.2.11 dans `third_party/wx-install`, détectée automatiquement par `CMakeLists.txt` — pas besoin de passer `-DCMAKE_PREFIX_PATH`. Idempotent (`--force` pour recommencer), options avec `--help`.

**Permissions du port série :** ajoutez l'utilisateur au groupe `dialout` (`sudo usermod -aG dialout $USER`, puis reconnectez-vous).

### Distribuer l'exécutable sur une autre machine Linux (sans installer wxGTK)

Sur Linux, une build vraiment "sans dépendances" comme sous Windows n'est
pas réaliste (wxGTK s'appuie sur GTK, qui charge de toute façon des
thèmes/icônes du système au moment de l'exécution, même en cas de liaison
statique). L'alternative pratique est un **bundle portable** : un dossier
avec l'exécutable et toutes ses bibliothèques partagées non systèmes, prêt
à être copié et exécuté sur une autre machine (même architecture) sans
rien installer :

```bash
bash scripts/setup-wxwidgets.sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j$(nproc)
bash scripts/package-linux.sh
```

Crée `dist/AliveMonitor-linux-x64/` avec l'exécutable et un dossier `lib/`
contenant wxGTK, GTK, glib, pango, cairo, X11, etc. (détectées avec `ldd`).
Les bibliothèques de base (glibc, libpthread, ld-linux...) sont délibérément
exclues : elles sont liées au noyau/à la libc de la machine cible, les
inclure casserait la portabilité au lieu de l'aider. Cela fonctionne grâce
à un RPATH relatif (`$ORIGIN/lib`) déjà incorporé dans l'exécutable par
`CMakeLists.txt` : il suffit de copier tout le dossier
`dist/AliveMonitor-linux-x64/` et d'exécuter `./AliveMonitor` — aucun
`LD_LIBRARY_PATH` à définir (`run.sh` est inclus comme filet de sécurité).
Le script lui-même vérifie avec `ldd` qu'aucune dépendance ne manque avant
de se déclarer terminé.

Limite : la machine cible doit avoir une libc compatible (même
distribution/famille, ou une version égale ou plus récente que celle de la
machine de build) et la même architecture. Pour une portabilité
inter-distributions complète (incluant aussi glibc), il faudrait un format
comme AppImage — non inclus ici, mais le bundle de `package-linux.sh`
couvre la plupart des cas réels (même distribution ou famille, par ex. de
Slackware à Slackware, d'Ubuntu à Debian).

## Compilation — Windows

### Option 0 (recommandée) : script automatique, build statique sans DLL

Il faut d'abord un compilateur **MinGW-w64** installé (par ex. via
[MSYS2](https://www.msys2.org/) ou [WinLibs](https://winlibs.com/)).
Ensuite, sans étape intermédiaire :

```bat
cd AliveMonitor
scripts\setup-wxwidgets.bat
cmake -S . -B build -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Release
cmake --build build -j8
build\AliveMonitor.exe
```

Le script télécharge, compile et installe une wxWidgets 3.2.11 statique
dans `%LOCALAPPDATA%\AliveMonitor\wxWidgets` — délibérément **en dehors**
du projet : si le chemin du dépôt contient des caractères spéciaux pour
les expressions régulières (par ex. un dossier nommé `c++`), le fichier
`wxWidgetsConfig.cmake` généré par wxWidgets n'est pas trouvé correctement
par CMake. `%LOCALAPPDATA%` évite ce problème quel que soit
l'emplacement du projet, et est de toute façon détecté automatiquement par
`CMakeLists.txt`. Il recherche le compilateur MinGW par lui-même (dans le
`PATH`, puis dans les dossiers d'installation les plus courants) : s'il le
trouve à un seul endroit, il continue sans rien demander. S'il ne le
trouve pas, ou s'il en trouve plusieurs installés, il s'arrête et demande
de le préciser explicitement :

```bat
scripts\setup-wxwidgets.bat -MinGwBin C:\chemin\vers\mingw64\bin
```

`setup-wxwidgets.bat` est un wrapper qui appelle `setup-wxwidgets.ps1`
(en transmettant tous les arguments) ; si vous préférez, vous pouvez
appeler directement le script PowerShell :
`powershell -ExecutionPolicy Bypass -File scripts\setup-wxwidgets.ps1 [-MinGwBin ...]`.
Le script est idempotent (saute la recompilation si déjà faite ; `-Force`
pour la refaire).
L'exécutable obtenu ne dépend d'aucune DLL tierce (vérifié avec
`objdump -p AliveMonitor.exe`), seulement des DLL système de Windows : il
peut être copié et distribué seul.

**Nettoyage :** wxWidgets compilée (sources + build + installation, environ
quelques centaines de Mo) reste dans `%LOCALAPPDATA%\AliveMonitor`,
partagée entre d'éventuels clones du projet. Pour libérer de l'espace ou
forcer une recompilation complète, supprimez-la avec :

```bat
rmdir /s /q "%LOCALAPPDATA%\AliveMonitor"
```

(cela ne touche ni le projet ni le dossier `build\` d'AliveMonitor : seule
wxWidgets devra être recompilée, avec `scripts\setup-wxwidgets.bat`, si
nécessaire.)

Les options A/B/C ci-dessous restent valables pour ceux qui préfèrent
gérer wxWidgets manuellement (par ex. build partagée/DLL, ou une
wxWidgets déjà installée ailleurs).

### Option A : wxWidgets compilée depuis les sources avec MinGW (mode CONFIG)

Si wxWidgets a été compilée avec CMake depuis les sources, par exemple :

```bat
cd C:\library\wxWidgets
mkdir build_gcc && cd build_gcc
cmake -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Release -DwxBUILD_SHARED=ON -DwxBUILD_SAMPLES=OFF -DwxBUILD_TESTS=OFF ..
cmake --build . -j8
```

il faut ensuite **l'installer** (dans wx 3.2.x, le fichier `wxWidgetsConfig.cmake` ne fonctionne que depuis le préfixe d'installation, pas depuis l'arbre de build) :

```bat
cmake --install C:\library\wxWidgets\build_gcc --prefix C:\library\wx-install
```

puis compiler AliveMonitor en indiquant le préfixe :

```bat
cd AliveMonitor
cmake -S . -B build -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Release -DCMAKE_PREFIX_PATH=C:/library/wx-install
cmake --build build -j8
build\AliveMonitor.exe
```

Remarques pour ce mode :
- avec `wxBUILD_SHARED=ON`, les DLL de wxWidgets sont **copiées automatiquement** à côté de l'exécutable lors de la build (désactivable avec `-DAM_COPY_WX_DLLS=OFF`, à utiliser pour les builds statiques) ;
- les DLL de MinGW (`libstdc++-6.dll`, `libgcc_s_seh-1.dll`, `libwinpthread-1.dll`) sont aussi nécessaires à l'exécution : le projet les lie **statiquement par défaut** (`AM_STATIC_RUNTIME=ON`, voir plus bas), donc normalement pas besoin de les copier ni de toucher au `PATH` ;
- **utilisez le même compilateur MinGW** que celui avec lequel wxWidgets a été compilée (même chaîne d'outils dans le `PATH`) — une incompatibilité de chaîne d'outils est la cause la plus fréquente de l'erreur *"point d'entrée introuvable"* au démarrage.

### Build entièrement statique (aucune DLL à distribuer)

Pour obtenir un `AliveMonitor.exe` qui ne dépend d'aucune DLL tierce (seulement des DLL système de Windows), en plus du runtime du compilateur il faut aussi compiler **wxWidgets en mode statique** :

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

- `AM_STATIC_RUNTIME` (par défaut **ON**) ajoute `-static-libgcc -static-libstdc++ -static`, incorporant libstdc++/libgcc/winpthread dans l'exe. Désactivable avec `-DAM_STATIC_RUNTIME=OFF` si vous préférez distribuer ces DLL séparément.
- Avec wx statique (`wxBUILD_SHARED=OFF`), l'étape de copie des DLL (`AM_COPY_WX_DLLS`) se désactive automatiquement : il n'y a rien à copier.
- Sur MSVC, le même objectif s'obtient avec `AM_STATIC_RUNTIME=ON` (CRT `/MT`, automatique) plus une wxWidgets/vcpkg compilée avec un triplet statique, par ex. `vcpkg install wxwidgets:x64-windows-static`.
- L'exécutable statique est plus volumineux (le runtime et wx sont incorporés), mais se copie et se distribue comme un fichier unique, sans risque d'incompatibilité de version entre les DLL.

### Option B : MSYS2/MinGW avec des paquets précompilés

1. Installez [MSYS2](https://www.msys2.org/), ouvrez le terminal *MSYS2 MINGW64* et exécutez :

```bash
pacman -S --needed mingw-w64-x86_64-toolchain mingw-w64-x86_64-cmake mingw-w64-x86_64-wxwidgets3.2-msw
cd /c/chemin/vers/AliveMonitor
cmake -S . -B build -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Release
cmake --build build -j
./build/AliveMonitor.exe
```

### Option C : Visual Studio 2022 + vcpkg

```bat
vcpkg install wxwidgets:x64-windows
cd AliveMonitor
cmake -S . -B build -DCMAKE_TOOLCHAIN_FILE=%VCPKG_ROOT%\scripts\buildsystems\vcpkg.cmake
cmake --build build --config Release
build\Release\AliveMonitor.exe
```

Le pilote du port COM de l'Arduino Uno est installé automatiquement par Windows 10/11 (pour les clones à base de CH340, installez le pilote du fabricant).

### Créer l'installateur Windows (setup.exe)

Pour distribuer AliveMonitor à un utilisateur final sans lui faire installer de compilateur ni wxWidgets, on peut générer un installateur autonome avec [Inno Setup](https://jrsoftware.org/isinfo.php) (gratuit, à installer uniquement sur la machine qui *crée* le setup, pas sur celle qui le *reçoit*) :

```bat
scripts\setup-wxwidgets.bat
cmake -S . -B build-static -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Release
cmake --build build-static -j8

powershell -ExecutionPolicy Bypass -File scripts\package-windows.ps1
```

Le wrapper `package-windows.ps1` vérifie que `build-static\AliveMonitor.exe` existe (il faut la **build entièrement statique**, sans DLL dont dépendre : voir ci-dessus), détermine automatiquement la version depuis `include/Version.h`, localise `ISCC.exe` (dans le `PATH` ou dans les dossiers d'installation par défaut d'Inno Setup) et produit `dist\AliveMonitor-Setup-<version>.exe` — icône, entrée dans le menu Démarrer, raccourci bureau optionnel, licence affichée pendant l'installation, désinstalleur dans "Applications et fonctionnalités". En plus de l'exécutable, l'installateur copie aussi le sketch `AliveMonitor.ino` (dossier `arduino\AliveMonitor\`, également accessible depuis le menu Démarrer) : quiconque installe depuis ce setup.exe, sans avoir cloné le dépôt, doit tout de même pouvoir charger le firmware sur la carte.

Le script Inno Setup (`installer\AliveMonitor.iss`) empaquette l'**exécutable déjà compilé**, pas les sources (qui restent dans le dépôt GitHub) : le fichier `dist\AliveMonitor-Setup-*.exe` produit ne doit PAS être commité — c'est un artefact de build (le dossier `dist/` est déjà dans `.gitignore`), à distribuer séparément, par exemple en pièce jointe d'une GitHub Release.

## Firmware Arduino

1. Ouvrez `arduino/AliveMonitor/AliveMonitor.ino` avec l'IDE Arduino (≥ 1.8) ou `arduino-cli`.
2. Sélectionnez la carte **Arduino Uno** et le port correct.
3. Chargez le sketch. C'est fait : l'application PC trouvera la carte toute seule.

Avec `arduino-cli` :

```bash
arduino-cli compile --fqbn arduino:avr:uno arduino/AliveMonitor/AliveMonitor.ino
arduino-cli upload  --fqbn arduino:avr:uno -p /dev/ttyACM0 arduino/AliveMonitor/AliveMonitor.ino
```

## Utilisation

Au démarrage, le programme analyse automatiquement les ports série et se connecte à la carte qui répond `ARDUINO_UNO` (voyant vert en haut à gauche). *Démarrer* lance le streaming à la fréquence réglée avec le SpinCtrl ou le curseur (1–250 Hz, modifiable à la volée) ; *Pause* le suspend en conservant les données ; *Arrêter* l'arrête et vide le tampon. Les boutons D2–D9 commandent les sorties : le voyant à côté de chaque bouton reflète l'**état réel** confirmé par le firmware. Dans le graphique : molette = zoom sur le temps, Ctrl+molette = zoom sur la tension, glisser = déplacement, *Suivre* raccroche la fenêtre défilante, *Réinitialiser* restaure la vue par défaut (60 dernières secondes, 0–5 V). Si la carte est déconnectée, l'application le signale et reprend automatiquement la recherche, rétablissant les sorties et le streaming à la reconnexion.

### Enregistrement CSV continu

Appuyer sur *Démarrer* depuis un état arrêté (pas depuis *Pause*) fait
apparaître une fenêtre pour choisir où enregistrer le fichier, avec un nom
proposé déjà prêt (`acquisizione_AAAA-MM-JJ_HH-MM-SS.csv`) ; annuler la
fenêtre annule aussi le démarrage de l'acquisition. Une case "Enregistrer
CSV" sous Démarrer/Pause/Arrêter (cochée par défaut) permet de sauter
entièrement la boîte de dialogue et l'enregistrement : si décochée,
Démarrer part immédiatement sans rien demander ni écrire aucun fichier.
Avec l'enregistrement actif, chaque échantillon ADC reçu est écrit dans le
fichier en temps réel, sans jamais bloquer le thread série : c'est
implémenté avec un motif **producteur/consommateur** (`CsvLoggerController`)
— le thread série met les échantillons en file d'attente thread-safe
(`push()`, non bloquant), un troisième thread dédié les récupère et les
écrit sur le disque. Un voyant (vert = en écriture, rouge = arrêté) et une
étiquette avec le chemin complet du fichier, centrés dans la barre du
graphique, indiquent l'état de l'enregistrement en cours. *Pause*
n'interrompt pas le fichier (il reste le même, avec d'éventuels "trous"
pendant les périodes de pause) ; *Arrêter* ferme l'enregistrement en
attendant que le consommateur ait fini d'écrire tous les échantillons déjà
en file d'attente, de sorte qu'aucune donnée n'est perdue, que ce soit à
l'arrêt manuel ou à la fermeture de l'application.

### Étalonnage des canaux (V → grandeur physique)

Sous le panneau d'acquisition se trouve une grille (`CalibrationPanel`) avec
une ligne par canal (A0..A5) et quatre colonnes modifiables : coefficient
`a`, offset `b`, unité de mesure et description. Pour chaque capteur
connecté suivant une loi linéaire G = a·V + b (par ex. un capteur de
température 0–5 V → 0–100 °C aurait a=20, b=0, unité "°C"), entrez les
valeurs dans la ligne du canal correspondant : l'effet est immédiat sur la
légende du graphique, qui affiche aussi la valeur convertie en temps réel à
côté du nom du canal (par ex. "A0 : 23.4 °C"). Les canaux non configurés
restent à l'identité (a=1, b=0, unité "V"), c'est-à-dire qu'ils affichent
simplement la tension. La description est une étiquette libre (par ex.
"Température four") : si elle est renseignée, elle devient le titre de
l'onglet dédié à ce canal dans le graphique (voir plus bas), sinon l'onglet
garde le nom du canal ("A0".."A5"). L'étalonnage n'est valable que pour la
session en cours (il n'est pas enregistré sur le disque) ; si un
enregistrement CSV est actif, le fichier inclut — en plus des six colonnes
`A#_V` en Volts — six colonnes supplémentaires `A#_conv[unité]` avec la
valeur déjà convertie, en utilisant l'étalonnage défini au moment où
Démarrer a été pressé (une modification effectuée alors qu'un
enregistrement est déjà en cours ne modifie ni l'en-tête ni les lignes déjà
écrites dans cette session).

### Graphique à 7 onglets (axes indépendants par canal)

Le graphique est un `wxNotebook` à 7 onglets. Le premier, "Tous", est le
graphique combiné habituel : les six courbes partagent un seul axe Y en
Volts, avec les cases à cocher pour masquer/afficher les canaux individuels
et l'état de l'enregistrement CSV (voyant + chemin du fichier). Les six
autres onglets (A0..A5) affichent chacun un seul canal, avec la courbe et
l'axe Y dans la grandeur *déjà convertie* selon son étalonnage (ou en Volts
si non étalonné) — un moyen simple d'avoir en pratique des "axes multiples"
lorsque les canaux mesurent des grandeurs de nature et d'échelle
différentes, sans avoir à superposer des unités incompatibles sur le même
axe. Le titre de chacun de ces six onglets est la description définie dans
la grille d'étalonnage (colonne "description"), ou le nom du canal si elle
n'est pas renseignée ; il change en temps réel à chaque modification de la
grille, sans besoin de redémarrer. Chaque onglet à canal unique a Auto Y
activé par défaut (car la plage de la grandeur convertie n'est pas connue à
l'avance) et ses propres boutons Auto-échelle/Suivre/Réinitialiser/PNG
indépendants des autres onglets ; l'export PNG enregistre toujours l'onglet
actuellement affiché. La fenêtre temporelle (Paramètres) s'applique à tous
les onglets ensemble, tandis que l'auto-échelle Y continue du panneau
Paramètres ne concerne que l'onglet combiné — les six onglets par canal
restent indépendants.

### Langue de l'interface

Depuis le menu Outils &gt; Paramètres, vous pouvez choisir la langue de
l'interface parmi Italiano, English, Français, Español et Deutsch. Le
choix est enregistré dans `%APPDATA%\AliveMonitor\settings.ini` (Windows)
ou `~/.config/AliveMonitor/settings.ini` (Linux) et nécessite un
redémarrage de l'application pour prendre effet : les widgets déjà
construits ne sont pas retraduits à chaud. Le tableau complet des
traductions se trouve dans `docs/Translations.md`.

## Documentation

La description complète de l'architecture, des choix de conception et des suggestions d'extension se trouve dans `docs/Architecture.md` ; la spécification du protocole série dans `docs/Protocol.md` ; le tableau des traductions de l'interface dans `docs/Translations.md` ; les diagrammes UML (classes et séquence, PlantUML + Mermaid) dans `docs/UML/` ; le guide pas à pas pour compiler avec CMake GUI (wxWidgets statique incluse) dans `docs/CMakeGUI.md`.

## Guide intégré

Depuis le menu Aide &gt; Guide (ou F1), un guide rapide d'utilisation de l'application s'ouvre dans le navigateur par défaut, dans la langue actuelle de l'interface, incorporé dans l'exécutable comme les autres ressources (icône comprise) : aucun fichier externe à distribuer avec le binaire.

## Licence

Distribué sous licence **MIT** : utilisation, modification et redistribution libres, y compris à des fins commerciales, fourni "tel quel" sans aucune garantie ni responsabilité des auteurs pour d'éventuels dommages résultant de l'utilisation du logiciel. Texte complet dans [`LICENSE`](LICENSE).
