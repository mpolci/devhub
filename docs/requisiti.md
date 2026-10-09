# Requisiti

## Problema da risolvere

Durante il test di dispositivi a microcontrollore collegati via USB capita che il dispositivo si
blocchi e che il reset comandato via USB non funzioni. L'unico rimedio è scollegare e ricollegare
il cavo. Si vuole poterlo fare **dal PC**, in modo scriptabile, senza toccare il banco.

## Requisiti funzionali

### Porte downstream

- **RF-1** 4 porte USB-C e 1 porta USB-A, tutte USB 2.0 Hi-Speed.
- **RF-2** Ogni porta deve poter essere **staccata e riattaccata** su comando del PC. Lo stacco
  deve essere equivalente allo sfilare il cavo: il dispositivo deve risultare scollegato per
  l'host e perdere l'alimentazione da VBUS.
- **RF-3** Alimentazione e linee dati devono essere comandabili **separatamente** (es. togliere
  solo i dati lasciando alimentato il dispositivo, o viceversa).
- **RF-4** Comando di "ciclo" (stacca, attendi N ms, riattacca) eseguito in autonomia dal MCU, così
  che un singolo comando dal PC sia sufficiente anche se l'host perde il dispositivo nel frattempo.
- **RF-5** Ogni porta fornisce 5 V con almeno 2 A disponibili, con **limitazione di corrente** e
  segnalazione di sovracorrente per porta.
- **RF-6** Lettura della corrente assorbita da ogni porta (risoluzione dell'ordine dei mA).

### Dispositivo di controllo

- **RF-7** L'hub espone all'host, oltre alle porte, un dispositivo di controllo che **non dipende
  da driver** (USB HID con usage page vendor) e funziona su Linux, macOS e Windows.
- **RF-8** Il dispositivo di controllo resta raggiungibile anche quando tutte le porte downstream
  sono staccate (è su una porta interna non sezionabile).
- **RF-9** Comandi: stato globale, stato porta, on/off alimentazione, on/off dati, ciclo con durata,
  comando relè, stato relè, impostazione dello stato di default all'accensione.
- **RF-10** Strumento da riga di comando per il PC (es. `devhub port 2 cycle 500`).

### Relè ausiliari

- **RF-11** 2 relè indipendenti, con contatti portati su boccole a banana da 4 mm, comandabili dal
  PC (on/off e impulso di N ms).
- **RF-12** I contatti dei relè sono galvanicamente isolati dall'hub.

### Alimentazione

- **RF-13** Ingresso di alimentazione su connettore USB-C dedicato, da alimentatore USB-PD.
- **RF-14** L'hub negozia con l'alimentatore una tensione superiore a 5 V (20 V preferita) per
  disporre della potenza necessaria a tutte le porte.
- **RF-15** Il MCU conosce la potenza negoziata e, se insufficiente, lo segnala all'host e limita
  il numero di porte attivabili (budget di potenza).
- **RF-16** Con alimentatore assente, hub e MCU restano alimentati dalla porta upstream quanto basta
  per enumerare e riportare "alimentatore assente" (le porte downstream restano spente).

## Requisiti non funzionali

- **RNF-1** PCB realizzabile con servizio di prototipazione standard (4 strati, assemblaggio SMT
  dal produttore per i componenti fini).
- **RNF-2** Componenti reperibili presso distributori comuni (LCSC/JLCPCB, Mouser, DigiKey).
- **RNF-3** Protezione ESD su tutte le linee dati esposte e TVS su VBUS.
- **RNF-4** Firmware e strumento host open source e documentati; protocollo stabile e versionato.
- **RNF-5** LED di stato per porta (alimentata / dati / fault).

## Fuori ambito (v1)

- USB 3.x SuperSpeed (vedi motivazione in `architettura.md`).
- Power Delivery verso i dispositivi downstream (solo 5 V).
- Ricarica di laptop/host dall'hub.
- Logica di "hub smart" lato host (es. hot-plug automatico su timeout): è responsabilità degli
  script dell'utente, che usano la CLI.
