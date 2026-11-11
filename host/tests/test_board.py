"""Tests on a real devhub connected over USB. Run with: pytest -m board

They switch ports and the relay: disconnect anything that must not be power-cycled.
"""

import time
from argparse import Namespace

import pytest

from devhub import CommandError, Devhub, DevhubError, EventType, attached_devices, cli

pytestmark = pytest.mark.board

PROJECT_START = 1767225600  # 2026-01-01 UTC: no firmware can be older


@pytest.fixture(scope="module")
def dev():
    try:
        device = Devhub()
    except DevhubError as exc:
        pytest.skip(str(exc))
    yield device
    device.set_all(True)
    device.set_relay(1, disconnected=False)
    device.close()


def drain(dev):
    return list(dev.events(timeout=0.2))


def test_info(dev):
    info = dev.info()
    assert info.n_ports == 4 and info.n_relays == 1
    assert len(info.serial) == 24


def test_port_on_off(dev):
    for port in range(1, 5):
        dev.set_port(port, False)
        assert not dev.status().ports[port - 1].enabled
        dev.set_port(port, True)
        time.sleep(0.2)  # staggered power-on
        assert dev.status().ports[port - 1].enabled


def test_cycle_runs_in_the_device(dev):
    dev.set_port(1, True)
    time.sleep(0.2)
    drain(dev)
    start = time.monotonic()
    dev.cycle_port(1, 1000)
    assert dev.status().ports[0].cycle_active
    while not dev.status().ports[0].enabled:
        assert time.monotonic() - start < 3
        time.sleep(0.05)
    assert 0.9 < time.monotonic() - start < 1.5
    changes = [e for e in drain(dev) if e.type == EventType.PORT_CHANGED and e.port == 1]
    assert len(changes) >= 2


def test_all_on_is_staggered(dev):
    dev.set_all(False)
    drain(dev)
    dev.set_all(True)
    time.sleep(0.6)
    assert all(port.enabled for port in dev.status().ports)


def test_relay_pulse(dev):
    dev.set_relay(1, disconnected=False)
    dev.pulse_relay(1, 300)
    assert dev.status().relays[0] != 0
    time.sleep(0.5)
    assert dev.status().relays[0] == 0


def test_defaults_roundtrip(dev):
    original = dev.get_defaults()
    dev.set_defaults([1, 3])
    assert dev.get_defaults() == [1, 3]
    dev.set_defaults(original)  # not saved to flash


def test_temperature_and_build_time(dev):
    temp = dev.status().mcu_temp_c
    assert temp is not None and 10 <= temp <= 70
    assert PROJECT_START <= dev.info().build_time <= time.time() + 60


def test_measurements_available(dev):
    for port in dev.status().ports:
        assert port.current_ma is not None and port.charge_uah is not None, f"port {port.port}"


def test_charge_never_decreases_and_resets(dev):
    dev.set_port(1, True)
    first = dev.status().ports[0].charge_uah
    time.sleep(1.2)  # the controller updates the charge once a second
    assert dev.status().ports[0].charge_uah >= first
    dev.reset_charge(1)
    assert dev.status().ports[0].charge_uah < 1000  # µAh: at most one update since the reset


def test_charge_reset_at_turn_on(dev):
    ports = dev.status().ports
    port = max(ports, key=lambda p: p.charge_uah).port  # the one that shows the reset best
    if ports[port - 1].charge_uah < 50:
        pytest.skip("no port has accumulated charge")
    original = dev.get_charge_reset()
    try:
        dev.set_charge_reset([port])
        dev.set_port(port, False)
        dev.set_port(port, True)
        assert dev.status().ports[port - 1].charge_uah < 1000
    finally:
        dev.set_charge_reset(original)  # not saved to flash


def test_labels(dev):
    original = dev.get_labels()
    try:
        dev.set_label(3, "test-label")
        assert dev.get_label(3) == "test-label"
        assert cli._resolve_port(dev, "TEST-LABEL") == 3
        with pytest.raises(CommandError):
            dev.set_label(3, "a\x01")  # not printable: refused by the firmware
    finally:
        dev.set_label(3, original[2])  # not saved to flash


def test_cycle_wait_sees_the_new_enumeration(dev):
    serial = dev.info().serial
    devices = attached_devices(serial)
    if not devices:
        pytest.skip("no device attached to a port, or USB device list not available")
    port, before = min(devices.items())
    args = Namespace(n=str(port), action="cycle", ms=1000, wait=True, timeout=10)
    assert cli._port_command(dev, args) == cli.EXIT_OK
    after = attached_devices(serial)[port]
    assert (after.vid, after.pid) == (before.vid, before.pid) and after.session != before.session


def test_port_limits(dev):
    # On the port drawing least: a lower limit must not trip a device attached elsewhere.
    port = min(dev.status().ports, key=lambda p: p.current_ma or 0).port
    original = dev.get_limits()
    try:
        dev.set_port_limit(port, 1000, 250)
        limits = dev.get_limits()
        assert limits.ports_ma[port - 1] == 960 and limits.tolerances_ms[port - 1] == 250  # step not above 1000 mA
        with pytest.raises(CommandError):
            dev.set_port_limit(port, 1000, 5)  # tolerance below the minimum
        dev.set_port_limit(port, 9999)
        assert dev.get_limits().ports_ma[port - 1] == 2670  # the maximum set by the board
        dev.set_supply_limit(5000)
        assert dev.get_limits().supply_ma == 5000
        with pytest.raises(CommandError):
            dev.set_port_limit(port, 100)  # below the lowest step
        assert all(p.current_ma is not None for p in dev.status().ports)  # controllers still fine
    finally:
        dev.set_port_limit(port, original.ports_ma[port - 1], original.tolerances_ms[port - 1])
        dev.set_supply_limit(original.supply_ma)  # not saved to flash


def test_supply_budget(dev):
    """A supply declared below the load: the port drawing most is reduced (warning), then switched off."""
    ports = dev.status().ports
    top = max(ports, key=lambda port: port.current_ma or 0)
    if (top.current_ma or 0) < 700:
        pytest.skip("needs a device drawing at least 0.7 A on a port")
    original = dev.get_limits()
    try:
        drain(dev)
        dev.set_supply_limit(1000)  # every port load plus 0.48 A inside is above it
        time.sleep(0.5)
        port = dev.status().ports[top.port - 1]
        assert port.faulted or "budget" in port.to_dict()["warnings"]
        time.sleep(0.5)
        port = dev.status().ports[top.port - 1]
        assert port.faulted and port.to_dict()["fault_causes"] == ["budget"]
    finally:
        dev.set_supply_limit(original.supply_ma)  # not saved to flash
        dev.set_port(top.port, True)
        time.sleep(0.3)
        assert dev.get_limits().in_force_ma[top.port - 1] == original.ports_ma[top.port - 1]
