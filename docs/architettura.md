# Architettura

L'architettura funzionale segue i requisiti semplificati D60–D66 e le decisioni D68–D87: controllo delle porte
e riavvio temporizzato, senza misura dei consumi, delle tensioni delle porte/5V_SYS o delle CC
d'ingresso. Lo schema v0.3 la applica (D79); il PCB da 100 × 100 mm (D84) ha il
piazzamento iniziale ed è da sbrogliare.
D75 sceglie due UCS2114-1-V/LX con due bus I²C indipendenti e quattro PI3USB4002A.
La scarica automatica va configurata all’avvio, con 400 ms come durata iniziale da validare.
Componenti e alternative: [componenti.md](componenti.md).
**USB2517I-JZX con STM32G0B1CEU6 confermati (D78)**: HID multipiattaforma, cicli e impulsi
autonomi, cinque pulsanti e LED porta con lampeggio guasti. La
[variante USB4715](componenti.md#usb4715-senza-mcu-verifica-di-fattibilità) non è adottata.
La selezione mantiene USB2517I-JZX e riduce il MCU a STM32G0B1CEU6 (UFQFPN48).
Servono 37 segnali funzionali, inclusi i due bus I²C; restano quattro GPIO liberi oltre
ai due pin riservati DBCC. [Pinout](pinout-mcu.md).

All’avvio del firmware le quattro porte sono abilitate di fabbrica, con accensioni
sfalsate, salvo default diversi salvati (D74). Reset e bootloader restano da verificare
elettricamente sul prototipo.

## Schema a blocchi funzionale

Numerazione delle porte downstream (DS) del USB2517.

```
USB-C PWR IN, alimentatore richiesto 5 V / 3 A
  CC1/CC2: solo Rd 5,1 kΩ, nessuna lettura/verifica
  VBUS → TVS + eFuse → 5V_SYS → alimentazione porte e relè
                         └─→ Schottky + LDO 3,3 V → logica

USB-C UPSTREAM → HUB USB 2.0 USB2517I-JZX
  VBUS upstream → VBUS_DET (hub/MCU, conservato)
  DS1..DS4 → PI3USB4002A (dati) + UCS2114 (VBUS e scarica) → 4 USB-C
  DS5      → non utilizzata (disabilitata via SMBus)
  DS6      → STM32G0B1, USB HID (mai sezionato)
  DS7      → non utilizzata (disabilitata via SMBus, D76)

MCU → comando logico e gestione guasti per porta
    → un LED e un pulsante per porta
    → un relè con pulsante e LED dedicati
    → LED di host collegato
    → configurazione hub, SWD, UART
```

La scarica può essere automatica allo spegnimento della porta (D70): il MCU temporizza
il ciclo OFF/ON, senza necessità di un GPIO DIS o di un comando separato della scarica.
Si sceglie la soluzione complessivamente più semplice, rispettando i limiti elettrici.

Commutazione dati, VBUS e scarica possono essere integrate nello stesso componente:
non sono prescritti interruttori separati o una sequenza pilotata pin per pin dal MCU.
Non sono previsti INA3221, shunt o ADC sostitutivi per le misure eliminate.

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
- Dati e alimentazione rispondono a un unico comando logico (RF-3, D62). Sono ammessi ordine
  e tempi interni del controller; a porta disabilitata entrambi devono risultare interrotti.
  Non esiste un ritardo dati/VBUS configurabile.

Per una scheda alimentata anche esternamente il distacco dati resta richiesto, ma il riavvio
richiede un intervento sull'alimentazione esterna o sul reset. Il completamento del ciclo
non certifica elettricamente che la scheda si sia spenta o sia ripartita.

### Porte: stato solido, non relè

Commutazione VBUS, scarica e protezioni sono integrate nel controller delle porte, senza
sensori di corrente:

- **VBUS**: due switch doppi **Microchip UCS2114-1-V/LX** (VQFN-20 3×3, D75), U4 per le porte 1-2
  su I2C1 e U5 per le porte 3-4 su I2C2, entrambi all'indirizzo fisso 0x57. Lo strap
  **COMM_ILIM = 27 kΩ 1 %** seleziona il modo SMBus, PWR_EN attivi alti e il limite ILIM7
  **2,50–2,90 A** per porta. RON massimo 30 mΩ: a 2 A ~60 mV e ~120 mW per canale. Blocco della
  corrente inversa (soglia tipica 150 mV), ALERT# open-drain per porta, **scarica automatica di
  VBUS** (100 Ω interni) abilitata dal MCU all'avvio con EN_VBUS_DISCHG e timer di 400 ms:
  nessun MOSFET o GPIO di scarica. Disaccoppiamento: 100 nF su VDD e 1 µF per coppia di pin VS;
  BOOST# non usato, a massa. I PWR_EN hanno pull-down da 10 kΩ (D80): porte spente al reset e
  nel bootloader. Dettagli e alternative in [componenti.md](componenti.md#raccomandazione-per-le-quattro-porte-ucs2114-e-pi3usb4002a).
- **D+/D-**: **Diodes PI3USB4002A** (UQFN-10 1,5×2 mm): interruttore USB 2.0 (banda 1 GHz, 5 Ω)
  con EN alto = linee in alta impedenza, così l'hub vede il distacco. Il lato connettore regge
  24 V e si apre da solo sopra 4,75 V: un corto D+/D- su VBUS in una scheda in prova o in un cavo
  non arriva alla 3,3 V (D22). Sostituisce il TS3USB30E, che sui pin dati reggeva solo VCC + 0,3 V.

**Ciclo temporizzato (D61/D62).** Il comando disabilita VBUS e dati secondo la sequenza
del controller; una scarica attiva riduce VBUS. `off_ms` si conta dal comando di
disabilitazione VBUS, comprende la scarica e termina con il comando di riabilitazione.
Default indicativo **1000 ms**, da confermare sul circuito e sul prototipo. Non si misura VBUS,
non si attende il passaggio sotto 0,8 V e non si espone `on_delay_ms`.

Il tempo necessario dipende dalla capacità totale della porta e della scheda, oltre che dal
circuito di scarica. Per esempio, nel modello ideale da 5 V con 100 Ω, 1000 µF raggiungono
0,8 V in circa 183 ms; questo calcolo non garantisce il reset di qualunque dispositivo.
Capacità maggiori possono richiedere più tempo; nessuna attesa spegne una sorgente esterna.

Durata della scarica e durata del ciclo sono vincoli distinti: lasciare una porta spenta o
richiedere un ciclo lungo non deve mantenere una scarica attiva oltre i limiti del circuito.
Nell'UCS2114 la scarica è temporizzata dal controller, che la interrompe e segnala errore se
VBUS resta alta (sorgente esterna). Il default e l'intervallo configurabile restano da
validare insieme alla scarica.

Resta un relè per una sola linea ausiliaria su due boccole a banana (D66).

### Alimentazione: 5 V diretti, senza USB-PD

È richiesto un alimentatore USB-C che offra **5 V / 3 A senza negoziazione USB-PD**, con
cavo adeguato. Un alimentatore PD è compatibile solo se offre anche questa modalità:
non tutti lo fanno. L'utilizzatore deve scegliere alimentatore e cavo idonei; non si verifica
l'annuncio di 3 A e non si garantisce il funzionamento con potenza inferiore (D65).

CC1 e CC2 mantengono le **Rd da 5,1 kΩ**. Sono da eliminare collegamenti al MCU e filtri
R3/R4/C1/C2: nessuna lettura ADC, nessun uso di UCPD per verificare la corrente disponibile.
La gestione elettrica dei pin UCPD usati come GPIO e del bootloader resta un problema distinto.

I 3 A comprendono tutti i consumi. Stima con i componenti dello schema v0.3, da confermare
sul prototipo:

| Voce | Valore indicativo |
|---|---|
| Hub USB2517 (3,3 V tramite LDO), quattro porte Hi-Speed e la porta Full-Speed del MCU | 0,33 A tipici (datasheet, 4 porte HS), ~0,37 A massimi stimati |
| MCU, switch dati, sei LED e pull-down EN sulla 3,3 V; UCS2114 sulla +5V | circa 0,03 A |
| Un relè eccitato | circa 0,08 A |
| Disponibile alle porte | circa 2,5 A |

Le quattro USB-C conservano 2 A sulla singola porta; non sono correnti disponibili
contemporaneamente su tutte le porte. Hub e MCU sono alimentati solo dalla 5V_SYS (D32).

**Gestione dei guasti (D64/D73).** Un guasto porta segnalato blocca la porta in OFF
e interrompe l’eventuale ciclo, con LED lampeggiante; il MCU attende un tentativo esplicito
da pulsante o PC. Non è richiesta una funzione latch-off integrata nel controller.
 Le accensioni multiple sono sfalsate per contenere l'inrush,
con intervallo da verificare sul circuito. Restano limitazione hardware e segnali di guasto;
il firmware non calcola consumi o budget, non sceglie la porta più energivora e non espone
un budget impostabile. Un sovraccarico complessivo può far intervenire alimentatore/eFuse e
spegnere l'intero hub: non si garantisce la continuità delle altre porte. Le azioni del MCU
dipendono dai guasti realmente segnalati e dal fatto che la logica resti alimentata.

**Protezione d'ingresso.** TVS **SMAJ20A** ed eFuse **TPS259470A** (U13, D21), la cui uscita
è direttamente la 5V_SYS (rete `+5V`), senza shunt:
- con un solo partitore (R30/R31/R32, D85) stacca l'uscita sopra 5,69 V (5,52–5,90 V con le
  tolleranze, rientro sotto 5,17 V), sotto i 6 V massimi assoluti degli UCS2114 alimentati
  dalla +5V; in sottotensione accende sopra 4,0 V e stacca sotto 3,64 V. Lo zener D4 tiene il
  pin EN sotto il suo massimo (6,5 V) fino a 28 V d'ingresso;
- limite di corrente 3,5 A (R33), transitori ammessi fino a ~8 ms (C12), ripartenza automatica;
- avvio dolce: la 5V_SYS sale in ~5 ms e i 460 µF di bulk (C3/C4 elettrolitici SMD da 220 µF,
  D86, più C5/C6) si caricano con ~0,4 A invece di un picco
  di decine di ampere; i condensatori d'ingresso restano quelli attuali (D20);
- blocca la corrente inversa; FLT (open-drain) va al MCU su PB1. FLT va basso solo per
  sovracorrente prolungata (dopo il tempo di C12), sovratemperatura, guasto del pin ILM e corrente
  inversa; con sovratensione o sottotensione resta alto (datasheet, tabella 7-3);
- la logica è a valle dell'eFuse (D32): quando l'eFuse interrompe l'alimentazione, hub e MCU
  si spengono e non possono garantire una segnalazione al PC. FLT è gestibile finché il MCU
  è alimentato; non sostituisce una misura di corrente né garantisce un intervento preventivo.
  Non è prevista la lettura di corrente totale o tensione 5V_SYS.

Caduta aggiuntiva: ~28 mΩ, cioè ~0,1 V a 3 A.

**Limiti della catena d'ingresso (D31).** Il massimo assoluto dell'eFuse è 28 V. La SMAJ20A non conduce fino a 20 V, inizia a 22,2–23,4 V e limita a ~25 V con
1 A, ~26 V con 2 A, ~28 V con 5 A, 32,4 V alla corrente di prova di 12,3 A (10/1000 µs). La
protezione è quindi **garantita in continuo fino a 20 V**; sopra ~23 V la TVS conduce: fino a ~5 A tiene
l'ingresso sotto i 28 V dell'eFuse, con un alimentatore più potente si sacrifica. Le prove impulsive da 10/1000 µs riguardano linee lunghe o di rete,
non un ingresso USB; i picchi all'inserzione hanno poca energia e restano entro ~23–25 V.

**TVS (D25).** La TVS d'ingresso è una **SMAJ20A**: non conduce fino a 20 V (il massimo di un
alimentatore USB-C) e limita i picchi oltre ~22 V. Una TVS da 5 V (la SMAJ5.0A di prima) entrerebbe
in conduzione già a ~6,4 V e con un alimentatore da 12–20 V collegato per errore brucerebbe prima
che l'eFuse possa staccare; con la SMAJ20A è l'eFuse a staccare la 5V_SYS e, tolto l'alimentatore
sbagliato, tutto riparte. Sulle VBUS delle porte non c'è TVS: una scarica da 8 kV (~1,2 µC) sui
22 µF della porta alza la tensione di ~0,05 V, lo stacco di un cavo sotto carico di pochi decimi
di volt; contro una tensione continua rimandata da una scheda una TVS non serve (brucerebbe)
e una da 5 V limiterebbe comunque sopra i 6 V massimi assoluti dell'UCS2114.

