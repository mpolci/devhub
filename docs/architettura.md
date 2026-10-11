# Architettura

Documento di lavoro: raccoglie le scelte fatte, le motivazioni e i componenti scelti o candidati.
Le voci marcate **[da verificare]** vanno confermate sui datasheet prima di disegnare lo schema.
La tabella dei componenti con package, fornitore e classe JLCPCB è in [componenti.md](componenti.md).

## Schema a blocchi

```
USB-C PWR IN 5 V / 3 A ──► TVS + shunt + eFuse ───► 5V_SYS ────────────────────────────────┐
(qualunque alimentatore      │                       │                                     │
 USB-C, anche PD)            │ CC1/CC2 (Rd 5,1 kΩ)    ▼ ORing Schottky + LDO 3,3 V          │
                             └──► ADC del MCU      3V3 (hub, MCU, switch)                  │
                                  (0,5 / 1,5 / 3 A)    ▲                                   │
USB-C UPSTREAM ─── VBUS ───────────────────────────────┘ (RF-16)                           │
(verso il PC)   │                                                                          │
                └──► HUB USB 2.0, 7 porte (USB2517)                                        │
                         ├─ DS1..DS4 ─► PI3USB4002A ► USB-C 1..4 ◄─ TPS2561A + scarica ◄───┤
                         ├─ DS5 ─────► PI3USB4002A ► USB-A      ◄─ TPS2561A + scarica ◄───┤
                         ├─ DS6 ─────► MCU STM32G0B1 (USB FS) — porta interna, mai sezionata
                         └─ DS7 ─────  header interno (scorta)
                                             │
                                             ├─ per porta: EN, FAULT, OE dati, DIS (scarica), pulsante
                                             ├─ monitor corrente: 5 porte + ingresso (INA3221 ×2, I2C)
                                             ├─ 2 relè (ULN2003) ──► boccole a banana (COM/NO/NC)
                                             └─ LED via 74HC595, SWD, UART di debug
```

## Scelte progettuali

### USB 2.0, non USB 3

Scelta: **USB 2.0 Hi-Speed** per la v1. Motivi:

- Lo scopo è il test di dispositivi a microcontrollore, che sono Full-Speed o Hi-Speed: il
  SuperSpeed non porterebbe nulla.
- Un hub USB 3 a 7 porte (es. Microchip USB5807) richiede per ogni porta due coppie differenziali
  aggiuntive a 5 Gbit/s con impedenza controllata, condensatori di accoppiamento e, sui connettori
  USB-C, un **controller CC + mux SuperSpeed per porta** (es. TI HD3SS3220) per gestire il
  ribaltamento del cavo, più un secondo switch SuperSpeed per porta per il sezionamento.
  Si passa da ~1 chip a ~3-4 chip per porta e il costo della sola parte hub sale di 4-5 volte.
- Su USB 2.0 i connettori USB-C non hanno il problema dell'orientamento: le coppie D+/D- dei
  due lati sono in parallelo nel connettore e basta una resistenza di pull-up Rp su CC1 e CC2.

Il PCB sarà comunque a **4 strati** (costa poco, semplifica il routing delle coppie differenziali
Hi-Speed e dà piani solidi per i 3 A a 5 V). Un'eventuale variante USB 3 resta un progetto
separato: non si cerca di renderla "predisposta".

### Cosa si seziona: VBUS e D+/D-, non GND

L'idea iniziale era un relè su VBUS e GND. Si seziona invece **VBUS e le linee dati D+/D-** e si
lascia GND sempre collegata:

- Togliere GND lasciando D+/D- collegate fa sì che il dispositivo cerchi il ritorno di corrente
  attraverso le linee dati e i diodi di protezione dell'hub o del dispositivo: è il classico modo
  per danneggiare le porte.
- Togliere solo VBUS non basta per i dispositivi **auto-alimentati** (es. una scheda con la propria
  alimentazione): continuano a tenere il pull-up su D+ e l'host non li vede mai scollegati.
  Sezionando anche D+/D- lo scollegamento è garantito in ogni caso (RF-2).
- Dati e alimentazione hanno due interruttori distinti, comandati dal MCU, ma si staccano e si
  riattaccano sempre insieme e in sequenza (RF-3, D28).

