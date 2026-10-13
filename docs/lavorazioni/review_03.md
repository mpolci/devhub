# Revisione 03 — verifica dello schema contro i datasheet

Data: 12 ottobre 2026. **Esito: schema non ancora approvabile per la produzione.**

La verifica comprende tutti i **216 riferimenti** della netlist, inclusi cinque DNP e i due ponticelli di rame. Sono stati controllati collegamenti, numeri dei pin, polarità, package assegnati, geometria delle impronte in libreria e requisiti elettrici. L'appendice elenca ogni riferimento; i componenti ripetuti condividono la verifica di famiglia, ma sono stati controllati sulle reti delle singole istanze.

**Nessun file KiCad è stato modificato. La PCB non è stata esaminata.** Le impronte verificate sono quelle richiamate dallo schema e risolte nelle librerie locali, non le copie eventualmente modificate e incorporate nella PCB. Posizionamento, rame, impedenza USB, Kelvin, dissipazione reale, distanze d'isolamento e fabbricabilità della PCB restano fuori da questa revisione.

## Metodo e riproducibilità

- Copia temporanea dei fogli e delle librerie di progetto, senza rigenerazione dei fogli.
- Esportazione con `kicad-cli sch export netlist --format kicadxml`; controllo ERC sulla medesima copia: **0 errori e 0 warning con le impostazioni del progetto**. Sono esclusi dalla configurazione i controlli su etichette globali singole, giunzioni a quattro vie, modelli SPICE e corrispondenza dei filtri footprint; il confronto manuale delle impronte è quindi essenziale.
- Tutti i numeri di pin dei simboli trovano il corrispondente numero di pad nelle impronte assegnate. Questo controllo non rileva una polarità errata né una dimensione inadeguata.
- Nessuna rete con nome automatico `Net-...`; le reti `unconnected-...` sono i pin intenzionalmente non collegati.
- DNP effettivi: **C24, C81, C82, R81, R82**. Non sono stati trattati come componenti montati.
- Confronto dei pin con tabelle/disegni dei costruttori; ispezione visiva dei disegni nei casi in cui l'estrazione testuale perde orientamento o polarità.
- Datasheet mancanti scaricati in `datasheets/`, usando copie del costruttore anche quando ospitate da LCSC/DigiKey. Alcuni documenti sono cataloghi di famiglia, non schede di approvazione del singolo suffisso: le relative riserve sono esplicite.

La cartella `datasheets/` è ignorata da Git. Il report non attesta prove sperimentali né conformità USB. Le decisioni D27–D50 sono state considerate, senza riproporre come nuove anomalie le deroghe già deliberate.

## Rilievi da risolvere

### R03-01 — D1: polarità della SMAJ20A incompatibile con le reti assegnate

**Priorità: bloccante. Errore certo tra simbolo e footprint.**

Nel foglio alimentazione D1 ha valore `SMAJ20A`, simbolo con terminali `A1`/`A2`, footprint `Diode_SMD:D_SMA` e questi collegamenti:

| Pad | Rete nello schema | Significato nel footprint SMA polarizzato |
|---|---|---|
| 1 | GND | Catodo, lato della marcatura |
| 2 | VBUS_IN | Anodo |

La SMAJ20**A** scelta è **unidirezionale**; la variante bidirezionale sarebbe SMAJ20**CA**. Il datasheet Diodes identifica con una banda il catodo. Seguendo la marcatura del footprint, D1 viene quindi montata **in diretta tra VBUS_IN e GND**: conduce già all'alimentazione ordinaria, prima dell'eFuse U13, e può far intervenire la sorgente o danneggiarsi. L'ERC non vede il problema perché il simbolo usa terminali passivi non polarizzati.

Da correggere nella successiva sessione: simbolo unidirezionale coerente, catodo/pad 1 a VBUS_IN, anodo/pad 2 a GND; poi aggiornamento controllato della PCB. Una rotazione manuale del componente contraria alla serigrafia non è una correzione del progetto.

Fonte: [SMAJ Diodes, pp. 1 e 3](../../datasheets/smaj_diodes.pdf). La famiglia specifica per SMAJ20A breakdown **22,2–24,5 V**, non 22,2–23,4 V come indicato in `componenti.md`; clamp massimo 32,4 V alla corrente impulsiva specificata. La limitazione della protezione ai 20 V continui resta distinta dalla protezione da impulsi.

### R03-02 — U2 pin 44: VBUS_DET fuori dal campo ammesso a hub spento

**Priorità: alta. Condizione ordinaria non coperta dal datasheet.**

J2 alimenta il partitore R22/R23, 100 kΩ + 100 kΩ; il punto centrale, con C21 da 1 µF, va contemporaneamente al pin 44 dell'USB2517 e a PB2 del MCU. Con PC collegato e alimentatore esterno scollegato il partitore tende a **2,5 V**, mentre VDD33 dell'hub è zero.

USB2517, §8.2, p. 48: quando una sua alimentazione 3,3 V scende sotto 3,0 V, il massimo operativo sugli I/O diventa **VDD33 + 0,5 V**. A hub spento il limite è quindi 0,5 V. Il partitore non lo garantisce. Se una protezione interna limita il nodo, il valore non sarà più 2,5 V, ma si avrà corrente verso il circuito spento: manca comunque una specifica che autorizzi questo uso. Il limite assoluto di 5,5 V sugli I/O non equivale a tolleranza operativa a dispositivo spento.

Il passaggio del MCU a un pin FT risolve solo il lato MCU. PB2 è effettivamente **FT_ea / ADC_IN10**, ma ciò non rende tollerante il pin dell'hub collegato alla stessa rete.

Da risolvere: rilevamento VBUS che rispetti il limite dell'hub anche senza alimentazione, con verifica di inserzione/rimozione del PC e brownout. Non è dimostrato un danno inevitabile; è dimostrata l'assenza di margine rispetto al campo operativo prescritto.

Fonti: [USB2517, §8.2](../../datasheets/usb2517.pdf), [STM32G0B1, tabella dei pin e condizioni degli I/O](../../datasheets/DS_stm32g0b1cc.pdf).

**Esito:** chiuso senza modifiche al circuito (D53). La *USB2517 Hardware Design Checklist* di Microchip, sezione 5.1, raccomanda proprio il partitore 100 kΩ / 100 kΩ e ammette la VBUS presente prima dell'alimentazione dell'hub, purché l'impedenza limiti la corrente nel pin.

### R03-03 — J501: il connettore USB-A è nominalmente da 1,5 A

**Priorità: alta. RF-5 non soddisfatto dalla scelta del componente.**

Il datasheet Kycon KUSBX-AS1N-B dichiara **1,5 A**. RF-5 richiede almeno **2 A su ogni porta**; U6 permette 2,1–2,5 A. Il connettore è quindi il componente sottodimensionato, anche se piedinatura e impronta sono corrette.

Da risolvere: connettore specificato per la corrente richiesta e per il massimo limite ammesso, oppure revisione esplicita del requisito/limite della sola porta A. Il mantenimento del connettore non può essere giustificato dal fatto che il load switch sopporta 2,5 A.

Fonte: [Kycon KUSBX, Electrical Characteristics](../../datasheets/kusbx.pdf).

### R03-04 — R5/R65–R69: il TCR degli shunt non sostiene RF-6 in temperatura

**Priorità: alta. Accuratezza dichiarata non garantita.**

Il codice effettivamente scelto è **Yageo RL2512FK-070R02L**, 20 mΩ, 1%, 1 W, thick film. La tabella RL2512 per **18 mΩ < R ≤ 47 mΩ** ammette **±1200 ppm/°C**: a 20 °C di differenza dal punto di riferimento il solo shunt può variare del **2,4%**. A 2 A sono 48 mA, mentre RF-6 ammette ±28 mA complessivi.

L'azzeramento a porta spenta corregge l'offset dell'INA3221, **non il guadagno introdotto dal valore dello shunt**. Anche a temperatura stabile, la somma 1% dello shunt + 0,25% del monitor usa quasi tutto l'1,3% previsto. Vanno aggiunti deriva del monitor, autoriscaldamento, quantizzazione della misura e dell'azzeramento, rumore e connessione Kelvin.

La risoluzione resta corretta: 40 µV / 20 mΩ = **2 mA/LSB**. La potenza è adeguata: 0,08 W a 2 A, 0,125 W a 2,5 A e 0,245 W su R5 a 3,5 A, prima del derating termico. Il problema è metrologico, non la potenza nominale né il package 2512.

Da risolvere: shunt con TCR adeguato e bilancio d'errore completo, oppure specifica di accuratezza con intervallo di temperatura e calibrazione del guadagno espliciti.

