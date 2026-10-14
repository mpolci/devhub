# Piedinatura del MCU (STM32G0B1RET6, LQFP-64)

**Stato hardware:** la tabella resta allineata allo schema esistente, ancora da adeguare a
D60–D66. CC1_ADC, CC2_ADC e INA_ALERT sono da eliminare; i segnali di comando porta e LED
vanno riesaminati dopo la scelta del controller. Non sono già liberi nello schema.
D66 elimina inoltre i segnali della porta 5 e del secondo canale relè; il canale fisico
da conservare è da definire. La tabella non anticipa la nuova assegnazione; la proposta a 48 pin è riportata
in fondo, separata dal pinout effettivamente cablato.
VBUS_DET upstream, USB, SWD e gestione dei pin UCPD/bootloader restano necessari.

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
| 7 | VREF+ | +3V3 | C42 100 nF (D57) |
| 8 | VDD | +3V3 | C41 100 nF + C40 4,7 µF verso il pin 9 (D57) |
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
| 29 | PB2 | VBUS_DET | ADC: VBUS upstream / 2 (anche VBUS_DET del hub); pin FT, regge ~2,5 V a MCU spento (D32, D53) |
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
| 43 | PA9/PA11 | USB_MCU_DN | USB FS D- verso porta 6 del hub |
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
- I2C1 su PB8/PB9: hub USB2517 a 0x2C in modalità SMBus. U7/U8 sono ancora cablati agli indirizzi 0x40/0x41 nello schema, ma non sono richiesti dal firmware semplificato.
- PF1 (HUB_RST_N) va sempre configurato open-drain (mai push-pull alto): RESET_N è condiviso con il supervisore U12 e ha il pull-up R27.
- Sequenza di avvio del hub, dopo un'accensione o un calo di tensione del MCU (flag di RCC_CSR, oppure un flag in un registro di backup): tirare basso HUB_RST_N, impostare HUB_CFG_SEL2, verificare SCL alto (bus I2C fermo), rilasciare il reset e **aspettare che la linea risalga davvero** (lettura di PF1 in ingresso: U12 può tenerla bassa fino a 700 ms dopo che la 3,3 V è tornata); solo allora, se CFG_SEL2 = 0, scrivere i registri del hub via SMBus e poi il comando di attach (registro FFh). Il MCU parte prima che U12 rilasci il reset (150-700 ms), quindi il hub enumera una volta sola.
- Reset del solo hub (D35): un calo della 3,3 V tra ~1,8 e ~2,9 V fa scattare U12 ma non resetta il MCU, che funziona fino a 1,7 V (il brownout reset è disattivato di default). Il firmware legge HUB_RST_N ogni ≤10 ms (PF1 è open-drain e il suo ingresso resta attivo; U12 tiene il reset almeno 150 ms). Niente interrupt: EXTI1 è condivisa con EFUSE_FLT_N (PB1). Se lo trova basso senza averlo chiesto: porta basso anche PF1, così il rilascio non dipende da U12, ferma l'I2C, aspetta almeno 1 s, poi ripete la sequenza di avvio (CFG_SEL2, SCL alto, rilascio, attesa del livello alto, configurazione SMBus). Così il rilascio non capita mai durante una transazione I2C: SCL è anche CFG_SEL0. Attivare il brownout reset del MCU (~2,8 V) non basterebbe: resterebbe la finestra tra 2,8 e 3,0 V.
- Dopo un reset software, un watchdog o il ritorno dal bootloader non toccare HUB_RST_N: il hub è già configurato e il MCU si ricollega da solo sulla porta 6.
- LED USB (D63): dopo l’adeguamento, un GPIO per porta pilota il LED unico: spento = disabilitata, fisso = abilitata, lampeggiante = guasto. I collegamenti EN/LED_FLT della tabella sono ancora quelli dello schema da modificare; resta un LED relè, oltre ad acceso e host.
- I pulsanti delle porte 1-3 sono su PC13/PC14/PC15, pin a bassa capacità di pilotaggio: vanno usati solo come ingressi.
- FAULT_P1-P5: abilitare il pull-up interno. Il pull-up degli OCS_N del hub potrebbe mancare mentre il hub è in reset.
- SCL è anche CFG_SEL0 del hub, campionato al rilascio di HUB_RST_N: in quel momento il bus I2C dev'essere fermo (SCL alto).
- UCPD (D34): dopo ogni reset i pin CC dell'UCPD (PA8, PB15, PD0, PD2) hanno una pull-down Rd da ~5,1 kΩ, finché il firmware non imposta UCPD1_STROBE e UCPD2_STROBE in SYSCFG_CFGR1 (RM0444, §7.3.16 e §38.4.6). Il bootloader non la stacca. Il firmware la stacca all'avvio, prima di pilotare questi pin. Sono usati tutti in uscita, con la pull-down nello stato sicuro: PD2 (DIS_P4, scarica spenta), PA8, PB15 e PD0 (LED host e rossi delle porte 5 e 4, spenti). I DBCC dell'UCPD1 (PA9, PA10) hanno pull-down da 10 kΩ e non direttamente a massa, perché il bootloader pilota PA9 alto (USART1_TX); i DBCC dell'UCPD2 sono PD1 e PD3, con pull-up verso la 3,3 V: a scheda spenta sono comunque bassi.
- Bootloader ST (AN2606, tabella 111; D34): in DFU il bootloader configura USART1 (PA9/PA10), USART2 (PA2/PA3), USART3 (PC10/PC11), I2C1 (PB6/PB7), I2C2 (PB10/PB11), SPI1 (PA4-PA7), SPI2 (PB12-PB15), FDCAN1 (PD0/PD1) e USB. Regola: le uscite che comandano reset, alimentazioni o relè stanno su pin che il bootloader non tocca o tiene bassi. RLY1/RLY2 (PA4/PA5) ed EN_P4 (PA7) hanno la pull-down di SPI1, quindi relè a riposo e porta spenta; HUB_RST_N (PF1), VBUS_DET (PB2) e HUB_CFG_SEL2 (PA15) non vengono toccati. Effetti innocui: DIS_P3 (PC10, USART3_TX) va alto e scarica una porta già spenta, OE_N_P4 (PD1, FDCAN_TX) va alto e stacca i dati di una porta già spenta, PB6 è libero. Con il bootloader si può programmare anche dalla UART J9 (USART2).
- Ingresso alimentazione: EFUSE_FLT_N (PB1) segnala guasti hardware; con OVLO/UVLO può restare alto. Il MCU può gestirlo solo finché è alimentato: non si garantisce un evento prima dello spegnimento dell’hub. Nessuna lettura di corrente totale, 5V_SYS o CC d’ingresso (D60/D65).
- Ciclo (D61/D62): durata dal comando di disabilitazione VBUS, inclusa la scarica, senza letture INA3221 o soglia 0,8 V. Default indicativo 1 s; durata della scarica e intervallo ammesso da validare sul controller. Nessun ritardo dati/VBUS configurabile e nessuna diagnosi firmware di sorgente esterna tramite misura.
- VBUS_DET è su un pin FT (PB2), non TT: può avere tensione con il MCU spento (alimentatore staccato, PC collegato), e i pin TT non ammettono corrente iniettata verso VDD.
- I nomi dei pin 43/44 sono quelli del simbolo KiCad: sono PA11/PA12 (USB), rimappabili su PA9/PA10.