**Logica (D32).** 3V3 da LDO a basso dropout (**AP7361C-33E**, 1 A, SOT-223) alimentato dalla
5V_SYS tramite lo Schottky D2: la logica si alimenta solo dall'alimentatore e senza alimentatore
l'hub non si vede dal PC. D2 resta per abbassare la tensione sull'LDO e quindi la dissipazione
(senza, ~0,15 W in più a 0,4 A). Carico 3,3 V fino a ~0,4 A (USB2517 con quattro porte
Hi-Speed, più il resto della logica), ~0,36 A nell'uso tipico. Nel caso peggiore l'alimentatore
è al massimo (5,25 V, fino a 5,5 V per il Type-C), le porte assorbono poco e all'LDO arrivano
~4,9–5,15 V: dissipa **0,65–0,75 W**, ~0,45 W nell'uso tipico. Con i 110 °C/W della piazzola
minima la giunzione arriverebbe a ~120 °C a 40 °C ambiente, con poco margine dalla protezione
termica (150 °C): il tab (GND) va usato come dissipatore, con rame e via verso i piani GND, per
avere θJA ≤ ~60 °C/W, cioè ≤ ~85 °C di giunzione nel caso peggiore (D56,
[regole-pcb.md](regole-pcb.md)). Scartato l'AP2112K in SOT-23-5, che con lo stesso carico
rischiava la protezione termica (D17). Scartato anche un buck: dalla 5 V prenderebbe ~0,1 A in
meno (~0,3 A invece di 0,4 A nel caso peggiore), non abbastanza per giustificare componenti,
rumore di commutazione vicino al hub e rifacimento della zona di alimentazione (D56).
Variante possibile in futuro: hub funzionante anche dal solo PC (commutazione tra le due sorgenti,
budget e protezioni separati); scartata per ora perché complica molto l'alimentazione (D32).