Fonti: [Yageo RL, p. 5](../../datasheets/yageo_rl.pdf), [copia RL associata al codice LCSC C326611, p. 5](../../datasheets/yageo_rl_alt.pdf), [INA3221, Electrical Characteristics](../../datasheets/ina3221.pdf).

### R03-05 — Impronte custom: courtyard più piccolo delle piazzole

**Priorità: media. Errore geometrico nelle librerie, senza esaminare la PCB.**

Le numerazioni dei pad di U13 e U101/U201/U301/U401/U501 corrispondono ai rispettivi pinout, ma i courtyard non contengono il rame delle impronte:

- PI3USB4002A: courtyard x = ±0,80 mm, y = ±1,05 mm; i pad laterali arrivano a x = ±1,0425 mm e quelli 5/10 a y = ±1,2925 mm. Il package è correttamente 1,5 × 2 mm, passo 0,5 mm. I pad sono anche diversi dal land pattern consigliato Diodes: laterali 0,665 × 0,280 mm invece di 0,53 × 0,25 mm, con centri a ±0,71 invece di ±0,64 mm.
- TPS259470A: courtyard x/y = ±1,00 mm; i pad IN/OUT si estendono a y = ±1,20 mm e i pad laterali oltre x = ±1,20 mm. I pad 5/6 lunghi, senza pad termico aggiuntivo numerato 11, sono coerenti con RPW0010A; le coordinate arrotondate e i poligoni d'angolo richiedono comunque confronto con il land pattern TI.

Il courtyard attuale può lasciare passare collocazioni troppo vicine senza segnalarle come sovrapposizione di impronte. Non si afferma che ciò sia già successo sulla PCB. Da correggere nelle librerie e poi sincronizzare con la PCB quando consentito; le differenze dei pad rispetto al pattern raccomandato vanno accettate consapevolmente per il processo di saldatura.

Fonti: [PI3USB4002A, disegno ZUA, p. 8](../../datasheets/pi3usb4002a.pdf), [TPS25947, disegno RPW0010A, Example Board Layout](../../datasheets/tps25947.pdf).

## Verifiche con riserva e condizioni di validità

### Alimentazione 3,3 V e margine termico

U1 è correttamente **AP7361C-33E**, SOT-223: 1 IN, 2 GND e tab, 3 OUT. Non è la variante SOT223R e non ha la piedinatura dell'AMS1117. C8/C9 da 10 µF sono nominalmente superiori ai minimi richiesti; per la stabilità va garantita la capacità effettiva di C9 ≥2,2 µF. D2/B340A è orientato correttamente e regge ampiamente la corrente della logica.

Il solo USB2517 può assorbire **460 mA** nella condizione a sette porte HS. Con circa 0,50 A complessivi sulla 3,3 V e 4,9 V all'ingresso dell'LDO, P ≈ (4,9−3,3) × 0,50 = **0,80 W**. Usando θJA = 110 °C/W si ottengono 88 °C di incremento, ossia circa 128 °C di giunzione a 40 °C ambiente. È un esempio di margine critico, non una temperatura misurata: dipende da tensione reale, diodo, rame e carico. Il bilancio documentato a 4,6 V non copre ogni caso. Richiesta verifica termica al massimo carico/tensione prima di approvare U1.

Il MCP120-300 può rilasciare il reset fra 2,85 e 3,0 V; l'hub richiede almeno 3,0 V per operare. Il ritardo 150–700 ms aiuta all'avvio, ma non certifica il funzionamento in una permanenza prolungata sotto 3,0 V. La gestione firmware D35 è una mitigazione da provare, non una garanzia analogica della tensione minima.

Fonti: [AP7361C, pinout, caratteristiche e termica](../../datasheets/ap7361c.pdf), [USB2517, §8.3](../../datasheets/usb2517.pdf), [MCP120](../../datasheets/mcp120.pdf).

### Alimentazione USB e protezioni: cosa è effettivamente coperto

- La catena eFuse–shunt–switch–shunt introduce una caduta: usando i valori tipici 28 mΩ + 20 mΩ + 44 mΩ + 20 mΩ, a 2 A sono circa **224 mV**, senza connettori, cavi e rame. Una sorgente al limite inferiore non garantisce quindi esattamente 5,0 V al carico. RF-5 va interpretato come rail nominale; una tensione minima al connettore va specificata e verificata.
- I TPS2561A tollerano fino a 6,5 V nel campo operativo su IN/OUT, non 20–24 V. La tolleranza 24 V dei PI3USB4002A riguarda **solo C0+/C0−**, non VBUS, CC o l'intera porta. I 22 µF/10 V non sono clamp di sovratensione continua. D29/D37 non trasformano un controllo firmware in una protezione hardware da tensione arbitraria.
- Rp 56 kΩ e mancato attach-detect sulle quattro USB-C sono coerenti con D29/D30. Sono una deroga deliberata al comportamento di una sorgente Type-C, non conformità verificata. Il controllo di VBUS prima dell'accensione non rileva il successivo inserimento di una sorgente in una porta già accesa senza tempo di reazione.
- Il limite d'ingresso nominale 3,5 A supera il budget 3 A della sorgente: coerente con D21, ma la gestione firmware deve intervenire e non garantisce da sola l'assenza di brownout durante ogni transitorio.
- R101/R201/R301/R401/R501 da 100 Ω dissipano 0,25 W a 5 V; a 5,25 V e −1% di tolleranza diventano **0,278 W**. Il timeout limita la durata ma va confrontato con le specifiche d'impulso e con il derating, anche per cicli ripetuti. La scarica chiusa sull'INA3221 è adeguata come principio; la temporizzazione è ancora da provare.
- F1 è un PTC: 500 mA è **Ihold a 23 °C**, non un limite regolato. Per MF-MSMF050 la tabella di derating dà 0,40 A a 40 °C; la resistenza dopo intervento può arrivare a 1 Ω. La porta 7 di scorta non va presentata come una porta 500 mA garantita in ogni condizione. Non rientra nelle cinque porte RF-5.

Fonti: [TPS2561A](../../datasheets/tps2561a.pdf), [TPS25947](../../datasheets/tps25947.pdf), [PI3USB4002A](../../datasheets/pi3usb4002a.pdf), [Yageo RC](../../datasheets/resistor_1206.pdf), [Bourns MF-MSMF](../../datasheets/mf-msmf.pdf).

### Quarzo, connettore SWD e parti equivalenti

- Y1: pin 1/3 al cristallo, 2/4 a GND; package 3225 corretto. Il PDF YXC recuperato per C70590 è di famiglia YSX321SL e lascia CL configurabile: **non certifica da solo CL = 12 pF per X322524MOB4SI**. L'alternativa ECS-240-12-33Q-JES-TR esplicita 12 pF nel codice. C22=C23=15 pF forniscono 7,5 pF in serie: per arrivare a 12 pF occorrono circa 4,5 pF parassiti equivalenti. Avvio, frequenza e drive level vanno verificati; non basta la tolleranza nominale del quarzo. Il layout YXC consiglia centri verticali separati di 2,1 mm; il footprint generico usa 1,7 mm: sovrapposizione ai terminali presente, ma non è la copia del pattern raccomandato.
- J4: mappatura SWD corretta. Il Samtec FTSH-105-01-L-DV-K richiede passo 1,27 mm e pad larghi 0,74 mm, presenti. Il pattern Samtec consiglia lunghezza pad 2,79 mm e ingombro trasversale del rame 6,86 mm; il generico assegnato usa 2,40 e 6,30 mm. La numerazione è coerente, ma **l'impronta non è identica a quella consigliata**: da qualificare o sostituire con quella specifica, incluse geometria del corpo e chiave K. Nessun foro di allineamento è richiesto per la sola variante K.
- C3/C4: la scelta di magazzino Man Yue/Samxon EGT 220 µF/25 V e l'alternativa Panasonic EEU-FR1A221 220 µF/10 V devono restare distinguibili nella BOM. Entrambe sono descritte con D6,3/P2,5; per Panasonic questo è confermato dalla [scheda ufficiale del codice](https://na.industrial.panasonic.com/products/capacitors/aluminum-electrolytic-capacitors/series/83367/model/83617). Il catalogo Samxon recuperato non sostituisce la scheda d'approvazione del codice completo EGT227M1EE11RRS0P. Il valore nello schema è 10 V, sufficiente per il rail protetto, ma non identifica il pezzo da magazzino.
- J3/J9: verificati disegni e passo dei componenti LCSC indicati. Non è stata recuperata una copia PDF valida dei due codici alternativi Würth 61300411121/61300311121: la loro equivalenza completa resta da chiudere prima di usare quella colonna della BOM.