## Pinout proposto STM32G0B1CEU6 UFQFPN48

**MCU confermato (D78); assegnazione proposta non ancora applicata a KiCad.**
L’adeguamento dello schema è a cura dell’utente. La tabella a
64 pin sopra resta il riferimento dello schema esistente. Questa assegnazione usa
USB2517I-JZX, due UCS2114-1-V/LX e quattro PI3USB4002A; nessun header USB di scorta.

Verifica su DS13560 Rev. 6, figura 6 (versione **GP**, senza suffisso N), tabelle GPIO/AF,
AN2606 Rev. 70 e simbolo KiCad `STM32G0B1CEUx` (base `STM32G0B1C_B-C-E_Ux`).
I 48 terminali hanno la stessa numerazione della versione GP LQFP48; in UFQFPN si
aggiunge il pad esposto 49 a massa. Il footprint è diverso.

| Pin | Nome | Segnale proposto | Funzione / stato sicuro |
|---|---|---|---|
| 1 | PC13 | BTN_P1 | Ingresso con pull-up, pulsante a massa |
| 2 | PC14 | BTN_P2 | Ingresso con pull-up, nessun LSE |
| 3 | PC15 | BTN_P3 | Ingresso con pull-up, nessun LSE |
| 4 | VBAT | +3V3 | Nessuna batteria |
| 5 | VREF+ | +3V3 | Disaccoppiamento 100 nF |
| 6 | VDD/VDDA | +3V3 | 100 nF + 4,7 µF vicino ai pin di alimentazione |
| 7 | VSS/VSSA | GND | Massa |
| 8 | PF0 | LED_HOST | GPIO; nessun HSE |
| 9 | PF1 | HUB_RST_N | Open-drain, wired-OR con supervisore; pull-up esterno |
| 10 | PF2/NRST | NRST | SWD e condensatore di reset |
| 11 | PA0 | EN_P1 | Alto = VBUS abilitata; pull-down esterno |
| 12 | PA1 | EN_P2 | Alto = VBUS abilitata; pull-down esterno |
| 13 | PA2 | UART_TX | USART2_TX, AF1, header UART mantenuto |
| 14 | PA3 | UART_RX | USART2_RX, AF1 |
| 15 | PA4 | FAULT_P4 | ALERT UCS_B canale 2, EXTI4, pull-up esterno |
| 16 | PA5 | LED_P1 | Alto = acceso |
| 17 | PA6 | — | Libero; in ROM bootloader SPI1_MISO può pilotare alto |
| 18 | PA7 | LED_P2 | Alto = acceso |
| 19 | PB0 | FAULT_P1 | ALERT UCS_A canale 1, EXTI0, pull-up esterno |
| 20 | PB1 | EFUSE_FLT_N | Ingresso eFuse, EXTI1, pull-up esterno |
| 21 | PB2 | VBUS_DET | Upstream / 2, ADC_IN10, pin FT_ea |
| 22 | PB10 | I2C2_SCL | AF6, UCS_B a 0x57 |
| 23 | PB11 | I2C2_SDA | AF6, UCS_B a 0x57 |
| 24 | PB12 | BTN_P4 | Ingresso con pull-up, pulsante a massa |
| 25 | PB13 | BTN_RLY1 | Ingresso con pull-up, pulsante a massa |
| 26 | PB14 | OE_N_P3 | Alto = dati scollegati; pull-up esterno |
| 27 | PB15 | LED_P3 | Alto = acceso; disattivare Rd UCPD1 |
| 28 | PA8 | LED_P4 | Alto = acceso; disattivare Rd UCPD1 |
| 29 | PA9/UCPD1_DBCC1 | UCPD1_DBCC1 | Riservato, 10 kΩ a massa, non cortocircuitare a GND |
| 30 | PC6 | EN_P3 | Alto = VBUS abilitata; pull-down esterno |
| 31 | PC7 | EN_P4 | Alto = VBUS abilitata; pull-down esterno |
| 32 | PA10/UCPD1_DBCC2 | UCPD1_DBCC2 | Riservato, 10 kΩ a massa |
| 33 | PA11 [PA9] | USB_MCU_DN | USB D−, hub DS6; nessuna rimappatura PA9 |
| 34 | PA12 [PA10] | USB_MCU_DP | USB D+, hub DS6; nessuna rimappatura PA10 |
| 35 | PA13 | SWDIO | Debug/programmazione |
| 36 | PA14/BOOT0 | SWCLK | Debug; ingresso BOOT0 secondo option byte |
| 37 | PA15 | HUB_CFG_SEL2 | Pull-up: default ROM hub; basso: configurazione SMBus |
| 38 | PD0 | — | Libero; UCPD2_CC1 / FDCAN_RX nel bootloader |
| 39 | PD1 | OE_N_P4 | Alto = dati scollegati; pull-up esterno, anche UCPD2_DBCC1 |
| 40 | PD2 | RLY1 | Alto = bobina eccitata, COM/NC aperto; pull-down esterno, Rd UCPD2 da disattivare |
| 41 | PD3 | FAULT_P2 | ALERT UCS_A canale 2, EXTI3; pull-up esterno, anche UCPD2_DBCC2 |
| 42 | PB3 | OE_N_P1 | Alto = dati scollegati; pull-up esterno |
| 43 | PB4 | OE_N_P2 | Alto = dati scollegati; pull-up esterno |
| 44 | PB5 | FAULT_P3 | ALERT UCS_B canale 1, EXTI5; pull-up esterno |
| 45 | PB6 | — | Libero; I2C1_SCL nel bootloader ROM |
| 46 | PB7 | — | Libero; I2C1_SDA nel bootloader ROM |
| 47 | PB8 | I2C1_SCL | AF6, UCS_A a 0x57 e USB2517 a 0x2C |
| 48 | PB9 | I2C1_SDA | AF6, UCS_A a 0x57 e USB2517 a 0x2C |
| 49 (EP) | VSS | GND | Pad esposto: saldatura a massa obbligatoria |

