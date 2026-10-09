# devhub — hub USB "intelligente" per lo sviluppo hardware

Hub USB da banco pensato per chi sviluppa dispositivi a microcontrollore collegati via USB.
Oltre alle porte downstream espone all'host un **dispositivo di controllo** (USB HID) con cui
il PC può:

- **staccare e riattaccare fisicamente** ogni porta (alimentazione VBUS e linee dati D+/D-),
  per resettare un dispositivo bloccato anche quando il reset via USB non funziona;
- **leggere lo stato** di ogni porta (abilitata/disabilitata, sovracorrente, corrente assorbita);
- **comandare due relè indipendenti** con uscite su boccole a banana (es. per sezionare
  l'alimentazione esterna del dispositivo in prova o chiudere un contatto di reset).

## Caratteristiche previste (v1)

| Voce | Scelta v1 |
|---|---|
| Porte downstream | 4 × USB-C + 1 × USB-A |
| Velocità | USB 2.0 Hi-Speed (480 Mbit/s) |
| Alimentazione per porta | 5 V, ≥ 2 A, con limitazione di corrente e sezionamento per porta |
| Sezionamento | VBUS (load switch) **e** D+/D- (switch analogico), comandati separatamente |
| Alimentazione hub | ingresso USB-C con negoziazione USB-PD (20 V) e buck a 5 V |
| Upstream (verso il PC) | USB-C (USB 2.0) |
| Controllo | MCU su porta interna dell'hub, interfaccia USB HID senza driver + CLI Python |
| Relè ausiliari | 2 × SPDT su boccole a banana |

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
- [docs/protocollo-controllo.md](docs/protocollo-controllo.md) — bozza del protocollo HID tra PC e hub
- [docs/domande-aperte.md](docs/domande-aperte.md) — decisioni ancora da prendere

## Stato

Fase di definizione: requisiti e architettura. Nessun hardware ancora disegnato.
