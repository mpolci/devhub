# Protocollo di controllo (bozza)

Canale: USB HID, usage page vendor (0xFF00). Report ID fissi, dimensione fissa 64 byte, little
endian. Scelta HID perché non richiede driver su nessun sistema operativo e `hidapi` è disponibile
ovunque.

Il protocollo è versionato: `GET_INFO` restituisce `proto_version`; la CLI rifiuta versioni
maggiori che non conosce.

## Convenzioni

- Porte numerate da 1 a N (`n_ports` da `GET_INFO`). Le porte 1-4 sono le USB-C, la 5 è la USB-A.
- Relè numerati da 1 a 2. Lo stato di un relè è espresso come stato della **linea**: `connected`
  (riposo, bobina diseccitata) o `disconnected` (bobina eccitata).
- Ogni comando riceve una risposta con lo stesso `cmd` e un campo `status` (0 = OK, altrimenti
  codice di errore).

## Comandi (Output report, host → hub) e risposte (Input report)

| cmd | Nome | Parametri | Risposta |
|---|---|---|---|
| 0x01 | GET_INFO | — | proto_version, fw_version, n_ports, n_relays, capabilities (bitmask: current_meas, pd_status, …), serial |
| 0x02 | GET_STATUS | — | per porta: flags (power, data, fault, budget_blocked), corrente mA; relè: stato; alimentazione: tensione d'ingresso mV, stato dell'eFuse, 5V_SYS mV, corrente totale mA, corrente pubblicizzata dall'alimentatore (0,5/1,5/3 A), budget mA e usato |
| 0x10 | SET_PORT | port, on (0/1): alimentazione e dati insieme, in sequenza (RF-3) | status |
| 0x11 | CYCLE_PORT | port, off_ms (u16), on_delay_ms (u16, ritardo dati dopo VBUS) | status (il ciclo prosegue in autonomia) |
| 0x12 | SET_ALL | on (0/1) | status |
| 0x20 | SET_RELAY | relay, line (connected / disconnected) | status |
| 0x21 | PULSE_RELAY | relay, ms (u16): stacca la linea per ms, poi la ricollega | status |
| 0x30 | GET_DEFAULTS | — | stato all'accensione delle porte, limite budget |
| 0x31 | SET_DEFAULTS | stato all'accensione delle porte, limite budget (i relè all'accensione sono sempre "linea collegata") | status |
| 0x32 | SAVE_DEFAULTS | — | status (scrive in flash) |
| 0x40 | CLEAR_FAULT | port | status |
| 0x50 | RESET_HUB | — | status, inviato prima del reset (il hub e tutto ciò che ha a valle, compreso il dispositivo di controllo, ri-enumerano) |
| 0x7E | REBOOT | — | — |
| 0x7F | ENTER_BOOTLOADER | magic | — |

## Eventi (Input report non sollecitati)

| evt | Nome | Contenuto |
|---|---|---|
| 0x80 | PORT_FAULT | port, causa (sovracorrente / tensione bassa / budget / tensione inversa) |
| 0x81 | PORT_CHANGED | port, nuovi flags (fine di un ciclo, pulsante fisico) |
| 0x82 | POWER_CHANGED | alimentatore collegato/scollegato, nuova corrente pubblicizzata, eFuse in guasto (sovratensione / sovracorrente) |
| 0x83 | RELAY_CHANGED | relè, nuovo stato della linea (fine di un impulso, pulsante fisico) |

## CLI (host/)

```
devhub info
devhub status [--json]
devhub port <n> on|off
devhub port <n> cycle [--ms 500] [--on-delay 50]
devhub all on|off
devhub relay <n> connect|disconnect
devhub relay <n> cycle [--ms 500]   # stacca la linea per N ms
devhub defaults show|set ...|save
devhub hub reset
devhub watch            # stampa gli eventi
```

Uscita `--json` per l'uso negli script di test. Codice di uscita ≠ 0 su errore o su fault.
