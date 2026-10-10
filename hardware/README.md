# hardware

Progetto KiCad (schema, PCB, BOM) dell'hub. Non ancora iniziato: prima vanno chiuse le domande in
[../docs/domande-aperte.md](../docs/domande-aperte.md).

Struttura prevista:

```
hardware/devhub/        progetto KiCad
hardware/lib/           simboli e footprint non presenti nelle librerie standard
hardware/bom/           BOM e file per l'assemblaggio
```

I datasheet dei componenti sono in `../datasheets/` (fuori dal repository: non ridistribuibili).
Da scaricare a mano dal sito ST, che blocca i download automatici: datasheet STM32G0B1RE,
reference manual RM0444, datasheet USBLC6-2.

Licenza: CERN-OHL-W 2.0, vedi [LICENSE](LICENSE).
