# Piano di implementazione del firmware

Documento di lavorazione: descrive come si intende realizzare il firmware e la libreria host
minima, non lo stato del progetto. Le specifiche restano in `docs/` (requisiti, architettura,
protocollo, pinout, decisioni D96–D101); in caso di differenze prevalgono quelle.

## Stato

Il codice degli incrementi 1–10 è scritto in `firmware/` e `host/`; la logica indipendente
dall'hardware supera i test su PC. Sul prototipo Rev A, collegato via USB e programmato via
DFU, sono state fatte le prove possibili senza strumenti, dalla console sulla seriale
virtuale e dalla libreria Python. La versione provata comprende le correzioni della
revisione del codice (banco flash, sequenza dell'ADC, variabili persistenti fuori dalla RAM
del bootloader, coda dei report, sorveglianza del reset del hub, recupero del bus I²C).

| Incremento | Verificato sulla scheda | Resta da verificare |
|---|---|---|
| 1. Scheletro e avvio sicuro | MCU riconosciuto dalla sonda SWD su J4; avvio, console sulla CDC, log di avvio conservato | programmazione via SWD; console sulla UART di J9; EN e relè a riposo durante il reset, all'oscilloscopio |
| 2. GPIO della scheda | sei LED, cinque pulsanti senza rimbalzi, scatto del relè, VBUS sui test point TP11–TP14 | — |
| 3. Hub USB2517 | configurazione SMBus con VID/PID e stringhe del progetto; quattro porte esterne più il dispositivo di controllo; un reset del MCU non resetta il hub; `hub reset` | porta del MCU non rimovibile nel descrittore; ripartenza dopo un reset del hub da U12 |
| 4. USB del MCU | composito HID + CDC su macOS, console sulla seriale virtuale, LED_HOST | LED_HOST con bus sospeso |
| 5. UCS2114 | U4 e U5 identificati; scarica automatica 400 ms e maschera di ALERT# scritte e rilette; limite 2,67 A dallo strap | rampa di scarica con carico capacitivo; cause dei guasti con sovraccarico e sorgente esterna; punti aperti di D98 |
| 6. Logica delle porte | accensione, spegnimento e ciclo sulle quattro porte; chiavetta USB che enumera, sparisce e ricompare; accensioni sfalsate all'avvio | sfasamento e inrush misurati; corto su una porta con blocco, LED lampeggiante e riarmo |
| 7. Relè | comando e impulso dalla CLI e dai test, evento con impulso da 200 ms; continuità fra le boccole che si interrompe allo scatto; LED acceso a linea staccata (D90) | impulso misurato sui contatti; relè a riposo in DFU |
| 8. Protocollo HID e libreria | `info`, `status`, porte, relè, default, `REBOOT` dalla CLI, errori sui parametri, eventi di porta e relè, ciclo che prosegue a programma chiuso; sei test `pytest -m board` | eventi di guasto |
| 9. Default in flash | salvataggi ripetuti e rilettura dopo il riavvio | record corrotto con ritorno ai default di fabbrica |
| 10. Funzioni di sistema | DFU con flash vuota e da firmware, riprogrammazione senza sonda, ritorno al firmware dopo il "leave" di dfu-util; `REBOOT`; watchdog con `hang` (riavvio dopo circa 2,5 s, hub non resettato, porte che ripartono sfalsate); VBUS_DET letto dal MCU uguale alla misura col multimetro entro lo 0,2 % (vedi sotto) | evento `POWER_CHANGED` da EFUSE_FLT_N |
| 11. Validazione e documenti | — | tutto (vedi sotto) |

Dopo il "leave" di dfu-util il bootloader salta al firmware senza reset, lasciando vettori,
interrupt e clock propri: il firmware lo riconosce e si riavvia da solo.

VBUS_DET, misurato col multimetro con il PC collegato e la lettura del MCU stabile:

| Grandezza | Valore |
|---|---|
| VBUS upstream (R22 lato J2) | 5,281 V |
| nodo R22/R23 (ingresso ADC) | 2,631 V, rapporto 0,498 |
| +3V3 (TP3, riferimento dell'ADC) | 3,311 V |
| lettura del MCU (`status`) | 5271 mV |

L'errore complessivo è di −10 mV. Lo scarto fra il rapporto nominale e quello misurato
(−0,4 %) rientra nella tolleranza delle resistenze ed è compensato in parte dal riferimento
dell'ADC, che il firmware assume a 3,300 V. Non serve una correzione con VREFINT.

### Anomalie hardware

Tutte dovute a saldature, risolte con ripassate ad aria calda; nel PCB le reti sono corrette.

| Componente | Pin | Sintomo | Come è stato individuato |
|---|---|---|---|
| U3 (MCU) | 16 (PA5), 21 (PB2), 23 (I2C2_SDA), oltre a quelli di FAULT_P1 (PB0) ed EFUSE_FLT_N (PB1) | LED della porta 1 spento; VBUS_DET errato; U5 irraggiungibile su I2C2; porta 1 che non restava accesa e guasto d'ingresso segnalato | — |
| U4 e U5 | 1, PWR_EN1 | porte 1 e 3 sempre alimentate | rete EN bassa lato MCU, ma enable letto alto dal controller; con PIN_IGN (13h) il canale si spegneva |
| U4 | 15, PWR_EN2 | porta 2 mai alimentata | enable mai visto nel registro 0Fh; con PWR_EN2S (11h) scritto via I²C TP12 saliva a 5 V |
| U5 | 9 e 10, VS del canale 2 | porta 4 mai alimentata dopo una ripassata | enable visto, VS_LOW fisso nel registro 04h; sparito dopo la riparazione |
| J401 e U401 | — | sulla porta 4 i dati non passavano, nessun tentativo di collegamento visto dal hub | VBUS presente sul test point, chiavetta che enumera sulle altre porte |

Nota sui registri dell'UCS2114: con entrambi gli enable bassi il controller dorme e i
registri di stato (0Fh, 10h, 13h) si leggono con valori diversi da quelli a controller
sveglio; non vanno interpretati in quella condizione.

### Punti aperti

- **Guasto generico delle porte 1 e 2 all'accensione.** Visto una sola volta, alla prima
  accensione dopo la ripassata del pin 23; non riprodotto al ciclo di alimentazione
  successivo (log di avvio regolare, quattro porte accese), né dopo reset software o
  `hub reset`. Causa non determinata: se si ripresenta, il comando `log` mostra la maschera
  di ALERT# e i registri di stato letti al momento del guasto.
- Le misure dell'incremento 11 e le verifiche rimaste nella tabella sopra richiedono strumenti
  (oscilloscopio, carico elettronico, alimentatore esterno) o un guasto provocato.

## Punto di partenza

- Prototipo Rev A montato. Verificati al banco: alimentazioni (+5V, +3V3, eFuse),
  enumerazione del hub USB2517 con i default interni a MCU vuoto, MCU raggiungibile.
- Non ancora provati: comando delle porte (EN, OE), controller UCS2114, relè, pulsanti, LED,
  configurazione SMBus del hub.
- Strumenti presenti sulla macchina di sviluppo: arm-none-eabi-gcc 15.2, CMake, OpenOCD,
  st-flash, J-Link, dfu-util, Python 3. Manca ninja: si usa il generatore Makefile oppure lo
  si installa.

## Impostazione

Un solo firmware (D96): il collaudo della scheda avviene con la console diagnostica (D97)
del firmware definitivo. Ogni incremento aggiunge un modulo, i suoi comandi di console e una
verifica sulla scheda; un incremento è chiuso solo quando la verifica è passata.

### Dipendenze e build

- `firmware/` con CMake, toolchain file per arm-none-eabi-gcc, preset `debug` e `release`.
- Sottomoduli git in `firmware/third_party/`: `cmsis_device_g0`, `stm32g0xx_hal_driver`,
  `CMSIS_5` (solo Core) e `tinyusb`. Nessun progetto CubeMX; startup e linker script
  derivati da quelli del pacchetto CMSIS device per STM32G0B1xE.
- Target di comodo: `flash` (SWD con OpenOCD o J-Link), `dfu` (dfu-util attraverso il hub).
- Versione del firmware in un solo punto (CMake), riportata in `GET_INFO` e nei descrittori.

### Struttura dei sorgenti

```
firmware/
  CMakeLists.txt, CMakePresets.json, cmake/
  third_party/            sottomoduli
  src/
    main.c                avvio e ciclo principale
    board/                pin, clock, avvio sicuro dei GPIO (board.h, board.c)
    drivers/              i2c_bus, ucs2114, usb2517, uart, flash_store
    app/                  ports, relay, buttons, leds, hub_ctrl, power, defaults
    proto/                codifica dei report HID e dispatcher dei comandi
    usb/                  descrittori e callback TinyUSB (HID + CDC)
    console/              shell a comandi, log, backend UART e CDC
  tests/                  test su PC della logica pura (CTest)
host/
  devhub/                 libreria Python (hidapi) e CLI
  tests/                  test automatici sulla scheda (pytest)
```

### Architettura

- Ciclo principale cooperativo, tick di sistema da 1 ms; ogni modulo ha `init()` e
  `task(now_ms)` non bloccante. Nessun RTOS.
- Interrupt ridotti: EXTI di FAULT_P1–P4 ed EFUSE_FLT_N alzano un flag; USB e UART secondo
  i rispettivi driver. Pulsanti e HUB_RST_N in polling. Le transazioni I²C sono bloccanti
  con timeout breve, eseguite solo dal ciclo principale.
- Logica separata dall'hardware: `app/ports` è una macchina a stati che riceve eventi
  (comando, pulsante, guasto, tick) e chiama un'interfaccia ristretta (`set_power`,
  `set_data`, `read_cause`). La stessa logica e il codec del protocollo si compilano su PC
  per i test.