### Porte: stato solido, non relè

- **VBUS**: load switch **TI TPS2561A** (doppio canale, VSON-10 3×3): 2,5 A continui per canale,
  FAULT per canale (open-drain, deglitch 9 ms), soft-start, 44 mΩ, blocco della corrente inversa.
  Una **sola resistenza ILIM per chip** imposta il limite di entrambi i canali, indipendenti tra
  loro: I_OS(nom) = 56000 / R_ILIM(kΩ) mA. Con **R_ILIM = 24,3 kΩ 1 %** il limite è 2,1–2,5 A
  (2,3 A tipico) per canale, verificato sul datasheet (SLVSB28, tabella EC). Tre chip coprono le
  5 porte con un canale di scorta. Il TPS2561A **non ha la scarica dell'uscita**: per porta un
  MOSFET N (AO3400) con 100 Ω verso GND, pilotato dal MCU per ~100 ms dopo lo spegnimento
  (pin DIS); senza scarica un dispositivo con grossi condensatori resta "acceso" per secondi e
  non si resetta.
  Alternativa a pin: **TPS25221** (SOT-23-6), ma 2 A continui, al limite del requisito RF-5.
- **D+/D-**: **Diodes PI3USB4002A** (UQFN-10 1,5×2 mm): interruttore USB 2.0 (banda 1 GHz, 5 Ω)
  con EN alto = linee in alta impedenza, così l'hub vede il distacco. Il lato connettore regge
  24 V e si apre da solo sopra 4,75 V: un corto D+/D- su VBUS in una scheda in prova o in un cavo
  non arriva alla 3,3 V (D22). Sostituisce il TS3USB30E, che sui pin dati reggeva solo VCC + 0,3 V.

Sequenza di stacco (default nel firmware): OE dati off → EN VBUS off → DIS on per 100 ms.
Riattacco: EN VBUS on → dopo `on_delay_ms` (default 50 ms) OE dati on, così il dispositivo si
enumera già alimentato stabilmente.

I relè restano per le due linee ausiliarie su banana.

### Alimentazione: 5 V diretti, senza USB-PD

Decisione: l'ingresso è un USB-C a **5 V senza negoziazione PD**. Su USB-C un alimentatore eroga
al massimo 5 V / 3 A (15 W) senza PD; per avere di più bisogna negoziare una tensione più alta e
abbassarla con un buck. Si è scelto di **non** farlo, accettando un totale di ~2,3 A per le porte,
perché:

- le schede a microcontrollore assorbono in genere 50–500 mA; una porta da sola arriva comunque a
  2 A grazie al limite per porta (RF-5);
- spariscono il TCPP01, il buck LM61495 con induttore e condensatori da 20 V, il rail 20 V e,
  soprattutto, lo **stack USB-PD nel firmware**, che era il rischio maggiore del progetto;
- qualunque alimentatore USB-C funziona, anche quelli PD: tutti danno 5 V / 3 A di default;
- un dispositivo affamato (es. un single-board computer) si alimenta dal proprio alimentatore e lo
  si seziona con uno dei relè ausiliari, che è il loro caso d'uso.

Bilancio (alimentatore 5 V / 3 A):

| Voce | Valore |
|---|---|
| Hub USB2517 (3,3 V tramite LDO) | 0,23–0,46 A (datasheet: da 1 a 7 porte Hi-Speed attive) |
| MCU, switch dati, INA3221, LED | ~0,05 A |
| 2 relè eccitati (G5Q-1 SPDT, 80 mA per bobina) | ~0,16 A |
| Disponibile per le porte | ~2,3 A nel caso peggiore |

La logica è alimentata dall'ORing tra 5V_SYS e VBUS upstream: quando la VBUS del PC è più alta
della 5V_SYS la logica assorbe in parte dal PC e quella corrente non passa dallo shunt d'ingresso.

**Lettura del budget.** Sulle CC dell'ingresso il MCU ha Rd da 5,1 kΩ e legge con l'ADC la
tensione imposta dall'Rp dell'alimentatore: ~0,4 V → 0,5 A, ~0,9 V → 1,5 A, ~1,7 V → 3 A
(soglie USB Type-C: 0,66 V e 1,23 V). Il budget è la corrente pubblicizzata dall'alimentatore,
oppure quella impostata a mano.
La periferica UCPD dello STM32G0B1 può fare la stessa lettura senza stack PD, ma l'ADC basta.

