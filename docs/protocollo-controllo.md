# Protocollo di controllo (bozza)

Canale USB HID, usage page vendor (0xFF00), report ID fissi, report da 64 byte, little endian.
HID non richiede driver dedicati e `hidapi` è disponibile sui sistemi previsti.
La bozza riflette D60–D66, D68 e D96–D103; firmware e libreria host la implementano e sono
provati sul prototipo Rev A.
Il controllo resta HID e i cicli/impulsi sono eseguiti dal MCU: il PC invia il comando
senza dover restare in esecuzione per inviare la riaccensione.

Il protocollo è versionato: `GET_INFO` restituisce `proto_version`; la CLI rifiuta versioni
maggiori che non conosce. Non si assegnano misure fittizie o zeri a funzioni assenti.
Il layout binario qui descritto è la versione **0.1**: resta modificabile finché firmware e
libreria non sono collaudati, poi diventa 1.0.

## Convenzioni

- Porte da 1 a N (`n_ports` da `GET_INFO`): 1–4 USB-C (`n_ports = 4`); nessuna porta 5.
- Un relè, identificato da 1 (`n_relays = 1`): `connected` = linea collegata, bobina a riposo; `disconnected` = linea
  staccata, bobina eccitata, con cablaggio COM/NC.
- Ogni comando riceve una risposta con lo stesso `cmd` e uno `status` (0 = OK, altrimenti errore),
  salvo i comandi di riavvio/bootloader indicati in tabella.
- Lo stato porta è logico: `enabled`, `cycle_active`, `fault`. Non è una misura della VBUS
  né una conferma del riavvio del dispositivo. Guasti e cause sono solo quelli rilevabili
  dall'hardware selezionato; un segnale generico resta un guasto generico.
- Grandezze misurate: corrente e carica accumulata di ogni porta, dai controller delle porte
  (D103), e temperatura del MCU (D102). Nessun campo di tensione, corrente totale, budget,
  corrente annunciata sulle CC o `budget_blocked`.
- Il MCU governa VBUS e dati con un unico comando logico; la sequenza interna del controller
  è ammessa. Non esiste il parametro `on_delay_ms`.

Lo stato di fabbrica è tutte le porte abilitate (D74). I default salvati sostituiscono
questa impostazione; le accensioni restano sfalsate di 100 ms, valore da validare sul
prototipo. Il relè parte a riposo, linea collegata.

Lo stato di porte e relè non dipende dalla presenza del PC: scollegamento o sospensione
dell'host non lo modificano (D100).

## Dispositivo USB

Il MCU è un dispositivo composito Full-Speed sulla porta 6 del hub (D97):

| Interfaccia | Funzione |
|---|---|
| HID vendor | canale di controllo descritto in questo documento |
| CDC ACM | console diagnostica a comandi di testo e log; non fa parte del protocollo |

VID 0x1209, PID di test 0x0001 durante lo sviluppo (D13). Il numero di serie USB è l'UID a
96 bit del MCU in esadecimale, lo stesso restituito da `GET_INFO`.

## Formato dei report

Tre report HID da 64 byte, report ID compreso. I byte non usati sono a zero.

| Report ID | Direzione | Uso | Byte 1 | Byte 2 | Byte 3 | Dal byte 4 |
|---|---|---|---|---|---|---|
| 0x01 | PC → hub (OUT) | comando | `cmd` | `seq` | parametri → | |
| 0x02 | hub → PC (IN) | risposta | `cmd` | `seq` | `status` | dati |
| 0x03 | hub → PC (IN) | evento | `evt` | dati → | | |

`seq` è un contatore scelto dal PC e riportato nella risposta, per associare risposta e
comando; gli eventi possono arrivare fra un comando e la sua risposta. Nel report di comando
i parametri iniziano al byte 3, in quello di evento i dati iniziano al byte 2.

### Codici di `status`

| Valore | Nome | Significato |
|---|---|---|
| 0x00 | OK | comando accettato |
| 0x01 | UNKNOWN_CMD | comando non riconosciuto |
| 0x02 | BAD_PARAM | porta, relè o valore fuori intervallo |
| 0x03 | PORT_FAULT | porta bloccata per guasto: serve `SET_PORT` on o `CLEAR_FAULT` |
| 0x04 | HW_ERROR | il controller della porta o il hub non risponde |
| 0x05 | STORAGE_ERROR | scrittura dei default in flash non riuscita |

## Comandi e risposte

