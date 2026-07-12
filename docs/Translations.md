# Traduzioni dell'interfaccia (IT / EN / FR / ES / DE)

Tabella di riferimento per ogni `StringId` definito in `i18n/Strings.h` e
implementato in `i18n/Strings.cpp` (unica fonte di verità a runtime: questo
documento la descrive per revisione/manutenzione, non la sostituisce). Le
chiavi sono raggruppate per file sorgente d'origine, nello stesso ordine
dell'enum `StringId`.

Le stringhe con `%d`, `%s`, `%.1f` ecc. sono passate a `wxString::Format()`
dal chiamante: i placeholder vanno mantenuti (stesso tipo e stesso ordine)
in ogni lingua.

Stringhe **non tradotte deliberatamente** (non compaiono in tabella,
restano letterali nel codice sorgente): unità di misura e sigle tecniche
universali — V, Hz, ms, °C, FPS, RX, CRC, CPU, Baud, PNG — perché nessun
software tecnico le traduce in nessuna delle 5 lingue; ON/OFF (convenzione
universale per interruttori/relè); "a"/"b" nella griglia di calibrazione
(lettere di variabile matematica, G = a·V + b); il protocollo seriale
(`HELLO`, `ARDUINO_UNO`, comandi `GET`/`SET`...), contratto fisso col
firmware indipendente dalla lingua dell'interfaccia; i nomi propri nei
crediti (splash screen, about).

## ToolbarPanel

| Chiave | IT | EN | FR | ES | DE |
|---|---|---|---|---|---|
| TbSearching | Ricerca... | Searching... | Recherche... | Buscando... | Suche... |
| TbConnected | Connesso | Connected | Connecté | Conectado | Verbunden |
| TbDisconnected | Disconnesso | Disconnected | Déconnecté | Desconectado | Getrennt |
| TbPortPrefix | Porta: | Port: | Port : | Puerto: | Port: |
| TbFwPrefix | FW: | FW: | FW : | FW: | FW: |
| TbConnectBtn | Connetti | Connect | Connecter | Conectar | Verbinden |
| TbDisconnectBtn | Disconnetti | Disconnect | Déconnecter | Desconectar | Trennen |

## DigitalOutputPanel

| Chiave | IT | EN | FR | ES | DE |
|---|---|---|---|---|---|
| DoBoxTitle | Uscite digitali | Digital outputs | Sorties numériques | Salidas digitales | Digitale Ausgänge |
| DoOn | ON | ON | ON | ON | ON |
| DoOff | OFF | OFF | OFF | OFF | OFF |

## AcquisitionPanel

| Chiave | IT | EN | FR | ES | DE |
|---|---|---|---|---|---|
| AqBoxTitle | Acquisizione | Acquisition | Acquisition | Adquisición | Erfassung |
| AqStart | Start | Start | Démarrer | Iniciar | Start |
| AqPause | Pause | Pause | Pause | Pausa | Pause |
| AqStop | Stop | Stop | Arrêter | Detener | Stopp |
| AqCsvCheck | Registra CSV | Record CSV | Enregistrer CSV | Registrar CSV | CSV aufzeichnen |
| AqCsvTooltip | Se spuntata, Start chiede dove salvare la registrazione CSV. Se smarcata, Start avvia l'acquisizione senza chiedere né scrivere alcun file. | If checked, Start asks where to save the CSV recording. If unchecked, Start begins acquisition immediately, without asking or writing any file. | Si cochée, Démarrer demande où enregistrer le fichier CSV. Si décochée, l'acquisition démarre immédiatement, sans demander ni écrire aucun fichier. | Si está marcada, Iniciar pregunta dónde guardar el registro CSV. Si no lo está, la adquisición comienza de inmediato, sin preguntar ni escribir ningún archivo. | Falls aktiviert, fragt Start, wo die CSV-Aufzeichnung gespeichert werden soll. Falls deaktiviert, beginnt die Erfassung sofort, ohne Nachfrage und ohne eine Datei zu schreiben. |
| AqFreqLabel | Frequenza [Hz]: | Frequency [Hz]: | Fréquence [Hz] : | Frecuencia [Hz]: | Frequenz [Hz]: |
| AqSetRateFmt | Impostata: %d Hz (conferma FW: %s) | Set: %d Hz (FW confirms: %s) | Réglée : %d Hz (confirmation FW : %s) | Ajustada: %d Hz (confirmación FW: %s) | Eingestellt: %d Hz (FW-Bestätigung: %s) |
| AqMeasuredRateFmt | Misurata: %.1f Hz | Measured: %.1f Hz | Mesurée : %.1f Hz | Medida: %.1f Hz | Gemessen: %.1f Hz |
| AqMeasuredRateNone | Misurata: - Hz | Measured: - Hz | Mesurée : - Hz | Medida: - Hz | Gemessen: - Hz |
| AqSamplePeriodFmt | Campionamento: %.2f ms | Sampling: %.2f ms | Échantillonnage : %.2f ms | Muestreo: %.2f ms | Abtastung: %.2f ms |
| AqSamplePeriodNone | Campionamento: - ms | Sampling: - ms | Échantillonnage : - ms | Muestreo: - ms | Abtastung: - ms |

