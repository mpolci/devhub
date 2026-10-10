# Architettura

Documento di lavoro: raccoglie le scelte fatte, le motivazioni e i componenti scelti o candidati.
Le voci marcate **[da verificare]** vanno confermate sui datasheet prima di disegnare lo schema.
La tabella dei componenti con package, fornitore e classe JLCPCB è in [componenti.md](componenti.md).

## Schema a blocchi

```
USB-C PWR IN 5 V / 3 A ──► TVS + shunt INA3221 ──► 5V_SYS ────────────────────────────────┐
(qualunque alimentatore      │                       │                                     │
 USB-C, anche PD)            │ CC1/CC2 (Rd 5,1 kΩ)    ▼ ORing Schottky + LDO 3,3 V          │
                             └──► ADC del MCU      3V3 (hub, MCU, switch)                  │
                                  (0,5 / 1,5 / 3 A)    ▲                                   │
USB-C UPSTREAM ─── VBUS ───────────────────────────────┘ (RF-16)                           │
(verso il PC)   │                                                                          │
                └──► HUB USB 2.0, 7 porte (USB2517)                                        │
                         ├─ DS1..DS4 ─► TS3USB30E ─► USB-C 1..4 ◄─ TPS2561A + scarica ◄────┤
                         ├─ DS5 ─────► TS3USB30E ─► USB-A      ◄─ TPS2561A + scarica ◄────┤
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
Hi-Speed e dà piani solidi per i 10 A a 5 V). Un'eventuale variante USB 3 resta un progetto
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
- Sezionare dati e alimentazione separatamente dà in più il requisito RF-3.

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
- **D+/D-**: **TI TS3USB30E** (VSSOP-10): mux 1:2 per USB Hi-Speed, banda 1 400 MHz, Ron 10 Ω,
  ESD 15 kV sulle linee. Si usa un solo lato del mux; con OE alto le linee sono in alta impedenza e
  l'hub vede il distacco.

Sequenza di stacco (default nel firmware): OE dati off → EN VBUS off → DIS on per 100 ms.
Riattacco: EN VBUS on → dopo `on_delay_ms` (default 50 ms) OE dati on, così il dispositivo si
enumera già alimentato stabilmente.

I relè restano per le due linee ausiliarie su banana.

### Alimentazione: 5 V diretti, senza USB-PD

Decisione: l'ingresso è un USB-C a **5 V senza negoziazione PD**. Su USB-C un alimentatore eroga
al massimo 5 V / 3 A (15 W) senza PD; per avere di più bisogna negoziare una tensione più alta e
abbassarla con un buck. Si è scelto di **non** farlo, accettando un totale di ~2,5 A per le porte,
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
| Hub + MCU + switch + LED | ~0,25 A |
| 2 relè eccitati (bobine 5 V) | ~0,15 A |
| Disponibile per le porte | ~2,5 A |

**Lettura del budget.** Sulle CC dell'ingresso il MCU ha Rd da 5,1 kΩ e legge con l'ADC la
tensione imposta dall'Rp dell'alimentatore: ~0,4 V → 0,5 A, ~0,9 V → 1,5 A, ~1,7 V → 3 A
(soglie USB Type-C: 0,66 V e 1,23 V). Budget per le porte = corrente pubblicizzata − 0,4 A.
La periferica UCPD dello STM32G0B1 può fare la stessa lettura senza stack PD, ma l'ADC basta.

**Gestione del budget (firmware).** Il sesto canale dell'INA3221 misura la corrente totale
d'ingresso. Regole:
1. una porta si accende solo se `I_totale + 200 mA < budget`;
2. le porte si accendono sfalsate di ~30 ms (inrush);
3. se `I_totale > budget` per più di 50 ms, oppure 5V_SYS scende sotto 4,5 V (ADC), si spegne
   la porta che assorbe di più e si manda l'evento `PORT_FAULT` con causa "budget";
4. il budget è anche impostabile a mano via HID (`SET_DEFAULTS`), per alimentatori che non
   pubblicizzano correttamente.
Il limite di corrente dell'alimentatore resta la protezione di ultima istanza.

**Protezione d'ingresso.** TVS 5 V (SMAJ5.0A) e, opzionale, un eFuse / load switch da 3 A con
blocco della corrente inversa (es. TPS2596) al posto del semplice diodo di ORing.

**Logica (RF-16).** 3V3 da LDO a basso dropout (AP2112K-3.3, 600 mA) alimentato da un ORing a diodi
Schottky tra 5V_SYS e la VBUS upstream: con alimentatore assente hub e MCU enumerano e riportano "alimentatore assente"; le
porte downstream si attivano solo se 5V_SYS è presente. Il diodo impedisce di iniettare 5V_SYS
verso l'host.

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
- **Reset**: l'errata dice che il POR interno non funziona; RESET_N è pilotato dal MCU (pull-down,
  hub in reset finché il firmware non lo rilascia), con ponticello verso un pull-up RC per il
  bring-up. Questo dà anche il comando "reset hub" via HID.
- **VBUS_DET** dalla VBUS upstream con partitore 100 k / 100 k; **LOCAL_PWR** da un partitore su
  5V_SYS (alto = alimentatore presente).
- RBIAS 12,0 kΩ 1 %; quarzo 24 MHz con condensatori da adattare al C_L del quarzo; 1,8 V dal
  regolatore interno (VDD18, VDD18PLL con 1 µF ciascuno); TEST a massa; slug a massa.
- Mappa porte: 1–4 → USB-C, 5 → USB-A, 6 → MCU, 7 → header interno. Le porte 1–5 sono sul lato
  inferiore del QFN, da orientare verso il pannello frontale.

Alternativa saldabile a mano: **FE2.1** (LQFP-48), documentazione scarsa ma diffusissimo.

### MCU di controllo: STM32G0B1RET6

Scelto tra gli STM32 per questi motivi:

- **USB FS device senza quarzo** (HSI48 + CRS). Ha anche la periferica **UCPD**: non serve in v1
  (nessuna negoziazione PD) ma lascia aperta la variante ad alta potenza senza cambiare MCU.
- **LQFP-64**, saldabile a mano, ~60 GPIO: bastano per pilotare tutto senza expander. Conteggio:
  5 porte × (EN, FAULT, OE, DIS, pulsante) = 25, 74HC595 per i LED = 3, 2 relè, I2C condiviso = 2,
  USB = 2, ADC (CC1, CC2, 5V_SYS) = 3, reset hub = 1, SWD = 2, UART debug = 2:
  totale ~42 sui 60 disponibili.
- 512 kB flash / 144 kB RAM: largo per stack USB + logica, e per uno stack PD futuro.
- Disponibile su LCSC (C2829307, ~2,7 $) e su Mouser/DigiKey.

Il modello **STM32G0B1RCT6** (256 kB) è equivalente se più disponibile.

Interfacce USB esposte:
- **HID** con usage page vendor: il canale di controllo, senza driver.
- **CDC** (opzionale): console di debug e log.
- Bootloader di sistema ST via DFU (USB) per l'aggiornamento del firmware, oppure SWD.

Stack software: STM32CubeG0 HAL; per l'USB device TinyUSB o la libreria USB device di ST. Build con CMake e arm-none-eabi-gcc (già presenti sulla macchina di sviluppo).

VID/PID: VID 0x1209 (pid.codes) con PID di test (0x0001–0x0010) durante lo sviluppo; PID definitivo
da richiedere a pid.codes alla pubblicazione del repository.

### Porte downstream

Per ogni porta:

| Funzione | Componente | Note |
|---|---|---|
| Load switch VBUS | TPS2561A (½ chip) | R_ILIM 24,3 kΩ 1 % → 2,1–2,5 A per canale |
| Scarica VBUS | AO3400 + 100 Ω | pilotato dal MCU |
| Switch D+/D- | TS3USB30E DGS | OE dal MCU, S fisso |
| Misura corrente | INA3221 (3 canali) × 2 | shunt 20 mΩ sul lato 5V_SYS, prima del load switch |
| ESD dati | USBLC6-2SC6 | tra switch e connettore |
| TVS VBUS | SMAJ5.0A | |
| USB-C: CC1, CC2 | Rp **22 kΩ** a VBUS commutata (pubblicizza 1,5 A) | la VBUS commutata fa cadere anche CC quando la porta è spenta |
| LED | 2 (alimentata / fault) via 74HC595 | |
| Pulsante | tattile, toggle della porta | debounce in firmware |

Sul valore di Rp: con limite hardware a ~2,2 A si pubblicizza 1,5 A (22 kΩ); 3 A (10 kΩ) sarebbe
scorretto. Le schede di sviluppo in genere ignorano CC.

### Relè ausiliari

Specifica contatti: **max 16 V, max 3 A**. 2 relè **SPDT** con bobina 5 V (es. Omron G5Q-1,
Songle SRD-05VDC-SL-C: 10 A nominali, ampio margine) pilotati da ULN2003 (diodi di ricircolo
integrati). Su pannello, per ogni canale, **due boccole a banana da 4 mm su COM e NC**: la linea
è **normalmente collegata** e la bobina si eccita solo per staccarla. Così lo stacco dura il tempo
di un impulso, e se l'hub perde alimentazione o si blocca la linea torna collegata (comportamento
sicuro per una scheda in prova). Il contatto NO è portato su un **ponticello a saldare** interno:
spostandolo la seconda boccola diventa NO per un eventuale uso "normalmente aperto".
Nel protocollo i comandi parlano di linea ("collegata / staccata", "stacca per N ms"), non di
bobina. LED per canale, acceso quando la linea è staccata.

### Upstream

Connettore **USB-C** con Rd 5,1 kΩ su CC1 e CC2 (dispositivo), solo D+/D-. In alternativa USB-B,
più robusto meccanicamente. Vedi domande aperte.

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
ripiego se la rifusione dell'hub dà problemi. La D è il ripiego se non si vuole toccare i QFN: il FE2.1 è poco documentato. La A costa ~30 € in più per ordine ma consente il TPS2561A (più margine sui 2 A)
senza pensieri.

## Meccanica

- Scatola **stampata in 3D su misura**, disegnata sulle misure reali del PCB a layout finito.
- Tutti i connettori saldati sul PCB e affacciati alle aperture della scatola; boccole a banana
  in versione da PCB. Nessun cablaggio interno a filo.
- Connettori sui due lati corti: lato "banco" le 5 porte downstream, i pulsanti con i LED e le
  4 boccole con i LED dei relè; lato posteriore upstream e alimentazione.
- PCB fissato alla scatola con viti su boccole filettate o inserti; i pulsanti tattili con
  estensori stampati o pulsanti ad asta lunga.

## Rischi / punti aperti tecnici

- Integrità del segnale Hi-Speed attraverso lo switch analogico: tracce corte, 90 Ω differenziali,
  nessun via inutile, ESD subito al connettore.
- Inrush sulla 5V_SYS con 5 porte accese insieme: il firmware accende le porte sfalsate di qualche
  decina di ms.
- Alimentatori USB-C che pubblicizzano 3 A ma non li reggono: il budget è impostabile a mano via
  HID e il firmware reagisce al calo di 5V_SYS.
- TPS2561A: due canali a 2,3 A nello stesso VSON-10 (RθJA 47,8 °C/W): pad termico su piano di
  rame con via verso il piano di massa.
