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

    def get_labels(self):
        return ["", "", "Nucleo", ""]


def test_device_matching():
    esp, stick = _devices()[1], _devices()[4]
    assert esp.matches("ESPRESSIF") and esp.matches("usbmodem2121") and esp.matches("303a:1001")
    assert stick.matches("cruzer") and not stick.matches("esp")


def test_resolve_port(monkeypatch):
    monkeypatch.setattr(cli, "attached_devices", lambda serial: _devices())
    assert cli._resolve_port(_InfoDev(), "3") == 3
    assert cli._resolve_port(_InfoDev(), "@cruzer") == 4
    assert cli._resolve_port(_InfoDev(), "nucleo") == 3
    for spec, message in [("@e", "several ports match: 1, 4"), ("@nope", "no device matches"), ("x", "not a number nor a label")]:
        with pytest.raises(cli._UsageError, match=message):
            cli._resolve_port(_InfoDev(), spec)


def test_log_csv(monkeypatch, capsys):
    from types import SimpleNamespace
    ports = [SimpleNamespace(enabled=True, current_ma=120, charge_uah=44491),
             SimpleNamespace(enabled=False, current_ma=None, charge_uah=None)]

    class Dev:
        def info(self):
            return SimpleNamespace(n_ports=2)

        def status(self):
            return SimpleNamespace(mcu_temp_c=33, ports=ports)

    monkeypatch.setattr(cli.time, "sleep", lambda s: None)
    assert cli._log(Dev(), 100, duration=0) == cli.EXIT_OK
    header, row = capsys.readouterr().out.splitlines()
    assert header == "time_s,temp_c,p1_on,p1_ma,p1_mah,p2_on,p2_ma,p2_mah"
    assert row.split(",")[1:] == ["33", "1", "120", "44.491", "0", "", ""]
    with pytest.raises(cli._UsageError):
        cli._log(Dev(), 10, duration=0)


@pytest.mark.parametrize("text", ["", "a b", "@x", "12", "x" * 16, "è" * 8])
def test_invalid_labels(text):
    with pytest.raises(cli._UsageError):
        cli._check_label(text)


def test_valid_label():
    cli._check_label("nucleo-f4")
    cli._check_label("è" * 7)  # 14 bytes


@pytest.mark.parametrize("text, ma", [("1.07", 1070), ("0.53A", 530), ("530mA", 530), ("2A", 2000), ("max", 2670)])
def test_parse_current(text, ma):
    assert cli._parse_current(text) == ma


def test_parse_current_invalid():
    with pytest.raises(cli._UsageError):
        cli._parse_current("lots")


def test_limits_verdict(capsys):
    from devhub import protocol as p
    cli._print_limits(p.Limits(ports_ma=[530, 530, 530, 530], supply_ma=3000, tolerances_ms=[100] * 4, in_force_ma=[530] * 4))
    out = capsys.readouterr().out.splitlines()
    assert out[0] == "port 1: 0.53 A, up to 0.96 A for at most 100 ms"
    assert out[-1] == "worst case: 2.12 A from the ports + 0.48 A inside = 2.60 A, 0.40 A margin"
    cli._print_limits(p.Limits(ports_ma=[2670] * 4, supply_ma=3000, tolerances_ms=[100] * 4, in_force_ma=[2670, 2670, 960, 2670]))
    out = capsys.readouterr().out.splitlines()
    assert out[2] == "port 3: 2.67 A, lowered to 0.96 A by the supply budget"
    assert out[-1].endswith("= 11.16 A, 8.16 A above the supply")
