/**
 * AliveMonitor.ino
 * ============================================================================
 * Firmware per Arduino Uno - controparte dell'applicazione PC AliveMonitor.
 *
 * Caratteristiche:
 *  - Serial a 115200 baud;
 *  - D2..D9 configurati come OUTPUT (inizialmente LOW);
 *  - lettura continua di A0..A5;
 *  - ciclo COMPLETAMENTE non bloccante: nessun delay(), temporizzazione
 *    con micros() per sostenere 250 Hz (e oltre) con jitter minimo;
 *  - protocollo testuale estendibile (una riga = un comando/una risposta).
 *
 * Protocollo (righe terminate da '\n'):
 *   PC -> Arduino                Arduino -> PC
 *   -----------------------      -----------------------------------------
 *   HELLO                        ARDUINO_UNO
 *   VER                          VERSION 1.0.0
 *   GET                          ADC,v0,v1,v2,v3,v4,v5*CS   (una riga)
 *   SET D<pin> <0|1>             OK D<pin>=<0|1>   |  ERR ...
 *   RATE <1..250>                OK RATE=<hz>      |  ERR ...
 *   STREAM <0|1>                 OK STREAM=<0|1>   |  ERR ...
 *   (comando ignoto)             ERR UNKNOWN
 *
 * Le righe ADC portano un checksum stile NMEA: '*' seguito da due cifre
 * esadecimali = XOR di tutti i caratteri che precedono l'asterisco.
 * Con STREAM 1 le righe ADC vengono emesse automaticamente alla frequenza
 * impostata con RATE.
 *
 * Estensione: aggiungere un comando = aggiungere un ramo in handleCommand().
 * ============================================================================
 */

// ---------------------------------------------------------------------------
// Configurazione
// ---------------------------------------------------------------------------
static const unsigned long BAUD_RATE = 115200UL;
static const char FIRMWARE_VERSION[] = "1.0.0";

static const uint8_t FIRST_OUTPUT_PIN = 2;   // D2
static const uint8_t NUM_OUTPUT_PINS  = 8;   // D2..D9
static const uint8_t NUM_ADC_CHANNELS = 6;  // A0..A5

static const unsigned int MIN_RATE_HZ = 1;
static const unsigned int MAX_RATE_HZ = 250;

// Dimensione del buffer di ricezione comandi (una riga per volta).
static const uint8_t RX_BUFFER_SIZE = 32;

// ---------------------------------------------------------------------------
// Stato del firmware (locale al modulo, nessuna esposizione globale)
// ---------------------------------------------------------------------------
static char rxBuffer[RX_BUFFER_SIZE];  // riga in costruzione
static uint8_t rxLength = 0;           // caratteri accumulati
static bool rxDiscarding = false;      // overflow in corso: scarta fino al prossimo '\n'

static bool streaming = false;             // STREAM 1 attivo?
static uint32_t samplePeriodUs = 10000UL;  // periodo campione (default 100 Hz)
static uint32_t nextSampleAtUs = 0;        // istante del prossimo campione

// ---------------------------------------------------------------------------
// setup(): inizializzazione hardware
// ---------------------------------------------------------------------------
void setup()
{
    Serial.begin(BAUD_RATE);

    // D2..D9 come uscite, inizialmente spente.
    for (uint8_t i = 0; i < NUM_OUTPUT_PINS; ++i) {
        const uint8_t pin = FIRST_OUTPUT_PIN + i;
        pinMode(pin, OUTPUT);
        digitalWrite(pin, LOW);
    }

    nextSampleAtUs = micros();
}

// ---------------------------------------------------------------------------
// loop(): ciclo non bloccante
// ---------------------------------------------------------------------------
void loop()
{
    pollSerial();

    if (streaming) {
        const uint32_t now = micros();
        // Confronto robusto all'overflow di micros() (aritmetica unsigned).
        if ((int32_t)(now - nextSampleAtUs) >= 0) {
            nextSampleAtUs += samplePeriodUs;
            // Se il loop è rimasto indietro di molti periodi (es. burst di
            // comandi), si riallinea senza tentare di "recuperare" campioni.
            if ((int32_t)(now - nextSampleAtUs) > (int32_t)(4UL * samplePeriodUs)) {
                nextSampleAtUs = now + samplePeriodUs;
            }
            sendAdcFrame();
        }
    }
}

// ---------------------------------------------------------------------------
// Ricezione comandi: accumula caratteri senza mai bloccare
// ---------------------------------------------------------------------------
static void pollSerial()
{
    while (Serial.available() > 0) {
        const char c = (char)Serial.read();

        if (c == '\n') {
            if (!rxDiscarding) {
                rxBuffer[rxLength] = '\0';
                if (rxLength > 0) {
                    handleCommand(rxBuffer);
                }
            }
            // Fine riga: sia in caso normale sia a valle di uno scarto per
            // overflow, si riparte puliti dalla riga successiva.
            rxLength = 0;
            rxDiscarding = false;
        } else if (c != '\r') {
            if (rxDiscarding) {
                // Overflow già in corso: i caratteri della riga troppo lunga
                // vengono ignorati fino al prossimo '\n' (vedi sotto), invece
                // di essere accumulati come se fossero un nuovo comando.
            } else if (rxLength < RX_BUFFER_SIZE - 1) {
                rxBuffer[rxLength++] = c;
            } else {
                // Riga troppo lunga: PRIMA la vecchia versione azzerava solo
                // rxLength, ma continuava ad accumulare i caratteri restanti
                // della stessa riga come se fossero un comando nuovo,
                // producendo un ERR UNKNOWN spurio (contato come errore lato
                // PC). Ora si scarta l'intera riga fino al prossimo '\n'.
                rxDiscarding = true;
            }
        }
    }
}

