# hardware

Progetto KiCad 10 (schema, PCB, BOM) dell'hub.

```
hardware/devhub/        progetto KiCad: devhub.kicad_pro, devhub.kicad_sch (radice) e i fogli
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
| `power_in` | USB-C di alimentazione a 5 V, Rd e partitori CC, TVS, shunt d'ingresso, eFuse TPS259470A (U13), bulk, Schottky D2, LDO 3,3 V (AP7361C-33E, SOT-223) |
| `hub` | USB2517, USB-C upstream con ESD, quarzo, RBIAS, supervisore di reset (U12) in wired-OR con il MCU, strap di configurazione, header porta 7 |
| `mcu` | STM32G0B1RET6, decoupling, SWD (J4) e UART (J9) collegati a filo, pull-up I2C, INA_ALERT e LED_OE_N, pull-down EN |
| `switches` | 3 × TPS2561A con R_ILIM, shunt 20 mΩ per porta, 2 × INA3221 |
| `port_c` ×4, `port_a` ×1 | connettore, switch dati PI3USB4002A (con protezione ESD integrata), scarica VBUS, condensatori (sulla USB-A anche 100 µF), LED, pulsante |
| `relays` | ULN2003V12 (3 canali in parallelo per relè), 2 × G5Q-1, ponticelli NC/NO, boccole Cliff FCR7350, snubber DNP, pulsanti SW1/SW2 |
| `ui` | 2 × 74HC595 (/OE dal MCU, pull-up), LED relè / alimentatore / host |

Piedinatura del MCU: [../docs/pinout-mcu.md](../docs/pinout-mcu.md).

Connettori USB decisi (D18): GCT USB4105-GF-A per tutti gli USB-C, Kycon KUSBX-AS1N-B per
l'USB-A; i footprint nello schema sono già questi.

Quarzo deciso (D19): YXC X322524MOB4SI, C_L 12 pF, C22/C23 da 15 pF.

Da fare prima del PCB:
- footprint di U13 (TPS259470A, VQFN-HR-10 RPW0010A: nella libreria KiCad non c'è, va creato dal
  disegno del datasheet TI) e degli switch PI3USB4002A (UQFN-10 ZUA 1,5×2 mm: candidato
  `Package_DFN_QFN:Texas_UQFN-10_1.5x2mm_P0.5mm`, da verificare sul disegno Diodes);
- BOM.

Per l'ERC da riga di comando:

```bash
/Applications/KiCad/KiCad.app/Contents/MacOS/kicad-cli sch erc --severity-all --output /tmp/erc.rpt hardware/devhub/devhub.kicad_sch
```

I datasheet dei componenti sono in `../datasheets/` (fuori dal repository: non ridistribuibili).
Da scaricare a mano dal sito ST, che blocca i download automatici: datasheet STM32G0B1RE,
reference manual RM0444, datasheet USBLC6-2.

Licenza: CERN-OHL-W 2.0, vedi [LICENSE](LICENSE).
