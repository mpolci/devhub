# Piedinatura del MCU (STM32G0B1RET6, LQFP-64)

Corrisponde allo schema `hardware/devhub/mcu.kicad_sch`: in caso di modifiche allo schema va aggiornata a mano.
I pin senza segnale sono liberi (no-connect nello schema).

| Pin | Nome | Segnale | Funzione |
|---|---|---|---|
| 1 | PC11 | FAULT_P3 | ingresso: FAULT load switch porta 3 (basso = sovracorrente; anche OCS3_N del hub) |
| 2 | PC12 | INA_ALERT | ingresso: CRITICAL dei due INA3221 (open-drain, pull-up R47) |
| 3 | PC13 | BTN_P1 | ingresso: pulsante porta 1 (basso = premuto; pull-up interno) |
| 4 | PC14 | BTN_P2 | ingresso: pulsante porta 2 (basso = premuto; pull-up interno) |
| 5 | PC15 | BTN_P3 | ingresso: pulsante porta 3 (basso = premuto; pull-up interno) |
| 6 | VBAT | +3V3 |  |
| 7 | VREF+ | +3V3 |  |
| 8 | VDD | +3V3 |  |
| 9 | VSS | GND |  |
| 10 | PF0 | BTN_P5 | ingresso: pulsante porta 5 (basso = premuto; pull-up interno) |
| 11 | PF1 | HUB_RST_N | uscita open-drain: RESET_N del hub (basso = reset), in wired-OR con il supervisore U12 |
| 12 | PF2 | NRST | reset (SWD J4 pin 10, C46) |
| 13 | PC0 | EN_P1 | uscita: EN load switch porta 1 (alto = VBUS on; pull-down) |
| 14 | PC1 | OE_N_P1 | uscita: EN del PI3USB4002A porta 1 (basso = dati collegati; pull-up) |
| 15 | PC2 | DIS_P1 | uscita: scarica VBUS porta 1 (alto = scarica; pull-down) |
| 16 | PC3 | FAULT_P1 | ingresso: FAULT load switch porta 1 (basso = sovracorrente; anche OCS1_N del hub) |
| 17 | PA0 | CC1_ADC | ADC: tensione CC1 ingresso alimentazione |
| 18 | PA1 | CC2_ADC | ADC: tensione CC2 |
| 19 | PA2 | UART_TX | USART2 TX, J9 |
| 20 | PA3 | UART_RX | USART2 RX, J9 |
| 21 | PA4 | RLY1 | uscita: relè 1 (alto = linea staccata) |
| 22 | PA5 | RLY2 | uscita: relè 2 |
| 23 | PA6 | — |  |
| 24 | PA7 | EN_P4 | uscita: EN load switch porta 4 (alto = VBUS on; pull-down) |
| 25 | PC4 | EN_P2 | uscita: EN load switch porta 2 (alto = VBUS on; pull-down) |
| 26 | PC5 | OE_N_P2 | uscita: EN del PI3USB4002A porta 2 (basso = dati collegati; pull-up) |
| 27 | PB0 | — |  |
| 28 | PB1 | EFUSE_FLT_N | ingresso: guasto dell'eFuse d'ingresso U13 (basso = guasto; pull-up R34) |
| 29 | PB2 | VBUS_DET | ADC: VBUS upstream / 2 (anche VBUS_DET del hub); pin FT, regge ~2,5 V a MCU spento |
| 30 | PB10 | — |  |
| 31 | PB11 | — |  |
| 32 | PB12 | BTN_RLY1 | ingresso: pulsante relè 1 (basso = premuto; pull-up interno) |
| 33 | PB13 | BTN_RLY2 | ingresso: pulsante relè 2 (basso = premuto; pull-up interno) |
| 34 | PB14 | — |  |
| 35 | PB15 | LED_FLT_P5 | uscita: LED rosso porta 5 (alto = acceso) |
| 36 | PA8 | LED_HOST | uscita: LED host collegato (alto = acceso) |
| 37 | PA9/UCPD1_DBCC1 | UCPD1_DBCC1 | pull-down R48 10 kΩ (dead battery disattivato) |
| 38 | PC6 | DIS_P2 | uscita: scarica VBUS porta 2 (alto = scarica; pull-down) |
| 39 | PC7 | FAULT_P2 | ingresso: FAULT load switch porta 2 (basso = sovracorrente; anche OCS2_N del hub) |
| 40 | PD8 | DIS_P5 | uscita: scarica VBUS porta 5 (alto = scarica; pull-down) |
| 41 | PD9 | FAULT_P5 | ingresso: FAULT load switch porta 5 (basso = sovracorrente; anche OCS5_N del hub) |
| 42 | PA10/UCPD1_DBCC2 | UCPD1_DBCC2 | pull-down R49 10 kΩ (dead battery disattivato) |
| 43 | PA9/PA11 | USB_MCU_DM | USB FS D- verso porta 6 del hub |
| 44 | PA10/PA12 | USB_MCU_DP | USB FS D+ verso porta 6 del hub |
| 45 | PA13 | SWDIO | SWD |
| 46 | PA14 | SWCLK | SWD (anche BOOT0 via nBOOT_SEL) |
| 47 | PA15 | HUB_CFG_SEL2 | uscita: CFG_SEL2 del hub, campionato al rilascio del reset (0 = SMBus, 1 = default interni) |
| 48 | PC8 | EN_P3 | uscita: EN load switch porta 3 (alto = VBUS on; pull-down) |
| 49 | PC9 | OE_N_P3 | uscita: EN del PI3USB4002A porta 3 (basso = dati collegati; pull-up) |
| 50 | PD0 | LED_FLT_P4 | uscita: LED rosso porta 4 (alto = acceso) |
| 51 | PD1 | OE_N_P4 | uscita: EN del PI3USB4002A porta 4 (basso = dati collegati; pull-up) |
| 52 | PD2 | DIS_P4 | uscita: scarica VBUS porta 4 (alto = scarica; pull-down) |
| 53 | PD3 | FAULT_P4 | ingresso: FAULT load switch porta 4 (basso = sovracorrente; anche OCS4_N del hub) |
| 54 | PD4 | BTN_P4 | ingresso: pulsante porta 4 (basso = premuto; pull-up interno) |
| 55 | PD5 | EN_P5 | uscita: EN load switch porta 5 (alto = VBUS on; pull-down) |
| 56 | PD6 | OE_N_P5 | uscita: EN del PI3USB4002A porta 5 (basso = dati collegati; pull-up) |
| 57 | PB3 | LED_FLT_P1 | uscita: LED rosso porta 1 (alto = acceso) |
| 58 | PB4 | LED_FLT_P2 | uscita: LED rosso porta 2 (alto = acceso) |
| 59 | PB5 | LED_FLT_P3 | uscita: LED rosso porta 3 (alto = acceso) |
| 60 | PB6 | — |  |
| 61 | PB7 | — |  |
| 62 | PB8 | SCL | I2C1: INA3221 (0x40, 0x41), hub SMBus (0x2C) |
| 63 | PB9 | SDA | I2C1 |
| 64 | PC10 | DIS_P3 | uscita: scarica VBUS porta 3 (alto = scarica; pull-down) |

