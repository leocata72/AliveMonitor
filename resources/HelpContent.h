/**
 * @file HelpContent.h
 * @brief Guida utente incorporata (HTML), mostrata dal menu Aiuto > Guida.
 *
 * Come per l'icona (resources/AliveMonitor.xpm), il contenuto è incorporato
 * direttamente nell'eseguibile come stringa C++: nessun file esterno da
 * distribuire insieme al binario. MainFrame lo scrive in un file temporaneo
 * e lo apre con il browser predefinito (wxLaunchDefaultBrowser), quindi non
 * serve alcun componente wxHTML: la resa è quella del browser dell'utente.
 *
 * Una variante HTML completa per ciascuna delle 5 lingue supportate (vedi
 * Language.h): helpHtmlFor(lang) restituisce quella corrispondente alla
 * lingua corrente dell'interfaccia. Il CSS è identico in tutte le varianti
 * (solo il testo cambia): duplicarlo per lingua, anziché fattorizzarlo,
 * mantiene ogni variante un unico blocco autonomo, più semplice da
 * rileggere/tradurre di nuovo in futuro senza dover ricostruire il
 * documento a pezzi.
 */
#pragma once

#include "Language.h"

namespace am {

inline constexpr const char* kHelpHtmlIt = R"HTML(<!DOCTYPE html>
<html lang="it">
<head>
<meta charset="utf-8">
<title>AliveMonitor &mdash; Guida</title>
<style>
  body { font-family: -apple-system, Segoe UI, Helvetica, Arial, sans-serif;
         max-width: 760px; margin: 2rem auto; padding: 0 1.2rem;
         color: #222; line-height: 1.55; }
  h1 { border-bottom: 3px solid #1f77b4; padding-bottom: .3rem; }
  h2 { color: #1f77b4; margin-top: 2.2rem; border-bottom: 1px solid #ddd;
       padding-bottom: .2rem; }
  code { background: #f2f2f2; padding: .1rem .35rem; border-radius: 3px;
         font-size: .92em; }
  pre { background: #f2f2f2; padding: .6rem .9rem; border-radius: 5px;
        overflow-x: auto; font-size: .85em; }
  pre code { background: none; padding: 0; }
  .led { display:inline-block; width:.8em; height:.8em; border-radius:50%;
         margin-right:.3em; vertical-align:middle; }
  .verde { background:#2ecc40; } .rosso { background:#c82828; }
  ul { padding-left: 1.3rem; }
  .nota { background:#fff8e1; border-left: 4px solid #f0ad4e;
          padding:.6rem 1rem; margin: 1rem 0; }
  footer { margin-top: 3rem; padding-top: 1rem; border-top: 1px solid #ddd;
           color: #777; font-size: .9em; }
</style>
</head>
<body>

<h1>AliveMonitor &mdash; Guida rapida</h1>
<p>Applicazione desktop per l'acquisizione in tempo reale delle tensioni
analogiche A0&ndash;A5 e il controllo delle uscite digitali D2&ndash;D9 di
una scheda Arduino Uno (o compatibile) collegata via USB, con firmware
dedicato incluso.</p>

<h2>1. Prima di tutto: carica il firmware sulla scheda</h2>
<p>Prima di collegare la scheda all'applicazione, va caricato lo sketch
<code>AliveMonitor.ino</code> con Arduino IDE o <code>arduino-cli</code>.
Si trova nella cartella <code>arduino\AliveMonitor\</code>: accanto
all'eseguibile se hai installato AliveMonitor con il setup (raggiungibile
anche dal menu Avvio, voce "Cartella firmware Arduino"), oppure nella
cartella del repository se hai compilato AliveMonitor dai sorgenti. Senza
firmware la scheda non risponde all'handshake e l'applicazione non la
troverà mai, per quanto sia collegata correttamente via USB.</p>
<pre><code>arduino-cli compile --fqbn arduino:avr:uno AliveMonitor.ino
arduino-cli upload  --fqbn arduino:avr:uno -p /dev/ttyACM0 AliveMonitor.ino</code></pre>
<p>Fatto questo passo una sola volta (il firmware resta sulla scheda anche
spegnendola), si può passare alla connessione.</p>

<h2>2. Connessione</h2>
<p>All'avvio l'applicazione scandisce automaticamente le porte seriali
disponibili e si connette alla prima scheda che risponde correttamente
all'handshake del firmware. Il LED in alto a sinistra indica lo stato:
<span class="led verde"></span>connessa, <span class="led rosso"></span>
in ricerca. Se la scheda viene scollegata, la ricerca riprende da sola e,
alla riconnessione, uscite digitali e streaming vengono ripristinati
automaticamente.</p>

<h2>3. Uscite digitali</h2>
<p>I pulsanti D2&ndash;D9 comandano le uscite digitali della scheda. Il LED
accanto a ogni pulsante riflette lo stato <em>reale</em> confermato dal
firmware, non solo quello richiesto: se il comando non arriva a
destinazione (scheda scollegata, errore seriale) il LED non cambia.</p>

<h2>4. Acquisizione</h2>
<p><b>Start</b> avvia lo streaming delle sei tensioni analogiche alla
frequenza impostata (1&ndash;250&nbsp;Hz, modificabile anche a
acquisizione in corso). <b>Pause</b> sospende conservando i dati già
acquisiti; <b>Stop</b> ferma e azzera il buffer. La checkbox "Registra
CSV" sotto questi tre pulsanti, se spuntata (default), fa comparire alla
pressione di Start una finestra per scegliere dove salvare la
registrazione &mdash; vedi "Registrazione CSV" più sotto.</p>

<h2>5. Il grafico a 7 schede</h2>
<p>Il grafico è organizzato in 7 schede. La prima, <b>"Tutti"</b>, mostra
le sei curve sovrapposte in Volt, con checkbox per nascondere i singoli
canali. Le altre sei schede (una per canale) mostrano una sola curva
ciascuna, con l'asse Y nella <em>grandezza fisica convertita</em> secondo
la calibrazione del canale (vedi sotto) &mdash; comodo per confrontare
grandezze diverse (temperatura, pressione, ...) ognuna sulla propria
scala, senza doverle sovrapporre su un unico asse in Volt.</p>
<p>In ogni scheda: rotella del mouse = zoom sul tempo, Ctrl+rotella = zoom
sull'asse Y, trascinamento = pan. I pulsanti <b>Segui</b> riaggancia la
finestra scorrevole al tempo corrente, <b>Autoscala</b> adatta una volta
sola l'asse Y ai dati visibili, <b>Reset</b> ripristina la vista
predefinita, <b>PNG</b> esporta la scheda attualmente visualizzata come
immagine.</p>

<h2>6. Calibrazione dei canali (Volt &rarr; grandezza fisica)</h2>
<p>Nel pannello a sinistra, sotto i controlli di acquisizione, la griglia
di calibrazione permette di associare a ciascun canale A0&ndash;A5 un
trasduttore con legge lineare <code>G = a&middot;V + b</code>: inserire il
coefficiente <code>a</code>, l'offset <code>b</code>, l'unità di misura e,
opzionalmente, una descrizione libera (es. "Temperatura forno"). Se
compilata, la descrizione diventa il titolo della scheda dedicata a quel
canale nel grafico; l'effetto sulla legenda e sull'asse Y è immediato. I
canali non configurati restano all'identità (<code>a=1, b=0</code>, unità
"V"), cioè mostrano semplicemente la tensione.</p>
<div class="nota">La calibrazione vale solo per la sessione corrente
dell'applicazione: non viene salvata su disco, va reinserita a ogni
riavvio.</div>

<h2>7. Registrazione CSV</h2>
<p>Con la registrazione attiva, ogni campione ricevuto viene scritto su
file in tempo reale senza mai bloccare l'acquisizione. Il file contiene,
oltre al tempo e alle sei tensioni in Volt (<code>A0_V</code>&hellip;
<code>A5_V</code>), sei colonne aggiuntive con il valore già convertito e
la sua unità nel nome colonna (es. <code>A0_conv[&deg;C]</code>), usando
la calibrazione impostata al momento dello Start. Un LED e un'etichetta
nella barra del grafico combinato mostrano lo stato della registrazione:
<span class="led verde"></span>in scrittura, <span class="led rosso">
</span>ferma.</p>

<h2>8. Impostazioni</h2>
<p>Dal menu Strumenti &gt; Impostazioni si regolano gli FPS di rendering
del grafico, l'ampiezza della finestra temporale visibile (applicata a
tutte le 7 schede), l'autoscale Y continuo della scheda combinata (le sei
schede per canale hanno un proprio Auto Y indipendente, già attivo di
default) e la lingua dell'interfaccia (Italiano, English, Français,
Español, Deutsch) &mdash; quest'ultima richiede il riavvio
dell'applicazione per avere effetto.</p>

