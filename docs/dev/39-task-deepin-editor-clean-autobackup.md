# deepin-editor 启动前清理自动备份并规避沙盒 D-Bus 等待

> 文档元数据
> - 文件编号：39
> - 文档类型：task
> - 文件路径：docs/dev/39-task-deepin-editor-clean-autobackup.md
> - 文档版本：v1.0.0
> - 最后更新：2026-07-15
> - 需求级别：L2
> - 关联需求：打开 deepin-editor 前自动删除自动备份目录、清理恢复历史，并解决沙盒内 DDE Dock D-Bus 自动激活导致的启动/双击卡顿

## 1. 目标

- 要解决的问题：deepin-editor 从文件管理器打开时会受用户级自动备份目录和恢复历史配置影响；远程沙盒实测还显示，deepin-editor 二次启动/单实例转发会在 DDE Dock D-Bus 自动激活上等待 18-25 秒，导致已有编辑器窗口短时间不可编辑。
- 成功标准：文件管理器识别到 deepin-editor 时，在启动前清理固定自动备份目录，并将 `~/.config/deepin/deepin-editor/config.conf` 中 `[advance.editor.browsing_history_temfile] value` 置为 `@Invalid()`；当父进程缺失或空 `DBUS_SESSION_BUS_ADDRESS` 时，为 deepin-editor 子进程发现并设置同 namespace 的 fcitx D-Bus 地址；非 deepin-editor 不触发；正常已有 D-Bus 地址不覆盖；缺失目录或配置不报错；根路径为符号链接时只删除符号链接自身。

## 2. 背景与边界

- 背景：远程排查显示 deepin-editor 配置中存在自动备份/历史恢复数据，且用户明确拒绝预启动方案，要求启动前删除自动备份目录；后续采样确认双击时会出现第二个 `deepin-editor` 进程并存活十几秒，已有 editor 窗口短暂不可编辑，配置中仍残留 `modify=true` 的恢复历史项。再次进入用户指定沙盒 root 后，确认当前运行的 `sandbox-nemo` 子进程环境没有 `DBUS_SESSION_BUS_ADDRESS`；D-Bus 监控显示二次启动会调用已有实例 `com.deepin.Editor.openFilesInTab`，随后 editor 端在 `com.deepin.dde.daemon.Dock` 的 `Introspect`/自动激活上等待 18-25 秒。
- 包含：`crates/filesystem-gui/src/apps.rs` 中外部应用启动前的 deepin-editor 专项清理逻辑和单元测试。
- 不包含：修改 deepin-editor 配置、预启动、替换为 `gio`/`xdg-open`、调整其它应用打开逻辑。
- 关键假设：目标目录和配置文件是当前用户家目录下的 deepin-editor 固定路径；删除自动备份目录和清空恢复历史会丢失 deepin-editor 未保存内容的恢复数据，用户已明确要求/同意该行为。
- 非目标：预启动 deepin-editor、修改系统 D-Bus/输入法服务、改变所有外部应用的打开环境。
- 最大修改范围：`crates/filesystem-gui/src/apps.rs`、本地任务文档和索引。
- 禁止触碰范围：不执行远端或本机真实用户目录删除命令；不修改 `AGENTS.project.md`；不提交 `docs/dev`，除非用户明确要求。

## 3. 风险门禁

| 项 | 结论 |
|----|------|
| 风险矩阵 | L2：单模块行为变更，涉及用户级持久化恢复数据的删除逻辑 |
| 高风险开发门禁 | 是：持久化数据/用户数据副作用，按 L2 执行 |
| 破坏性操作 | 是：运行时删除 deepin-editor 自动备份目录并清空恢复历史配置；用户已明确要求/同意代码行为，不执行真实目录删除命令 |
| 用户已有修改 | 是：`docs/dev` 存在上一任务本地上下文未提交；本次仅追加 39 文档和索引行 |
| 底层/系统风险 | 否：不涉及 Rust unsafe、内核、ABI/API、并发 |
| 命令权限 | C1：工作区内代码/文档修改和本地测试；远端只读/短超时启动探测；不执行 C3 删除真实用户数据命令 |
| 用户确认事项 | 用户已明确要求 deepin-editor 打开前自动删除指定目录 |
| 回滚/止损方式 | 回退本次代码即可停止自动删除和配置清理；已删除/清空的 deepin-editor 恢复数据不可由文件管理器恢复 |

## 4. 方案

- 推荐方案：在 `open_file_with_app` 成功构建启动命令后，若应用 ID 或启动命令指向 `deepin-editor`，使用 Rust 文件 API 清理 `$HOME/.local/share/deepin/deepin-editor/autoBackup-files`，并定点更新 `$HOME/.config/deepin/deepin-editor/config.conf` 的 `[advance.editor.browsing_history_temfile] value=@Invalid()`；在 `spawn` 前若最终程序名是 `deepin-editor` 且父进程缺失/空 `DBUS_SESSION_BUS_ADDRESS`，从当前 `/proc` 的同 mount namespace、同 UID 进程中找到 fcitx `dbus-daemon --config-file /usr/share/fcitx/dbus/daemon.conf` 的监听 socket，并设置为 deepin-editor 子进程的 `DBUS_SESSION_BUS_ADDRESS`。
- 取舍理由：保持现有直接启动路径，不引入 `gio`/`xdg-open`，避免破坏沙盒内文件路径可见性；不可达 D-Bus 虽能快速返回但会破坏 deepin-editor 单实例打开语义，已废弃；同 namespace 的 fcitx bus 能保留单实例转发，又没有 DDE Dock 自动激活声明，远端验证首次初始化约 145ms、二次打开约 47ms。
- 风险与应对：删除/清空恢复数据存在数据丢失风险，通过固定路径、应用识别、定点 INI section/key 更新和符号链接根路径保护缩小影响面；fcitx bus 只在父进程没有有效 session bus 时启用，正常桌面已有 D-Bus 地址时不覆盖；找不到 fcitx bus 时不设置环境，保持原行为。