**Gestione del budget (firmware).** Il sesto canale dell'INA3221 misura la corrente totale
d'ingresso (`I_totale`), che comprende già logica e relè quando sono alimentati dalla 5V_SYS: per
questo si confronta direttamente con il budget, senza sottrarre una quota fissa per la logica.
Regole:
1. una porta si accende solo se `I_totale + 200 mA < budget`;
2. le porte si accendono sfalsate di ~30 ms (inrush);
3. se `I_totale > budget` per più di 50 ms, oppure 5V_SYS scende sotto 4,5 V (ADC), si spegne
   la porta che assorbe di più e si manda l'evento `PORT_FAULT` con causa "budget". I 50 ms vanno
   verificati al primo montaggio con gli alimentatori reali: se un alimentatore va in protezione
   prima, il tempo va accorciato (revisione 02, R02-03);
4. il budget è anche impostabile a mano via HID (`SET_DEFAULTS`), per alimentatori che non
   pubblicizzano correttamente.
Il limite di corrente dell'alimentatore resta la protezione di ultima istanza.

**Protezione d'ingresso.** TVS **SMAJ20A**, shunt R5 e poi l'eFuse **TPS259470A** (U13, D21):
- stacca l'uscita sopra 5,84 V (riaccende sotto 5,3 V) e sotto 4,0 V, con un solo partitore
  (R30/R31/R32); lo zener D4 tiene il pin EN sotto il suo massimo (6,5 V) fino a 28 V d'ingresso;
- limite di corrente 3,5 A (R33), transitori ammessi fino a ~8 ms (C12), ripartenza automatica;
- avvio dolce: la 5V_SYS sale in ~5 ms e i 460 µF di bulk si caricano con ~0,4 A invece di un picco
  di decine di ampere; i condensatori d'ingresso restano quelli attuali (D20);
- blocca la corrente inversa; FLT (open-drain) va al MCU su PB1;
- lo shunt sta prima dell'eFuse, così l'INA3221 misura la tensione d'ingresso anche quando l'eFuse
  ha staccato e il firmware può segnalare "ingresso fuori tensione".
Caduta aggiuntiva: ~28 mΩ, cioè ~0,1 V a 3 A.

**TVS (D25).** La TVS d'ingresso è una **SMAJ20A**: non conduce fino a 20 V (il massimo di un
alimentatore USB-C) e limita i picchi oltre ~22 V. Una TVS da 5 V (la SMAJ5.0A di prima) entrerebbe
in conduzione già a ~6,4 V e con un alimentatore da 12–20 V collegato per errore brucerebbe prima
che l'eFuse possa staccare; con la SMAJ20A è l'eFuse a staccare la 5V_SYS e, tolto l'alimentatore
sbagliato, tutto riparte. Sulle VBUS delle porte non c'è TVS: una scarica da 8 kV (~1,2 µC) sui
22 µF della porta alza la tensione di ~0,05 V, lo stacco di un cavo sotto carico di pochi decimi
di volt, i picchi negativi li ferma il diodo interno del MOSFET di scarica; contro una
tensione continua rimandata da una scheda una TVS non serve (brucerebbe) e una da 5 V limiterebbe
comunque sopra i 7 V massimi del TPS2561A.

