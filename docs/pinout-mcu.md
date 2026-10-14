# Piedinatura del MCU (STM32G0B1CEU6, UFQFPN48)

Corrisponde allo schema `hardware/devhub/mcu.kicad_sch` (U3): in caso di modifiche allo schema
va aggiornata a mano. I pin senza segnale sono liberi (no-connect nello schema).

MCU scelto con D78: versione **GP** in UFQFPN48, non la variante N, in cui i pin 30/31
diventano alimentazioni invece di PC6/PC7. Verifica su DS13560 Rev. 6, figura 6, tabelle
GPIO/AF, AN2606 Rev. 70 e simbolo KiCad `STM32G0B1CEUx` (base `STM32G0B1C_B-C-E_Ux`).
I 48 terminali hanno la stessa numerazione della versione GP LQFP48; in UFQFPN si aggiunge
il pad esposto 49 a massa.

| Pin | Nome | Segnale | Funzione / stato sicuro |
|---|---|---|---|
| 1 | PC13 | BTN_P1 | ingresso: pulsante porta 1 (basso = premuto; pull-up interno) |
| 2 | PC14 | BTN_P2 | ingresso: pulsante porta 2 (pull-up interno), nessun LSE |
| 3 | PC15 | BTN_P3 | ingresso: pulsante porta 3 (pull-up interno), nessun LSE |
| 4 | VBAT | +3V3 | nessuna batteria, collegato a VDD senza condensatore |
| 5 | VREF+ | +3V3 | C42 100 nF |
| 6 | VDD/VDDA | +3V3 | C41 100 nF + C40 4,7 µF |
| 7 | VSS/VSSA | GND |  |
| 8 | PF0 | LED_HOST | uscita: LED host collegato (alto = acceso); nessun HSE |
| 9 | PF1 | HUB_RST_N | uscita open-drain: RESET_N del hub (basso = reset), in wired-OR con il supervisore U12, pull-up R27 |
| 10 | PF2/NRST | NRST | reset (SWD J4 pin 10, C46) |
| 11 | PA0 | EN_P1 | uscita: PWR_EN1 di U4, porta 1 (alto = VBUS abilitata; pull-down R42) |
| 12 | PA1 | EN_P2 | uscita: PWR_EN2 di U4, porta 2 (pull-down R43) |
| 13 | PA2 | UART_TX | USART2_TX (AF1), J9 |
| 14 | PA3 | UART_RX | USART2_RX (AF1), J9 |
| 15 | PA4 | FAULT_P4 | ingresso: ALERT#2 di U5, porta 4 (basso = guasto; pull-up R65), EXTI4 |
| 16 | PA5 | LED_P1 | uscita: LED porta 1 (alto = acceso) |
| 17 | PA6 | — | libero; nel bootloader SPI1_MISO può andare alto |
| 18 | PA7 | LED_P2 | uscita: LED porta 2 (alto = acceso) |
| 19 | PB0 | FAULT_P1 | ingresso: ALERT#1 di U4, porta 1 (pull-up R62), EXTI0 |
| 20 | PB1 | EFUSE_FLT_N | ingresso: guasto dell'eFuse d'ingresso U13 (basso = guasto; pull-up R34), EXTI1 |
| 21 | PB2 | VBUS_DET | ADC_IN10: VBUS upstream / 2 (anche VBUS_DET del hub); pin FT, regge ~2,5 V a MCU spento (D32, D53) |
| 22 | PB10 | I2C2_SCL | I2C2 (AF6): U5 a 0x57; pull-up R50 |
| 23 | PB11 | I2C2_SDA | I2C2 (AF6); pull-up R51 |
| 24 | PB12 | BTN_P4 | ingresso: pulsante porta 4 (pull-up interno) |
| 25 | PB13 | BTN_RLY1 | ingresso: pulsante del relè (pull-up interno) |
| 26 | PB14 | OE_N_P3 | uscita: EN del PI3USB4002A porta 3 (alto = dati scollegati; pull-up R304) |
| 27 | PB15 | LED_P3 | uscita: LED porta 3 (alto = acceso); Rd dell'UCPD1 da disattivare |
| 28 | PA8 | LED_P4 | uscita: LED porta 4 (alto = acceso); Rd dell'UCPD1 da disattivare |
| 29 | PA9/UCPD1_DBCC1 | UCPD1_DBCC1 | pull-down R48 10 kΩ (dead battery disattivato), non a massa diretta |
| 30 | PC6 | EN_P3 | uscita: PWR_EN1 di U5, porta 3 (pull-down R44) |
| 31 | PC7 | EN_P4 | uscita: PWR_EN2 di U5, porta 4 (pull-down R45) |
| 32 | PA10/UCPD1_DBCC2 | UCPD1_DBCC2 | pull-down R49 10 kΩ |
| 33 | PA11 [PA9] | USB_MCU_DN | USB FS D− verso la porta 6 del hub; nessuna rimappatura su PA9 |
| 34 | PA12 [PA10] | USB_MCU_DP | USB FS D+ verso la porta 6 del hub; nessuna rimappatura su PA10 |
| 35 | PA13 | SWDIO | SWD |
| 36 | PA14/BOOT0 | SWCLK | SWD; ingresso BOOT0 secondo gli option byte |
| 37 | PA15 | HUB_CFG_SEL2 | uscita: CFG_SEL2 del hub, campionato al rilascio del reset (0 = SMBus, 1 = default interni); pull-up R29 |
| 38 | PD0 | — | libero; UCPD2_CC1 e FDCAN_RX nel bootloader |
| 39 | PD1 | OE_N_P4 | uscita: EN del PI3USB4002A porta 4 (pull-up R404); anche UCPD2_DBCC1 |
| 40 | PD2 | RLY1 | uscita: relè (alto = bobina eccitata, linea COM/NC staccata); Rd dell'UCPD2 a riposo |
| 41 | PD3 | FAULT_P2 | ingresso: ALERT#2 di U4, porta 2 (pull-up R63), EXTI3; anche UCPD2_DBCC2 |
| 42 | PB3 | OE_N_P1 | uscita: EN del PI3USB4002A porta 1 (pull-up R104) |
| 43 | PB4 | OE_N_P2 | uscita: EN del PI3USB4002A porta 2 (pull-up R204) |
| 44 | PB5 | FAULT_P3 | ingresso: ALERT#1 di U5, porta 3 (pull-up R64), EXTI5 |
| 45 | PB6 | — | libero; I2C1_SCL del bootloader |
| 46 | PB7 | — | libero; I2C1_SDA del bootloader |
| 47 | PB8 | I2C1_SCL | I2C1 (AF6): hub U2 a 0x2C e U4 a 0x57; pull-up R40; anche CFG_SEL0 del hub |
| 48 | PB9 | I2C1_SDA | I2C1 (AF6); pull-up R41 |
| 49 (EP) | VSS | GND | pad esposto: saldatura a massa obbligatoria |

