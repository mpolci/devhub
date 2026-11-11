"""Command line interface: `devhub ...` or `python -m devhub ...`."""

from __future__ import annotations

import argparse
import json
import sys
from typing import List, Optional

from . import protocol as p
from .device import Devhub, DevhubError, list_devices
from .protocol import RelayState

EXIT_OK = 0
EXIT_ERROR = 1
EXIT_FAULT = 2  # the command worked but a port is latched in fault

# Help texts. Each command has a usage line, a one-line summary for the short guide
# (`devhub` with no arguments) and a detailed description for `devhub help [command]`.
_COMMANDS = [
    ("list", "list", "list connected devhubs", """
Print the serial number of every devhub connected to this computer, one per line.
Exit code 1 when none is found.
"""),
    ("info", "info", "firmware and protocol versions", """
Print protocol and firmware versions, number of ports and relays, optional
capabilities of the firmware and serial number.
"""),
    ("status", "status [--json]", "state of ports and relay", f"""
Print the system flags, the state of each port and of the relay.

Port states: off, on, cycling (power cycle in progress), FAULT (latched off).
  fault causes  overcurrent, overtemperature, generic: the port is switched off
                and stays off until 'port <n> on' or 'port <n> clear', or until
                its button is pressed.
  warnings      discharge_failed, back_voltage (the board on the port has its own
                supply), vbus_low: reported only, the port keeps working.
System flags: hub_configured (normal), input_fault (power input protection
tripped), controller_12_error / controller_34_error (port controller of ports
1-2 or 3-4 not responding).

  --json   print the status as JSON, for scripts.

Exit code {EXIT_FAULT} when a port is latched in fault.
"""),
    ("port", "port <n> on|off|cycle|clear [--ms MS]", "switch port n, power cycle it, clear a fault", f"""
Control port n (1-4, as on the front panel). VBUS and data lines are switched
together.

  on       power the port and connect its data lines. On a port latched in
           fault this is the explicit retry: the fault is cleared first.
  off      switch the port off.
  cycle    switch the port off for --ms milliseconds, then on again
           ({p.CYCLE_MIN_MS}-{p.CYCLE_MAX_MS}, default {p.CYCLE_DEFAULT_MS}). The off time starts at
           switch-off and includes the VBUS discharge. The cycle runs in the
           devhub: the command returns at once.
  clear    remove the fault latch; the port stays off.

on and off during a cycle cancel it. A cycle only removes the power that the
port supplies: a board with its own power supply keeps running, use the relay
or a reset line for it.

  devhub port 2 cycle            port 2 off for 1 s, then on
  devhub port 2 cycle --ms 3000  port 2 off for 3 s
"""),
    ("all", "all on|off", "switch every port", """
  on    switch on every port, 100 ms apart to limit the inrush current. Ports
        latched in fault stay off.
  off   switch off every port and cancel the cycles in progress.
"""),
    ("relay", "relay <n> connect|disconnect|cycle [--ms MS]", "auxiliary relay line", f"""
Control the auxiliary line on the two banana jacks (COM and NC). There is one
relay: n is always 1. At power-on the relay is at rest: line connected.

  connect      close the line (relay at rest).
  disconnect   open the line; the relay LED is on while the line is open.
  cycle        open the line for --ms milliseconds, then close it again
               ({p.RELAY_PULSE_MIN_MS}-{p.RELAY_PULSE_MAX_MS}, default 500). It runs in the devhub: the
               command returns at once.

  devhub relay 1 cycle --ms 2000   cut the line for 2 s
"""),
    ("defaults", "defaults show|set [ports...]|save", "ports enabled at power-on", """
The ports switched on when the devhub powers up (factory setting: all).

  show               print the ports enabled at power-on.
  set [ports...]     change them; no ports = all off at power-on.
  save               write them to flash. Without save the change is lost
                     at the next power-on.

  devhub defaults set 1 3 && devhub defaults save
"""),
    ("watch", "watch [--json]", "print events as they happen", """
Print events until Ctrl-C: port_changed, port_fault, port_warning,
relay_changed, power_changed. Each event says what caused it (origin):
command, button, timer (end of a cycle), fault or startup.

  --json   one JSON object per line, for scripts.
"""),
    ("hub", "hub reset", "reset the USB hub", """
Reset the USB hub chip. Every device on the ports, and the devhub control
interface itself, disconnects from the computer and enumerates again.
Use it when the hub stops talking to the computer.
"""),
    ("reboot", "reboot", "restart the controller firmware", """
Restart the firmware of the devhub controller. The USB hub is not reset; the
ports switch off and come back according to the power-on defaults.
"""),
    ("bootloader", "bootloader", "enter firmware update mode (DFU)", """
Restart the controller in the ST bootloader, ready for a firmware update over
USB through the hub. Then, with dfu-util installed:

  dfu-util -a 0 -s 0x08000000:leave -D devhub.bin

The devhub restarts with the new firmware at the end of the update.
"""),
    ("help", "help [command]", "detailed guide, or details of one command", """
Without arguments print the whole guide; with a command name print only the
details of that command.
"""),
]

