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


def _parser() -> argparse.ArgumentParser:
    ap = argparse.ArgumentParser(prog="devhub", description="Control a devhub USB hub.")
    ap.add_argument("-s", "--serial", help="serial number, when several devhubs are connected")
    sub = ap.add_subparsers(dest="command", required=True)

    sub.add_parser("list", help="list connected devhubs")
    sub.add_parser("info", help="firmware and protocol versions")

    st = sub.add_parser("status", help="state of ports and relay")
    st.add_argument("--json", action="store_true")

    port = sub.add_parser("port", help="control one port")
    port.add_argument("n", type=int)
    port.add_argument("action", choices=["on", "off", "cycle", "clear"])
    port.add_argument("--ms", type=int, default=p.CYCLE_DEFAULT_MS, help="off time of a cycle (default %(default)s)")

    all_ = sub.add_parser("all", help="control every port")
    all_.add_argument("action", choices=["on", "off"])

    relay = sub.add_parser("relay", help="control the auxiliary relay line")
    relay.add_argument("n", type=int)
    relay.add_argument("action", choices=["connect", "disconnect", "cycle"])
    relay.add_argument("--ms", type=int, default=500, help="disconnect time of a cycle (default %(default)s)")

    df = sub.add_parser("defaults", help="ports enabled at power-on")
    df.add_argument("action", choices=["show", "set", "save"])
    df.add_argument("ports", nargs="*", type=int, help="for 'set': ports enabled at power-on (none = all off)")

    hub = sub.add_parser("hub", help="USB hub commands")
    hub.add_argument("action", choices=["reset"])

    watch = sub.add_parser("watch", help="print events as they arrive")
    watch.add_argument("--json", action="store_true")

    sub.add_parser("reboot", help="restart the firmware")
    sub.add_parser("bootloader", help="enter the ROM bootloader (firmware update with dfu-util)")
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
    args = _parser().parse_args(argv)
    try:
        return _run(args)
    except DevhubError as exc:
        print(f"devhub: {exc}", file=sys.stderr)
        return EXIT_ERROR


if __name__ == "__main__":
    raise SystemExit(main())
