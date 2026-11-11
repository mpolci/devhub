"""Control protocol 0.1: constants, report encoding and decoding.

Pure functions, no I/O. See docs/protocollo-controllo.md.
"""

from __future__ import annotations

import datetime
import enum
import struct
from dataclasses import dataclass
from typing import List, Optional, Tuple, Union

USB_VID = 0x1209
USB_PID = 0x0001

REPORT_SIZE = 64  # on the wire, report ID included

PROTO_VERSION_MAJOR = 0  # highest major version this library understands

REPORT_ID_COMMAND = 0x01
REPORT_ID_RESPONSE = 0x02
REPORT_ID_EVENT = 0x03

BOOTLOADER_MAGIC = 0x21554644  # "DFU!"

CYCLE_DEFAULT_MS = 1000
CYCLE_MIN_MS = 500
CYCLE_MAX_MS = 60000
RELAY_PULSE_MIN_MS = 10
RELAY_PULSE_MAX_MS = 60000


class Command(enum.IntEnum):
    GET_INFO = 0x01
    GET_STATUS = 0x02
    SET_PORT = 0x10
    CYCLE_PORT = 0x11
    SET_ALL = 0x12
    RESET_CHARGE = 0x13
    SET_RELAY = 0x20
    PULSE_RELAY = 0x21
    GET_DEFAULTS = 0x30
    SET_DEFAULTS = 0x31
    SAVE_DEFAULTS = 0x32
    GET_CHARGE_RESET = 0x33
    SET_CHARGE_RESET = 0x34
    GET_LABEL = 0x35
    SET_LABEL = 0x36
    GET_LIMITS = 0x37
    SET_PORT_LIMIT = 0x38
    SET_SUPPLY_LIMIT = 0x39
    CLEAR_FAULT = 0x40
    RESET_HUB = 0x50
    REBOOT = 0x7E
    ENTER_BOOTLOADER = 0x7F


class StatusCode(enum.IntEnum):
    OK = 0x00
    UNKNOWN_CMD = 0x01
    BAD_PARAM = 0x02
    PORT_FAULT = 0x03
    HW_ERROR = 0x04
    STORAGE_ERROR = 0x05


class EventType(enum.IntEnum):
    PORT_FAULT = 0x80
    PORT_CHANGED = 0x81
    POWER_CHANGED = 0x82
    RELAY_CHANGED = 0x83
    PORT_WARNING = 0x84


class Origin(enum.IntEnum):
    COMMAND = 0
    BUTTON = 1
    TIMER = 2
    FAULT = 3
    STARTUP = 4


class PortState(enum.IntFlag):
    ENABLED = 0x01
    CYCLE_ACTIVE = 0x02
    FAULT = 0x04


class PortFault(enum.IntFlag):
    OVERCURRENT = 0x01
    OVERTEMPERATURE = 0x02
    GENERIC = 0x80


class PortWarning(enum.IntFlag):
    DISCHARGE_FAILED = 0x01
    BACK_VOLTAGE = 0x02
    VBUS_LOW = 0x04


class RelayState(enum.IntFlag):
    DISCONNECTED = 0x01
    PULSE_ACTIVE = 0x02


class SystemFlags(enum.IntFlag):
    INPUT_FAULT = 0x01
    HUB_CONFIGURED = 0x02
    CONTROLLER_12_ERROR = 0x04
    CONTROLLER_34_ERROR = 0x08


class Capabilities(enum.IntFlag):
    CDC_CONSOLE = 0x01


TEMP_UNAVAILABLE = -128
CURRENT_UNAVAILABLE = 0xFFFF
CHARGE_UNAVAILABLE = 0xFFFFFFFF
PORT_LIMIT_STEPS_MA = (530, 960, 1070, 1280, 1600, 2130, 2670)  # port controller current limits
SUPPLY_LIMIT_MIN_MA = 1000
SUPPLY_LIMIT_MAX_MA = 6000
INTERNAL_MAX_MA = 480  # hub, MCU, LEDs and an energized relay, worst case (architettura.md)
LABEL_MAX_BYTES = 15  # UTF-8 bytes of a port label
PORT_STATUS_SIZE = 9  # state, fault, warning, current (u16 mA), charge (u32 µAh)


