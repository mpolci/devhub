"""Protocol encoding/decoding and the Devhub class against a fake transport."""

import struct
from collections import deque

import pytest

from devhub import CommandError, Devhub, DevhubError, EventType, Origin, PortFault, PortState, PortWarning
from devhub import protocol as p


def response(cmd, seq, status=0, data=b""):
    return (bytes([p.REPORT_ID_RESPONSE, cmd, seq, status]) + data).ljust(p.REPORT_SIZE, b"\x00")


def event(evt, data):
    return (bytes([p.REPORT_ID_EVENT, evt]) + bytes(data)).ljust(p.REPORT_SIZE, b"\x00")


def status_data(ports, relay=1, temp=37, system=0x02):
    """GET_STATUS payload; ports: (state, fault, warning, mA, µAh) per port."""
    return bytes([system]) + b"".join(struct.pack("<BBBHI", *port) for port in ports) + bytes([relay, temp & 0xFF])


INFO_DATA = bytes([0, 1, 0, 1, 0, 4, 1]) + struct.pack("<I", 1) + bytes(range(0xA0, 0xAC))


class FakeTransport:
    """Answers commands like the firmware would; can inject events first."""

    def __init__(self):
        self.sent = []
        self.rx = deque()
        self.pending_events = []
        self.status_code = 0
        self.status_data = status_data([(1, 0, 0, 120, 85300), (2, 0, 1, 0, 0), (4, 1, 0, 0, 0),
                                        (0, 0, 2, 0xFFFF, 0xFFFFFFFF)])
        self.silent = False

    def write(self, report):
        assert len(report) == p.REPORT_SIZE and report[0] == p.REPORT_ID_COMMAND
        self.sent.append(bytes(report))
        cmd, seq = report[1], report[2]
        self.rx.extend(self.pending_events)
        self.pending_events = []
        if self.silent or cmd in (p.Command.REBOOT, p.Command.ENTER_BOOTLOADER):
            return
        data = {p.Command.GET_INFO: INFO_DATA, p.Command.GET_STATUS: self.status_data,
                p.Command.GET_DEFAULTS: bytes([0x05]), p.Command.GET_CHARGE_RESET: bytes([0x02]),
                p.Command.GET_LABEL: b"esp\0"}.get(cmd, b"")
        self.rx.append(response(cmd, seq, self.status_code, data))

    def read(self, timeout_ms):
        return self.rx.popleft() if self.rx else None

    def close(self):
        pass


@pytest.fixture
def fake():
    return FakeTransport()


@pytest.fixture
def dev(fake):
    return Devhub(transport=fake, timeout=0.05)


def test_encode_command():
    report = p.encode_command(p.Command.CYCLE_PORT, 7, struct.pack("<BH", 2, 1000))
    assert len(report) == 64
    assert report[:6] == bytes([0x01, 0x11, 7, 2, 0xE8, 0x03])
    assert report[6:] == bytes(58)


def test_info(dev):
    info = dev.info()
    assert info.proto_version == (0, 1)
    assert info.fw_version == (0, 1, 0)
    assert (info.n_ports, info.n_relays) == (4, 1)
    assert info.serial == "A0A1A2A3A4A5A6A7A8A9AAAB"
    assert info.to_dict()["capabilities"] == ["cdc_console"]


def test_newer_protocol_is_rejected(dev, fake):
    fake.write = lambda report: fake.rx.append(response(report[1], report[2], 0, bytes([1, 0]) + INFO_DATA[2:]))
    with pytest.raises(DevhubError):
        dev.info()


def test_status(dev):
    status = dev.status()
    assert [port.port for port in status.ports] == [1, 2, 3, 4]
    assert status.ports[0].enabled and not status.ports[0].faulted
    assert status.ports[1].cycle_active and status.ports[1].warning == PortWarning.DISCHARGE_FAILED
    assert status.ports[2].faulted and status.ports[2].fault == PortFault.OVERCURRENT
    assert not status.ports[3].enabled and status.ports[3].warning == PortWarning.BACK_VOLTAGE
    d = status.to_dict()
    assert d["system"] == ["hub_configured"]
    assert d["relays"] == [{"relay": 1, "line": "disconnected", "pulse_active": False}]
    assert d["ports"][2]["fault_causes"] == ["overcurrent"]
    assert status.mcu_temp_c == 37 and d["mcu_temp_c"] == 37
    assert (status.ports[0].current_ma, status.ports[0].charge_uah) == (120, 85300)
    assert d["ports"][0]["charge_mah"] == 85.3
    assert status.ports[3].current_ma is None and d["ports"][3]["charge_mah"] is None  # not readable