- Stati di una porta: `OFF`, `ON`, `CYCLE_OFF` (in attesa della riaccensione), `FAULT`.
  Accensioni in coda con sfasamento di 100 ms, comune ad avvio, `SET_ALL` e fine ciclo.
- Clock: HSI16 come clock di sistema senza PLL; HSI48 con CRS sul SOF per l'USB.
- Watchdog indipendente (IWDG) attivato dopo l'eventuale salto al bootloader.

## Incrementi

### 1. Scheletro e avvio sicuro

- Progetto CMake, startup, clock, tick, UART di J9 a 115200 8N1 con `printf` dei log.
- Avvio sicuro dei GPIO come da [pinout-mcu.md](../pinout-mcu.md): registri di uscita
  precaricati (EN bassi, OE_N alti, RLY1 basso), UCPD1_STROBE e UCPD2_STROBE prima di
  pilotare PA8, PB15 e PD2, pull-down di PA4 rimosso, PF1 solo open-drain.
- Console minima: `help`, `version`, `reset`.

Verifica: programmazione via SWD, banner sulla UART, LED_HOST che lampeggia come segno di
vita, porte e relè a riposo con l'oscilloscopio sugli EN durante il reset.

### 2. GPIO della scheda

- Moduli `leds`, `buttons` (debounce 20 ms, evento alla pressione), comando grezzo di RLY1.
- Console: `led <n> on|off|blink`, `btn` (stato e pressioni), `relay on|off`,
  `gpio en|oe <n> 0|1` per forzare i singoli segnali di una porta.

Verifica: sei LED, cinque pulsanti, scatto del relè e continuità COM/NC; con `gpio`
VBUS presente sui test point TP11–TP14 e dati della porta collegati, un dispositivo USB
enumera sul PC tramite il hub ancora con i default interni.

### 3. Hub USB2517

- `i2c_bus` su I2C1 (100 kHz) e driver `usb2517`: scrittura a blocchi SMBus dei registri,
  VID/PID e stringhe, porta 6 non rimovibile, porte 5 e 7 disabilitate, comando di attach.
- `hub_ctrl`: sequenza di avvio (reset basso, CFG_SEL2, SCL alto, rilascio, attesa della
  risalita reale di HUB_RST_N, configurazione, attach), distinzione fra accensione e reset
  software tramite i flag di RCC_CSR, sorveglianza del reset ogni ≤10 ms (D35).
- Console: `hub status|reset|dump`.

Verifica: il hub enumera con VID/PID e stringhe del progetto, quattro porte esterne più la 6;
dopo `reset` del MCU il hub non si resetta; forzando HUB_RST_N basso la sequenza riparte.

### 4. USB del MCU

- TinyUSB con dispositivo composito HID vendor (report da 64 byte, ID 0x01–0x03) + CDC,
  numero di serie dall'UID.
- Backend CDC della console; LED_HOST acceso con dispositivo configurato e bus non sospeso.
- Eco di prova sull'HID: un comando qualsiasi riceve una risposta `UNKNOWN_CMD`.

Verifica: enumerazione su macOS attraverso la porta 6, console sulla seriale virtuale,
report HID scambiati con uno script `hidapi`, LED_HOST che segue collegamento e sospensione.

### 5. Controller delle porte UCS2114

- Driver `ucs2114` su I2C1 (U4) e I2C2 (U5): risveglio con lettura a vuoto, verifica di
  FDh/FEh, configurazione di 11h e 12h (scarica automatica 400 ms, ALERT mascherato, D98)
  con rilettura, lettura e decodifica di 02h, 03h, 04h, riconfigurazione se compare il bit
  RESET.
- Console: `ucs <1|2> id|dump|init|status`.

Verifica: entrambi i chip identificati; rampa di scarica a 400 ms su TP11–TP14 togliendo
l'EN con un carico capacitivo; cause lette correttamente con un sovraccarico e con una
sorgente esterna a 5 V sulla porta. Qui si chiudono i punti aperti di D98: ALERT# con
maschera attiva, lettura della causa prima di togliere PWR_EN, uscita dallo stato di errore.

### 6. Logica delle porte

- `app/ports`: macchina a stati, comando unico VBUS + dati, cicli temporizzati, accensioni
  sfalsate, blocco su guasto con riarmo esplicito (D73), avvisi (D98), LED di porta,
  pulsanti con annullamento del ciclo (D99), stato indipendente dal PC (D100).