## Note per il firmware

- USB FS senza quarzo: HSI48 + CRS sincronizzato sul SOF.
- I2C1 su PB8/PB9: INA3221 a 0x40 (A0 = GND, porte 1-3) e 0x41 (A0 = VS, porte 4-5 e corrente d'ingresso); hub USB2517 a 0x2C in modalità SMBus.
- PF1 (HUB_RST_N) va sempre configurato open-drain (mai push-pull alto): RESET_N è condiviso con il supervisore U12 e ha il pull-up R27.
- Sequenza di avvio del hub, dopo un'accensione o un calo di tensione del MCU (flag di RCC_CSR, oppure un flag in un registro di backup): tirare basso HUB_RST_N, impostare HUB_CFG_SEL2, verificare SCL alto (bus I2C fermo), rilasciare il reset e **aspettare che la linea risalga davvero** (lettura di PF1 in ingresso: U12 può tenerla bassa fino a 700 ms dopo che la 3,3 V è tornata); solo allora, se CFG_SEL2 = 0, scrivere i registri del hub via SMBus e poi il comando di attach (registro FFh). Il MCU parte prima che U12 rilasci il reset (150-700 ms), quindi il hub enumera una volta sola.
- Reset del solo hub (D35): un calo della 3,3 V tra ~1,8 e ~2,9 V fa scattare U12 ma non resetta il MCU, che funziona fino a 1,7 V (il brownout reset è disattivato di default). Il firmware legge HUB_RST_N ogni ≤10 ms (PF1 è open-drain e il suo ingresso resta attivo; U12 tiene il reset almeno 150 ms). Niente interrupt: EXTI1 è condivisa con EFUSE_FLT_N (PB1). Se lo trova basso senza averlo chiesto: porta basso anche PF1, così il rilascio non dipende da U12, ferma l'I2C, aspetta almeno 1 s, poi ripete la sequenza di avvio (CFG_SEL2, SCL alto, rilascio, attesa del livello alto, configurazione SMBus). Così il rilascio non capita mai durante una transazione I2C: SCL è anche CFG_SEL0. Attivare il brownout reset del MCU (~2,8 V) non basterebbe: resterebbe la finestra tra 2,8 e 3,0 V.
- Dopo un reset software, un watchdog o il ritorno dal bootloader non toccare HUB_RST_N: il hub è già configurato e il MCU si ricollega da solo sulla porta 6.
- LED (D38): il verde di ogni porta è collegato al suo EN e il LED di ogni relè al suo comando RLY, quindi seguono lo stato senza firmware; "acceso" è sulla 3,3 V. Dal MCU (1 kΩ, ~1,2 mA) vengono i 5 rossi (LED_FLT_P1-P5) e "host collegato" (LED_HOST). Codici del rosso: fisso = sovracorrente; lampeggio lento (~1 Hz) = porta bloccata dal budget; lampeggio veloce (~4 Hz) = sorgente esterna o tensione inversa. I 6 pin sono scelti perché in DFU restino spenti: PB3-PB5 non sono toccati dal bootloader, PA8/PB15/PD0 hanno la pull-down Rd dell'UCPD.
- I pulsanti delle porte 1-3 sono su PC13/PC14/PC15, pin a bassa capacità di pilotaggio: vanno usati solo come ingressi.
- FAULT_P1-P5: abilitare il pull-up interno. Il pull-up degli OCS_N del hub potrebbe mancare mentre il hub è in reset.
- SCL è anche CFG_SEL0 del hub, campionato al rilascio di HUB_RST_N: in quel momento il bus I2C dev'essere fermo (SCL alto).
- UCPD (D34): dopo ogni reset i pin CC dell'UCPD (PA8, PB15, PD0, PD2) hanno una pull-down Rd da ~5,1 kΩ, finché il firmware non imposta UCPD1_STROBE e UCPD2_STROBE in SYSCFG_CFGR1 (RM0444, §7.3.16 e §38.4.6). Il bootloader non la stacca. Il firmware la stacca all'avvio, prima di pilotare questi pin. Sono usati tutti in uscita, con la pull-down nello stato sicuro: PD2 (DIS_P4, scarica spenta), PA8, PB15 e PD0 (LED host e rossi delle porte 5 e 4, spenti). I DBCC dell'UCPD1 (PA9, PA10) hanno pull-down da 10 kΩ e non direttamente a massa, perché il bootloader pilota PA9 alto (USART1_TX); i DBCC dell'UCPD2 sono PD1 e PD3, con pull-up verso la 3,3 V: a scheda spenta sono comunque bassi.
- Bootloader ST (AN2606, tabella 111; D34): in DFU il bootloader configura USART1 (PA9/PA10), USART2 (PA2/PA3), USART3 (PC10/PC11), I2C1 (PB6/PB7), I2C2 (PB10/PB11), SPI1 (PA4-PA7), SPI2 (PB12-PB15), FDCAN1 (PD0/PD1) e USB. Regola: le uscite che comandano reset, alimentazioni o relè stanno su pin che il bootloader non tocca o tiene bassi. RLY1/RLY2 (PA4/PA5) ed EN_P4 (PA7) hanno la pull-down di SPI1, quindi relè a riposo e porta spenta; HUB_RST_N (PF1), VBUS_DET (PB2) e HUB_CFG_SEL2 (PA15) non vengono toccati. Effetti innocui: DIS_P3 (PC10, USART3_TX) va alto e scarica una porta già spenta, OE_N_P4 (PD1, FDCAN_TX) va alto e stacca i dati di una porta già spenta, PB6 è libero. Con il bootloader si può programmare anche dalla UART J9 (USART2).
- Ingresso alimentazione: EFUSE_FLT_N (PB1) basso = eFuse in limitazione di corrente da più del tempo impostato (o sovratemperatura, guasto ILM, corrente inversa); con sovratensione o sottotensione FLT resta alto. La logica è a valle dell'eFuse (D32): quando l'eFuse stacca il MCU si spegne, quindi FLT serve solo durante la limitazione, per spegnere subito le porte. Corrente totale e tensione 5V_SYS si leggono dal canale 3 di U8 (shunt R5, dopo l'eFuse: D40); la tensione a monte dell'eFuse non si misura.
- Corrente delle porte, azzeramento (D36): a porta spenta nello shunt non passa corrente, quindi la lettura del canale dell'INA3221 è il suo offset (fino a ±80 µV = ±4 mA). Il firmware la memorizza a ogni spegnimento e la sottrae alle letture successive.
- Scarica della VBUS (D37): dopo EN off accendere DIS e leggere la VBUS della porta dall'INA3221; sotto 0,8 V tenere DIS ancora ~50 ms e poi spegnerla. Timeout 1 s: DIS off comunque (la 100 Ω in 1206 a 5 V è al suo massimo di 0,25 W), porta spenta, LED rosso, PORT_FAULT "sorgente esterna". Nel CYCLE_PORT `off_ms` parte quando la VBUS è sotto 0,8 V. Il tempo di conversione dell'INA3221 va scelto in modo che ogni canale si aggiorni almeno ogni ~5 ms durante la scarica.
- Porte: l'INA3221 misura anche la tensione di ogni VBUS. Se una porta accesa supera ~5,5 V (scheda in prova che rimanda tensione) spegnerla e segnalarlo (PORT_FAULT, causa "tensione inversa").
- Porte, sorgente esterna (D29): prima di accendere una porta leggere la sua VBUS dall'INA3221; se a porta spenta c'è già tensione (soglia 0,8 V, la stessa della scarica: D37) non accenderla. A porta accesa, se la corrente misurata è entrante (shunt negativo) spegnerla. In entrambi i casi LED rosso della porta acceso e PORT_FAULT con causa "sorgente esterna".
- VBUS_DET è su un pin FT (PB2), non TT: può avere tensione con il MCU spento (alimentatore staccato, PC collegato), e i pin TT non ammettono corrente iniettata verso VDD.
- I nomi dei pin 43/44 sono quelli del simbolo KiCad: sono PA11/PA12 (USB), rimappabili su PA9/PA10.
