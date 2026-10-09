# Architettura

Documento di lavoro: raccoglie le scelte fatte, le motivazioni e i componenti candidati.
Le voci marcate **[da verificare]** vanno confermate sui datasheet prima di disegnare lo schema.

## Schema a blocchi

```
USB-C PWR IN ──► PD sink ──► 20 V ──► Buck 5 V / 10 A ──► 5V_SYS ─────────────────────────┐
(alimentatore)   (STUSB4500)          (LM61495 o simile)     │                             │
                     │ I2C (stato PD)                        ▼ LDO 3.3 V                   │
                     │                                   3V3 (hub, MCU)                    │
                     │                                       ▲                             │
USB-C UPSTREAM ──► ORing ideal-diode ─────────────────────────┘ (RF-16)                    │
(verso il PC)   │                                                                          │
                └──► HUB USB 2.0, 7 porte (USB2517)                                        │
                         ├─ DS1..DS4 ─► switch dati ─► USB-C 1..4 ◄─ load switch 2.5 A ◄───┤
                         ├─ DS5 ─────► switch dati ─► USB-A      ◄─ load switch 2.5 A ◄───┤
                         ├─ DS6 ─────► MCU di controllo (RP2040) — porta interna, mai sezionata
                         └─ DS7 ─────  libera (header interno / espansione futura)
                                             │
                                             ├─ EN / FAULT di ogni load switch
                                             ├─ OE di ogni switch dati
                                             ├─ monitor corrente per porta (INA3221 ×2, I2C)
                                             ├─ stato PD (STUSB4500, I2C) + ADC su 20 V / 5 V
                                             ├─ driver 2 relè ──► boccole a banana (isolate)
                                             └─ LED di stato, pulsanti opzionali per porta
```

## Scelte progettuali

### USB 2.0, non USB 3

Scelta: **USB 2.0 Hi-Speed** per la v1. Motivi:

- Lo scopo è il test di dispositivi a microcontrollore, che sono Full-Speed o Hi-Speed: il
  SuperSpeed non porterebbe nulla.
- Un hub USB 3 a 7 porte (es. Microchip USB5807) richiede per ogni porta due coppie differenziali
  aggiuntive a 5 Gbit/s con impedenza controllata, condensatori di accoppiamento e, sui connettori
  USB-C, un **controller CC + mux SuperSpeed per porta** (es. TI HD3SS3220) per gestire il
  ribaltamento del cavo. Serve inoltre un secondo switch SuperSpeed per porta per il sezionamento.
  Si passa da ~1 chip a ~3-4 chip per porta e il costo della sola parte hub sale di 4-5 volte.
- Su USB 2.0 i connettori USB-C non hanno il problema dell'orientamento: le coppie D+/D- dei
  due lati sono in parallelo nel connettore e basta una resistenza di pull-up Rp su CC1 e CC2.

Il PCB sarà comunque a **4 strati** (costa poco, semplifica il routing delle coppie differenziali
Hi-Speed e dà piani solidi per i 10 A a 5 V). Un'eventuale variante USB 3 resta un progetto
separato: non si cerca di renderla "predisposta".

### Cosa si seziona: VBUS e D+/D-, non GND

L'idea iniziale era un relè su VBUS e GND. Si propone invece di sezionare **VBUS e le linee dati
D+/D-** e di lasciare GND sempre collegata:

- Togliere GND lasciando D+/D- collegate fa sì che il dispositivo cerchi il ritorno di corrente
  attraverso le linee dati e i diodi di protezione dell'hub o del dispositivo: è il classico modo
  per danneggiare le porte.
- Togliere solo VBUS non basta per i dispositivi **auto-alimentati** (es. una scheda con la propria
  alimentazione): continuano a tenere il pull-up su D+ e l'host non li vede mai scollegati.
  Sezionando anche D+/D- lo scollegamento è garantito in ogni caso (RF-2).
- Sezionare dati e alimentazione separatamente dà in più il requisito RF-3.

### Relè o stato solido per le porte

Per le porte si propongono componenti a **stato solido**:

- **VBUS**: load switch con limite di corrente regolabile (vedi componenti). Dà gratis limitazione
  di corrente, soft-start (evita il crollo del 5 V all'inserzione di grossi condensatori), flag di
  fault e scarica dell'uscita quando spento (importante: senza scarica, il dispositivo può restare
  "acceso" per secondi sulla sua capacità e il reset non avviene).
- **D+/D-**: switch analogico per USB Hi-Speed (banda ≥ 1 GHz, es. TS3USB221) con ingresso OE:
  a OE disattivo le linee sono in alta impedenza, l'hub vede il distacco.

I relè restano per le due linee ausiliarie su banana, dove servono isolamento e contatti "puliti"
per tensioni e correnti arbitrarie.

Sequenza di stacco consigliata (default nel firmware): prima i dati, poi VBUS; riattacco: prima
VBUS, poi i dati dopo un breve ritardo (il dispositivo si enumera già alimentato stabilmente).

### Alimentazione

Bilancio di potenza (peggior caso: tutte le porte al massimo):

| Voce | Valore |
|---|---|
| 5 porte × 2,0 A @ 5 V | 50 W |
| Hub + MCU + logica | ~1 W |
| Efficienza buck 20 V → 5 V (~92 %) | ~55 W in ingresso |
| Corrente a 20 V | ~2,8 A |

Un alimentatore USB-PD da **65 W** (20 V / 3,25 A) copre il caso peggiore. Con un alimentatore più
piccolo il MCU legge il PDO negoziato e riduce il budget (RF-15). L'ingresso da 5 V puro (15 W) non
basta: in quel caso il firmware consente al massimo una porta con limite ridotto, oppure nulla.

