# Confronto dei costi della BOM

Prezzi letti il 12/10/2026 sui siti dei fornitori e, per 25 righe Mouser, dal carrello Mouser
della stessa data, IVA, dazi e spedizione esclusi, da
ricontrollare all'ordine. Tutto in euro: LCSC (USD) e Mouser (il sito risponde dalla Svizzera, in
CHF) convertiti al cambio BCE del 12/10/2026, 1 € = 1,1181 USD = 0,9312 CHF.

I codici sono anche nei campi `warehouse`, `lcsc`, `mouser` e `digikey` dei simboli dello schema.

## Come leggere la griglia

- Costo di riga per **una scheda**: quantità montata (DNP esclusi) per il prezzo dello scaglione
  corrispondente. Se il fornitore impone un minimo d'ordine o il primo scaglione è più alto, il
  costo è quello della quantità minima, indicata con "×N" (tipico delle resistenze LCSC, a 100 pezzi).
- **Magazzino**: ID della riga nel foglio del magazzino, solo per articoli comprati da fornitori
  affidabili (LCSC, TME, Mouser, DigiKey; esclusi gli acquisti AliExpress). Il codice LCSC
  delle righe a magazzino è quello del pezzo a magazzino, quando è stato comprato da LCSC.
- **MPN** con due codici: il primo è il pezzo a magazzino o su LCSC, il secondo l'equivalente
  usato per Mouser e DigiKey, che non hanno il primo (vedi note sotto).
- **—**: non a catalogo.

## Totali

| Scenario | LCSC | Mouser | DigiKey |
|---|---|---|---|
| 1 scheda, tutto dal fornitore | 88,88 | 62,37 | 90,93 |
| 1 scheda, magazzino + resto dal fornitore | 72,46 | 48,78 | 77,62 |
| 1 scheda, solo le righe disponibili su Mouser | 67,50 | 62,37 | 64,01 |
| 10 schede, tutto dal fornitore | 646,52 | 539,10 | 669,78 |
| 10 schede, solo le righe disponibili su Mouser | 432,70 | 539,10 | 441,18 |

Mouser non ha le boccole Cliff, LCSC non ha le nere: nessun fornitore oltre a DigiKey copre
tutta la BOM. Sulle righe che Mouser ha, per una scheda Mouser costa ~62 €, DigiKey ~64 € e
LCSC ~68 €, perché i minimi d'ordine dei passivi (100 resistenze per valore)
annullano il vantaggio sui circuiti integrati e sui relè. Le boccole sono la voce più cara: su
DigiKey 26,92 € per una scheda e ~229 € per dieci; su LCSC le sole rosse costano 11,95 $ al pezzo.
Per tutta la BOM da un unico fornitore resta solo DigiKey (90,93 € per una scheda, 77,62 € usando
il magazzino). Le righe a magazzino valgono ~13 € a scheda ai prezzi DigiKey.
Il totale Mouser per 10 schede è sovrastimato: per le 25 righe prese dal carrello si conosce solo
il prezzo alla quantità di una scheda, senza gli sconti di quantità.
Spedizione: DigiKey Italia gratis sopra 75 €; il carrello Mouser da 33,98 CHF ne chiedeva
23,00 CHF; LCSC da verificare all'ordine.

## Griglia

