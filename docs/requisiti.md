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
- **RF-3** Alimentazione e linee dati si staccano e si riattaccano **sempre insieme**, in sequenza:
  allo stacco prima i dati, poi VBUS e la sua scarica; al riattacco prima VBUS, poi i dati dopo un
  ritardo (`on_delay_ms`), così il dispositivo si enumera già alimentato stabilmente.
- **RF-4** Comando di "ciclo" (stacca, attendi N ms, riattacca) eseguito in autonomia dal MCU, così
  che un singolo comando dal PC sia sufficiente anche se l'host perde il dispositivo nel frattempo.
- **RF-5** Ogni porta fornisce 5 V con almeno 2 A erogabili sulla singola porta, con
  **limitazione di corrente** e segnalazione di sovracorrente per porta. Il totale disponibile per
  tutte le porte insieme è limitato dall'alimentatore (vedi RF-13) e gestito dal firmware (RF-15).
  Sulle porte USB-C la corrente **annunciata** sulle CC è quella standard USB (500 mA), ma la porta
  non la impone: qualunque dispositivo che ignora l'annuncio (tipico delle schede di sviluppo) può
  assorbire fino al limite hardware della porta (~2,3 A). I dispositivi USB-C conformi si limitano
  da soli a quanto annunciato (D30).
- **RF-6** Lettura della corrente assorbita da ogni porta: risoluzione dell'ordine dei mA,
  accuratezza ±(2 mA + 1,3 % della lettura) con l'azzeramento dell'offset a porta spenta (D36).

### Dispositivo di controllo

- **RF-7** L'hub espone all'host, oltre alle porte, un dispositivo di controllo che **non dipende
  da driver** (USB HID con usage page vendor) e funziona su Linux, macOS e Windows.
- **RF-8** Il dispositivo di controllo resta raggiungibile anche quando tutte le porte downstream
  sono staccate (è su una porta interna non sezionabile).
- **RF-9** Comandi: stato globale, stato porta, on/off della porta (alimentazione e dati insieme), ciclo con durata,
  comando relè, stato relè, impostazione dello stato di default all'accensione.
- **RF-10** Libreria Python generica per il PC e, sopra di essa, uno strumento da riga di comando
  per macOS (es. `devhub port 2 cycle 500`).

### Relè ausiliari

- **RF-11** 2 relè indipendenti, con contatti portati su boccole a banana da 4 mm, comandabili dal
  PC (on/off e impulso di N ms) e da un pulsante sul pannello (vedi RF-17).
- **RF-12** I contatti dei relè sono galvanicamente isolati dall'hub.

### Alimentazione

- **RF-13** Ingresso di alimentazione su connettore USB-C dedicato, a **5 V senza negoziazione
  PD**: funziona con qualunque alimentatore USB-C da 5 V / 3 A (compresi quelli PD, che erogano
  5 V / 3 A di default). Totale disponibile per le porte: ~2,3 A.
- **RF-14** Il MCU legge la corrente pubblicizzata dall'alimentatore sulle linee CC (0,5 / 1,5 /
  3 A) e ne ricava il budget di potenza.
- **RF-15** Il MCU misura la corrente totale e per porta; non accende una porta se il budget non
  basta, spegne una porta se il totale lo supera e lo segnala all'host (budget di potenza).
- ~~**RF-16**~~ Eliminato (D32): hub e MCU si alimentano solo dall'alimentatore; senza
  alimentatore l'hub non si vede dal PC.

### Comandi sul pannello

- **RF-17** Un pulsante per ogni porta e uno per ogni relè. Ogni pressione alterna lo stato:
  la porta si accende o si spegne (alimentazione e dati sempre insieme, con la stessa sequenza
  dei comandi da PC), la linea del relè si collega o si stacca. L'accensione di una porta da
  pulsante rispetta il budget di potenza (RF-15). Ogni cambio di stato da pulsante è notificato
  all'host.

## Requisiti non funzionali

- **RNF-1** PCB realizzabile con servizio di prototipazione standard (4 strati); componenti
  montabili a mano con piastra riscaldante e aria calda (strategia C in `architettura.md`).
- **RNF-2** Componenti reperibili presso distributori comuni (LCSC/JLCPCB, Mouser, DigiKey).
- **RNF-3** Protezione ESD su tutte le linee dati esposte e TVS sulla VBUS d'ingresso (sulle VBUS
  delle porte bastano il clamp dell'ESD e i condensatori, vedi D25).
- **RNF-4** Firmware e strumento host open source e documentati; protocollo stabile e versionato.
- **RNF-5** LED di stato per porta (alimentata / fault).

## Fuori ambito (v1)

- USB 3.x SuperSpeed (vedi motivazione in `architettura.md`).
- Negoziazione USB-PD sull'ingresso (variante ad alta potenza documentata in `architettura.md`).
- Power Delivery verso i dispositivi downstream (solo 5 V).
- Ricarica di laptop/host dall'hub.
- Logica di "hub smart" lato host (es. hot-plug automatico su timeout): è responsabilità degli
  script dell'utente, che usano la CLI.
