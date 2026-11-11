"""Devhub device: commands and events over USB HID."""

from __future__ import annotations

import struct
import time
from collections import deque
from typing import Callable, Deque, Iterator, List, Optional

from . import protocol as p
from .protocol import Command, Event, Info, Response, Status, StatusCode


class DevhubError(Exception):
    """Communication problem with the device."""


class CommandError(DevhubError):
    """The device refused a command."""

    def __init__(self, cmd: int, status: int):
        self.cmd = cmd
        self.status = status
        try:
            name = StatusCode(status).name
        except ValueError:
            name = f"0x{status:02X}"
        super().__init__(f"command 0x{cmd:02X} failed: {name}")


class HidTransport:
    """Thin wrapper over hidapi; replaceable in tests."""

    def __init__(self, path: bytes):
        import hid  # from the "hidapi" package

        self._dev = hid.device()
        self._dev.open_path(path)

    def write(self, report: bytes) -> None:
        if self._dev.write(report) < 0:
            raise DevhubError("HID write failed")

    def read(self, timeout_ms: int) -> Optional[bytes]:
        data = self._dev.read(p.REPORT_SIZE, timeout_ms)
        return bytes(data) if data else None

    def close(self) -> None:
        self._dev.close()


def list_devices() -> List[dict]:
    """Connected devhubs as hidapi device dictionaries (see 'serial_number')."""
    import hid

    return [d for d in hid.enumerate(p.USB_VID, p.USB_PID)]


def _mask(ports: List[int]) -> int:
    mask = 0
    for port in ports:
        if not 1 <= port <= 8:
            raise ValueError(f"invalid port {port}")
        mask |= 1 << (port - 1)
    return mask


def _ports(mask: int) -> List[int]:
    return [i + 1 for i in range(8) if mask & (1 << i)]