def _flag_names(value: enum.IntFlag) -> List[str]:
    """Names of the set bits, lower case; unknown bits as hex."""
    names = []
    remaining = int(value)
    for member in type(value):
        if remaining & member.value:
            names.append(member.name.lower())
            remaining &= ~member.value
    if remaining:
        names.append(f"0x{remaining:02x}")
    return names


@dataclass(frozen=True)
class Info:
    proto_version: Tuple[int, int]
    fw_version: Tuple[int, int, int]
    n_ports: int
    n_relays: int
    capabilities: Capabilities
    serial: str
    build_time: int  # Unix time (UTC) the firmware was built

    def to_dict(self) -> dict:
        return {
            "proto_version": "%d.%d" % self.proto_version,
            "fw_version": "%d.%d.%d" % self.fw_version,
            "n_ports": self.n_ports,
            "n_relays": self.n_relays,
            "capabilities": _flag_names(self.capabilities),
            "serial": self.serial,
            "build_time": datetime.datetime.fromtimestamp(self.build_time, datetime.timezone.utc)
            .strftime("%Y-%m-%d %H:%M UTC"),
        }


@dataclass(frozen=True)
class PortStatus:
    port: int  # numbered from 1
    state: PortState
    fault: PortFault
    warning: PortWarning
    current_ma: Optional[int]  # None if the port controller cannot be read
    charge_uah: Optional[int]  # charge delivered since the last reset, µAh

    @property
    def enabled(self) -> bool:
        return bool(self.state & PortState.ENABLED)

    @property
    def cycle_active(self) -> bool:
        return bool(self.state & PortState.CYCLE_ACTIVE)

    @property
    def faulted(self) -> bool:
        return bool(self.state & PortState.FAULT)

    def to_dict(self) -> dict:
        return {
            "port": self.port,
            "enabled": self.enabled,
            "cycle_active": self.cycle_active,
            "fault": self.faulted,
            "fault_causes": _flag_names(self.fault),
            "warnings": _flag_names(self.warning),
            "current_ma": self.current_ma,
            "charge_mah": None if self.charge_uah is None else self.charge_uah / 1000,
        }


@dataclass(frozen=True)
class Status:
    system: SystemFlags
    ports: List[PortStatus]
    relays: List[RelayState]
    mcu_temp_c: Optional[int]  # MCU die temperature; None if the reading failed

    def to_dict(self) -> dict:
        return {
            "system": _flag_names(self.system),
            "mcu_temp_c": self.mcu_temp_c,
            "ports": [p.to_dict() for p in self.ports],
            "relays": [
                {
                    "relay": i + 1,
                    "line": "disconnected" if r & RelayState.DISCONNECTED else "connected",
                    "pulse_active": bool(r & RelayState.PULSE_ACTIVE),
                }
                for i, r in enumerate(self.relays)
            ],
        }


@dataclass(frozen=True)
class Limits:
    ports_ma: List[int]  # current limit of each port, in port order
    supply_ma: int  # current the power supply can deliver

    @property
    def worst_case_ma(self) -> int:
        """Input current if every port draws its limit, internal consumption included."""
        return sum(self.ports_ma) + INTERNAL_MAX_MA

    def to_dict(self) -> dict:
        return {"ports_ma": list(self.ports_ma), "supply_ma": self.supply_ma, "worst_case_ma": self.worst_case_ma}


@dataclass(frozen=True)
class Response:
    cmd: int
    seq: int
    status: int
    data: bytes  # from byte 4 of the report