## Bilancio e collegamenti

**37 segnali funzionali + 2 DBCC riservati + 4 GPIO liberi + 4 alimentazioni + NRST =
48 terminali**, oltre al pad esposto. I 37 segnali comprendono 24 per le porte (EN, OE_N,
FAULT, LED, pulsanti e quattro fili I²C), relè e pulsante del relè, LED host, guasto eFuse,
VBUS upstream, due controlli del hub, UART, USB e SWD.

- I²C1: U4 (porte 1/2) e hub; I²C2: U5 (porte 3/4). Indirizzi a 7 bit. Pull-up a 3,3 V
  da 4,7 kΩ per bus, da verificare con la capacità reale a 100 kHz. Nessun conflitto fra i
  due UCS2114, entrambi a 0x57 su bus diversi.
- Gli ALERT# dei quattro canali raggiungono anche OCS1_N…OCS4_N del USB2517; i PRTPWR del
  hub non comandano gli UCS2114. I pull-up esterni da 10 kΩ (R62–R65) rendono il segnale
  definito anche con il hub in reset. Il MCU legge i registri UCS2114 per distinguere i guasti.
- EXTI0, EXTI3, EXTI4, EXTI5 per le quattro porte, EXTI1 per l'eFuse: nessun conflitto.
  Pulsanti e HUB_RST_N letti in polling; non assegnare EXTI1 anche a PF1.
