# host

Lato PC, in Python: libreria `devhub` e comando `devhub`, sopra `hidapi`. Protocollo in
[../docs/protocollo-controllo.md](../docs/protocollo-controllo.md), versione 0.1.

**Stato:** libreria e CLI di base scritte e provate con un trasporto simulato; non ancora
provate con il dispositivo reale.

## Installazione

```
python3 -m venv .venv
.venv/bin/pip install -e ".[test]"
```

## Libreria

```python
from devhub import Devhub

with Devhub() as hub:              # Devhub("numero di serie") se ce n'è più d'uno
    print(hub.info())
    hub.cycle_port(2, off_ms=1000)  # il ciclo prosegue nel dispositivo
    for event in hub.events(timeout=3):
        print(event.to_dict())
    print(hub.status().to_dict())
```

Porte e relè sono numerati da 1. I comandi rifiutati dal dispositivo sollevano
`CommandError`, i problemi di comunicazione `DevhubError`.

## CLI

```
devhub list
devhub info
devhub status [--json]
devhub port <n> on|off|cycle [--ms 1000]|clear
devhub all on|off
devhub relay <n> connect|disconnect|cycle [--ms 500]
devhub defaults show|set [porte...]|save
devhub hub reset
devhub watch [--json]
devhub reboot
devhub bootloader
```

`-s <seriale>` sceglie il dispositivo. Codice di uscita 0 se tutto va bene, 1 su errore,
2 quando `status` trova una porta bloccata per guasto.

## Test

```
.venv/bin/python -m pytest            # codifica del protocollo e libreria, senza dispositivo
.venv/bin/python -m pytest -m board   # sul dispositivo collegato: accende e spegne porte e relè
```
