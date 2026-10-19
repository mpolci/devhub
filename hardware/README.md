# hardware

Progetto KiCad 10 (schema, PCB, BOM) dell'hub.

```
hardware/devhub/        progetto KiCad: devhub.kicad_pro, devhub.kicad_sch (radice), i fogli e devhub.kicad_pcb
hardware/lib/           devhub.kicad_sym: simboli non presenti nelle librerie standard (USB2517, PI3USB4002A, TPS259470A, UCS2114) e SMAJ20A con pin K/A espliciti
hardware/bom/           BOM e file per l'assemblaggio (da fare)
```

## Stato: schema v0.3

Lo schema applica i [requisiti semplificati](../docs/requisiti.md) e le decisioni D60–D90
(dettaglio in D79): quattro porte USB-C, un relè, cinque pulsanti, un LED per porta, LED del relè e di host. Il PCB
da 100 × 100 mm ha il piazzamento iniziale ed è da sbrogliare (vedi sotto).

Simboli delle librerie standard di KiCad più quelli di progetto in `lib/`; fogli gerarchici riusati per le
porte. Componenti vicini collegati a filo, ma ogni rete ha comunque un nome esplicito: etichette
locali per le reti interne a un foglio, globali per quelle che passano tra fogli, gerarchiche per
le porte. Passa l'ERC di `kicad-cli` con zero violazioni.

Fogli:

| Foglio | Contenuto |
|---|---|
| `power_in` | USB-C di alimentazione a 5 V, Rd 5,1 kΩ su CC1/CC2 senza lettura, TVS, eFuse TPS259470A (U13), bulk, Schottky D2, LDO 3,3 V (AP7361C-33E, SOT-223) |
| `hub` | USB2517, USB-C upstream con ESD, quarzo, RBIAS, supervisore di reset (U12) in wired-OR con il MCU, strap di configurazione, VBUS_DET; DS5, DS7 e OCS5_N non collegati. Switch VBUS: 2 × UCS2114-1-V/LX (U4 porte 1-2 su I2C1, U5 porte 3-4 su I2C2), strap COMM_ILIM 27 kΩ, disaccoppiamenti, pull-up degli ALERT# |
| `mcu` | STM32G0B1CEU6 (UFQFPN48), decoupling, SWD (J4) e UART (J9) collegati a filo, pull-up dei due bus I²C, pull-down degli EN porta, pull-down dei DBCC dell'UCPD, LED di host collegato |
| `port_c` ×4 | connettore, switch dati PI3USB4002A (con protezione ESD integrata), condensatori VBUS, Rp 56 kΩ, LED verde dal MCU, pulsante |
| `relays` | ULN2003V12 (3 canali in parallelo), G5Q-1, ponticello NC/NO, boccole Cliff FCR7350, snubber DNP, pulsante SW1, LED del relè D90 |

Piedinatura del MCU: [../docs/pinout-mcu.md](../docs/pinout-mcu.md).

Connettori USB decisi (D18): GCT USB4105-GF-A per tutti gli USB-C.

Quarzo deciso (D19): YXC X322524MOB4SI, C_L 12 pF, C22/C23 da 15 pF.

Footprint locali assegnati dalla libreria `lcsc_footprints` (`devhub/libs/footprints.pretty`):
- U13 TPS259470A: `C3662799_VQFN-10_L2_0-W2_0-P0_45-TL`;
- U101–U401 PI3USB4002A: `C3657361_UQFN-10_L2_0-W1_5-P0_50-TL`;
- U4, U5 UCS2114: `UCS2114_VQFN-20_3x3mm_P0.5mm`;
- U2 USB2517: `QFN-64-1EP_9x9mm_P0.5mm_EP4.7x4.7mm_ThermalVias_0.3mm` (D50).

