# Componenti candidati

**Stato della selezione:** le tabelle sotto documentano componenti e alternative dello schema
attuale, ancora da adeguare alle decisioni D60–D66. Non è approvato alcun nuovo controller.
**D68 conferma un hub con MCU separato; il modello del hub è da scegliere (D69)**. Restano cinque pulsanti e LED USB con lampeggio guasti.
La [variante USB4715 senza MCU](#usb4715-senza-mcu-verifica-di-fattibilità) è un’alternativa
non adottata; TCA9535 non è previsto nella soluzione corrente.

| Blocco | Indirizzo per la versione semplificata |
|---|---|
| Misure | Eliminare U7/U8 INA3221, R5/R65–R69 e supporti dedicati; nessun sensore o partitore sostitutivo per porte/5V_SYS |
| CC d'ingresso | Eliminare lettura ADC e R3/R4/C1/C2; mantenere Rd R1/R2 |
| Porte | Valutare integrazione di switch VBUS, dati, scarica e protezioni; sensore di corrente non richiesto; sequenza interna e scarica automatica ammesse, nessun GPIO DIS obbligatorio; priorità alla semplicità complessiva (D70) |
| LED USB | Quattro LED comandati dal MCU; eliminare la coppia della USB-A e uno dei due LED per ciascuna USB-C; riferimenti da definire |
| Uscite e pannello | Quattro USB-C, un relè, due boccole COM/NC, cinque pulsanti e sette LED complessivi; rimuovere la USB-A e il secondo canale relè (D66) |
| Logica e ingresso | Hub da scegliere (D69); USB2517 e STM32G0B1RET6 sono i riferimenti nello schema; possibile MCU più piccolo dopo il conteggio GPIO; conservare le protezioni richieste |

La selezione richiede verifica di corrente per porta, distacco dei dati con dispositivi
autoalimentati, blocco inverso, ESD, guasti disponibili, scarica e stato al reset.
Costo, disponibilità e risparmio netto vanno confrontati sul circuito completo; le alternative
di misura elencate sotto non sono candidate per la versione semplificata.

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

## Codici fornitore e magazzino

Ogni simbolo dello schema ha i campi `warehouse` (ID della riga nel foglio del magazzino),
`lcsc`, `mouser` e `digikey`. Il magazzino si usa solo per articoli comprati da LCSC, TME,
Mouser o DigiKey. Dove un fornitore non ha il pezzo scelto, il campo contiene un equivalente
(resistenze Yageo al posto di UNI-ROYAL/FOJAN, Panasonic EEU-FR1A221 per C3/C4, Würth per gli
header, ECS per il quarzo, G5Q-14 su LCSC, DMN3404L-7 su Mouser). Il campo `mouser` è vuoto
solo per le boccole Cliff, che Mouser non ha. Elenco completo, equivalenze e costi in
[confronto-costi.md](confronto-costi.md).

Package scelti per usare il magazzino (D45): C106-C506 1 µF 25 V in 0603 (era 0805), F1 PTC
MF-MSMF050-2 in 1812 (era 1206), C3/C4 radiali D6,3 passo 2,5 mm (era D8 passo 3,5),
C505 SMD 6,3 × 5,8 mm (era 6,3 × 7,7). I 100 nF di disaccoppiamento di U2 e U12 sono 0402 (ID 217,
Yageo CC0402KRX7R7BB104), per stare accanto ai pin fra le coppie USB (D58).

## Componenti nello schema esistente

Le quantità e i riferimenti nelle tabelle seguenti descrivono lo schema ancora da adeguare,
compresi USB-A, secondo relè e misure da eliminare. Per i nuovi candidati vedere la
[ricerca per la versione semplificata](#ricerca-per-la-versione-semplificata-d60d66).

## Circuiti integrati

| Funzione | Candidato | Package | A mano | Classe JLC | LCSC | Note |
|---|---|---|---|---|---|---|
| Hub USB 2.0, 7 porte | Microchip **USB2517I-JZX** | QFN-64 9×9, passo 0,5 mm, pad esposto | aria calda + stencil | extended (da confermare) | C478081, ~4,4 $ cad. | 3 000+ pezzi in stock a ottobre 2026 |
| Hub, alternativa saldabile a mano | Terminus **FE2.1** | LQFP-48 | sì | extended | sì | documentazione scarsa, usato in molti hub commerciali |
| MCU di controllo | ST **STM32G0B1RET6** | LQFP-64 | sì | extended | C2829307, ~2,7 $ cad. | USB FS device senza quarzo, 2× UCPD, 512 kB flash, ~60 GPIO |
| Load switch VBUS (×3, doppio canale) | TI **TPS2561A** | VSON-10 3×3 | aria calda / piastra | extended | sì | 2,5 A continui per canale, limite regolabile 0,25–2,8 A (accurato 2,1–2,5 A), FAULT per canale, 44 mΩ; nessuna scarica dell'uscita (verificato): MOSFET di scarica esterno; R_ILIM 24,3 kΩ, 33,2 kΩ su U6 per la USB-A (D54) |
| Load switch, alternativa a pin | TI **TPS25221DBV** | SOT-23-6 | sì | extended | sì | 2 A continui (al limite del requisito), limite 0,275–2,7 A, FAULT, nessuna scarica dell'uscita |
| Scarica VBUS porta (×5) | MOSFET N **AO3400** + R 100 Ω 1206 | SOT-23 | sì | basic | sì | presente nello schema; durata da rivalutare con il ciclo temporizzato D61, o sostituzione con scarica integrata |
| Switch dati D+/D- (×5) | Diodes **PI3USB4002AZUAEX** | UQFN-10 1,5×2 mm, passo 0,5 mm | aria calda | — | no (DigiKey) | interruttore USB 2.0, 1 GHz, 5 Ω, EN alto = scollegato; lato connettore tollerante 24 V con OVP a 4,75 V, ESD 10 kV IEC; REF con 1 µF 25 V (D22) |
| Misura corrente | TI **INA3221** ×2 | VQFN-16 | aria calda | extended | sì | 3 canali, I2C: 5 porte + corrente totale d'ingresso |
| Misura corrente, alternativa a mano | TI **INA219** ×5 | SOT-23-8 | sì | extended | sì | 1 canale, 16 indirizzi I2C |
| eFuse d'ingresso | TI **TPS259470ARPWR** | VQFN-HR-10 2×2 mm (RPW) | aria calda | extended | C3662799 | 2,7–23 V, 5,5 A, 28 mΩ; OVLO/UVLO regolabili, limite attivo, ripartenza automatica, FLT, blocco corrente inversa (D21) |
| Zener su EN dell'eFuse | **BZT52C5V1** | SOD-123 | sì | basic | sì | tiene EN sotto 6,5 V con ingresso fino a 28 V |
| LDO 3,3 V | Diodes **AP7361C-33E** | SOT-223 | sì | extended | sì | 1 A, dropout 90 mV tipici a 300 mA, θJA 110 °C/W con piazzola minima: regge 0,3–0,6 W. Sostituisce l'AP2112K (SOT-23-5), troppo caldo con il carico del hub |
| Supervisore reset hub | Microchip **MCP120T-300I/TT** | SOT-23 | sì | extended | sì | open-drain, soglia 2,85–3,0 V, ritardo 150–700 ms; in wired-OR con PF1 del MCU su RESET_N (errata USB2517: POR interno non funzionante) |
| Diodo davanti all'LDO | Schottky **SS34** (D2): Diodes **B340A-13-F** | SMA | sì | basic | sì | tra 5V_SYS e l'LDO: abbassa la tensione sull'LDO e la sua dissipazione (D32) |
| Driver relè | TI **ULN2003V12** (U9) | SOIC-16 | sì | da verificare | C94916 | uscite MOSFET, stessa piedinatura del ULN2003A, diodi di ricircolo verso COM e pull-down sugli ingressi integrati; 3 canali in parallelo per relè: ~0,2 V di caduta a 80 mA (D33) |
| ESD dati upstream (×1) | **USBLC6-2SC6** | SOT-23-6 | sì | basic | sì | solo sulla porta upstream; sulle porte downstream la protezione è nel PI3USB4002A (D27) |
| TVS VBUS d'ingresso (×1) | **SMAJ20A**: ST SMAJ20A-TR o Littelfuse SMAJ20A (famiglia JEDEC, valori equivalenti, da confermare col fornitore) | SMA | sì | basic | sì | non conduce fino a 20 V, rottura 22,2–24,5 V, clamp ~25 V a 1 A e 32,4 V a 12,3 A; lascia lavorare l'eFuse fino a 20 V; nessuna TVS sulle porte (D25, D31) |
| Quarzo hub | YXC **X322524MOB4SI**: 24 MHz, 3225 4 pad, C_L 12 pF, ±20 ppm | SMD | aria calda | basic | C70590, ~0,07 $ | C22/C23 = 15 pF C0G; il USB2517 tollera ±350 ppm, quindi va bene qualunque 24 MHz fondamentale (su DigiKey: ECS-240-12-33Q-JES-TR, 12 pF ±20 ppm, con gli stessi condensatori); il MCU non ha bisogno di quarzo (HSI48 + CRS) |

## Alternative sugli altri fornitori (Mouser / DigiKey)

Con il montaggio a mano e un fornitore unico il catalogo si allarga. Per blocco:

| Blocco | Scelta attuale | Alternative a catalogo Mouser/DigiKey | Giudizio |
|---|---|---|---|
| Hub 7 porte | USB2517I (QFN-64) | 2 × Microchip **USB2514B** (QFN-36, 4 porte, cascata 4+3); 2 × Infineon/Cypress **CY7C65642** (QFN-56) | Non esiste un hub Hi-Speed a 7 porte in package a pin da nessun fornitore. Gli hub a 4 porte in SSOP-28 (GL850G, FE1.1s) sono solo su LCSC. La cascata aggiunge un quarzo e un hop. USB2517 resta la scelta. |
| Load switch | TPS2561A | TPS25221 (SOT-23-6, 2 A); Diodes **AP22814** (SOT-25, limite fisso 3,45 A, troppo alto per il budget) | TPS2561A è il migliore sui 2 A: 2,5 A continui e limite accurato. Un canale resta libero. |
| Switch dati | PI3USB4002A (UQFN-10) | vedi "Alternative già valutate" | Unico trovato con protezione dai corti su VBUS, package saldabile e disponibilità |
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
| USB-A downstream (×1) | Kycon **KUSBX-AS1N-B**: orizzontale, passante | corrente nominale 1,5 A: la porta USB-A è limitata a 1,5 A (D54) |
| Pulsanti (×7: 5 porte, 2 relè) | C&K **PTS645SL70SMTR92 LFS**: tattile 6×6 SMD, terminali gull-wing, attuatore 7 mm (DigiKey) | sulla faccia superiore, premuti dal coperchio con estensore stampato; footprint KiCad `Button_Switch_SMD:SW_SPST_PTS645Sx43SMTR92`, comune a tutte le altezze della serie SMD (da verificare sul disegno C&K); ogni pressione alterna lo stato (RF-17) (D43) |
| LED (×14) | 0805 | 2 per porta (verde alimentata, rosso fault), 1 verde per relè (acceso = linea staccata), acceso (verde), host collegato (arancio) |

## Conteggio dei tipi "extended" per l'assemblaggio JLCPCB

Se JLCPCB montasse tutto: hub, MCU, load switch, switch dati, INA3221, più eventuali connettori e
il quarzo se non basic: **circa 8–11 tipi extended**, cioè ~25–35 € di sovrapprezzo per ordine. Vedi la strategia in
[architettura.md](architettura.md#produzione-e-assemblaggio).

## Alternative già valutate (per fornitore)

Ricerche fatte durante le scelte, da riusare se si cambia fornitore. Prezzi e scorte sono quelli
visti a ottobre 2026 e vanno ricontrollati.

| Funzione | Scelta | Alternative | Note |
|---|---|---|---|
| Quarzo 24 MHz 3225 | YXC **X322524MOB4SI** (LCSC C70590, C_L 12 pF, ~0,07 $, scorte ampie) | YXC X322524MRB4SI (LCSC C70571, C_L 18 pF, ~0,06 $, scorte scarse); Abracon ABM8-24.000MHZ-B2-T (C_L 18 pF, ~0,5–1 $, DigiKey esaurito, lead time 10 settimane); su DigiKey ECS **ECS-240-12-33Q-JES-TR** (C_L 12 pF, ±20 ppm, ESR 40 Ω, ~0,75 €) | Con C_L 12 pF: C22/C23 = 15 pF; con 18 pF: 33 pF. Il hub tollera ±350 ppm |
| Switch dati D+/D- protetto | Diodes **PI3USB4002AZUAEX** (DigiKey, ~1,2 $, >3 000 pezzi) | TI **TS5USBC410YFFR** (20 V OVP, LCSC C2150855 e DigiKey ~1,1 $; ma DSBGA-12 1,2×1,6 mm passo 0,4 mm: sconsigliato a mano); TI TS5USBC402 (DigiKey, DSBGA); Renesas ISL54226 / ISL54225 (obsoleti); ADI MAX4907 (sconsigliato per nuovi progetti); ADI MAX20046 (automotive, 18 V, più costoso); TI TPD3S714-Q1 (limite VBUS fisso 550 mA, troppo basso, un solo enable per dati e VBUS) | Il TS3USB30E di prima non ha protezione: sui pin dati regge solo VCC + 0,3 V |
| eFuse d'ingresso | TI **TPS259470ARPWR** (DigiKey ~1,3 $, Mouser, LCSC C3662799) | TPS259474ARPWR (LCSC C3662807: interviene a circuit breaker invece di limitare); TPS2595 con clamp 5,7 V e ST STEF05S (limitano la tensione invece di staccare: con 12 V in ingresso scaldano fino alla protezione termica) | — |
| Driver relè | TI **ULN2003V12DR** (DigiKey 296-30538: a ottobre 2026 risultava esaurito, 6 settimane; LCSC C94916 disponibile; TI diretto) | 2× **AO3400A** (MOSFET N SOT-23, 48 mΩ max a 2,5 V) con 2 diodi di ricircolo (1N4148W) e 2 pull-down da 100 kΩ sul gate: caduta trascurabile, 6 componenti invece di uno; scartati ULN2003A (Darlington, 0,9–1,1 V di caduta: la bobina non è garantita) e TPL7407L (vuole COM ≥ 8,5 V) | D33 |
| USB-C 16 pin | GCT **USB4105-GF-A** (DigiKey, 5 A) | Würth 632723300011, Amphenol 12401610E4#2A | Modelli ruotati di 90° scartati (D18) |

## Ricerca per la versione semplificata D60–D66

Valutazione al **14 ottobre 2026**. Configurazione: quattro USB-C, un relè con due boccole,
HID autonomo, alimentazione 5 V / 3 A, nessuna telemetria elettrica. **Nessun nuovo componente
è approvato né inserito nello schema.** I giudizi seguenti riguardano l'idoneità a devhub,
non una graduatoria generale dei prodotti. I valori tipici non sono garanzie di progetto.

### Esito e priorità

La prima soluzione da dimensionare è **AP22615 + PI3USB4002A per porta**: integra la scarica
nel ramo di alimentazione mantenendo la protezione dati già prevista. È una raccomandazione
condizionata alla verifica termica della scarica e del limite di corrente, non una scelta
conclusa. Non emerge dalla ricerca un singolo chip già verificato per tutti i requisiti.

Per la massima integrazione, **TPS254900A-Q1** è il candidato più interessante, ma resta
aperta la protezione inversa a porta accesa: non va sostituita con una semplice promessa
basata sulla protezione da sovratensione. **TPD3S716-Q1** è comodo da saldare, ma richiede
sia una scarica esterna sia una soluzione per il ritorno di corrente: il vantaggio si riduce.
**TPS22950C** è compatto e integra molte funzioni, ma non segnala direttamente la sola
limitazione per corto: non è la prima scelta per i guasti per porta richiesti da RF-5.

### Controller con VBUS e dati nello stesso chip

| Candidato / fonte primaria | Funzioni e package | Limite rispetto a devhub | Valutazione |
|---|---|---|---|
| TI [TPS254900A-Q1](https://www.ti.com/lit/ds/symlink/tps254900a-q1.pdf), SLUSCU5B, §§5, 6.5, 8.3.5, 8.4 | 3,2 A, VBUS + dati HS, scarica 400–630 Ω, FAULT, protezione dati/VBUS fino a 18 V; WQFN-20 3×4 mm | EN basso apre entrambi; scarica attiva con ingresso alimentato. Blocco inverso documentato a switch spento; nessuna specifica trovata che garantisca il blocco a porta accesa sotto OVP | Miglior candidato a chip unico, **condizionato** |
| TI [TPS254900-Q1](https://www.ti.com/product/TPS254900-Q1) | Variante da 3 A, switch VBUS/dati, protezioni da corto a batteria, WQFN-20 | Non trasferire automaticamente specifiche e comportamento dalla variante A; nessun vantaggio identificato che giustifichi preferirla | Secondaria rispetto alla A |
| TI [TPD3S716-Q1](https://www.ti.com/lit/ds/symlink/tpd3s716-q1.pdf), SLVSDH9D, §§8.3.9–8.4.5 | VBUS + dati, limite regolabile fino a 2,4 A, OVP 18 V ed ESD IEC; SSOP-16. VEN/DEN attivi bassi possono condividere il comando | Nessuna scarica VBUS integrata documentata. Tabella 8-2: con guasto inverso il percorso VBUS resta **ON**; FLT segnala soltanto. Servono blocco inverso e scarica aggiuntivi | Non soddisfa da solo RF-4/RNF-3 |
| TI [TPS2546](https://www.ti.com/lit/ds/symlink/tps2546.pdf), SLVSBJ2C | VBUS 2,7 A, dati HS, limitazione, FAULT e scarica; QFN-16 | La gestione BC1.2/CTL include stati di ricarica e wake: non equivalgono tutti a distacco dati. Non offre la protezione dati ad alta tensione del PI3USB4002A; aggiungerla erode il risparmio | Inferiore al TPS254900A per questo banco |
| Diodes [PI5USB2546A](https://www.diodes.com/datasheet/download/PI5USB2546A.pdf) | Controller di ricarica con switch VBUS e USB 2.0 e scarica per rinegoziazione | Non è il PI3USB4002A: la sigla simile non implica la stessa protezione da corto dati/VBUS. EN, CTL, limiti e protezioni da qualificare separatamente | Nessun vantaggio dimostrato sul candidato TI protetto |
| Microchip [UCS1001-1](https://www.microchip.com/en-us/product/ucs1001-1) / [UCS1002-1](https://www.microchip.com/en-us/product/UCS1002-1) | 2,5 A, dati HS, limitazione, gestione ricarica e ritorno di alimentazione; QFN-20 4×4. UCS1002 aggiunge SMBus, misura e scarica BC1.2 | Funzioni interessanti, ma non è dimostrata un'equivalenza alla protezione dati attuale. La misura integrata non è un requisito né una ragione per preferire UCS1002 | Alternative subordinate alla verifica di protezioni e stati |
| Microchip [UCS1003-1/2/3](https://ww1.microchip.com/downloads/en/DeviceDoc/20005346B.pdf), DS20005346B, §§1, 5.2.3, 7.3 | Switch dati/VBUS, scarica, back-voltage con apertura anche dei dati; QFN-20 4×4 | Massimo assoluto dati **VDD + 0,3 V anche da spento**: non sostituisce la protezione dati richiesta per schede autoalimentate o corti. La scarica ha una propria gestione degli errori da rispettare | Occorre protezione dati aggiuntiva; non prima scelta |
| ADI [MAX20046](https://www.analog.com/media/en/technical-documentation/data-sheets/MAX20046.pdf) | VBUS + dati HS, OVP ed ESD; TQFN-12 3×3 | Soglie di corrente nell'ordine di decine/centinaia di mA secondo variante, non 2 A | Escluso come controller completo della porta |

Per **TPS254900A-Q1**: configurazione proposta SDP, CTL1=0/CTL2=1, limite su ILIM_LO,
EN con pull-down, FAULT al MCU. La protezione IEC richiede anche la rete BIAS e le TVS
indicate nell'applicazione: non contare il solo chip. CS/IMON restano inutilizzati secondo
le prescrizioni dei pin; nessun ADC aggiunto. Il confronto economico è sospeso finché non
si risolve il blocco inverso acceso e si conteggiano tutti i supporti. L'eventuale protezione
esterna per porta può annullare il vantaggio rispetto alla coppia proposta.

### Alimentazione e scarica integrate, dati separati

| Candidato / fonte primaria | Funzioni verificate | Limiti e giudizio |
|---|---|---|
| Diodes [AP22615AWU-7](https://www.diodes.com/datasheet/download/AP22815_615.pdf), DS41022 Rev. 7-2 | TSOT26, 3 A, limite regolabile, FLG, soft-start, OVP uscita, blocco inverso e scarica circa 100 Ω a EN disabilitato | **Prima opzione da dimensionare** con PI3USB4002A. Soglia inversa tipica 65 mV; prima dell'intervento può esserci corrente inversa, indicata circa 350 mA. Non è un diodo ideale a corrente nulla. Verificare tolleranza ILIM, temperatura e scarica sostenuta da una sorgente esterna |
| Diodes [AP22815](https://www.diodes.com/part/AP22815/) | Variante a limite fisso e scarica della stessa famiglia | Il limite fisso è meno adatto alla singola uscita del sistema da 3 A; preferibile AP22615 regolabile |
| TI [TPS22950C](https://www.ti.com/tw/lit/gpn/tps22950), SLVSFJ2B, tab. 5, 9-1 e §9.3.4 | SOT-23-THIN-6, 3,2 A, limite regolabile, scarica e blocco inverso anche abilitato | La tabella 9-1 lascia FLT alto durante il corto limitato; segnala termica/inversione. RCB interviene dopo una corrente inversa circa 900 mA: non equivale a zero ritorno. VOUT massimo assoluto 6 V. **Non equivalente** a FLT del limitatore attuale |
| TI [TPS2001E](https://www.ti.com/lit/ds/symlink/tps2001e.pdf) | 2 A, scarica, guasto e blocco inverso da disabilitato; limite fisso | Semplice, ma esattamente al requisito di corrente e senza blocco inverso continuo dichiarato: alternativa meno adatta alle schede alimentate esternamente |
| Microchip [UCS2114](https://www.microchip.com/en-us/product/ucs2114) / [UCS2113](https://www.microchip.com/en-us/product/UCS2113) | Due canali di potenza, protezioni e misura integrata; UCS2113 dichiara 3 A continui per porta | Il doppio canale **non integra due switch dati USB**. Restano protezioni dati e verifica della scarica per il codice scelto; la telemetria non serve. Da rivalutare solo con un vantaggio di BOM dimostrato |

La variante **TPS22950L** non ha RCB; non va ordinata come sostituta della C. Per la C,
la documentazione conferma QOD a disabilitazione e RCB, ma il comportamento FLT impedisce
di trattarla come sostituzione diretta con pari diagnostica.

Con AP22615 rimangono condensatori, resistenza ILIM, pull-up FLG e lo switch dati con i suoi
supporti. Il suo EN attivo alto e l'EN di apertura del PI3USB4002A hanno polarità opposta:
si possono mantenere due GPIO comandati dalla stessa funzione firmware, oppure introdurre
un invertitore. RF-3 richiede un comando logico unico, non necessariamente un solo filo.
Questa scelta va inclusa nel conteggio, non nascosta nel presunto risparmio.

### Scarica, corrente e riduzione reale

Stima analitica per scarica resistiva ideale: `t = R × C × ln(5 / 0,8)`.
Per 1000 µF, 100 Ω danno circa 183 ms; 500 Ω circa 916 ms; 630 Ω circa 1155 ms.
Quindi il default indicativo di un secondo **non copre nemmeno 1000 µF nel caso da 630 Ω**,
senza includere ritardi e capacità locali. Questi sono calcoli di confronto, non specifiche
di reset né una reintroduzione della misura VBUS nel firmware.

Con una sorgente esterna costante da 5,25 V, una scarica resistiva dissipa `V²/R`:
276 mW su 100 Ω, 69 mW su 400 Ω. La resistenza integrata non rende automaticamente sicura
una porta lasciata spenta indefinitamente: servono limiti del produttore per quel modo,
bilancio termico e prova; se occorre un circuito di scarica esterno, il risparmio va ricalcolato.
La protezione termica dell'interruttore non dimostra da sola che protegga anche la scarica.

Conteggio **dei soli semiconduttori del blocco delle quattro porte**, esclusi passivi,
connettori, LED, hub, MCU e ingresso; non è una BOM completa:

| Soluzione | Integrati | MOSFET di scarica separati | Totale package considerati |
|---|---:|---:|---:|
| Circuito attuale ridotto a quattro porte: 2 TPS2561A + 4 PI3USB4002A | 6 | 4 | 10 |
| 4 AP22615 + 4 PI3USB4002A, se la scarica interna è qualificata | 8 | 0 | 8 |
| 4 TPS254900A, prima di eventuali protezioni inverse esterne | 4 | 0 | 4, **non ancora soluzione completa** |
| 4 TPD3S716 + scarica discreta, prima del blocco inverso aggiuntivo | 4 | 4 | 8, **non ancora soluzione completa** |

Per AP22615 il risparmio rispetto al circuito discreto a quattro porte è di due package
attivi, più le reti di scarica eliminate; il numero di IC aumenta perché si passa da
switch di potenza doppi a singoli. Togliere USB-A e secondo relè è un risparmio distinto,
come la rimozione INA3221/shunt già decisa: non attribuirlo al nuovo controller.
Il limite va scelto affinché il **minimo** garantito superi 2 A, includendo tolleranza della
resistenza e temperatura; impostare semplicemente “2 A tipici” non soddisfa RF-5.

### Hub, MCU, ingresso e relè

**Hub.** Quattro porte esterne più la porta HID interna richiedono almeno cinque downstream;
con l'header D14 ne servono sei. Un USB2514B o un
[Infineon HX2VL a quattro porte](https://www.infineon.com/products/universal-serial-bus/ez-usb-hub-controllers)
non basta; una cascata aggiunge circuito. Il
[Microchip USB4715](https://ww1.microchip.com/downloads/en/DeviceDoc/00002514B.pdf)
integra hub a quattro porte e bridge USB verso GPIO/I2C/SPI/UART, ma questo non dimostra
un firmware utente capace di HID, pulsanti, LED e cicli autonomi. Usarlo per eliminare
il MCU richiederebbe provare quei requisiti e l'interfaccia multipiattaforma; non è una
sostituzione adottata. **D68 conferma hub e MCU separato**, con modello del hub da scegliere (D69), controllo autonomo,
pulsanti e lampeggio guasti. La [verifica USB4715](#usb4715-senza-mcu-verifica-di-fattibilità)
resta disponibile come confronto con l’alternativa senza MCU.

**MCU.** Lo [STM32G0B1CB](https://www.st.com/en/microcontrollers-microprocessors/stm32g0b1cb.html)
è una variante con USB FS disponibile in LQFP-48: può ridurre area, non il numero di chip.
Non ridurre il package prima del conteggio GPIO, delle funzioni alternative e della verifica
DFU/bootloader. Non proporre espansori I/O per compensare un MCU scelto troppo piccolo.

**Alimentazione.** [MAX16984](https://www.analog.com/en/products/max16984.html) integra
buck, ricarica, switch dati e protezioni; [TPS25840-Q1](https://www.ti.com/product/TPS25840-Q1)
integra buck e funzioni USB. Sono interessanti partendo da tensioni maggiori, ma con l'ingresso
5 V già stabilizzato introducono induttore, switching e vincoli di dropout: non riducono la
complessità della v1. [TPS25810](https://www.ti.com/product/TPS25810) integra CC, VBUS e
scarica, ma non il sezionamento D+/D−; cambia inoltre la gestione dell'inserimento D29/D30.
Non è una sostituzione trasparente delle Rp attuali.

L'eFuse [TPS25947](https://www.ti.com/lit/ds/symlink/tps25947.pdf) già integra FET,
limitazione e blocco inverso: mantenerla è coerente con l'obiettivo. Un'eFuse sull'ingresso
non impedisce che una porta alimenti le altre attraverso 5V_SYS: il ritorno va gestito
anche per porta. Un PMIC non elimina automaticamente TVS, disaccoppiamenti e requisiti termici.

**Relè.** ULN2003V12 è già un unico chip con driver, pull-down e diodi; con un solo relè
molti canali restano inutilizzati, ma sostituirlo con MOSFET, diodo e pull-down aumenta i pezzi.
Mantenere provvisoriamente il driver attuale. Un interruttore elettronico non è equivalente
al contatto isolato COM/NC, normalmente chiuso senza alimentazione: questa funzione resta.

### Reperibilità per fornitore

Schede di catalogo individuate; **presenza a catalogo non significa stock confermato per
l'ordine**. Non usare prezzi/scorte indicizzati per aggiornare i totali della BOM. Classe JLC,
prezzo del taglio minimo, IVA/spedizione e disponibilità simultanea vanno verificati una volta
scelta la soluzione; resta il vincolo di un solo fornitore per il montaggio manuale.

| Codice esatto | DigiKey | Mouser | LCSC |
|---|---|---|---|
| TPS254900AIRVCRQ1 | [296-51885-1-ND](https://www.digikey.com/en/products/detail/texas-instruments/TPS254900AIRVCRQ1/8347159) | [595-TPS254900AIRVCRQ](https://www.mouser.com/ProductDetail/Texas-Instruments/TPS254900AIRVCRQ1?qs=W0yvOO0ixfHlSzPeGsl0DA%3D%3D) | Non verificato |
| AP22615AWU-7 | [AP22615AWU-7DICT-ND](https://www.digikey.com/en/products/detail/diodes-incorporated/AP22615AWU-7/9562755) | [Scheda AP22615AWU-7](https://eu.mouser.com/ProductDetail/Diodes-Incorporated/AP22615AWU-7?qs=sGAEpiMZZMvjbjwkTuU2aduMrJFCl4LIlrpr%252BPMkvo2DNG8A7KzOPQ%3D%3D) | [C2680357](https://www.lcsc.com/pl/product-detail/Power-Distribution-Switches_Diodes-Incorporated-AP22615AWU-7_C2680357.html) |
| TPS22950CDDCR | Non verificato | [Scheda TPS22950CDDCR](https://www.mouser.it/ProductDetail/Texas-Instruments/TPS22950CDDCR?qs=Jm2GQyTW%2Fbirn3QwntKsqg%3D%3D) | Non verificato |
| TPD3S716QDBQRQ1 | [296-44161-1-ND](https://www.digikey.be/en/products/detail/texas-instruments/TPD3S716QDBQRQ1/6023561) | Non verificato | Non verificato |

### Condizioni per la scelta definitiva

Prima dell'adeguamento KiCad occorre chiudere, per la soluzione scelta:

1. Corrente minima garantita, caduta a 2 A, temperatura e coordinamento col limite d'ingresso.
2. Blocco inverso con porta accesa, spenta e hub non alimentato: soglie, ritardo, corrente
   transitoria e tensione esterna ammessa. Il requisito non implica tolleranza illimitata.
3. Scarica con capacità definite e sorgente esterna, inclusa disabilitazione prolungata.
4. Apertura dati con dispositivo autoalimentato, corto dati/VBUS e comportamento al reset/DFU.
5. Segnali di guasto effettivi, livello logico/pull-up e collegamento agli OCS dell'hub:
   un FAULT multifunzione non identifica da solo la sovracorrente.
6. BOM completa dei quattro canali, inclusi ESD/TVS, BIAS, inversioni logiche e passivi,
   costo da un unico distributore e verifica USB Hi-Speed sul prototipo.

La ricerca restringe la scelta; non sostituisce la qualificazione elettrica né autorizza
l'eliminazione di protezioni non coperte dal candidato.

## USB4715 senza MCU: verifica di fattibilità

**Alternativa non adottata (D68).** La soluzione corrente mantiene MCU, cinque pulsanti
e LED porta con lampeggio guasti. I dettagli seguenti descrivono la fattibilità
dell’alternativa e non costituiscono attività richieste per la v1.

Valutazione al **14 ottobre 2026**, nello scenario D67: niente MCU esterno né pulsanti;
sequenze, impulsi e lampeggi gestiti dal PC. **Esito: soluzione plausibile, non ancora
collaudata sull'hardware.** La configurazione alternativa che mantiene tutti i segnali è **USB4715 + TCA9535**,
con comando tramite il dispositivo USB interno HFC. L'espansore non esegue firmware utente.
Il solo hub non offre un numero di GPIO documentati sufficiente per tutti i comandi e LED
indipendenti previsti. Schema, pinout e requisiti non devono essere convertiti a questa variante.

### Collegamento e accesso dal PC

USB4715 dispone di quattro porte esterne e di una **quinta porta interna dedicata all'HFC**:
eliminare lo STM32 non consuma una delle quattro USB-C. L'HFC deve rimanere abilitato e
raggiungibile anche quando tutte le uscite esterne sono spente. L'header USB di scorta D14
non trova invece una porta fisica libera: mantenerlo richiederebbe un altro hub/modello.
Fonte: [AN2437, tabella 1 e figura 2](https://www.microchip.com/en-us/application-notes/an2437).

Architettura proposta, ancora da dimensionare:

```
PC → USB4715 → quattro coppie dati → switch dati protetti → quattro USB-C
       └─ HFC interno → bridge I²C → TCA9535 → enable porte, LED, driver relè
                                      ↑
                             fault porte e ingresso
```

La libreria host deve aprire **l'HFC**, non acquisire il dispositivo USB di classe hub.
Il bridge non richiede di modificare il firmware interno: espone trasferimenti USB di
controllo. Gli switch fisici VBUS/D+/D−, la scarica e le protezioni restano esterni.
Disabilitare una porta con un comando USB standard non viene assunto equivalente a
sezionare elettricamente le linee di una scheda autoalimentata.

### macOS: cosa è verificato e cosa resta da provare

Microchip documenta un'implementazione diretta dei comandi EP0 per sistemi diversi da
Windows/Linux; non serve eseguire la DLL Windows su macOS. I suoi esempi Linux usano
`libusb_control_transfer`, e [libusb supporta macOS, anche Apple Silicon](https://github.com/libusb/libusb/wiki).
La conclusione di fattibilità è un'inferenza da protocollo e API, **non una certificazione
Microchip del controllo GPIO su ogni versione di macOS**.

Verifica locale: **macOS 15.7.7 arm64, libusb 1.0.29**, inizializzazione e enumerazione USB
riuscite; nessun USB4715/HFC con identificativi Microchip attesi presente. Non sono stati
inviati comandi a dispositivi, né verificati apertura dell'HFC, GPIO o trasferimenti I²C.
La presenza del backend sul Mac non prova il funzionamento del chip.

Per la CLI Python è proponibile PyUSB con backend libusb, oppure un piccolo binding diretto.
Il percorso da collaudare è: individuare l'HFC per descrittori e posizione USB, selezionare
l'interfaccia vendor, aprirla e inviare i comandi documentati. Non assumere che l'interfaccia
sia sempre la numero zero se sono attivi CDC/audio. Non cambiare configurazione USB né
staccare il driver del nodo hub per inviare i comandi all'HFC.

[AN2437, implementazione manuale](https://ww1.microchip.com/downloads/aemDocuments/documents/UNG/ApplicationNotes/ApplicationNotes/AN2437%2BUSB-to-GPIO%2BBridging%2Bwith%2BMicrochip%2BUSB47xx%2Band%2BUSB49xx%2BHubs.pdf)
definisce lettura registri con `bmRequestType=0xC0`, `bRequest=0x04` e scrittura con
`0x40`, `0x03`: metà bassa dell'indirizzo in `wValue`, metà alta in `wIndex`.
Usare i valori numerici: 0x40/0xC0 identificano recipient **device**, non interface,
nonostante la descrizione testuale nelle tabelle. Non convertire arbitrariamente in 0x41/0xC1.

Il [sorgente ufficiale Microchip](https://github.com/MicrochipTech/USB-Hub-Linux-Examples/blob/master/USB%20Hub%20Feature%20Examples/USB4715/Register_Access_via_USB/MPLABConnect/MchpUSBInterface.cpp)
è una conferma dell'uso di libusb, non un port macOS pronto: include esplorazione del nodo hub
e assunzioni su PID/interfaccia. Per il nostro software conviene un accesso mirato all'HFC.
La [FAQ libusb](https://github.com/libusb/libusb/wiki/FAQ) distingue inoltre l'accesso normale
dai casi in cui un driver di classe occupa l'interfaccia: evitare di confondere il controllo
vendor dell'HFC con HID/audio/CDC o con il driver del hub.

### Windows: installazione HFC/WinUSB

La [guida CLI Microchip DS50002629F, appendice F](https://ww1.microchip.com/downloads/aemDocuments/documents/UNG/ProductDocuments/UserGuides/MPLAB-Connect-Configurator-CLI-User-Guide-DS50002629.pdf)
include USB471x e documenta:

- Installazione automatica del pacchetto HFC tramite **Windows Update**, con Internet e
  aggiornamenti abilitati.
- In alternativa, installazione manuale una tantum da terminale amministratore:
  `MPLABConnect.exe /iw`; la guida non richiede riavvio per questa operazione.
- Il comando `/i` installa anche il filtro VSM ed è un percorso diverso, con riavvio:
  non è necessario nella proposta con HFC già abilitato.

L'installazione riguarda l'HFC; il nodo hub mantiene il driver USB di sistema. La DLL
Microchip non è obbligatoria per la nostra CLI: libusb può usare il backend WinUSB.
Non sostituire il driver del nodo hub con WinUSB per tentare di raggiungere i GPIO.

**Automazione documentata non equivale a installazione verificata oggi.** Non è stata
eseguita una prova su Windows 10/11 pulito, offline o Windows ARM64; non sono stati verificati
il pacchetto attualmente distribuito, la sua firma e il matching dei suoi ID sul campione.
La disponibilità attraverso Windows Update va provata con il dispositivo reale.

La [guida GUI Microchip, §3.1.1.2.4](https://ww1.microchip.com/downloads/en/DeviceDoc/MPLAB-Connect-Configurator-GUI-User-Guide-50002634D.pdf)
avverte che cambiare VID/PID dell'HFC richiede un nuovo pacchetto WinUSB. Per il prototipo
conviene mantenere gli ID di fabbrica. [AN2651, §8.1](https://www.microchip.com/en-us/application-notes/an2651)
distingue HFC WinUSB-only (`0424:4940`) e WinUSB+CDC (`0424:494C`): rilevare i descrittori,
non cercare soltanto 4940. L'HID della configurazione audio non è l'HID vendor di devhub.

L'installazione inbox completamente offline e senza INF è un'altra modalità, che richiede
[i descrittori Microsoft opportuni](https://learn.microsoft.com/en-us/windows-hardware/drivers/usbcon/automatic-installation-of-winusb).
Non è dimostrato che l'USB4715 nella configurazione scelta li fornisca: non promettere lo
stesso comportamento plug-and-play dell'HID. L'uso di ID personalizzati di D13 va riesaminato
quando si adotta la nuova architettura, senza cambiarli ora nella documentazione dello schema.

### GPIO realmente disponibili

Con **CONFIG2** sono documentati contemporaneamente quattro GPIO e SMBus1:

| Pin USB4715 | Funzione | Uso possibile |
|---:|---|---|
| 30, PF4 | GPIO6 | I/O libero |
| 29, PF5 | GPIO8 | I/O libero |
| 28, PF6 | GPIO10 | I/O libero |
| 11, PF7 | GPIO11 | I/O libero; pull-down al reset da considerare |
| 37, PF1 | SMB1_DAT | SDA dell'espansore |
| 38, PF8 | SMB1_CLK | SCL dell'espansore |

Fonti: [datasheet DS00002514B, §§3.1/3.3.4](https://ww1.microchip.com/downloads/en/DeviceDoc/00002514B.pdf),
[AN2651, tabella 3](https://ww1.microchip.com/downloads/aemDocuments/documents/UNG/ApplicationNotes/ApplicationNotes/AN2651-Configuration-of-Microchip-USB47xx-USB49xx-Application-Note-DS00002651.pdf),
[AN2438, tabella 2](https://www.microchip.com/en-us/application-notes/an2438).
AN2437 riporta fino a otto GPIO mediante configurazione personalizzata, ma non fornisce
una mappa sufficiente a usarli tutti nell'USB4715; la tabella specifica di AN2651 ne mappa
quattro. **Il dimensionamento usa quattro, senza contare pin ipotetici.**

I pin **PRT_CTL1–4/OCS1–4** sono ulteriori segnali dedicati, non quattro GPIO liberi:
combinano consenso di potenza e lettura di sovracorrente. Non pilotarli come normali
uscite push-pull contro un FAULT open-drain. I registri `PORT_CFG_SEL_n` sono marcati
**Configuration**, non comandi on/off da riscrivere arbitrariamente a runtime.
Usarli per il controllo standard dell'hub è un'opzione distinta, che richiede definire
consensi e interazione col sistema operativo; non è la base del conteggio seguente.

### Conteggio e assegnazione proposta con espansore

Con un comando elettrico comune per VBUS e dati, mantenendo tutte le indicazioni e i guasti:

| Funzione | Uscite | Ingressi |
|---|---:|---:|
| Quattro enable porte | 4 | 0 |
| Quattro LED porta indipendenti | 4 | 0 |
| Quattro FAULT porta | 0 | 4 |
| Driver relè e LED relè | 2 | 0 |
| LED host | 1 | 0 |
| FAULT eFuse ingresso | 0 | 1 |
| **Totale** | **11** | **5** |

Il LED alimentazione non richiede GPIO. Nessun pulsante è incluso. Un singolo
[**TCA9535PWR**, TSSOP-24](https://www.ti.com/product/TCA9535/part-details/TCA9535PWR)
fornisce 16 I/O e basta per tutto; i quattro GPIO nativi restano disponibili.
Proposta di assegnazione, non piedinatura dello schema:

| I/O espansore | Funzione |
|---|---|
| P00–P03 | EN_N porte 1–4 |
| P04 | Comando driver relè |
| P05 | LED relè |
| P06 | LED host |
| P07 | EFUSE_FLT_N |
| P10–P13 | FAULT porte 1–4 |
| P14–P17 | LED porte 1–4 |

Per la coppia di switch proposta, valutare **AP22615BWU-7**, variante a enable attivo basso,
con PI3USB4002A: il livello basso abilita entrambi, quello alto li disabilita. Questo evita
quattro invertitori o quattro GPIO aggiuntivi rispetto alla variante A. Fonte per la
polarità B: [AP22815/AP22615, Ordering Information](https://www.diodes.com/datasheet/download/AP22815_615.pdf).
La selezione di corrente/scarica rimane condizionata alle verifiche già elencate.

Il [TCA9535](https://www.ti.com/lit/ds/symlink/tca9535.pdf) parte con I/O configurati come ingressi:
prevedere pull-up per porte spente e pull-down sul comando relè. All'inizializzazione
scrivere prima i valori di uscita, poi le direzioni, evitando impulsi involontari.
Servono alimentazione 3,3 V, disaccoppiamento, indirizzo A0–A2 definito, pull-up I²C e FAULT,
e resistenze LED; il relè resta sul driver, non direttamente sull'espansore.

I comandi I²C HFC sono documentati dalla [AN2438](https://ww1.microchip.com/downloads/aemDocuments/documents/OTH/ApplicationNotes/ApplicationNotes/AN2438-USB-to-I2C-Bridging-with-USB47xx-USB49xx-Hubs-Application-Note-00002438C.pdf):
la libreria deve gestire inizializzazione del bridge, START/STOP, indirizzi e risultato
ACK/NACK. Non basta inviare un byte USB e assumere che il relè sia stato comandato.
Polling degli ingressi dal PC evita di aggiungere un collegamento INT dedicato.

La connessione degli OCS standard e il modello di alimentazione dichiarato al sistema
operativo restano da definire: leggere FAULT via espansore non produce automaticamente
un evento USB standard di sovracorrente. In ogni soluzione, limitazione, protezione inversa
e protezione termica devono funzionare in hardware senza intervento del PC.

### Avvio e comportamento quando il PC non esegue la CLI

Proposta da validare: al power-on porte spente, bobina relè diseccitata e COM/NC collegati;
la CLI apre HFC, configura espansore e applica i default salvati sul PC. CONFIG2 consente
la valutazione con strap e ROM; una memoria esterna o OTP non è assunta necessaria solo
per ottenere GPIO/I²C. Personalizzazione di descrittori, default permanenti e patch va
valutata separatamente. Non usare OTP come memoria riscrivibile delle preferenze utente.

A programma terminato, gli stati già scritti possono restare memorizzati nell'espansore
alimentato. Un impulso relè può quindi lasciare la linea staccata e un ciclo può lasciare
una porta spenta se manca il secondo comando. USB scollegata, reset del hub e power-cycle
non sono eventi equivalenti: un reset USB non garantisce il reset dell'espansore.
Occorrono lettura degli stati e una politica esplicita di riconciliazione alla riapertura.

La gestione PC deve serializzare i comandi fra processi, attendere l'esito di OFF prima
di contare `off_ms` e usare un orologio monotono; la durata fisica può essere maggiore per
latenza USB, carico o sospensione del sistema. Nessuna garanzia di completamento dopo crash.
I LED di guasto lampeggiano solo mentre il software effettua polling e aggiornamenti;
il LED host pilotato via software non certifica da solo una connessione ancora attiva.
Queste semantiche sostituiscono quelle autonome dello STM32 solo adottando la variante.

### Risparmio, vincoli e prova decisiva

USB4715 è VQFN-48 7×7 mm e richiede un riferimento a **25 MHz**: non riutilizzare il quarzo
24 MHz del USB2517. Restano alimentazioni, reset, disaccoppiamenti e protezioni da verificare.
USB4715 + espansore sono comunque **due IC**, come hub + MCU: il vantaggio è eliminare
firmware embedded, pulsanti e relativa componentistica e ridurre gli ingombri della logica;
non è una riduzione dimostrata a un solo chip o una garanzia di minor costo.

L'[errata DS80000813A](https://ww1.microchip.com/downloads/en/DeviceDoc/USB4715_Errata_DS80000813A.pdf)
segnala un problema di enumerazione per USB4715-VAA B1 e una patch OTP prevista in produzione.
Verificare variante e revisione del campione e l'applicabilità della patch; non estendere
automaticamente la tabella dei Device ID a tutti i codici ordinabili né riprogrammare
una patch già presente.

| Verifica | Esito attuale |
|---|---|
| Quattro porte esterne + HFC interno | Documentato |
| Protocollo GPIO e I²C pubblico | Documentato; nessun firmware utente necessario |
| 16 segnali con un espansore | Conteggio e mappa proposti; dimensionamento elettrico da completare |
| Backend libusb sul Mac disponibile | Verificato localmente, senza USB4715 collegato |
| GPIO/I²C comandati da macOS | Non provato sul chip |
| Installazione WinUSB/HFC | Procedura ufficiale individuata; non eseguita su Windows |
| Default, guasti, USB reset e sospensione | Da provare su circuito/EVB |

Solo in caso di riapertura della variante, prova su **EVB-USB4715 + espansore a 3,3 V**, carichi LED iniziali. Prima leggere
un registro HFC su macOS senza privilegi elevati, poi comandare e rileggere un I/O I²C.
Ripetere su Windows 11 x64 pulito, sia con Windows Update sia col pacchetto offline,
verificando che il driver sia associato soltanto all'HFC. Infine verificare HFC raggiungibile
con tutte le porte esterne disabilitate, fault simulato, reset, distacco USB, sospensione
e arresto del processo durante un ciclo. Windows ARM64 richiede una prova separata.

### Fornitori della variante

| Componente | Scheda verificata a catalogo | Stato della selezione |
|---|---|---|
| USB4715T-I/Y9X | [DigiKey, taglio nastro](https://www.digikey.com/en/products/detail/microchip-technology/USB4715T-I-Y9X/8275735) | Candidato; ordinabile e revisione da riconfermare |
| TCA9535PWR | [DigiKey 296-25129-1-ND](https://www.digikey.ca/en/products/detail/texas-instruments/TCA9535PWR/2139128) | Candidato per 16 I/O; stesso fornitore possibile |
| EVB-USB4715 | [DigiKey](https://www.digikey.com/en/products/detail/microchip-technology/EVB-USB4715/8037697), [Mouser](https://www.mouser.com/en/ProductDetail/Microchip-Technology/EVB-USB4715?qs=f9yNj16SXrKRG%252BmSPibMRQ%3D%3D) | Piattaforma per la prova; nessun acquisto eseguito |

Nessun prezzo o stock è assunto garantito; la BOM della variante non è ancora finalizzata.
