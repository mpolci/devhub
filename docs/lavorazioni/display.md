# Display di stato

Documento tecnico per l'inserimento del **Newhaven NHD-2.4-240320AF-CTXP**.
Data: 2026-10-12. Orientamento concordato: mantenere STM32G0B1RET6 e usare questo
pannello come candidato di riferimento. Collegamenti e valori sotto sono una proposta
da implementare e verificare, non lo stato attuale dello schema.

## Funzione e perimetro

Il display mostra correnti, stati delle cinque porte, budget d'ingresso e cause dei fault.
Una schermata riepilogativa deve rendere visibile la porta in errore senza menu o pagine
alternate. I pulsanti mantengono la funzione attuale; non sono previsti touch o nuovi
comandi di navigazione.

Si propone di eliminare i cinque LED rossi e le loro resistenze, affidando al display le
cause dei fault. Restano i LED verdi delle porte, i LED dei relè, alimentazione e host.
La rimozione libera fra gli altri PB3, PB4 e PB5. Il display dipende dal firmware, come i
LED rossi attuali; quando l'eFuse interrompe l'alimentazione si spegne anche il display.

## Pannello

| Parametro | Valore |
|---|---|
| Codice | NHD-2.4-240320AF-CTXP, senza suffisso touch |
| Tecnologia | TFT IPS trasmissivo, normalmente nero, antiriflesso |
| Diagonale / risoluzione | 2,4 pollici, 240 × 320 pixel |
| Area attiva | 36,72 × 48,96 mm; utilizzabile in orizzontale |
| Ingombro cornice | circa 42,8 × 59,9 × 2,55 mm, flat escluso |
| Controller | ST7789VI, memoria immagine integrata |
| Interfaccia scelta | SPI a tre fili, frame a 9 bit |
| Alimentazione VDD | 3,3 V nominali, campo 2,4–3,6 V |
| Alimentazione VDDI | collegabile alla nostra 3,3 V, campo 1,65–3,6 V |
| Corrente IDD dichiarata | 6 mA tipici, 12 mA massimi a VDD = 3,3 V |
| Retroilluminazione | quattro LED con anodo comune e catodi separati |
| Retroilluminazione nominale | 80 mA totali, caduta 3,1 V tipica (2,7–3,5 V a 80 mA) |
| Luminosità nominale | 350 cd/m² tipici a 80 mA |
| Connessione | FPC 40 contatti, passo 0,5 mm |

La retroilluminazione è necessaria per la lettura normale. La versione CSXP, più luminosa,
non è quella valutata in questo documento. Per disegno, orientamento contatti e quote
vincolanti usare il datasheet del pannello, non le immagini del distributore.

## Collegamento al MCU

Proposta compatibile con le assegnazioni documentate in [pinout-mcu.md](pinout-mcu.md),
previa rimozione dei LED rossi. Verificare nuovamente sullo schema al momento dell'implementazione.

| Rete proposta | Pin MCU | Funzione | Pin pannello |
|---|---|---|---|
| LCD_SCK | PB3, pin 57 | SPI1_SCK, AF0; ora LED_FLT_P1 | 11, DCX/SCL |
| LCD_MOSI | PB5, pin 59 | SPI1_MOSI, AF0; ora LED_FLT_P3 | 9, SDA |
| LCD_CS_N | PB4, pin 58 | GPIO, selezione attiva bassa; ora LED_FLT_P2 | 10, CSX |
| LCD_RST_N | PA6, pin 23 | GPIO, reset attivo basso; libero | 30, RESX |
| LCD_BL_PWM | PB0, pin 27 | TIM3_CH3, AF1; libero | gate MOSFET, non direttamente al pannello |

SPI a tre fili significa **SCL, SDA e CS**: non c'è un GPIO D/C separato.
Ogni comando/dato è trasmesso in un frame di 9 bit: bit iniziale D/C (0 comando,
1 dato), poi gli 8 bit MSB first. Il MCU supporta lunghezze da 4 a 16 bit
(RM0444, SPI_CR2.DS): usare 9 bit, non emulare un nono impulso con GPIO.

Ulteriori connessioni, secondo lo schema SPI del pannello:

- Pin 7 VDD e 8 VDDI alla 3V3; 100 nF e 2,2 µF locali proposti vicino al connettore.
- Pin 1, 33 e 39 a GND.
- IM0 (31) e IM2 (32) alla 3V3 per selezionare SPI a tre fili.
- WRX (12), RDX (13) e DB0–DB15 (14–29) a GND.
- SDO (6) non collegato: il progetto iniziale usa soltanto scritture.
- TE (40) non collegato: niente sincronizzazione con il tearing per la prima versione.
- Pin 2–5 NC: non collegati.
- Pull-up da 10 kΩ proposto su LCD_CS_N e pull-down da 10 kΩ su LCD_RST_N.