Le piazzole di U13 e U101–U401 sono quelle generate da LCSC, non copiate dai land pattern dei
costruttori: nel PI3USB4002A le laterali sono 0,665 × 0,28 mm invece di 0,53 × 0,25 (più lunghe
verso l'esterno, stesso bordo interno), nel TPS259470A le coordinate coincidono con l'esempio TI
RPW0010A entro ~0,05 mm. Differenze accettate: il bordo esterno più lungo aiuta la saldatura ad
aria calda. Courtyard ridisegnati a 0,25 mm dalle piazzole (quelli importati erano più piccoli
del rame).

L'impronta dell'UCS2114 segue il land pattern Microchip C04-2421 Rev C (datasheet
DS20005743B): 20 piazzole 0,25 × 0,5 mm su C1 = C2 = 2,9 mm, otto angoli smussati di 0,1 mm,
area termica 2,00 × 1,13 mm coperta da solder mask e collegata al pin 17 (GND), con 6 via
0,3/0,5 mm. Il package non ha pad esposto. Modello 3D: quello di KiCad per UFQFPN-20 3 × 3 mm,
come segnaposto (stesse dimensioni in pianta, altezza diversa).

J4 (SWD, Samtec FTSH-105-01-L-DV-K) usa l'impronta generica
`PinHeader_2x05_P1.27mm_Vertical_SMD`: KiCad non ha quella Samtec, e rispetto al land pattern
Samtec le piazzole sono più corte (2,40 invece di 2,79 mm, ingombro 6,30 invece di 6,86 mm), con
stesso passo e stessa larghezza. Accettata: è un connettore di debug saldato a mano.

I footprint usano i modelli in `devhub/libs/3dmodels` con percorsi relativi al progetto.
Il modello C3657361 è un segnaposto dell'importatore, non una rappresentazione del package.

BOM: i simboli hanno i campi `warehouse` (ID del magazzino), `lcsc`, `mouser` e `digikey`;
confronto dei costi della v0.2 in [../docs/confronto-costi.md](../docs/confronto-costi.md). Su
Mouser mancano le boccole Cliff (campo vuoto). Da completare: U4/U5 (nessun codice fornitore),
U3 su Mouser e LCSC, R60/R61 su LCSC.

Per l'ERC da riga di comando:

```bash
/Applications/KiCad/KiCad.app/Contents/MacOS/kicad-cli sch erc --severity-all --output /tmp/erc.rpt hardware/devhub/devhub.kicad_sch
```

## PCB

Regole di progettazione: [stackup, routing e verifiche PCB](../docs/regole-pcb.md), configurate
nel progetto: stackup nel PCB, vincoli e netclass in `devhub.kicad_pro`, regole personalizzate in
`devhub.kicad_dru`.

`devhub.kicad_pcb`: 4 strati, profilo **100 × 100 mm** con angoli R3 (D84), allineato allo schema
v0.3 (parità senza differenze). Vista dall'alto, fronte (lato banco) in basso; u dal bordo
sinistro, v dal bordo frontale. Piazzamento iniziale fatto, sbroglio in corso.

| Cosa | Faccia | Posizione (u, v in mm) |
|---|---|---|
| J5 COM, J6 NC boccole | inferiore | u = 10 e 27; corpo a v = 11,85 come prima, manicotto a filo della faccia esterna della parete |
| J401, J301, J201, J101 USB-C (porte 4…1) | inferiore | u = 46, 61, 76, 91; faccia d'innesto a filo del bordo frontale |
| LED delle porte D402…D102 (verdi) | superiore | in asse con la porta, v = 2 |
| D90 LED relè | superiore | u = 18,5, v = 2, sopra la coppia di boccole (D90) |
| D93 LED host | superiore | u = 32,5, v = 98, sul retro accanto a J1 (D90) |
| SW1, SW401…SW101 pulsanti | superiore | v = 14,5 (D87); SW1 al centro delle boccole (u = 18,5), gli altri in asse con le porte |
| J1 alimentazione, J2 upstream | inferiore | u = 24 e 82, a filo del bordo posteriore |
| H1–H4 fori M3 | — | H1 (4,5; 58) dietro il relè K1, H2 (95,5; 28), H3 (4,5; 95,5), H4 (95,5; 95,5) |

Gruppi funzionali sulla faccia inferiore, traslati dal piazzamento v0.2 con le rispettive aree
di regola: porte con switch dati e condensatori dietro i connettori; U4/U5 UCS2114 dietro i
pulsanti, fra le porte che servono; relè K1, driver U9 e ponticello a sinistra dietro le boccole;
MCU al centro; hub con quarzo e disaccoppiamenti al centro-destra; alimentazione (eFuse, LDO,
bulk) dietro a sinistra; upstream con ESD e VBUS_DET dietro a destra; J4 (SWD) e J9 (UART) sul
retro, fra J1 e J2.

Sbroglio completo. Piani: GND su In2.Cu (`GND_In2`), +3V3 su In1.Cu (`3V3_In1`) con isole VBUS
sotto i connettori e una zona +5V lungo il bordo frontale verso U4/U5 (D93). Il PCB va
modificato in KiCad dallo stato corrente, non rigenerato.

Per il controllo di parità con lo schema da riga di comando:

```bash
/Applications/KiCad/KiCad.app/Contents/MacOS/kicad-cli pcb drc --schematic-parity --severity-all --output /tmp/drc.rpt hardware/devhub/devhub.kicad_pcb
```

I datasheet dei componenti sono in `../datasheets/` (fuori dal repository: non ridistribuibili).
Da scaricare a mano dal sito ST, che blocca i download automatici: datasheet STM32G0B1, reference
manual RM0444, datasheet USBLC6-2.

Licenza: CERN-OHL-W 2.0, vedi [LICENSE](LICENSE).
