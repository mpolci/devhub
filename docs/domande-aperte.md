# Domande aperte

Decisioni che cambiano il progetto e che è meglio prendere prima dello schema. Per ciascuna è
indicata l'ipotesi adottata nel frattempo.

1. **Relè ausiliari: cosa devono commutare?** Tensione e corrente massime (es. 30 V / 5 A DC, o
   anche rete 230 V?), e se servono SPDT (COM/NO/NC, 6 boccole) o basta SPST (4 boccole).
   *Ipotesi: SPDT 10 A, max 30 VDC / 250 VAC, 6 boccole.*
2. **Corrente per porta e pubblicità su CC.** Limite hardware a 2,5 A con Rp che dichiara 1,5 A
   (22 kΩ) oppure 3 A (10 kΩ)? Serve davvero che tutte e 5 le porte possano erogare 2 A insieme
   (alimentatore da 65 W) o basta un budget totale minore (45 W)?
   *Ipotesi: limite 2,5 A, Rp 22 kΩ, alimentatore 65 W.*
3. **Misura di corrente per porta**: la vuoi (INA3221, ~+6 € e un po' di spazio) o basta il flag di
   sovracorrente? *Ipotesi: sì.*
4. **Pulsanti fisici per porta** per staccare/riattaccare a mano dal banco? *Ipotesi: sì, uno per
   porta, più LED.*
5. **Connettore upstream**: USB-C o USB-B? *Ipotesi: USB-C.*
6. **MCU**: RP2040 va bene o preferisci STM32 (hai la toolchain ARM, vale per entrambi)?
   *Ipotesi: RP2040.*
7. **Produzione**: JLCPCB con assemblaggio SMT (vincola la scelta ai componenti a catalogo LCSC)
   oppure assemblaggio a mano / altro servizio? Questo decide tra USB2517 e FE2.1, tra STUSB4500 e
   CH224K. *Ipotesi: JLCPCB + assemblaggio per i QFN.*
8. **Sistemi operativi host** su cui deve girare la CLI (macOS sicuramente; Linux? Windows?).
   *Ipotesi: tutti e tre, via HID.*
9. **VID/PID USB**: richiedere un PID su pid.codes (gratuito, progetto open) o usare il VID Raspberry
   Pi con PID riservato per RP2040? Serve decidere la licenza del progetto per pid.codes.
10. **Licenza** del repository (hardware: CERN-OHL-P/S; firmware e host: MIT/Apache/GPL).
    *Nessuna licenza aggiunta finché non decidi.*
11. **Porta interna di scorta** (DS7 del hub): la esponiamo su un header interno? *Ipotesi: sì.*
12. **Scatola e formato**: dimensioni, montaggio su guida, stampa 3D? Nessuna ipotesi.
