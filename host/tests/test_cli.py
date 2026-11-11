"""Command line interface logic, without a device."""

import pytest

from devhub import cli


def test_no_arguments_prints_short_guide(capsys):
    assert cli.main([]) == cli.EXIT_OK
    out = capsys.readouterr().out
    assert "Usage: devhub" in out and "devhub help" in out
    for cmd, *_ in cli._COMMANDS:
        assert f"\n  {cmd}" in out


def test_help_prints_every_command(capsys):
    assert cli.main(["help"]) == cli.EXIT_OK
    out = capsys.readouterr().out
    for cmd, usage, *_ in cli._COMMANDS:
        assert f"devhub {usage}" in out


def test_help_of_one_command(capsys):
    assert cli.main(["help", "port"]) == cli.EXIT_OK
    out = capsys.readouterr().out
    assert out.startswith("devhub port ") and "devhub relay" not in out


def test_help_of_unknown_command(capsys):
    assert cli.main(["help", "nope"]) == cli.EXIT_ERROR
    assert "unknown command" in capsys.readouterr().err


class _FakeStatusDev:
    """Port 1 cycling for the first status reads, then on."""

    def __init__(self, cycling_reads=2):
        self.reads = 0
        self.cycling_reads = cycling_reads

    def status(self):
        from types import SimpleNamespace
        self.reads += 1
        cycling = self.reads <= self.cycling_reads
        port = SimpleNamespace(faulted=False, enabled=not cycling, cycle_active=cycling)
        return SimpleNamespace(ports=[port])


def test_wait_ignores_the_old_enumeration(monkeypatch, capsys):
    from devhub.usbtree import AttachedDevice
    old = AttachedDevice(port=1, vid=0x303A, pid=0x1001, product="Board", session=1)
    new = AttachedDevice(port=1, vid=0x303A, pid=0x1001, product="Board", ttys=["/dev/cu.x"], session=2)
    seq = iter([{1: old}, {1: old}, {1: new}, {1: new}])
    monkeypatch.setattr(cli, "attached_devices", lambda serial: next(seq))
    monkeypatch.setattr(cli.time, "sleep", lambda s: None)
    assert cli._wait_for_device(_FakeStatusDev(), "S", 1, old, timeout=5) == cli.EXIT_OK
    assert capsys.readouterr().out == "port 1: Board [303a:1001] /dev/cu.x\n"


def test_wait_times_out(monkeypatch, capsys):
    monkeypatch.setattr(cli, "attached_devices", lambda serial: {})
    monkeypatch.setattr(cli.time, "sleep", lambda s: None)
    assert cli._wait_for_device(_FakeStatusDev(0), "S", 1, None, timeout=0) == cli.EXIT_ERROR
    assert "no device enumerated on port 1" in capsys.readouterr().err


def _devices():
    from devhub.usbtree import AttachedDevice
    return {
        1: AttachedDevice(port=1, vid=0x303A, pid=0x1001, manufacturer="Espressif", product="USB JTAG/serial",
                          ttys=["/dev/cu.usbmodem212101"]),
        4: AttachedDevice(port=4, vid=0x0781, pid=0x556B, manufacturer="SanDisk", product="Cruzer Edge"),
    }


class _InfoDev:
    def info(self):
        from types import SimpleNamespace
        return SimpleNamespace(serial="S")


def test_device_matching():
    esp, stick = _devices()[1], _devices()[4]
    assert esp.matches("ESPRESSIF") and esp.matches("usbmodem2121") and esp.matches("303a:1001")
    assert stick.matches("cruzer") and not stick.matches("esp")


def test_resolve_port(monkeypatch):
    monkeypatch.setattr(cli, "attached_devices", lambda serial: _devices())
    assert cli._resolve_port(_InfoDev(), "3") == 3
    assert cli._resolve_port(_InfoDev(), "@cruzer") == 4
    for spec, message in [("@e", "several ports match: 1, 4"), ("@nope", "no device matches"), ("x", "invalid port")]:
        with pytest.raises(cli._UsageError, match=message):
            cli._resolve_port(_InfoDev(), spec)