Fonti: [YXC](../../datasheets/yxc_24mhz.pdf), [ECS ECX-33Q](../../datasheets/ecx-33q.pdf), [Samtec disegno DV](../../datasheets/samtec_ftsh_dv.pdf), [Samtec land pattern](../../datasheets/samtec_ftsh_land.pdf), [catalogo Samxon](../../datasheets/manyue_egt.pdf).

## Verifica dei circuiti integrati e dei componenti polarizzati

| Riferimenti | Piedinatura e collegamenti verificati | Esito elettrico/package |
|---|---|---|
| U1 | 1 IN da D2, 2/tab GND, 3 OUT 3V3 | Corretti per AP7361C-33E; riserva termica sopra |
| U2 | Tutti i 64 pin e pad 65 confrontati; DS1–5 alle porte, DS6 MCU, DS7 scorta; upstream 58 DM/59 DP; 60 XTAL2/61 XTAL1; 63 RBIAS; 19 TEST e 65 VSS a GND | QFN64 9×9, passo 0,5, EP4,7; corretti. R03-02. Via termiche di libreria coerenti con D50; efficacia non verificata sulla PCB |
| U2 alimentazioni | 5/10/52/57 VDDA33, 24 VDD33CR, 46 VDD33, 64 VDD33PLL a 3V3; 25 VDD18 e 62 VDD18PLL con reti distinte, solo condensatori | Nessun carico esterno sulle uscite 1,8 V; bypass presenti |
| U2 configurazione | 13 CFG_SEL2 al MCU/pull-up; 41 SCL/CFG0 pull-up; 42 CFG1 pull-down; 43 RESET_N a supervisore/MCU; OCS1–5 ai FAULT corrispondenti | Stati strap coerenti con default/SMBus documentati. PRTPWR scollegati deliberatamente: potenza controllata dal MCU. Configurazione e sequenza restano firmware |
| U3 | Confrontati tutti i 64 pin della variante **GP RxT**, non RxTxN: 6 VBAT, 7 VREF+, 8 VDD/VDDA a 3V3; 9 VSS/VSSA GND; 43 USB_DM, 44 USB_DP; 45/46 SWD; 62/63 I2C | STM32G0B1RET6, LQFP64 10×10 passo 0,5 corretto. Nessuno scambio con il pinout della variante N. Tabella `pinout-mcu.md` coerente; nomi alimentazioni nel simbolo abbreviati |
| U4/U5/U6 | 1 GND, 2/3 IN, 4 EN1, 5 EN2, 6 FAULT2, 7 ILIM, 8 OUT2, 9 OUT1, 10 FAULT1, 11 EP GND | TPS2561A attivo alto corretto; VSON DRC 3×3, passo 0,5, EP1,65×2,4. Canale 2 U6 disabilitato a GND, OUT/FAULT NC. ILIM24,3k → 2,1–2,5 A |
| U7/U8 | 1 IN−3, 2 IN+3, 3 GND, 4 VS, 5 A0, 6 SCL, 7 SDA, 8 WARNING, 9 CRITICAL, 10 PV, 11 IN−1, 12 IN+1, 13 TC, 14 IN−2, 15 IN+2, 16 VPU, 17 EP | VQFN16 RGV 4×4 passo 0,65 EP2,1 corretto; indirizzi 0x40/0x41. IN+ a monte shunt, IN− a valle; totale dopo eFuse. CRITICAL open-drain condiviso, altre uscite NC. R03-04 |
| U9 | Ingressi 1–3 RLY1, 4–6 RLY2; uscite 16–14 bobina1, 13–11 bobina2; 8 GND; 9 COM a 5V; canale7 NC | ULN2003V12DR SOIC16 3,9×9,9 passo1,27 corretto. Tre canali in parallelo ammessi; diodi di ricircolo collegati. Caduta circa0,2V plausibile, non massimo garantito all'intera temperatura |
| U12 | 1 RESET_N, 2 VDD, 3 VSS | MCP120T-300I/TT SOT23 corretto; open-drain compatibile con PF1 e R27; C24 effettivamente DNP |
| U13 | 1 EN/UVLO, 2 OVLO, 3 AUXOFF NC, 4 FLT, 5 IN, 6 OUT, 7 dVdt, 8 GND, 9 ILM, 10 ITIMER | TPS259470A RPW 2×2 corretto; AUXOFF è un'uscita e può restare NC. R30–32 → UVLO~3,99V/OVLO~5,84V nominali; R33=953Ω → ~3,5A. C11/C12 presenti. C7 da 1 µF direttamente su OUT aggiunto come D48. R03-05 |
| U101/U201/U301/U401/U501 | 1 L0+, 2 L0− verso hub; 3/4 NC; 5 GND; 6 REF con 1 µF25V a massa; 7 C0−, 8 C0+ verso connettore; 9 EN con pull-up; 10 VDD3V3 | ZUA UQFN10 1,5×2 corretto; EN alto isola. Protezione e power-off lato dati coerenti. R03-05 |
| Q101/Q201/Q301/Q401/Q501 | 1 G al DIS, 2 S a GND, 3 D alla 100 Ω; pull-down100k | AO3400A SOT23 e alternativa DMN3404L con lo stesso pinout; comando3,3V adeguato. Riserva sulla potenza della resistenza di scarica |
| D1 | Pad1 GND, pad2 VBUS_IN | **Errato: R03-01** |
| D2 | Catodo1 a5V_LOGIC, anodo2 a5V_SYS | B340A SMA corretto, 40V/3A |
| D4 | Catodo1 EN, anodo2 GND | BZT52C5V1 SOD123 corretto; protezione EN in aggiunta al partitore. Tensione zener dipende dalla corrente; non assumerla esattamente5,1V |
| D20 | 1/6 DN, 3/4 DP, 2 GND, 5 VBUS upstream | USBLC6-2SC6 SOT23-6 corretto; sui dati upstream soltanto, coerente con D27 |
| K1/K2 | 1/5 bobina, 2 COM, 3 NO, 4 NC | G5Q-1 DC5 e impronta corretti, inclusa vista dal basso del datasheet. NC3A/30VDC per carico resistivo; 16V/3A non estendibile a ogni spunto/carico induttivo. Scatto75% a23°C; prova al limite termico resta necessaria |
| D90–D93 e LED porte | Catodo1 a GND, anodo2 alla resistenza 1k e relativo comando | Lite-On C171: package2×1,25 corretto; corrente dell'ordine1mA, non eccessiva per i GPIO. Verdi seguono EN/RLY, non misurano VBUS |

Per U3 sono stati verificati anche PA0/PA1 come ADC per CC, PB2/ADC_IN10, PF1 open-drain per reset, GPIO dei pulsanti, R48/R49 sui DBCC e alimentazioni. La sicurezza di tutte le transizioni bootloader/watchdog richiede esecuzione del firmware e prova su hardware; non è dichiarata superata da questa revisione statica.

## Connettori, pulsanti e passivi

| Gruppo | Risultato |
|---|---|
| J1/J2/J101/J201/J301/J401 | USB4105: A1/A12/B1/B12 GND; A4/A9/B4/B9 VBUS; A5/B5 CC; A6/B6 DP; A7/B7 DN; A8/B8 SBU NC; SH GND. Il footprint replica pad sovrapposti per contatti di potenza uniti; sono sulle stesse reti. Quote di fori0,65, ancoraggi a±4,32 e passo dati0,5 coerenti con il disegno. Corrente VBUS complessiva5A adeguata |
| J501 | 1 VBUS, 2 DN, 3 DP, 4 GND, SH GND; passo2,5/2/2,5mm e ancoraggi13,35mm coerenti. Corrente inadeguata: R03-03 |
| J5–J8 | Cliff FCR7350: singolo contatto; foro2,2mm e quattro appoggi a interasse10,16mm corrispondono. Rating24A ampio per 3 A. Circuiti isolati dalla massa della logica nello schema |
| J3/J9 | Header2,54mm: J3 VBUS/DN/DP/GND, J9 GND/TX/RX. Pin1 e sequenza coerenti; USB HS sull'header7 resta da qualificare come interconnessione |
| SW1/SW2/SW101/SW201/SW301/SW401 | PTS645SL70SMTR92: variante SMD G, altezza7mm. Impronta comune all'altezza4,3mm corretta in pianta: estensione pad9,5mm, gap6,4mm, distanza righe4,5mm. I contatti 1/2 del costruttore sono in comune, come3/4: i duplicati pad1 e pad2 KiCad riproducono correttamente le due coppie. Contatto fra GPIO e GND,100nF, pull-up interno da abilitare |
| JP2/JP3 | Rame1–2 chiuso: NC verso boccola; pad3 NO alternativo. Nessun datasheet commerciale. La capacità di portare3A del ponticello non si certifica senza la PCB |
| Resistenze0603 | Valori dei pull-up/down, partitori, bias e LED coerenti con le funzioni. Famiglie Yageo RC, UNI-ROYAL e FOJAN in1608 metrici: ingombri compatibili. Le potenze ordinarie sono molto inferiori al rating; R30 dissipa circa4mW a20V anche considerando il clamp EN |
| Shunt2512 | Package e potenza corretti; TCR non adeguato alla garanzia di accuratezza: R03-04 |
| Resistenze1206 | Scarica100Ω e snubber DNP coerenti come collegamenti. Potenza scarica da qualificare; snubber non dimensionati per un carico specifico |
| C3/C4/C505 | Polarità positiva su alimentazione, negativa a GND. C505 EEE-FK1C101P confermato100µF16V, D6,3×5,8mm. Riserva documentale sul codice completo EGT e distinzione dall'alternativa Panasonic |
| Ceramici0603/0805 | Formati e tensioni nominali compatibili con i rail. Samsung1µF25V corretto su REF dei cinque PI3; C11=2,2nF50V adeguato al requisito dVdt di VIN+5V anche per20V d'ingresso. C13=100nF50V a monte eFuse; Murata22µF10V a valle delle porte; Murata10µF50V su alimentazioni. Stabilità e bypass dipendono dalla capacità effettiva e dal posizionamento escluso dalla verifica |
| C22/C23/R24 |15pF C0G e1M fra XTAL1/XTAL2; corretto principio, carico del quarzo da confermare come sopra |

