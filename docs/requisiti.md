# Requisiti

## Problema da risolvere

Durante il test di schede a microcontrollore collegate via USB può servire scollegare e
ricollegare il cavo per recuperare un dispositivo bloccato. Devhub esegue questa operazione
dal PC, in modo scriptabile, e permette di gestire le porte e due relè ausiliari.
Non è uno strumento per misurare consumi o tensioni.

Questi requisiti includono le semplificazioni D60–D65 in
[domande-aperte.md](domande-aperte.md). Lo schema e il PCB presenti nel repository devono
ancora essere adeguati; il controller integrato delle porte non è ancora scelto.

## Requisiti funzionali

### Porte downstream

- **RF-1** 4 porte USB-C e 1 porta USB-A, tutte USB 2.0 Hi-Speed.
- **RF-2** Ogni porta può essere staccata e riattaccata su comando del PC: a porta
  disabilitata VBUS e D+/D− sono interrotte, GND resta collegata. Il distacco dei dati deve
  funzionare anche con dispositivi autoalimentati. Il ciclo di alimentazione riguarda le
  schede alimentate esclusivamente dalla porta USB; per spegnere una scheda alimentata anche
  esternamente occorre intervenire sulla sorgente esterna o sul reset, per esempio con un relè.
  Non si verifica elettricamente che la scheda si sia spenta né che abbia completato il riavvio.
- **RF-3** Alimentazione VBUS e dati D+/D− sono abilitati e disabilitati con un unico comando
  logico. È ammessa la sequenza interna del controller scelto, senza imporre ordine o ritardo
  regolabile fra alimentazione e dati. A porta disabilitata entrambe le connessioni devono
  risultare interrotte. Non sono richiesti comandi separati per dati e alimentazione.
- **RF-4** Il MCU esegue autonomamente un ciclo di spegnimento e riaccensione a seguito di
  un solo comando. È richiesta una **scarica attiva di VBUS**, anche integrata nel controller.
  Il tempo di spegnimento è predefinito e configurabile: valore iniziale indicativo **1 s**,
  da confermare con il circuito scelto e sul prototipo. Si conta dal comando di disabilitazione
  di VBUS e comprende la scarica, senza attesa di una soglia misurata. Durata della scarica e
  intervallo configurabile devono rispettare i limiti del circuito, anche a porta lasciata
  spenta o in presenza di alimentazione esterna. Capacità molto elevate possono richiedere
  attese maggiori; una sorgente esterna non viene eliminata aumentando l'attesa.
- **RF-5** Ogni porta fornisce 5 V con almeno 2 A erogabili sulla singola porta USB-C e
  1,5 A sulla USB-A (limite del connettore, D54), con **limitazione hardware della corrente**
  e segnalazione di guasto per porta. I valori per porta non sono disponibili simultaneamente
  su tutte le uscite: il totale è limitato dall'alimentatore e dai consumi interni (RF-13).
  Sulle USB-C resta l'annuncio di corrente standard USB tramite Rp da 56 kΩ (D30); non si
  annunciano 2 A sulle CC. Il limite hardware della porta resta distinto dall'annuncio USB.
- **RF-6 — Eliminato (D60).** Non sono richieste misure di corrente per porta o all'ingresso,
  accuratezza, calibrazione, azzeramento o visualizzazione degli assorbimenti. Non sono
  richieste misure di tensione delle porte o della 5V_SYS, né diagnostica ricavata da tali misure.

### Dispositivo di controllo

- **RF-7** L'hub espone un dispositivo di controllo USB HID con usage page vendor, senza
  driver dedicati, utilizzabile su Linux, macOS e Windows.
- **RF-8** Il dispositivo di controllo resta raggiungibile con tutte le porte downstream
  disabilitate, tramite una porta interna non sezionabile, finché hub e MCU sono alimentati.
- **RF-9** Comandi: stato globale, stato logico e guasti hardware delle porte, on/off,
  ciclo con durata, comando e stato dei relè, impostazione dello stato di default all'accensione.
  Lo stato riportato non è una verifica elettrica della VBUS o del riavvio del dispositivo.
