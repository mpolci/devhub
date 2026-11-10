"""Tests on a real devhub connected over USB. Run with: pytest -m board

They switch ports and the relay: disconnect anything that must not be power-cycled.
"""

import time

import pytest

from devhub import Devhub, DevhubError, EventType

pytestmark = pytest.mark.board


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