- Avvio: porte accese secondo i default (per ora di fabbrica, tutte abilitate).
- Test su PC della macchina a stati: transizioni, guasto durante un ciclo, pulsante durante
  un ciclo, coda delle accensioni.
- Console: `port <n> on|off|cycle [ms]|clear`, `ports`.

Verifica: accensione sfalsata misurata all'avvio; ciclo da 1 s su una scheda alimentata dalla
porta; corto su una porta con blocco, LED lampeggiante e riarmo da pulsante; le altre porte
restano accese.

### 7. Relè

- `app/relay`: stato della linea, impulso temporizzato, pulsante.
- Console: `relay connect|disconnect|pulse <ms>`.

Verifica: impulso misurato sui contatti, pulsante, relè a riposo dopo reset e in DFU.

### 8. Protocollo HID e libreria host

- `proto/`: codec dei report e dispatcher secondo
  [protocollo-controllo.md](../protocollo-controllo.md) 0.1: `GET_INFO`, `GET_STATUS`,
  comandi di porte e relè, `CLEAR_FAULT`, eventi con coda di uscita.
- Test su PC del codec: layout dei campi, parametri fuori intervallo, codici di stato.
- `host/devhub`: libreria Python su `hidapi` con gli stessi comandi ed eventi, CLI di base
  (`info`, `status`, `port`, `all`, `relay`, `watch`).
- `host/tests`: test pytest sulla scheda collegata, che diventano la prova di regressione.

Verifica: tutti i comandi da CLI; eventi ricevuti premendo i pulsanti e provocando un
guasto; il ciclo prosegue chiudendo il programma sul PC.

### 9. Default in flash

- `flash_store`: un record con versione e CRC nell'ultima pagina della flash (verificare
  negli option byte la configurazione a banco singolo o doppio), scrittura solo su
  `SAVE_DEFAULTS`.
- `GET_DEFAULTS`, `SET_DEFAULTS`, `SAVE_DEFAULTS`; applicazione all'avvio.

Verifica: default diversi dopo un ciclo di alimentazione; record corrotto → default di fabbrica.

### 10. Funzioni di sistema

- `REBOOT`; `ENTER_BOOTLOADER` con flag in RAM non inizializzata, reset e salto alla memoria
  di sistema prima di ogni inizializzazione; `RESET_HUB`.
- IWDG; `power`: EFUSE_FLT_N con evento `POWER_CHANGED`; lettura di VBUS_DET sulla console.
- CLI: `hub reset`, `defaults`, comando di aggiornamento via DFU.

Verifica: aggiornamento del firmware con dfu-util attraverso il hub, senza SWD e con il hub
che resta acceso; DFU con flash vuota; blocco simulato del ciclo principale recuperato dal
watchdog senza reset del hub.

### 11. Validazione sul prototipo e allineamento dei documenti

- Misure rimaste aperte in [domande-aperte.md](../domande-aperte.md): durata della scarica e
  default del ciclo con capacità note, sfasamento delle accensioni e inrush, comportamento
  con sorgente esterna, tempi I²C con i pull-up da 4,7 kΩ, corrente dei LED, transitori al
  reset e in DFU.
- Aggiornamento di protocollo (versione 1.0), pinout, architettura e decisioni con i valori
  validati; `firmware/README.md` e `host/README.md` con istruzioni di build e uso.

## Ordine e dipendenze

Gli incrementi 1–5 sono il collaudo della scheda e vanno in ordine: ciascuno abilita il
successivo (console → GPIO → hub → USB → controller). L'incremento 7 è indipendente dal 5 e
dal 6. La libreria host nasce con l'incremento 8; prima di allora le prove passano dalla
console. Un difetto hardware trovato negli incrementi 1–5 si annota subito, prima di
proseguire, perché può cambiare pinout o schema.

## Rischi

- **Stato di errore degli UCS2114 con ALERT# mascherato (D98).** Se con tensione di ritorno
  il controller non riprende da solo, il firmware dovrà cancellare ERR in polling; se
  nemmeno questo basta si rivede D98.
- **Rilascio del reset del hub durante una transazione I²C.** SCL è anche CFG_SEL0: tutto
  l'accesso a I2C1, compreso U4, passa da `hub_ctrl`, che lo sospende durante il reset.
- **Salto al bootloader.** Il comportamento dei pin in DFU è previsto dal pinout ma non
  ancora dimostrato: è la verifica dell'incremento 10, anticipabile dopo l'incremento 4.
- **TinyUSB su STM32G0B1.** Porta supportata, ma clock HSI48/CRS e composito HID + CDC
  vanno provati presto: per questo l'USB è al quarto incremento e non alla fine.