## Copertura dei requisiti

| Requisito | Esito della revisione statica |
|---|---|
| RF-1 | Quattro USB-C e una USB-A, percorsi USB2 presenti; Hi-Speed reale richiede verifica segnali |
| RF-2/RF-3/RF-4 | Interruttori dati/VBUS e scarica presenti. Sequenze, ciclo e reset del dispositivo dipendono dal firmware e dal carico |
| RF-5 | **Non soddisfatto da J501**; limiti switch corretti. Tensione minima e transitori da quantificare |
| RF-6 | Risoluzione2mA coerente; **accuratezza non garantita**, R03-04 |
| RF-7/RF-9/RF-10 | Funzioni firmware/host, non dimostrabili dal solo schema |
| RF-8 | MCU su DS6 non sezionata: topologia corretta; alimentazione/reset e R03-01/02 impediscono un'approvazione globale |
| RF-11/RF-12 | Due relè e quattro boccole, isolamento galvanico nello schema corretto; portata dei contatti valida per il tipo di carico specificato dal costruttore |
| RF-13/RF-14/RF-15 | Rd e ADC CC, eFuse e misura totale presenti. R03-01 bloccante; budget e protezione dinamica richiedono firmware/prototipo |
| RF-16 | Eliminato; non è richiesto funzionamento dal solo upstream. Rimane necessario tollerare il PC collegato a scheda spenta: R03-02 |
| RF-17 | Sette pulsanti con reteRC e GPIO coerenti; debounce e notifiche firmware |
| RNF-1 | Package coerenti con montaggio assistito da aria calda/piastra; courtyard e land pattern da chiudere. PCB esclusa |
| RNF-2 | Codici fornitore presenti per i componenti montati; disponibilità commerciale non riesaminata, equivalenze non tutte certificate |
| RNF-3 | ESD upstream USBLC6 e downstream PI3 presenti; TVS ingresso **errata nella polarità**. Prestazioni ESD di sistema non equivalgono al rating del chip |
| RNF-4/RNF-5 | Licenza/protocollo fuori dalla verifica componenti; LED presenti e polarizzati correttamente, comportamento fault firmware |

## Fonti e limiti della raccolta

I collegamenti locali nelle tabelle seguenti puntano ai PDF effettivamente presenti. I datasheet preesistenti di AMS1117, ULN2003A, TS3USB30E e74HC595 non sono stati usati per approvare i componenti sostitutivi attuali. Il file `DS_stm32g0b1cc.pdf`, nonostante il nome, è **DS13560 Rev 6 per STM32G0B1xB/xC/xE** e contiene correttamente STM32G0B1RET6.

La raccolta include datasheet di famiglia per resistenze e MLCC. Per YXC, EGT e alternative Würth rimangono le riserve sopra; non sono trasformate in esiti positivi solo perché esiste un codice fornitore. Il datasheet dell'alternativa Panasonic FR non è stato scaricato con successo dal sito del costruttore; le quote del codice sono state controllate sulla sua pagina ufficiale. Lo schema non è stato alterato per sostituire scelte o aggiornare campi Datasheet.


### Indice delle fonti locali

| ID | Documento |
|---|---|
| S01 | [AP7361C, Diodes DS37274, SOT223 e caratteristiche](../../datasheets/ap7361c.pdf) |
| S02 | [USB2517/USB2517I, Microchip DS00001598C](../../datasheets/usb2517.pdf) |
| S03 | [STM32G0B1xB/xC/xE, ST DS13560 Rev 6](../../datasheets/DS_stm32g0b1cc.pdf) |
| S04 | [TPS2560A/TPS2561A, TI SLVSB28](../../datasheets/tps2561a.pdf) |
| S05 | [INA3221, TI SBOS576C](../../datasheets/ina3221.pdf) |
| S06 | [ULN2003V12, TI](../../datasheets/uln2003v12.pdf) |
| S07 | [MCP120/130, Microchip DS11184D](../../datasheets/mcp120.pdf) |
| S08 | [TPS25947, TI, inclusi RPW0010A e applicazioni](../../datasheets/tps25947.pdf) |
| S09 | [PI3USB4002A, Diodes DS41994, pp. 2–4 e 8](../../datasheets/pi3usb4002a.pdf) |
| S10 | [AO3400A, Alpha & Omega](../../datasheets/ao3400a.pdf) |
| S11 | [DMN3404L, Diodes, alternativa MOSFET](../../datasheets/dmn3404l.pdf) |
| S12 | [SMAJ, Diodes DS19005, pp.1 e3](../../datasheets/smaj_diodes.pdf) |
| S13 | [B340A, Diodes](../../datasheets/b340a.pdf) |
| S14 | [BZT52Cxx, Diodes](../../datasheets/bzt52c.pdf) |
| S15 | [USBLC6-2, ST](../../datasheets/usblc6-2.pdf) |
| S16 | [G5Q, Omron, pp. 2 e4–6](../../datasheets/omron_g5q.pdf) |
| S17 | [USB4105, GCT, disegno e specifiche](../../datasheets/usb4105.pdf) |
| S18 | [KUSBX, Kycon, corrente e foratura](../../datasheets/kusbx.pdf) |
| S19 | [FCR7350/S16N-PC, Cliff](../../datasheets/cliff_s16npc.pdf) |
| S20 | [PTS645, C&K, pp.1 e3–5](../../datasheets/pts645.pdf) |
| S21 | [FTSH-DV, Samtec, disegno meccanico specifico](../../datasheets/samtec_ftsh_dv.pdf) |
| S22 | [FTSH-DV, Samtec, land pattern](../../datasheets/samtec_ftsh_land.pdf) |
| S23 | [Header3, disegno associato a C49257](../../datasheets/header3_lcsc.pdf) |
| S24 | [Header4, disegno associato a C124378](../../datasheets/header4_lcsc.pdf) |
| S25 | [MF-MSMF, Bourns, valori e derating](../../datasheets/mf-msmf.pdf) |
| S26 | [YSX321SL, YXC, documento di famiglia per C70590](../../datasheets/yxc_24mhz.pdf) |
| S27 | [ECX-33Q, ECS, alternativa quarzo](../../datasheets/ecx-33q.pdf) |
| S28 | [RL, Yageo, Rev 3 2023, p.5 TCR](../../datasheets/yageo_rl.pdf) |
| S29 | [RC, Yageo, catalogo di famiglia 0603/1206](../../datasheets/resistor_1206.pdf) |
| S30 | [Resistenze thick film UNI-ROYAL, famiglia 0603](../../datasheets/resistor_0603_uniroyal.pdf) |
| S31 | [FOJAN FRC, famiglia 0603](../../datasheets/resistor_0603_fojan.pdf) |
| S32 | [Samsung CL10B104KB8NNNC, specifica individuale](../../datasheets/cl10b104.pdf) |
| S33 | [Samsung MLCC, catalogo 2015 con codici dei ceramici](../../datasheets/cl10b105.pdf) |
| S34 | [Samsung CL10B105KA8NNNC, specifica individuale](../../datasheets/cl10b105_dk.pdf) |
| S35 | [Murata GRM21BR61H106KE43, 10µF50V0805](../../datasheets/murata_10u.pdf) |
| S36 | [Murata GRM21BZ71A226ME15, 22µF10V0805](../../datasheets/murata_22u.pdf) |
| S37 | [Samxon/Man Yue, catalogo 2017–2018 incl. EGT](../../datasheets/manyue_egt.pdf) |
| S38 | [Panasonic FK, incluso EEEFK1C101P](../../datasheets/panasonic_fk.pdf) |
| S39 | [Multicomp 0805B104K500CT, alternativa C81/C82](../../datasheets/cl21b104_alt.pdf) |
| S40 | [Lite-On LTST-C171GKT, verde](../../datasheets/led_green_lcsc.pdf) |
| S41 | [Lite-On LTST-C171KRKT, rosso](../../datasheets/led_red_lcsc.pdf) |
| S42 | [Lite-On LTST-C171KFKT, arancio](../../datasheets/led_orange_lcsc.pdf) |

