/**
 * @file HelpContent.h
 * @brief Guida utente incorporata (HTML), mostrata dal menu Aiuto > Guida.
 *
 * Come per l'icona (resources/AliveMonitor.xpm), il contenuto è incorporato
 * direttamente nell'eseguibile come stringa C++: nessun file esterno da
 * distribuire insieme al binario. MainFrame lo scrive in un file temporaneo
 * e lo apre con il browser predefinito (wxLaunchDefaultBrowser), quindi non
 * serve alcun componente wxHTML: la resa è quella del browser dell'utente.
 */
#pragma once

namespace am {

inline constexpr const char* kHelpHtml = R"HTML(<!DOCTYPE html>
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
<code>arduino/AliveMonitor/AliveMonitor.ino</code> con Arduino IDE o
<code>arduino-cli</code>. Senza firmware la scheda non risponde
all'handshake e l'applicazione non la troverà mai, per quanto sia collegata
correttamente via USB.</p>
<pre><code>arduino-cli compile --fqbn arduino:avr:uno arduino/AliveMonitor/AliveMonitor.ino
arduino-cli upload  --fqbn arduino:avr:uno -p /dev/ttyACM0 arduino/AliveMonitor/AliveMonitor.ino</code></pre>
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
tutte le 7 schede) e l'autoscale Y continuo della scheda combinata (le sei
schede per canale hanno un proprio Auto Y indipendente, già attivo di
default).</p>

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

} // namespace am
