# hardware

Progetto KiCad 10 (schema, PCB, BOM) dell'hub.

```
hardware/devhub/        progetto KiCad: devhub.kicad_pro, devhub.kicad_sch (radice) e i fogli
hardware/lib/           devhub.kicad_sym: simboli non presenti nelle librerie standard (USB2517)
hardware/bom/           BOM e file per l'assemblaggio (da fare)
```

## Stato: schema v0.1

Lo schema iniziale è stato generato da script (poi rimossi): simboli delle librerie standard di
KiCad più il simbolo USB2517, collegamenti con etichette globali e gerarchiche su ogni pin, fogli
gerarchici riusati per le porte. Passa l'ERC di `kicad-cli` con zero violazioni. È un punto di
partenza da rifinire in KiCad (disposizione, fili al posto delle etichette dove utile).

Fogli:

| Foglio | Contenuto |
|---|---|
| `power_in` | USB-C di alimentazione a 5 V, Rd e partitori CC, TVS, shunt d'ingresso, bulk, ORing Schottky, LDO 3,3 V (AP2112K) |
| `hub` | USB2517, USB-C upstream con ESD, quarzo, RBIAS, reset dal MCU, strap di configurazione, header porta 7 |
| `mcu` | STM32G0B1RET6, decoupling, SWD, UART, pull-up I2C, pull-down EN, pull-up INA_ALERT |
| `switches` | 3 × TPS2561A con R_ILIM, shunt 20 mΩ per porta, 2 × INA3221 |
| `port_c` ×4, `port_a` ×1 | connettore, USBLC6-2, TS3USB30E, scarica VBUS, condensatori, LED, pulsante |
| `relays` | ULN2003A, 2 × G5Q-1, ponticelli NC/NO, boccole, snubber DNP |
| `ui` | 2 × 74HC595, LED relè / alimentatore / host |

Piedinatura del MCU: [../docs/pinout-mcu.md](../docs/pinout-mcu.md).

Da fare prima del PCB: footprint delle boccole a banana (J5-J8), scelta definitiva dei
connettori USB-C e USB-A (footprint GCT USB4105 e Kycon KUSBX come segnaposto), verifica dei
valori dei condensatori del quarzo, BOM.

Per l'ERC da riga di comando:

```bash
/Applications/KiCad/KiCad.app/Contents/MacOS/kicad-cli sch erc --severity-all --output /tmp/erc.rpt hardware/devhub/devhub.kicad_sch
```

I datasheet dei componenti sono in `../datasheets/` (fuori dal repository: non ridistribuibili).
Da scaricare a mano dal sito ST, che blocca i download automatici: datasheet STM32G0B1RE,
reference manual RM0444, datasheet USBLC6-2.

Licenza: CERN-OHL-W 2.0, vedi [LICENSE](LICENSE).