#### Variante ad alta potenza (non in v1)

Se servissero 6–10 A totali: PD sink sul MCU (UCPD + middleware ST, AN5418) o **STUSB4500**
standalone, protezione **TCPP01-M12**, buck **LM62460 / LM61480 / LM61495** (6 / 8 / 10 A,
pin-compatibili nello stesso VQFN-HR 16, 3–36 V in, ~1,6 $), alimentatore PD da 45–65 W. La gestione della potenza richiederebbe requisiti propri: non fa parte della v1 semplificata.

### Hub

**Microchip USB2517I** (7 porte, QFN-64 passo 0,5 mm con pad esposto, quarzo 24 MHz,
configurazione via resistenze o SMBus, regolatore 1,8 V interno). Il package si salda a mano con
aria calda o piastra riscaldante e stencil: i pad perimetrali sono sotto il bordo e il pad
centrale è la massa e va saldato; con il solo saldatore è un ripiego (pad centrale scaldato dai
via termici sul retro). 7 porte coprono 4 USB-C + 1 porta interna per il MCU,
due porte non utilizzate (DS5 e DS7), senza header di scorta (D76).

Dettagli verificati sul datasheet (DS00001598C) e sull'errata (DS80000628A):

- **PRTPWR** del hub non usati: l'abilitazione delle porte è del MCU. Gli **ALERT#** degli
  UCS2114 (open-drain, pull-up esterni da 10 kΩ) vanno sia al MCU sia agli ingressi
  **OCS1_N…OCS4_N** del hub: l'host riceve così la notifica standard di sovracorrente.
  OCS5_N non è collegato (pull-up interno).
