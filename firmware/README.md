# firmware

Firmware del MCU di controllo (STM32G0B1CEU6, C, CMake + arm-none-eabi-gcc). Non ancora iniziato.

Stack (D96): HAL di STM32CubeG0 e TinyUSB per il dispositivo composito HID + CDC; nessun
RTOS, ciclo principale cooperativo con tick da 1 ms.

Responsabilità:
- interfaccia USB HID secondo [../docs/protocollo-controllo.md](../docs/protocollo-controllo.md);
- console diagnostica a comandi di testo e log sulla UART di J9 e sulla CDC USB (D97), usata
  anche per il collaudo delle singole periferiche;
- sequenza di reset e configurazione SMBus del hub USB2517, con sorveglianza del reset (D35);
- inizializzazione dei due UCS2114-1-V/LX su bus I²C separati (D75), abilitazione e
  verifica della scarica automatica prima di accendere le porte;
- comando logico delle porte tramite EN VBUS e OE dati separati;
- cicli temporizzati autonomi OFF/ON, con scarica anche automatica nel controller (D70),
  senza comando DIS obbligatorio né verifica VBUS; default indicativo 1 s da validare;
- accensioni sfalsate e gestione dei guasti segnalati dall'hardware;
- blocco della porta e interruzione del ciclo dopo guasto, LED lampeggiante e riarmo
  manuale da pulsante o PC (D73); nessun riavvio automatico alla scomparsa del guasto;
  bloccano solo sovracorrente e sovratemperatura, gli altri segnali del controller sono
  avvisi (D98);
- un LED per porta: spento, fisso, lampeggiante per guasto;
- cinque pulsanti, quattro per le porte e uno per il relè, con debounce e notifica all’host;
- nessuna misura di corrente/tensione, lettura delle CC d'ingresso o gestione del budget;
- comando e impulsi autonomi del solo relè ausiliario (`n_relays = 1`), quattro porte USB (`n_ports = 4`);
- stato di default all'accensione salvato in flash; di fabbrica tutte le porte abilitate
  dopo l’avvio, con accensioni sfalsate (D74).

Pinout e note di avvio in [../docs/pinout-mcu.md](../docs/pinout-mcu.md); il rilevamento
VBUS upstream necessario al funzionamento USB resta distinto dalle misure eliminate.
