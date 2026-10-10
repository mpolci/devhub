# Revisione elettrica 02

Data: 10 ottobre 2026.

Versione analizzata: commit `06b944187dddbc253e0edb53689f4543b6270991` di `main`
su GitHub, «Pulsanti per i relè (SW1/SW2 su PB12/PB13); requisito RF-17 sui pulsanti
del pannello (D26)».

## Ambito e metodo

Revisione del funzionamento elettrico e dello schema logico: alimentazioni,
sezionamento delle porte, protezioni, interfacce USB, misura della corrente,
pilotaggio dei relè e condizioni di avvio. La disposizione grafica dello schema
KiCad e il layout PCB non sono oggetto di questa revisione.

Sono stati esaminati documentazione, connessioni della netlist esportata dalla
versione indicata e datasheet dei componenti. L'ERC con KiCad CLI 10.0.6 sulla
versione analizzata riporta **zero violazioni**. Questo risultato non esclude
problemi funzionali o di dimensionamento, che l'ERC non verifica.

La prima ricognizione era partita dal checkout locale `f221d1f`, poi aggiornato
alla versione GitHub prima di completare la revisione. I rilievi seguenti si
riferiscono tutti a `06b9441`, inclusi eFuse TPS259470A, switch PI3USB4002A,
regolatore AP7361C e supervisore MCP120.

Non sono state effettuate modifiche progettuali, simulazioni o misure su un
prototipo. Le proposte sono raccomandazioni di revisione, non nuove decisioni
progettuali già approvate.

## Esito

**Ci sono problemi funzionali da risolvere prima del PCB.** Le modifiche recenti
migliorano alimentazione e protezioni, ma alcuni componenti, corretti
singolarmente, interagiscono in modo problematico.

Le priorità sono:

1. eliminare i percorsi indesiderati introdotti dalle protezioni USBLC6;
2. definire gestione CC e allocazione della corrente delle porte USB-C;
3. coordinare TVS, misura ed eFuse all'ingresso.

## Rilievi principali

### R02-01 — USBLC6: accoppiamento dati/VBUS e percorso che aggira la protezione OVP

**Priorità: alta. Problema circuitale identificato.**

Riferimenti: `port_c.kicad_sch`, D101 e componenti equivalenti delle porte 2–4;
`port_a.kicad_sch`, D501. Il pin VBUS degli USBLC6-2SC6 è collegato alla VBUS
commutata della rispettiva porta. I diodi interni collegano le linee dati a quel
rail.

Questo crea due problemi:

- Con **VBUS spenta e dati collegati**, un livello alto sui dati può alimentare
  parzialmente VBUS. Durante la scarica attiva, il percorso verso massa può
  invece abbassare il livello alto e compromettere la comunicazione. Il circuito
  non garantisce quindi l'indipendenza richiesta tra dati e alimentazione.
- Se una scheda applica **12–24 V a D+ o D−**, il PI3USB4002A può aprirsi, ma
  l'USBLC6 offre comunque un percorso verso VBUS e il proprio clamp. Il guasto
  può danneggiare la protezione o coinvolgere l'alimentazione della porta.

**La tolleranza a 24 V dello switch non vale per l'intera porta.**

Raccomandazione: scegliere e verificare una protezione ESD compatibile sia con
VBUS assente sia con la tensione continua di guasto prevista. Non basta spostare
il collegamento a un altro rail senza analizzare dove finirebbe la corrente.

