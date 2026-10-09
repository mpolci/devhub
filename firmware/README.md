# firmware

Firmware del MCU di controllo (RP2040, pico-sdk + TinyUSB, C). Non ancora iniziato.

Responsabilità:
- interfaccia USB HID secondo [../docs/protocollo-controllo.md](../docs/protocollo-controllo.md);
- comando dei load switch e degli switch dati, sequenze di stacco/riattacco e cicli temporizzati;
- lettura di fault e correnti, budget di potenza in base al PDO negoziato;
- comando dei relè ausiliari;
- stato di default all'accensione salvato in flash.