- Unico LED di sistema: host collegato (D93) su LED_HOST; nessun LED del relè né di
  accensione (D82). Correnti dei cinque LED pilotati dal MCU da verificare entro i limiti per
  pin e totali.
- Nessun GPIO DIS: la scarica degli UCS2114 è configurata all'avvio e parte automaticamente.

## Note per il firmware

- USB FS senza quarzo: HSI48 + CRS sincronizzato sul SOF.
- I2C1 su PB8/PB9: hub USB2517 a 0x2C in modalità SMBus e U4; I2C2 su PB10/PB11: U5.
- UCS2114 (D75): all'avvio PWR_EN bassi e dati aperti, risveglio dei controller, abilitazione
  di EN_VBUS_DISCHG con timer 400 ms e verifica per lettura, poi accensione sfalsata delle
  porte. Charge rationing e override software dei pin restano disabilitati; niente comandi
  DISCHG_LOAD senza timer. Dopo una perdita di alimentazione dei controller reinizializzare.
- PF1 (HUB_RST_N) va sempre configurato open-drain (mai push-pull alto): RESET_N è condiviso con il supervisore U12 e ha il pull-up R27.
- Sequenza di avvio del hub, dopo un'accensione o un calo di tensione del MCU (flag di RCC_CSR, oppure un flag in un registro di backup): tirare basso HUB_RST_N, impostare HUB_CFG_SEL2, verificare I2C1_SCL alto (bus I2C1 fermo), rilasciare il reset e **aspettare che la linea risalga davvero** (lettura di PF1 in ingresso: U12 può tenerla bassa fino a 700 ms dopo che la 3,3 V è tornata); solo allora, se CFG_SEL2 = 0, scrivere i registri del hub via SMBus, compresa la disabilitazione delle porte 5 e 7, e poi il comando di attach (registro FFh). Il MCU parte prima che U12 rilasci il reset (150-700 ms), quindi il hub enumera una volta sola.
- Reset del solo hub (D35): un calo della 3,3 V tra ~1,8 e ~2,9 V fa scattare U12 ma non resetta il MCU, che funziona fino a 1,7 V (il brownout reset è disattivato di default). Il firmware legge HUB_RST_N ogni ≤10 ms (PF1 è open-drain e il suo ingresso resta attivo; U12 tiene il reset almeno 150 ms). Niente interrupt: EXTI1 è usata da EFUSE_FLT_N (PB1). Se lo trova basso senza averlo chiesto: porta basso anche PF1, così il rilascio non dipende da U12, ferma l'I2C1, aspetta almeno 1 s, poi ripete la sequenza di avvio. Così il rilascio non capita mai durante una transazione I2C: SCL è anche CFG_SEL0. Attivare il brownout reset del MCU (~2,8 V) non basterebbe: resterebbe la finestra tra 2,8 e 3,0 V.
- Dopo un reset software, un watchdog o il ritorno dal bootloader non toccare HUB_RST_N: il hub è già configurato e il MCU si ricollega da solo sulla porta 6.
- LED delle porte (D63): spento = disabilitata, fisso = abilitata, lampeggiante = guasto, con priorità al guasto.
- I pulsanti delle porte 1-3 sono su PC13/PC14/PC15, pin a bassa capacità di pilotaggio: vanno usati solo come ingressi.
- FAULT_P1-P4: pull-up esterni da 10 kΩ, nessun pull-up interno necessario.
- Ingresso alimentazione: EFUSE_FLT_N (PB1) segnala guasti hardware; con OVLO/UVLO può restare alto. Il MCU può gestirlo solo finché è alimentato: non si garantisce un evento prima dello spegnimento dell'hub. Nessuna lettura di corrente totale, 5V_SYS o CC d'ingresso (D60/D65).
- Ciclo (D61/D62): durata dal comando di disabilitazione VBUS, inclusa la scarica, senza misure o soglia 0,8 V. Default indicativo 1 s; durata della scarica e intervallo ammesso da validare sul prototipo. Nessun ritardo dati/VBUS configurabile.
- VBUS_DET è su un pin FT (PB2), non TT: può avere tensione con il MCU spento (alimentatore staccato, PC collegato), e i pin TT non ammettono corrente iniettata verso VDD.
- I nomi dei pin 33/34 sono quelli del simbolo KiCad: sono PA11/PA12 (USB), rimappabili su PA9/PA10.