Catena: **STUSB4500** (sink PD standalone, profili in NVM: 20 V, 15 V, 9 V, 5 V; stato via I2C)
→ **buck sincrono 5 V / ≥ 10 A** (candidati: TI LM61495, 3,8–36 V in, 10 A; in alternativa due
LM61460 da 6 A su due gruppi di porte) → 5V_SYS → LDO 3,3 V per hub e MCU.

Alternativa più economica al STUSB4500: **CH224K** (selezione della tensione a resistenza, nessun
I2C); il MCU in tal caso ricava la potenza disponibile misurando la tensione del bus con un
partitore sull'ADC.

Per RF-16 (hub raggiungibile senza alimentatore): la 3V3 di hub e MCU è alimentata da un ORing a
diodo ideale tra 5V_SYS e la VBUS upstream; le porte downstream si attivano solo se 5V_SYS è
presente. Attenzione a non iniettare mai 5V_SYS verso la VBUS dell'host.

### Hub

**Microchip USB2517 / USB2517i** (7 porte, QFN64, quarzo 24 MHz, configurazione via resistenze o
SMBus, regolatore 1,8 V interno). 7 porte coprono 4 USB-C + 1 USB-A + 1 porta interna per il MCU
e ne lasciano 1 di scorta (header interno).

Alternative:
- 2 × **USB2514B** in cascata (4+3 porte utili): chip più piccoli e comuni, ma due quarzi e un hop
  in più per i dispositivi sul secondo hub.
- **FE2.1** (Terminus, 7 porte): economico e disponibile su LCSC, documentazione scarsa.

I pin PRTPWR/OCS del hub non pilotano i load switch: l'abilitazione è decisa dal MCU. I flag di
fault dei load switch arrivano al MCU, che li riporta via HID. **[da verificare]** se collegare gli
OCS anche al hub per far arrivare all'host la notifica standard di sovracorrente.

### MCU di controllo

**RP2040** (TinyUSB, pico-sdk, cmake già presente sulla macchina di sviluppo) + flash QSPI
2 MB + quarzo 12 MHz. Collegato alla porta DS6 del hub, mai sezionata.

Interfacce USB esposte:
- **HID** con usage page vendor: il canale di controllo, senza driver.
- **CDC** (opzionale): console di debug e log.
- Bootloader UF2 nativo per l'aggiornamento del firmware (comando HID "entra in bootloader").

Alternativa: **STM32G0B1** (USB device + UCPD): potrebbe negoziare il PD da solo eliminando lo
STUSB4500, ma al prezzo di portarsi in casa lo stack PD. Non vale la pena per la v1.

VID/PID: da decidere (pid.codes per progetti open source, oppure VID Raspberry Pi 0x2E8A con PID
assegnato per RP2040). Vedi domande aperte.

### Porte downstream

Per ogni porta:

| Funzione | Componente candidato | Note |
|---|---|---|
| Load switch VBUS | TI **TPS2561** (doppio canale, limite regolabile) o Diodes **AP22814** (3 A) | limite impostato a ~2,5 A **[da verificare]** intervallo e scarica dell'uscita |
| Switch D+/D- | TI **TS3USB221** o Nexperia **NX3DV221** | 2:1 mux Hi-Speed con OE; si usa solo come on/off |
| Misura corrente | TI **INA3221** (3 canali, I2C) × 2 | shunt 10–20 mΩ sul lato 5V_SYS, prima del load switch |
| ESD dati | **USBLC6-2SC6** o equivalente | |
| TVS VBUS | 5 V, es. SMAJ5.0A / PESD5V0 | |
| USB-C: CC1, CC2 | Rp **22 kΩ** a VBUS commutata (pubblicizza 1,5 A) | la VBUS commutata fa cadere anche CC quando la porta è spenta |
| LED | 1 bicolore o 2 LED per porta | alimentata / fault |

Sul valore di Rp: con limite hardware a ~2,5 A si può pubblicizzare 1,5 A (22 kΩ) o 3 A (10 kΩ).
Le schede di sviluppo in genere ignorano CC; vedi domande aperte.

### Relè ausiliari

2 relè **SPDT** (es. Omron G5LE-1 o G2RL-1, 10 A / 250 VAC, bobina 5 V) pilotati da transistor con
diodo di ricircolo (o ULN2003). Su pannello, per ogni canale: **COM, NO, NC** su boccole a banana
da 4 mm (6 boccole). Stato all'accensione configurabile da firmware (RF-9); il contatto NC permette
di avere una linea "normalmente collegata" senza alimentazione della bobina.

Taglia dei relè e tensioni ammesse: vedi domande aperte.

### Upstream

Connettore **USB-C** con Rd 5,1 kΩ su CC1 e CC2 (dispositivo), solo D+/D-. In alternativa USB-B,
più robusto meccanicamente. Vedi domande aperte.

## Meccanica e PCB

- PCB 4 strati (JLCPCB o simile) con assemblaggio SMT per QFN e passivi 0402/0603.
- Connettori su due fronti: lato "banco" le 5 porte downstream e le boccole; lato posteriore
  upstream e alimentazione.
- Scatola: da definire (stampata 3D o profilo in alluminio).

## Rischi / punti aperti tecnici

- Integrità del segnale Hi-Speed attraverso lo switch analogico: tenere corte le tracce, 90 Ω
  differenziali, nessun via inutile.
- Eye diagram del hub con dispositivi marginali: prevedere i test di compliance "a occhio" con un
  dispositivo Hi-Speed noto (es. una chiavetta).
- Inrush sulla 5V_SYS con 5 porte accese insieme: il firmware accende le porte sfalsate di qualche
  decina di ms.
- Il buck da 10 A va posizionato lontano dalle coppie differenziali e dal quarzo del hub.
