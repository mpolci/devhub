# Componenti candidati

Tabella di lavoro per la scelta dei componenti. "Classe JLC" è lo stato nella libreria di
assemblaggio JLCPCB al momento della verifica (ottobre 2026): va **riconfermato al momento
dell'ordine**, cambia spesso. "A mano" indica se il package si salda con il solo saldatore ("sì"), se richiede aria calda
o piastra riscaldante più stencil ("aria calda"), o se è sconsigliato anche così ("no").

Vincolo di approvvigionamento: i componenti saldati a mano vanno presi da un **unico fornitore**
(Mouser o DigiKey o LCSC). I componenti sono stati scelti per funzione e datasheet, non per
catalogo: LCSC è stato verificato solo quando era in gioco il montaggio JLCPCB. Tutti i
componenti principali qui sotto sono di produttori di prima fascia (Microchip, ST, TI) e sono gli
stessi pezzi su qualsiasi distributore; Mouser, DigiKey e LCSC li hanno tutti.
Le alternative reperibili solo su LCSC (FE1.1s, GL850G, FE2.1) sono indicate come tali.
I componenti della variante ad alta potenza (TCPP01-M12, STUSB4500, LM61495) sono
descritti in [architettura.md](architettura.md#variante-ad-alta-potenza-non-in-v1) e non fanno
parte della v1.

## Circuiti integrati

| Funzione | Candidato | Package | A mano | Classe JLC | LCSC | Note |
|---|---|---|---|---|---|---|
| Hub USB 2.0, 7 porte | Microchip **USB2517I-JZX** | QFN-64 9×9, passo 0,5 mm, pad esposto | aria calda + stencil | extended (da confermare) | C478081, ~4,4 $ cad. | 3 000+ pezzi in stock a ottobre 2026 |
| Hub, alternativa saldabile a mano | Terminus **FE2.1** | LQFP-48 | sì | extended | sì | documentazione scarsa, usato in molti hub commerciali |
| MCU di controllo | ST **STM32G0B1RET6** | LQFP-64 | sì | extended | C2829307, ~2,7 $ cad. | USB FS device senza quarzo, 2× UCPD, 512 kB flash, ~60 GPIO |
| Load switch VBUS (×3, doppio canale) | TI **TPS2561A** | VSON-10 3×3 | aria calda / piastra | extended | sì | 2,5 A continui per canale, limite regolabile 0,25–2,8 A (accurato 2,1–2,5 A), FAULT per canale, 44 mΩ; nessuna scarica dell'uscita (verificato): MOSFET di scarica esterno; R_ILIM 24,3 kΩ |
| Load switch, alternativa a pin | TI **TPS25221DBV** | SOT-23-6 | sì | extended | sì | 2 A continui (al limite del requisito), limite 0,275–2,7 A, FAULT, nessuna scarica dell'uscita |
| Scarica VBUS porta (×5) | MOSFET N **AO3400** + R 100 Ω 1206 | SOT-23 | sì | basic | sì | pilotato dal MCU per ~100 ms dopo lo spegnimento |
| Switch dati D+/D- (×5) | TI **TS3USB30E DGS** | VSSOP-10 | sì | extended | sì | banda 1 400 MHz, Ron 10 Ω, OE, ESD 15 kV su I/O; si usa solo un lato del mux |
| Misura corrente | TI **INA3221** ×2 | VQFN-16 | aria calda | extended | sì | 3 canali, I2C: 5 porte + corrente totale d'ingresso |
| Misura corrente, alternativa a mano | TI **INA219** ×5 | SOT-23-8 | sì | extended | sì | 1 canale, 16 indirizzi I2C |
| Protezione / ORing ingresso 5 V (opzionale) | TI **TPS2596** eFuse 3 A o diodo Schottky | SOT-23-8 / SMA | sì | extended / basic | sì | blocco corrente inversa, limite 3 A; in v1 basta la TVS più l'ORing |
| LDO 3,3 V | Diodes **AP7361C-33E** | SOT-223 | sì | extended | sì | 1 A, dropout 90 mV tipici a 300 mA (serve per funzionare dalla sola VBUS upstream), θJA 110 °C/W con piazzola minima: regge 0,3–0,6 W. Sostituisce l'AP2112K (SOT-23-5), troppo caldo con il carico del hub |
| Supervisore reset hub | Microchip **MCP120T-300I/TT** | SOT-23 | sì | extended | sì | open-drain, soglia 2,85–3,0 V, ritardo 150–700 ms; in wired-OR con PA8 del MCU su RESET_N (errata USB2517: POR interno non funzionante) |
| ORing 3V3 (RF-16) | 2× Schottky **SS34** | SMA | sì | basic | sì | tra 5V_SYS e VBUS upstream, prima dell'LDO |
| Driver LED | **74HC595** ×2 | SOIC-16 | sì | basic | sì | 14 LED (10 porte, 2 relè, alimentatore, host) con 4 GPIO; /OE dal MCU con pull-up (LED spenti all'accensione) |
| Driver relè | **ULN2003** o 2× 2N7002 + 1N4148 | SOIC-16 / SOT-23 | sì | basic | sì | |
| ESD dati (×6) | **USBLC6-2SC6** | SOT-23-6 | sì | basic | sì | upstream e 5 downstream |
| TVS VBUS d'ingresso (×1) | **SMAJ5.0A** o PESD5V0 | SMA | sì | basic | sì | sulle VBUS delle porte basta il pin VBUS dell'USBLC6-2 |
| Quarzo hub | YXC **X322524MOB4SI**: 24 MHz, 3225 4 pad, C_L 12 pF, ±20 ppm | SMD | aria calda | basic | C70590, ~0,07 $ | C22/C23 = 15 pF C0G; il USB2517 tollera ±350 ppm, quindi va bene qualunque 24 MHz fondamentale (su Mouser/DigiKey: un 3225 da 12 pF equivalente, con gli stessi condensatori); il MCU non ha bisogno di quarzo (HSI48 + CRS) |

## Alternative sugli altri fornitori (Mouser / DigiKey)

Con il montaggio a mano e un fornitore unico il catalogo si allarga. Per blocco:

| Blocco | Scelta attuale | Alternative a catalogo Mouser/DigiKey | Giudizio |
|---|---|---|---|
| Hub 7 porte | USB2517I (QFN-64) | 2 × Microchip **USB2514B** (QFN-36, 4 porte, cascata 4+3); 2 × Infineon/Cypress **CY7C65642** (QFN-56) | Non esiste un hub Hi-Speed a 7 porte in package a pin da nessun fornitore. Gli hub a 4 porte in SSOP-28 (GL850G, FE1.1s) sono solo su LCSC. La cascata aggiunge un quarzo e un hop. USB2517 resta la scelta. |
| Load switch | TPS2561A | TPS25221 (SOT-23-6, 2 A); Diodes **AP22814** (SOT-25, limite fisso 3,45 A, troppo alto per il budget) | TPS2561A è il migliore sui 2 A: 2,5 A continui e limite accurato. Un canale resta libero. |
| Switch dati | TS3USB30E (VSSOP-10) | TI TS3USB221 (QFN); Nexperia NX3DV221 (XSON, minuscolo); Diodes PI3USB102 (package da verificare); onsemi FSUSB42 (segnalato obsoleto) | TS3USB30E resta: a pin, banda 1,4 GHz, ESD integrata. |
| Misura corrente | INA3221 ×2 (VQFN-16) | TI **INA260** ×5 (TSSOP-16, shunt 2 mΩ integrato, ~4,7 € cad., lead time lungo segnalato); TI **INA226** ×5 (MSOP-10, shunt esterno); TI **INA219** ×5 (SOT-23-8) | INA3221 è il più compatto e il più economico (2 chip). INA260 elimina gli shunt ma costa 5 volte tanto e la disponibilità è incerta. |
| Relè | Omron G5Q-1 / Songle SRD | Omron **G5LE-1**, TE **OJE-SH-105LM** (5 A, slim), Panasonic **JS1-5V** | Tutti equivalenti per 16 V / 3 A; scelta per ingombro e prezzo. |
| Connettori USB-C 16 pin | — | GCT **USB4105-GF-A**, Würth **632723300011**, Amphenol **12401610E4#2A** | Tutti a catalogo Mouser/DigiKey, footprint KiCad disponibili. |

## Elettromeccanica (tutta a mano, through-hole o SMD grossi)

| Funzione | Candidato | Note |
|---|---|---|
| Relè ausiliari (×2) | SPDT 5–10 A, bobina 5 V: Omron **G5Q-1** DC5, Songle **SRD-05VDC-SL-C**, Hongfa **HF32F** | specifica contatti: 16 V / 3 A max; usati su COM/NC; il contatto NC del G5Q-1 è tarato 3 A a 30 V DC, accettato senza margine; snubber RC opzionale sui contatti |
| Boccole a banana 4 mm (×4) | Cliff **FCR7350** (S16N-PC, da pannello con montaggio su PCB): nera FCR7350B su COM, rossa FCR7350R su NC | COM e NC per canale; NO su ponticello interno; footprint KiCad `Connector:Banana_Cliff_FCR7350*` |
| USB-C downstream (×4) | GCT **USB4105-GF-A**: 16 pin (solo USB 2.0), orizzontale, montato sopra il PCB, 5 A, guscio con linguette passanti | stesso modello per tutti gli USB-C (D18); sui 16 pin bastano D+/D-, CC1, CC2, VBUS, GND |
| USB-C upstream (×1) | GCT **USB4105-GF-A** | Rd 5,1 kΩ su CC1 e CC2 |
| USB-C alimentazione (×1) | GCT **USB4105-GF-A** | solo CC1, CC2 (Rd 5,1 kΩ, verso ADC), VBUS, GND; 5 V / 3 A (il connettore regge 5 A) |
| USB-A downstream (×1) | Kycon **KUSBX-AS1N-B**: orizzontale, passante | |
| Pulsanti porta (×5) | tattili 6×6 ad asta lunga, o con estensore stampato in 3D | |
| LED (×14) | 0805 o 3 mm | 2 per porta (alimentata / fault), 1 per relè (acceso = linea staccata), alimentatore presente, host collegato |

## Conteggio dei tipi "extended" per l'assemblaggio JLCPCB

Se JLCPCB montasse tutto: hub, MCU, load switch, switch dati, INA3221, più eventuali connettori e
il quarzo se non basic: **circa 8–11 tipi extended**, cioè ~25–35 € di sovrapprezzo per ordine. Vedi la strategia in
[architettura.md](architettura.md#produzione-e-assemblaggio).
