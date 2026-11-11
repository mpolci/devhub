"""Devices attached to the ports of a devhub, read from the operating system's USB tree.

The devhub firmware cannot see what is plugged into its ports: the computer does. The
devhub's USB hub reports the same serial number as the control device, so the hub is
found by serial number, and the devices one level below it are on its ports. Hub
downstream ports 1-4 are the front panel ports 1-4; the control device sits on hub
port 5 (physical port 6, renumbered because ports 5 and 7 are disabled).

Supported on macOS (ioreg) and Linux (sysfs). Elsewhere `attached_devices` returns None.
"""

from __future__ import annotations

import os
import plistlib
import re
import subprocess
import sys
from dataclasses import dataclass, field
from pathlib import Path
from typing import Dict, List, Optional

from . import protocol as p

HUB_PID = 0x0002  # USB2517 hub as configured by the firmware

_SPEEDS_MAC = {0: "low", 1: "full", 2: "high", 3: "super", 4: "super+"}
_SPEEDS_LINUX = {"1.5": "low", "12": "full", "480": "high", "5000": "super", "10000": "super+", "20000": "super+"}


@dataclass(frozen=True)
class AttachedDevice:
    port: int  # front panel port, numbered from 1
    vid: int
    pid: int
    manufacturer: Optional[str] = None
    product: Optional[str] = None
    serial: Optional[str] = None
    speed: Optional[str] = None  # low, full, high, super, super+
    is_hub: bool = False
    ttys: List[str] = field(default_factory=list)  # serial ports, e.g. /dev/cu.usbmodem1103
    session: Optional[int] = None  # changes at every enumeration (macOS sessionID, Linux devnum)

    def describe(self) -> str:
        """One line: names, VID:PID and serial ports."""
        names = [s for s in (self.manufacturer, self.product) if s]
        if self.manufacturer and self.product and self.product.startswith(self.manufacturer):
            names = [self.product]
        text = " ".join(names) or "unknown device"
        if self.is_hub:
            text += " (hub)"
        text += f" [{self.vid:04x}:{self.pid:04x}]"
        if self.ttys:
            text += " " + " ".join(self.ttys)
        return text

    def to_dict(self) -> dict:
        return {
            "vid": f"{self.vid:04x}",
            "pid": f"{self.pid:04x}",
            "manufacturer": self.manufacturer,
            "product": self.product,
            "serial": self.serial,
            "speed": self.speed,
            "hub": self.is_hub,
            "ttys": list(self.ttys),
        }


def attached_devices(hub_serial: str) -> Optional[Dict[int, AttachedDevice]]:
    """Devices on the ports of the devhub with this serial number, by port.

    Ports with nothing attached are missing from the result. None when the platform is
    not supported or the USB tree cannot be read; {} when the hub is not found.
    """
    try:
        if sys.platform == "darwin":
            return _mac(hub_serial, _ioreg())
        if sys.platform.startswith("linux"):
            return _linux(hub_serial, Path("/sys/bus/usb/devices"))
    except Exception:  # the device list is extra information: never let it break a command
        pass
    return None


# --- macOS ---------------------------------------------------------------------


def _ioreg() -> list:
    out = subprocess.run(
        ["ioreg", "-a", "-l", "-r", "-c", "IOUSBHostDevice", "-w0"],
        check=True, capture_output=True, timeout=5,
    ).stdout
    return _parse_ioreg(out)


# Control characters are not allowed in XML, but ioreg copies them as they are from
# string descriptors (some debug probes have a binary serial number).
_XML_INVALID = re.compile(rb"[\x00-\x08\x0b\x0c\x0e-\x1f]")


def _parse_ioreg(out: bytes) -> list:
    out = _XML_INVALID.sub(b"?", out)
    return plistlib.loads(out) if out.strip() else []


def _is_usb_device(node: dict) -> bool:
    return node.get("IOObjectClass") == "IOUSBHostDevice"


