# firmware

Firmware del MCU di controllo (STM32G0B1RET6, C, CMake + arm-none-eabi-gcc). Non ancora iniziato.

Stack: STM32CubeG0 HAL, TinyUSB o libreria USB device di ST per HID/CDC.

Responsabilità:
- interfaccia USB HID secondo [../docs/protocollo-controllo.md](../docs/protocollo-controllo.md);
- comando logico delle porte, accettando la sequenza interna del controller;
- cicli temporizzati con scarica attiva, senza verificare VBUS; default indicativo 1 s da validare;
- accensioni sfalsate e gestione dei guasti segnalati dall'hardware;
- un LED per porta: spento, fisso, lampeggiante per guasto;
- nessuna misura di corrente/tensione, lettura delle CC d'ingresso o gestione del budget;
- comando dei relè ausiliari;
- stato di default all'accensione salvato in flash.

Il pinout documenta lo schema attuale, ancora da adeguare a D60–D65; il rilevamento VBUS
upstream necessario al funzionamento USB resta distinto dalle misure eliminate.