| # | Riferimenti | Q.tà | Componente | MPN | Magazzino | LCSC | € LCSC | Mouser | € Mouser | DigiKey | € DigiKey |
|---|---|---|---|---|---|---|---|---|---|---|---|
| 1 | C1 … (41) | 40 | 100 nF 50 V X7R 0603 | CL10B104KB8NNNC | **235** | C1591 | 0,73 ×100 | 187-CL10B104KB8NNNC | 1,42 | 1276-1000-1-ND | 1,12 |
| 2 | C3, C4 | 2 | 220 µF 25 V elettrolitico radiale D6,3 P2,5 | EGT227M1EE11RRS0P / EEU-FR1A221 | **60** | C238354 | 0,47 ×10 | 667-EEU-FR1A221 | 0,83 | P14375-ND | 0,84 |
| 3 | C5, C6, C8, C9 | 4 | 10 µF 50 V X5R 0805 | GRM21BR61H106KE43L | **192** | C440198 | 0,93 ×5 | 81-GRM21BR61H106KE3L | 1,55 | 490-18663-1-ND | 1,16 |
| 4 | C11 | 1 | 2,2 nF 50 V X7R 0603 | CL10B222KB8NNNC |  | C16033 | 0,45 ×50 | 187-CL10B222KB8NNNC | 0,13 | 1276-1110-1-ND | 0,09 |
| 5 | C12 | 1 | 10 nF 50 V X7R 0603 | CL10B103KB8NNNC | **234** | C1589 | 0,43 ×50 | 187-CL10B103KB8NNNC | 0,09 | 1276-1009-1-ND | 0,09 |
| 6 | C20 | 1 | 4,7 µF 16 V X5R 0805 | CL21A475KOFNNNE | **236** | C1746 | 0,41 ×10 | 187-CL21A475KOFNNNE | 0,16 | 1276-1065-1-ND | 0,12 |
| 7 | C7, C21 … (12) | 12 | 1 µF 25 V X7R 0603 | CL10B105KA8NNNC | **240** | C29936 | 0,21 | 187-CL10B105KA8NNNC | 0,45 | 1276-1184-1-ND | 0,46 |
| 8 | C22, C23 | 2 | 15 pF 50 V C0G 0603 | CL10C150JB8NNNC |  | C1644 | 0,51 ×50 | 187-CL10C150JB8NNNC | 0,17 | 1276-1296-1-ND | 0,18 |
| 9 | C40 | 1 | 4,7 µF 16 V X5R 0603 | CL10A475KO8NNNC | **276** | C19666 | 0,27 ×10 | 187-CL10A475KO8NNNC | 0,17 | 1276-1784-1-ND | 0,12 |
| 10 | C81, C82 | 0 (DNP) | 100 nF 50 V X7R 0805 (DNP) | 0805B104K500CT / CL21B104KBCNNNC | **178** | C83055 | — | 187-CL21B104KBCNNNC | — | 1276-1003-1-ND | — |
| 11 | C102 … (5) | 5 | 22 µF 10 V X7R 0805 | GRM21BZ71A226ME15L | **300** | C907991 | 1,14 | 81-GRM21BZ71A226ME5L | 1,51 | 490-GRM21BZ71A226ME15LCT-ND | 1,55 |
| 12 | C505 | 1 | 100 µF 16 V elettrolitico SMD 6,3×5,8 | EEE-FK1C101P | **184** | C128532 | 1,22 ×5 | 667-EEE-FK1C101P | 0,53 | PCE3783CT-ND | 0,54 |
| 13 | D1 | 1 | TVS SMAJ20A, SMA | SMAJ20A-13-F | **221** | C134969 | 1,03 ×5 | 621-SMAJ20A-13-F | 0,36 | SMAJ20A-FDICT-ND | 0,42 |
| 14 | D2 | 1 | Schottky 40 V 3 A, SMA | B340A-13-F |  | C85098 | 0,73 ×10 | 621-B340A-F | 0,50 | B340A-FDICT-ND | 0,47 |
| 15 | D4 | 1 | Zener 5,1 V SOD-123 | BZT52C5V1-7-F |  | C151588 | 0,56 ×10 | 621-BZT52C5V1-F | 0,18 | BZT52C5V1-FDICT-ND | 0,18 |
| 16 | D20 | 1 | ESD USBLC6-2SC6 | USBLC6-2SC6 | **277** | C7519 | 0,80 ×5 | 511-USBLC6-2SC6 | 0,51 | 497-5235-1-ND | 0,47 |
| 17 | D21 | 1 | Schottky 30 V 200 mA SOD-323 | BAT54WS-7-F |  | C124205 | 0,57 ×10 | 621-BAT54WS-F | 0,09 | BAT54WS-FDICT-ND | 0,19 |
| 18 | D90 … (8) | 8 | LED verde 0805 | LTST-C171GKT |  | C913075 | 0,56 ×20 | 859-LTST-C171GKT | 1,04 | 160-1423-1-ND | 1,04 |
| 19 | D93 | 1 | LED arancio 0805 | LTST-C171KFKT |  | C284932 | 0,55 ×20 | 859-LTST-C171KFKT | 0,14 | 160-1425-1-ND | 0,14 |
| 20 | D103 … (5) | 5 | LED rosso 0805 | LTST-C171KRKT |  | C284871 | 0,47 ×20 | 859-LTST-C171KRKT | 0,69 | 160-1427-1-ND | 0,70 |
| 21 | F1 | 1 | PTC 500 mA 15 V 1812 | MF-MSMF050-2 | **210** | C17313 | 0,82 ×10 | 652-MF-MSMF050-2 | 0,28 | MF-MSMF050-2CT-ND | 0,33 |
| 22 | J1 … (6) | 6 | USB-C 16 pin GCT USB4105-GF-A | USB4105-GF-A | **279** | C3020560 | 7,03 | 640-USB4105-GF-A | 4,15 | 2073-USB4105-GF-ACT-ND | 4,20 |
| 23 | J3 | 1 | Pin header 1×4 2,54 mm | B-2100S04P-A110 / 61300411121 |  | C124378 | 0,51 ×10 | 710-61300411121 | 0,16 | 732-5317-ND | 0,16 |
| 24 | J9 | 1 | Pin header 1×3 2,54 mm | 2.54-1*3P / 61300311121 |  | C49257 | 0,51 ×20 | 710-61300311121 | 0,11 | 732-5316-ND | 0,11 |
| 25 | J4 | 1 | Header SWD 2×5 1,27 mm SMD | FTSH-105-01-L-DV-K |  | C5155080 | 1,31 | 200-FTSH10501LDVK | 1,07 | SAM8799-ND | 1,20 |
| 26 | J5, J7 | 2 | Boccola 4 mm nera Cliff FCR7350B | FCR7350B |  | — | — | — | — | 4654-FCR7350B-ND | 13,46 |
| 27 | J6, J8 | 2 | Boccola 4 mm rossa Cliff FCR7350R | FCR7350R |  | C6041087 | 21,38 | — | — | 4654-FCR7350R-ND | 13,46 |
| 28 | J501 | 1 | USB-A Kycon KUSBX-AS1N-B | KUSBX-AS1N-B |  | C7212838 | 0,66 | 806-KUSBX-AS1N-B | 0,61 | 2092-KUSBX-AS1N-B-ND | 0,62 |
| 29 | K1, K2 | 2 | Relè Omron G5Q-1 DC5 | G5Q-1 DC5 |  | C397241 | 1,98 | 653-G5Q-1-DC5 | 3,09 | Z12713-ND | 3,16 |
| 30 | Q101 … (5) | 5 | MOSFET N AO3400A SOT-23 | AO3400A |  | C20917 | 0,40 | 621-DMN3404L-7 | 1,73 | 785-1000-1-ND | 2,30 |
| 31 | R1, R2, R20, R21 | 4 | 5,1 kΩ 1% 0603 | 0603WAF5101T5E / RC0603FR-075K1L | **238** | C23186 | 0,13 ×100 | 603-RC0603FR-075K1L | 0,41 | 311-5.10KHRCT-ND | 0,36 |
| 32 | R3 … (16) | 16 | 1 kΩ 1% 0603 | 0603WAF1001T5E / RC0603FR-071KL | **284** | C21190 | 0,23 ×100 | 603-RC0603FR-071KL | 0,19 | 311-1.00KHRCT-ND | 0,35 |
| 33 | R5 … (6) | 6 | 20 mΩ 1% 2512 | RL2512FK-070R02L |  | C326611 | 0,65 | 603-RL2512FK-070R02L | 3,06 | 311-0.02TCT-ND | 3,18 |
| 34 | R22 … (17) | 17 | 100 kΩ 1% 0603 | FRC0603F1003TS / RC0603FR-07100KL | **247** | C2906980 | 0,21 ×100 | 603-RC0603FR-07100KL | 0,20 | 311-100KHRCT-ND | 0,37 |
| 35 | R24 | 1 | 1 MΩ 1% 0603 | FRC0603F1004TS / RC0603FR-071ML | **249** | C2907003 | 0,18 ×100 | 603-RC0603FR-071ML | 0,09 | 311-1.00MHRCT-ND | 0,09 |
| 36 | R25 | 1 | 12,0 kΩ 1% 0603 | RC0603FR-0712KL |  | C114659 | 0,38 ×100 | 603-RC0603FR-0712KL | 0,09 | 311-12.0KHRCT-ND | 0,09 |
| 37 | R26 … (8) | 8 | 10 kΩ 1% 0603 | FRC0603F1002TS / RC0603FR-0710KL | **248** | C2906982 | 0,19 ×100 | 603-RC0603FR-0710KL | 0,69 | 311-10.0KHRCT-ND | 0,72 |
| 38 | R30 | 1 | 69,8 kΩ 1% 0603 | RC0603FR-0769K8L |  | C137689 | 0,35 ×100 | 603-RC0603FR-0769K8L | 0,10 | 311-69.8KHRCT-ND | 0,10 |
| 39 | R31 | 1 | 9,53 kΩ 1% 0603 | 0603WAF9531T5E / RC0603FR-079K53L |  | C23127 | 0,15 ×100 | 603-RC0603FR-079K53L | 0,09 | 311-9.53KHRCT-ND | 0,09 |
| 40 | R32 | 1 | 20,5 kΩ 1% 0603 | RC0603FR-0720K5L |  | C273789 | 0,55 ×100 | 603-RC0603FR-0720K5L | 0,09 | 311-20.5KHRCT-ND | 0,09 |
| 41 | R33 | 1 | 953 Ω 1% 0603 | RC0603FR-07953RL |  | C137668 | 0,33 ×100 | 603-RC0603FR-07953RL | 0,09 | 311-953HRCT-ND | 0,09 |
| 42 | R40, R41 | 2 | 4,7 kΩ 1% 0603 | RC0603FR-074K7L |  | C99782 | 0,30 ×100 | 603-RC0603FR-074K7L | 0,17 | 311-4.70KHRCT-ND | 0,18 |
| 43 | R60, R61 | 2 | 24,3 kΩ 1% 0603 | RC0603FR-0724K3L |  | C276290 | 0,40 ×100 | 603-RC0603FR-0724K3L | 0,17 | 311-24.3KHRCT-ND | 0,18 |
| 44 | R62 | 1 | 33,2 kΩ 1% 0603 | RC0603FR-0733K2L |  | C185338 | 0,55 ×100 | 603-RC0603FR-0733K2L | 0,09 | 311-33.2KHRCT-ND | 0,09 |
| 45 | R81 … (7) | 5 | 100 Ω 1% 1206 | RC1206FR-07100RL |  | C137392 | 0,50 ×50 | 603-RC1206FR-07100RL | 0,43 | 311-100FRCT-ND | 0,45 |
| 46 | R102 … (8) | 8 | 56 kΩ 1% 0603 | RC0603FR-0756KL |  | C114630 | 0,38 ×100 | 603-RC0603FR-0756KL | 0,69 | 311-56KHRCT-ND | 0,72 |
| 47 | SW1 … (7) | 7 | Pulsante C&K PTS645 SMD 7 mm | PTS645SL70SMTR92 LFS |  | C285485 | 8,31 | 611-645SL70SMTR92LFS | 2,54 | CKN10820CT-ND | 2,66 |
| 48 | U1 | 1 | LDO AP7361C-33E | AP7361C-33E-13 |  | C500795 | 0,73 | 621-AP7361C-33E-13 | 0,45 | AP7361C-33E-13DICT-ND | 0,46 |
| 49 | U2 | 1 | Hub USB2517I | USB2517I-JZX |  | C478081 | 4,06 | 886-USB2517I-JZX | 5,80 | 638-1090-ND | 5,89 |
| 50 | U3 | 1 | MCU STM32G0B1RET6 | STM32G0B1RET6 |  | C2829307 | 6,08 | 511-STM32G0B1RET6 | 5,42 | 497-STM32G0B1RET6-ND | 5,51 |
| 51 | U4, U5, U6 | 3 | Load switch TPS2561A | TPS2561ADRCR |  | C473357 | 4,69 | 595-TPS2561ADRCR | 4,16 | 296-43616-1-ND | 4,20 |
| 52 | U7, U8 | 2 | INA3221 | INA3221AIRGVR |  | C181255 | 4,22 | 595-INA3221AIRGVR | 5,99 | 296-INA3221AIRGVRCT-ND | 6,10 |
| 53 | U9 | 1 | ULN2003V12 | ULN2003V12DR |  | C94916 | 0,53 | 595-ULN2003V12DR | 1,07 | 296-30538-1-ND | 1,09 |
| 54 | U12 | 1 | Supervisore MCP120-300 | MCP120T-300I/TT |  | C625180 | 0,49 | 579-MCP120T-300I/TT | 0,52 | MCP120T-300I/TTCT-ND | 0,53 |
| 55 | U13 | 1 | eFuse TPS259470A | TPS259470ARPWR |  | C3662799 | 1,15 | 595-TPS259470ARPWR | 1,70 | 296-TPS259470ARPWRCT-ND | 1,81 |
| 56 | U101 … (5) | 5 | Switch dati PI3USB4002A | PI3USB4002AZUAEX |  | C6735706 | 6,08 | 621-PI3USB4002A | 5,64 | 31-PI3USB4002AZUAEXCT-ND | 5,90 |
| 57 | Y1 | 1 | Quarzo 24 MHz 3225 12 pF | X322524MOB4SI / ECS-240-12-33Q-JES-TR |  | C70590 | 0,43 ×5 | 520-240-12-33Q-JEST | 0,73 | XC2150CT-ND | 0,75 |