// ---------------------------------------------------------------------------
// Interpretazione dei comandi
// ---------------------------------------------------------------------------
static void handleCommand(const char* cmd)
{
    if (strcmp(cmd, "HELLO") == 0) {
        Serial.println(F("ARDUINO_UNO"));
        return;
    }

    if (strcmp(cmd, "VER") == 0) {
        Serial.print(F("VERSION "));
        Serial.println(FIRMWARE_VERSION);
        return;
    }

    if (strcmp(cmd, "GET") == 0) {
        sendAdcFrame();
        return;
    }

    if (strncmp(cmd, "SET ", 4) == 0) {
        handleSet(cmd + 4);
        return;
    }

    if (strncmp(cmd, "RATE ", 5) == 0) {
        handleRate(cmd + 5);
        return;
    }

    if (strncmp(cmd, "STREAM ", 7) == 0) {
        handleStream(cmd + 7);
        return;
    }

    Serial.println(F("ERR UNKNOWN"));
}

// "SET D<pin> <0|1>" -> "OK D<pin>=<v>"
static void handleSet(const char* args)
{
    if (args[0] != 'D') {
        Serial.println(F("ERR SET SYNTAX"));
        return;
    }

    char* endPtr = NULL;
    const long pin = strtol(args + 1, &endPtr, 10);
    if (endPtr == args + 1 || *endPtr != ' ') {
        Serial.println(F("ERR SET SYNTAX"));
        return;
    }

    const long value = strtol(endPtr + 1, &endPtr, 10);
    if (*endPtr != '\0' || (value != 0 && value != 1)) {
        Serial.println(F("ERR SET VALUE"));
        return;
    }

    if (pin < FIRST_OUTPUT_PIN || pin >= FIRST_OUTPUT_PIN + NUM_OUTPUT_PINS) {
        Serial.println(F("ERR SET PIN"));
        return;
    }

    digitalWrite((uint8_t)pin, value == 1 ? HIGH : LOW);

    // Conferma dello stato REALE: il PC accende il LED solo su questa riga.
    Serial.print(F("OK D"));
    Serial.print(pin);
    Serial.print('=');
    Serial.println(value);
}

// "RATE <hz>" -> "OK RATE=<hz>"
static void handleRate(const char* args)
{
    char* endPtr = NULL;
    const long hz = strtol(args, &endPtr, 10);
    if (endPtr == args || *endPtr != '\0') {
        Serial.println(F("ERR RATE SYNTAX"));
        return;
    }
    if (hz < (long)MIN_RATE_HZ || hz > (long)MAX_RATE_HZ) {
        Serial.println(F("ERR RATE RANGE"));
        return;
    }

    samplePeriodUs = 1000000UL / (uint32_t)hz;
    nextSampleAtUs = micros() + samplePeriodUs;  // riallineamento pulito

    Serial.print(F("OK RATE="));
    Serial.println(hz);
}

// "STREAM <0|1>" -> "OK STREAM=<v>"
static void handleStream(const char* args)
{
    if (args[0] == '1' && args[1] == '\0') {
        streaming = true;
        nextSampleAtUs = micros() + samplePeriodUs;
        Serial.println(F("OK STREAM=1"));
    } else if (args[0] == '0' && args[1] == '\0') {
        streaming = false;
        Serial.println(F("OK STREAM=0"));
    } else {
        Serial.println(F("ERR STREAM VALUE"));
    }
}

// ---------------------------------------------------------------------------
// Campionamento e trasmissione del frame ADC
// ---------------------------------------------------------------------------
// Formato: "ADC,v0,v1,v2,v3,v4,v5*CS\r\n"
// CS = XOR (due cifre esadecimali maiuscole) dei caratteri prima di '*'.
//
// Lunghezza massima: 4 + 6*5 + 3 + 2 = 39 caratteri -> buffer da 48.
static void sendAdcFrame()
{
    char line[48];
    uint8_t len = 0;

    line[len++] = 'A';
    line[len++] = 'D';
    line[len++] = 'C';

    for (uint8_t ch = 0; ch < NUM_ADC_CHANNELS; ++ch) {
        const int value = analogRead(A0 + ch);  // ~112 us per lettura
        line[len++] = ',';
        len += writeUInt(&line[len], (unsigned int)value);
    }

    // Checksum XOR dei caratteri accumulati finora.
    uint8_t cs = 0;
    for (uint8_t i = 0; i < len; ++i) {
        cs ^= (uint8_t)line[i];
    }
    line[len++] = '*';
    line[len++] = toHexDigit((uint8_t)(cs >> 4));
    line[len++] = toHexDigit((uint8_t)(cs & 0x0F));
    line[len++] = '\r';
    line[len++] = '\n';

    // Trasmissione solo se il buffer TX ha spazio: il ciclo NON si blocca mai.
    // Se il buffer fosse pieno il frame viene saltato: il PC lo rileva come
    // "pacchetto perso" dall'intervallo temporale anomalo.
    if (Serial.availableForWrite() >= (int)len) {
        Serial.write((const uint8_t*)line, len);
    }
}

// Scrive un intero senza segno (0..65535) come testo. Ritorna i caratteri scritti.
static uint8_t writeUInt(char* dst, unsigned int value)
{
    char tmp[6];
    uint8_t n = 0;
    do {
        tmp[n++] = (char)('0' + (value % 10U));
        value /= 10U;
    } while (value > 0U && n < sizeof(tmp));

    // Le cifre sono in ordine inverso: ricopiatura dritta.
    for (uint8_t i = 0; i < n; ++i) {
        dst[i] = tmp[n - 1U - i];
    }
    return n;
}

static char toHexDigit(uint8_t v)
{
    return (char)(v < 10 ? '0' + v : 'A' + (v - 10));
}