@dataclass(frozen=True)
class Event:
    type: EventType
    port: Optional[int] = None  # PORT_* events
    relay: Optional[int] = None  # RELAY_CHANGED
    state: Optional[enum.IntFlag] = None  # PortState, RelayState or SystemFlags
    fault: Optional[PortFault] = None
    warning: Optional[PortWarning] = None
    origin: Optional[Origin] = None

    def to_dict(self) -> dict:
        d = {"event": self.type.name.lower()}
        if self.port is not None:
            d["port"] = self.port
        if self.relay is not None:
            d["relay"] = self.relay
        if self.state is not None:
            d["state"] = _flag_names(self.state)
        if self.fault is not None:
            d["fault_causes"] = _flag_names(self.fault)
        if self.warning is not None:
            d["warnings"] = _flag_names(self.warning)
        if self.origin is not None:
            d["origin"] = self.origin.name.lower()
        return d


class ProtocolError(Exception):
    """Malformed or unexpected report."""


def encode_command(cmd: int, seq: int, params: bytes = b"") -> bytes:
    """Build a 64-byte command report (report ID first)."""
    if len(params) > REPORT_SIZE - 3:
        raise ValueError("parameters too long")
    report = bytes([REPORT_ID_COMMAND, cmd & 0xFF, seq & 0xFF]) + params
    return report.ljust(REPORT_SIZE, b"\x00")


def _origin(value: int) -> Origin:
    try:
        return Origin(value)
    except ValueError as exc:
        raise ProtocolError(f"unknown event origin {value}") from exc


def decode_report(report: bytes) -> Union[Response, Event, None]:
    """Decode an input report. Returns None for unknown report IDs or events."""
    if len(report) < 4:
        raise ProtocolError("report too short")
    report = bytes(report)
    rid = report[0]
    if rid == REPORT_ID_RESPONSE:
        return Response(cmd=report[1], seq=report[2], status=report[3], data=report[4:])
    if rid != REPORT_ID_EVENT:
        return None
    evt, d = report[1], report[2:]
    if evt == EventType.PORT_FAULT:
        return Event(EventType.PORT_FAULT, port=d[0], fault=PortFault(d[1]))
    if evt == EventType.PORT_CHANGED:
        return Event(EventType.PORT_CHANGED, port=d[0], state=PortState(d[1]), origin=_origin(d[2]))
    if evt == EventType.POWER_CHANGED:
        return Event(EventType.POWER_CHANGED, state=SystemFlags(d[0]))
    if evt == EventType.RELAY_CHANGED:
        return Event(EventType.RELAY_CHANGED, relay=d[0], state=RelayState(d[1]), origin=_origin(d[2]))
    if evt == EventType.PORT_WARNING:
        return Event(EventType.PORT_WARNING, port=d[0], warning=PortWarning(d[1]))
    return None


def parse_info(data: bytes) -> Info:
    proto_major, proto_minor, fw_major, fw_minor, fw_patch, n_ports, n_relays, caps = struct.unpack_from(
        "<BBBBBBBI", data, 0
    )
    serial = bytes(data[11:23]).hex().upper()
    (build_time,) = struct.unpack_from("<I", data, 23)
    return Info(
        proto_version=(proto_major, proto_minor),
        fw_version=(fw_major, fw_minor, fw_patch),
        n_ports=n_ports,
        n_relays=n_relays,
        capabilities=Capabilities(caps),
        serial=serial,
        build_time=build_time,
    )


def parse_status(data: bytes, n_ports: int, n_relays: int) -> Status:
    system = SystemFlags(data[0])
    ports = []
    for i in range(n_ports):
        state, fault, warning, ma, uah = struct.unpack_from("<BBBHI", data, 1 + PORT_STATUS_SIZE * i)
        ports.append(PortStatus(
            i + 1, PortState(state), PortFault(fault), PortWarning(warning),
            current_ma=None if ma == CURRENT_UNAVAILABLE else ma,
            charge_uah=None if uah == CHARGE_UNAVAILABLE else uah,
        ))
    base = 1 + PORT_STATUS_SIZE * n_ports
    relays = [RelayState(data[base + i]) for i in range(n_relays)]
    temp = struct.unpack_from("<b", data, base + n_relays)[0]
    if temp == TEMP_UNAVAILABLE:
        temp = None
    return Status(system=system, ports=ports, relays=relays, mcu_temp_c=temp)