- **RF-10** Libreria Python generica e multipiattaforma, con CLI per macOS costruita sopra
  (es. `devhub port 2 cycle --ms 1000`).

### Relè ausiliari

- **RF-11** 2 relè indipendenti, con contatti portati su boccole a banana da 4 mm, comandabili
  dal PC (on/off e impulso di N ms) e da un pulsante sul pannello (RF-17).
- **RF-12** I contatti dei relè sono galvanicamente isolati dall'hub; max 16 V / 3 A.

### Alimentazione

- **RF-13** Ingresso USB-C dedicato: è richiesto un alimentatore che offra **5 V / 3 A senza
  negoziazione USB-PD**, con cavo adeguato. Un alimentatore PD è compatibile solo se offre
  anche questa modalità; non tutti gli alimentatori PD la offrono. Alimentatore e cavo idonei
  sono a carico dell'utilizzatore; il funzionamento con capacità inferiore non è garantito.
  I 3 A sono totali all'ingresso e comprendono hub, MCU e relè; alle porte restano indicativamente
  ~2,3 A con l'elettronica attuale, da rivalutare con i componenti definitivi.
- **RF-14 — Eliminato (D65).** Nessuna lettura delle CC d'ingresso, verifica dell'annuncio
  di 3 A o gestione di budget diversi. Restano le resistenze Rd da 5,1 kΩ su CC1 e CC2,
  necessarie per ricevere alimentazione USB-C, e le protezioni hardware d'ingresso.
- **RF-15** Le accensioni multiple sono sfalsate per contenere l'inrush. Il firmware gestisce
  le segnalazioni di guasto disponibili dall'hardware, senza misurare consumi, calcolare budget
  o scegliere la porta più energivora. La protezione da sovraccarico è hardware: un sovraccarico
  complessivo può interrompere l'intero hub. Non è garantita la continuità delle altre porte.
- **RF-16 — Eliminato (D32).** Hub e MCU sono alimentati solo dall'alimentatore esterno;
  senza alimentatore l'hub non è visibile dal PC.

### Comandi sul pannello

- **RF-17** Un pulsante per ogni porta USB e uno per ogni relè. Ogni pressione alterna lo
  stato della porta (dati e alimentazione con un unico comando logico) o della linea del relè.
  I cambi di stato sono notificati all'host. Non è prevista una verifica del budget di corrente.

## Requisiti non funzionali

- **RNF-1** PCB a 4 strati realizzabile con prototipazione standard; componenti montabili
  a mano con piastra riscaldante e aria calda (strategia C in `architettura.md`).
- **RNF-2** Componenti reperibili presso distributori comuni (LCSC/JLCPCB, Mouser, DigiKey).
- **RNF-3** Restano le protezioni ESD sulle linee dati esposte, la protezione d'ingresso,
  la limitazione di corrente e il blocco della corrente inversa. Le protezioni non devono
  dipendere dalle misure eliminate; condizioni e limiti vanno verificati sul circuito scelto.
- **RNF-4** Firmware e strumento host open source e documentati; protocollo stabile e versionato.
- **RNF-5** **Un solo LED per porta USB**, pilotato dal MCU: spento = disabilitata,
  acceso fisso = abilitata, lampeggiante = guasto. Il guasto ha priorità sull'indicazione di
  abilitazione; il dettaglio disponibile dall'hardware si legge dal PC. Il LED indica lo stato
  logico, non la presenza misurata di VBUS. LED dei relè, alimentazione e host invariati.

## Fuori ambito (v1)

- USB 3.x SuperSpeed e isolamento galvanico delle porte USB.
- USB-PD all'ingresso o verso i dispositivi; ricarica del laptop/host dall'hub.
- Misura dei consumi, telemetria delle tensioni e gestione dinamica del budget di potenza.
- Certificazione elettrica dello spegnimento e diagnosi di sorgenti esterne tramite misure.
- Riavvio automatico su blocco rilevato dal PC: è responsabilità degli script dell'utente.