## Reset, avvio e bootloader

Le uscite di potenza hanno pull-down e gli switch dati pull-up esterni: **durante il reset
VBUS disabilitata, dati scollegati, relè a riposo**. Dopo l'inizializzazione il firmware
abilita tutte le porte con accensioni sfalsate, salvo default salvati (D74). Precaricare i
registri di uscita prima di configurare i GPIO come uscite.

UCPD (D34): dopo ogni reset i pin CC dell'UCPD (PA8, PB15, PD0, PD2) hanno una pull-down Rd
da ~5,1 kΩ, finché il firmware non imposta UCPD1_STROBE e UCPD2_STROBE in SYSCFG_CFGR1
(RM0444, §7.3.16 e §38.4.6). Il bootloader non la stacca. Il firmware la stacca all'avvio,
prima di pilotare PA8, PB15 e PD2. La Rd del bootloader su PD2 mantiene il relè spento: il
relè è volutamente su PD2, mentre PA4 è un ingresso guasto, quindi nessuna uscita seriale
della ROM può eccitare la bobina. I DBCC dell'UCPD1 (PA9, PA10) hanno pull-down da 10 kΩ e
non direttamente a massa, perché il bootloader pilota PA9 alto (USART1_TX); i DBCC
dell'UCPD2 sono PD1 e PD3, con pull-up verso la 3,3 V: a scheda spenta sono comunque bassi.

Bootloader ST (AN2606): PA4 è inizializzato dalla ROM come SPI1_NSS con pull-down; nel
firmware rimuovere quel pull-down e verificare gli ALERT# prima di abilitare gli interrupt.
SPI1_MISO su PA6 resta libero. SPI2_MISO su PB14 e FDCAN_TX su PD1 possono andare alti nel
bootloader: sono assegnati a OE_N e mantengono i dati scollegati. Gli EN PA0/PA1/PC6/PC7 non
sono usati dalle periferiche ROM. I LED possono avere transitori innocui durante il DFU.
Con il bootloader si può programmare anche dalla UART J9 (USART2).

In ROM I2C2 usa PB10/PB11 come **slave**, non invia configurazioni a U5. I2C1 della ROM usa
PB6/PB7, lasciati liberi; il bus applicativo PB8/PB9 non è usato dalla ROM. Il hub deve poter
partire dai default quando il MCU è vuoto o in DFU: CFG_SEL2 alto per il pull-up R29 e reset
rilasciato dal supervisore. Nel firmware conservare la sequenza SMBus descritta sopra, con il
bus fermo durante il rilascio del reset. Non resettare il hub nel normale salto al bootloader
o al ritorno, per preservare il collegamento USB interno.

PB2 mantiene il rilevamento upstream su pin FT: verificare il partitore alla massima VBUS
upstream e con VDD = 0; nessun pull-up interno. USB usa HSI48 + CRS, senza HSE. Restano da
verificare sul prototipo i transitori di avvio, la scarica con sorgente esterna, l'ingresso
DFU, la corrente dei LED e i tempi I²C.