### Bilancio e collegamenti

**37 segnali funzionali + 2 DBCC riservati + 4 GPIO liberi + 4 alimentazioni + NRST =
48 terminali**, oltre al pad esposto. I 37 segnali comprendono 24 per le porte (EN, OE,
FAULT, LED, pulsanti e quattro fili I²C), relè e pulsante relè, LED host, guasto eFuse,
VBUS upstream, due controlli hub, UART, USB e SWD.

- I²C1: UCS_A per porte 1/2 e hub; I²C2: UCS_B per porte 3/4. Indirizzi a 7 bit.
  Una coppia di pull-up a 3,3 V per bus; 4,7 kΩ come punto iniziale a 100 kHz,
  da verificare con la capacità reale. Nessun conflitto fra i due dispositivi a 0x57.
- Gli ALERT dei quattro canali raggiungono anche OCS1_N…OCS4_N del USB2517;
  PRTPWR non comanda UCS2114. Pull-up ALERT esterni rendono il segnale definito
  anche con hub in reset. Il MCU legge i registri UCS per distinguere i guasti.
- EXTI0, EXTI3, EXTI5, EXTI4 per le quattro porte, EXTI1 per eFuse: nessun conflitto.
  Pulsanti e HUB_RST_N letti in polling; non assegnare EXTI1 anche a PF1.
