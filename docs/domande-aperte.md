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
| D16 | Reset del hub | supervisore **MCP120T-300** open-drain su RESET_N con pull-up, in wired-OR con il MCU (PA8 open-drain): il hub non dipende dal firmware e resta acceso durante reset e DFU del MCU |
| D17 | Regolatore 3,3 V | LDO **AP7361C-33E** (1 A, SOT-223) al posto dell'AP2112K in SOT-23-5 |
| D18 | Connettori USB | orizzontali (angolo retto, montati sopra il PCB): **GCT USB4105-GF-A** per tutti e sei gli USB-C (16 pin USB 2.0, 5 A, guscio con linguette passanti), **Kycon KUSBX-AS1N-B** passante per l'USB-A; scartati i modelli ruotati di 90° (rari e meno robusti) |
| D19 | Quarzo del hub | YXC **X322524MOB4SI** (24 MHz, 3225, C_L 12 pF, il più economico e con più disponibilità), C22/C23 = 15 pF C0G |
| D20 | Condensatori d'ingresso | invariati (2 × 220 µF + 2 × 10 µF): il picco all'inserzione lo elimina l'avvio dolce dell'eFuse |
| D21 | Protezione d'ingresso | eFuse **TPS259470A**: stacca sopra 5,84 V e sotto 4,0 V, limite 3,5 A, avvio dolce ~5 ms, FLT al MCU (PB1) |
| D22 | Switch dati delle porte | **PI3USB4002A** al posto del TS3USB30E: il lato connettore regge 24 V e si apre sopra 4,75 V (corti D+/D- su VBUS) |
| D23 | Fornitore | orientamento verso DigiKey (non ancora definitivo, vedi D11); le alternative trovate sono in `componenti.md` |

## Aperte

Nessuna. Le decisioni di dettaglio si prendono durante lo schema e il PCB.