Usare SPI mode 0 come impostazione iniziale, da verificare sui diagrammi temporali;
partire con clock basso (1–4 MHz). La frequenza definitiva deve rispettare sia il
datasheet del pannello sia quello del controller e il cablaggio reale.
Non assumere che il limite del solo ST7789VI sia quello del modulo completo.

Questi segnali non condividono il bus I2C dei sensori/hub. Per reset e DFU, verificare
le assegnazioni del bootloader AN2606: PB0 mantiene la retroilluminazione spenta grazie
al pull-down sul gate, CS resta alto e RESX basso finché il firmware non li configura.
PA6 è fra i pin SPI1 usati dal bootloader: provare il comportamento sul prototipo.
Non spostare i comandi di porte, relè o reset del hub.

## Retroilluminazione dalla 5V_SYS

Prelevare l'alimentazione **dopo eFuse e shunt d'ingresso**, sulla 5V_SYS.
Non usare la VBUS upstream del PC né la VBUS commutata di una porta.
In questo modo la retroilluminazione è protetta e compresa nella corrente totale misurata.

Circuito proposto:

```text
5V_SYS ---------------------- LED-A (38)
LED-K1 (34) -- R_BL1 --+
LED-K2 (35) -- R_BL2 --+
LED-K3 (36) -- R_BL3 --+------- drain Q_BL
LED-K4 (37) -- R_BL4 --+        source -------- GND

PB0 -- R_GATE -- gate Q_BL
                  |
                R_PD
                  |
                 GND
```

- Q_BL: AO3400A, già presente fra i componenti del progetto, pilotabile a 3,3 V.
- R_BL1–R_BL4: **180 Ω, 1 %, almeno 0,125 W**, valore iniziale conservativo proposto.
- R_GATE: 100 Ω; R_PD: 100 kΩ.
- PWM iniziale proposto: circa 1 kHz, duty cycle configurabile; inizialmente 0 %.

Una resistenza per catodo limita ciascun ramo: non mettere una sola resistenza comune
ai quattro LED. Il MOSFET permette spegnimento e PWM senza far passare corrente nei GPIO.
Non occorre un driver LED integrato per questa soluzione; un driver a corrente costante
resta un'alternativa se serve luminosità stabile al variare della 5V_SYS.

### Stima elettrica, non specifica garantita

Con 5,0 V e Vf = 3,1 V, trascurando la piccola caduta sul MOSFET:

```text
I_ramo = (5,0 - 3,1) / 180 = 10,6 mA
I_backlight = 4 × I_ramo = 42,2 mA
P_resistenza = (5,0 - 3,1)² / 180 = 20 mW
P_ingresso_backlight = 5,0 × 42,2 mA = 0,211 W
```

Questo valore è inferiore agli 80 mA nominali: non si può promettere la luminosità
nominale del pannello. Usando, solo come controllo preliminare, 6,0 V e Vf = 2,7 V con
R minima 178,2 Ω, risultano 18,5 mA per ramo, 74,1 mA totali e 61 mW per resistenza.
I 6,0 V sono un inviluppo di calcolo, non un limite già certificato della nostra 5V_SYS.

Prima di approvare i valori, verificare soglie/tolleranze/transitori dell'eFuse e Vf dei
LED alla corrente effettiva e in temperatura: i 2,7–3,5 V del datasheet sono dichiarati
alla corrente nominale e non costituiscono da soli una caratterizzazione di ogni ramo.
Misurare correnti separate e leggibilità al minimo e massimo ingresso. Non ridurre le
resistenze soltanto per raggiungere 80 mA tipici senza rifare queste verifiche.

Con PWM il consumo medio della retroilluminazione diminuisce approssimativamente con
il duty cycle; la corrente istantanea resta quella imposta da resistenze e tensione.
La logica resta sulla 3V3: circa 6 mA tipici aggiuntivi, anziché caricare l'LDO anche
con la retroilluminazione. Non sottrarre una seconda quota fissa dal budget firmware:
la misura dello shunt d'ingresso comprende già questi carichi (D40).

## Distinta preliminare

| Elemento | Quantità | Valore / candidato |
|---|---:|---|
| Pannello | 1 | NHD-2.4-240320AF-CTXP |
| Connettore FPC | 1 | Molex 54132-4062 o equivalente, 40 poli, passo 0,5 mm |
| Disaccoppiamento logica | 2 | 100 nF + 2,2 µF |
| Resistenze LED | 4 | 180 Ω 1 %, ≥0,125 W, da validare |
| MOSFET | 1 | AO3400A |
| Resistenza gate | 1 | 100 Ω |
| Pull-down gate | 1 | 100 kΩ |
| Pull-up CS | 1 | 10 kΩ |
| Pull-down reset | 1 | 10 kΩ |
| **Totale oltre al pannello** | **12** | proposta con stato definito anche durante reset |

