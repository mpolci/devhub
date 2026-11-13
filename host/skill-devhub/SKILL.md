---
name: devhub
description: Control a devhub, a USB hub with four switchable ports and an auxiliary relay line, through its `devhub` CLI or Python library. Use it when a board or device attached to a devhub has to be switched off, on or power cycled, found on its port, waited for when it enumerates again, or when the state of the ports (current, faults) is needed.
---

# devhub

A devhub is a USB 2.0 hub whose four downstream ports can be switched off and on from the
computer: VBUS and data lines together, as unplugging and plugging the cable. It also has one
relay line on two banana jacks (COM and NC): closed at rest, it can be opened to interrupt
whatever the user wired through it. Ports, cycles and relay pulses run in the devhub itself; the
front panel buttons always work, so the user may change things behind your back: read the state
before acting on it.

It is typically used to restart a board after flashing it, to get back one that stopped
responding, or to remove power between tests; when to use it is yours to judge from what
follows.

## Finding the tools

1. `command -v devhub`: the CLI is on the PATH.
2. Otherwise the CLI is in the repository that holds this skill: resolve the real path of this
   skill's folder (`cd -P <skill dir> && pwd`, it is usually a symlink); its parent is `host/`,
   the CLI is `host/bin/devhub`. The first run creates `host/.venv` with the library and its
   dependencies.
3. The Python library runs with `host/.venv/bin/python` (run `host/bin/devhub list` once if the
   virtualenv does not exist yet). See [references/library.md](references/library.md).

`devhub help` prints the full guide, `devhub help <command>` one command: read it before using a
command or option not described here, the CLI is the reference and this skill does not repeat it.

## What you may do without asking

- Switch on, switch off and power cycle any port, all ports, and reset charge counts.
- Connect, disconnect and pulse the relay.
- Read anything: `list`, `info`, `status`, `find`, `label`, `limit`, `defaults show`.

Ask first, saying what it does, before: `label <port> <text>`, `limit <...>`, `defaults
set|charge-reset|save`, `hub reset` (every device on the hub and the devhub itself disconnect and
enumerate again), `reboot`, `bootloader`. They change settings kept by the device or interrupt
everything attached to it.

Even when allowed, say which port you switched and why. A port may power the debug probe or the
programmer you are using, or another board the user is working on: if `status` shows a device on
a port you are about to switch that is not the board in question, mention it.

## Choosing the port

Never guess a port number. In order of preference:

- the label the user gave the port (`devhub label` lists them), usable in place of the number;
- `@text`, the port whose attached device matches the text (manufacturer, product, serial
  number, VID:PID or serial port name); it must match exactly one port;
- `devhub find <text>` to see which ports match, or `devhub status --json` to see every port with
  its device (`ports[].device`: vid, pid, manufacturer, product, serial, ttys).

A board that is powered off or stuck before enumeration shows no device: ask the user which port
it is on, or look for a port that is on with no device (`-` in `status`).

`find`, `@text`, `--wait` and the device column need macOS or Linux; on Windows ask the user for
the port number.

## How ports behave

- **Off and on.** `port <port> off` removes VBUS and disconnects the data lines, `on` restores
  both: the computer sees the device unplugged and plugged in again.
- **Cycle.** `port <port> cycle [--ms N]` switches the port off for N ms (500-60000, default
  1000, VBUS discharge included), then on again. It runs in the devhub: the command returns at
  once while the cycle is still running, and `on` or `off` during a cycle cancel it.
- **`--wait`** on `on` and `cycle` returns only when a device has enumerated on the port (after a
  cycle, the new enumeration) and prints it with its serial ports, as in
  `port 1: Espressif USB JTAG/serial debug unit [303a:1001] /dev/cu.usbmodem212101`. Exit code 1
  if none appears within `--timeout` (default 10 s). The serial port name can change at every
  enumeration: take it from this line.
- **Programs holding the board's serial port** (monitors, debuggers) lose it when the port goes
  off: close them before and reopen them after.
- **Only the power supplied by the port is removed.** A board with its own supply keeps running
  through a port cycle (warning `back_voltage`); the relay line can interrupt that supply if the
  user wired it so.

## The relay line

`relay 1 disconnect` opens the line, `connect` closes it, `cycle [--ms N]` opens it for N ms
(10-60000, default 500) and runs in the devhub. What opening the line does depends on how the
user wired it: ask if you do not know.

## Faults and warnings

- A port in fault is switched off and stays off (latched) until `port <port> on`, which clears
  the fault and retries, `port <port> clear`, which leaves it off, or its button. `status` exits
  with code 2 while a port is latched.
- `fault_causes` says why: `overcurrent`, `current_limit` and `budget` mean the device drew more
  than the port limit or the power supply allow, `overtemperature` that the port controller
  overheated. Read the cause before switching the port on again. One retry is reasonable;
  retrying in a loop is not: tell the user. After `overtemperature` do not retry.
- Warnings (`discharge_failed`, `back_voltage`, `vbus_low`, `budget`) are reported only: the port
  keeps working.

## Reading the state

- `devhub status --json` when you parse the output; the text form is for showing to the user.
  Port states: `on`, `off`, `cycling`, `FAULT`.
- Current is ±27 mA below 200 mA: a few mA on an empty port is the zero error, not a load.
- When the devhub does not answer (`no devhub found`, timeouts), run `devhub list`. On macOS
  only one program at a time can open it: a running `devhub watch` or a script holding it
  blocks the others.

## Things to avoid

- `devhub watch` and `devhub log` without `--duration` run until Ctrl-C: do not start them in
  the foreground. Use `log --duration`, or `watch` in the background and stop it afterwards; on
  macOS nothing else can talk to the devhub while it runs.
- More than one devhub connected: every command needs `-s <serial>` (serials from `devhub list`).
- Do not save settings to flash (`defaults save`) as a side effect of something else.