- LED relè collegato al comando/driver relè, LED alimentazione passivo: nessun GPIO
  aggiuntivo. Correnti dei cinque LED MCU da dimensionare entro i limiti per pin e totali.
- Nessun GPIO DIS: la scarica UCS2114 è configurata all’avvio e parte automaticamente.
  SWD e UART restano; l’eliminazione richiesta riguarda solo l’header USB di scorta.

### Reset, avvio e bootloader

Le uscite di potenza hanno pull-down e gli switch dati pull-up esterni: **durante reset
VBUS disabilitata, dati scollegati, relè a riposo**. Dopo inizializzazione riuscita il
firmware abilita tutte le porte con accensioni sfalsate, salvo default salvati (D74).
Precaricare i registri di uscita prima di configurare i GPIO come uscite.

Impostare UCPD1_STROBE e UCPD2_STROBE prima di usare PA8/PB15/PD2; la Rd del
bootloader su PD2 mantiene il relè spento. Il relè è volutamente spostato su PD2,
mentre PA4 è un ingresso guasto: nessuna uscita seriale della ROM può eccitare la bobina.
PA4 è inizializzato dalla ROM come SPI1_NSS con pull-down; nel firmware rimuovere quel
pull-down e verificare gli ALERT prima di abilitare gli interrupt.

SPI1_MISO su PA6 resta libero. SPI2_MISO su PB14 e FDCAN_TX su PD1 possono andare alti
nel bootloader: sono assegnati a OE_N e mantengono i dati scollegati. Gli EN PA0/PA1/PC6/PC7
non sono usati dalle periferiche ROM. I LED possono avere transitori innocui durante DFU.
USART1_TX su PA9 richiede il resistore DBCC da 10 kΩ, non un collegamento diretto a massa.

In ROM I2C2 usa PB10/PB11 come **slave**, non invia configurazioni allo UCS_B. I2C1 ROM
usa PB6/PB7, lasciati liberi; il bus applicativo PB8/PB9 non è usato dalla ROM.
Il hub deve poter partire dai default quando il MCU è vuoto o in DFU: CFG_SEL2 alto
per pull-up e reset rilasciato dal supervisore. Nel firmware conservare la sequenza
SMBus già descritta, con bus fermo durante il rilascio del reset. Non resettare il hub
nel normale salto al bootloader o al ritorno, per preservare il collegamento USB interno.

PB2 mantiene il rilevamento upstream su pin FT: verificare il partitore alla massima
VBUS upstream e con VDD = 0; nessun pull-up interno. USB usa HSI48 + CRS, senza HSE.
Restano da verificare sullo schema definitivo e sul prototipo i transitori di avvio,
la scarica con sorgente esterna, l’ingresso DFU, la corrente dei LED e i tempi I²C.
La verifica del pinout non sostituisce ERC e confronto netlist dopo le modifiche KiCad.
