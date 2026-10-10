# Istruzioni per gli agenti

## Schema elettrico (KiCad)

- Non rigenerare i fogli da script, una rigenerazione cancella le modifiche fatte a mano in KiCad. 
  I fogli vanno modificati sempre rispetto allo stato corrente.
- Dopo ogni modifica: ERC con `kicad-cli` senza violazioni e confronto della netlist con quella
  precedente, per essere sicuri che cambi solo quello che si voleva cambiare.
- Ogni rete deve avere un nome esplicito (etichetta o simbolo di alimentazione), anche quando i
  componenti sono collegati a filo. Etichette locali per le reti interne a un foglio, globali per
  quelle che passano tra fogli, gerarchiche per i fogli porta.

## Documentazione

- Decisioni in `docs/domande-aperte.md` (tabella "Decise", numerazione progressiva).
- Componenti scelti e alternative valutate per fornitore in `docs/componenti.md`.
- Piedinatura del MCU in `docs/pinout-mcu.md`: va tenuta allineata allo schema.
- I datasheet sono in `datasheets/` (escluso da git).