**Logica (RF-16).** 3V3 da LDO a basso dropout (**AP7361C-33E**, 1 A, SOT-223) alimentato da un ORing a diodi
Schottky tra 5V_SYS e la VBUS upstream: con alimentatore assente hub e MCU enumerano e riportano "alimentatore assente"; le
porte downstream si attivano solo se 5V_SYS è presente. Il diodo impedisce di iniettare 5V_SYS
verso l'host. Con 4,6 V in ingresso e 0,25–0,48 A di carico l'LDO dissipa 0,3–0,6 W: il SOT-223
(θJA 110 °C/W con la piazzola minima, meno con un'area di rame sul tab) resta sotto i 70 °C di
sovratemperatura nel caso peggiore. Dropout 90 mV tipici a 300 mA: funziona anche dalla sola VBUS
upstream. Scartato l'AP2112K in SOT-23-5, che con lo stesso carico rischiava la protezione termica (D17).

#### Variante ad alta potenza (non in v1)

Se servissero 6–10 A totali: PD sink sul MCU (UCPD + middleware ST, AN5418) o **STUSB4500**
standalone, protezione **TCPP01-M12**, buck **LM62460 / LM61480 / LM61495** (6 / 8 / 10 A,
pin-compatibili nello stesso VQFN-HR 16, 3–36 V in, ~1,6 $), alimentatore PD da 45–65 W. Lo stesso meccanismo di budget resta valido con il budget
ricavato dal PDO negoziato.

### Hub

**Microchip USB2517I** (7 porte, QFN-64 passo 0,5 mm con pad esposto, quarzo 24 MHz,
configurazione via resistenze o SMBus, regolatore 1,8 V interno). Il package si salda a mano con
aria calda o piastra riscaldante e stencil: i pad perimetrali sono sotto il bordo e il pad
centrale è la massa e va saldato; con il solo saldatore è un ripiego (pad centrale scaldato dai
via termici sul retro). 7 porte coprono 4 USB-C + 1 USB-A + 1 porta interna per il MCU
e ne lasciano 1 di scorta su un header interno a 4 pin (VBUS, D-, D+, GND), non sezionato e
alimentato direttamente dalla 5V_SYS con un fusibile ripristinabile.

Dettagli verificati sul datasheet (DS00001598C) e sull'errata (DS80000628A):

- **PRTPWR** del hub non usati: l'abilitazione delle porte è del MCU. I **FAULT** dei load switch
  (open-drain) vanno sia al MCU sia agli ingressi **OCS_N** del hub, che hanno pull-up interno:
  l'host riceve così la notifica standard di sovracorrente senza componenti in più.
- **Configurazione via SMBus** (CFG_SEL = 001, indirizzo 0x2C): il MCU scrive VID/PID, stringhe,
  porta 6 "non rimovibile" (il MCU stesso), corrente massima, e poi comanda l'attach. CFG_SEL0 è
  il pin SCL (alto per il pull-up I2C), CFG_SEL1 è a massa, **CFG_SEL2 lo pilota il MCU** prima di
  rilasciare il reset: basso = SMBus (001), alto = default interni con dynamic power (101), che è
  anche lo stato del pull-up per il bring-up senza firmware.
- **Reset** (D16): l'errata dice che il POR interno non funziona e suggerisce un supervisore
  esterno. RESET_N ha un pull-up da 10 kΩ e due sorgenti in wired-OR: il supervisore **MCP120T-300**
  (open-drain, soglia 2,85–3,0 V, ritardo 150–700 ms), che tiene il hub in reset finché la 3,3 V non
  è stabile, e PA8 del MCU in open-drain, che può forzare il reset (comando `RESET_HUB`). Un reset del
  MCU (watchdog, `REBOOT`, debugger, bootloader) lascia PA8 in alta impedenza e **non resetta il
  hub**. All'accensione il MCU parte prima che il supervisore rilasci, tiene basso il reset, sceglie
  CFG_SEL2 e lo rilascia; senza firmware il hub parte comunque con i default interni (101).
- **VBUS_DET** dalla VBUS upstream con partitore 100 k / 100 k; **LOCAL_PWR** da un partitore su
  5V_SYS (alto = alimentatore presente).
- RBIAS 12,0 kΩ 1 %; quarzo 24 MHz YXC X322524MOB4SI (C_L 12 pF) con 15 pF C0G (D19; il hub tollera ±350 ppm); 1,8 V dal
  regolatore interno (VDD18, VDD18PLL con 1 µF ciascuno); TEST a massa; slug a massa.
- Mappa porte: 1–4 → USB-C, 5 → USB-A, 6 → MCU, 7 → header interno. Le porte 1–5 sono sul lato
  inferiore del QFN, da orientare verso il pannello frontale.

Alternativa saldabile a mano: **FE2.1** (LQFP-48), documentazione scarsa ma diffusissimo.

### MCU di controllo: STM32G0B1RET6

Scelto tra gli STM32 per questi motivi:

- **USB FS device senza quarzo** (HSI48 + CRS). Ha anche la periferica **UCPD**: non serve in v1
  (nessuna negoziazione PD) ma lascia aperta la variante ad alta potenza senza cambiare MCU.
- **LQFP-64**, saldabile a mano, ~60 GPIO: bastano per pilotare tutto senza expander. Conteggio:
  5 porte × (EN, FAULT, OE, DIS, pulsante) = 25, 74HC595 per i LED = 4 (SER, SCK, latch, /OE), 2 relè, I2C condiviso = 2,
  USB = 2, ADC (CC1, CC2, 5V_SYS) = 3, reset hub = 1, guasto eFuse = 1, pulsanti dei relè = 2, SWD = 2,
  UART debug = 2: totale ~46 sui 60 disponibili.
- 512 kB flash / 144 kB RAM: largo per stack USB + logica, e per uno stack PD futuro.
- Disponibile su LCSC (C2829307, ~2,7 $) e su Mouser/DigiKey.

Il modello **STM32G0B1RCT6** (256 kB) è equivalente se più disponibile.

Interfacce USB esposte:
- **HID** con usage page vendor: il canale di controllo, senza driver.
- **CDC** (opzionale): console di debug e log.
- Bootloader di sistema ST via DFU (USB) per l'aggiornamento del firmware: `ENTER_BOOTLOADER`, poi
  `dfu-util` attraverso il hub, che resta acceso. Con la flash vuota il G0 avvia da solo il
  bootloader (empty check), quindi anche la prima programmazione può passare dall'USB. SWD (J4) per
  debug e recupero.

Stack software: STM32CubeG0 HAL; per l'USB device TinyUSB o la libreria USB device di ST. Build con CMake e arm-none-eabi-gcc (già presenti sulla macchina di sviluppo).

VID/PID: VID 0x1209 (pid.codes) con PID di test (0x0001–0x0010) durante lo sviluppo; PID definitivo
da richiedere a pid.codes alla pubblicazione del repository.

### Porte downstream

Per ogni porta:

| Funzione | Componente | Note |
|---|---|---|
| Load switch VBUS | TPS2561A (½ chip) | R_ILIM 24,3 kΩ 1 % → 2,1–2,5 A per canale |
| Scarica VBUS | AO3400 + 100 Ω 1206 | pilotato dal MCU |
| Switch D+/D- | PI3USB4002A | EN (OE_N) dal MCU, alto = scollegato; 1 µF 25 V sul pin REF |
| Misura corrente | INA3221 (3 canali) × 2 | shunt 20 mΩ tra l'uscita del load switch e il connettore |
| ESD dati | integrata nel PI3USB4002A (10 kV IEC 61000-4-2 sul lato connettore) | nessuna protezione esterna sulle porte (D27): una USBLC6 con il pin VBUS sulla VBUS commutata accoppiava dati e alimentazione e aggirava la tolleranza a 24 V dello switch. Layout: Diodes consiglia di non accorciare troppo la pista tra connettore e switch |
| Protezione VBUS | i 22 µF + 100 nF della porta | TVS dedicata (SMAJ20A) solo sull'ingresso (D25) |
| USB-C: CC1, CC2 | Rp **56 kΩ** a VBUS commutata (annuncio standard USB, 500 mA) | la VBUS commutata fa cadere anche CC quando la porta è spenta |
| LED | 2 (alimentata / fault) via 74HC595 | |
| Pulsante | tattile: ogni pressione alterna la porta (alimentazione e dati insieme) | debounce in firmware; rispetta il budget (RF-17) |
| Condensatori VBUS | 22 µF + 100 nF; sulla USB-A anche 100 µF | USB 2.0 chiede ≥ 120 µF sulle porte di tipo A |

Le porte USB-C **non rilevano l'inserimento** sulle CC (D29): appena il MCU accende una porta la
VBUS è presente anche senza dispositivo, come su una presa USB-A. La specifica Type-C vorrebbe VBUS
accesa solo dopo aver visto la Rd del dispositivo; si è scelto di non farlo per semplicità e per
continuare ad alimentare schede e cavi privi delle resistenze CC. I rischi (un'altra sorgente
collegata alla porta) sono coperti dai controlli del firmware descritti in "Protezioni".

Sul valore di Rp (D30): si annuncia la corrente standard USB (56 kΩ, 500 mA), così le cinque porte
promettono in tutto ~2,5 A, in linea con il budget di ~2,3 A. Annunciare 1,5 A (22 kΩ) avrebbe
promesso 6 A sulle sole USB-C: due dispositivi conformi scarichi possono salire insieme a 1,5 A e
mandare in protezione l'alimentatore prima che il firmware intervenga. L'annuncio non è un limite:
la porta eroga fino a ~2,3 A (TPS2561A) a chi lo ignora, come la maggior parte delle schede di
sviluppo; telefoni e computer conformi si limitano a 500 mA.

### Relè ausiliari

Specifica contatti: **max 16 V, max 3 A**. 2 relè **SPDT** con bobina 5 V (es. Omron G5Q-1,
Songle SRD-05VDC-SL-C: 10 A nominali, ampio margine) pilotati da ULN2003 (diodi di ricircolo
integrati). Su pannello, per ogni canale, **due boccole a banana da 4 mm su COM e NC**: la linea
è **normalmente collegata** e la bobina si eccita solo per staccarla. Così lo stacco dura il tempo
di un impulso, e se l'hub perde alimentazione o si blocca la linea torna collegata (comportamento
sicuro per una scheda in prova). Il contatto NO è portato su un **ponticello a saldare** interno:
spostandolo la seconda boccola diventa NO per un eventuale uso "normalmente aperto".
Nel protocollo i comandi parlano di linea ("collegata / staccata", "stacca per N ms"), non di
bobina. LED per canale, acceso quando la linea è staccata. Un pulsante per canale (SW1/SW2, su
PB12/PB13): ogni pressione alterna la linea, collegata o staccata (RF-17).