## StatusPanel

| Chiave | IT | EN | FR | ES | DE |
|---|---|---|---|---|---|
| StAcqFmt | Acq: %d Hz (eff. %.1f) | Acq: %d Hz (eff. %.1f) | Acq : %d Hz (eff. %.1f) | Adq: %d Hz (ef. %.1f) | Erf.: %d Hz (eff. %.1f) |
| StAcqFmtNoEff | Acq: %d Hz | Acq: %d Hz | Acq : %d Hz | Adq: %d Hz | Erf.: %d Hz |
| StLostFmt | Persi: %llu | Lost: %llu | Perdus : %llu | Perdidos: %llu | Verloren: %llu |
| StErrFmt | Err: %llu | Err: %llu | Err : %llu | Err: %llu | Fehl.: %llu |
| StConnPrefix | Conn: | Conn: | Conn : | Con: | Verb.: |
| StCpuFmt | CPU: %.1f%% | CPU: %.1f%% | CPU : %.1f%% | CPU: %.1f%% | CPU: %.1f%% |
| StCpuNone | CPU: n/d | CPU: n/a | CPU : n/d | CPU: n/d | CPU: k. A. |

## SettingsDialog

| Chiave | IT | EN | FR | ES | DE |
|---|---|---|---|---|---|
| SdTitle | Impostazioni | Settings | Paramètres | Configuración | Einstellungen |
| SdFpsLabel | FPS del grafico: | Graph FPS: | FPS du graphique : | FPS del gráfico: | Diagramm-FPS: |
| SdWindowLabel | Finestra temporale [s]: | Time window [s]: | Fenêtre temporelle [s] : | Ventana temporal [s]: | Zeitfenster [s]: |
| SdAutoYLabel | Autoscale Y continuo: | Continuous Y autoscale: | Autoéchelle Y continue : | Autoescala Y continua: | Fortlaufende Y-Autoskalierung: |
| SdLanguageLabel | Lingua: | Language: | Langue : | Idioma: | Sprache: |
| SdRestartNotice | Riavvia AliveMonitor per applicare la nuova lingua. | Restart AliveMonitor to apply the new language. | Redémarrez AliveMonitor pour appliquer la nouvelle langue. | Reinicia AliveMonitor para aplicar el nuevo idioma. | Starten Sie AliveMonitor neu, um die neue Sprache zu übernehmen. |

## SplashScreen

Stringhe stilistiche di branding (tagline), deliberatamente **identiche in
tutte le lingue** — erano già in inglese anche nella build italiana
originale, scelta grafica indipendente dalla lingua dell'interfaccia.

| Chiave | Tutte le lingue |
|---|---|
| SpSubtitle | Simple Control Software |
| SpVersionFmt | Version %s |
| SpCreditsFmt | by: %s *(i nomi propri non si traducono)* |

## CalibrationPanel

| Chiave | IT | EN | FR | ES | DE |
|---|---|---|---|---|---|
| CalBoxTitle | Calibrazione (G = a·V + b) | Calibration (G = a·V + b) | Étalonnage (G = a·V + b) | Calibración (G = a·V + b) | Kalibrierung (G = a·V + b) |
| CalColUnit | unità | unit | unité | unidad | Einheit |
| CalColDescription | descrizione | description | description | descripción | Beschreibung |

## GraphPanel

