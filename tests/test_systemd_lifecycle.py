#!/usr/bin/env python3
# SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
# SPDX-License-Identifier: GPL-3.0-or-later

"""Manual integration test: python3 tests/test_systemd_lifecycle.py ../dde-session

Requires a running user systemd manager and python3-gi. Uses uniquely named
temporary units and a private bus name; never stops the actual desktop services.
This is not a CTest test: Debian builders need no running user session.
"""

import json
import os
from pathlib import Path
import signal
import subprocess
import sys
import tempfile
import time


def fixture(role, label, logfile, busname):
    from gi.repository import Gio, GLib

    loop = GLib.MainLoop()
    owner_pid = None

    def mark(event):
        with open(logfile, "a") as stream:
            stream.write(json.dumps(dict(event=event, role=role, label=label,
                                         pid=os.getpid(), owner_pid=owner_pid,
                                         time=time.monotonic())) + "\n")

    def stop():
        mark("stopping")
        time.sleep(float(os.environ.get("STOP_DELAY", "0")))
        mark("exited")
        loop.quit()
        return False

    GLib.unix_signal_add(GLib.PRIORITY_DEFAULT, signal.SIGTERM, stop)
    if role in ("dock", "desktop"):
        def ready():
            name = busname if role == "dock" else busname + ".Desktop"
            Gio.bus_own_name(Gio.BusType.SESSION, name, Gio.BusNameOwnerFlags.NONE,
                             None, lambda *args: mark("ready"), None)
            return False

        GLib.timeout_add(int(os.environ.get("READY_DELAY_MS", "350")), ready)
    elif role == "tray":
        bus = Gio.bus_get_sync(Gio.BusType.SESSION, None)
        reply = bus.call_sync("org.freedesktop.DBus", "/org/freedesktop/DBus",
                              "org.freedesktop.DBus", "GetConnectionUnixProcessID",
                              GLib.Variant("(s)", (busname,)), None,
                              Gio.DBusCallFlags.NONE, 1000, None)
        owner_pid = reply.unpack()[0]
    mark("started")
    loop.run()


def ctl(*args, check=True):
    result = subprocess.run(["systemctl", "--user", *args], text=True,
                            capture_output=True, timeout=15)
    if check and result.returncode:
        raise RuntimeError(f"systemctl {args}: {result.stdout}{result.stderr}")
    return result.stdout


def state(unit):
    return dict(line.split("=", 1) for line in
                ctl("show", unit, "-p", "MainPID", "-p", "ActiveState", "-p", "SubState").splitlines())


def until(predicate):
    deadline = time.monotonic() + 12
    while time.monotonic() < deadline:
        if predicate():
            return
        time.sleep(0.05)
    raise AssertionError("Timed out waiting for service state or fixture event")


