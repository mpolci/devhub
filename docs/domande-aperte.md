# Decisioni e domande aperte

## Decise

| # | Decisione | Scelta |
|---|---|---|
| D1 | Velocità | USB 2.0 Hi-Speed |
| D2 | Sezionamento | VBUS + D+/D- a stato solido; GND mai interrotta |
| D3 | Relè ausiliari | 2 canali, contatti max 16 V / 3 A, bobina 5 V; relè SPDT da 5–10 A |
| D4 | Misura corrente per porta | sì |
| D5 | Pulsanti fisici per porta | sì, uno per porta, più LED |
| D6 | MCU | STM32: **STM32G0B1RET6** (LQFP-64, USB FS + UCPD) |
| D7 | PCB e assemblaggio | JLCPCB 4 strati; **strategia C**: montaggio tutto a mano con piastra riscaldante e aria calda (stencil per i QFN), componenti da un unico fornitore; nessun tipo extended |
| D8 | Alimentazione | USB-C a **5 V / 3 A senza PD**; ~2,5 A totali per le porte, 2 A su una singola porta; budget letto da CC e gestito dal firmware con le misure di corrente |

## Aperte

Per ciascuna è indicata l'ipotesi adottata nel frattempo.

1. **Fornitore unico** per i componenti: Mouser, DigiKey o LCSC? Senza il TCPP01 tutti e tre
   hanno tutto; contano prezzo, spedizione e footprint dei connettori. *Ipotesi: Mouser.*
2. **Relè: SPDT con COM/NO/NC (6 boccole) o SPST (4 boccole)?** *Ipotesi: SPDT.*
3. **Connettore upstream**: USB-C o USB-B? *Ipotesi: USB-C.*
4. **VID/PID USB**: pid.codes (gratuito, richiede licenza open) oppure VID ST con PID di test?
5. **Licenza** del repository (hardware: CERN-OHL-P/S; firmware e host: MIT/Apache/GPL).
   *Nessuna licenza aggiunta finché non decidi.*
6. **Porta interna di scorta** (DS7 del hub) su header interno? *Ipotesi: sì.*
7. **Scatola e formato**: dimensioni, montaggio, stampa 3D? Nessuna ipotesi.
8. **Sistemi operativi host** per la CLI (macOS certo; Linux? Windows?). *Ipotesi: tutti e tre.*
