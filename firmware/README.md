# firmware

Firmware del MCU di controllo (STM32G0B1RET6, C, CMake + arm-none-eabi-gcc). Non ancora iniziato.

Stack: STM32CubeG0 HAL, TinyUSB o libreria USB device di ST per HID/CDC.

Responsabilità:
- interfaccia USB HID secondo [../docs/protocollo-controllo.md](../docs/protocollo-controllo.md);
- comando dei load switch e degli switch dati, sequenze di stacco/riattacco e cicli temporizzati;
- lettura della corrente pubblicizzata dall'alimentatore (CC via ADC) e budget di potenza;
- lettura di fault e correnti, spegnimento delle porte oltre budget;
- comando dei relè ausiliari;
- stato di default all'accensione salvato in flash.
