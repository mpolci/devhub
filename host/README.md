# host

Lato PC, in Python: libreria `devhub` e comando `devhub`, sopra `hidapi`. Protocollo in
[../docs/protocollo-controllo.md](../docs/protocollo-controllo.md), versione 0.1.

**Stato:** libreria e CLI coprono tutti i comandi e gli eventi del protocollo 0.1. Sono
provate con un trasporto simulato e sul prototipo Rev A, dove passano i test `pytest -m board`.

## Avvio

Serve solo `python3`. Lo script `bin/devhub` lancia la CLI direttamente dal repository:

```
host/bin/devhub status
```

Al primo avvio crea `host/.venv` e ci installa il pacchetto con le dipendenze; dopo parte
subito. Per averlo come comando `devhub` basta un link in una cartella del PATH:

```
ln -s "$PWD/host/bin/devhub" ~/.local/bin/devhub
```

Per sviluppare e lanciare i test, dalla cartella `host/`:

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
devhub port <n>|@testo on|off|cycle [--ms 1000]|clear|reset-charge [--wait [--timeout 10]]
devhub find <testo>
devhub log [--interval 1000] [--duration secondi]
devhub all on|off
devhub relay <n> connect|disconnect|cycle [--ms 500]
devhub defaults show|set [porte...]|charge-reset [porte...]|save
devhub hub reset
devhub watch [--json]
devhub reboot
devhub bootloader
devhub help [comando]
```

`devhub` senza argomenti stampa una guida breve con i comandi ed esempi; `devhub help` la
guida completa, `devhub help <comando>` i dettagli di un comando.

`status` mostra corrente e carica erogata di ogni porta (D103) e la temperatura del chip del MCU
(D102). Su macOS e Linux mostra accanto a ogni porta anche il dispositivo collegato, come lo
vede il PC: produttore e prodotto, VID:PID e porte seriali. Per esempio:

```
port 1: on        133 mA     1.00 mAh  Espressif USB JTAG/serial debug unit [303a:1001] /dev/cu.usbmodem212101
port 2: on         13 mA     0.42 mAh  -
port 3: off         0 mA     0.38 mAh
port 4: on         53 mA     4.41 mAh  SanDisk Cruzer Edge [0781:556b]
```

`devhub find <testo>` elenca le porte il cui dispositivo corrisponde al testo (produttore,
prodotto, numero di serie, VID:PID o porta seriale); `@testo` al posto del numero sceglie la
porta allo stesso modo, per esempio `devhub port @esp32 cycle`, e deve corrispondere a una
sola porta.

`devhub log` stampa in CSV, a intervalli fissi, temperatura e per ogni porta stato, corrente
e carica: `devhub log --interval 100 --duration 60 > prova.csv`.

`port <n> on|cycle --wait` torna solo quando sulla porta c'è un dispositivo enumerato dal PC
(dopo un ciclo, la nuova enumerazione) e lo stampa con le sue porte seriali; codice di uscita 1
se non compare entro `--timeout` secondi.

Il firmware non vede cosa è collegato alle porte: la libreria cerca nell'albero USB del
sistema (`ioreg` su macOS, `/sys/bus/usb/devices` su Linux) il hub del devhub, che ha lo
stesso numero di serie del dispositivo di controllo, e i dispositivi collegati alle sue porte
1–4. Dalla libreria: `attached_devices(serial)`. Su Windows non è ancora disponibile.

Su macOS un solo programma alla volta può aprire il dispositivo: con `devhub watch` in
esecuzione gli altri comandi non riescono ad aprirlo.

`-s <seriale>` sceglie il dispositivo. Codice di uscita 0 se tutto va bene, 1 su errore,
2 quando `status` trova una porta bloccata per guasto.

## Test

```
.venv/bin/python -m pytest            # codifica del protocollo e libreria, senza dispositivo
.venv/bin/python -m pytest -m board   # sul dispositivo collegato: accende e spegne porte e relè
```
