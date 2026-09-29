# dde-tray-loader

dde-tray-loader项目提供一套任务栏的托盘插件以及加载插件的工具。

## 依赖

查看 `debian/control` 文件来了解此项目的构建与运行时依赖，或者使用 `cmake` 检查缺失的必要组件。

## 构建

常规的 CMake 构建步骤，即：

```shell
$ cmake -Bbuild
$ cmake --build build
```

构建完毕后，可执行文件位于 `build/src/loader/trayplugin-loader`。
在 DDE Dock 合成器运行时，可以执行 `trayplugin-loader -p /path/to/plugin.so` 加载插件。
也可以通过如下方式进行安装：

```shell
$ cmake --install build # 当你知道这条命令的作用时再执行它
```

为在 *deepin* 桌面发行版进行此软件包的构建，我们还提供了一个 `debian` 目录。若要构建软件包，可参照下面的命令进行构建：

```shell
$ sudo apt build-dep . # 安装构建依赖
$ dpkg-buildpackage -uc -us -nc -b # 构建二进制软件包
```

## 托盘生命周期与测试

`dde-shell@DDE.service` 拉起 `dde-tray-loader.target`。target 等待 Dock 的
`Type=dbus` 启动完成后，并行启动配置的 `dde-tray-loader@<group>.service` 实例。
每个实例执行 `trayplugin-loader --group <group>`，只解析一次插件列表。
同组插件在 GUI 线程上逐个初始化，插件之间返回事件循环，使已初始化的插件
能够处理显示事件。

target 通过 `BindsTo=` 和 `PartOf=` 跟随 Dock 的生命周期及重启。
停止时各托盘组并行退出，然后 Dock 退出；桌面与 Dock 之间没有启动或退出顺序约束。
Dock 在合成器就绪后注册总线名称，以此提供就绪保证，无需单独的 ready 服务。

执行 `ctest --test-dir build --output-on-failure` 运行事件分发测试。
测试使用 offscreen 平台、构建目录中的接口库和可写缓存，无需运行中的桌面。

如需验证 systemd 集成，在具备用户 systemd manager 和 `python3-gi` 的环境运行
`python3 tests/test_systemd_lifecycle.py ../dde-session`。
此手动测试通过临时模拟单元验证启动、重启、失败恢复及退出顺序，不重启实际桌面服务，
也不参与打包阶段的自动测试。

电源缓存集成测试需在隔离总线上执行：

```shell
cmake --build build --target power-dbus-cache-test
dbus-run-session --config-file=tests/power-dbus-session.conf -- build/tests/power-dbus-cache-test
```

用例验证 `GetAll` 失败恢复、延迟快照与实时通知交错、部分属性更新、属性失效、
重试取消及服务重启。快照保留请求期间已更新的属性，属性失效后重新获取快照；
失败后间隔一秒异步重试，服务拥有者变化时丢弃未完成的旧请求。
此手动测试不参与 CTest。

## 参与贡献

- [通过 GitHub 发起代码贡献](https://github.com/linuxdeepin/dde-tray-loader/)
- [通过 GitHub Issues 与 GitHub Discussions 汇报缺陷与反馈建议](https://github.com/linuxdeepin/developer-center/issues/new/choose)

## 许可协议

**dde-tray-loader** 使用 [GPL-3.0-or-later](LICENSE) 许可协议进行发布。