## Note sulle righe

- C3, C4: a magazzino ci sono Man Yue EGT 220 µF 25 V D6,3 × 11 mm (passo 2,5 mm), che non sono
  su Mouser e DigiKey; lì l'equivalente è Panasonic EEU-FR1A221 (220 µF 10 V, D6,3 × 11,2 mm,
  stesso passo).
- C81, C82 (DNP): a magazzino Walsin 0805B104K500CT; su Mouser e DigiKey Samsung CL21B104KBCNNNC.
- Resistenze a magazzino (UNI-ROYAL, FOJAN, da LCSC): su Mouser e DigiKey l'equivalente Yageo
  RC0603FR-07…L. R31 su LCSC è UNI-ROYAL perché la Yageo era esaurita.
- D2: Diodes B340A-13-F (Schottky 40 V 3 A, SMA), presente su tutti e tre i fornitori.
  Il "SS34" di Vishay e onsemi è in SMC e non entra nel footprint.
- D21, R62: aggiunti dopo la revisione 03 (D53, D54). Prezzi Mouser non letti dal carrello: per
  D21 dal listino USA di Mouser (0,10 $, da un aggregatore), per R62 come le altre Yageo RC0603 a
  pezzo singolo. Il 12/10/2026 DigiKey non aveva scorte di BAT54WS-7-F (ordinabile in arretrato).
  Nei totali per 10 schede su Mouser la variazione di R60-R62 è stimata nulla.