- **Configurazione via SMBus** (CFG_SEL = 001, indirizzo 0x2C): il MCU scrive VID/PID, stringhe,
  porta 6 "non rimovibile" (il MCU stesso), disabilitazione delle porte 5 e 7, corrente massima,
  e poi comanda l'attach. Gli strap di disabilitazione delle porte non servono: con CFG_SEL 001
  e 101 sono disattivati, e senza firmware DS5/DS7 risultano solo porte vuote. CFG_SEL0 è
  il pin SCL (alto per il pull-up I2C), CFG_SEL1 è a massa, **CFG_SEL2 lo pilota il MCU** prima di
  rilasciare il reset: basso = SMBus (001), alto = default interni con dynamic power (101), che è
  anche lo stato del pull-up per il bring-up senza firmware.
- **Reset** (D16): l'errata dice che il POR interno non funziona e suggerisce un supervisore
  esterno. RESET_N ha un pull-up da 10 kΩ e due sorgenti in wired-OR: il supervisore **MCP120T-300**
  (open-drain, soglia 2,85–3,0 V, ritardo 150–700 ms), che tiene il hub in reset finché la 3,3 V non
  è stabile, e PF1 del MCU in open-drain, che può forzare il reset (comando `RESET_HUB`). Il ritardo lo genera
  il MCP120 internamente; il 100 nF C24 sulla linea è DNP (D41): servirebbe solo da filtro contro
  disturbi (il hub si resetta con 1 µs basso) e scaricarlo di colpo da PF1 supera i 15 mA per pin
  dello STM32. Da montare, anche da 1 nF, solo se sul prototipo compaiono reset spuri del hub. Un reset del
  MCU (watchdog, `REBOOT`, debugger, bootloader) lascia PF1 in alta impedenza e **non resetta il
  hub**. All'accensione il MCU parte prima che il supervisore rilasci, tiene basso il reset, sceglie
  CFG_SEL2 e lo rilascia; senza firmware il hub parte comunque con i default interni (101).
  Il reset del hub non sta su PA8: è un pin CC dell'UCPD, con una pull-down da ~5,1 kΩ dopo ogni
  reset del MCU e anche nel bootloader. Con R27 da 10 kΩ avrebbe tenuto RESET_N a ~1,1 V, tra le
  soglie del hub (0,8 / 2,0 V), proprio durante il DFU (D34).
  Un calo della 3,3 V sotto la soglia di U12 ma sopra quella del MCU (1,7 V) resetta solo il hub
  (D35): il firmware lo rileva leggendo HUB_RST_N, prende il controllo del reset, lo rilascia con il
  bus I2C fermo (SCL è anche CFG_SEL0) e riconfigura il hub solo dopo il rilascio effettivo di U12.
  Procedura in [pinout-mcu.md](pinout-mcu.md).