### Upstream

Connettore **USB-C** con Rd 5,1 kΩ su CC1 e CC2 (dispositivo), solo D+/D- (decisione D9).

## Produzione e assemblaggio

PCB **4 strati JLCPCB**. Le strategie differiscono per quanti tipi "extended" si fanno montare a
JLCPCB (sovrapprezzo di ~3 € per tipo) e per l'attrezzatura necessaria a mano. I package QFN
(hub USB2517, TPS2561A, INA3221) **si saldano a mano con aria calda o piastra riscaldante più
stencil**, non con il solo saldatore.

| Strategia | JLCPCB monta | A mano (unico fornitore) | Tipi extended | Attrezzatura a mano |
|---|---|---|---|---|
| **A. JLCPCB monta tutto l'SMD** | tutto: basic + ~8–11 extended | solo through-hole: connettori, relè, boccole, pulsanti | 8–11 (~25–35 €) | saldatore |
| **B. Ibrida** | basic + il QFN più critico: **USB2517** | STM32 LQFP-64, TPS25221 (a pin) o TPS2561A, TS3USB30E, INA219 o INA3221, LDO, 74HC595, ULN2003 e tutto il through-hole, da Mouser | 1 (~3 €) | saldatore; aria calda per TPS2561A e INA3221 |
| **C. Tutto a mano, stessi componenti (scelta)** | solo i basic (passivi, diodi, MOSFET) o niente | tutto, compreso USB2517 | 0 | aria calda o piastra + stencil, lente, test di continuità |
| **D. Tutto a mano, solo package a pin** | solo i basic o niente | tutto, con hub **FE2.1** (LQFP-48), TPS25221, INA219 | 0 | saldatore |