def exercise(session_repo, dock_delay, tray_delay, work):
    repo = Path(__file__).resolve().parents[1]
    base = f"tray-lifecycle-test-{os.getpid()}"
    dock, desktop = base + "-dock.service", base + "-desktop.service"
    core, target = base + "-core.target", base + ".target"
    groups = [base + "@one.service", base + "@two.service"]
    bus = f"org.example.TrayLifecycle.p{os.getpid()}"
    logfile = work / "events.jsonl"
    directory = Path(os.environ["XDG_RUNTIME_DIR"]) / "systemd/user"
    directory.mkdir(parents=True, exist_ok=True)
    # Quote paths for systemd's command-line syntax (not a shell).
    command = f'/usr/bin/python3 "{Path(__file__).resolve()}" --fixture'

    def start_command(role, label):
        return f'{command} {role} {label} "{logfile}" {bus}'

    def renamed(source):
        for old, new in {
            "dde-shell@DDE.service": dock,
            "dde-shell-plugin@org.deepin.ds.desktop.service": desktop,
            "dde-session-core.target": core,
            "dde-tray-loader.target": target,
            "dde-tray-loader@": base + "@",
            "org.deepin.dde.Dock1": bus,
        }.items():
            source = source.replace(old, new)
        return source

    dock_source = renamed((session_repo / "systemd/dde-session-core.target.wants/dde-shell@DDE.service").read_text())
    dock_source = "\n".join(
        "ExecStart=" + start_command("dock", "dock") if line.startswith("ExecStart=") else line
        for line in dock_source.splitlines()
        if not line.startswith(("Requisite=", "After=dde-session-pre", "Requires=dbus",
                                "After=dbus", "Wants=org.desktopspec", "Slice=")))
    dock_source = dock_source.replace("[Service]", f"Wants={desktop}\n[Service]")
    dock_source += f"\nEnvironment=READY_DELAY_MS=350 STOP_DELAY={dock_delay}\nTimeoutStopSec=5s\n"
    group_source = renamed((repo / "systemd/dde-tray-loader@.service.in").read_text())
    group_source = "\n".join(
        "ExecStart=" + start_command("tray", "%i") if line.startswith("ExecStart=") else line
        for line in group_source.splitlines() if not line.startswith("Slice="))
    group_source += f"\nEnvironment=STOP_DELAY={tray_delay}\nTimeoutStopSec=5s\n"
    files = {
        dock: dock_source,
        desktop: f"[Unit]\nPartOf={core}\nBefore={core}\n[Service]\n"
                 f"Type=dbus\nBusName={bus}.Desktop\nEnvironment=READY_DELAY_MS=1000\n"
                 f"ExecStart={start_command('desktop', 'desktop')}\n",
        core: f"[Unit]\nDefaultDependencies=no\nWants={dock} {desktop}\n",
        target: renamed((repo / "systemd/dde-tray-loader.target").read_text())
                + "\nWants=" + " ".join(groups) + "\n",
        base + "@.service": group_source,
    }

    def rows(since=0):
        events = [json.loads(line) for line in logfile.read_text().splitlines()] if logfile.exists() else []
        return [event for event in events if event["time"] >= since]

    def trays_started(old=None):
        events = rows()
        for unit in groups:
            current = state(unit)
            if current["ActiveState"] != "active" or (old and current["MainPID"] == old[unit]):
                return False
            if not any(r["role"] == "tray" and r["event"] == "started"
                       and str(r["pid"]) == current["MainPID"] for r in events):
                return False
        return True

    def check_generation(since):
        events = rows(since)
        ready = next(r for r in events if r["role"] == "dock" and r["event"] == "ready")
        starts = [r for r in events if r["role"] == "tray" and r["event"] == "started"]
        assert len(starts) == len(groups), starts
        assert all(r["time"] >= ready["time"] and r["owner_pid"] == ready["pid"] for r in starts), events
        assert state(target)["ActiveState"] == "active"

    all_units = [core, dock, desktop, target, *groups]
    try:
        for name, data in files.items():
            path = directory / name
            assert not path.exists(), path
            path.write_text(data)
        ctl("daemon-reload")
        ctl("start", core)
        until(trays_started)
        check_generation(0)
        events = rows()
        dock_start = next(r["time"] for r in events if r["role"] == "dock" and r["event"] == "started")
        desktop_ready = next(r["time"] for r in events if r["role"] == "desktop" and r["event"] == "ready")
        assert dock_start < desktop_ready, events
        print("PASS: Dock starts before desktop readiness; trays wait for ready Dock", flush=True)

        for crash in (False, True):
            old = {unit: state(unit)["MainPID"] for unit in groups}
            since = time.monotonic()
            if crash:
                ctl("kill", "--kill-whom=main", "--signal=KILL", dock)
            else:
                ctl("restart", dock)
            until(lambda: trays_started(old))
            check_generation(since)
            assert all(not Path("/proc", pid).exists() for pid in old.values())
            print(f"PASS: {'crash recovery' if crash else 'explicit restart'} uses replacement Dock", flush=True)

        since = time.monotonic()
        ctl("stop", core)
        until(lambda: all(state(unit)["MainPID"] == "0" for unit in [dock, desktop, *groups]))
        events = rows(since)
        tray_stops = [r["time"] for r in events if r["event"] == "stopping" and r["role"] == "tray"]
        tray_exits = [r["time"] for r in events if r["event"] == "exited" and r["role"] == "tray"]
        dock_stop = next(r["time"] for r in events if r["role"] == "dock" and r["event"] == "stopping")
        dock_exit = next(r["time"] for r in events if r["role"] == "dock" and r["event"] == "exited")
        desktop_stop = next(r["time"] for r in events if r["role"] == "desktop" and r["event"] == "stopping")
        assert len(tray_stops) == len(groups) and max(tray_stops) - min(tray_stops) < 0.5, events
        assert len(tray_exits) == len(groups) and max(tray_exits) <= dock_stop, events
        # Core shutdown must not make the desktop wait for a slow Dock.
        # Startup and shutdown have no desktop/Dock ordering edge.
        assert abs(desktop_stop - min(tray_stops)) < 0.5, events
        if dock_delay > 0.5:
            assert desktop_stop < dock_exit, events
        if tray_delay > dock_delay:
            assert desktop_stop < min(r["time"] for r in events if r["role"] == "tray" and r["event"] == "exited"), events
        print(f"PASS: tray groups stop in parallel, Dock waits for their exit; "
              f"Dock stops at {dock_stop-since:.3f}s and exits at {dock_exit-since:.3f}s, "
              f"desktop stops at {desktop_stop-since:.3f}s", flush=True)

        delayed = dock_source.replace("READY_DELAY_MS=350", "READY_DELAY_MS=4000")
        (directory / dock).write_text(delayed)
        ctl("daemon-reload")
        since = time.monotonic()
        ctl("start", "--no-block", core)
        until(lambda: state(dock)["SubState"] == "start" and
              any(r["role"] == "dock" and r["event"] == "started" for r in rows(since)))
        ctl("stop", core, dock)
        until(lambda: all(state(unit)["ActiveState"] in ("inactive", "failed") for unit in all_units))
        assert not any(r["role"] == "tray" and r["event"] == "started" for r in rows(since))
        print("PASS: cancellation during Dock startup launches no trays", flush=True)

        failed = delayed.replace("TimeoutStartSec=30s", "TimeoutStartSec=1s").replace("Restart=always", "Restart=no")
        (directory / dock).write_text(failed)
        ctl("daemon-reload")
        # Inactive fixtures can be unloaded between glob expansion and reset.
        # Reset is best-effort cleanup; the state/event assertions below verify
        # that the next start actually succeeds or fails as expected.
        ctl("reset-failed", base + "*", check=False)
        since = time.monotonic()
        ctl("start", "--no-block", core)
        until(lambda: state(dock)["ActiveState"] == "failed" and state(target)["ActiveState"] != "active")
        assert state(target)["ActiveState"] != "active"
        assert not any(r["role"] == "tray" and r["event"] == "started" for r in rows(since))
        print("PASS: Dock startup failure does not launch trays", flush=True)

        ctl("stop", core)
        (directory / dock).write_text(dock_source)
        ctl("daemon-reload")
        ctl("reset-failed", base + "*", check=False)
        since = time.monotonic()
        ctl("start", groups[0])
        until(trays_started)
        check_generation(since)
        print("PASS: starting a tray instance recovers failed Dock via its target", flush=True)
    except Exception:
        print(logfile.read_text() if logfile.exists() else "No fixture events", file=sys.stderr)
        raise
    finally:
        ctl("stop", *all_units, check=False)
        ctl("reset-failed", *all_units, check=False)
        for name in files:
            (directory / name).unlink(missing_ok=True)
        ctl("daemon-reload")


if __name__ == "__main__":
    if sys.argv[1:2] == ["--fixture"]:
        fixture(*sys.argv[2:])
    else:
        if len(sys.argv) != 2:
            sys.exit(__doc__)
        for dock_delay, tray_delay in ((1.5, 0.1), (0.0, 1.5)):
            print(f"Testing dock stop delay={dock_delay}s, tray stop delay={tray_delay}s", flush=True)
            with tempfile.TemporaryDirectory(prefix="tray-lifecycle-") as directory:
                exercise(Path(sys.argv[1]).resolve(), dock_delay, tray_delay, Path(directory))
