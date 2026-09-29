# dde-tray-loader

The `dde-tray-loader` project provides a set of tray plugins that integrated into task bar and the tool loader which can load the plugins.

## Dependencies

Check `debian/control` for build-time and runtime dependencies, or use `cmake` to check the missing required dependencies.

## Building

Regular CMake building steps applies, in short:

```shell
$ cmake -Bbuild
$ cmake --build build
```

After building, the executable is `build/src/loader/trayplugin-loader`.
With a running DDE Dock compositor, use `trayplugin-loader -p /path/to/plugin.so`
to load a plugin. You can optionally install it by:

```shell
$ cmake --install build # only do this if you know what you are doing
```

A `debian` folder is provided to build the package under the *deepin* linux desktop distribution. To build the package, use the following command:

```shell
$ sudo apt build-dep . # install build dependencies
$ dpkg-buildpackage -uc -us -nc -b # build binary package(s)
```

## Tray lifecycle and tests

`dde-shell@DDE.service` starts `dde-tray-loader.target`. The target waits for
the Dock's `Type=dbus` startup to complete, then starts the configured
`dde-tray-loader@<group>.service` instances in parallel. Each instance runs
`trayplugin-loader --group <group>` and resolves its plugin list once.
Plugins within a group initialize on the GUI thread, yielding to the event
loop between plugins so initialized plugins can handle display events.

The target uses `BindsTo=` and `PartOf=` to follow the Dock's lifetime and
restarts. On stop, the tray groups stop in parallel before the Dock. The
desktop and Dock have no mutual startup or shutdown ordering. Readiness is
provided by the Dock's bus name, registered after its compositor is ready;
there is no separate readiness service.

Run the event-dispatch test with `ctest --test-dir build --output-on-failure`.
It uses an offscreen platform, the build-tree interface library, and a writable
cache under the build directory. No running desktop is needed.

For systemd integration, run
`python3 tests/test_systemd_lifecycle.py ../dde-session` with a running user
systemd manager and `python3-gi`. This manual test creates temporary fixture
units to check startup, restart, failure recovery, and shutdown ordering; it
does not restart desktop services and is not part of package-build tests.

For the power cache integration test, build and run it on an isolated bus:

```shell
cmake --build build --target power-dbus-cache-test
dbus-run-session --config-file=tests/power-dbus-session.conf -- build/tests/power-dbus-cache-test
```

It checks failed `GetAll` recovery, delayed snapshots racing with live updates,
partial updates, invalidation, retry cancellation, and service restart.
Snapshots preserve properties updated in flight, and invalidation requests a
fresh snapshot. Failures retry asynchronously after one second; service owner
changes discard pending replies. This manual test is not part of CTest.

## Getting Involved

- [Code contribution via GitHub](https://github.com/linuxdeepin/dde-tray-loader/)
- [Submit bug or suggestions to GitHub Issues or GitHub Discussions](https://github.com/linuxdeepin/developer-center/issues/new/choose)

## License

**dde-tray-loader** is licensed under [GPL-3.0-or-later](LICENSE).