Attrezzatura disponibile: piastra riscaldante e stazione ad aria calda, quindi la **C è la
strategia scelta**: nessun tipo extended, tutti i componenti da un unico fornitore. La B resta il
ripiego se la rifusione dell'hub dà problemi. La D è il ripiego se non si vuole toccare i QFN: il FE2.1 è poco documentato. La A costa ~30 € in più per ordine ma evita di saldare a mano i QFN.

## Meccanica

- Scatola **stampata in 3D su misura**, disegnata sulle misure reali del PCB a layout finito.
- Tutti i connettori saldati sul PCB e affacciati alle aperture della scatola; boccole a banana
  in versione da PCB. Nessun cablaggio interno a filo.
- Connettori sui due lati corti: lato "banco" le 5 porte downstream con i loro pulsanti e LED, le
  4 boccole con i pulsanti e i LED dei relè; lato posteriore upstream e alimentazione.
- PCB fissato alla scatola con viti su boccole filettate o inserti; i pulsanti tattili con
  estensori stampati o pulsanti ad asta lunga.

## Protezioni (uso in laboratorio)

Il dispositivo vive su un banco: si attaccano e staccano spesso schede, alimentatori, cavi e host,
le schede in prova possono essere montate male, ci sono scariche elettrostatiche. Quadro per rischio:

| Rischio | Protezione |
|---|---|
| Alimentatore sbagliato (> 5,84 V, fino a 28 V) all'ingresso | eFuse U13: stacca la 5V_SYS; il firmware lo vede da FLT e dalla tensione misurata sullo shunt |
| Picco all'inserzione dell'alimentatore | avvio dolce dell'eFuse (~5 ms) |
| Corto o sovraccarico su una porta | TPS2561A: limite ~2,3 A, protezione termica, FAULT al MCU e agli OCS del hub; il firmware spegne la porta |
| Corto o sovraccarico sulla 5V_SYS (prima delle porte) | limite 3,5 A dell'eFuse |
| D+/D- in corto con VBUS, o con tensioni fino a 24 V, su una scheda o un cavo | PI3USB4002A: regge 24 V sul lato connettore e si apre sopra 4,75 V; tra switch e connettore non c'è altro che possa portare la tensione verso le alimentazioni (D27) |
| D+/D- in corto con GND o tra loro | innocuo (richiesto dalla specifica USB 2.0) |
| CC in corto con VBUS sulle porte USB-C | innocuo: c'è solo la Rp da 22 kΩ |
| Scheda che rimanda tensione sulla VBUS di una porta | a porta spenta il TPS2561A blocca fino a 7 V; a porta accesa il firmware legge la VBUS dall'INA3221 e spegne la porta sopra ~5,5 V; oltre 7 V non c'è protezione (servirebbe un integrato per porta) |
| Altra sorgente di 5 V su una porta (cavo A-to-C verso un PC o un caricatore, sorgente USB-C non conforme) | le porte non rilevano l'inserimento (D29): il firmware non accende una porta che ha già tensione sulla VBUS e spegne una porta accesa che riceve corrente invece di erogarla (INA3221); in entrambi i casi LED rosso della porta e avviso al PC. Resta scoperto il caso con la nostra VBUS più alta di quella esterna: la corrente esce verso l'altra sorgente come verso un carico, limitata a ~2,3 A dal TPS2561A |
| Scariche elettrostatiche su D+/D- e VBUS | sulle porte: protezione integrata nel PI3USB4002A (10 kV IEC) e condensatori sulla VBUS; sulla porta upstream: USBLC6-2SC6 (15 kV); in layout le protezioni vanno vicino al connettore con ritorno corto a GND |
| Scariche elettrostatiche su CC, pulsanti, LED, boccole | nessun integrato esposto sulle CC (solo resistenze; verso l'ADC filtro 1 kΩ + 100 nF); prolunghe dei pulsanti in plastica; boccole sui contatti dei relè, isolati dalla scheda |
| Corto o sovracorrente sulle boccole | contatti da 3 A: va protetto il circuito esterno (fusibile); snubber RC previsti |
| Anelli di massa (sonda dell'oscilloscopio, schede con massa a un altro potenziale) | non risolti a bordo: la massa USB unisce PC, hub e schede. Isolare a 480 Mbit/s costa e toglie l'alimentazione dal PC (RF-16); per lavori a rischio si usa un isolatore USB commerciale tra PC e hub |
| Corrente dal hub verso il PC | diodo D3 nell'ORing |

## Rischi / punti aperti tecnici

- Integrità del segnale Hi-Speed attraverso lo switch analogico: tracce corte, 90 Ω differenziali,
  nessun via inutile, ESD subito al connettore.
- Inrush sulla 5V_SYS con 5 porte accese insieme: il firmware accende le porte sfalsate di qualche
  decina di ms.
- Alimentatori USB-C che pubblicizzano 3 A ma non li reggono: il budget è impostabile a mano via
  HID e il firmware reagisce al calo di 5V_SYS.
- TPS2561A: due canali a 2,3 A nello stesso VSON-10 (RθJA 47,8 °C/W): pad termico su piano di
  rame con via verso il piano di massa.
