# enclosure

Scatola stampata in FDM (PETG) per il PCB di `hardware/`, progetto FreeCAD 26.3 in Part Design,
parametrico tramite il VarSet `Params` (D102).

```
enclosure/devhub-enclosure.FCStd   progetto: Params, corpi Lid, BottomPlate, LightPipes, collegamento PcbRef
enclosure/pcb/devhub-pcb.FCStd     riferimento del PCB (STEP di KiCad + segnaposto), generato
enclosure/pcb/devhub-pcb.step      STEP esportato da KiCad, escluso da git
enclosure/scripts/                 script di costruzione, modifica e verifica
enclosure/scripts/edits/           modifiche da script già applicate al file, in ordine
enclosure/print/                   STL orientati per la stampa e progetto Bambu Studio
```

## Struttura

Vista dall'alto con il fronte in basso: X dal bordo sinistro del PCB, Y dal bordo frontale,
Z = 0 sulla faccia superiore del PCB. Le quote dei componenti vengono dallo STEP esportato da KiCad.

| Corpo | Contenuto |
|---|---|
| `Lid` | Coperchio profondo: piano superiore e quattro pareti. Colonnine `Standoffs` sopra i fori H1–H4 del PCB, con i fori pilota delle viti. Fori `ButtonHoles` per i cappucci con smusso a 45° (`ButtonChamfers`), `LightPipeHoles` e `Shrouds` opachi attorno alle guide luce. Aperture USB-C sul corpo della presa, con la parete assottigliata attorno: un ribasso unico per le quattro porte frontali (`FrontUsbRecess`) e uno per porta sul retro (`BackUsbRecesses`), con i fianchi inclinati a 45°. Asole a U aperte verso il basso per le boccole, feritoie a cavallo del PCB, con le estremità alla stessa distanza dalla faccia superiore e dal fondo esterno della scatola chiusa: `VentSlots` sui fianchi (14 per lato) e `BackVentSlots` sul retro (8, fra J1 e J2, vicino all'LDO U1 e a D2); ogni parete ha la sua tasca, che non raggiunge le colonnine interne, scalino `Rebate` sul bordo interno delle pareti |
| `BottomPlate` | Fondo con un bordo `Lip` che entra nello scalino delle pareti, colonnine cave `Columns` sotto H1–H4 con sede delle teste (`ScrewHeadBores`) e foro passante (`ScrewHoles`), due `Tongues` che chiudono dal basso le asole delle boccole e reggono i manicotti in una culla |
| `LightPipes` | Sei guide luce Ø 3 mm (cinque LED frontali e D93), stampate in PETG trasparente insieme al coperchio |
| `PcbRef` | Collegamento a `pcb/devhub-pcb.FCStd#PCB`, solo riferimento, non si stampa |

Dimensioni con i valori attuali: 106,7 × 104,4 mm in pianta, 31,9 mm di altezza (coperchio 29,2,
gioco 0,2, fondo 2,5). Il gioco fra PCB e pareti è di 0,2 mm su tre lati e di 2,5 mm a destra
(`SideGapLeft`, `SideGapRight`), dove il ribasso frontale delle USB deve finire prima
dell'arrotondamento dell'angolo. I cappucci dei pulsanti
sono incassati di 0,3 mm, le guide luce sono a filo. Il fondo dei ribassi USB misura 14 × 8 mm
per porta; con i fianchi a 45° il bordo in superficie è 1 mm più largo per lato, e a destra il
ribasso frontale finisce 1,5 mm prima dell'arrotondamento dell'angolo.

Fissaggio: quattro viti autofilettanti M3 × 10–12 entrano dal fondo nelle colonnine cave, passano
nei fori H1–H4 del PCB e si avvitano nei fori pilota delle colonnine del coperchio. Il PCB resta
stretto fra le colonnine del coperchio e quelle del fondo; la testa appoggia 3 mm sotto il PCB.
L'altezza del fondo la danno le colonnine: fra fondo e pareti restano 0,2 mm, nascosti dallo
scalino, così le viti serrano il PCB senza appoggi in conflitto. Senza il fondo il PCB non è
fissato al coperchio.

Montaggio: si capovolge il coperchio, si cala il PCB a faccia in giù con i manicotti delle
boccole nelle asole, si appoggia il fondo con il bordo nello scalino e si avvitano le quattro
viti. Il PCB entra solo lungo Z, quindi nessuna parte del coperchio può stare sotto il profilo
del PCB (lo controlla `verify.py`).

Stampa: coperchio capovolto con la faccia superiore sul piatto, guide luce come seconda parte
dello stesso oggetto in PETG trasparente; fondo con la faccia esterna sul piatto. Nessuna parte
richiede supporti: le asole a U e le feritoie verticali restano aperte verso l'alto, le aperture
USB-C hanno un ponte di 9,3 mm, i fianchi dei ribassi e lo smusso dei pulsanti sono a 45°.

## Parametri

Tutto è guidato dal VarSet `Params`, gruppi principali:

| Gruppo | Contenuto |
|---|---|
| `PCB` | dimensioni e spessore del PCB, altezza del componente più basso (K1, 15,8 mm), quote di LED, cappucci, USB-C e boccole |
| `Shell` | spessori di pareti e piano, giochi attorno al PCB, raggio degli angoli, spessore del fondo |
| `Buttons`, `LightPipes`, `Mounting`, `Openings` | posizioni con il riferimento del componente nel nome (`SW401X`, `D93Y`, `H1X`, `J5X`…) e diametri, giochi, profondità dei fori |
| `Ventilation` | feritoie (larghezza, passo, numero per lato `VentCount` e sul retro `BackVentCount`, distanza `VentEndMargin` delle estremità dalle facce esterne superiore e inferiore); le file sono centrate da sole |
| `Derived` | quote calcolate con espressioni (`TopZ`, `BottomZ`, profilo esterno…): non si modificano a mano |

Ogni sketch è completamente vincolato e le quote sono espressioni sui parametri. Se il PCB cambia
basta aggiornare i parametri e rigenerare il riferimento del PCB.

## Lavoro in alternanza con le modifiche manuali

Il file si modifica sempre a partire dallo stato corrente, non si rigenera:

- a mano in FreeCAD, salvando normalmente;
- da script, con `scripts/fcedit.py`, a file chiuso in FreeCAD:

```bash
python3 enclosure/scripts/fcedit.py enclosure/devhub-enclosure.FCStd modifica.py
```

Lo script di modifica gira in `freecadcmd` con `doc`, `Params`, `App` e `fclib` già definiti. Per
esempio `Params.WallThickness = 2.4`, oppure nuove feature con gli helper di `fclib.py`.
`freecadcmd` salverebbe senza `GuiDocument.xml`, perdendo colori, trasparenze e visibilità.
`fcedit.py` lo conserva e lo rimette nell'archivio nell'ordine in cui FreeCAD lo rilegge:

- aggiorna la visibilità cambiata dalla modifica, compreso lo spostamento del Tip di un corpo;
- dà alle nuove feature di Part Design lo stile della feature su cui si appoggiano, come fanno
  i comandi della GUI;
- toglie gli stili degli oggetti eliminati.

Se il ricalcolo dà errori il file non viene toccato. Se il file cambia su disco durante la
modifica, la scrittura viene rifiutata. Prima di ogni scrittura una copia va in
`enclosure/backup/`, escluso da git. Con `--dry-run` esegue e controlla senza salvare.

`scripts/build_enclosure.py` crea il file solo se non esiste e non si usa per le modifiche. Le
modifiche da script applicate dopo la creazione sono in `scripts/edits/`, numerate: rieseguite in
ordine dopo `build_enclosure.py` ridanno la geometria attuale, ma non le modifiche fatte a mano.

## Riferimento del PCB

```bash
/Applications/KiCad/KiCad.app/Contents/MacOS/kicad-cli pcb export step --force --subst-models --user-origin 60.5x147.5mm -o enclosure/pcb/devhub-pcb.step hardware/devhub/devhub.kicad_pcb
```

```bash
/Applications/FreeCAD.app/Contents/MacOS/FreeCAD enclosure/scripts/build_pcb_ref.py
```

L'origine dell'esportazione è l'angolo anteriore sinistro del PCB. Le librerie di KiCad non hanno
i modelli 3D di K1 (Omron G5Q-1) e dei test point Keystone 5000 (TP1, TP10): `build_pcb_ref.py`
li sostituisce con l'ingombro massimo del datasheet. Aggiunge anche i cappucci dei pulsanti
(Ø 6 mm, 10 mm sopra il PCB). Il riferimento è interamente generato, quindi si può rifare dopo
ogni modifica del PCB.

## Esportazione per la stampa

```bash
/Applications/FreeCAD.app/Contents/Resources/bin/freecadcmd -c "exec(open('enclosure/scripts/export_print.py').read())"
```

Scrive in `enclosure/print/` tre STL già orientati, centrati in X/Y e con Z = 0 sul piatto:

- `lid.stl`: il coperchio capovolto, con la faccia superiore sul piatto.
- `light-pipes.stl`: le guide luce, con la stessa trasformazione del coperchio. In Bambu
  Studio vanno aggiunte come parte dell'oggetto del coperchio, non come oggetto separato,
  così restano nei loro fori e si assegnano al filamento trasparente.
- `bottom-plate.stl`: il fondo, con la faccia esterna sul piatto.

Il documento FreeCAD viene solo letto. Il progetto Bambu Studio (`devhub-enclosure.3mf`) si
gestisce dalla GUI: dopo una nuova esportazione le mesh si aggiornano con "Ricarica dal disco",
che rilegge gli STL da cui sono state importate.

## Verifica

```bash
/Applications/FreeCAD.app/Contents/Resources/bin/freecadcmd -c "exec(open('enclosure/scripts/verify.py').read())"
```

Controlla che ogni corpo sia un solido valido, che nessun taglio intacchi le colonnine di
coperchio e fondo, che le parti stampate non si sovrappongano fra
loro né ai componenti del PCB, che il PCB entri nel coperchio lungo Z (manicotti compresi) e
riporta i giochi minimi. Stato attuale: nessuna interferenza, montaggio libero; gioco di 0,15 mm
fra manicotti e asole, 0,23 mm fra prese USB-C e aperture, 0,3 mm attorno a cappucci e LED,
1,7 mm fra K1 e fondo; le colonnine del fondo toccano il PCB.

## Da verificare sulla prima stampa

- Accoppiamento dei fori pilota da 2,5 mm con le viti autofilettanti M3 nel PETG.
- Giochi di 0,2 mm fra i bordi anteriore, posteriore e sinistro del PCB e le pareti.
- Innesto completo delle spine USB-C con la parete a 1,2 mm dal fronte delle prese, e ingombro
  dei pressofusi nei ribassi da 8 mm di altezza.
- Accoppiamento del bordo del fondo (0,85 mm) nello scalino, con 0,15 mm di gioco.
- Quota dei cappucci rispetto alla superficie del coperchio.
- Aderenza delle guide luce al coperchio e diffusione della luce dei LED.