### Provenienza dei nuovi PDF

I link seguenti consentono di recuperare i documenti anche senza la cartella locale ignorata da Git. Le copie LCSC sono documenti dei rispettivi produttori; il nome del file locale non certifica automaticamente il singolo MPN.

| File locale | Origine |
|---|---|
| `ap7361c.pdf` | [Download](https://www.diodes.com/datasheet/download/AP7361C.pdf) |
| `pi3usb4002a.pdf` | [Download](https://www.diodes.com/datasheet/download/PI3USB4002A.pdf) |
| `mcp120.pdf` | [Download](https://ww1.microchip.com/downloads/en/DeviceDoc/11184d.pdf) |
| `uln2003v12.pdf` | [Download](https://www.ti.com/lit/ds/symlink/uln2003v12.pdf) |
| `usb4105.pdf` | [Download](https://gct.co/files/drawings/usb4105.pdf) |
| `smaj_diodes.pdf` | [Download](https://www.diodes.com/datasheet/download/SMAJ5.0A.pdf) |
| `b340a.pdf` | [Download](https://www.diodes.com/datasheet/download/B340A.pdf) |
| `bzt52c.pdf` | [Download](https://www.diodes.com/datasheet/download/BZT52Cxx.pdf) |
| `mf-msmf.pdf` | [Download](https://www.bourns.com/docs/product-datasheets/mfmsmf.pdf) |
| `dmn3404l.pdf` | [Download](https://www.diodes.com/datasheet/download/DMN3404L.pdf) |
| `kusbx.pdf` | [Download](https://www.kycon.com/Catalog_PDF/KUSBX.pdf) |
| `ecx-33q.pdf` | [Download](https://ecsxtal.com/store/pdf/ECX-33Q.pdf) |
| `cliff_s16npc.pdf` | [Download](https://www.cliffuk.co.uk/products/testleads/sockets/S16NPC.pdf) |
| `samtec_ftsh_dv.pdf` | [Download](https://suddendocs.samtec.com/prints/ftsh-1xx-xx-xxx-dv-xxx-mkt.pdf) |
| `samtec_ftsh_land.pdf` | [Download](https://suddendocs.samtec.com/prints/ftsh-1xx-xx-xxx-dv-xxx-footprint.pdf) |
| `yageo_rl.pdf` | [Download](https://yageogroup.com/content/Resource%20Library/Datasheet/PYU-RL_GROUP_521_ROHS_L.pdf) |
| `cl10b104.pdf` | [Download](https://media.digikey.com/pdf/Data%20Sheets/Samsung%20PDFs/CL10B104KB8NNNC_Spec.pdf) |
| `cl10b105_dk.pdf` | [Download](https://mm.digikey.com/Volume0/opasdata/d220001/medias/docus/3644/CL10B105KA8NNNC_spec.pdf) |
| `murata_22u.pdf` | [Download](https://search.murata.co.jp/Ceramy/image/img/A01X/G101/ENG/GRM21BZ71A226ME15-01A.pdf) |
| `cl10b105.pdf` | [Download](https://datasheet.lcsc.com/datasheet/pdf/02336ea48ea44ca18c72517dd3cb7b47.pdf?productCode=C29936) |
| `cl21b104_alt.pdf` | [Download](https://datasheet.lcsc.com/datasheet/pdf/669be6f52207435bbc19b68e1b3bcb79.pdf?productCode=C83055) |
| `murata_10u.pdf` | [Download](https://datasheet.lcsc.com/datasheet/pdf/8cca754305bc7d881114d4ba10b82bf2.pdf?productCode=C440198) |
| `panasonic_fk.pdf` | [Download](https://datasheet.lcsc.com/datasheet/pdf/08528ee87311db943c8fade5cfd84e29.pdf?productCode=C128532) |
| `manyue_egt.pdf` | [Download](https://datasheet.lcsc.com/datasheet/pdf/3d8fe1f76744aa579617f8d60853bfaa.pdf?productCode=C238354) |
| `pts645.pdf` | [Download](https://datasheet.lcsc.com/datasheet/pdf/8581544a63b019c8b538cabd37a87ade.pdf?productCode=C285485) |
| `usblc6-2.pdf` | [Download](https://datasheet.lcsc.com/datasheet/pdf/0d3a2ab954b34651a0695e7ccf534db0.pdf?productCode=C7519) |
| `resistor_0603_uniroyal.pdf` | [Download](https://datasheet.lcsc.com/datasheet/pdf/0a975aaa49b7c97f38a963127be4a823.pdf?productCode=C23186) |
| `resistor_0603_fojan.pdf` | [Download](https://datasheet.lcsc.com/datasheet/pdf/106fb934f95f063b695dad1b31b1eab8.pdf?productCode=C2906980) |
| `resistor_1206.pdf` | [Download](https://datasheet.lcsc.com/datasheet/pdf/b426be3ce22c6de3e0c3a2248e49159e.pdf?productCode=C137392) |
| `yxc_24mhz.pdf` | [Download](https://datasheet.lcsc.com/datasheet/pdf/a84bd8d530dd46e4b0f6d0ee59d8a89c.pdf?productCode=C70590) |
| `led_green_lcsc.pdf` | [Download](https://datasheet.lcsc.com/datasheet/pdf/cd76ed74f8293b99e5c24e328eb50b21.pdf?productCode=C913075) |
| `led_red_lcsc.pdf` | [Download](https://datasheet.lcsc.com/datasheet/pdf/975a7c8bc4a67d739b2244371db3562e.pdf?productCode=C284871) |
| `led_orange_lcsc.pdf` | [Download](https://datasheet.lcsc.com/datasheet/pdf/97cba8cf5ae197efe99c7e63bd116de1.pdf?productCode=C284932) |
| `header4_lcsc.pdf` | [Download](https://datasheet.lcsc.com/datasheet/pdf/219f7e7eb5f34457895b2c164313179f.pdf?productCode=C124378) |
| `header3_lcsc.pdf` | [Download](https://datasheet.lcsc.com/datasheet/pdf/d52c83a07830c4e9e9a614accc55f5cd.pdf?productCode=C49257) |

## Appendice A — registro completo dei 216 riferimenti

**OK** = nessuna incongruenza rilevata nel perimetro statico descritto; non è una certificazione della PCB o del prodotto. **Riserva** = requisito, componente equivalente o dettaglio del package non completamente dimostrato. **R03-xx** rinvia al rilievo. Per le impronte, Fxx è il nome esatto riportato nell’appendice B.

| Ref | Valore nello schema | Impronta | Fonte | Esito e funzione verificata |
|---|---|---|---|---|
| C1 | 100n | F01 | S32 | OK100nF filtro CC/pulsante,0603 |
| C2 | 100n | F01 | S32 | OK100nF filtro CC/pulsante,0603 |
| C3 | 220u 10V | F02 | S37 | Riserva codice completo EGT; polarità e package D6,3/P2,5 coerenti |
| C4 | 220u 10V | F02 | S37 | Riserva codice completo EGT; polarità e package D6,3/P2,5 coerenti |
| C5 | 10u | F03 | S35 | OK10µF50V0805; Ceff e termica LDO come sopra |
| C6 | 10u | F03 | S35 | OK10µF50V0805; Ceff e termica LDO come sopra |
| C7 | 1u 25V | F01 | S33 | OK1µF25V0603; rete REF/bypass coerente |
| C8 | 10u | F03 | S35 | OK10µF50V0805; Ceff e termica LDO come sopra |
| C9 | 10u | F03 | S35 | OK10µF50V0805; Ceff e termica LDO come sopra |
| C10 | 100n | F01 | S32 | OK100nF50V0603; disaccoppiamento rail indicato |
| C11 | 2.2n | F01 | S33 | OK2,2nF50V dVdt; tensione nominale adeguata |
| C12 | 10n | F01 | S33 | OK10nF50V ITIMER |
| C13 | 100n 50V | F01 | S32 | OK100nF50V bypass ingresso prima eFuse |
| D1 | SMAJ20A | F04 | S12 | R03-01: catodo sulla rete errata |
| D2 | SS34 | F04 | S13 | OK A2 ingresso/K1 LDO |
| D4 | BZT52C5V1 | F05 | S14 | OK K1 EN/A2 GND |
| J1 | USB-C PWR IN | F06 | S17 | OK pinout/land pattern; ingresso Rd |
| R1 | 5.1k | F07 | S30 | OK Rd5,1k a GND |
| R2 | 5.1k | F07 | S30 | OK Rd5,1k a GND |
| R3 | 1k | F07 | S30 | OK filtro CC1k/100nF |
| R4 | 1k | F07 | S30 | OK filtro CC1k/100nF |
| R5 | 20m 1% 2512 | F08 | S28 | R03-04; shunt20mΩ, potenza/package corretti |
| R30 | 69.8k 1% | F07 | S29 | OK partitore UVLO/OVLO e potenza |
| R31 | 9.53k 1% | F07 | S29 | OK partitore UVLO/OVLO e potenza |
| R32 | 20.5k 1% | F07 | S29 | OK partitore UVLO/OVLO e potenza |
| R33 | 953 1% | F07 | S29 | OK ILM953Ω, limite nominale~3,5A |
| R34 | 10k | F07 | S31 | OK pull-up/down; stato logico e rail coerenti |
| U1 | AP7361C-33E | F09 | S01 | Riserva termica; pin 1IN/2GND/3OUT |
| U13 | TPS259470ARPWR | F10 | S08 | R03-05; pinout/soglie nominali corretti |
| C20 | 4.7u | F03 | S33 | OK4,7µF16V; package0805 per C20/0603 per C40 |
| C21 | 1u | F01 | S33 | R03-02; filtro VBUS_DET1µF |
| C22 | 15p C0G | F01 | S33 | OK15pF C0G; carico effettivo Y1 da confermare |
| C23 | 15p C0G | F01 | S33 | OK15pF C0G; carico effettivo Y1 da confermare |
| C24 | 100n | F01 | S32 | DNP verificato; reset hub |
| C25 | 100n | F01 | S32 | OK100nF50V0603; disaccoppiamento rail indicato |
| C26 | 100n | F01 | S32 | OK100nF50V0603; disaccoppiamento rail indicato |
| C27 | 100n | F01 | S32 | OK100nF50V0603; disaccoppiamento rail indicato |
| C28 | 100n | F01 | S32 | OK100nF50V0603; disaccoppiamento rail indicato |
| C29 | 100n | F01 | S32 | OK100nF50V0603; disaccoppiamento rail indicato |
| C30 | 100n | F01 | S32 | OK100nF50V0603; disaccoppiamento rail indicato |
| C31 | 1u | F01 | S33 | OK1µF25V0603; rete REF/bypass coerente |
| C32 | 1u | F01 | S33 | OK1µF25V0603; rete REF/bypass coerente |
| C33 | 1u | F01 | S33 | OK1µF25V0603; rete REF/bypass coerente |
| C34 | 100n | F01 | S32 | OK100nF50V0603; disaccoppiamento rail indicato |
| C35 | 1u | F01 | S33 | OK1µF25V0603; rete REF/bypass coerente |
| C36 | 100n | F01 | S32 | OK100nF50V0603; disaccoppiamento rail indicato |
| C37 | 100n | F01 | S32 | OK100nF50V0603; disaccoppiamento rail indicato |
| D20 | USBLC6-2SC6 | F11 | S15 | OK pin dati e clamp upstream |
| F1 | 500mA | F12 | S25 | Riserva: Ihold e caduta in temperatura, porta7 |
| J2 | USB-C UPSTREAM | F06 | S17 | OK pinout/land pattern; ingresso Rd |
| J3 | PORTA 7 (scorta) | F13 | S24 | OK header LCSC; alternativa Würth non chiusa |
| R20 | 5.1k | F07 | S30 | OK Rd5,1k a GND |
| R21 | 5.1k | F07 | S30 | OK Rd5,1k a GND |
| R22 | 100k | F07 | S29 | R03-02; partitore100k/100k upstream |
| R23 | 100k | F07 | S29 | R03-02; partitore100k/100k upstream |
| R24 | 1M | F07 | S31 | OK feedback quarzo1M; vedere riserva Y1 |
| R25 | 12.0k 1% | F07 | S29 | OK RBIAS12k1% a GND |
| R26 | 10k | F07 | S31 | OK pull-up/down; stato logico e rail coerenti |
| R27 | 10k | F07 | S31 | OK pull-up/down; stato logico e rail coerenti |
| R28 | 10k | F07 | S31 | OK pull-up/down; stato logico e rail coerenti |
| R29 | 10k | F07 | S31 | OK pull-up/down; stato logico e rail coerenti |
| U2 | USB2517I-JZX | F14 | S02 | R03-02; pinout e alimentazioni corretti |
| U12 | MCP120T-300I/TT | F15 | S07 | OK pinout; riserva soglia minima hub |
| Y1 | 24MHz 12pF | F16 | S26 | Riserva CL12pF/suffisso e land pattern; pin 1/3, GND 2/4 corretti |
| C40 | 4.7u | F01 | S33 | OK4,7µF16V; package0805 per C20/0603 per C40 |
| C41 | 100n | F01 | S32 | OK100nF50V0603; disaccoppiamento rail indicato |
| C42 | 100n | F01 | S32 | OK100nF50V0603; disaccoppiamento rail indicato |
| C43 | 100n | F01 | S32 | OK100nF50V0603; disaccoppiamento rail indicato |
| C44 | 100n | F01 | S32 | OK100nF50V0603; disaccoppiamento rail indicato |
| C45 | 1u | F01 | S33 | OK1µF25V0603; rete REF/bypass coerente |
| C46 | 100n | F01 | S32 | OK100nF NRST a GND |
| J4 | SWD | F17 | S22 | Riserva differenza pad; pinout SWD corretto |
| J9 | UART debug | F18 | S23 | OK header LCSC; alternativa Würth non chiusa |
| R40 | 4.7k | F07 | S29 | OK pull-up I2C4,7k a 3V3; rise time da misurare |
| R41 | 4.7k | F07 | S29 | OK pull-up I2C4,7k a 3V3; rise time da misurare |
| R42 | 100k | F07 | S31 | OK pull-up/down; stato logico e rail coerenti |
| R43 | 100k | F07 | S31 | OK pull-up/down; stato logico e rail coerenti |
| R44 | 100k | F07 | S31 | OK pull-up/down; stato logico e rail coerenti |
| R45 | 100k | F07 | S31 | OK pull-up/down; stato logico e rail coerenti |
| R46 | 100k | F07 | S31 | OK pull-up/down; stato logico e rail coerenti |
| R47 | 10k | F07 | S31 | OK pull-up/down; stato logico e rail coerenti |
| R48 | 10k | F07 | S31 | OK DBCC pull-down10k |
| R49 | 10k | F07 | S31 | OK DBCC pull-down10k |
| U3 | STM32G0B1RET6 | F19 | S03 | OK pinout GP e assegnazione periferiche; firmware da provare |
| C60 | 100n | F01 | S32 | OK100nF50V0603; disaccoppiamento rail indicato |
| C61 | 100n | F01 | S32 | OK100nF50V0603; disaccoppiamento rail indicato |
| C62 | 100n | F01 | S32 | OK100nF50V0603; disaccoppiamento rail indicato |
| C63 | 100n | F01 | S32 | OK100nF50V0603; disaccoppiamento rail indicato |
| C64 | 100n | F01 | S32 | OK100nF50V0603; disaccoppiamento rail indicato |
| R60 | 24.3k 1% | F07 | S29 | OK ILIM24,3k1%, canali2,1–2,5A |
| R61 | 24.3k 1% | F07 | S29 | OK ILIM24,3k1%, canali2,1–2,5A |
| R62 | 24.3k 1% | F07 | S29 | OK ILIM24,3k1%, canali2,1–2,5A |
| R65 | 20m 1% 2512 | F08 | S28 | R03-04; shunt20mΩ, potenza/package corretti |
| R66 | 20m 1% 2512 | F08 | S28 | R03-04; shunt20mΩ, potenza/package corretti |
| R67 | 20m 1% 2512 | F08 | S28 | R03-04; shunt20mΩ, potenza/package corretti |
| R68 | 20m 1% 2512 | F08 | S28 | R03-04; shunt20mΩ, potenza/package corretti |
| R69 | 20m 1% 2512 | F08 | S28 | R03-04; shunt20mΩ, potenza/package corretti |
| U4 | TPS2561ADRC | F20 | S04 | OK; canali porte1/2 |
| U5 | TPS2561ADRC | F20 | S04 | OK; canali porte3/4 |
| U6 | TPS2561ADRC | F20 | S04 | OK; porta5, secondo canale disabilitato |
| U7 | INA3221 | F21 | S05 | R03-04; misura porte1–3, indirizzo0x40 |
| U8 | INA3221 | F21 | S05 | R03-04; porte4–5 e totale, indirizzo0x41 |
| C81 | 100n 50V DNP | F03 | S39 | DNP; snubber100nF50V0805 |
| C82 | 100n 50V DNP | F03 | S39 | DNP; snubber100nF50V0805 |
| C83 | 100n | F01 | S32 | OK100nF filtro CC/pulsante,0603 |
| C84 | 100n | F01 | S32 | OK100nF filtro CC/pulsante,0603 |
| J5 | Banana COM 1 | F22 | S19 | OK contatto unico/foro2,2/appoggi10,16mm |
| J6 | Banana NC 1 | F23 | S19 | OK contatto unico/foro2,2/appoggi10,16mm |
| J7 | Banana COM 2 | F22 | S19 | OK contatto unico/foro2,2/appoggi10,16mm |
| J8 | Banana NC 2 | F23 | S19 | OK contatto unico/foro2,2/appoggi10,16mm |
| JP2 | NC/NO | F24 | — | OK collegamenti NC/NO; portata rame3A fuori verifica |
| JP3 | NC/NO | F24 | — | OK collegamenti NC/NO; portata rame3A fuori verifica |
| K1 | G5Q-1 DC5 | F25 | S16 | OK bobina1/5 COM2 NO3 NC4; carico resistivo |
| K2 | G5Q-1 DC5 | F25 | S16 | OK bobina1/5 COM2 NO3 NC4; carico resistivo |
| R81 | 100 DNP | F26 | S29 | DNP; snubber100Ω, carico da dimensionare |
| R82 | 100 DNP | F26 | S29 | DNP; snubber100Ω, carico da dimensionare |
| SW1 | relè on/off | F27 | S20 | OK SMD G7mm; coppie contatti e RC |
| SW2 | relè on/off | F27 | S20 | OK SMD G7mm; coppie contatti e RC |
| U9 | ULN2003V12 | F28 | S06 | OK; tre canali in parallelo per bobina |
| D90 | verde | F29 | S40 | OK K1 GND/A2 resistenza 1k; formato 0805 |
| D91 | verde | F29 | S40 | OK K1 GND/A2 resistenza 1k; formato 0805 |
| D92 | verde | F29 | S40 | OK K1 GND/A2 resistenza 1k; formato 0805 |
| D93 | arancio | F29 | S42 | OK K1 GND/A2 resistenza 1k; formato 0805 |
| R90 | 1k | F07 | S30 | OK limitazione LED1k, carico GPIO modesto |
| R91 | 1k | F07 | S30 | OK limitazione LED1k, carico GPIO modesto |
| R92 | 1k | F07 | S30 | OK limitazione LED1k, carico GPIO modesto |
| R93 | 1k | F07 | S30 | OK limitazione LED1k, carico GPIO modesto |
| C101 | 100n | F01 | S32 | OK100nF50V0603; disaccoppiamento rail indicato |
| C102 | 22u 10V | F03 | S36 | OK22µF10V0805 suVBUS; Ceff/transitori da provare |
| C103 | 100n | F01 | S32 | OK100nF50V0603; disaccoppiamento rail indicato |
| C104 | 100n | F01 | S32 | OK100nF filtro CC/pulsante,0603 |
| C106 | 1u 25V | F01 | S33 | OK1µF25V0603; rete REF/bypass coerente |
| D102 | verde | F29 | S40 | OK K1 GND/A2 resistenza 1k; formato 0805 |
| D103 | rosso | F29 | S41 | OK K1 GND/A2 resistenza 1k; formato 0805 |
| J101 | USB-C | F06 | S17 | OK pinout/package; deroga Type-C D29 |
| Q101 | AO3400A | F15 | S10 | OK G1/S2/D3; scarica con pull-down |
| R101 | 100 1206 | F26 | S29 | Riserva potenza scarica100Ω/0,25W |
| R102 | 56k | F07 | S29 | OK Rp56k a VBUS; deroga D29/D30 |
| R103 | 56k | F07 | S29 | OK Rp56k a VBUS; deroga D29/D30 |
| R104 | 100k | F07 | S31 | OK pull-up/down; stato logico e rail coerenti |
| R105 | 100k | F07 | S31 | OK pull-up/down; stato logico e rail coerenti |
| R106 | 1k | F07 | S30 | OK limitazione LED1k, carico GPIO modesto |
| R107 | 1k | F07 | S30 | OK limitazione LED1k, carico GPIO modesto |
| SW101 | porta on/off | F27 | S20 | OK SMD G7mm; coppie contatti e RC |
| U101 | PI3USB4002A | F30 | S09 | R03-05; pinout ZUA e lato protetto corretti |
| C501 | 100n | F01 | S32 | OK100nF50V0603; disaccoppiamento rail indicato |
| C502 | 22u 10V | F03 | S36 | OK22µF10V0805 suVBUS; Ceff/transitori da provare |
| C503 | 100n | F01 | S32 | OK100nF50V0603; disaccoppiamento rail indicato |
| C504 | 100n | F01 | S32 | OK100nF filtro CC/pulsante,0603 |
| C505 | 100u 10V | F31 | S38 | OK positivo VBUS/negativo GND;100µF16V D6,3×5,8 |
| C506 | 1u 25V | F01 | S33 | OK1µF25V0603; rete REF/bypass coerente |
| D502 | verde | F29 | S40 | OK K1 GND/A2 resistenza 1k; formato 0805 |
| D503 | rosso | F29 | S41 | OK K1 GND/A2 resistenza 1k; formato 0805 |
| J501 | USB-A | F32 | S18 | R03-03; pinout/foratura corretti, rating1,5A |
| Q501 | AO3400A | F15 | S10 | OK G1/S2/D3; scarica con pull-down |
| R501 | 100 1206 | F26 | S29 | Riserva potenza scarica100Ω/0,25W |
| R504 | 100k | F07 | S31 | OK pull-up/down; stato logico e rail coerenti |
| R505 | 100k | F07 | S31 | OK pull-up/down; stato logico e rail coerenti |
| R506 | 1k | F07 | S30 | OK limitazione LED1k, carico GPIO modesto |
| R507 | 1k | F07 | S30 | OK limitazione LED1k, carico GPIO modesto |
| SW501 | porta on/off | F27 | S20 | OK SMD G7mm; coppie contatti e RC |
| U501 | PI3USB4002A | F30 | S09 | R03-05; pinout ZUA e lato protetto corretti |
| C201 | 100n | F01 | S32 | OK100nF50V0603; disaccoppiamento rail indicato |
| C202 | 22u 10V | F03 | S36 | OK22µF10V0805 suVBUS; Ceff/transitori da provare |
| C203 | 100n | F01 | S32 | OK100nF50V0603; disaccoppiamento rail indicato |
| C204 | 100n | F01 | S32 | OK100nF filtro CC/pulsante,0603 |
| C206 | 1u 25V | F01 | S33 | OK1µF25V0603; rete REF/bypass coerente |
| D202 | verde | F29 | S40 | OK K1 GND/A2 resistenza 1k; formato 0805 |
| D203 | rosso | F29 | S41 | OK K1 GND/A2 resistenza 1k; formato 0805 |
| J201 | USB-C | F06 | S17 | OK pinout/package; deroga Type-C D29 |
| Q201 | AO3400A | F15 | S10 | OK G1/S2/D3; scarica con pull-down |
| R201 | 100 1206 | F26 | S29 | Riserva potenza scarica100Ω/0,25W |
| R202 | 56k | F07 | S29 | OK Rp56k a VBUS; deroga D29/D30 |
| R203 | 56k | F07 | S29 | OK Rp56k a VBUS; deroga D29/D30 |
| R204 | 100k | F07 | S31 | OK pull-up/down; stato logico e rail coerenti |
| R205 | 100k | F07 | S31 | OK pull-up/down; stato logico e rail coerenti |
| R206 | 1k | F07 | S30 | OK limitazione LED1k, carico GPIO modesto |
| R207 | 1k | F07 | S30 | OK limitazione LED1k, carico GPIO modesto |
| SW201 | porta on/off | F27 | S20 | OK SMD G7mm; coppie contatti e RC |
| U201 | PI3USB4002A | F30 | S09 | R03-05; pinout ZUA e lato protetto corretti |
| C301 | 100n | F01 | S32 | OK100nF50V0603; disaccoppiamento rail indicato |
| C302 | 22u 10V | F03 | S36 | OK22µF10V0805 suVBUS; Ceff/transitori da provare |
| C303 | 100n | F01 | S32 | OK100nF50V0603; disaccoppiamento rail indicato |
| C304 | 100n | F01 | S32 | OK100nF filtro CC/pulsante,0603 |
| C306 | 1u 25V | F01 | S33 | OK1µF25V0603; rete REF/bypass coerente |
| D302 | verde | F29 | S40 | OK K1 GND/A2 resistenza 1k; formato 0805 |
| D303 | rosso | F29 | S41 | OK K1 GND/A2 resistenza 1k; formato 0805 |
| J301 | USB-C | F06 | S17 | OK pinout/package; deroga Type-C D29 |
| Q301 | AO3400A | F15 | S10 | OK G1/S2/D3; scarica con pull-down |
| R301 | 100 1206 | F26 | S29 | Riserva potenza scarica100Ω/0,25W |
| R302 | 56k | F07 | S29 | OK Rp56k a VBUS; deroga D29/D30 |
| R303 | 56k | F07 | S29 | OK Rp56k a VBUS; deroga D29/D30 |
| R304 | 100k | F07 | S31 | OK pull-up/down; stato logico e rail coerenti |
| R305 | 100k | F07 | S31 | OK pull-up/down; stato logico e rail coerenti |
| R306 | 1k | F07 | S30 | OK limitazione LED1k, carico GPIO modesto |
| R307 | 1k | F07 | S30 | OK limitazione LED1k, carico GPIO modesto |
| SW301 | porta on/off | F27 | S20 | OK SMD G7mm; coppie contatti e RC |
| U301 | PI3USB4002A | F30 | S09 | R03-05; pinout ZUA e lato protetto corretti |
| C401 | 100n | F01 | S32 | OK100nF50V0603; disaccoppiamento rail indicato |
| C402 | 22u 10V | F03 | S36 | OK22µF10V0805 suVBUS; Ceff/transitori da provare |
| C403 | 100n | F01 | S32 | OK100nF50V0603; disaccoppiamento rail indicato |
| C404 | 100n | F01 | S32 | OK100nF filtro CC/pulsante,0603 |
| C406 | 1u 25V | F01 | S33 | OK1µF25V0603; rete REF/bypass coerente |
| D402 | verde | F29 | S40 | OK K1 GND/A2 resistenza 1k; formato 0805 |
| D403 | rosso | F29 | S41 | OK K1 GND/A2 resistenza 1k; formato 0805 |
| J401 | USB-C | F06 | S17 | OK pinout/package; deroga Type-C D29 |
| Q401 | AO3400A | F15 | S10 | OK G1/S2/D3; scarica con pull-down |
| R401 | 100 1206 | F26 | S29 | Riserva potenza scarica100Ω/0,25W |
| R402 | 56k | F07 | S29 | OK Rp56k a VBUS; deroga D29/D30 |
| R403 | 56k | F07 | S29 | OK Rp56k a VBUS; deroga D29/D30 |
| R404 | 100k | F07 | S31 | OK pull-up/down; stato logico e rail coerenti |
| R405 | 100k | F07 | S31 | OK pull-up/down; stato logico e rail coerenti |
| R406 | 1k | F07 | S30 | OK limitazione LED1k, carico GPIO modesto |
| R407 | 1k | F07 | S30 | OK limitazione LED1k, carico GPIO modesto |
| SW401 | porta on/off | F27 | S20 | OK SMD G7mm; coppie contatti e RC |
| U401 | PI3USB4002A | F30 | S09 | R03-05; pinout ZUA e lato protetto corretti |

## Appendice B — nomi esatti delle impronte

| ID | Impronta assegnata nello schema |
|---|---|
| F01 | `Capacitor_SMD:C_0603_1608Metric` |
| F02 | `Capacitor_THT:CP_Radial_D6.3mm_P2.50mm` |
| F03 | `Capacitor_SMD:C_0805_2012Metric` |
| F04 | `Diode_SMD:D_SMA` |
| F05 | `Diode_SMD:D_SOD-123` |
| F06 | `Connector_USB:USB_C_Receptacle_GCT_USB4105-xx-A_16P_TopMnt_Horizontal` |
| F07 | `Resistor_SMD:R_0603_1608Metric` |
| F08 | `Resistor_SMD:R_2512_6332Metric` |
| F09 | `Package_TO_SOT_SMD:SOT-223-3_TabPin2` |
| F10 | `lcsc_footprints:C3662799_VQFN-10_L2_0-W2_0-P0_45-TL` |
| F11 | `Package_TO_SOT_SMD:SOT-23-6` |
| F12 | `Fuse:Fuse_1812_4532Metric` |
| F13 | `Connector_PinHeader_2.54mm:PinHeader_1x04_P2.54mm_Vertical` |
| F14 | `lcsc_footprints:QFN-64-1EP_9x9mm_P0.5mm_EP4.7x4.7mm_ThermalVias_0.3mm` |
| F15 | `Package_TO_SOT_SMD:SOT-23` |
| F16 | `Crystal:Crystal_SMD_3225-4Pin_3.2x2.5mm` |
| F17 | `Connector_PinHeader_1.27mm:PinHeader_2x05_P1.27mm_Vertical_SMD` |
| F18 | `Connector_PinHeader_2.54mm:PinHeader_1x03_P2.54mm_Vertical` |
| F19 | `Package_QFP:LQFP-64_10x10mm_P0.5mm` |
| F20 | `Package_SON:VSON-10-1EP_3x3mm_P0.5mm_EP1.65x2.4mm_ThermalVias` |
| F21 | `Package_DFN_QFN:Texas_RGV0016A_VQFN-16-1EP_4x4mm_P0.65mm_EP2.1x2.1mm` |
| F22 | `Connector:Banana_Cliff_FCR7350B_S16N-PC_Horizontal` |
| F23 | `Connector:Banana_Cliff_FCR7350R_S16N-PC_Horizontal` |
| F24 | `Jumper:SolderJumper-3_P1.3mm_Bridged12_RoundedPad1.0x1.5mm` |
| F25 | `Relay_THT:Relay_SPDT_Omron-G5Q-1` |
| F26 | `Resistor_SMD:R_1206_3216Metric` |
| F27 | `Button_Switch_SMD:SW_SPST_PTS645Sx43SMTR92` |
| F28 | `Package_SO:SOIC-16_3.9x9.9mm_P1.27mm` |
| F29 | `LED_SMD:LED_0805_2012Metric` |
| F30 | `lcsc_footprints:C3657361_UQFN-10_L2_0-W1_5-P0_50-TL` |
| F31 | `Capacitor_SMD:CP_Elec_6.3x5.8` |
| F32 | `Connector_USB:USB_A_Kycon_KUSBX-AS1N-B_Horizontal` |

## Appendice C — identificazione dei fogli verificati

SHA-256 della copia usata per netlist/ERC. **Controllo finale: tutti e nove i fogli presenti nel progetto hanno gli stessi hash della copia verificata.** Le modifiche concorrenti alla PCB e alla documentazione non sono attribuite a questa revisione.

| Foglio | SHA-256 |
|---|---|
| devhub.kicad_sch | `d84786edbc49daf06ebdf1168f71a4e9dc86ba10562fbf7389e6cc69e9e5a1a4` |
| hub.kicad_sch | `8216fca5d63961defff90617ca58b0bfadedf5e522badaa7ccc3d2b4735537c2` |
| mcu.kicad_sch | `c38875800b2fcdef5d8bc66a10cb2ebff20e45a2c01ccd21b8b706ac47ffefca` |
| port_a.kicad_sch | `91f166699bd7ac84b9cdc5ce2f0b93a1e7d053b951814cc0be8fc355f71513f6` |
| port_c.kicad_sch | `62cba05052292a3e0cda6498d5d5fffbd48fe061662bbf876afb36291dcbe672` |
| power_in.kicad_sch | `2b34fb0dbccd733e73c0ffae78b1e4a6ce90431b8fec77c5cbf3058229dc3599` |
| relays.kicad_sch | `7727ddf14500e19e31b9891c1321ee155aa22865315a70e7c1266a9aa80625bf` |
| switches.kicad_sch | `3d85d470bb9cf364ae4e66845910fbbbe9dca0ba836c0eb7ace83e4e37439c1a` |
| ui.kicad_sch | `65d3f03c5eb615ce0a6ac0bb50a05cee57c39143b464bed230b52673f2366093` |

## Ordine di chiusura consigliato

1. Risolvere D1 e il comportamento di VBUS_DET a hub spento.
2. Scegliere una soluzione per J501/RF-5 e per shunt/RF-6.
3. Chiudere courtyard, differenze dei land pattern e riserve sui codici esatti.
4. Nella sessione che modifica lo schema: ERC e confronto netlist prima/dopo; quindi sincronizzazione controllata della PCB.
5. Sul prototipo: termica LDO/hub, VBUS ai limiti, scarica ripetuta, misure in temperatura, reset/brownout/DFU, enumerazione e transitori USB.