| cmd | Nome | Parametri | Risposta |
|---|---|---|---|
| 0x01 | GET_INFO | — | proto_version, fw_version, n_ports, n_relays, capabilities, serial |
| 0x02 | GET_STATUS | — | stato di sistema; per porta stato logico, guasti, avvisi, corrente e carica; stato relè; temperatura del MCU |
| 0x10 | SET_PORT | port, on (0/1): VBUS e dati insieme | status |
| 0x11 | CYCLE_PORT | port, off_ms (u16): durata dal comando di disabilitazione VBUS, inclusa la scarica | status di accettazione; il ciclo prosegue autonomamente nel MCU |
| 0x12 | SET_ALL | on (0/1) | status; le accensioni sono sfalsate |
| 0x13 | RESET_CHARGE | port | status; la carica accumulata della porta riparte da zero; `HW_ERROR` se il controller non risponde |
| 0x20 | SET_RELAY | relay, line (0 = connected, 1 = disconnected) | status |
| 0x21 | PULSE_RELAY | relay, ms (u16): stacca la linea e poi la ricollega | status |
| 0x30 | GET_DEFAULTS | — | maschera delle porte abilitate all'accensione |
| 0x31 | SET_DEFAULTS | maschera delle porte abilitate all'accensione (u8, bit 0 = porta 1) | status; i relè all'accensione sono sempre a riposo, linea collegata |
| 0x32 | SAVE_DEFAULTS | — | status; scrive in flash le porte abilitate all'accensione e l'azzeramento automatico della carica |
| 0x33 | GET_CHARGE_RESET | — | maschera delle porte la cui carica si azzera a ogni accensione |
| 0x34 | SET_CHARGE_RESET | maschera (u8, bit 0 = porta 1); di fabbrica 0 | status |
| 0x40 | CLEAR_FAULT | port | status; toglie il blocco e lascia la porta disabilitata |
| 0x50 | RESET_HUB | — | status prima del reset; hub e dispositivi downstream, controllo incluso, ri-enumerano |
| 0x7E | REBOOT | — | — |
| 0x7F | ENTER_BOOTLOADER | magic (u32 = 0x21554644, "DFU!") | — |

Dati di `GET_INFO`, dal byte 4 della risposta:

| Offset | Campo | Tipo | Note |
|---|---|---|---|
| 4 | proto_version | u8 major, u8 minor | 0.1 |
| 6 | fw_version | u8 major, u8 minor, u8 patch | |
| 9 | n_ports | u8 | 4 |
| 10 | n_relays | u8 | 1 |
| 11 | capabilities | u32 | bit 0 = console CDC presente; gli altri a zero |
| 15 | serial | 12 byte | UID del MCU |

Dati di `GET_STATUS`, dal byte 4 della risposta:

| Offset | Campo | Tipo | Note |
|---|---|---|---|
| 4 | system | u8 | bit 0 = guasto ingresso segnalato dall'eFuse; bit 1 = hub configurato via SMBus; bit 2 = controller porte 1-2 non risponde; bit 3 = controller porte 3-4 non risponde |
| 5 | porte | 9 byte per porta | `state` (u8), `fault` (u8), `warning` (u8), `current` (u16), `charge` (u32), nell'ordine delle porte |
| 5 + 9·n_ports | relè | 1 byte per relè | bit 0 = linea staccata; bit 1 = impulso in corso |
| 5 + 9·n_ports + n_relays | mcu_temp | i8 | temperatura del chip del MCU in °C (D102); −128 se la lettura non riesce |

Byte di una porta:

| Campo | Bit | Significato |
|---|---|---|
| `state` | 0 | `enabled`: VBUS e dati abilitati |
| | 1 | `cycle_active`: ciclo in corso, porta nella fase spenta |
| | 2 | `fault`: porta bloccata per guasto (D73) |
| `fault` | 0 | sovracorrente |
| | 1 | sovratemperatura del controller |
| | 7 | guasto generico: ALERT# attivo senza causa leggibile |
| `warning` | 0 | scarica non riuscita: VBUS sostenuta dall'esterno |
| | 1 | tensione di ritorno sulla VBUS |
| | 2 | VBUS sotto il minimo del controller |

| `current` | — | corrente della porta in mA, passo 13,3 mA; 0xFFFF se il controller non risponde |
| `charge` | — | carica erogata dall'ultimo azzeramento in µAh, aggiornata ogni secondo; 0xFFFFFFFF se il controller non risponde |

