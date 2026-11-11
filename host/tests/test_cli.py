"""Help output of the command line interface; no device needed."""

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