<h2>Licenza</h2>
<p>AliveMonitor è distribuito sotto licenza <b>MIT</b>: libero da usare,
modificare e ridistribuire, anche a scopo commerciale, <b>senza alcuna
garanzia né responsabilità degli autori</b> per eventuali danni derivanti
dall'uso del software. Testo completo nel file <code>LICENSE</code> incluso
nel repository.</p>

<footer>
AliveMonitor &mdash; sviluppato da Leonardo Catalano con l'assistenza di
Claude (Anthropic). Basato su C++20 e wxWidgets.
</footer>

</body>
</html>
)HTML";

inline constexpr const char* kHelpHtmlEn = R"HTML(<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="utf-8">
<title>AliveMonitor &mdash; Guide</title>
<style>
  body { font-family: -apple-system, Segoe UI, Helvetica, Arial, sans-serif;
         max-width: 760px; margin: 2rem auto; padding: 0 1.2rem;
         color: #222; line-height: 1.55; }
  h1 { border-bottom: 3px solid #1f77b4; padding-bottom: .3rem; }
  h2 { color: #1f77b4; margin-top: 2.2rem; border-bottom: 1px solid #ddd;
       padding-bottom: .2rem; }
  code { background: #f2f2f2; padding: .1rem .35rem; border-radius: 3px;
         font-size: .92em; }
  pre { background: #f2f2f2; padding: .6rem .9rem; border-radius: 5px;
        overflow-x: auto; font-size: .85em; }
  pre code { background: none; padding: 0; }
  .led { display:inline-block; width:.8em; height:.8em; border-radius:50%;
         margin-right:.3em; vertical-align:middle; }
  .verde { background:#2ecc40; } .rosso { background:#c82828; }
  ul { padding-left: 1.3rem; }
  .nota { background:#fff8e1; border-left: 4px solid #f0ad4e;
          padding:.6rem 1rem; margin: 1rem 0; }
  footer { margin-top: 3rem; padding-top: 1rem; border-top: 1px solid #ddd;
           color: #777; font-size: .9em; }
</style>
</head>
<body>

<h1>AliveMonitor &mdash; Quick Guide</h1>
<p>Desktop application for real-time acquisition of the six analog
voltages A0&ndash;A5 and control of the digital outputs D2&ndash;D9 of an
Arduino Uno (or compatible) board connected via USB, with a dedicated
firmware sketch included.</p>

<h2>1. First things first: flash the firmware onto the board</h2>
<p>Before connecting the board to the application, the
<code>AliveMonitor.ino</code> sketch must be uploaded using the Arduino
IDE or <code>arduino-cli</code>. It is found in the
<code>arduino\AliveMonitor\</code> folder: next to the executable if you
installed AliveMonitor with the setup (also reachable from the Start
menu, "Arduino firmware folder" entry), or in the repository folder if
you built AliveMonitor from source. Without the firmware the board will
never respond to the handshake and the application will never find it,
no matter how correctly it is connected via USB.</p>
<pre><code>arduino-cli compile --fqbn arduino:avr:uno AliveMonitor.ino
arduino-cli upload  --fqbn arduino:avr:uno -p /dev/ttyACM0 AliveMonitor.ino</code></pre>
<p>Once this step is done (the firmware stays on the board even after
powering it off), you can move on to connecting.</p>

<h2>2. Connection</h2>
<p>On startup the application automatically scans the available serial
ports and connects to the first board that correctly responds to the
firmware handshake. The LED in the top-left corner shows the status:
<span class="led verde"></span>connected, <span class="led rosso"></span>
searching. If the board is disconnected, the search resumes on its own
and, on reconnection, digital outputs and streaming are restored
automatically.</p>

<h2>3. Digital outputs</h2>
<p>The D2&ndash;D9 buttons control the board's digital outputs. The LED
next to each button reflects the <em>actual</em> state confirmed by the
firmware, not just the requested one: if the command fails to reach its
destination (board disconnected, serial error) the LED does not
change.</p>

<h2>4. Acquisition</h2>
<p><b>Start</b> begins streaming the six analog voltages at the
configured rate (1&ndash;250&nbsp;Hz, adjustable even while acquisition
is running). <b>Pause</b> suspends it while keeping the data already
acquired; <b>Stop</b> stops it and clears the buffer. The "Record CSV"
checkbox below these three buttons, when checked (the default), makes a
window appear when Start is pressed to choose where to save the
recording &mdash; see "CSV recording" below.</p>

<h2>5. The 7-tab graph</h2>
<p>The graph is organized into 7 tabs. The first, <b>"All"</b>, shows all
six curves overlaid in Volts, with checkboxes to hide individual
channels. The other six tabs (one per channel) each show a single curve,
with the Y axis in the <em>converted physical quantity</em> according to
the channel's calibration (see below) &mdash; convenient for comparing
different quantities (temperature, pressure, ...) each on its own scale,
without having to overlay them on a single Volt axis.</p>
<p>In every tab: mouse wheel = zoom on time, Ctrl+wheel = zoom on the Y
axis, drag = pan. The <b>Follow</b> button re-attaches the scrolling
window to the current time, <b>Autoscale</b> adapts the Y axis once to
the visible data, <b>Reset</b> restores the default view, <b>PNG</b>
exports the currently displayed tab as an image.</p>

<h2>6. Channel calibration (Volts &rarr; physical quantity)</h2>
<p>In the left panel, below the acquisition controls, the calibration
grid lets you associate each A0&ndash;A5 channel with a transducer
following the linear law <code>G = a&middot;V + b</code>: enter the
coefficient <code>a</code>, the offset <code>b</code>, the unit of
measurement and, optionally, a free-text description (e.g. "Oven
temperature"). If filled in, the description becomes the title of that
channel's dedicated tab in the graph; the effect on the legend and the Y
axis is immediate. Unconfigured channels stay at the identity
(<code>a=1, b=0</code>, unit "V"), i.e. they simply show the voltage.</p>
<div class="nota">Calibration is only valid for the current application
session: it is not saved to disk and must be re-entered at every
restart.</div>

<h2>7. CSV recording</h2>
<p>While recording is active, every sample received is written to file in
real time without ever blocking acquisition. Besides the time and the six
voltages in Volts (<code>A0_V</code>&hellip;<code>A5_V</code>), the file
contains six additional columns with the already-converted value and its
unit in the column name (e.g. <code>A0_conv[&deg;C]</code>), using the
calibration set at the moment Start was pressed. An LED and a label in
the combined graph's toolbar show the recording status:
<span class="led verde"></span>writing, <span class="led rosso">
</span>stopped.</p>

<h2>8. Settings</h2>
<p>From the Tools &gt; Settings menu you can adjust the graph's rendering
FPS, the width of the visible time window (applied to all 7 tabs), the
combined tab's continuous Y autoscale (the six per-channel tabs have
their own independent Auto Y, already enabled by default), and the
interface language (Italiano, English, Français, Español, Deutsch)
&mdash; the latter requires restarting the application to take
effect.</p>

<h2>License</h2>
<p>AliveMonitor is distributed under the <b>MIT</b> license: free to use,
modify and redistribute, including commercially, <b>with no warranty and
no liability for the authors</b> for any damages arising from the use of
the software. Full text in the <code>LICENSE</code> file included in the
repository.</p>

<footer>
AliveMonitor &mdash; developed by Leonardo Catalano with the assistance of
Claude (Anthropic). Built with C++20 and wxWidgets.
</footer>

</body>
</html>
)HTML";

inline constexpr const char* kHelpHtmlFr = R"HTML(<!DOCTYPE html>
<html lang="fr">
<head>
<meta charset="utf-8">
<title>AliveMonitor &mdash; Guide</title>
<style>
  body { font-family: -apple-system, Segoe UI, Helvetica, Arial, sans-serif;
         max-width: 760px; margin: 2rem auto; padding: 0 1.2rem;
         color: #222; line-height: 1.55; }
  h1 { border-bottom: 3px solid #1f77b4; padding-bottom: .3rem; }
  h2 { color: #1f77b4; margin-top: 2.2rem; border-bottom: 1px solid #ddd;
       padding-bottom: .2rem; }
  code { background: #f2f2f2; padding: .1rem .35rem; border-radius: 3px;
         font-size: .92em; }
  pre { background: #f2f2f2; padding: .6rem .9rem; border-radius: 5px;
        overflow-x: auto; font-size: .85em; }
  pre code { background: none; padding: 0; }
  .led { display:inline-block; width:.8em; height:.8em; border-radius:50%;
         margin-right:.3em; vertical-align:middle; }
  .verde { background:#2ecc40; } .rosso { background:#c82828; }
  ul { padding-left: 1.3rem; }
  .nota { background:#fff8e1; border-left: 4px solid #f0ad4e;
          padding:.6rem 1rem; margin: 1rem 0; }
  footer { margin-top: 3rem; padding-top: 1rem; border-top: 1px solid #ddd;
           color: #777; font-size: .9em; }
</style>
</head>
<body>

<h1>AliveMonitor &mdash; Guide rapide</h1>
<p>Application de bureau pour l'acquisition en temps réel des six tensions
analogiques A0&ndash;A5 et le contrôle des sorties numériques D2&ndash;D9
d'une carte Arduino Uno (ou compatible) connectée en USB, avec un
firmware dédié inclus.</p>

<h2>1. Avant tout : chargez le firmware sur la carte</h2>
<p>Avant de connecter la carte à l'application, il faut charger le sketch
<code>AliveMonitor.ino</code> avec l'IDE Arduino ou
<code>arduino-cli</code>. Il se trouve dans le dossier
<code>arduino\AliveMonitor\</code> : à côté de l'exécutable si vous avez
installé AliveMonitor avec le programme d'installation (également
accessible depuis le menu Démarrer, entrée "Dossier firmware Arduino"),
ou dans le dossier du dépôt si vous avez compilé AliveMonitor depuis les
sources. Sans le firmware, la carte ne répondra jamais à la négociation
et l'application ne la trouvera jamais, même si elle est correctement
connectée en USB.</p>
<pre><code>arduino-cli compile --fqbn arduino:avr:uno AliveMonitor.ino
arduino-cli upload  --fqbn arduino:avr:uno -p /dev/ttyACM0 AliveMonitor.ino</code></pre>
<p>Une fois cette étape effectuée (le firmware reste sur la carte même
après l'avoir éteinte), vous pouvez passer à la connexion.</p>

<h2>2. Connexion</h2>
<p>Au démarrage, l'application analyse automatiquement les ports série
disponibles et se connecte à la première carte qui répond correctement à
la négociation du firmware. Le voyant en haut à gauche indique l'état :
<span class="led verde"></span>connectée, <span class="led rosso"></span>
recherche en cours. Si la carte est déconnectée, la recherche reprend
d'elle-même et, à la reconnexion, les sorties numériques et le streaming
sont rétablis automatiquement.</p>

<h2>3. Sorties numériques</h2>
<p>Les boutons D2&ndash;D9 commandent les sorties numériques de la carte.
Le voyant à côté de chaque bouton reflète l'état <em>réel</em> confirmé
par le firmware, pas seulement celui demandé : si la commande n'atteint
pas sa destination (carte déconnectée, erreur série), le voyant ne
change pas.</p>

<h2>4. Acquisition</h2>
<p><b>Démarrer</b> lance le streaming des six tensions analogiques à la
fréquence configurée (1&ndash;250&nbsp;Hz, modifiable même pendant
l'acquisition). <b>Pause</b> la suspend en conservant les données déjà
acquises ; <b>Arrêter</b> l'arrête et vide le tampon. La case "Enregistrer
CSV" sous ces trois boutons, si cochée (par défaut), fait apparaître à
l'appui sur Démarrer une fenêtre pour choisir où enregistrer le fichier
&mdash; voir "Enregistrement CSV" plus bas.</p>

<h2>5. Le graphique à 7 onglets</h2>
<p>Le graphique est organisé en 7 onglets. Le premier, <b>"Tous"</b>,
affiche les six courbes superposées en Volts, avec des cases à cocher
pour masquer les canaux individuels. Les six autres onglets (un par
canal) affichent chacun une seule courbe, avec l'axe Y dans la
<em>grandeur physique convertie</em> selon l'étalonnage du canal (voir
plus bas) &mdash; pratique pour comparer des grandeurs différentes
(température, pression, ...) chacune sur sa propre échelle, sans avoir à
les superposer sur un seul axe en Volts.</p>
<p>Dans chaque onglet : molette de la souris = zoom sur le temps,
Ctrl+molette = zoom sur l'axe Y, glisser = déplacement. Le bouton
<b>Suivre</b> raccroche la fenêtre défilante au temps courant,
<b>Auto-échelle</b> adapte une fois l'axe Y aux données visibles,
<b>Réinitialiser</b> restaure la vue par défaut, <b>PNG</b> exporte
l'onglet actuellement affiché en image.</p>

<h2>6. Étalonnage des canaux (Volts &rarr; grandeur physique)</h2>
<p>Dans le panneau de gauche, sous les contrôles d'acquisition, la grille
d'étalonnage permet d'associer à chaque canal A0&ndash;A5 un capteur
suivant la loi linéaire <code>G = a&middot;V + b</code> : saisissez le
coefficient <code>a</code>, l'offset <code>b</code>, l'unité de mesure
et, en option, une description libre (ex. "Température four"). Si elle
est renseignée, la description devient le titre de l'onglet dédié à ce
canal dans le graphique ; l'effet sur la légende et sur l'axe Y est
immédiat. Les canaux non configurés restent à l'identité
(<code>a=1, b=0</code>, unité "V"), c'est-à-dire qu'ils affichent
simplement la tension.</p>
<div class="nota">L'étalonnage n'est valable que pour la session en
cours de l'application : il n'est pas enregistré sur le disque et doit
être ressaisi à chaque redémarrage.</div>

<h2>7. Enregistrement CSV</h2>
<p>Lorsque l'enregistrement est actif, chaque échantillon reçu est écrit
dans le fichier en temps réel sans jamais bloquer l'acquisition. Le
fichier contient, en plus du temps et des six tensions en Volts
(<code>A0_V</code>&hellip;<code>A5_V</code>), six colonnes
supplémentaires avec la valeur déjà convertie et son unité dans le nom de
colonne (ex. <code>A0_conv[&deg;C]</code>), en utilisant l'étalonnage
défini au moment où Démarrer a été pressé. Un voyant et une étiquette
dans la barre de l'onglet combiné indiquent l'état de l'enregistrement :
<span class="led verde"></span>en écriture, <span class="led rosso">
</span>arrêté.</p>

<h2>8. Paramètres</h2>
<p>Depuis le menu Outils &gt; Paramètres, vous réglez les FPS de rendu du
graphique, la largeur de la fenêtre temporelle visible (appliquée aux 7
onglets), l'auto-échelle Y continue de l'onglet combiné (les six onglets
par canal ont leur propre Auto Y indépendant, déjà activé par défaut) et
la langue de l'interface (Italiano, English, Français, Español, Deutsch)
&mdash; ce dernier réglage nécessite un redémarrage de l'application pour
prendre effet.</p>

<h2>Licence</h2>
<p>AliveMonitor est distribué sous licence <b>MIT</b> : libre
d'utilisation, de modification et de redistribution, y compris à des
fins commerciales, <b>sans aucune garantie ni responsabilité des
auteurs</b> pour les dommages éventuels résultant de l'utilisation du
logiciel. Texte complet dans le fichier <code>LICENSE</code> inclus dans
le dépôt.</p>

<footer>
AliveMonitor &mdash; développé par Leonardo Catalano avec l'assistance de
Claude (Anthropic). Basé sur C++20 et wxWidgets.
</footer>

</body>
</html>
)HTML";

inline constexpr const char* kHelpHtmlEs = R"HTML(<!DOCTYPE html>
<html lang="es">
<head>
<meta charset="utf-8">
<title>AliveMonitor &mdash; Guía</title>
<style>
  body { font-family: -apple-system, Segoe UI, Helvetica, Arial, sans-serif;
         max-width: 760px; margin: 2rem auto; padding: 0 1.2rem;
         color: #222; line-height: 1.55; }
  h1 { border-bottom: 3px solid #1f77b4; padding-bottom: .3rem; }
  h2 { color: #1f77b4; margin-top: 2.2rem; border-bottom: 1px solid #ddd;
       padding-bottom: .2rem; }
  code { background: #f2f2f2; padding: .1rem .35rem; border-radius: 3px;
         font-size: .92em; }
  pre { background: #f2f2f2; padding: .6rem .9rem; border-radius: 5px;
        overflow-x: auto; font-size: .85em; }
  pre code { background: none; padding: 0; }
  .led { display:inline-block; width:.8em; height:.8em; border-radius:50%;
         margin-right:.3em; vertical-align:middle; }
  .verde { background:#2ecc40; } .rosso { background:#c82828; }
  ul { padding-left: 1.3rem; }
  .nota { background:#fff8e1; border-left: 4px solid #f0ad4e;
          padding:.6rem 1rem; margin: 1rem 0; }
  footer { margin-top: 3rem; padding-top: 1rem; border-top: 1px solid #ddd;
           color: #777; font-size: .9em; }
</style>
</head>
<body>

<h1>AliveMonitor &mdash; Guía rápida</h1>
<p>Aplicación de escritorio para la adquisición en tiempo real de las seis
tensiones analógicas A0&ndash;A5 y el control de las salidas digitales
D2&ndash;D9 de una placa Arduino Uno (o compatible) conectada por USB,
con firmware dedicado incluido.</p>

<h2>1. Antes de nada: carga el firmware en la placa</h2>
<p>Antes de conectar la placa a la aplicación, hay que cargar el sketch
<code>AliveMonitor.ino</code> con el IDE de Arduino o
<code>arduino-cli</code>. Se encuentra en la carpeta
<code>arduino\AliveMonitor\</code>: junto al ejecutable si instalaste
AliveMonitor con el instalador (también accesible desde el menú Inicio,
entrada "Carpeta de firmware Arduino"), o en la carpeta del repositorio
si compilaste AliveMonitor desde el código fuente. Sin el firmware, la
placa nunca responderá al protocolo de enlace y la aplicación nunca la
encontrará, por muy bien conectada que esté por USB.</p>
<pre><code>arduino-cli compile --fqbn arduino:avr:uno AliveMonitor.ino
arduino-cli upload  --fqbn arduino:avr:uno -p /dev/ttyACM0 AliveMonitor.ino</code></pre>
<p>Una vez hecho este paso (el firmware permanece en la placa incluso
apagándola), se puede pasar a la conexión.</p>

<h2>2. Conexión</h2>
<p>Al iniciarse, la aplicación explora automáticamente los puertos serie
disponibles y se conecta a la primera placa que responda correctamente al
protocolo de enlace del firmware. El LED de la esquina superior izquierda
indica el estado: <span class="led verde"></span>conectada,
<span class="led rosso"></span>buscando. Si la placa se desconecta, la
búsqueda se reanuda sola y, al reconectarse, las salidas digitales y el
streaming se restauran automáticamente.</p>

<h2>3. Salidas digitales</h2>
<p>Los botones D2&ndash;D9 controlan las salidas digitales de la placa. El
LED junto a cada botón refleja el estado <em>real</em> confirmado por el
firmware, no solo el solicitado: si el comando no llega a su destino
(placa desconectada, error de puerto serie) el LED no cambia.</p>

<h2>4. Adquisición</h2>
<p><b>Iniciar</b> comienza el streaming de las seis tensiones analógicas a
la frecuencia configurada (1&ndash;250&nbsp;Hz, modificable incluso con la
adquisición en curso). <b>Pausa</b> la suspende conservando los datos ya
adquiridos; <b>Detener</b> la para y vacía el búfer. La casilla "Registrar
CSV" bajo estos tres botones, si está marcada (por defecto), hace
aparecer al pulsar Iniciar una ventana para elegir dónde guardar el
registro &mdash; ver "Registro CSV" más abajo.</p>

<h2>5. El gráfico de 7 pestañas</h2>
<p>El gráfico está organizado en 7 pestañas. La primera, <b>"Todos"</b>,
muestra las seis curvas superpuestas en Voltios, con casillas para ocultar
canales individuales. Las otras seis pestañas (una por canal) muestran
cada una una sola curva, con el eje Y en la <em>magnitud física
convertida</em> según la calibración del canal (ver más abajo) &mdash;
cómodo para comparar magnitudes distintas (temperatura, presión, ...)
cada una en su propia escala, sin tener que superponerlas en un único eje
en Voltios.</p>
<p>En cada pestaña: rueda del ratón = zoom en el tiempo, Ctrl+rueda = zoom
en el eje Y, arrastrar = desplazar. El botón <b>Seguir</b> vuelve a
enganchar la ventana desplazable al tiempo actual, <b>Autoescala</b>
adapta una vez el eje Y a los datos visibles, <b>Restablecer</b> restaura
la vista predeterminada, <b>PNG</b> exporta la pestaña mostrada
actualmente como imagen.</p>

<h2>6. Calibración de los canales (Voltios &rarr; magnitud física)</h2>
<p>En el panel izquierdo, bajo los controles de adquisición, la rejilla de
calibración permite asociar a cada canal A0&ndash;A5 un transductor con
ley lineal <code>G = a&middot;V + b</code>: introduce el coeficiente
<code>a</code>, el offset <code>b</code>, la unidad de medida y,
opcionalmente, una descripción libre (p. ej. "Temperatura horno"). Si se
rellena, la descripción pasa a ser el título de la pestaña dedicada a ese
canal en el gráfico; el efecto sobre la leyenda y el eje Y es inmediato.
Los canales no configurados permanecen en la identidad (<code>a=1,
b=0</code>, unidad "V"), es decir, muestran simplemente la tensión.</p>
<div class="nota">La calibración solo es válida para la sesión actual de
la aplicación: no se guarda en disco y hay que volver a introducirla en
cada reinicio.</div>

<h2>7. Registro CSV</h2>
<p>Con el registro activo, cada muestra recibida se escribe en el archivo
en tiempo real sin bloquear nunca la adquisición. Además del tiempo y las
seis tensiones en Voltios (<code>A0_V</code>&hellip;<code>A5_V</code>),
el archivo contiene seis columnas adicionales con el valor ya convertido
y su unidad en el nombre de columna (p. ej. <code>A0_conv[&deg;C]</code>),
usando la calibración establecida en el momento de pulsar Iniciar. Un LED
y una etiqueta en la barra del gráfico combinado muestran el estado del
registro: <span class="led verde"></span>escribiendo, <span
class="led rosso"></span>detenido.</p>

<h2>8. Configuración</h2>
<p>Desde el menú Herramientas &gt; Configuración se ajustan los FPS de
renderizado del gráfico, el ancho de la ventana temporal visible
(aplicada a las 7 pestañas), la autoescala Y continua de la pestaña
combinada (las seis pestañas por canal tienen su propio Auto Y
independiente, ya activado por defecto) y el idioma de la interfaz
(Italiano, English, Français, Español, Deutsch) &mdash; este último
requiere reiniciar la aplicación para surtir efecto.</p>

<h2>Licencia</h2>
<p>AliveMonitor se distribuye bajo licencia <b>MIT</b>: libre de usar,
modificar y redistribuir, incluso con fines comerciales, <b>sin ninguna
garantía ni responsabilidad de los autores</b> por los daños que pudieran
derivarse del uso del software. Texto completo en el archivo
<code>LICENSE</code> incluido en el repositorio.</p>

<footer>
AliveMonitor &mdash; desarrollado por Leonardo Catalano con la asistencia
de Claude (Anthropic). Basado en C++20 y wxWidgets.
</footer>

</body>
</html>
)HTML";

inline constexpr const char* kHelpHtmlDe = R"HTML(<!DOCTYPE html>
<html lang="de">
<head>
<meta charset="utf-8">
<title>AliveMonitor &mdash; Anleitung</title>
<style>
  body { font-family: -apple-system, Segoe UI, Helvetica, Arial, sans-serif;
         max-width: 760px; margin: 2rem auto; padding: 0 1.2rem;
         color: #222; line-height: 1.55; }
  h1 { border-bottom: 3px solid #1f77b4; padding-bottom: .3rem; }
  h2 { color: #1f77b4; margin-top: 2.2rem; border-bottom: 1px solid #ddd;
       padding-bottom: .2rem; }
  code { background: #f2f2f2; padding: .1rem .35rem; border-radius: 3px;
         font-size: .92em; }
  pre { background: #f2f2f2; padding: .6rem .9rem; border-radius: 5px;
        overflow-x: auto; font-size: .85em; }
  pre code { background: none; padding: 0; }
  .led { display:inline-block; width:.8em; height:.8em; border-radius:50%;
         margin-right:.3em; vertical-align:middle; }
  .verde { background:#2ecc40; } .rosso { background:#c82828; }
  ul { padding-left: 1.3rem; }
  .nota { background:#fff8e1; border-left: 4px solid #f0ad4e;
          padding:.6rem 1rem; margin: 1rem 0; }
  footer { margin-top: 3rem; padding-top: 1rem; border-top: 1px solid #ddd;
           color: #777; font-size: .9em; }
</style>
</head>
<body>

<h1>AliveMonitor &mdash; Kurzanleitung</h1>
<p>Desktop-Anwendung zur Echtzeiterfassung der sechs analogen Spannungen
A0&ndash;A5 und zur Steuerung der digitalen Ausgänge D2&ndash;D9 eines
über USB angeschlossenen Arduino Uno (oder kompatiblen) Boards, mit
enthaltenem, dediziertem Firmware-Sketch.</p>

<h2>1. Zuerst: Firmware auf das Board laden</h2>
<p>Bevor das Board mit der Anwendung verbunden wird, muss der Sketch
<code>AliveMonitor.ino</code> mit der Arduino IDE oder
<code>arduino-cli</code> hochgeladen werden. Er befindet sich im Ordner
<code>arduino\AliveMonitor\</code>: neben der ausführbaren Datei, wenn
AliveMonitor über das Setup installiert wurde (auch über das Startmenü
erreichbar, Eintrag "Arduino-Firmware-Ordner"), oder im
Repository-Ordner, wenn AliveMonitor aus dem Quellcode gebaut wurde. Ohne
die Firmware antwortet das Board nie auf das Handshake, und die
Anwendung wird es nie finden, egal wie korrekt es über USB angeschlossen
ist.</p>
<pre><code>arduino-cli compile --fqbn arduino:avr:uno AliveMonitor.ino
arduino-cli upload  --fqbn arduino:avr:uno -p /dev/ttyACM0 AliveMonitor.ino</code></pre>
<p>Ist dieser Schritt einmal erledigt (die Firmware bleibt auch nach dem
Ausschalten auf dem Board), kann man mit der Verbindung fortfahren.</p>

<h2>2. Verbindung</h2>
<p>Beim Start durchsucht die Anwendung automatisch die verfügbaren
seriellen Schnittstellen und verbindet sich mit dem ersten Board, das
korrekt auf das Firmware-Handshake antwortet. Die LED oben links zeigt
den Status an: <span class="led verde"></span>verbunden,
<span class="led rosso"></span>Suche läuft. Wird das Board getrennt, wird
die Suche von selbst fortgesetzt, und bei der Wiederverbindung werden
digitale Ausgänge und Streaming automatisch wiederhergestellt.</p>

<h2>3. Digitale Ausgänge</h2>
<p>Die Schaltflächen D2&ndash;D9 steuern die digitalen Ausgänge des
Boards. Die LED neben jeder Schaltfläche zeigt den vom Firmware
<em>tatsächlich</em> bestätigten Zustand, nicht nur den angeforderten:
Erreicht der Befehl sein Ziel nicht (Board getrennt, serieller Fehler),
ändert sich die LED nicht.</p>

<h2>4. Erfassung</h2>
<p><b>Start</b> beginnt das Streaming der sechs analogen Spannungen mit
der eingestellten Frequenz (1&ndash;250&nbsp;Hz, auch während laufender
Erfassung änderbar). <b>Pause</b> unterbricht sie, wobei die bereits
erfassten Daten erhalten bleiben; <b>Stopp</b> beendet sie und leert den
Puffer. Das Kontrollkästchen "CSV aufzeichnen" unter diesen drei
Schaltflächen lässt, wenn aktiviert (Standard), beim Drücken von Start
ein Fenster erscheinen, um den Speicherort der Aufzeichnung zu wählen
&mdash; siehe "CSV-Aufzeichnung" weiter unten.</p>

<h2>5. Das Diagramm mit 7 Registerkarten</h2>
<p>Das Diagramm ist in 7 Registerkarten gegliedert. Die erste,
<b>"Alle"</b>, zeigt alle sechs Kurven überlagert in Volt, mit
Kontrollkästchen zum Ausblenden einzelner Kanäle. Die anderen sechs
Registerkarten (eine pro Kanal) zeigen jeweils eine einzelne Kurve, mit
der Y-Achse in der <em>umgerechneten physikalischen Größe</em> gemäß der
Kalibrierung des Kanals (siehe unten) &mdash; praktisch, um
unterschiedliche Größen (Temperatur, Druck, ...) jeweils auf ihrer
eigenen Skala zu vergleichen, ohne sie auf einer einzigen Volt-Achse
überlagern zu müssen.</p>
<p>Auf jeder Registerkarte: Mausrad = Zoom auf der Zeitachse,
Strg+Mausrad = Zoom auf der Y-Achse, Ziehen = Verschieben. Die
Schaltfläche <b>Folgen</b> koppelt das scrollende Fenster wieder an die
aktuelle Zeit, <b>Autoskalierung</b> passt die Y-Achse einmalig an die
sichtbaren Daten an, <b>Zurücksetzen</b> stellt die Standardansicht
wieder her, <b>PNG</b> exportiert die aktuell angezeigte Registerkarte
als Bild.</p>

<h2>6. Kalibrierung der Kanäle (Volt &rarr; physikalische Größe)</h2>
<p>Im linken Bereich, unterhalb der Erfassungssteuerung, kann in der
Kalibrierungstabelle jedem Kanal A0&ndash;A5 ein Messumformer mit
linearem Zusammenhang <code>G = a&middot;V + b</code> zugeordnet werden:
Koeffizient <code>a</code>, Offset <code>b</code>, Maßeinheit und
optional eine freie Beschreibung eingeben (z. B. "Ofentemperatur"). Ist
sie ausgefüllt, wird die Beschreibung zum Titel der diesem Kanal
gewidmeten Registerkarte im Diagramm; die Auswirkung auf Legende und
Y-Achse erfolgt sofort. Nicht konfigurierte Kanäle bleiben bei der
Identität (<code>a=1, b=0</code>, Einheit "V"), zeigen also einfach die
Spannung.</p>
<div class="nota">Die Kalibrierung gilt nur für die aktuelle Sitzung der
Anwendung: Sie wird nicht auf der Festplatte gespeichert und muss bei
jedem Neustart erneut eingegeben werden.</div>

<h2>7. CSV-Aufzeichnung</h2>
<p>Bei aktiver Aufzeichnung wird jeder empfangene Messwert in Echtzeit in
die Datei geschrieben, ohne die Erfassung jemals zu blockieren. Die Datei
enthält neben der Zeit und den sechs Spannungen in Volt
(<code>A0_V</code>&hellip;<code>A5_V</code>) sechs zusätzliche Spalten
mit dem bereits umgerechneten Wert und seiner Einheit im Spaltennamen
(z. B. <code>A0_conv[&deg;C]</code>), unter Verwendung der zum Zeitpunkt
des Startens eingestellten Kalibrierung. Eine LED und eine Beschriftung
in der Werkzeugleiste des kombinierten Diagramms zeigen den Status der
Aufzeichnung: <span class="led verde"></span>läuft,
<span class="led rosso"></span>gestoppt.</p>

<h2>8. Einstellungen</h2>
<p>Über das Menü Extras &gt; Einstellungen lassen sich die Render-FPS des
Diagramms, die Breite des sichtbaren Zeitfensters (auf alle 7
Registerkarten angewendet), die fortlaufende Y-Autoskalierung der
kombinierten Registerkarte (die sechs Registerkarten pro Kanal haben ihr
eigenes, standardmäßig bereits aktives Auto Y) sowie die
Oberflächensprache (Italiano, English, Français, Español, Deutsch)
einstellen &mdash; Letzteres erfordert einen Neustart der Anwendung, um
wirksam zu werden.</p>

<h2>Lizenz</h2>
<p>AliveMonitor wird unter der <b>MIT</b>-Lizenz vertrieben: frei
nutzbar, änderbar und weitergebbar, auch kommerziell, <b>ohne jegliche
Gewährleistung oder Haftung der Autoren</b> für Schäden, die aus der
Nutzung der Software entstehen. Vollständiger Text in der Datei
<code>LICENSE</code> im Repository.</p>

<footer>
AliveMonitor &mdash; entwickelt von Leonardo Catalano mit Unterstützung
von Claude (Anthropic). Basiert auf C++20 und wxWidgets.
</footer>

</body>
</html>
)HTML";

/// Restituisce l'HTML della guida nella lingua richiesta.
inline constexpr const char* helpHtmlFor(Language lang)
{
    switch (lang) {
    case Language::IT: return kHelpHtmlIt;
    case Language::EN: return kHelpHtmlEn;
    case Language::FR: return kHelpHtmlFr;
    case Language::ES: return kHelpHtmlEs;
    case Language::DE: return kHelpHtmlDe;
    }
    return kHelpHtmlIt;  // irraggiungibile: switch esaustivo su tutti i valori dell'enum
}

} // namespace am