- **Avvio senza firmware e DFU (D34).** I pin del MCU sono scelti in modo che con il bootloader ST
  (AN2606) il hub parta con i default, veda VBUS e LOCAL_PWR corretti, e relè e porte restino a
  riposo: dettaglio in [pinout-mcu.md](pinout-mcu.md). Non è ancora dimostrato: è la prima prova da
  fare sul prototipo (DFU con flash vuota e dopo `ENTER_BOOTLOADER`).
- **VBUS_DET** dalla VBUS upstream con partitore 100 k / 100 k, letto anche dal MCU (PB2, ADC).
  È il circuito raccomandato dalla USB2517 Hardware Design Checklist di Microchip (sezione 5.1):
  con l'alimentatore staccato e il PC collegato il nodo è a ~2,5 V con hub e MCU spenti, e i
  100 kΩ limitano a qualche decina di µA la corrente che entra nel pin (D53). PB2 è un pin FT
  (D32).
- **LOCAL_PWR** con pull-up R26 da 10 kΩ verso la 3,3 V (D40): con la logica alimentata solo
  dall'alimentatore il hub è sempre self-powered. Resistenza e non collegamento diretto, perché con
  la configurazione via SMBus il pin 45 può diventare l'uscita SUSP_IND.
- RBIAS 12,0 kΩ 1 %; quarzo 24 MHz YXC X322524MOB4SI (C_L 12 pF) con 15 pF C0G (D19; il hub tollera ±350 ppm); 1,8 V dal
  regolatore interno (VDD18, VDD18PLL con 1 µF ciascuno); TEST a massa; slug a massa.
