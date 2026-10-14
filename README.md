# devhub — hub USB "intelligente" per lo sviluppo hardware

Hub USB da banco pensato per chi sviluppa dispositivi a microcontrollore collegati via USB.
Oltre alle porte downstream espone all'host un **dispositivo di controllo** (USB HID) con cui
il PC può:

- **staccare e riattaccare fisicamente** ogni porta (alimentazione VBUS e linee dati D+/D-),
  per resettare un dispositivo bloccato anche quando il reset via USB non funziona;
- **leggere lo stato** di ogni porta (abilitata/disabilitata, ciclo in corso, guasti hardware);
- **comandare due relè indipendenti** con uscite su boccole a banana (es. per sezionare
  l'alimentazione esterna del dispositivo in prova o chiudere un contatto di reset).

## Caratteristiche previste (v1)

| Voce | Scelta v1 |
|---|---|
| Porte downstream | 4 × USB-C + 1 × USB-A |
| Velocità | USB 2.0 Hi-Speed (480 Mbit/s) |
| Alimentazione per porta | 5 V, fino a 2 A sulla singola porta USB-C e 1,5 A sulla USB-A, ~2,3 A totali, con limitazione di corrente e sezionamento per porta |
| Sezionamento | VBUS **e** D+/D− con unico comando logico; ammessa la sequenza interna del controller |
| Riavvio | Ciclo temporizzato con scarica attiva, durata configurabile; default indicativo 1 s da validare, nessuna misura di corrente o tensione |
| Indicazione porta | Un LED dal MCU: spento, acceso fisso, lampeggiante per guasto |
| Alimentazione hub | richiesti alimentatore USB-C 5 V / 3 A senza negoziazione PD e cavo adeguato; nessuna verifica delle CC |
| Upstream (verso il PC) | USB-C (USB 2.0) |
| Controllo | STM32G0B1 su porta interna dell'hub, interfaccia USB HID senza driver + CLI Python |
| Relè ausiliari | 2 × SPDT su boccole a banana, contatti max 16 V / 3 A |

I 3 A d'ingresso comprendono anche i consumi interni: un sovraccarico complessivo può
interrompere l'intero hub. Il ciclo non verifica elettricamente lo spegnimento e riguarda
l'alimentazione fornita dalla porta; per riavviare schede autoalimentate servono relè o reset.

Le motivazioni delle scelte (in particolare USB 2.0 invece di USB 3 e il non sezionare GND)
sono in [docs/architettura.md](docs/architettura.md).

## Struttura del repository

```
docs/        requisiti, architettura, protocollo di controllo, domande aperte
hardware/    progetto KiCad (schema, PCB, BOM)
firmware/    firmware del MCU di controllo
host/        strumento da riga di comando per il PC
```

## Documenti

- [docs/requisiti.md](docs/requisiti.md) — cosa deve fare il dispositivo
- [docs/architettura.md](docs/architettura.md) — schema a blocchi, scelte progettuali, componenti candidati, bilancio di potenza
- [docs/componenti.md](docs/componenti.md) — componenti candidati, package, fornitori, classe JLCPCB
- [docs/protocollo-controllo.md](docs/protocollo-controllo.md) — bozza del protocollo HID tra PC e hub
- [docs/regole-pcb.md](docs/regole-pcb.md) — stackup, piste, USB, alimentazione, piazzamento e verifiche di produzione
- [docs/pinout-mcu.md](docs/pinout-mcu.md) — piedinatura del MCU e note per il firmware
- [docs/domande-aperte.md](docs/domande-aperte.md) — decisioni ancora da prendere

## Licenze

- **Hardware** (`hardware/`): [CERN-OHL-W 2.0](hardware/LICENSE).
- **Firmware, libreria e CLI, documentazione** (tutto il resto): [MIT](LICENSE).

## Stato

Requisiti semplificati definiti (D60–D65); schema elettrico v0.2 e PCB iniziato descrivono
ancora l'implementazione da adeguare (vedi [hardware/README.md](hardware/README.md)).

Prossimo passo: scegliere il controller delle porte e adeguare schema, PCB e BOM ai nuovi requisiti.
