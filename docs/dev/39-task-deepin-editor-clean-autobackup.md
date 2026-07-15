# deepin-editor 启动前清理自动备份并规避沙盒 D-Bus 等待

> 文档元数据
> - 文件编号：39
> - 文档类型：task
> - 文件路径：docs/dev/39-task-deepin-editor-clean-autobackup.md
> - 文档版本：v1.0.0
> - 最后更新：2026-07-15
> - 需求级别：L2
> - 关联需求：打开 deepin-editor 前自动删除自动备份目录、清理恢复历史；D-Bus 能力改由 sandbox 私有 session bus 提供

## 1. 目标

- 要解决的问题：deepin-editor 从文件管理器打开时会受用户级自动备份目录和恢复历史配置影响；沙盒 session D-Bus 缺失导致的启动/双击卡顿改由 sandbox 私有 session bus 解决，filesystem 不再做 deepin-editor 专项 D-Bus/fcitx 兼容。
- 成功标准：文件管理器识别到 deepin-editor 时，在启动前清理固定自动备份目录，并将 `~/.config/deepin/deepin-editor/config.conf` 中 `[advance.editor.browsing_history_temfile] value` 置为 `@Invalid()`；filesystem 不扫描 `/proc`、不查找 fcitx bus、不设置 deepin-editor 专属 `DBUS_SESSION_BUS_ADDRESS`；非 deepin-editor 不触发清理；缺失目录或配置不报错；根路径为符号链接时只删除符号链接自身。

## 2. 背景与边界

- 背景：远程排查显示 deepin-editor 配置中存在自动备份/历史恢复数据，且用户明确拒绝预启动方案，要求启动前删除自动备份目录；后续采样确认双击时会出现第二个 `deepin-editor` 进程并存活十几秒，已有 editor 窗口短暂不可编辑，配置中仍残留 `modify=true` 的恢复历史项。旧方案在缺失 session D-Bus 时尝试借助同 namespace 的 fcitx bus，但现场存在无 fcitx 进程环境，因此该兼容逻辑移出 filesystem，改由 sandbox 提供私有 session bus。
- 包含：`crates/filesystem-gui/src/apps.rs` 中外部应用启动前的 deepin-editor 专项清理逻辑和单元测试。
- 不包含：修改 deepin-editor 程序、预启动、替换为 `gio`/`xdg-open`、调整其它应用打开逻辑、在 filesystem 中处理 session D-Bus。
- 关键假设：目标目录和配置文件是当前用户家目录下的 deepin-editor 固定路径；删除自动备份目录和清空恢复历史会丢失 deepin-editor 未保存内容的恢复数据，用户已明确要求/同意该行为。
- 非目标：预启动 deepin-editor、修改系统 D-Bus/输入法服务、改变所有外部应用的打开环境。
- 最大修改范围：`crates/filesystem-gui/src/apps.rs`、本地任务文档和索引。
- 禁止触碰范围：不执行远端或本机真实用户目录删除命令；不修改 `AGENTS.project.md`；不提交 `docs/dev`，除非用户明确要求。

## 3. 风险门禁

| 项 | 结论 |
|----|------|
| 风险矩阵 | L2：单模块行为变更，涉及用户级持久化恢复数据的删除逻辑；D-Bus 运行时能力迁移到 sandbox 项目 |
| 高风险开发门禁 | 是：持久化数据/用户数据副作用，按 L2 执行 |
| 破坏性操作 | 是：运行时删除 deepin-editor 自动备份目录并清空恢复历史配置；用户已明确要求/同意代码行为，不执行真实目录删除命令 |
| 用户已有修改 | 是：`docs/dev` 存在上一任务本地上下文未提交；本次仅追加 39 文档和索引行 |
| 底层/系统风险 | 否：不涉及 Rust unsafe、内核、ABI/API、并发 |
| 命令权限 | C1：工作区内代码/文档修改和本地测试；远端只读/短超时启动探测；不执行 C3 删除真实用户数据命令 |
| 用户确认事项 | 用户已明确要求 deepin-editor 打开前自动删除指定目录 |
| 回滚/止损方式 | 回退本次代码即可停止自动删除和配置清理；已删除/清空的 deepin-editor 恢复数据不可由文件管理器恢复 |

## 4. 方案

- 推荐方案：在 `open_file_with_app` 成功构建启动命令后，若应用 ID 或启动命令指向 `deepin-editor`，使用 Rust 文件 API 清理 `$HOME/.local/share/deepin/deepin-editor/autoBackup-files`，并定点更新 `$HOME/.config/deepin/deepin-editor/config.conf` 的 `[advance.editor.browsing_history_temfile] value=@Invalid()`；删除 filesystem 内所有 deepin-editor 专项 D-Bus/fcitx 环境覆盖。
- 取舍理由：filesystem 只负责应用启动前的 deepin-editor 用户态恢复数据清理；session D-Bus 是沙盒运行时能力，应由 sandbox 私有 bus 统一提供，避免依赖 fcitx 或宿主 session bus。
- 风险与应对：删除/清空恢复数据存在数据丢失风险，通过固定路径、应用识别、定点 INI section/key 更新和符号链接根路径保护缩小影响面；D-Bus 行为依赖 sandbox 新增私有 session bus，filesystem 不再兜底。

