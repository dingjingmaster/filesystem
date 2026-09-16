# 云盒 WPS 演示文件打开错误修复记录

> 文档元数据
> - 文件编号：47
> - 文档类型：fix
> - 文件路径：docs/dev/47-fix-yunbox-wps-presentation-open.md
> - 完成日期：2026-09-16
> - 需求级别：L2

## 1. 问题定义

- 问题现象：Kylin V10 SP1 云盒环境中，通过文件管理器打开 `.dps` 文件时实际进入 WPS Writer 组件，导致演示文件打开出错。
- 预期行为：`.dps`、PPT/PPTX 等演示家族文件在云盒中使用 WPS Presentation 组件打开；此前 Writer/表格在沙盒中依赖 `wpsoffice /prometheus` 的兼容行为不能退化。
- 影响范围：WPS Office 外部应用启动命令生成逻辑，限定在 `FILESYSTEM_HOOK_MODE=yunbox` 且目标应用为 WPS Presentation 时。
- 不包含：修改远端系统 MIME 配置、修改 WPS 安装、修改云盒 hook 或系统脚本。

## 2. 证据与根因

- 复现方式：远程只读取证 `192.168.122.42`，宿主直接打开 `/home/dingjing/桌面/ss.dps` 时为 `/usr/bin/wpp` 和 `office6/wpp`；云盒异常进程为 `office6/wps /yunbox/ss.dps`。
- 证据等级：E2。
- 关键日志/堆栈/输入：
  - `xdg-mime query filetype /home/dingjing/桌面/ss.dps` 返回 `application/wps-office.dps`。
  - `gio mime application/wps-office.dps` 默认应用为 `wps-office-wpp.desktop`。
  - 云盒异常进程环境包含 `FILESYSTEM_HOOK_MODE=yunbox`、`LD_PRELOAD=/usr/local/andsec/hook/yun/fileman.so`、`BOXDIR=/yunbox`。
  - `/proc/<pid>/root/yunbox/ss.dps` 解密后为 OLE，系统 `file --mime-type` 识别为 `application/vnd.ms-powerpoint`。
- 根因：文件管理器对 WPS Writer、表格、演示统一优先生成 `wpsoffice /prometheus <path>`。在 Kylin 云盒环境中，`wpsoffice /prometheus /yunbox/ss.dps` 会被 WPS 内部派发到 Writer 组件 `wps`，而不是 Presentation 组件 `wpp`。
- 相关代码路径：`crates/filesystem-gui/src/apps.rs` 的 `build_exec_commands`、WPS 家族识别与命令生成逻辑。

## 3. 修复方案

- 最小修复点：仅在 `FILESYSTEM_HOOK_MODE=yunbox` 且目标应用识别为 WPS Presentation 时，绕过通用 `wpsoffice /prometheus` 派发器，优先直启 `office6/wpp <path>`，并保留原 `.desktop` 命令作为 fallback。
- 代码逻辑改动：新增云盒 WPS 演示组件命令生成分支；Writer 和 Spreadsheets 继续沿用原 `wpsoffice /prometheus` 优先逻辑。
- 影响的使用场景：云盒内打开 `.dps`、PPT/PPTX 等由 WPS Presentation 处理的演示文件。
- 不影响的使用场景：普通环境 WPS Presentation 打开、云盒/沙盒内 WPS Writer 和 Spreadsheets 的 Prometheus 兼容入口、非 WPS 应用打开。

## 4. 修复执行计划

| 步骤 | 修改内容 | 验证方式 | 状态 |
|------|----------|----------|------|
| 1 | 增加 `yunbox + WPS Presentation` 命令生成回归测试，并保留 Writer/表格 Prometheus 行为测试 | `cargo test -p filesystem-gui yunbox_wps_ -- --nocapture` | 完成 |
| 2 | 调整 WPS 命令生成逻辑：云盒 Presentation 优先 `office6/wpp`，其它 WPS 家族不变 | `cargo test -p filesystem-gui yunbox_wps_ -- --nocapture` | 完成 |
| 3 | 执行格式化、WPS 相关回归和 GUI crate 测试 | `cargo fmt --check`、相关 `cargo test` | 完成 |

## 5. 风险摘要

| 项 | 结论 |
|----|------|
| 风险矩阵 | L2 |
| 命令权限 | C0/C1 |
| 高风险门禁 | 是，Rust 行为逻辑变更；通过定向回归测试覆盖 |
| 破坏性操作 | 否 |
| 用户已有修改 | 否，修改前工作区干净 |
| 用户确认 | 用户要求修复 |
| 副作用/风险 | 若某 WPS 安装没有 `/opt/kingsoft/wps-office/office6/wpp`，仍保留原 `/usr/bin/wpp` desktop 命令 fallback |

## 6. 验证

- 验证环境：本地开发环境 `/data/code/andsec-dev/filesystem`；远端证据环境为 Kylin V10 SP1 `192.168.122.42`。
- 回归/相关验证：
  - `cargo test -p filesystem-gui yunbox_wps_ -- --nocapture`
  - `cargo test -p filesystem-gui wps -- --nocapture`
  - `cargo test -p filesystem-gui build_exec_command -- --nocapture`
  - `cargo fmt --check`
  - `cargo test -p filesystem-gui`
- 结果：定向新增测试、WPS 相关回归、Exec 命令生成回归、格式检查和 GUI crate 全量单测通过。
- 未执行验证项：远端云盒实机复测未执行；当前修复以命令生成自动化测试覆盖。
- 残余风险：不同 WPS 安装路径如果缺少标准 `office6/wpp`，会依赖 `.desktop` fallback。

## 7. 修复总结

- 最终结果：已完成。云盒模式下 WPS Presentation 不再优先走 `wpsoffice /prometheus`，而是先尝试 `office6/wpp <path>`，并保留原 `.desktop` 命令 fallback；Writer/表格仍保留 Prometheus 首选入口。
- 计划偏差：无。
- 后续建议：如后续需要实机验证，可在 Kylin 云盒中确认 `.dps` 进程为 `office6/wpp /yunbox/ss.dps`。
