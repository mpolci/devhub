"""Lookup of the devices attached to the ports, on fake macOS and Linux USB trees."""

from pathlib import Path

from devhub import usbtree

SERIAL = "34003C00045046594B393620"


def _mac_dev(loc, vid, pid, name, children=(), **extra):
    node = {"IOObjectClass": "IOUSBHostDevice", "locationID": loc, "idVendor": vid, "idProduct": pid,
            "USB Product Name": name, "IORegistryEntryChildren": list(children)}
    node.update(extra)
    return node


def _mac_tree():
    serial_port = {"IOObjectClass": "IOSerialBSDClient", "IOCalloutDevice": "/dev/cu.usbmodem212102"}
    interface = {"IOObjectClass": "IOUSBHostInterface", "IORegistryEntryChildren": [serial_port]}
    probe = _mac_dev(0x02121000, 0x0483, 0x374E, "STLINK-V3", [interface],
                     **{"USB Vendor Name": "STMicroelectronics", "USB Serial Number": "X1", "Device Speed": 2})
    behind_hub = _mac_dev(0x02123100, 0x1234, 0x0001, "Deep", [interface])
    small_hub = _mac_dev(0x02123000, 0x05E3, 0x0610, "USB2.0 Hub", [behind_hub], bDeviceClass=9)
    control = _mac_dev(0x02125000, 0x1209, 0x0001, "devhub control", [interface], **{"USB Serial Number": SERIAL})
    # In the IOService plane the hub driver and its port nodes sit between a hub and its devices.
    hub_ports = [{"IOObjectClass": "AppleUSB20HubPort", "IORegistryEntryChildren": [dev]}
                 for dev in (probe, small_hub, control)]
    hub_driver = {"IOObjectClass": "AppleUSB20Hub", "IORegistryEntryChildren": hub_ports}
    hub = _mac_dev(0x02120000, 0x1209, 0x0002, "devhub USB hub", [hub_driver], **{"USB Serial Number": SERIAL})
    jtag = _mac_dev(0x02124000, 0x303A, 0x1001, "USB JTAG_serial debug unit",
                    kUSBProductString="USB JTAG/serial debug unit")
    hub_ports.append({"IOObjectClass": "AppleUSB20HubPort", "IORegistryEntryChildren": [jtag]})
    other = _mac_dev(0x02110000, 0x3564, 0xFEFB, "Camera")
    return [_mac_dev(0x02100000, 0x1D5C, 0x5801, "USB2.0 Hub", [other, hub], bDeviceClass=9)]


def test_mac_ports_and_ttys():
    devices = usbtree._mac(SERIAL, _mac_tree())
    assert sorted(devices) == [1, 3, 4]  # control device on hub port 5 is left out
    probe = devices[1]
    assert (probe.vid, probe.pid, probe.speed) == (0x0483, 0x374E, "high")
    assert probe.ttys == ["/dev/cu.usbmodem212102"]
    assert probe.describe() == "STMicroelectronics STLINK-V3 [0483:374e] /dev/cu.usbmodem212102"
    hub = devices[3]
    assert hub.is_hub and hub.ttys == []  # devices behind a hub are not its own
    assert hub.describe() == "USB2.0 Hub (hub) [05e3:0610]"
    assert devices[4].product == "USB JTAG/serial debug unit"  # device string, not the sanitized name


def test_mac_other_serial():
    assert usbtree._mac("OTHER", _mac_tree()) == {}


def test_mac_location_ports():
    assert usbtree._mac_port(0x02120000, 0x02124000) == 4
    assert usbtree._mac_port(0x02120000, 0x02124100) is None  # two levels down
    assert usbtree._mac_port(0x02120000, 0x03124000) is None  # other bus


def _sysfs_dev(root: Path, name: str, **attrs):
    d = root / name
    d.mkdir()
    for key, value in attrs.items():
        (d / key).write_text(value + "\n")
    return d


def test_linux_ports_and_ttys(tmp_path):
    _sysfs_dev(tmp_path, "1-2", idVendor="1209", idProduct="0002", serial=SERIAL, bDeviceClass="09")
    _sysfs_dev(tmp_path, "1-2.5", idVendor="1209", idProduct="0001", serial=SERIAL)
    probe = _sysfs_dev(tmp_path, "1-2.2", idVendor="0483", idProduct="374e", manufacturer="STMicroelectronics",
                       product="STLINK-V3", speed="480", bDeviceClass="ef")
    (probe / "1-2.2:1.1" / "tty" / "ttyACM0").mkdir(parents=True)  # interfaces live in the device directory
    uart = _sysfs_dev(tmp_path, "1-2.4", idVendor="0403", idProduct="6001", product="FT232R USB UART", speed="12")
    (uart / "1-2.4:1.0" / "ttyUSB0").mkdir(parents=True)
    (tmp_path / "1-2.4:1.0").mkdir()  # interfaces are also listed next to the devices
    _sysfs_dev(tmp_path, "1-2.4.1", idVendor="1234", idProduct="0001")  # not a direct child

    devices = usbtree._linux(SERIAL, tmp_path)
    assert sorted(devices) == [2, 4]
    assert devices[2].ttys == ["/dev/ttyACM0"] and devices[2].speed == "high"
    assert devices[4].describe() == "FT232R USB UART [0403:6001] /dev/ttyUSB0"


def test_linux_other_serial(tmp_path):
    _sysfs_dev(tmp_path, "1-2", idVendor="1209", idProduct="0002", serial=SERIAL)
    assert usbtree._linux("OTHER", tmp_path) == {}