- J3, J9: su LCSC header generici (Ckmtw, BOOMELE), su Mouser e DigiKey Würth 61300411121 e
  61300311121.
- J5, J7 (FCR7350B): non a catalogo su LCSC e su Mouser. J6, J8 (FCR7350R): non a catalogo su
  Mouser; su LCSC senza scorte e a 11,95 $ al pezzo sotto i 200 pezzi.
- K1, K2: su LCSC c'è solo G5Q-14 DC5, la versione da 10 A della stessa serie, con lo stesso
  ingombro e piedinatura.
- Q101-Q501: AO3400A non è a catalogo Mouser; lì l'equivalente è Diodes DMN3404L-7 (SOT-23,
  stessa piedinatura, 30 V, 5,8 A, R_DS(on) max 82 mΩ con V_GS = 3 V, abbondante per la scarica
  a ~50 mA).
- Y1: lo YXC X322524MOB4SI c'è solo su LCSC; su Mouser e DigiKey l'equivalente è ECS-240-12-33Q-JES-TR
  (24 MHz, C_L 12 pF, ±20 ppm, 3225, ESR 40 Ω), quindi C22/C23 restano 15 pF.
- Scorte LCSC a zero il 12/10/2026: C1591 (100 nF), C29936 (1 µF), USB4105-GF-A, FCR7350R,
  KUSBX-AS1N-B, PI3USB4002AZUAEX; STM32G0B1RET6 ne aveva 4. Le prime due righe e i connettori
  USB-C sono a magazzino.

