# Regole per la progettazione PCB

**Ambito:** regole del PCB v0.3, 100 × 100 mm (D84), allineato allo schema v0.3 (D79–D83).
Il piazzamento iniziale è fatto; sbroglio, zone e serigrafia sono da completare.

Devhub usa un PCB a quattro strati da 1,6 mm con stackup **JLC04161H-7628**, lo stesso
specificato per prog-board-native (D44). Le porte esterne sono USB 2.0 Hi-Speed a 480 Mbit/s;
la porta interna del MCU è Full-Speed. Alimentazione e contatti relè
richiedono regole distinte dalle coppie USB.

Lo stackup è deciso. Le dimensioni indicate come **valori iniziali** sono la base proposta
per lo sbroglio: vanno verificate sul layout e sul processo produttivo prima del rilascio.
Le regole esprimibili in KiCad sono configurate nel progetto (vedi
[Configurazione nel progetto KiCad](#configurazione-nel-progetto-kicad)); le altre restano
verifiche di layout da fare a mano.

Riferimenti di progetto: [architettura](architettura.md), [componenti](componenti.md),
[decisioni](domande-aperte.md), [pinout MCU](pinout-mcu.md) e
[vincoli meccanici del PCB](../hardware/README.md#pcb).
Tutte le dimensioni seguenti sono in mm, salvo indicazione diversa.

## Stackup fisico

Stackup simmetrico, FR4, spessore finito nominale 1,6 mm, rame esterno 1 oz e interno
0,5 oz. Valori della tabella del produttore per **JLC04161H-7628**, senza suffissi:

| Strato | Materiale | Spessore | εr |
|---|---|---:|---:|
| F.Cu, L1 | Rame esterno | 0,035 | — |
| Dielettrico 1 | Prepreg 7628 | 0,2104 | 4,4 |
| In1.Cu, L2 | Rame interno | 0,0152 | — |
| Dielettrico 2 | Core FR4 | 1,065 | 4,6 |
| In2.Cu, L3 | Rame interno | 0,0152 | — |
| Dielettrico 3 | Prepreg 7628 | 0,2104 | 4,4 |
| B.Cu, L4 | Rame esterno | 0,035 | — |

La somma della tabella è 1,5862 mm; non correggere il core per farla diventare esattamente
1,6 mm. Sono valori nominali di costruzione; spessore finito e tolleranza vanno confermati
nell'ordine. Per il modello d'impedenza JLC indica solder mask con εr 3,8, spessore sopra
la pista 0,6 mil (0,01524 mm) e sopra il substrato/fra piste 1,2 mil (0,03048 mm).
Una sola quota uniforme della mask in KiCad non rappresenta tutto questo modello.
Fonte: [stackup e parametri JLCPCB](https://jlcpcb.com/impedance).

La fonte locale della scelta è
`../../autopilot-remote-proto/hardware/prog-board/pcb_design_rules.md`, sezione USB Full-Speed.
Il file `prog-board-native.kicad_pcb` di quel progetto contiene invece un modello diverso:
rame 0,035 su tutti i layer, dielettrici 0,10 / 1,24 / 0,10, εr 4,5 e mask 0,01 per lato.
**Non usare quel modello per il calcolo delle piste di devhub.**

## Funzione dei layer e ritorni di corrente

Distribuzione proposta per rispettare D42, con elettronica e connettori sulla faccia inferiore:

| Layer | Uso previsto |
|---|---|
| F.Cu | LED, pulsanti, segnali lenti, distribuzione di potenza a zone; USB solo se necessario |
| In1.Cu | Piano +3V3 (`3V3_In1`) su tutta la scheda, con isole VBUS delle porte sotto i connettori |
| In2.Cu | Piano GND continuo (`GND_In2`) su tutta la scheda; riferimento delle coppie USB su B.Cu |
| B.Cu | Elettronica principale, coppie USB, collegamenti locali e zone di alimentazione |

Le coppie USB stanno su B.Cu, con riferimento il piano GND di In2.Cu. F.Cu ha come piano
adiacente il +3V3 di In1.Cu: non instradarci USB. Distribuire la +5V sulle facce esterne; non
ricavare altre isole di potenza nei piani interni sotto USB e quarzo.
Le aree dei contatti isolati dei relè fanno eccezione: lì si applicano i keepout descritti sotto.

Collegare il piano GND alle masse delle facce esterne con via vicino ai GND dei componenti e dei connettori, ai condensatori e
ai cambi di layer dei segnali. Come punto iniziale, stitching lungo il bordo ogni 5–10 mm,
più fitto presso connettori e protezioni; la continuità locale del ritorno conta più del passo.
Rimuovere isole di rame scollegate e verificare strozzature dovute a file di fori o antipad.
Per non ferire i piani interni: zone interne (`GND_In2`, +3V3 su In1) con clearance 0,2 mm,
come la clearance ordinaria; via con "rimuovi strati non collegati, tranne inizio e fine", così
sugli strati interni dove la via non si collega non resta l'anello di rame e il foro nel piano è
di circa 0,8 mm (foro 0,3 più 0,25 per lato) invece di 1,2. Via di reti diverse a passo di
almeno 1,05 mm lasciano una striscia di piano fra i fori; più vicine i fori si fondono.
Non separare una massa analogica: tenere lontani i percorsi di potenza dalle zone di misura
mediante il piazzamento, conservando GND comune.

## Dimensioni iniziali e regole KiCad

Valori iniziali di progetto, da confrontare con le capacità del fabbricante e i footprint:

| Parametro | Valore iniziale | Applicazione |
|---|---:|---|
| Pista segnali ordinari | 0,25 | GPIO, I2C, UART, enable, fault, pulsanti |
| Pista minima locale | 0,15 | Solo uscita dai pad a passo fine, per il tratto più corto possibile |
| Clearance ordinaria | 0,20 | Fra reti diverse, salvo regole più restrittive |
| Clearance locale minima | 0,15 | USB e fanout fine; non estendere automaticamente a tutta la scheda |
| Via passante ordinaria | pad 0,60 / foro 0,30 | Corona nominale 0,15 |
| Via termica | pad 0,60 / foro 0,30 | Numero e geometria secondo land pattern e dissipazione; U2 secondo D50 |
| Rame rispetto a Edge.Cuts | ≥ 0,50 | Anche per le zone; eccezioni dei connettori da verificare esplicitamente |
| Distanza fra bordi dei fori | ≥ 0,25 | Da confermare con il processo di foratura |
| Serigrafia | testo ≥ 1,0; tratto ≥ 0,15 | Fuori dalle aperture mask e dalle aree coperte |

Usare via passanti; non sono previsti microvia, via ciechi o interrati. Evitare via aperti
nei pad da saldare: sotto QFN/VSON verificare l'assorbimento della pasta e il processo di
assemblaggio. Tenting e riempimento delle via non sono equivalenti.

Netclass da predisporre:

| Classe | Pista iniziale | Clearance | Reti o funzione |
|---|---:|---:|---|
| DEFAULT | 0,25 | 0,20 | Segnali digitali lenti |
| USB_DIFF | 0,2474 | 0,15 | Tutti i segmenti D+/D−; gap coppia 0,1519 |
| SENSE | 0,20 | 0,20 | Bias del hub (RBIAS) |
| PWR_3V3 | 0,60 | 0,20 | +3V3 e alimentazione locale della logica; preferire zone |
| PWR_PORT | 1,50 | 0,20 | VBUS_P1…P4 e relativi tratti locali |
| PWR_MAIN | 2,00 | 0,20 | VBUS_IN, +5V; preferire zone ampie |
| RELAY_CONTACT | 2,00 | 1,00 | COM, NC, NO, ponticelli e collegamenti alle boccole |

Le larghezze di potenza si riferiscono al rame esterno da 35 µm e non certificano una
portata. I restringimenti sui pad richiedono controllo locale; le clearance di footprint
particolari richiedono regole circoscritte, non l'abbassamento dei minimi globali.

I nomi locali KiCad includono il percorso gerarchico. Assegnare le classi usando i nomi
completi effettivi dopo l'aggiornamento dallo schema; `5V_SYS` nell'architettura corrisponde
alla rete **+5V** nello schema. Le coppie USB si chiamano `…DP` / `…DN` (D49), con eventuale
numero finale: KiCad le riconosce come coppie differenziali per il router e per i controlli
di gap e skew. Il nome `…DM` non verrebbe riconosciuto e il suffisso va mantenuto anche
in nuove reti USB.

### Configurazione nel progetto KiCad

**Stackup** in `devhub.kicad_pcb`: JLC04161H-7628 come nella tabella, solder mask 0,01524 con
εr 3,8 (quota sopra la pista), finitura non scelta e nessun vincolo d'impedenza sui dielettrici.
KiCad calcola lo spessore includendo la mask: 1,6167.

**Vincoli e netclass** in `devhub.kicad_pro`:

- minimi globali: pista 0,15, clearance 0,15, via 0,60/0,30 con corona 0,15, foro 0,30,
  fori a 0,25 fra loro e dal rame, rame dal bordo 0,50, testo 1,0 × 0,15;
- larghezze predefinite 0,15 / 0,20 / 0,25 / 0,60 / 1,50 / 2,00, via 0,60/0,30, coppia
  0,2474 / 0,1519;
- netclass della tabella, assegnate per nome di rete:

| Classe | Reti |
|---|---|
| USB_DIFF | `*CONN_DP`, `*CONN_DN` (porte e upstream), `HUB_DP?`, `HUB_DN?`, `USB_MCU_DP/DN`, `USBUP_DP/DN` |
| RELAY_CONTACT | `BAN?_COM`, `BAN?_2`, `K?_NC`, `K?_NO`, `SNUB?` |
| PWR_MAIN | `VBUS_IN`, `+5V` |
| PWR_PORT | `VBUS_P?` |
| PWR_3V3 | `+3V3`, `5V_LOGIC` |
| SENSE | `RBIAS` |

**Regole personalizzate** in `devhub.kicad_dru`:

- solo via passanti;
- USB: pista minima 0,15 (fanout), ottimale 0,2474; gap 0,1519; differenza di lunghezza
  ≤ 0,5 dentro ciascuna coppia, senza confronti fra porte diverse, e ≤ 3 sui tratti fra i
  connettori USB-C e il primo componente, dove la somma comprende il ramo morto dei pin doppi; ≥ 0,75 da piste e zone
  di altre reti, comprese le altre coppie USB (propria coppia, pad e via esclusi, così gap,
  fanout e via GND di ritorno restano liberi);
- larghezze iniziali di PWR_MAIN, PWR_PORT, PWR_3V3 e SENSE come **avvisi**: ogni
  restringimento compare nel DRC e va giustificato localmente;
- contatti relè a ≥ 1,0 dalle altre reti; fra le reti del contatto 0,20, secondo il land
  pattern;
- eccezioni circoscritte alle impronte: 0,15 fra i pad di U13 (passo 0,45) e 0,19 fra pad GND
  e fori di centraggio dei GCT USB4105;
- fanout del USB2517: un'unica area di regola `U2_FANOUT`, quadrato di 21 mm centrato su U2,
  che comprende disaccoppiamenti e RBIAS con le loro piste. Fra tratti USB dentro l'area la distanza minima è 0,15
  invece di 0,75, perché le coppie adiacenti escono da pad a 1 mm di passo. Per il MCU
  vale lo stesso con l'area `U3_FANOUT`, quadrato di 19 mm centrato su U3 (USB_MCU su PA11/PA12);
- piste vicine al USB2517 e al MCU: fra le piste USB e le piste di altre reti che toccano
  `U2_FANOUT` o `U3_FANOUT` la distanza minima è 0,2 invece di 0,75: a
  passo 0,5 i pad di alimentazione, quarzo, PLL e CFG_SEL2 sono accanto a quelli delle coppie
  (D58);
- uscita dei pin CC dai connettori USB-C: fra le piste USB e le piste CC (`CC1`, `CC2`, `CC1_UP`,
  `CC2_UP`) dentro le aree di regola `USBC_FANOUT` la distanza minima è 0,2 invece di 0,75. Il pad
  A5 (CC1) è a 0,5 mm dal pad B7 della coppia, il cui ramo scende parallelo per circa 1,5 mm; le
  aree coprono la fila di pad e i primi 3 mm verso l'interno di J2 e J101-J401;
- switch dati PI3USB4002A (U101-U401, passo 0,5): GND, REF, EN e VDD sono a 0,5 mm dai pad delle
  coppie; dentro le aree di regola `SW_FANOUT` (4 × 4 mm centrate su ogni switch) le piste di
  altre reti possono stare a 0,2 dalle coppie invece di 0,75.

**Zone e aree di regola** nel PCB: piano GND `GND_In2` su tutto In2.Cu; area
`RELAY1_CONTACTS` senza riempimenti su tutti i layer, sotto contatti del relè, ponticello,
snubber e boccole; aree `U2_FANOUT`, `U3_FANOUT`, `USBC_FANOUT` e `SW_FANOUT` usate solo
dalle regole sopra, spostate insieme al proprio componente nel piazzamento v0.3: vanno
ricontrollate dopo la rifinitura. Piano +3V3 `3V3_In1` su tutto In1.Cu (priorità 2), con le
isole VBUS delle porte sotto i connettori a priorità più alta; piccole zone VBUS delle porte su
B.Cu, zona +3V3 sul pin 24 di U2 e `U1_HEATSINK`. Le zone di potenza della +5V sono da
disegnare con lo sbroglio.

Nelle condizioni dei file `.kicad_dru` di KiCad 10 i caratteri jolly si usano con `==`;
`matches` e `Parent.Reference` non funzionano e un errore fa scartare l'intero file senza
messaggi. Dopo ogni modifica, controllare con il DRC che le regole compaiano nei messaggi.

Le regole coprono solo ciò che il DRC può misurare: il resto è nella sezione
[Verifiche manuali](#verifiche-manuali).

**Avvisi aperti nel DRC**: serigrafia dei connettori a filo del bordo tagliata dal bordo;
testi di serigrafia sovrapposti fra loro e ai pad dopo il piazzamento; zone non riempite.
Da sistemare con lo sbroglio e la serigrafia definitiva.

## Coppie USB

La geometria USB è confermata dal
[calcolatore JLCPCB](https://jlcpcb.com/pcb-impedance-calculator), con i seguenti parametri:

| Parametro | Valore |
|---|---|
| Tipo PCB | Rigid, 4 layer, spessore nominale 1,6 mm |
| Rame esterno / interno | 1 oz / 0,5 oz |
| Stackup selezionato | **JLC04161H-7628 Standard**, spessore finito indicato 1,59 mm ±10 % |
| Unità | mm |
| Tipo di linea | **Differential Pair (Non coplanar)**, con solder mask |
| Signal Layer | **L4, B.Cu** |
| Top Ref / Bottom Ref | **L3, In2.Cu / nessuno** |
| Impedenza richiesta | **90 Ω differenziali** |
| Trace Spacing impostato | **0,1519 mm**, gap bordo-bordo |
| Trace Width calcolato | **0,2474 mm per pista** |
| Impedance Tolerance del calcolatore | ±0,5 %, parametro del calcolo; non tolleranza produttiva |

Il risultato conferma la geometria riportata per prog-board-native. Si applica al percorso
**B.Cu rispetto al piano GND In2.Cu**, con coppia non coplanare e solder mask; la stessa geometria varrebbe su F.Cu
rispetto a In1.Cu, che però porta il piano +3V3: le coppie restano su B.Cu.
Se cambiano stackup, rame, mask o geometria del rame laterale, ripetere il calcolo.

Il valore è un risultato di calcolo, da verificare sul prodotto finito. Come obiettivo iniziale
richiedere 90 Ω ±10 % al fabbricante; il servizio di controllo d'impedenza resta da definire
nell'ordine. Con `Impedance Control: No` il valore resta un obiettivo progettuale, senza
garanzia di fabbricazione. La tolleranza dello spessore finito e quella del calcolatore non
sostituiscono la tolleranza d'impedenza concordata con il produttore.

| Percorso | Coppie da includere |
|---|---|
| Upstream J2 → ESD → hub | UP_CONN_DP / UP_CONN_DN e USBUP_DP / USBUP_DN nel foglio hub |
| Hub → switch dati porte 1–4 | HUB_DP1…4 / HUB_DN1…4 |
| Switch dati → connettore | CONN_DP / CONN_DN in ogni istanza dei fogli porta |
| Porta interna 6 → MCU | USB_MCU_DP / USB_MCU_DN |

Regole di routing:

- Percorso diretto, stessa faccia per entrambi i conduttori e piano continuo sottostante.
  Niente attraversamenti di tagli, vuoti, bordi del piano o aree dei contatti relè.
- Niente test point a derivazione, tee o tratti morti. Unire i pin duplicati USB-C D+ fra loro
  e D− fra loro immediatamente al connettore; mantenere minime le derivazioni inevitabili.
- Preferire zero cambi di layer. Se necessari, stesso numero e geometria di via sui due
  conduttori e via GND vicine per collegare i piani di riferimento.
- Evitare curve brusche e variazioni ripetute di larghezza; adattare simmetricamente il fanout
  ai pad e ripristinare subito larghezza e gap nominali.
- Tenere le protezioni nel percorso, senza bypass in rame. L'upstream attraversa USBLC6;
  sulle quattro porte la protezione è nel PI3USB4002A, secondo D27.

Questi criteri seguono le [linee guida TI per interfacce veloci](https://www.ti.com/lit/an/spraar7j/spraar7j.pdf).

Obiettivi geometrici iniziali per devhub: differenza D+/D− **≤ 0,5 mm** sul percorso completo
fra endpoint, sommando i segmenti attraverso ESD o switch (eccetto l'incrocio dei pin doppi
all'uscita dei connettori USB-C, vedi sotto); distanza da altre piste e da rame
laterale **≥ 0,75 mm**, fuori dai fanout. Quest'ultima riduce l'effetto coplanare, ma va comunque
inclusa nel calcolo d'impedenza. Mantenere queste distanze anche dalle altre coppie.
Non pareggiare le lunghezze fra porte diverse e non introdurre serpentine inutili.
La lunghezza totale va minimizzata con il piazzamento e registrata per ogni porta; non viene
fissato un massimo arbitrario che sostituisca la verifica dell'intero canale a 480 Mbit/s.
Non aggiungere resistenze serie, choke o condensatori non presenti nello schema.

Realizzazione nel PCB, tutte le coppie su B.Cu sopra il piano GND di In2.Cu:

- USB-C (porte e upstream): una coppia di pad adiacenti esce verso l'interno della scheda; il
  secondo pad della stessa rete si collega con un anello sotto il corpo del connettore, attorno
  all'estremità del pad centrale, e l'altro pin doppio raggiunge la propria pista passando
  sopra il pad lasciato libero. Niente via, monconi corti, nessuna ansa di compensazione.
  In un verso di inserimento il segnale percorre l'anello, nell'altro no: la differenza D+/D−
  resta di circa 2,3 mm in un verso (circa 15 ps, meno dell'1 % del bit a 480 Mbit/s) e meno di
  1 mm nell'altro, ed è accettata. Un'ansa non la elimina in entrambi i versi senza allungare il
  ramo morto, e un incrocio simmetrico richiederebbe via sulle linee USB. Lo skew del DRC su
  queste reti somma anche il ramo morto e non rappresenta nessuno dei due versi: per
  `CONN_D?` delle porte USB-C e `UP_CONN_D?` vale un limite largo (regola "USB-C uscita dal
  connettore"); l'accordo stretto resta sul resto del percorso.
- Lato connettore: PI3USB4002A a 12 mm dal connettore, coppia diritta dai pad ai pin.
- Upstream: USBLC6 D20 in linea fra J2 e il hub, canali secondo D52.
- Porta 6 (MCU, Full-Speed): hub e MCU sono entrambi sulla faccia inferiore; se D+ e D−
  si incrociano, l'incrocio con due via su una sola linea (D52) è ammesso a 12 Mbit/s; il DRC
  conta anche la lunghezza delle via.
- Coppie parallele con la geometria nominale, senza compensazione di lunghezza: l'accordo
  D+/D− si fa con lo strumento di KiCad per le coppie differenziali.

## Alimentazione e caduta di tensione

Dimensionare la dorsale per **3 A continui**, verificando anche l'intervento del limite
d'ingresso da 3,5 A nominali e le sue tolleranze. Per i collegamenti locali IN/OUT
dell'eFuse vale inoltre il criterio di almeno 6 A nella sezione dedicata al TPS259470A.
Per ogni porta considerare **2 A continui**
e il limite hardware fino a **2,9 A** (UCS2114, ILIM7); il budget totale non permette quattro
porte a 2 A simultaneamente. Contatti relè: **3 A**.

Preferire zone esterne ampie e percorsi corti. Collegare tutti i pad VBUS e GND dei connettori
USB-C senza concentrare la corrente in un solo collo di bottiglia. Il ritorno GND va verificato
insieme all'andata. Se una corrente di potenza deve cambiare faccia, partire da almeno quattro
via 0,60/0,30 in parallelo, distribuite sul rame largo; confermare il numero con metallizzazione,
corrente e riscaldamento effettivi. Una singola via ordinaria non va assunta idonea alla dorsale.

La verifica deve comprendere lunghezza, spessore del rame, restringimenti, via, thermal relief,
contatti e temperatura. Usare IPC-2152 o un modello termico adeguato per la portata; come
obiettivi iniziali di layout adottare **ΔT del rame ≤ 10 °C** e **caduta nel solo rame di andata
più ritorno ≤ 100 mV** sul percorso ingresso → porta peggiore, al carico di progetto.
La verifica della tensione alla porta comprende anche cavo, connettori, eFuse e switch.

Controllo resistivo a 20 °C: `R = ρ × L / (w × t)`, con rame `ρ ≈ 1,72×10⁻⁸ Ω·m`.
Per esempio, 100 mm di pista larga 2 mm e spessa 35 µm valgono circa 24,6 mΩ:
**74 mV a 3 A sulla sola pista di andata**, prima di via e ritorno. Anche su una scheda da
100 × 100 mm la sola larghezza nominale non basta: allargare le zone o accorciare i percorsi.

La catena è ingresso → TVS/eFuse → +5V → carichi (UCS2114 delle porte, LDO, bobina del relè);
per le porte UCS2114 → VBUS della porta. VBUS upstream è una rete separata dalla +5V di sistema.

## Piazzamento e dissipazione

Rispettare profilo **100 × 100 mm, R3**, fori M3 e posizioni bloccate del pannello in D84 e D87 (D43
per l'allineamento di LED e pulsanti). F.Cu ospita solo LED e pulsanti come componenti; tutto il
resto è su B.Cu.
Verificare ingombri 3D, corsa dei pulsanti, accesso dei cavi, boccole e utensili di montaggio.
Prevedere keepout su tutti i layer intorno a viti e distanziali, dimensionati sul loro ingombro
reale, non solo sul foro. Conservare i vincoli già presenti nel PCB.

Ordine di piazzamento consigliato:

1. Hub fra upstream e porte, orientato per ridurre incroci; PI3USB4002A nella zona della
   rispettiva porta, lato protetto rivolto al connettore e lato hub rivolto all'USB2517.
   La distanza dal connettore va verificata anche per ESD, come precisato sotto.
2. ESD upstream vicino a J2, TVS ed eFuse vicino a J1, con ritorni GND corti e larghi.
3. UCS2114 fra le due porte servite, con condensatori VS/VDD e resistenza COMM_ILIM presso i
   pin; uscite VBUS verso i connettori senza lunghi giri di potenza.
4. MCU vicino alla porta interna dell'hub.
5. LDO e Schottky con spazio per dissipare; relè e driver nella zona delle boccole.

Piazzare ogni integrato insieme ai componenti che ne vincolano il funzionamento: riservare
subito spazio per quarzo, disaccoppiamenti, REF, bias e regolazione dei limiti. Solo dopo
collocare i passivi meno critici ancora liberi; LED e pulsanti conservano le posizioni bloccate. Verificare già in questa fase il passaggio
delle coppie USB e delle zone di potenza: la vicinanza geometrica non basta se il percorso
reale deve aggirare altri componenti.

### Componenti di supporto e condensatori

Le indicazioni seguenti specificano il piazzamento dei componenti già presenti nello schema
(D47); non prevedono l'aggiunta di capacità o filtri.

| Gruppo | Componenti | Regola di piazzamento |
|---|---|---|
| Switch USB U101–U401 | C106/C206/C306/C406, 1 µF 25 V su REF | Accanto ai pin REF e GND dello switch, con anello REF → condensatore → GND corto; evitare giri attorno alle coppie dati |
| Alimentazione switch USB | C101/C201/C301/C401, 100 nF | Presso VDD del rispettivo switch, con ritorno GND locale; non sostituiti dal condensatore REF |
| eFuse U13 | C13, 100 nF; C7, 1 µF; R30–R33, C11 e C12 | C13 direttamente presso IN/GND e C7 presso OUT/GND; partitore, resistenza ILM e condensatori dV/dt e timer presso i rispettivi pin, lontani dai segnali commutati |
| Riserva sulla +5V | C3/C4, 220 µF elettrolitici SMD Ø 6,3 × 7,7 mm sulla faccia inferiore (D86); C5/C6, 10 µF | Nella zona di distribuzione della +5V verso i carichi; evitare un lungo ramo morto che ne aumenti l'induttanza. Non considerarli sostitutivi dei bypass presso gli integrati |
| Switch VBUS U4/U5 | C60/C63, 100 nF su VDD; C61/C62 e C64/C65, 1 µF sulle coppie VS; R60/R61 su COMM_ILIM; R62–R65 pull-up ALERT# | 100 nF presso VDD (pin 8), 1 µF presso ciascuna coppia VS (6/7 e 9/10) con ritorno GND corto; resistenza COMM_ILIM presso il pin 3; pin VS e VBUS duplicati uniti con rame largo; area termica collegata al pin 17 |
| Hub U2 | 100 nF 0402: C27 (pin 46), C25 (5), C38 (10), C28 (52), C26 (57), C30 (64), C29 (24); 1 µF: C31 condiviso dei VDDA33 (vicino al pin 57), C32 (24); 1 µF C33 su VDD18 (25), C35 su VDD18PLL (62) | Ogni condensatore presso il proprio pin, il 100 nF più vicino del 1 µF, collegato al pad con una pista sullo stesso lato (niente via fra pad e condensatore) e ritorno GND corto; per i pin 5, 10 e 57, fra coppie USB, vale l'area `U2_FANOUT` (D58) |
| Supervisore U12 | C37, 100 nF 0402 | Presso il pin VDD di U12, non del hub |
| MCU U3 | C41, 100 nF 0402; C40, 4,7 µF; C42, 100 nF 0402; C46, 100 nF 0402 su NRST | C41 il più vicino al pin VDD/VDDA (6), con ritorno su VSS (7) e sul pad esposto (49); C40 subito dietro; C42 per VREF+ (pin 5) accanto a C41. VBAT (pin 4) senza condensatore proprio (D57) |
| LDO U1 | C8 in ingresso; C9/C10 in uscita | C8 presso IN/GND, C9/C10 presso OUT/GND, con anelli corti; la vicinanza di D2 non deve togliere spazio a condensatori e rame di dissipazione |
| VBUS delle porte | C102…C402 da 22 µF e C103…C403 da 100 nF | Presso VBUS/GND del rispettivo connettore; collocare il ceramico piccolo con il percorso più corto. Non raggruppare tutte le capacità nella zona dei load switch |

REF e VDD seguono le indicazioni del
[datasheet PI3USB4002A, Application Information](https://www.diodes.com/datasheet/download/PI3USB4002A.pdf).
Lo stesso documento suggerisce una tratta più lunga fra connettore e switch per favorire
la risposta ESD, senza fissarne la lunghezza: non equiparare quindi lo switch a una TVS da
mettere obbligatoriamente a ridosso del connettore. La distanza va conciliata con l'integrità
USB Hi-Speed e verificata sul prototipo; non allungare artificialmente la coppia con serpentine
né aggiungere induttori non presenti nello schema.

Per gli UCS2114, riferimento:
[datasheet DS20005743B](https://ww1.microchip.com/downloads/aemDocuments/documents/APID/ProductDocuments/DataSheets/UCS2114-Data-Sheet-20005743B.pdf),
land pattern C04-2421.

### Layout specifico del TPS259470A

Prescrizioni del [datasheet TPS25947, SLVSFC9C, §8.4.1](https://www.ti.com/lit/gpn/tps25947):

- Bypass ceramico IN–GND ≥ 100 nF presso i pin, con area del loop minima.
- Percorsi di potenza corti, dimensionati per almeno due volte la corrente a pieno carico.
- GND locale analogico senza correnti di carico, collegato alla massa di sistema a stella.
- Dissipazione dai pin IN e OUT con rame sulle due facce e via termiche; distribuire
  uniformemente la corrente nei pad.
- Componenti di regolazione presso i pin, ritorni corti al GND dell'integrato; capacità
  parassita su ILM < 50 pF e assenza di accoppiamento con segnali commutati.
- Protezioni presso i pin protetti, collegamenti corti. Ceramico OUT–GND ≥ 1 µF presso OUT;
  Schottky di clamp per transitori negativi da carichi induttivi, quando applicabile.

**Applicazione a devhub.** C13 realizza il bypass d'ingresso; D1 e C13 vanno disposti in modo
che il percorso di protezione di U13 sia corto, non soltanto vicino a J1. Per il carico di
progetto da 3 A, il criterio TI richiede di verificare i collegamenti locali IN/OUT per
**almeno 6 A**: è un margine per rame, pad e via, non una nuova corrente erogabile né una
modifica del limite dell'eFuse. La larghezza iniziale da 2 mm e il numero iniziale di via
non dimostrano da soli il rispetto di questo criterio.

Le zone termiche di IN e OUT appartengono rispettivamente a VBUS_IN e +5V:
non collegarle a GND né unirle tra loro. Conservare la continuità dei piani sotto le USB;
il ritorno analogico locale va organizzato nella zona di U13. R33 sul pin ILM non deve avere
stub, test point o capacità aggiunte; verificare il limite dei 50 pF considerando piste,
pad e via, non interpretarlo come un condensatore da montare. R30–R32, C11 e C12 restano
presso i rispettivi pin di regolazione.

C7 (1 µF, D48) realizza il bypass d'uscita: va a ridosso dei pin OUT e GND di U13, con loop
minimo e via GND accanto al pad; C3–C6 non lo sostituiscono. Non è previsto lo Schottky di
clamp su OUT (D48): la +5V resta sulla scheda e alimenta solo carichi locali; per questo è
importante tenere corto e largo il collegamento OUT → C3/C4, così che i condensatori di riserva
limitino anche i transitori negativi all'apertura dell'eFuse.

### Separazione termica

La scarica delle porte è interna agli UCS2114 (D75): non ci sono componenti di scarica da
piazzare. Evitare di concentrare LDO, Schottky, eFuse e UCS2114 nella stessa zona, senza
allungare inutilmente i percorsi di potenza. Non fissare una distanza termica universale:
usare area di rame, potenza e temperatura nel case per verificare la separazione. Anche i
condensatori elettrolitici vanno tenuti lontani dalle sorgenti di calore, compatibilmente con
la loro funzione elettrica.

### Disaccoppiamento e dissipazione degli integrati

Ogni condensatore di disaccoppiamento va presso il proprio pin, con collegamento corto al
piano GND; evitare un lungo tratto comune prima dei condensatori. Il quarzo 24 MHz e C22/C23
restano a ridosso dell'hub, lontani da potenza e commutazioni. RBIAS richiede un collegamento
locale al pin e un ritorno GND dedicato e corto. Non usare le uscite dei regolatori interni
dell'hub per alimentare altri carichi. Riferimento:
[checklist hardware USB2517](https://ww1.microchip.com/downloads/en/DeviceDoc/USB2517-Hardware-Design-Checklist-00004211.pdf).

Applicare i land pattern e i collegamenti dei pad esposti del produttore, compresi rame,
via termiche e apertura della pasta. Il pad esposto di U2 ha 16 via 0,60/0,30 a passo
1,35–1,40, aperte su entrambi i lati, con apertura della maschera 1:1 sul lato opposto (D50):
verificare nei Gerber che le 16 aperture restino e, sul prototipo, saldatura e temperatura
di U2. Per la produzione le via diventano "encroached" (punto 7 delle
[verifiche prima della produzione](#verifiche-prima-della-produzione)). Il tab dell'AP7361C-33E
(U1) è il pin 2, GND, e fa da dissipatore: l'LDO dissipa fino a 0,65–0,75 W nel caso peggiore
(D56, [architettura.md](architettura.md)). Zona GND `U1_HEATSINK` sul tab, sul lato del componente (B.Cu), collegamento pieno al
pad, ~1,4 cm² (lo spazio fra C8/C9, D2 e R1/R2), con 10 via 0,60/0,30 ai lati e sopra/sotto il
tab verso il piano GND In2.Cu: obiettivo θJA ≤ ~60 °C/W. Il rame non va interrotto da piste di
segnale; il pin 2 centrale va collegato al tab con una pista corta. Verificare nel case chiuso il caso peggiore: LDO, Schottky,
eFuse, UCS2114 e relè eccitati; sul prototipo misurare la temperatura di U1 con le quattro porte
Hi-Speed attive.
Dimensionare i thermal relief dei pad di potenza per la corrente; verificare il compromesso
con la saldatura manuale. La dimensione del pad da sola non garantisce dissipazione sufficiente.

## Segnali sensibili

Per le CC d'ingresso, R1/R2 (Rd da 5,1 kΩ) restano nella zona del connettore J1, senza
collegamenti al MCU (D65).

Tenere corte le reti ILM, dV/dt e timer dell'eFuse e le resistenze COMM_ILIM degli UCS2114.
Collocare resistenze e condensatori presso i pin interessati. Non aggiungere filtri rispetto
allo schema senza verificarne l'effetto sulle temporizzazioni. Instradare i due bus I2C sopra
GND, lontano dai loop di potenza; verificare i fronti con la capacità reale del bus sul prototipo.

## Relè e isolamento dei contatti

I contatti sono circuiti separati da GND e dalle alimentazioni USB.
Sono previsti **16 V / 3 A massimi**, non l'uso su rete elettrica.

Come regola iniziale di layout, mantenere **almeno 1 mm di separazione** fra rame dei contatti
e circuiti dell'hub, su tutti i layer. Escludere piani GND e
alimentazioni sotto la zona dei contatti e le piste verso le boccole. Questa distanza è una
scelta funzionale per la bassa tensione prevista, non una qualifica d'isolamento di sicurezza.
Non ridurre le distanze del footprint del relè rispetto al land pattern del produttore.

Tenere le piste COM/NC/NO larghe e corte fino ai ponticelli e alle boccole; controllare anche
la portata di ponticelli, pad e thermal relief. Gli snubber DNP restano sul lato contatti.
Bobine, ULN2003V12 e ritorni di ricircolo appartengono invece alla massa dell'hub: rendere
compatto il loop bobina → driver → clamp/COM → alimentazione, lontano da quarzo e coppie USB.
Non creare una separazione della massa logica per imitare l'isolamento dei contatti.

## Pulsanti e condensatori di filtro

I cinque condensatori da **100 nF** delle linee dei pulsanti vanno sulla faccia inferiore
**vicino ai rispettivi GPIO dell'MCU** (D46), con il tratto condensatore → pin il più corto
possibile e una via GND accanto al pad di massa. I pulsanti restano sulla faccia superiore,
nelle posizioni definite da D43 e D84.

La posizione presso l'ingresso permette di filtrare anche i disturbi raccolti lungo la pista
pulsante → MCU. Instradare queste linee sopra un piano GND continuo, lontano dai percorsi
delle bobine dei relè e dai loop di potenza. Per il rimbalzo meccanico la posizione del
condensatore sul PCB è meno determinante; il **debounce resta in firmware**, secondo D39.

## Assemblaggio e collaudo

- Verificare pin 1 e corrispondenza simbolo/pad per USB2517, MCU, TPS259470A e PI3USB4002A;
  controllare i footprint importati contro i disegni dei package, non solo contro il modello 3D.
- Definire stencil e aperture dei pad esposti; prevedere accesso per rilavorazione dei package
  piccoli senza urtare connettori e componenti alti. La finitura superficiale resta da scegliere
  in base a planarità e processo di assemblaggio.
- Identificare porte 1–4, PWR IN, UPSTREAM, COM e NC, pin 1 degli header, revisione PCB e polarità.
  Conservare le marcature leggibili anche a scheda montata, dove servono all'utente.
- Prevedere accesso a GND, +5V, +3V3, reset hub/MCU, SWD, I2C, FLT e VBUS di ogni porta.
  Preferire pad già accessibili; eventuali nuovi TP devono essere coerenti con lo schema.
  Non aggiungere TP a stub sulle coppie USB.
- Al primo avvio usare alimentazione limitata, verificare rail e reset, poi enumerazione hub
  e MCU. Provare ogni porta in Hi-Speed e Full-Speed, entrambi gli orientamenti USB-C,
  trasferimenti simultanei e cicli di stacco sotto carico.
- Misurare tensione alla porta peggiore, temperatura nel case, scarica
  VBUS e assenza di rialimentazione indesiderata; verificare i guasti previsti dall'architettura.
  Il solo successo dell'enumerazione non dimostra la qualità del canale USB Hi-Speed.

## Verifiche manuali

Controlli che il DRC non copre e che vanno fatti sul layout, prima del rilascio e dopo ogni
modifica significativa. Il dettaglio di ciascun punto è nelle sezioni indicate.

**Coppie USB** ([Coppie USB](#coppie-usb))

- Piano GND continuo sotto ogni tratto, senza attraversare tagli, vuoti, bordi del piano,
  antipad in fila o aree dei contatti relè; controllo visivo su In2.Cu.
- Differenza D+/D− ≤ 0,5 sul percorso completo fra endpoint, sommando i segmenti attraverso
  USBLC6 e PI3USB4002A: il DRC controlla solo ogni tratto separato.
- Cambi di layer: stesso numero di via sui due conduttori e via GND vicine.
- Nessuno stub, test point o tee; pin D+/D− duplicati dell'USB-C uniti subito al connettore.
- Distanza connettore–PI3USB4002A conciliata fra ESD e integrità del segnale.
- Lunghezza totale di ogni porta registrata; ripetere il calcolo d'impedenza se cambiano
  stackup, mask o rame laterale.

**Alimentazione** ([Alimentazione e caduta di tensione](#alimentazione-e-caduta-di-tensione))

- Caduta in andata e ritorno ≤ 100 mV e ΔT del rame ≤ 10 °C sul percorso ingresso → porta
  peggiore, con lunghezze, restringimenti, via e thermal relief reali.
- Almeno quattro via 0,60/0,30 a ogni cambio di faccia della dorsale; tutti i pad VBUS e GND
  dei connettori USB-C collegati.
- Ogni avviso di larghezza delle classi PWR_* e SENSE giustificato da un restringimento locale.
- Collegamenti locali IN/OUT di U13 dimensionati per 6 A.

**Segnali sensibili** ([Segnali sensibili](#segnali-sensibili))

- Pin ILM di U13 senza stub e con capacità parassita < 50 pF; reti dV/dt, timer, ILM e
  COMM_ILIM corte.

**Relè e meccanica** ([Relè e isolamento dei contatti](#relè-e-isolamento-dei-contatti),
[Piazzamento e dissipazione](#piazzamento-e-dissipazione))

- Aree di esclusione su tutti i layer sotto i contatti e le piste verso le boccole: il DRC
  controlla la distanza di 1 mm solo sullo stesso layer.
- Keepout attorno a viti e distanziali, dimensionati sul loro ingombro reale.
- Ingombri 3D, corsa dei pulsanti, accesso a cavi e boccole.

**Piani, piazzamento e dissipazione**

- Stitching fra i piani GND lungo il bordo e presso connettori, protezioni e cambi di layer;
  nessuna isola di rame scollegata o strozzatura dei piani.
- Disaccoppiamenti, quarzo, RBIAS, REF dei PI3USB4002A e bypass C7/C13 di U13 presso i pin
  ([Componenti di supporto e condensatori](#componenti-di-supporto-e-condensatori)).
- Separazione termica fra sorgenti di calore (LDO, Schottky, eFuse, UCS2114) e condensatori
  elettrolitici; caso peggiore nel case chiuso.
- Pad esposto di U2: nei Gerber della maschera del lato opposto le 16 aperture 1:1 delle via
  termiche; sul prototipo saldatura e temperatura di U2 con 7 porte Hi-Speed attive (D50).

## Verifiche prima della produzione

1. Aggiornamento PCB dallo schema e controllo di tutti i footprint, comprese le istanze
   gerarchiche delle porte; nessuna connessione mancante o modifica involontaria dei componenti bloccati.
2. Stackup inserito in KiCad e confermato nel preventivo; geometria USB verificata per lo
   stackup ordinato, con eventuale coupon e report di impedenza concordati con il fabbricante.
3. Zone riempite, DRC e parità schema/PCB senza errori; esame esplicito degli avvisi e delle
   eccezioni locali. Controllo visivo dei piani sotto tutte le coppie, non coperto dal solo DRC.
4. Tutte le [verifiche manuali](#verifiche-manuali) completate.
5. Per ogni modifica dello schema: ERC con zero violazioni e confronto della netlist prima/dopo,
   come richiesto da AGENTS.md. Non rigenerare fogli o PCB per applicare queste regole.
6. Ispezione dei Gerber di tutti i layer, mask, paste, serigrafia, Edge.Cuts e drill PTH/NPTH;
   verifica BOM, DNP, orientamenti e documentazione di assemblaggio. Nessuna esclusione DRC
   ereditata da prog-board-native.
7. Via termiche di U2 (D50): l'impronta del prototipo ha apertura della maschera 1:1 sul lato
   opposto. Per la produzione restringerla a via "encroached", con apertura di 0,36 sulle via
   0,60/0,30 (`solder_mask_margin` −0,12 sui 16 pad 65 forati, nell'impronta e sul PCB), e
   nell'ordine JLCPCB:
   - scrivere nel campo "PCB Remark": *U2 uses solder mask defined openings on the 16 thermal
     vias of the exposed pad (opposite-side mask opening 0.36 mm on 0.6 mm via pads, Microchip
     AN18.15 "encroached" vias). Please do not enlarge or modify the solder mask openings of
     these vias.*;
   - scegliere "Confirm Production File: Yes" e, prima di approvare, controllare nei Gerber
     preparati da JLCPCB che le 16 aperture siano ancora da circa 0,36.

   Senza nota JLCPCB allarga le aperture più piccole del pad
   ([guida JLCPCB](https://jlcpcb.com/help/article/how-to-order-boards-with-solder-mask-defined-pads)).

Stackup, netclass e regole sono configurati nel progetto KiCad. Prima dello sbroglio
definitivo restano da confermare con il fabbricante processo, finitura e controllo
d'impedenza. Questi passaggi non richiedono di spostare i vincoli meccanici già definiti.
