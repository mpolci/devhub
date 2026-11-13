# devhub Python library

Use the library when a task needs more than a few CLI calls: a test loop that flashes, power
cycles and checks the board many times, or reactions to events. For single actions the CLI is
simpler. Run scripts with `host/.venv/bin/python` (see SKILL.md, "Finding the tools").

The permission rules of SKILL.md apply to scripts too: a script that saves settings, resets the
hub or reboots the devhub needs the user's approval like the CLI command.

## Basics

```python
from devhub import Devhub, DevhubError, CommandError, attached_devices

with Devhub() as hub:                  # Devhub("SERIAL") when several are connected
    info = hub.info()                  # .serial, .n_ports, .fw_version, ...
    st = hub.status()
    for port in st.ports:              # numbered from 1
        print(port.port, port.enabled, port.cycle_active, port.faulted,
              port.current_ma, port.charge_uah)
    hub.set_port(2, True)              # on; on a faulted port this clears the fault first
    hub.set_port(2, False)
    hub.cycle_port(2, off_ms=1000)     # 500-60000 ms; returns at once, the cycle runs in the devhub
    hub.clear_fault(2)                 # remove the fault latch, the port stays off
    hub.set_all(True)
    hub.pulse_relay(1, ms=2000)        # open the relay line for 2 s
    hub.set_relay(1, disconnected=True)
```

`to_dict()` on `status()`, `info()`, ports and events gives the same structures as
`devhub status --json` and `devhub watch --json`.

Errors: `CommandError` when the device refuses a command (e.g. a value out of range),
`DevhubError` for communication problems (no devhub, cannot open it).

Keep the device open only while needed (`with` block). On macOS only one program at a time can
open it: a script holding it blocks the CLI, and the reverse.

## Attached devices and waiting for enumeration

`attached_devices(serial)` returns `{port: AttachedDevice}` for the devices the computer sees on
the devhub ports, or `None` on platforms without support (Windows). `AttachedDevice` has `vid`,
`pid`, `manufacturer`, `product`, `serial`, `ttys` (serial ports), `matches(text)`, `describe()`
and `session`, which changes at every enumeration.

To wait for the board to come back after a cycle, compare `session` with the one seen before, as
`devhub port <n> cycle --wait` does:

```python
import time
from devhub import Devhub, attached_devices

def cycle_and_wait(hub, port, off_ms=1000, timeout=10.0):
    serial = hub.info().serial
    before = (attached_devices(serial) or {}).get(port)
    hub.cycle_port(port, off_ms)
    deadline = time.monotonic() + timeout
    seen = None
    while time.monotonic() < deadline:
        state = hub.status().ports[port - 1]
        if state.faulted:
            raise RuntimeError(f"port {port} latched in fault: {state.to_dict()['fault_causes']}")
        if state.enabled and not state.cycle_active:
            dev = (attached_devices(serial) or {}).get(port)
            if dev and (before is None or dev.session != before.session):
                if dev == seen:          # serial ports appear shortly after: two equal reads
                    return dev
                seen = dev
        time.sleep(0.2)
    raise TimeoutError(f"no device enumerated on port {port} within {timeout} s")

with Devhub() as hub:
    dev = cycle_and_wait(hub, 2)
    print(dev.describe(), dev.ttys)
```

## Events

```python
with Devhub() as hub:
    for event in hub.events(timeout=5):   # stops after 5 s without events
        print(event.to_dict())            # e.g. {"event": "port_fault", "port": 2, "fault_causes": [...], "origin": "fault"}
```

Events: `port_changed`, `port_fault`, `port_warning`, `relay_changed`, `power_changed`; `origin`
is `command`, `button`, `timer` (end of a cycle), `fault` or `startup`. A `button` origin means
the user pressed a front panel button.

The full method list is in `host/devhub/device.py`; the protocol in
`docs/protocollo-controllo.md`.