_HEADER = "devhub - control a devhub USB hub: four switchable USB ports and an auxiliary relay line"
_USAGE = "devhub [-s SERIAL] <command> [arguments]"


def _short_guide() -> str:
    lines = [_HEADER, "", f"Usage: {_USAGE}", "", "Commands:"]
    for _, usage, summary, _ in _COMMANDS:
        if len(usage) <= 32:
            lines.append(f"  {usage:<32}  {summary}")
        else:
            lines += [f"  {usage}", f"  {'':<32}  {summary}"]
    lines += [
        "",
        "Examples:",
        "  devhub status                     state of every port",
        "  devhub port 2 cycle               power cycle port 2 (1 s off)",
        "  devhub relay 1 cycle --ms 2000    cut the relay line for 2 s",
        "",
        "Run 'devhub help' for the full guide, 'devhub help <command>' for one command.",
    ]
    return "\n".join(lines)


def _command_help(name: str) -> str:
    for cmd, usage, summary, details in _COMMANDS:
        if cmd == name:
            return f"devhub {usage}\n    {summary}\n{details}"
    raise KeyError(name)


def _full_guide() -> str:
    parts = [
        _HEADER,
        "",
        f"Usage: {_USAGE}",
        "",
        "Options:",
        "  -s, --serial SERIAL   choose the devhub when more than one is connected",
        "                        (serial numbers from 'devhub list').",
        "  -h, --help            short guide.",
        "",
        "Ports and relays are numbered from 1. Ports, cycles and relay pulses are",
        "handled by the devhub itself: they keep working when the computer is",
        "disconnected, and the front panel buttons always work.",
        "",
        "Exit codes: 0 success, 1 error (device not found, command refused, ...),",
        f"{EXIT_FAULT} from 'status' when a port is latched in fault.",
        "",
        "On macOS only one program at a time can open the devhub: while 'devhub watch'",
        "runs, other devhub commands cannot reach it.",
        "",
        "COMMANDS",
    ]
    for cmd, *_ in _COMMANDS:
        parts += ["", _command_help(cmd).rstrip("\n")]
    return "\n".join(parts)


class _ShortHelpAction(argparse.Action):
    def __init__(self, option_strings, dest, **kwargs):
        super().__init__(option_strings, dest, nargs=0, default=argparse.SUPPRESS, help="short guide")

    def __call__(self, parser, namespace, values, option_string=None):
        print(_short_guide())
        parser.exit()


def _parser() -> argparse.ArgumentParser:
    ap = argparse.ArgumentParser(prog="devhub", usage=_USAGE, add_help=False)
    ap.add_argument("-h", "--help", action=_ShortHelpAction)
    ap.add_argument("-s", "--serial", help="serial number, when several devhubs are connected")
    sub = ap.add_subparsers(dest="command", required=True)
    summaries = {cmd: summary for cmd, _, summary, _ in _COMMANDS}

    def add(name: str) -> argparse.ArgumentParser:
        return sub.add_parser(name, help=summaries[name], description=summaries[name])

    add("list")
    add("info")

    st = add("status")
    st.add_argument("--json", action="store_true", help="print the status as JSON")

    port = add("port")
    port.add_argument("n", type=int, help="port number, 1-4")
    port.add_argument("action", choices=["on", "off", "cycle", "clear"])
    port.add_argument("--ms", type=int, default=p.CYCLE_DEFAULT_MS, help="off time of a cycle (default %(default)s)")

    all_ = add("all")
    all_.add_argument("action", choices=["on", "off"])

    relay = add("relay")
    relay.add_argument("n", type=int, help="relay number, always 1")
    relay.add_argument("action", choices=["connect", "disconnect", "cycle"])
    relay.add_argument("--ms", type=int, default=500, help="disconnect time of a cycle (default %(default)s)")

    df = add("defaults")
    df.add_argument("action", choices=["show", "set", "save"])
    df.add_argument("ports", nargs="*", type=int, help="for 'set': ports enabled at power-on (none = all off)")

    hub = add("hub")
    hub.add_argument("action", choices=["reset"])

    watch = add("watch")
    watch.add_argument("--json", action="store_true", help="one JSON object per line")

    add("reboot")
    add("bootloader")

    hp = add("help")
    hp.add_argument("topic", nargs="?", metavar="command", help="command to describe")
    return ap


