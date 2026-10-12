# hardware

Progetto KiCad 10 (schema, PCB, BOM) dell'hub.

```
hardware/devhub/        progetto KiCad: devhub.kicad_pro, devhub.kicad_sch (radice), i fogli e devhub.kicad_pcb
hardware/lib/           devhub.kicad_sym: simboli non presenti nelle librerie standard (USB2517, PI3USB4002A, TPS259470A)
hardware/bom/           BOM e file per l'assemblaggio (da fare)
```

## Stato: schema v0.2

Simboli delle librerie standard di KiCad più il simbolo USB2517; fogli gerarchici riusati per le
porte. Componenti vicini collegati a filo, ma ogni rete ha comunque un nome esplicito: etichette
locali per le reti interne a un foglio, globali per quelle che passano tra fogli, gerarchiche per
le porte. Passa l'ERC di `kicad-cli` con zero violazioni.

Fogli:

| Foglio | Contenuto |
|---|---|
| `power_in` | USB-C di alimentazione a 5 V, Rd e partitori CC, TVS, eFuse TPS259470A (U13), shunt della corrente totale, bulk, Schottky D2, LDO 3,3 V (AP7361C-33E, SOT-223) |
| `hub` | USB2517, USB-C upstream con ESD, quarzo, RBIAS, supervisore di reset (U12) in wired-OR con il MCU, strap di configurazione, header porta 7 |
| `mcu` | STM32G0B1RET6, decoupling, SWD (J4) e UART (J9) collegati a filo, pull-up I2C e INA_ALERT, pull-down EN, pull-down dei DBCC dell'UCPD |
| `switches` | 3 × TPS2561A con R_ILIM, shunt 20 mΩ per porta, 2 × INA3221 |
| `port_c` ×4, `port_a` ×1 | connettore, switch dati PI3USB4002A (con protezione ESD integrata), scarica VBUS, condensatori (sulla USB-A anche 100 µF), LED (verde dall'EN, rosso dal MCU), pulsante |
| `relays` | ULN2003V12 (3 canali in parallelo per relè), 2 × G5Q-1, ponticelli NC/NO, boccole Cliff FCR7350, snubber DNP, pulsanti SW1/SW2 |
| `ui` | LED dei relè (dai comandi RLY), acceso (3,3 V), host collegato (dal MCU) |

Piedinatura del MCU: [../docs/pinout-mcu.md](../docs/pinout-mcu.md).

Connettori USB decisi (D18): GCT USB4105-GF-A per tutti gli USB-C, Kycon KUSBX-AS1N-B per
l'USB-A; i footprint nello schema sono già questi.

Quarzo deciso (D19): YXC X322524MOB4SI, C_L 12 pF, C22/C23 da 15 pF.

Footprint locali assegnati dalla libreria `lcsc_footprints` (`devhub/libs/footprints.pretty`):
- U13 TPS259470A: `C3662799_VQFN-10_L2_0-W2_0-P0_45-TL`;
- U101–U501 PI3USB4002A: `C3657361_UQFN-10_L2_0-W1_5-P0_50-TL`.

I footprint usano i modelli in `devhub/libs/3dmodels` con percorsi relativi al progetto.
Il modello C3657361 è un segnaposto dell’importatore, non una rappresentazione del package.

BOM: i simboli hanno i campi `warehouse` (ID del magazzino), `lcsc`, `mouser` e `digikey`;
confronto dei costi in [../docs/confronto-costi.md](../docs/confronto-costi.md). Su Mouser
mancano le boccole Cliff (campo vuoto).

## PCB (iniziato)

Regole di progettazione: [stackup, routing e verifiche PCB](../docs/regole-pcb.md), configurate
nel progetto: stackup nel PCB, vincoli e netclass in `devhub.kicad_pro`, regole personalizzate in
`devhub.kicad_dru`.

`devhub.kicad_pcb`: 4 strati, profilo 169 × 85 mm con angoli R3 e piazzati, bloccati, i
componenti che vincolano la scatola (D42, D43), con il profilo al centro del foglio A4. Vista
dall'alto, fronte (lato banco) in basso. Il bordo
del PCB sta sulla faccia interna delle pareti (spessore previsto 2,5 mm).

| Cosa | Faccia | Posizione (u dal bordo sinistro, v dal bordo frontale) |
|---|---|---|
| J101-J401 USB-C, J501 USB-A | inferiore | u = 16, 31, 46, 61, 81 mm; faccia d'innesto a filo del bordo frontale |
| J5-J8 boccole (COM1, NC1, COM2, NC2) | inferiore | u = 101, 118, 138, 155 mm; corpo a v = 11,85 mm, manicotto Ø 11 × 14,35 mm fino a v = -2,5 mm (a filo della faccia esterna della parete) |
| LED delle porte (verde D*02, rosso D*03) | superiore | in colonna sull'asse della porta: verde a v = 2 mm, rosso a v = 6 mm |
| D90, D91 LED dei relè (verdi) | superiore | v = 2 mm, al centro della coppia di boccole (u = 109,5 e 146,5 mm) |
| SW101-SW501, SW1, SW2 pulsanti SMD PTS645 (6×6, attuatore 7 mm) | superiore | v = 36 mm (centro), in asse con porte e coppie di boccole |
| D92 acceso, D93 host | superiore | in colonna a u = 162 mm, v = 34 e 38 mm (centrati sulla fila dei pulsanti) |
| K1, K2 relè (il componente più alto sotto) | inferiore | dietro i pulsanti dei relè, v = 40-52 mm |
| J2 upstream, J1 alimentazione | inferiore | u = 40 e 110 mm, a filo del bordo posteriore |
| H1-H4 fori M3 (solo PCB, non nello schema) | — | a 4,5 mm dai bordi |

Gli altri componenti sono già sulla faccia inferiore, fuori dal profilo a destra, raggruppati per
foglio. Il PCB è allineato allo schema (controllo di parità senza differenze), con i footprint
di U2 (via termiche da 0,30, D50), U13 e U101-U501 dalla libreria di progetto
`libs/footprints.pretty` e i package da magazzino di C3, C4, C505, F1 e C106-C506 (D45). Il PCB va modificato in KiCad dallo stato corrente,
non rigenerato.

Per il controllo di parità con lo schema da riga di comando:

```bash
/Applications/KiCad/KiCad.app/Contents/MacOS/kicad-cli pcb drc --schematic-parity --severity-all --output /tmp/drc.rpt hardware/devhub/devhub.kicad_pcb
```

Nel PCB: collegamenti Kelvin sugli shunt di misura (R5 e i 5 shunt delle porte), con le piste
verso l'INA3221 che partono dalle piazzole dello shunt (D36).

Per l'ERC da riga di comando:

```bash
/Applications/KiCad/KiCad.app/Contents/MacOS/kicad-cli sch erc --severity-all --output /tmp/erc.rpt hardware/devhub/devhub.kicad_sch
```

I datasheet dei componenti sono in `../datasheets/` (fuori dal repository: non ridistribuibili).
Da scaricare a mano dal sito ST, che blocca i download automatici: datasheet STM32G0B1RE,
reference manual RM0444, datasheet USBLC6-2.

Licenza: CERN-OHL-W 2.0, vedi [LICENSE](LICENSE).
