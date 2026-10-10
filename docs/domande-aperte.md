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
| D8 | Alimentazione | USB-C a **5 V / 3 A senza PD**; ~2,5 A totali per le porte, 2 A su una singola porta; budget letto da CC e gestito dal firmware con le misure di corrente |
| D9 | Upstream verso il PC | connettore USB-C |
| D10 | Strumento host | libreria Python generica (multipiattaforma via `hidapi`) + CLI per macOS costruita sopra |
| D11 | Fornitore | si decide alla fine, a componenti finalizzati |
| D12 | Licenze | hardware **CERN-OHL-W 2.0**; firmware, libreria e CLI **MIT** |
| D13 | VID/PID USB | VID 0x1209 (pid.codes) con PID di test durante lo sviluppo; PID definitivo da richiedere a pid.codes alla pubblicazione |
| D14 | Porta hub di scorta (DS7) | su header interno a 4 pin (VBUS, D-, D+, GND), non sezionata |
| D15 | Scatola | stampata in 3D su misura; tutti i connettori saldati sul PCB, boccole a banana in versione da PCB; connettori sui due lati corti (banco: porte, pulsanti, boccole; retro: upstream e alimentazione) |

## Aperte

Nessuna. Le decisioni di dettaglio si prendono durante lo schema e il PCB.
