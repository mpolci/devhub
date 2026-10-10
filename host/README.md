# host

Lato PC, in Python. Non ancora iniziato. Due livelli:

- **`devhub` libreria**: API Python generica e multipiattaforma sopra `hidapi` (apre il dispositivo,
  espone stato, porte, relè, eventi). Usabile direttamente negli script di test.
- **`devhub` CLI**: comando da terminale per macOS costruito sulla libreria.

Vedi i comandi previsti in [../docs/protocollo-controllo.md](../docs/protocollo-controllo.md).
