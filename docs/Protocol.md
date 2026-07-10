# Protocollo seriale AliveMonitor

Protocollo **testuale a righe** (ASCII, terminatore `\n`; `\r` ignorato in ricezione da entrambe le parti). Parametri di linea: **115200 baud, 8N1**, nessun controllo di flusso.

Una riga = un comando o una risposta. Il protocollo è volutamente leggibile e verificabile con un qualsiasi monitor seriale.

## Handshake e rilevamento automatico

| Direzione | Riga | Note |
|---|---|---|
| PC → Arduino | `HELLO` | Inviato su ogni porta candidata durante la scansione |
| Arduino → PC | `ARDUINO_UNO` | Identifica la scheda; il PC considera stabilita la connessione |

All'apertura della porta il PC attende ~1,8 s (reset automatico della Uno via DTR + bootloader) prima di inviare `HELLO`. `HELLO` è usato anche come *keep-alive* quando lo streaming è fermo: l'assenza di risposta entro 4 s fa scattare la riconnessione automatica.

## Comandi

### Versione firmware

```
PC:      VER
Arduino: VERSION 1.0.0
```

### Uscite digitali (D2..D9)

```
PC:      SET D2 1
Arduino: OK D2=1
PC:      SET D2 0
Arduino: OK D2=0
```

Errori: `ERR SET SYNTAX`, `ERR SET VALUE`, `ERR SET PIN`.
La GUI accende il LED di un'uscita **solo** alla ricezione di `OK Dx=v` (stato reale, non richiesto).

### Richiesta dati singola

```
PC:      GET
Arduino: ADC,512,234,876,100,200,400*4B
```

### Frequenza di campionamento

```
PC:      RATE 100
Arduino: OK RATE=100        (oppure ERR RATE SYNTAX / ERR RATE RANGE)
```

Intervallo valido: 1..250 Hz. Modificabile in qualunque momento, anche con streaming attivo.

### Streaming continuo

```
PC:      STREAM 1
Arduino: OK STREAM=1
         ADC,...*CS         (righe automatiche alla frequenza RATE)
PC:      STREAM 0
Arduino: OK STREAM=0
```

### Comando sconosciuto

```
Arduino: ERR UNKNOWN
```

## Frame dati ADC

```
ADC,v0,v1,v2,v3,v4,v5*CS
```

`v0..v5` = valori grezzi 0..1023 di A0..A5. `CS` = checksum stile NMEA: **XOR di tutti i caratteri che precedono `*`**, espresso con due cifre esadecimali maiuscole. Il PC scarta i frame con checksum errato incrementando il contatore *Errori CRC*.

Esempio di calcolo per `ADC,0,0,0,0,0,0`: XOR di `A`,`D`,`C`,`,`,`0`,... → `*7E` (due cifre hex).

## Budget di banda

Frame massimo: `ADC,1023,1023,1023,1023,1023,1023*XX\r\n` = 39 caratteri → 390 bit con framing UART.
A 250 Hz: ~97,5 kbit/s < 115,2 kbit/s. **OK con margine.**
Per 500 Hz futuri: portare il baud a 230400 (già previsto dal layer seriale) o passare a un framing binario (vedi `Architecture.md`).

## Gestione errori e robustezza

- **Checksum errato** → frame scartato, contatore CRC incrementato (nessuna richiesta di ritrasmissione: il dato successivo arriva dopo 4 ms).
- **Pacchetti persi** → stimati dal PC misurando i "buchi" temporali tra frame consecutivi rispetto al periodo nominale.
- **Righe malformate / rumore** → contatore errori seriali; l'accumulatore RX viene svuotato se supera 4 KiB senza terminatore.
- **Silenzio prolungato** (> 4 s) → disconnessione, ritorno alla scansione automatica, riconnessione con ripristino di uscite, RATE e streaming.

## Estensione del protocollo

Regole per i nuovi comandi (mantengono la compatibilità):
1. comando = parola chiave maiuscola + parametri separati da spazi;
2. risposta = `OK <CHIAVE>=<valore>` oppure `ERR <motivo>`;
3. dati periodici = riga `NOME,...*CS` con checksum XOR.

Esempi già previsti dal design:

| Funzione | Comando proposto | Risposta |
|---|---|---|
| PWM | `PWM D5 128` | `OK PWM5=128` |
| Ingressi digitali | `GETD` | `DIN,1,0,1,0,1,0,0,1*CS` |
| Servo | `SERVO D9 90` | `OK SERVO9=90` |
| EEPROM | `EEW 10 255` / `EER 10` | `OK EE10=255` / `EE,10,255*CS` |
| I2C | `I2CR 0x48 2` | `I2C,0x48,ab,cd*CS` |
| SPI | `SPIX a5 3` | `SPI,a5,01,02,03*CS` |

Lato PC è sufficiente aggiungere il builder e il ramo di parsing in `CommunicationProtocol`, un metodo in `CommandController` e l'eventuale widget nella View; lato firmware un ramo in `handleCommand()`.