def test_status_temperature():
    ports = [(0, 0, 0, 0, 0)] * 4
    assert p.parse_status(status_data(ports, temp=-10), 4, 1).mcu_temp_c == -10
    assert p.parse_status(status_data(ports, temp=-128), 4, 1).mcu_temp_c is None  # reading failed


def test_command_payloads(dev, fake):
    dev.set_port(3, True)
    dev.cycle_port(1)
    dev.set_all(False)
    dev.clear_fault(4)
    dev.set_relay(1, disconnected=True)
    dev.pulse_relay(1, 250)
    dev.set_defaults([1, 3])
    dev.reset_charge(2)
    dev.set_charge_reset([2, 4])
    dev.set_label(1, "ab")
    dev.save_defaults()
    dev.reset_hub()
    dev.reboot()
    dev.enter_bootloader()
    sent = [(r[1], r[3:9].rstrip(b"\x00")) for r in fake.sent]
    assert sent == [
        (0x10, bytes([3, 1])),
        (0x11, bytes([1, 0xE8, 0x03])),
        (0x12, b""),
        (0x40, bytes([4])),
        (0x20, bytes([1, 1])),
        (0x21, bytes([1, 250])),
        (0x31, bytes([0x05])),
        (0x13, bytes([2])),
        (0x34, bytes([0x0A])),
        (0x36, bytes([1]) + b"ab"),
        (0x32, b""),
        (0x50, b""),
        (0x7E, b""),
        (0x7F, b"DFU!"),
    ]
    seqs = [r[2] for r in fake.sent]
    assert seqs == list(range(1, len(seqs) + 1))


def test_defaults(dev):
    assert dev.get_defaults() == [1, 3]
    with pytest.raises(ValueError):
        dev.set_defaults([0])
    assert dev.get_charge_reset() == [2]
    assert dev.get_label(2) == "esp" and dev.get_labels() == ["esp"] * 4
    with pytest.raises(ValueError):
        dev.set_label(1, "x" * 16)
    with pytest.raises(ValueError):
        dev.set_charge_reset([9])


def test_error_status(dev, fake):
    fake.status_code = p.StatusCode.PORT_FAULT
    with pytest.raises(CommandError) as exc:
        dev.cycle_port(1)
    assert exc.value.status == p.StatusCode.PORT_FAULT
    assert "PORT_FAULT" in str(exc.value)


def test_timeout(dev, fake):
    fake.silent = True
    with pytest.raises(DevhubError):
        dev.set_port(1, True)


def test_events_between_command_and_response(dev, fake):
    seen = []
    dev.on_event = seen.append
    fake.pending_events = [event(0x81, [2, 0x01, 0]), event(0x80, [3, 0x01])]
    dev.set_port(2, True)  # response arrives after the two events
    assert [e.type for e in seen] == [EventType.PORT_CHANGED, EventType.PORT_FAULT]
    first = dev.wait_event(0)
    assert (first.port, first.state, first.origin) == (2, PortState.ENABLED, Origin.COMMAND)
    assert dev.wait_event(0).fault == PortFault.OVERCURRENT
    assert dev.wait_event(0.01) is None


def test_stale_response_is_ignored(dev, fake):
    fake.rx.append(response(p.Command.SET_PORT, 99))  # leftover from an earlier session
    dev.set_port(1, True)
    assert not fake.rx


def test_event_decoding():
    e = p.decode_report(event(0x83, [1, 0x03, 2]))
    assert (e.type, e.relay, e.origin) == (EventType.RELAY_CHANGED, 1, Origin.TIMER)
    assert e.to_dict() == {"event": "relay_changed", "relay": 1, "state": ["disconnected", "pulse_active"],
                           "origin": "timer"}
    assert p.decode_report(event(0x84, [4, 0x03])).to_dict()["warnings"] == ["discharge_failed", "back_voltage"]
    assert p.decode_report(event(0x82, [0x09])).to_dict()["state"] == ["input_fault", "controller_34_error"]
    assert p.decode_report(event(0x99, [0])) is None  # unknown event: ignored
    assert p.decode_report(bytes([0x55]) + bytes(63)) is None


def test_events_iterator(dev, fake):
    fake.rx.extend([event(0x81, [1, 0, 1]), event(0x81, [1, 1, 1])])
    assert [e.state for e in dev.events(timeout=0.01)] == [PortState(0), PortState.ENABLED]