`current` e `charge` vengono dagli UCS2114 (D103): corrente ±2 % sopra 200 mA e ±2 passi sotto,
carica ±4,5 %. Lo zero della corrente non è esatto: a porta vuota si legge 0 o 13 mA e la
carica cresce di qualche mAh all'ora. La carica si conserva con la porta spenta; si azzera con
`RESET_CHARGE`, a ogni accensione per le porte indicate da `SET_CHARGE_RESET`, e quando il
controller si resetta o resta senza alimentazione; un riavvio del solo MCU non la azzera.

`fault` conserva la causa finché la porta è bloccata. Gli avvisi non bloccano la porta né
il ciclo (D98) e restano impostati fino al comando successivo sulla stessa porta.

### Regole dei comandi

- `SET_PORT` on su una porta bloccata è il tentativo esplicito di riabilitazione: toglie il
  blocco e riaccende; se il guasto persiste la porta torna bloccata con un nuovo `PORT_FAULT`.
  `SET_PORT` off su una porta bloccata equivale a `CLEAR_FAULT`.
- `CYCLE_PORT` su una porta disabilitata è ammesso e termina con la porta abilitata; su una
  porta bloccata risponde `PORT_FAULT`. Un nuovo `CYCLE_PORT` durante un ciclo lo fa ripartire;
  `SET_PORT` durante un ciclo lo annulla e applica lo stato richiesto.
- `off_ms`: da **500 a 60000 ms**; 0 = default del firmware, **1000 ms**. Fuori intervallo
  `BAD_PARAM`. Il minimo copre la scarica automatica da 400 ms del controller.
- `PULSE_RELAY`: `ms` da 10 a 60000; un impulso su linea già staccata la ricollega alla scadenza.
- `SET_ALL` on non riabilita le porte bloccate; `SET_ALL` off annulla i cicli in corso.

I limiti di `off_ms` e lo sfasamento delle accensioni vanno validati sul prototipo.
La scarica non resta attiva senza limiti quando la porta resta spenta o l'utente chiede
un'attesa lunga: è temporizzata dal controller. Non si attende una soglia misurata di VBUS;
se il controller segnala la scarica non riuscita, il ciclo prosegue e la segnalazione
diventa un avviso. Fine ciclo significa completamento della sequenza temporizzata, non
conferma del reset della scheda.

## Ripristino delle porte dopo guasto (D73)

Un guasto hardware segnalato (sovracorrente o sovratemperatura, D98) interrompe il ciclo e
porta VBUS e dati in OFF, con LED lampeggiante. La scomparsa del segnale non riabilita la
porta. Un comando esplicito di riabilitazione da PC o una pressione del pulsante permette un
nuovo tentativo; un guasto persistente o ricorrente riporta la porta in OFF con LED
lampeggiante. `CLEAR_FAULT` toglie il blocco senza riaccendere.

## Pulsanti

Ogni pressione alterna lo stato della porta o della linea del relè e genera l'evento
corrispondente. Su una porta bloccata la pressione è il tentativo di riabilitazione; durante
un ciclo lo annulla e lascia la porta disabilitata (D99).

## Eventi

| evt | Nome | Dati (dal byte 2) |
|---|---|---|
| 0x80 | PORT_FAULT | port, `fault` |
| 0x81 | PORT_CHANGED | port, `state`, origine |
| 0x82 | POWER_CHANGED | `system` |
| 0x83 | RELAY_CHANGED | relay, stato del relè, origine |
| 0x84 | PORT_WARNING | port, `warning` |

Origine: 0 = comando dal PC, 1 = pulsante, 2 = fine ciclo o impulso, 3 = guasto, 4 = avvio.

Non sono previste cause dedotte da misure: budget esaurito o VBUS bassa misurata. Scarica
non riuscita e tensione di ritorno sono segnalazioni dirette del controller, esposte come
avvisi. La perdita dell'alimentazione può spegnere hub e MCU prima dell'invio di un evento;
il PC rileva allora la disconnessione USB.

## CLI

```
devhub info
devhub status [--json]
devhub port <n> on|off
devhub port <n> cycle [--ms 1000]
devhub all on|off
devhub relay <n> connect|disconnect
devhub relay <n> cycle [--ms 500]
devhub defaults show|set ...|save
devhub hub reset
devhub watch
```

`--json` serve agli script di test. Codice di uscita diverso da zero su errore o guasto.
Nessuna opzione `--on-delay` o impostazione del budget; nessuna telemetria elettrica.
