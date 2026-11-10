# Piano di implementazione del firmware

Documento di lavorazione: descrive come si intende realizzare il firmware e la libreria host
minima, non lo stato del progetto. Le specifiche restano in `docs/` (requisiti, architettura,
protocollo, pinout, decisioni D96–D101); in caso di differenze prevalgono quelle.

## Stato

Il codice degli incrementi 1–10 è scritto in `firmware/` e `host/`; la logica indipendente
dall'hardware supera i test su PC. Sul prototipo Rev A, collegato via USB e programmato via
DFU, sono state fatte le prove possibili senza strumenti, dalla console sulla seriale
virtuale e dalla libreria Python.

Verificato sulla scheda:

- DFU con flash vuota attraverso il hub; ingresso nel bootloader da firmware (`dfu`) con il
  hub che resta acceso e configurato; riprogrammazione senza sonda.
- Avvio dopo il "leave" di dfu-util: il bootloader salta al firmware senza reset, lasciando
  vettori, interrupt e clock propri; il firmware lo riconosce e si riavvia da solo.
- Sequenza di reset e configurazione SMBus del hub: enumera con VID/PID e stringhe del
  progetto; il MCU compare sulla porta 6 come dispositivo composito HID + CDC su macOS; un
  reset software del MCU non resetta il hub.
- Controller U4 (porte 1-2) su I2C1: identificato, scarica automatica 400 ms e maschera di
  ALERT# scritte e rilette, limite 2,67 A dallo strap.
- Protocollo HID con la CLI: `info`, `status`, ciclo temporizzato di una porta, impulso del
  relè, default, errori sui parametri.

- Dopo la ripassata delle saldature di U3: FAULT_P1 (PB0) ed EFUSE_FLT_N (PB1) regolari, la
  porta 1 resta accesa e la segnalazione di guasto ingresso è rientrata. Tutti e sei i test
  `pytest -m board` passano: accensione e spegnimento delle quattro porte, ciclo da 1 s
  eseguito dal MCU, accensioni sfalsate, impulso del relè, default.
- Record dei default in flash: salvataggi ripetuti e rilettura dopo il riavvio; `REBOOT`
  da PC; eventi di porta e relè ricevuti con i tempi attesi (impulso da 200 ms, ciclo da
  600 ms).
- La versione provata comprende le correzioni della revisione del codice (banco flash,
  sequenza dell'ADC, variabili persistenti fuori dalla RAM del bootloader, coda dei report,
  sorveglianza del reset del hub, recupero del bus I²C).

- Dopo la seconda ripassata (pin 23): I2C2_SDA regolare, controller U5 (porte 3-4)
  identificato e configurato come U4. Nessuna anomalia di saldatura nota resta aperta.

Punti aperti dalle prove:

- **Guasto generico delle porte 1 e 2 all'accensione.** Visto una volta dopo un ciclo di
  alimentazione con entrambi i controller presenti; un nuovo comando di accensione le
  riabilita senza problemi, e non si ripete dopo un reset software né dopo `hub reset`.
  Il comando `log` della console conserva ora i messaggi di avvio, con la maschera di
  ALERT# e i registri di stato letti al momento del guasto: va riletto dopo un ciclo di
  alimentazione.
- VBUS_DET legge 5,5–5,6 V con il PC collegato: da confrontare con un multimetro, il valore
  reale atteso è intorno a 5 V.

Ancora da verificare al banco, perché richiedono strumenti o l'osservazione della scheda:
LED e pulsanti, VBUS e scarica sui test point, relè sui contatti, accensione sfalsata,
guasti reali (corto, sorgente esterna) e comportamento di ALERT# mascherato (D98), numero
di porte e porta non rimovibile nel descrittore del hub, watchdog, default in flash, UART
di J9.

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
