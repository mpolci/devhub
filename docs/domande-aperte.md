# Decisioni e domande aperte

## Decise

| # | Decisione | Scelta |
|---|---|---|
| D1 | Velocità | USB 2.0 Hi-Speed |
| D2 | Sezionamento | VBUS + D+/D- a stato solido; GND mai interrotta |
| D3 | Relè ausiliari | 2 canali, contatti max 16 V / 3 A, bobina 5 V; relè SPDT da 5–10 A, sul pannello **COM e NC** (2 boccole per canale): linea normalmente collegata, bobina eccitata solo per staccare; NO su ponticello a saldare interno |
| D4 | Misura corrente per porta | sì |
| D5 | Pulsanti fisici per porta | sì, uno per porta, più LED |
| D6 | MCU | STM32: **STM32G0B1RET6** (LQFP-64, USB FS + UCPD) |
| D7 | PCB e assemblaggio | JLCPCB 4 strati; **strategia C**: montaggio tutto a mano con piastra riscaldante e aria calda (stencil per i QFN), componenti da un unico fornitore; nessun tipo extended |
| D8 | Alimentazione | USB-C a **5 V / 3 A senza PD**; ~2,3 A totali per le porte, 2 A su una singola porta; budget letto da CC e gestito dal firmware con le misure di corrente |
| D9 | Upstream verso il PC | connettore USB-C |
| D10 | Strumento host | libreria Python generica (multipiattaforma via `hidapi`) + CLI per macOS costruita sopra |
| D11 | Fornitore | si decide alla fine, a componenti finalizzati |
| D12 | Licenze | hardware **CERN-OHL-W 2.0**; firmware, libreria e CLI **MIT** |
| D13 | VID/PID USB | VID 0x1209 (pid.codes) con PID di test durante lo sviluppo; PID definitivo da richiedere a pid.codes alla pubblicazione |
| D14 | Porta hub di scorta (DS7) | su header interno a 4 pin (VBUS, D-, D+, GND), non sezionata |
| D15 | Scatola | stampata in 3D su misura; tutti i connettori saldati sul PCB, boccole a banana in versione da PCB; connettori sui due lati corti (banco: porte, pulsanti, boccole; retro: upstream e alimentazione) |
| D16 | Reset del hub | supervisore **MCP120T-300** open-drain su RESET_N con pull-up, in wired-OR con il MCU (open-drain, ora su PF1: D34): il hub non dipende dal firmware e resta acceso durante reset e DFU del MCU |
| D17 | Regolatore 3,3 V | LDO **AP7361C-33E** (1 A, SOT-223) al posto dell'AP2112K in SOT-23-5 |
| D18 | Connettori USB | orizzontali (angolo retto, montati sopra il PCB): **GCT USB4105-GF-A** per tutti e sei gli USB-C (16 pin USB 2.0, 5 A, guscio con linguette passanti), **Kycon KUSBX-AS1N-B** passante per l'USB-A; scartati i modelli ruotati di 90° (rari e meno robusti) |
| D19 | Quarzo del hub | YXC **X322524MOB4SI** (24 MHz, 3225, C_L 12 pF, il più economico e con più disponibilità), C22/C23 = 15 pF C0G |
| D20 | Condensatori d'ingresso | invariati (2 × 220 µF + 2 × 10 µF): il picco all'inserzione lo elimina l'avvio dolce dell'eFuse |
| D21 | Protezione d'ingresso | eFuse **TPS259470A**: stacca sopra 5,84 V e sotto 4,0 V, limite 3,5 A, avvio dolce ~5 ms, FLT al MCU (PB1) |
| D22 | Switch dati delle porte | **PI3USB4002A** al posto del TS3USB30E: il lato connettore regge 24 V e si apre sopra 4,75 V (corti D+/D- su VBUS) |
| D23 | Fornitore | orientamento verso DigiKey (non ancora definitivo, vedi D11); le alternative trovate sono in `componenti.md` |
| D24 | LED per porta | due: alimentata (verde) e fault (rosso); lo stato delle linee dati non ha un LED |
| D25 | TVS | sull'ingresso **SMAJ20A** (non conduce fino a 20 V, l'eFuse gestisce le sovratensioni); sulle VBUS delle porte nessuna TVS e nessuna piazzola: bastano i 22 µF + 100 nF |
| D26 | Pulsanti | uno per porta e uno per relè (SW1/SW2 aggiunti); ogni pressione alterna lo stato: porta accesa/spenta con alimentazione e dati sempre insieme, linea del relè collegata/staccata (RF-17) |
| D27 | ESD delle porte downstream | tolte le USBLC6-2SC6 (D101-D501): il loro pin VBUS sulla VBUS commutata faceva passare corrente dai dati all'alimentazione della porta e portava le sovratensioni dei dati verso la 5 V; la protezione ESD è quella integrata nel PI3USB4002A (10 kV IEC, invece dei 15 kV dell'USBLC6). Revisione 02, rilievo R02-01 |
| D28 | Comando delle porte | alimentazione e dati sempre insieme (RF-3 cambiato): niente comandi separati per VBUS e dati né da PC né da pulsante; restano due interruttori distinti per poter rispettare la sequenza |
| D29 | Rilevamento dell'inserimento sulle porte USB-C | non fatto (opzione C, revisione 02, R02-02): VBUS presente appena la porta è accesa; il firmware non accende una porta con tensione già presente e spegne una porta che riceve corrente, con LED rosso e avviso al PC. Scartati un controller USB-C per porta (TPS25810) e la lettura delle CC dal MCU (richiedeva tutti i pin liberi e lasciava senza alimentazione le schede prive di resistenze CC) |
| D30 | Corrente annunciata dalle porte USB-C | standard USB (Rp 56 kΩ, 500 mA) al posto di 1,5 A (revisione 02, R02-03): annuncio coerente con il budget; la porta non lo impone, chi lo ignora assorbe fino a ~2,3 A |
| D31 | Limiti della protezione d'ingresso | protezione garantita in continuo fino a 20 V; sopra ~23 V la SMAJ20A si sacrifica e l'INA3221 (26 V) non è garantito; TVS invariata, codice fissato in `componenti.md` (revisione 02, R02-04) |
| D32 | Alimentazione della logica | RF-16 eliminato: hub e MCU solo dall'alimentatore, tolto D3 (ORing dalla VBUS upstream), D2 resta per la dissipazione dell'LDO; VBUS_DET e VSYS_ADC su pin FT (PA6, PA7; poi PB2, PB0 con D34) perché il partitore VBUS_DET ha tensione anche a MCU spento; con sovratensione o sottotensione d'ingresso si spegne tutto senza segnalazione al PC; hub funzionante dal solo PC rimandato come variante futura (revisione 02, R02-05) |
| D33 | Driver dei relè | **ULN2003V12** al posto del ULN2003A (stessa piedinatura, uscite MOSFET), tre canali in parallelo per relè: caduta ~0,2 V invece di 0,9–1,1 V, scatto garantito fino a 5V_SYS = 4,5 V e 40 °C; alternativa 2× AO3400A (revisione 02, R02-06) |
| D34 | Pin del MCU, UCPD e bootloader | HUB_RST_N da PA8 (pin CC dell'UCPD, pull-down Rd dopo ogni reset) a PF1; spostati VBUS_DET (PB2), VSYS_ADC (PB0), INA_ALERT (PC12), RLY1/RLY2 (PA4/PA5), EN_P4 (PA7) perché nel bootloader ST relè e porte restino a riposo e il hub funzioni; DBCC dell'UCPD1 a massa con 10 kΩ (R48, R49); il firmware stacca le Rd con UCPDx_STROBE; DFU da provare sul prototipo (revisione 02, R02-07) |
| D35 | Reset del solo hub | gestito dal firmware senza modifiche hardware: lettura di HUB_RST_N ogni ≤10 ms, presa del reset con PF1, rilascio con I2C fermo e configurazione SMBus dopo il rilascio effettivo di U12; brownout reset del MCU non usato perché lascerebbe una finestra 2,8-3,0 V (revisione 02, R02-08) |

## Aperte

Nessuna. Le decisioni di dettaglio si prendono durante lo schema e il PCB.