## 5. 执行计划

| 步骤 | 修改内容 | 验证方式 | 状态 |
|------|----------|----------|------|
| 1 | 记录任务边界、风险和计划 | 人工检查文档与索引 | 完成 |
| 2 | 实现 deepin-editor 启动前清理固定自动备份目录 | `cargo test -p filesystem-gui deepin_editor` | 完成 |
| 3 | 补充目录删除、缺失目录、符号链接和非 deepin-editor 识别测试 | `cargo test -p filesystem-gui deepin_editor` | 完成 |
| 4 | 运行格式和回归验证 | `cargo fmt --check`、相关测试、`git diff --check` | 完成 |
| 5 | 清理 deepin-editor 恢复历史配置 | `cargo test -p filesystem-gui deepin_editor` | 完成 |
| 6 | 删除 deepin-editor 缺失 session D-Bus 时发现并设置同 namespace fcitx bus 的逻辑 | `cargo test -p filesystem-gui deepin_editor`、`cargo test -p filesystem-gui` | 完成 |

## 6. 实现记录

- 修改文件：`crates/filesystem-gui/src/apps.rs`
- 关键决策：在 `open_file_with_app` 构建启动命令成功后、`spawn` 前执行专项清理；通过 desktop app id 或最终启动程序名识别 `deepin-editor`；固定清理 `$HOME/.local/share/deepin/deepin-editor/autoBackup-files`；定点清理 `[advance.editor.browsing_history_temfile] value`；根路径为符号链接时删除链接自身，不递归进入目标目录；删除 `/proc` 扫描 fcitx bus 和设置 deepin-editor 专属 `DBUS_SESSION_BUS_ADDRESS` 的代码。
- 计划偏差：根据后续远端采样结论追加清理恢复历史配置，原因是只删自动备份目录后，配置里仍可能保留 `modify=true` 的恢复历史项；原 fcitx bus 兜底因无 fcitx 现场不可用，已迁移为 sandbox 私有 session bus 方案。
- 安全门禁执行结果：未执行真实用户目录删除命令；代码行为会在用户运行文件管理器并打开 deepin-editor 时删除指定恢复目录、清空指定恢复历史配置；清理失败按尽力清理处理，不阻断文件打开。

## 7. 验证记录

- 验证环境：本地开发环境
- 系统信息（OS/内核/架构/编译器/运行时，按需）：Linux 7.0.11-gentoo-dingjing x86_64 GNU/Linux；rustc 1.95.0；cargo 1.95.0

| 验证项 | 命令/步骤 | 结果 | 备注 |
|--------|-----------|------|------|
| deepin-editor 专项测试 | `cargo test -p filesystem-gui deepin_editor` | 通过 | 10 个测试覆盖路径、目录删除、缺失目录、符号链接、应用识别和恢复历史配置清理 |
| GUI crate 回归测试 | `cargo test -p filesystem-gui` | 通过 | 139 个测试通过 |
| 格式检查 | `cargo fmt --check` | 通过 | Rust 格式 |
| Diff 空白检查 | `git diff --check` | 通过 | 无空白错误 |
| 远端沙盒 D-Bus 监控 | `dbus-monitor --session "type='method_call'"` + `deepin-editor /home/dingjing/s.c` | 通过 | 二次启动调用 `com.deepin.Editor.openFilesInTab` 后，editor 端卡在 `com.deepin.dde.daemon.Dock` 的 `Introspect`/自动激活上约 18-25s |
| 远端 fcitx bus 探测 | `DBUS_SESSION_BUS_ADDRESS=unix:abstract=/tmp/dbus-q8fXyGcQd7 deepin-editor ...` | 通过 | 首次启动初始化约 145ms；二次打开约 47ms；文件能正常打开 |

- 未执行验证项：尚未替换远端正在运行的 sandbox 和文件管理器二进制做端到端双击实测；该操作会覆盖测试环境二进制并停止/重启进程，需要单独执行部署步骤。
- 残余风险：该行为会删除 deepin-editor 未保存内容的自动恢复文件并清空恢复历史；session D-Bus 卡顿依赖 sandbox 私有 bus 的目标环境验证。

## 8. 总结

- 最终结果：文件管理器打开 deepin-editor 前会自动清理用户级 `autoBackup-files` 目录、清空 deepin-editor 恢复历史配置；filesystem 不再为 deepin-editor 查找 fcitx bus 或设置专属 D-Bus 地址；非 deepin-editor 不触发。
- 遗留风险：恢复文件和恢复历史删除/清空不可由文件管理器自动恢复；清理失败时仍继续打开应用；需要部署 sandbox 私有 session bus 后确认完整双击链路。
- 后续建议：远端替换测试环境 sandbox 和文件管理器二进制前先备份原二进制，再重启相关进程做首次打开和重复双击实测。