def _print_status(dev: Devhub, as_json: bool) -> int:
    status = dev.status()
    if as_json:
        print(json.dumps(status.to_dict(), indent=2))
    else:
        system = status.to_dict()["system"]
        print("system:", ", ".join(system) if system else "-")
        for port in status.ports:
            d = port.to_dict()
            state = "FAULT" if d["fault"] else "cycling" if d["cycle_active"] else "on" if d["enabled"] else "off"
            extra = []
            if d["fault_causes"]:
                extra.append("cause: " + ", ".join(d["fault_causes"]))
            if d["warnings"]:
                extra.append("warnings: " + ", ".join(d["warnings"]))
            print(f"port {port.port}: {state}" + (f"  ({'; '.join(extra)})" if extra else ""))
        for i, relay in enumerate(status.relays):
            line = "disconnected" if relay & RelayState.DISCONNECTED else "connected"
            pulse = " (pulse in progress)" if relay & RelayState.PULSE_ACTIVE else ""
            print(f"relay {i + 1}: {line}{pulse}")
    return EXIT_FAULT if any(port.faulted for port in status.ports) else EXIT_OK


def _run(args: argparse.Namespace) -> int:
    if args.command == "help":
        if args.topic is None:
            print(_full_guide())
            return EXIT_OK
        try:
            print(_command_help(args.topic).rstrip("\n"))
        except KeyError:
            print(f"devhub: unknown command '{args.topic}'; run 'devhub help' for the list", file=sys.stderr)
            return EXIT_ERROR
        return EXIT_OK

    if args.command == "list":
        devices = list_devices()
        serials = sorted({d.get("serial_number") or "?" for d in devices})
        for serial in serials:
            print(serial)
        return EXIT_OK if serials else EXIT_ERROR

    with Devhub(args.serial) as dev:
        if args.command == "info":
            info = dev.info().to_dict()
            for key, value in info.items():
                print(f"{key}: {', '.join(value) if isinstance(value, list) else value}")
        elif args.command == "status":
            return _print_status(dev, args.json)
        elif args.command == "port":
            if args.action == "on":
                dev.set_port(args.n, True)
            elif args.action == "off":
                dev.set_port(args.n, False)
            elif args.action == "cycle":
                dev.cycle_port(args.n, args.ms)
            else:
                dev.clear_fault(args.n)
        elif args.command == "all":
            dev.set_all(args.action == "on")
        elif args.command == "relay":
            if args.action == "cycle":
                dev.pulse_relay(args.n, args.ms)
            else:
                dev.set_relay(args.n, disconnected=args.action == "disconnect")
        elif args.command == "defaults":
            if args.action == "set":
                dev.set_defaults(args.ports)
            elif args.action == "save":
                dev.save_defaults()
            print("ports enabled at power-on:", " ".join(map(str, dev.get_defaults())) or "none")
        elif args.command == "hub":
            dev.reset_hub()
        elif args.command == "watch":
            try:
                for event in dev.events():
                    d = event.to_dict()
                    if args.json:
                        print(json.dumps(d), flush=True)
                    else:
                        name = d.pop("event")
                        print(name, " ".join(f"{k}={','.join(v) if isinstance(v, list) else v}" for k, v in d.items()),
                              flush=True)
            except KeyboardInterrupt:
                pass
        elif args.command == "reboot":
            dev.reboot()
        elif args.command == "bootloader":
            dev.enter_bootloader()
    return EXIT_OK


def main(argv: Optional[List[str]] = None) -> int:
    if not (sys.argv[1:] if argv is None else argv):
        print(_short_guide())
        return EXIT_OK
    args = _parser().parse_args(argv)
    try:
        return _run(args)
    except DevhubError as exc:
        print(f"devhub: {exc}", file=sys.stderr)
        return EXIT_ERROR


if __name__ == "__main__":
    raise SystemExit(main())