Il conteggio non include fissaggi, eventuale prolunga/adattatore FPC, né resistenze serie
sui segnali che potrebbero servire dopo le prove di integrità. Eliminando cinque LED
rossi e cinque resistenze si tolgono dieci componenti: l'incremento netto della proposta
è quindi di tre elementi, pannello incluso, senza confrontarne area o costo.

Il Molex è il riferimento consigliato dal produttore, non una scelta meccanica congelata:
verificare lato contatti, spessore del flat, verso di inserzione, accesso alla slitta e
orientamento del display rispetto al PCB. Fissare il pannello alla scatola senza caricare
meccanicamente il flat. Evitare prolunghe lunghe e tenere il PWM lontano da USB e shunt.

## Firmware

1. All'avvio tenere luce spenta, CS alto e reset basso; attendere la stabilità della 3V3.
2. Rilasciare il reset rispettando i tempi del pannello/controller. Inviare la sequenza
   specifica Newhaven, adattando l'esempio parallelo al trasporto SPI a 9 bit.
3. Impostare RGB565 (COLMOD), orientamento MADCTL e finestra di scrittura; rispettare i
   tempi di reset, Sleep Out e Display On. Usare almeno 120 ms dopo Sleep Out come attesa
   iniziale conservativa; riconciliare la sequenza completa con i due datasheet.
4. Cancellare la GRAM, disegnare una schermata valida e soltanto allora accendere la luce.
5. Aggiornare a bassa frequenza le sole regioni cambiate. Testo/stato e fault hanno priorità
   sui dettagli grafici; nessun menu necessario per leggere gli errori.

Il framebuffer intero RGB565 sarebbe 153.600 byte, quindi non va allocato nel MCU.
Usare piccoli buffer per righe/rettangoli e la GRAM del controller. A ogni byte di pixel
va aggiunto il bit D/C = 1: un pixel RGB565 richiede due frame da 9 bit.
Per DMA usare contenitori da 16 bit per i frame e configurare coerentemente ampiezza
degli accessi, FIFO e SPI_CR2.DS. Aspettare la fine effettiva della trasmissione SPI
prima di alzare CS, non soltanto l'interrupt di fine DMA.

Un riempimento intero richiede almeno 1.382.400 bit (circa 346 ms a 4 MHz), senza
overhead: non deve essere una chiamata bloccante nel ciclo di controllo. Suddividere
le trasmissioni e preservare le scadenze già previste per scarica VBUS, budget, pulsanti,
USB HID e reset hub. Un errore del display non deve bloccare il controllo delle porte.

## Verifiche per l'inserimento nello schema

- Confermare la meccanica e il connettore prima di assegnare il footprint.
- Confermare valori e dissipazioni della retroilluminazione sull'intero campo elettrico.
- Controllare disponibilità di TIM3_CH3 e DMA nella futura architettura firmware.
- Provare power-on, reset software, watchdog, DFU e scollegamento dell'alimentatore.
- Verificare la leggibilità con cinque porte, fault multipli e luminosità ridotta.
- Provare il controllo porte durante aggiornamenti grafici e trasferimenti USB.
- Aggiornare schema, requisiti LED, architettura e pinout insieme all'implementazione;
  non modificare ora la tabella del pinout, che descrive lo schema esistente.
- Dopo le future modifiche KiCad: ERC senza violazioni e confronto netlist prima/dopo.
  Etichette esplicite su ogni rete, comprese le quattro reti fra LED e resistenze.

## Riferimenti

Datasheet locali (la cartella `datasheets/` è esclusa da git):

- [Pannello Newhaven](../datasheets/nhd-2.4-240320af-ctxp.pdf): pinout, schema SPI,
  disegno meccanico, alimentazioni e inizializzazione; revisione con aggiornamento del 12 marzo 2026.
- [Controller ST7789VI](../datasheets/st7789vi.pdf): protocollo, comandi, GRAM e tempi.
- [STM32G0B1](../datasheets/DS_stm32g0b1cc.pdf): funzioni alternative PB3/PB5/PB0.
- [RM0444](../datasheets/rm0444-stm32g0x1-advanced-armbased-32bit-mcus-stmicroelectronics.pdf): SPI_CR2.DS, DMA, FIFO e timer.
- [AN2606](../datasheets/an2606-introduction-to-system-memory-boot-mode-on-stm32-mcus-stmicroelectronics.pdf): bootloader.
- [AO3400A](../datasheets/ao3400a.pdf): pilotaggio e limiti del MOSFET.

Fonti del produttore, consultate il 2026-10-12:

- https://newhavendisplay.com/content/specs/NHD-2.4-240320AF-CTXP.pdf
- https://support.newhavendisplay.com/hc/en-us/articles/10814990300823-ST7789VI
- https://www.molex.com/en-us/products/part-detail/541324062

Il disegno specifico del connettore va acquisito e verificato prima del footprint.
