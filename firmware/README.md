# firmware

Firmware del MCU di controllo (STM32G0B1CEU6), in C con CMake e arm-none-eabi-gcc.

Stack (D96): HAL di STM32CubeG0 e TinyUSB per il dispositivo composito HID + CDC; nessun
RTOS, ciclo principale cooperativo con tick da 1 ms.

**Stato:** il firmware gira sul prototipo Rev A: hub configurato via SMBus, HID e console
verificati, aggiornamento via DFU funzionante. Il collaudo delle singole funzioni e le
anomalie hardware trovate sono in
[../docs/lavorazioni/piano-firmware.md](../docs/lavorazioni/piano-firmware.md).

## Funzioni

- interfaccia USB HID secondo [../docs/protocollo-controllo.md](../docs/protocollo-controllo.md);
- console diagnostica a comandi di testo e log sulla UART di J9 e sulla CDC USB (D97), usata
  anche per il collaudo delle singole periferiche;
- sequenza di reset e configurazione SMBus del hub USB2517, con sorveglianza del reset (D35);
  se la configurazione non riesce il hub riparte con i default interni;
- inizializzazione dei due UCS2114-1-V/LX su bus I²C separati (D75), abilitazione e
  verifica della scarica automatica prima di accendere le porte;
- comando logico delle porte tramite EN VBUS e OE dati separati;
- cicli temporizzati autonomi OFF/ON, senza verifica VBUS; default 1 s da validare;
- accensioni sfalsate di 100 ms e gestione dei guasti segnalati dall'hardware;
- blocco della porta e interruzione del ciclo dopo guasto, LED lampeggiante e riarmo
  manuale da pulsante o PC (D73); bloccano solo sovracorrente e sovratemperatura, gli altri
  segnali del controller sono avvisi (D98);
- un LED per porta: spento, fisso, lampeggiante per guasto; LED di host collegato;
- cinque pulsanti, quattro per le porte e uno per il relè, con debounce e notifica all'host;
- comando e impulsi autonomi del relè ausiliario;
- stato di default all'accensione salvato nell'ultima pagina della flash; di fabbrica
  tutte le porte abilitate (D74);
- riavvio, salto al bootloader ST per l'aggiornamento via DFU, reset del hub, watchdog;
- nessuna misura di corrente/tensione, lettura delle CC d'ingresso o gestione del budget.

Pinout e note di avvio in [../docs/pinout-mcu.md](../docs/pinout-mcu.md).

## Compilazione

Servono arm-none-eabi-gcc, CMake e make. Le dipendenze sono sottomoduli git in
`third_party/` (CMSIS, HAL STM32G0, TinyUSB):

```
git submodule update --init --depth 1
cmake --preset debug          # oppure release
cmake --build build/debug
```

Il risultato è `build/debug/devhub.elf` (e `devhub.bin`).

## Programmazione

- SWD con ST-Link e OpenOCD, dal connettore J4: `cmake --build build/debug --target flash`
- DFU attraverso il hub, senza sonda: comando `dfu` sulla console (o `devhub bootloader`
  dal PC), poi `cmake --build build/debug --target dfu`

## Console

UART di J9 a 115200 8N1 e porta seriale virtuale USB, con gli stessi comandi; `help` li
elenca. I principali:

| Comando | Uso |
|---|---|
| `status` | stato di sistema, hub, porte (logico e segnali), relè, default |
| `port <n> on\|off\|cycle [ms]\|clear`, `all on\|off` | comando logico delle porte |
| `relay connect\|disconnect\|pulse <ms>` | linea del relè |
| `hub [status\|reset\|dump]` | stato del hub, nuova sequenza di reset e configurazione |
| `ucs <1\|2> id\|init\|status\|dump` | controller delle porte 1-2 e 3-4: identificazione, registri |
| `led`, `btn`, `gpio en\|data <n> 0\|1` | collaudo di LED, pulsanti e singoli segnali delle porte |
| `io`, `pin <porta><n>`, `i2c <1\|2> scan` | registri GPIO, prova di un pin con pull-up e pull-down interni, scansione di un bus I²C |
| `defaults [set <maschera hex>\|save]` | porte abilitate all'accensione |
| `reset`, `dfu` | riavvio, ingresso nel bootloader |

## Sorgenti

```
src/main.c        avvio e ciclo principale
src/board/        pin, clock, avvio sicuro dei GPIO, watchdog
src/drivers/      bus I²C, UART, UCS2114, USB2517, record in flash
src/app/          logica di porte e relè, pulsanti, controllo del hub, default, servizi di sistema
src/proto/        protocollo HID: costanti, comandi, coda dei report
src/usb/          descrittori e callback TinyUSB
src/console/      console e comandi diagnostici
tests/            test su PC della logica (porte, relè, protocollo)
```

La logica in `app/ports.c`, `app/relay.c` e `proto/proto.c` non dipende dall'hardware e si
prova su PC:

```
cmake -S tests -B build/tests
cmake --build build/tests
ctest --test-dir build/tests --output-on-failure
```