def _mac(hub_serial: str, roots: list) -> Dict[int, AttachedDevice]:
    hub = _find_mac_hub(roots, hub_serial)
    if hub is None:
        return {}
    hub_loc = hub.get("locationID", 0)
    result = {}
    for child in _mac_child_devices(hub):
        port = _mac_port(hub_loc, child.get("locationID", 0))
        if port is None or not 1 <= port <= 4:
            continue  # port 5 is the devhub control device
        result[port] = AttachedDevice(
            port=port,
            vid=child.get("idVendor", 0),
            pid=child.get("idProduct", 0),
            # The k* strings are the device's own; "USB Product Name" has "/" replaced by "_".
            manufacturer=child.get("kUSBVendorString") or child.get("USB Vendor Name") or None,
            product=child.get("kUSBProductString") or child.get("USB Product Name") or None,
            serial=child.get("kUSBSerialNumberString") or child.get("USB Serial Number") or None,
            speed=_SPEEDS_MAC.get(child.get("Device Speed")),
            is_hub=child.get("bDeviceClass") == 9,
            ttys=sorted(_mac_ttys(child)),
            session=child.get("sessionID"),
        )
    return result


def _mac_child_devices(node: dict) -> List[dict]:
    """USB devices one level below a device; driver nodes in between are skipped."""
    found = []
    for child in node.get("IORegistryEntryChildren", []):
        found += [child] if _is_usb_device(child) else _mac_child_devices(child)
    return found


def _find_mac_hub(nodes: list, serial: str) -> Optional[dict]:
    for node in nodes:
        if (_is_usb_device(node) and node.get("idVendor") == p.USB_VID and node.get("idProduct") == HUB_PID
                and node.get("USB Serial Number") == serial):
            return node
        found = _find_mac_hub(node.get("IORegistryEntryChildren", []), serial)
        if found is not None:
            return found
    return None


def _mac_port(hub_loc: int, loc: int) -> Optional[int]:
    """Port number from a macOS locationID: one hex digit per hub level after the bus byte."""
    hub_digits = f"{hub_loc & 0xFFFFFF:06x}".rstrip("0")
    digits = f"{loc & 0xFFFFFF:06x}".rstrip("0")
    if (loc >> 24) != (hub_loc >> 24) or len(digits) != len(hub_digits) + 1 or not digits.startswith(hub_digits):
        return None
    return int(digits[-1], 16)


def _mac_ttys(node: dict) -> List[str]:
    """Serial ports below a device, without descending into devices behind a hub."""
    ttys = []
    if "IOCalloutDevice" in node:
        ttys.append(node["IOCalloutDevice"])
    for child in node.get("IORegistryEntryChildren", []):
        if not _is_usb_device(child):
            ttys += _mac_ttys(child)
    return ttys


# --- Linux -----------------------------------------------------------------------


def _read(path: Path) -> Optional[str]:
    try:
        return path.read_text().strip() or None
    except OSError:
        return None


def _linux(hub_serial: str, sysfs: Path) -> Dict[int, AttachedDevice]:
    hub_name = None
    for dev in sysfs.iterdir():
        if (_read(dev / "idVendor") == f"{p.USB_VID:04x}" and _read(dev / "idProduct") == f"{HUB_PID:04x}"
                and _read(dev / "serial") == hub_serial):
            hub_name = dev.name
            break
    if hub_name is None:
        return {}
    # Child devices are named "<hub>.<port>"; the hub itself may be "<bus>-<path>".
    result = {}
    for dev in sysfs.glob(f"{hub_name}.*"):
        port_text = dev.name[len(hub_name) + 1:]
        if not port_text.isdigit() or not 1 <= int(port_text) <= 4:
            continue  # interfaces (":"), deeper levels, control device on port 5
        port = int(port_text)
        result[port] = AttachedDevice(
            port=port,
            vid=int(_read(dev / "idVendor") or "0", 16),
            pid=int(_read(dev / "idProduct") or "0", 16),
            manufacturer=_read(dev / "manufacturer"),
            product=_read(dev / "product"),
            serial=_read(dev / "serial"),
            speed=_SPEEDS_LINUX.get(_read(dev / "speed") or ""),
            is_hub=_read(dev / "bDeviceClass") == "09",
            ttys=sorted(_linux_ttys(dev)),
            session=int(_read(dev / "devnum") or "0") or None,
        )
    return result


def _linux_ttys(dev: Path) -> List[str]:
    """Serial ports of the device's own interfaces (ttyACM*, ttyUSB*)."""
    ttys = set()
    for intf in dev.glob(f"{dev.name}:*"):
        for entry in list(intf.glob("tty/*")) + list(intf.glob("ttyUSB*")):
            ttys.add("/dev/" + entry.name)
    return [t for t in ttys if os.path.basename(t).startswith("tty")]