Fonti: [USBLC6-2, schema interno e modello dei diodi](https://www.st.com/resource/en/datasheet/usblc6-2.pdf),
[PI3USB4002A](https://www.diodes.com/datasheet/download/PI3USB4002A.pdf).

### R02-02 — USB-C downstream: manca il rilevamento dell'inserimento tramite CC

**Priorità: alta. Funzione hardware mancante.**

Riferimenti: `port_c.kicad_sch`, R102/R103 e istanze equivalenti; gestione EN dei
TPS2561A.

Le Rp da 22 kΩ sono alimentate dalla stessa VBUS attivata dal MCU. Non è presente
un rilevamento di Rd per determinare quando sia collegato un sink.

Una sorgente USB-C deve presentare Rp, rilevare un dispositivo compatibile e
solo allora attivare VBUS, rimuovendola al distacco. Nel circuito attuale una
porta abilitata eroga tensione anche senza dispositivo. Collegando un'altra
sorgente si possono mettere in comunicazione due alimentazioni.

Raccomandazione: aggiungere gestione CC per porta, anche senza USB-PD, che
condizioni l'abilitazione del TPS2561A. Il collegamento in parallelo delle coppie
D+/D− nei due orientamenti è invece corretto per USB 2.0.

Fonte: [TI TPS25821, esempio di controller source con rilevamento del sink e comando VBUS](https://www.ti.com/product/TPS25821).

### R02-03 — Corrente annunciata dalle porte e budget disponibile non sono coerenti

**Priorità: alta. Incoerenza tra annuncio hardware e politica di allocazione.**

Riferimenti: Rp delle porte USB-C;
[`architettura.md`, gestione del budget](../architettura.md).

Ogni USB-C annuncia 1,5 A, mentre tutte le porte insieme dispongono di circa
2,3 A. La regola `I_totale + 200 mA < budget` permette di accendere più
dispositivi inizialmente inattivi, che possono poi richiedere contemporaneamente
la corrente annunciata.

Due dispositivi da 1,5 A bastano a superare il budget. Spegnerne uno dopo 50 ms
è una reazione al sovraccarico, ma nel frattempo l'alimentatore può già abbassare
o interrompere la tensione. Il limite d'ingresso da 3,5 A non assicura il rispetto
del budget di una sorgente da 3 A, e tanto meno di una sorgente da 1,5 A.

Raccomandazione: allocare la corrente prima di prometterla sulle CC, oppure
ridurre l'annuncio e definire una politica coerente per le porte abilitate.
Distinguere la corrente istantaneamente misurata dalla corrente già assegnata
ai dispositivi.

Inoltre, 2 A fisicamente erogabili non significano 2 A disponibili a un dispositivo
USB-C conforme quando si annunciano 1,5 A. La capacità elettrica della porta e
la corrente autorizzata dall'interfaccia devono essere descritte separatamente.

### R02-04 — La protezione d'ingresso non sostiene il limite dichiarato di 28 V

**Priorità: alta. Limiti dei componenti non coordinati.**

Riferimenti: `power_in.kicad_sch`, D1, R5, U13;
`switches.kicad_sch`, canale 3 di U8.

L'INA3221 U8 misura lo shunt prima dell'eFuse: i suoi ingressi hanno un massimo
assoluto di **26 V**. L'eFuse non può proteggerli aprendo l'uscita. Il caso
d'ingresso a 28 V dichiarato nella documentazione supera quindi il limite del
monitor di corrente.

La SMAJ20A, inoltre, non mantiene necessariamente la tensione vicino a 22 V:
quella è la zona iniziale di breakdown. Per esempio, il datasheet Littelfuse
specifica un clamp massimo di **32,4 V alla corrente impulsiva nominale**, oltre
il massimo assoluto di 28 V dell'eFuse. La tensione effettiva dipende dalla
corrente e dall'impulso; non si può dedurre una protezione sotto 28 V dal solo
valore nominale «20 V» della TVS.

Raccomandazione: coordinare tutta la catena TVS–misura–eFuse, distinguendo
tensione continua tollerata, campo operativo e impulso sopportabile. Proteggere
anche la misura a monte o scegliere un monitor con margine adeguato. Fissare
produttore e caratteristiche della TVS nella BOM.

Fonti: [INA3221, absolute maximum ratings](https://www.ti.com/lit/ds/symlink/ina3221.pdf),
[SMAJ Littelfuse](https://www.littelfuse.com/~/media/electronics/datasheets/tvs_diodes/littelfuse_tvs_diode_smaj_datasheet.pdf.pdf),
[TPS25947](https://www.ti.com/lit/ds/symlink/tps25947.pdf).

### R02-05 — Assorbimento dall'upstream non controllato e bilancio di avvio da dimostrare

**Priorità: media. Comportamento identificato; conformità e margini da verificare.**

Riferimenti: `power_in.kicad_sch`, D2/D3 e U1; `hub.kicad_sch`, J2 e U2.

L'ORing a diodi sceglie sostanzialmente l'alimentazione con tensione maggiore:
può assorbire dal PC anche con alimentatore esterno presente. La documentazione
lo riconosce, ma manca il controllo della corrente prelevata dall'upstream.

Il solo USB2517 dichiara circa **95 mA tipici prima della configurazione**.
Aggiungendo MCU e altri circuiti, non è dimostrato il rispetto dei 100 mA della
modalità USB 2.0 bus-powered prima dell'enumerazione. Le CC upstream non vengono
lette per conoscere un'eventuale disponibilità superiore. Il valore tipico del
solo hub non è un limite massimo garantito per l'intera scheda.

Raccomandazione: rivedere la selezione delle sorgenti con priorità
all'alimentatore esterno e produrre un bilancio esplicito per avvio, enumerazione,
funzionamento e sospensione. Rendere coerenti assorbimenti reali e configurazione
USB dell'hub.

Il diodo impedisce il ritorno verso il PC, ma non limita quanto viene assorbito
dal PC.

Fonte: [USB2517, caratteristiche elettriche e modalità self/bus-powered](https://ww1.microchip.com/downloads/aemDocuments/documents/UNG/ProductDocuments/DataSheets/USB2517-USB2517i-Data-Sheet-00001598C.pdf).

### R02-06 — Poco margine di tensione per l'attrazione dei relè

**Priorità: media. Attrazione non garantita nell'intero campo di alimentazione previsto.**

Riferimenti: `relays.kicad_sch`, U9, K1/K2.

L'ULN2003A introduce una caduta significativa: il datasheet riporta circa
0,9 V tipici e 1,1 V massimi a 100 mA nelle condizioni specificate. Il G5Q-1
da 5 V richiede fino a **3,75 V per garantire l'attrazione a 23 °C**.

Con 5V_SYS a 4,5 V, alla bobina possono restare circa 3,4–3,6 V: il relè
potrebbe non scattare proprio quando serve interrompere l'alimentazione del
dispositivo in prova. Cavo, shunt ed eFuse riducono il margine rispetto ai 5 V
nominali; anche la temperatura va inclusa nella verifica.

Raccomandazione: preferire due MOSFET a bassa resistenza, specificati per comando
a 3,3 V, con diodi di ricircolo, verificando comunque la tensione minima
disponibile sulle bobine.

Fonti: [G5Q](https://omronfs.omron.com/en_US/ecb/products/pdf/en-g5q.pdf),
[ULN2003A](https://www.ti.com/lit/ds/symlink/uln2003a.pdf).

## Punti da chiudere e verifiche richieste

### R02-07 — Reset, UCPD e avvio senza firmware

PA8, usato per HUB_RST_N, è anche UCPD1_CC1; PA9/PA10, associati alla funzione
dead-battery, sono lasciati scollegati. La nota «probabilmente vanno rilasciati»
in [`pinout-mcu.md`](../pinout-mcu.md) deve diventare una scelta verificata prima
di dare per garantiti l'avvio senza firmware e la programmazione iniziale via DFU.

ST prescrive una gestione precisa dei DBCC inutilizzati; RM0444 descrive anche
gli strobe per disabilitare i pull-down UCPD. Vanno analizzati gli stati di
accensione, reset e bootloader, senza assumere che la configurazione effettuata
dal firmware applicativo sia sempre presente. Considerare anche PD0/PD2 e i
relativi DBCC, riutilizzati per i segnali della porta 4.

Raccomandazione: valutare un GPIO ordinario per il reset dell'hub e definire
esplicitamente la gestione dei pin UCPD/DBCC. Il mancato avvio via DFU non è
stato dimostrato con una prova: questo è un rischio da chiudere, non un esito
sperimentale.

Fonte: [RM0444, §38.4.6 e SYSCFG_CFGR1](https://www.st.com/resource/en/reference_manual/rm0444-stm32g0x1-advanced-armbased-32bit-mcus-stmicroelectronics.pdf).

### R02-08 — Ripristino dell'hub dopo brownout

Il supervisore può resettare solo l'hub mentre il MCU continua a funzionare,
se il calo della 3,3 V attraversa la soglia del supervisore ma non provoca un
reset del MCU nella configurazione adottata.

Il firmware dovrà rilevare il reset dell'hub e riconfigurarlo. Controllare
soltanto la causa del reset del MCU all'avvio non copre questo caso. Va inoltre
rispettato il rilascio effettivo del supervisore prima della configurazione SMBus.

### R02-09 — Risoluzione e accuratezza della misura di corrente

Con INA3221 e shunt da 20 mΩ:

- risoluzione: 40 µV / 20 mΩ = **2 mA per conteggio**;
- offset massimo a 25 °C: 80 µV / 20 mΩ = **4 mA equivalenti**, prima degli
  altri contributi di errore.

È coerente con il requisito «ordine dei mA», ma non con accuratezza assoluta
di 1 mA. Tolleranza dello shunt, deriva, collegamenti Kelvin e calibrazione
devono essere inclusi se si vogliono specificare prestazioni quantitative.

Fonte: [INA3221, electrical characteristics](https://www.ti.com/lit/ds/symlink/ina3221.pdf).

### R02-10 — Durata della scarica VBUS

Una scarica di 100 ms non garantisce il reset di qualsiasi carico. Nel modello
RC ideale, con 1.000 µF e 100 Ω la costante di tempo è 100 ms: dopo 100 ms
rimangono circa **1,84 V** partendo da 5 V. La capacità comprende quella del
dispositivo collegato, non soltanto quella a bordo dell'hub.

Raccomandazione: usare anche la misura di VBUS per decidere quando riaccendere,
con soglia e timeout espliciti. Un dispositivo alimentato da un'altra sorgente
non può essere considerato spento soltanto perché il TPS2561A è disabilitato.

## Elementi verificati e coerenti

- Limite dei TPS2561A con R_ILIM da 24,3 kΩ: 2,1–2,5 A per canale nelle
  condizioni specificate dal datasheet.
- Polarità dei comandi dei PI3USB4002A: EN alto scollega le linee; lato C0 verso
  il connettore e lato L0 verso l'hub.
- Collegamento degli shunt rispetto agli ingressi IN+/IN− dei monitor.
- Indirizzi dei due INA3221, 0x40 e 0x41.
- Collegamento delle coppie USB 2.0 nei due orientamenti dei connettori USB-C.

Queste verifiche non sostituiscono le prove di integrità del segnale,
compatibilità USB, transitori, ESD e comportamento termico sul prototipo.

## Prove consigliate dopo le correzioni

1. VBUS spenta con dati abilitati: misurare tensioni e correnti residue, con
   dispositivo autoalimentato e durante la scarica.
2. USB-C: inserimento e distacco nei due orientamenti, porta vuota, collegamento
   a una sorgente e carichi simultanei entro la corrente annunciata.
3. Alimentazione: avvio da upstream soltanto, inserzione/rimozione
   dell'alimentatore esterno e sovraccarico di una porta mantenendo raggiungibile
   il controllo quando previsto.
4. Reset: flash vuota, DFU, watchdog del MCU e calo di tensione che resetti
   soltanto l'hub.
5. Relè: attrazione alla tensione minima e alla temperatura massima previste.
6. Protezioni: dopo aver definito i limiti, verificare sovratensioni e transitori
   con sorgenti limitate in corrente, misurando anche i nodi a monte dell'eFuse.