class Devhub:
    """One devhub. Ports and relays are numbered from 1.

    Events received while waiting for a response are queued and returned by
    `events()` / `wait_event()`; `on_event` is called for each one as it arrives.
    """

    def __init__(self, serial: Optional[str] = None, *, transport=None, timeout: float = 1.0):
        self.on_event: Optional[Callable[[Event], None]] = None
        self._timeout = timeout
        self._seq = 0
        self._events: Deque[Event] = deque()
        self._info: Optional[Info] = None
        if transport is None:
            transport = self._open(serial)
        self._t = transport

    @staticmethod
    def _open(serial: Optional[str]) -> HidTransport:
        try:
            devices = list_devices()
        except ImportError as exc:
            raise DevhubError("the 'hidapi' Python package is not installed") from exc
        if serial is not None:
            devices = [d for d in devices if (d.get("serial_number") or "").upper() == serial.upper()]
        if not devices:
            raise DevhubError("no devhub found" + (f" with serial {serial}" if serial else ""))
        if len({d.get("serial_number") for d in devices}) > 1:
            raise DevhubError("several devhubs connected: select one by serial number")
        try:
            return HidTransport(devices[0]["path"])
        except OSError as exc:
            raise DevhubError(f"cannot open the device ({exc}); is another program using it?") from exc

    def close(self) -> None:
        self._t.close()

    def __enter__(self) -> "Devhub":
        return self

    def __exit__(self, *exc) -> None:
        self.close()

    # --- Transport ------------------------------------------------------------

    def _dispatch(self, event: Event) -> None:
        self._events.append(event)
        if self.on_event:
            self.on_event(event)

    def _send(self, cmd: Command, params: bytes = b"") -> int:
        self._seq = (self._seq % 255) + 1
        self._t.write(p.encode_command(cmd, self._seq, params))
        return self._seq

    def _command(self, cmd: Command, params: bytes = b"") -> bytes:
        seq = self._send(cmd, params)
        deadline = time.monotonic() + self._timeout
        while True:
            remaining = deadline - time.monotonic()
            if remaining <= 0:
                raise DevhubError(f"no response to command {cmd.name}")
            report = self._t.read(max(1, int(remaining * 1000)))
            if not report:
                continue
            decoded = p.decode_report(report)
            if isinstance(decoded, Event):
                self._dispatch(decoded)
            elif isinstance(decoded, Response) and decoded.cmd == cmd and decoded.seq == seq:
                if decoded.status != StatusCode.OK:
                    raise CommandError(cmd, decoded.status)
                return decoded.data

    # --- Queries --------------------------------------------------------------

    def info(self) -> Info:
        info = p.parse_info(self._command(Command.GET_INFO))
        if info.proto_version[0] > p.PROTO_VERSION_MAJOR:
            raise DevhubError("protocol version %d.%d is newer than this library" % info.proto_version)
        self._info = info
        return info

    def status(self) -> Status:
        info = self._info or self.info()
        return p.parse_status(self._command(Command.GET_STATUS), info.n_ports, info.n_relays)

    # --- Ports ----------------------------------------------------------------

    def set_port(self, port: int, on: bool) -> None:
        """Enable or disable VBUS and data of a port. `on` also retries a faulted port."""
        self._command(Command.SET_PORT, bytes([port, 1 if on else 0]))

    def cycle_port(self, port: int, off_ms: int = p.CYCLE_DEFAULT_MS) -> None:
        """Start a timed off/on cycle; it runs in the device and returns at once."""
        self._command(Command.CYCLE_PORT, struct.pack("<BH", port, off_ms))

    def set_all(self, on: bool) -> None:
        self._command(Command.SET_ALL, bytes([1 if on else 0]))

    def clear_fault(self, port: int) -> None:
        """Remove the fault latch; the port stays disabled."""
        self._command(Command.CLEAR_FAULT, bytes([port]))

    # --- Relay ----------------------------------------------------------------

    def set_relay(self, relay: int = 1, *, disconnected: bool) -> None:
        self._command(Command.SET_RELAY, bytes([relay, 1 if disconnected else 0]))

    def pulse_relay(self, relay: int = 1, ms: int = 500) -> None:
        """Disconnect the line for `ms`, then reconnect it."""
        self._command(Command.PULSE_RELAY, struct.pack("<BH", relay, ms))

    def reset_charge(self, port: int) -> None:
        """Restart the charge count of a port from zero."""
        self._command(Command.RESET_CHARGE, bytes([port]))

    # --- Settings kept in flash -------------------------------------------------

    def get_defaults(self) -> List[int]:
        """Ports enabled at power-on."""
        return _ports(self._command(Command.GET_DEFAULTS)[0])

    def set_defaults(self, ports: List[int]) -> None:
        self._command(Command.SET_DEFAULTS, bytes([_mask(ports)]))

    def get_charge_reset(self) -> List[int]:
        """Ports whose charge count restarts from zero at every turn-on."""
        return _ports(self._command(Command.GET_CHARGE_RESET)[0])

    def set_charge_reset(self, ports: List[int]) -> None:
        self._command(Command.SET_CHARGE_RESET, bytes([_mask(ports)]))

    def save_defaults(self) -> None:
        """Write the power-on ports and the charge reset setting to flash."""
        self._command(Command.SAVE_DEFAULTS)

    # --- System ---------------------------------------------------------------

    def reset_hub(self) -> None:
        """Reset the USB hub: every downstream device, this one included, re-enumerates."""
        self._command(Command.RESET_HUB)

    def reboot(self) -> None:
        self._send(Command.REBOOT)

    def enter_bootloader(self) -> None:
        self._send(Command.ENTER_BOOTLOADER, struct.pack("<I", p.BOOTLOADER_MAGIC))

    # --- Events ---------------------------------------------------------------

    def wait_event(self, timeout: Optional[float] = None) -> Optional[Event]:
        """Next event, or None after `timeout` seconds (None = wait forever)."""
        deadline = None if timeout is None else time.monotonic() + timeout
        while not self._events:
            if deadline is None:
                wait_ms = 200
            else:
                remaining = deadline - time.monotonic()
                if remaining <= 0:
                    return None
                wait_ms = max(1, int(remaining * 1000))
            report = self._t.read(wait_ms)
            if report:
                decoded = p.decode_report(report)
                if isinstance(decoded, Event):
                    self._dispatch(decoded)
        return self._events.popleft()

    def events(self, timeout: Optional[float] = None) -> Iterator[Event]:
        """Iterate over events until `timeout` seconds pass without one."""
        while True:
            event = self.wait_event(timeout)
            if event is None:
                return
            yield event
