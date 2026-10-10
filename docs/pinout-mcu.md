# Piedinatura del MCU (STM32G0B1RET6, LQFP-64)

Corrisponde allo schema `hardware/devhub/mcu.kicad_sch`: in caso di modifiche allo schema va aggiornata a mano.
I pin senza segnale sono liberi (no-connect nello schema).

| Pin | Nome | Segnale | Funzione |
|---|---|---|---|
| 1 | PC11 | FAULT_P3 | ingresso: FAULT load switch porta 3 (basso = sovracorrente; anche OCS3_N del hub) |
| 2 | PC12 | — |  |
| 3 | PC13 | BTN_P1 | ingresso: pulsante porta 1 (basso = premuto; pull-up interno) |
| 4 | PC14 | BTN_P2 | ingresso: pulsante porta 2 (basso = premuto; pull-up interno) |
| 5 | PC15 | BTN_P3 | ingresso: pulsante porta 3 (basso = premuto; pull-up interno) |
| 6 | VBAT | +3V3 |  |
| 7 | VREF+ | +3V3 |  |
| 8 | VDD | +3V3 |  |
| 9 | VSS | GND |  |
| 10 | PF0 | BTN_P5 | ingresso: pulsante porta 5 (basso = premuto; pull-up interno) |
| 11 | PF1 | — |  |
| 12 | PF2 | NRST | reset (SWD J4 pin 10, C46) |
| 13 | PC0 | EN_P1 | uscita: EN load switch porta 1 (alto = VBUS on; pull-down) |
| 14 | PC1 | OE_N_P1 | uscita: OE del TS3USB30E porta 1 (basso = dati collegati; pull-up) |
| 15 | PC2 | DIS_P1 | uscita: scarica VBUS porta 1 (alto = scarica; pull-down) |
| 16 | PC3 | FAULT_P1 | ingresso: FAULT load switch porta 1 (basso = sovracorrente; anche OCS1_N del hub) |
| 17 | PA0 | CC1_ADC | ADC: tensione CC1 ingresso alimentazione |
| 18 | PA1 | CC2_ADC | ADC: tensione CC2 |
| 19 | PA2 | UART_TX | USART2 TX, J9 |
| 20 | PA3 | UART_RX | USART2 RX, J9 |
| 21 | PA4 | VSYS_ADC | ADC: 5V_SYS / 2 |
| 22 | PA5 | VBUS_DET | ADC: VBUS upstream / 2 (anche VBUS_DET del hub) |
| 23 | PA6 | — |  |
| 24 | PA7 | — |  |
| 25 | PC4 | EN_P2 | uscita: EN load switch porta 2 (alto = VBUS on; pull-down) |
| 26 | PC5 | OE_N_P2 | uscita: OE del TS3USB30E porta 2 (basso = dati collegati; pull-up) |
| 27 | PB0 | INA_ALERT | ingresso: CRITICAL dei due INA3221 (open-drain, pull-up R47) |
| 28 | PB1 | — |  |
| 29 | PB2 | — |  |
| 30 | PB10 | RLY1 | uscita: relè 1 (alto = linea staccata) |
| 31 | PB11 | RLY2 | uscita: relè 2 |
| 32 | PB12 | — |  |
| 33 | PB13 | — |  |
| 34 | PB14 | — |  |
| 35 | PB15 | — |  |
| 36 | PA8 | HUB_RST_N | uscita: RESET_N del hub (basso = reset) |
| 37 | PA9/UCPD1_DBCC1 | — |  |
| 38 | PC6 | DIS_P2 | uscita: scarica VBUS porta 2 (alto = scarica; pull-down) |
| 39 | PC7 | FAULT_P2 | ingresso: FAULT load switch porta 2 (basso = sovracorrente; anche OCS2_N del hub) |
| 40 | PD8 | DIS_P5 | uscita: scarica VBUS porta 5 (alto = scarica; pull-down) |
| 41 | PD9 | FAULT_P5 | ingresso: FAULT load switch porta 5 (basso = sovracorrente; anche OCS5_N del hub) |
| 42 | PA10/UCPD1_DBCC2 | — |  |
| 43 | PA9/PA11 | USB_MCU_DM | USB FS D- verso porta 6 del hub |
| 44 | PA10/PA12 | USB_MCU_DP | USB FS D+ verso porta 6 del hub |
| 45 | PA13 | SWDIO | SWD |
| 46 | PA14 | SWCLK | SWD (anche BOOT0 via nBOOT_SEL) |
| 47 | PA15 | HUB_CFG_SEL2 | uscita: CFG_SEL2 del hub, campionato al rilascio del reset (0 = SMBus, 1 = default interni) |
| 48 | PC8 | EN_P3 | uscita: EN load switch porta 3 (alto = VBUS on; pull-down) |
| 49 | PC9 | OE_N_P3 | uscita: OE del TS3USB30E porta 3 (basso = dati collegati; pull-up) |
| 50 | PD0 | EN_P4 | uscita: EN load switch porta 4 (alto = VBUS on; pull-down) |
| 51 | PD1 | OE_N_P4 | uscita: OE del TS3USB30E porta 4 (basso = dati collegati; pull-up) |
| 52 | PD2 | DIS_P4 | uscita: scarica VBUS porta 4 (alto = scarica; pull-down) |
| 53 | PD3 | FAULT_P4 | ingresso: FAULT load switch porta 4 (basso = sovracorrente; anche OCS4_N del hub) |
| 54 | PD4 | BTN_P4 | ingresso: pulsante porta 4 (basso = premuto; pull-up interno) |
| 55 | PD5 | EN_P5 | uscita: EN load switch porta 5 (alto = VBUS on; pull-down) |
| 56 | PD6 | OE_N_P5 | uscita: OE del TS3USB30E porta 5 (basso = dati collegati; pull-up) |
| 57 | PB3 | LED_SCK | SPI1 SCK → 74HC595 SRCLK |
| 58 | PB4 | LED_LATCH | uscita: 74HC595 RCLK |
| 59 | PB5 | LED_DATA | SPI1 MOSI → 74HC595 SER |
| 60 | PB6 | — |  |
| 61 | PB7 | — |  |
| 62 | PB8 | SCL | I2C1: INA3221 (0x40, 0x41), hub SMBus (0x2C) |
| 63 | PB9 | SDA | I2C1 |
| 64 | PC10 | DIS_P3 | uscita: scarica VBUS porta 3 (alto = scarica; pull-down) |

## Note per il firmware

- USB FS senza quarzo: HSI48 + CRS sincronizzato sul SOF.
- I2C1 su PB8/PB9: INA3221 a 0x40 (A0 = GND, porte 1-3) e 0x41 (A0 = VS, porte 4-5 e corrente d'ingresso); hub USB2517 a 0x2C in modalità SMBus.
- Sequenza di avvio del hub: tenere HUB_RST_N basso, impostare HUB_CFG_SEL2, rilasciare il reset; se CFG_SEL2 = 0 scrivere i registri del hub via SMBus e poi il comando di attach (registro FFh).
- 74HC595 ×2 in cascata (16 bit): U10 = LED porte 1-4 (PWR/FLT), U11 = LED porta 5, relè 1-2, alimentatore presente, host collegato.
- Pulsanti e FAULT sono su PC13/PC14/PC15 e altri pin a bassa capacità di pilotaggio: solo ingressi.