- Mappa: 1–4 → USB-C, 5 disabilitata, 6 → MCU, 7 disabilitata; D+/D− di DS5 e DS7 non
  collegati. Quattro porte esterne più il MCU richiedono almeno cinque downstream: un hub a quattro
  porte non basta. La ricerca di alternative è in [componenti.md](componenti.md#ricerca-per-la-versione-semplificata-d60d66).

Alternativa saldabile a mano: **FE2.1** (LQFP-48), documentazione scarsa ma diffusissimo.

### MCU di controllo: STM32G0B1CEU6

Scelto tra gli STM32 per questi motivi:

- **USB FS device senza quarzo** (HSI48 + CRS). Ha anche la periferica **UCPD**: non serve in v1
  (nessuna negoziazione PD) ma lascia aperta la variante ad alta potenza senza cambiare MCU.
- **UFQFPN48** 7 × 7 mm con pad esposto (D78): due I²C hardware, tutti i 37 segnali richiesti
  e quattro GPIO liberi; montaggio ad aria calda o piastra, come hub e controller delle porte.
  Collegamenti in [pinout-mcu.md](pinout-mcu.md).

- 512 kB flash / 144 kB RAM: largo per stack USB + logica, e per uno stack PD futuro.
- Su DigiKey 497-STM32G0B1CEU6-ND.

Le versioni LQFP48 (STM32G0B1CCT6/CET6) hanno la stessa assegnazione dei pin ma un'impronta
diversa; le varianti N non sono compatibili (pin 30/31 di alimentazione).

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

Requisiti del blocco da realizzare:

| Funzione | Scelta funzionale |
|---|---|
| Commutazione | VBUS e D+/D− interrotte con unico comando logico, anche con dispositivo autoalimentato; sequenza interna ammessa |
| Alimentazione | 5 V, 2 A su ciascuna delle quattro USB-C, entro il totale condiviso |
| Scarica | Attiva, integrata o discreta; ciclo temporizzato senza misura VBUS; durata sicura da definire |
| Protezioni | Limitazione hardware, blocco inverso, ESD e segnalazione di guasto; limiti da verificare sul controller |
| Misure | Nessun sensore di corrente o tensione per porta |
| CC downstream | Rp da 56 kΩ alla VBUS commutata; invariata la scelta di non rilevare l'inserimento (D29/D30) |
| LED | Uno dal MCU: spento = disabilitata, fisso = abilitata, lampeggiante = guasto, prioritario; stato logico |
| Pulsante | Ogni pressione alterna lo stato; debounce firmware e notifica all'host; nessun controllo del budget |

Le porte USB-C attuali non rilevano l'inserimento sulle CC: VBUS è presente quando la porta
è abilitata. La scelta resta distinta dalla rimozione della lettura CC **d'ingresso**. I
controlli firmware di VBUS e corrente inversa previsti da D29 sono eliminati: non si promette
più rilevamento o rifiuto automatico di sorgenti esterne. Le protezioni effettive dipendono
dal circuito, non da una verifica software.

L'annuncio standard USB con Rp da 56 kΩ resta invariato. Il limite fisico per porta è distinto
dall'annuncio e non garantisce la disponibilità simultanea delle correnti massime. I condensatori
e le protezioni locali vanno verificati sul prototipo; ogni porta ha 22 µF + 100 nF su VBUS.

### Relè ausiliario

Specifica contatti: **max 16 V, max 3 A**. Un relè **SPDT** con bobina 5 V (es. Omron G5Q-1,
Songle SRD-05VDC-SL-C: 10 A nominali, ampio margine) pilotato da un ULN2003V12 con tre canali in
parallelo (diodi di ricircolo integrati); gli altri canali hanno gli ingressi a massa. Su pannello **due boccole a banana da 4 mm su COM e NC**: la linea
è **normalmente collegata** e la bobina si eccita solo per staccarla. Così lo stacco dura il tempo
di un impulso, e se l'hub perde alimentazione o si blocca la linea torna collegata (comportamento
sicuro per una scheda in prova). Il contatto NO è portato su un **ponticello a saldare** interno:
spostandolo la seconda boccola diventa NO per un eventuale uso "normalmente aperto".
Nel protocollo i comandi parlano di linea ("collegata / staccata", "stacca per N ms"), non di
bobina. Un LED (D90, dal comando RLY1), acceso quando la linea è staccata (D90), e un pulsante (SW1):
ogni pressione alterna la linea, collegata o staccata (RF-17).