## 5. 执行计划

| 步骤 | 修改内容 | 验证方式 | 状态 |
|------|----------|----------|------|
| 1 | 记录任务边界、风险和计划 | 人工检查文档与索引 | 完成 |
| 2 | 实现 deepin-editor 启动前清理固定自动备份目录 | `cargo test -p filesystem-gui deepin_editor` | 完成 |
| 3 | 补充目录删除、缺失目录、符号链接和非 deepin-editor 识别测试 | `cargo test -p filesystem-gui deepin_editor` | 完成 |
| 4 | 运行格式和回归验证 | `cargo fmt --check`、相关测试、`git diff --check` | 完成 |
| 5 | 清理 deepin-editor 恢复历史配置 | `cargo test -p filesystem-gui deepin_editor` | 完成 |
| 6 | deepin-editor 缺失 session D-Bus 时发现并设置同 namespace 的 fcitx bus | 远端 namespace 探测、`cargo test -p filesystem-gui deepin_editor` | 完成 |

## 6. 实现记录

- 修改文件：`crates/filesystem-gui/src/apps.rs`
- 关键决策：在 `open_file_with_app` 构建启动命令成功后、`spawn` 前执行专项清理；通过 desktop app id 或最终启动程序名识别 `deepin-editor`；固定清理 `$HOME/.local/share/deepin/deepin-editor/autoBackup-files`；定点清理 `[advance.editor.browsing_history_temfile] value`；根路径为符号链接时删除链接自身，不递归进入目标目录；在最终程序名为 `deepin-editor` 且父进程缺失/空 `DBUS_SESSION_BUS_ADDRESS` 时，优先使用同 namespace 的 fcitx bus，避开 DDE Dock 自动激活等待，同时保留 deepin-editor 单实例打开语义。
- 计划偏差：根据后续远端采样结论追加清理恢复历史配置，原因是只删自动备份目录后，配置里仍可能保留 `modify=true` 的恢复历史项；再次远端确认后追加 fcitx bus 环境，原因是 `autoBackup-files` 和恢复历史不是首次启动超过 1 分钟的主因，真正阻塞点是 DDE Dock D-Bus 自动激活。
- 安全门禁执行结果：未执行真实用户目录删除命令；代码行为会在用户运行文件管理器并打开 deepin-editor 时删除指定恢复目录、清空指定恢复历史配置；清理失败按尽力清理处理，不阻断文件打开。

## 7. 验证记录

- 验证环境：本地开发环境
- 系统信息（OS/内核/架构/编译器/运行时，按需）：Linux 7.0.11-gentoo-dingjing x86_64 GNU/Linux；rustc 1.95.0；cargo 1.95.0

| 验证项 | 命令/步骤 | 结果 | 备注 |
|--------|-----------|------|------|
| deepin-editor 专项测试 | `cargo test -p filesystem-gui deepin_editor` | 通过 | 14 个测试覆盖路径、目录删除、缺失目录、符号链接、应用识别、恢复历史配置清理和 D-Bus 环境覆盖 |
| GUI crate 回归测试 | `cargo test -p filesystem-gui` | 通过 | 143 个测试通过 |
| 格式检查 | `cargo fmt --check` | 通过 | Rust 格式 |
| Diff 空白检查 | `git diff --check` | 通过 | 无空白错误 |
| 远端沙盒 D-Bus 监控 | `dbus-monitor --session "type='method_call'"` + `deepin-editor /home/dingjing/s.c` | 通过 | 二次启动调用 `com.deepin.Editor.openFilesInTab` 后，editor 端卡在 `com.deepin.dde.daemon.Dock` 的 `Introspect`/自动激活上约 18-25s |
| 远端 fcitx bus 探测 | `DBUS_SESSION_BUS_ADDRESS=unix:abstract=/tmp/dbus-q8fXyGcQd7 deepin-editor ...` | 通过 | 首次启动初始化约 145ms；二次打开约 47ms；文件能正常打开 |

- 未执行验证项：尚未替换远端正在运行的 `sandbox-nemo` 二进制并重启图形文件管理器做端到端双击实测；该操作会覆盖测试环境二进制并停止/重启进程，需要单独执行部署步骤。
- 残余风险：该行为会删除 deepin-editor 未保存内容的自动恢复文件并清空恢复历史；使用 fcitx bus 会让 deepin-editor 在该沙盒中看不到 DDE Dock 等 session bus 服务，但远端验证显示这是规避当前 18-25 秒卡顿且保持文件打开的有效路径。

## 8. 总结

- 最终结果：文件管理器打开 deepin-editor 前会自动清理用户级 `autoBackup-files` 目录、清空 deepin-editor 恢复历史配置；当沙盒父进程没有 session D-Bus 地址时，deepin-editor 子进程会使用同 namespace 的 fcitx bus，避免 DDE Dock 自动激活造成的 18-25 秒等待；非 deepin-editor 不触发。
- 遗留风险：恢复文件和恢复历史删除/清空不可由文件管理器自动恢复；清理失败时仍继续打开应用；需要部署到远端运行二进制后才能确认完整双击链路。
- 后续建议：远端替换测试环境 `sandbox-nemo` 前先备份原二进制，再重启文件管理器进程做首次打开和重复双击实测。