| Chiave | IT | EN | FR | ES | DE |
|---|---|---|---|---|---|
| GpTabAll | Tutti | All | Tous | Todos | Alle |
| GpAxisTime | Tempo [s] | Time [s] | Temps [s] | Tiempo [s] | Zeit [s] |
| GpCsvNone | CSV: nessuna registrazione | CSV: no recording | CSV : aucun enregistrement | CSV: sin registro | CSV: keine Aufzeichnung |
| GpCsvPrefix | CSV: | CSV: | CSV : | CSV: | CSV: |
| GpAutoYCheck | Auto Y | Auto Y | Auto Y | Auto Y | Auto Y |
| GpBtnAutoscale | Autoscala | Autoscale | Auto-échelle | Autoescala | Autoskalierung |
| GpBtnFollow | Segui | Follow | Suivre | Seguir | Folgen |
| GpBtnReset | Reset | Reset | Réinitialiser | Restablecer | Zurücksetzen |

Nota: il pulsante "PNG" (esportazione immagine) non è in tabella: resta
`"PNG"` non tradotto in tutte le lingue, come le altre sigle tecniche
universali.

## MainFrame — menu

| Chiave | IT | EN | FR | ES | DE |
|---|---|---|---|---|---|
| MfMenuFile | &File | &File | &Fichier | &Archivo | &Datei |
| MfMenuExportPng | Esporta grafico &PNG...\tCtrl+P | Export graph as &PNG...\tCtrl+P | Exporter le graphique en &PNG...\tCtrl+P | Exportar gráfico como &PNG...\tCtrl+P | Diagramm als &PNG exportieren...\tCtrl+P |
| MfMenuExportPngHelp | Salva il grafico corrente come immagine PNG | Save the current graph as a PNG image | Enregistre le graphique actuel au format PNG | Guarda el gráfico actual como imagen PNG | Speichert das aktuelle Diagramm als PNG-Bild |
| MfMenuExit | &Esci\tAlt+F4 | E&xit\tAlt+F4 | &Quitter\tAlt+F4 | &Salir\tAlt+F4 | &Beenden\tAlt+F4 |
| MfMenuTools | &Strumenti | &Tools | &Outils | &Herramientas | &Extras |
| MfMenuSettings | &Impostazioni...\tCtrl+, | &Settings...\tCtrl+, | &Paramètres...\tCtrl+, | &Configuración...\tCtrl+, | &Einstellungen...\tCtrl+, |
| MfMenuHelp | &Aiuto | &Help | &Aide | A&yuda | &Hilfe |
| MfMenuGuide | &Guida...\tF1 | &Guide...\tF1 | &Guide...\tF1 | &Guía...\tF1 | &Anleitung...\tF1 |
| MfMenuAbout | &Informazioni... | &About... | À &propos... | &Acerca de... | Übe&r... |
| MfGuideWriteError | Impossibile scrivere il file temporaneo della guida. | Unable to write the guide's temporary file. | Impossible d'écrire le fichier temporaire du guide. | No se pudo escribir el archivo temporal de la guía. | Die temporäre Datei der Anleitung konnte nicht geschrieben werden. |
| MfAboutDescription | Acquisizione dati analogici e controllo uscite digitali di Arduino Uno (o compatibile) via porta seriale. / wxWidgets | Analog data acquisition and digital output control for Arduino Uno (or compatible) via serial port. / wxWidgets | Acquisition de données analogiques et contrôle des sorties numériques d'un Arduino Uno (ou compatible) via port série. / wxWidgets | Adquisición de datos analógicos y control de salidas digitales de un Arduino Uno (o compatible) mediante puerto serie. / wxWidgets | Erfassung analoger Daten und Steuerung digitaler Ausgänge eines Arduino Uno (oder kompatibel) über die serielle Schnittstelle. / wxWidgets |
| MfAboutLicence | Licenza MIT: uso, modifica e ridistribuzione libera anche commerciale, fornito "così com'è" senza alcuna garanzia né responsabilità degli autori. Testo completo nel file LICENSE. | MIT License: free to use, modify and redistribute, including commercially, provided "as is" with no warranty and no liability for the authors. Full text in the LICENSE file. | Licence MIT : utilisation, modification et redistribution libres, y compris commerciales, fourni "tel quel" sans aucune garantie ni responsabilité des auteurs. Texte complet dans le fichier LICENSE. | Licencia MIT: uso, modificación y redistribución libres, incluso comerciales, proporcionado "tal cual" sin garantía ni responsabilidad de los autores. Texto completo en el archivo LICENSE. | MIT-Lizenz: freie Nutzung, Änderung und Weitergabe, auch kommerziell, bereitgestellt "wie besehen" ohne jegliche Gewährleistung oder Haftung der Autoren. Vollständiger Text in der Datei LICENSE. |