**Margine sulla bobina (D33).** Il G5Q-1 da 5 V (63 Ω, 80 mA) scatta garantito con 3,75 V a 23 °C
di bobina; la resistenza cresce con la temperatura (~0,4 %/°C) e con la bobina a 40 °C servono
~4,0 V. Obiettivo: scatto garantito fino a 5V_SYS = 4,5 V e 40 °C; è un punto di verifica elettrica, non una soglia misurata dal firmware. Il ULN2003A (Darlington) perdeva 0,9–1,1 V e lasciava 3,4–3,6 V alla bobina; il
ULN2003V12 ha uscite MOSFET (~6 Ω per canale a 3,3 V d'ingresso) e con tre canali in parallelo
perde ~0,2 V: alla bobina restano ~4,3 V.

### Upstream

Connettore **USB-C** con Rd 5,1 kΩ su CC1 e CC2 (dispositivo), solo D+/D- (decisione D9).

## Produzione e assemblaggio

PCB **4 strati JLCPCB**. Le strategie differiscono per quanti tipi "extended" si fanno montare a
JLCPCB (sovrapprezzo di ~3 € per tipo) e per l'attrezzatura necessaria a mano. I package QFN
(ad esempio il hub USB2517) **si saldano a mano con aria calda o piastra riscaldante più
stencil**, non con il solo saldatore.

| Strategia | JLCPCB monta | A mano (unico fornitore) | Tipi extended | Attrezzatura a mano |
|---|---|---|---|---|
| **A. JLCPCB monta tutto l'SMD** | tutto: basic + ~8–11 extended | solo through-hole: connettori, relè, boccole, pulsanti | 8–11 (~25–35 €) | saldatore |
| **B. Ibrida** | basic + il QFN più critico: **USB2517** | STM32 UFQFPN48, UCS2114 e PI3USB4002A, LDO, ULN2003V12 e tutto il through-hole, da Mouser | 1 (~3 €) | saldatore; aria calda per i QFN del controller scelto |
| **C. Tutto a mano, stessi componenti (scelta)** | solo i basic (passivi, diodi, MOSFET) o niente | tutto, compreso USB2517 | 0 | aria calda o piastra + stencil, lente, test di continuità |
| **D. Tutto a mano, solo package a pin** | solo i basic o niente | tutto, con hub **FE2.1** (LQFP-48), controller delle porte a pin da verificare | 0 | saldatore |

Attrezzatura disponibile: piastra riscaldante e stazione ad aria calda, quindi la **C è la
strategia scelta**: nessun tipo extended, tutti i componenti da un unico fornitore. La B resta il
ripiego se la rifusione dell'hub dà problemi. La D è il ripiego se non si vuole toccare i QFN: il FE2.1 è poco documentato. La A costa ~30 € in più per ordine ma evita di saldare a mano i QFN.

## Meccanica

- Scatola **stampata in 3D su misura**, disegnata sulle misure reali del PCB a layout finito.
- Tutti i connettori saldati sul PCB e affacciati alle aperture della scatola; boccole a banana
  in versione da PCB. Nessun cablaggio interno a filo.
- Connettori su due lati opposti: lato "banco" le 4 porte downstream e le 2 boccole del relè; lato
  posteriore upstream e alimentazione.
- PCB 100 × 100 mm (D84): sulla faccia superiore solo LED e pulsanti, che si vedono e si premono dal
  coperchio, ciascuno dietro la sua porta o coppia di boccole, con i pulsanti a 14,5 mm dal bordo frontale (D87); tutto il resto, connettori compresi,
  sulla faccia inferiore. Le due boccole (COM e NC del relè) sono affiancate; il loro manicotto
  isolante (14,35 mm, senza filetto) attraversa la parete frontale, ispessita intorno alle boccole
  perché il manicotto finisca a filo della faccia esterna.
- PCB fissato alla scatola con 4 viti M3 su boccole filettate o inserti, due agli angoli
  posteriori e due lontano dagli angoli anteriori, occupati da boccole e porte (D84); i pulsanti tattili con
  estensori stampati o pulsanti ad asta lunga.

## Protezioni (uso in laboratorio)

La rimozione delle misure non elimina le protezioni hardware: eFuse/TVS all'ingresso,
limitazione di corrente e blocco inverso negli UCS2114, switch protetti PI3USB4002A e
USBLC6-2SC6 upstream. Le prestazioni vanno verificate sul prototipo.

| Rischio | Comportamento richiesto o limite |
|---|---|
| Corto o sovraccarico di una porta | Limitazione hardware e gestione del guasto disponibile; nessuna misura di ampere |
| Sovraccarico totale | Protezioni d'ingresso/alimentatore; possibile spegnimento di tutto l'hub |
| Sovratensione o picco all'ingresso | Conservare eFuse, TVS e avvio dolce; limiti del circuito come descritti sopra |
| Corto dei dati su VBUS o scarica ESD | Protezione hardware; verificarne le prestazioni con il nuovo controller e il layout |
| Ritorno di alimentazione da una scheda | Blocco inverso negli UCS2114; nessuna diagnosi software tramite misure. Sorgente esterna 5 V nominali con tolleranza (D72); comportamento acceso/spento da verificare sul controller |
| Sorgente esterna sulle porte | Nessuna misura preventiva di VBUS o di corrente inversa; non garantiti rilevamento e riavvio della scheda. Permanenza indefinita senza danni richiesta (D71), con sorgente da 5 V nominali e tolleranza (D72), non 12/20 V; la scarica può interrompersi automaticamente |
| Corrente verso il PC tramite VBUS upstream | Nessun collegamento fra 5V_SYS e VBUS upstream (D32) |
| Guasto sulle boccole | Contatti max 16 V / 3 A; proteggere il circuito esterno, snubber opzionali |
| Anelli di massa | GND USB comune: nessun isolamento galvanico delle porte USB |

Le cause di guasto esposte al PC dipendono dai segnali del controller. Non si attribuisce
una causa specifica a un segnale generico e non si promette un evento prima della perdita
di alimentazione. Il LED lampeggia per il guasto, senza indicare una tensione misurata.

## Rischi / punti aperti tecnici

- Dimensionare il circuito D75 (UCS2114-1-V/LX e PI3USB4002A) e verificarne distacco
  dati/VBUS, sequenza, correnti, protezioni, scarica e stato al reset/bootloader.
- Validare default del ciclo (indicativamente 1 s), intervallo configurabile e durata della
  scarica, con capacità note e condizioni di alimentazione esterna. La prova non estende
  la garanzia a qualsiasi capacità o scheda autoalimentata.
- Verificare inrush e intervallo tra accensioni; rivalutare disponibilità di corrente e
  dissipazione con il circuito definitivo. Nessuna gestione dinamica del budget nel firmware.
- Adeguare schema, PCB, pinout, BOM e regole specifiche a D60–D66, mantenendo integrità del
  segnale USB Hi-Speed e requisiti termici. Il layout esistente non incorpora ancora le modifiche.
