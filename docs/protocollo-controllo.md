# Protocollo di controllo (bozza)

Canale USB HID, usage page vendor (0xFF00), report ID fissi, report da 64 byte, little endian.
HID non richiede driver dedicati e `hidapi` è disponibile sui sistemi previsti.
La bozza riflette D60–D66 e D68; firmware e host non sono ancora implementati.
Il controllo resta HID e i cicli/impulsi sono eseguiti dal MCU: il PC invia il comando
senza dover restare in esecuzione per inviare la riaccensione.

Il protocollo è versionato: `GET_INFO` restituisce `proto_version`; la CLI rifiuta versioni
maggiori che non conosce. Il layout binario dei campi e i codici delle cause di guasto vanno
finalizzati con il controller scelto. Non si assegnano misure fittizie o zeri a funzioni assenti.

## Convenzioni

- Porte da 1 a N (`n_ports` da `GET_INFO`): 1–4 USB-C (`n_ports = 4`); nessuna porta 5.
- Un relè, identificato da 1 (`n_relays = 1`): `connected` = linea collegata, bobina a riposo; `disconnected` = linea
  staccata, bobina eccitata, con cablaggio COM/NC.
- Ogni comando riceve una risposta con lo stesso `cmd` e uno `status` (0 = OK, altrimenti errore),
  salvo i comandi di riavvio/bootloader indicati in tabella.
- Lo stato porta è logico: `enabled`, `cycle_active`, `fault`. Non è una misura della VBUS
  né una conferma del riavvio del dispositivo. Guasti e cause sono solo quelli rilevabili
  dall'hardware selezionato; un segnale generico resta un guasto generico.
- Nessun campo di corrente, tensione, budget, corrente annunciata sulle CC o `budget_blocked`.
- Il MCU governa VBUS e dati con un unico comando logico; la sequenza interna del controller
  è ammessa. Non esiste il parametro `on_delay_ms`.

## Comandi e risposte

| cmd | Nome | Parametri | Risposta |
|---|---|---|---|
| 0x01 | GET_INFO | — | proto_version, fw_version, n_ports, n_relays, capabilities, serial |
| 0x02 | GET_STATUS | — | stato logico e guasti hardware per porta; stato relè; segnalazione di guasto ingresso, se disponibile |
| 0x10 | SET_PORT | port, on (0/1): VBUS e dati insieme | status |
| 0x11 | CYCLE_PORT | port, off_ms (u16): durata dal comando di disabilitazione VBUS, inclusa la scarica | status di accettazione; il ciclo prosegue autonomamente nel MCU |
| 0x12 | SET_ALL | on (0/1) | status; le accensioni sono sfalsate |
| 0x20 | SET_RELAY | relay, line (connected / disconnected) | status |
| 0x21 | PULSE_RELAY | relay, ms (u16): stacca la linea e poi la ricollega | status |
| 0x30 | GET_DEFAULTS | — | stato delle porte all'accensione |
| 0x31 | SET_DEFAULTS | stato delle porte all'accensione | status; i relè all'accensione sono sempre a riposo, linea collegata |
| 0x32 | SAVE_DEFAULTS | — | status; scrive in flash |
| 0x40 | CLEAR_FAULT | port | status; condizioni di ripristino da definire con il controller |
| 0x50 | RESET_HUB | — | status prima del reset; hub e dispositivi downstream, controllo incluso, ri-enumerano |
| 0x7E | REBOOT | — | — |
| 0x7F | ENTER_BOOTLOADER | magic | — |

Il default proposto per `off_ms` nella CLI/libreria è **1000 ms**, da validare sul circuito.
Il campo u16 non implica che tutto l'intervallo 0–65535 ms sia utilizzabile: limiti ammessi,
durata della scarica e comportamento in presenza di guasti vanno definiti con il controller.
La scarica non deve restare attiva senza limiti solo perché la porta resta spenta o l'utente
chiede un'attesa lunga. Non si attende una soglia misurata di VBUS e non si genera un errore
"sorgente esterna" per un timeout di misura. Fine ciclo significa completamento della
sequenza temporizzata, non conferma del reset della scheda.

## Eventi

| evt | Nome | Contenuto |
|---|---|---|
| 0x80 | PORT_FAULT | port, causa segnalata dall'hardware o guasto generico |
| 0x81 | PORT_CHANGED | port, nuovo stato logico (pulsante, comando o fine ciclo) |
| 0x82 | POWER_CHANGED | variazione della segnalazione hardware di guasto ingresso, se disponibile |
| 0x83 | RELAY_CHANGED | relè, stato della linea (pulsante o fine impulso) |

Non sono previste cause dedotte da misure: budget esaurito, VBUS bassa, sorgente esterna o
corrente inversa misurata. Un controller può segnalare direttamente un guasto equivalente:
la causa sarà esposta solo se distinguibile. La perdita dell'alimentazione può spegnere
hub e MCU prima dell'invio di un evento; il PC rileva allora la disconnessione USB.

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