## MainController

| Chiave | IT | EN | FR | ES | DE |
|---|---|---|---|---|---|
| McSaveCsvDialogTitle | Salva registrazione CSV | Save CSV recording | Enregistrer le fichier CSV | Guardar registro CSV | CSV-Aufzeichnung speichern |
| McCsvOpenError | Impossibile aprire il file CSV per la scrittura. | Unable to open the CSV file for writing. | Impossible d'ouvrir le fichier CSV en écriture. | No se pudo abrir el archivo CSV para escritura. | Die CSV-Datei konnte nicht zum Schreiben geöffnet werden. |
| McExportPngDialogTitle | Esporta grafico come PNG | Export graph as PNG | Exporter le graphique en PNG | Exportar gráfico como PNG | Diagramm als PNG exportieren |
| McExportPngError | Esportazione PNG non riuscita. | PNG export failed. | L'export PNG a échoué. | La exportación a PNG falló. | PNG-Export fehlgeschlagen. |
| McCsvFilter | File CSV (*.csv)\|*.csv | CSV files (*.csv)\|*.csv | Fichiers CSV (*.csv)\|*.csv | Archivos CSV (*.csv)\|*.csv | CSV-Dateien (*.csv)\|*.csv |
| McPngFilter | Immagine PNG (*.png)\|*.png | PNG image (*.png)\|*.png | Image PNG (*.png)\|*.png | Imagen PNG (*.png)\|*.png | PNG-Bild (*.png)\|*.png |
| McErrPrefix | ERR: | ERR: | ERR : | ERR: | FEHLER: |

## SerialController

Chiamata dal thread seriale (non dal thread GUI): `tr()` è sicura da
chiamare da lì perché legge solo dato costante (la tabella) più un
`std::atomic<Language>` (vedi `i18n/Strings.h`).

| Chiave | IT | EN | FR | ES | DE |
|---|---|---|---|---|---|
| ScDisconnectedByUser | Disconnesso dall'utente | Disconnected by user | Déconnecté par l'utilisateur | Desconectado por el usuario | Vom Benutzer getrennt |
| ScSearching | Ricerca di Arduino... | Searching for Arduino... | Recherche d'Arduino... | Buscando Arduino... | Suche nach Arduino... |
| ScIoError | Errore I/O sulla porta seriale | I/O error on serial port | Erreur d'E/S sur le port série | Error de E/S en el puerto serie | E/A-Fehler auf der seriellen Schnittstelle |
| ScTimeout | Nessuna risposta da Arduino (timeout) | No response from Arduino (timeout) | Aucune réponse d'Arduino (délai dépassé) | Sin respuesta de Arduino (tiempo agotado) | Keine Antwort von Arduino (Zeitüberschreitung) |
| ScWriteError | Errore di scrittura sulla porta seriale | Write error on serial port | Erreur d'écriture sur le port série | Error de escritura en el puerto serie | Schreibfehler auf der seriellen Schnittstelle |

## Nomi nativi delle lingue (`nativeLanguageName`)

Usati nel selettore di `SettingsDialog`: sempre nella lingua stessa, mai
tradotti (così restano leggibili anche a chi non capisce la lingua
correntemente attiva).

| Language | Nome nativo |
|---|---|
| IT | Italiano |
| EN | English |
| FR | Français |
| ES | Español |
| DE | Deutsch |

## Guida in-app (resources/HelpContent.h)

La guida HTML completa (Aiuto &gt; Guida / F1) non passa dalla tabella
`StringId`/`tr()` sopra: è un documento lungo e strutturato, mantenuto come
5 costanti HTML separate e complete (`kHelpHtmlIt`, `kHelpHtmlEn`,
`kHelpHtmlFr`, `kHelpHtmlEs`, `kHelpHtmlDe`) selezionate da
`helpHtmlFor(Language)`. Stesso contenuto informativo in ogni lingua,
tradotto integralmente; vedi il file sorgente per il testo completo.