## Magazzino

Righe a magazzino con le quantità comprate (la colonna "Utilizzo" del foglio indica in quali
altri progetti sono già stati usati; il residuo va controllato prima di montare):

| ID | Articolo | Fornitore | Comprati | Servono | Usati altrove |
|---|---|---|---|---|---|
| 235 | CL10B104KB8NNNC 100 nF 0603 | LCSC | 100 | 40 | — |
| 60 | EGT227M1EE11RRS0P 220 µF 25 V | TME | 10 | 2 | — |
| 192 | GRM21BR61H106KE43L 10 µF 50 V 0805 | TME | 50 | 4 | — |
| 234 | CL10B103KB8NNNC 10 nF 0603 | LCSC | 50 | 1 | — |
| 236 | CL21A475KOFNNNE 4,7 µF 0805 | LCSC | 50 | 1 | — |
| 240 | CL10B105KA8NNNC 1 µF 25 V 0603 | LCSC | 50 | 12 | — (anche ID 214, altri 50) |
| 276 | CL10A475KO8NNNC 4,7 µF 0603 | LCSC | 30 | 1 | pb-native, pb-uart, rema-d02 |
| 178 | 0805B104K500CT 100 nF 0805 (DNP) | TME | 100 | 0 | — |
| 300 | GRM21BZ71A226ME15L 22 µF 0805 | LCSC | 20 | 5 | gw-03 (4 pezzi) |
| 184 | EEE-FK1C101P 100 µF 16 V SMD | TME | 10 | 1 | — |
| 221 | SMAJ20A-13-F | LCSC | 3 | 1 | — |
| 277 | USBLC6-2SC6 | LCSC | 10 | 1 | pb-native, pb-uart |
| 210 | MF-MSMF050-2 PTC 1812 | LCSC | 8 | 1 | — |
| 279 | USB4105-GF-A | LCSC | 10 | 6 | pb-native, pb-uart |
| 238 | 0603WAF5101T5E 5,1 kΩ | LCSC | 100 | 4 | — |
| 284 | 0603WAF1001T5E 1 kΩ | LCSC | 100 | 16 | pb-uart |
| 247 | FRC0603F1003TS 100 kΩ | LCSC | 100 | 17 | — |
| 249 | FRC0603F1004TS 1 MΩ | LCSC | 100 | 1 | — |
| 248 | FRC0603F1002TS 10 kΩ | LCSC | 100 | 8 | — |

Non usati dal magazzino, anche se simili: i LED verdi TLMG1100 (0603, solo 5 pezzi), il
supervisore TLV803EA30 (piedinatura diversa dal MCP120), il 2N7002 al posto dell'AO3400A (soglia
di gate al limite con 3,3 V), i 47 pF e le resistenze 0402 (69,8 kΩ e altre: si è evitato il
passaggio a 0402).
