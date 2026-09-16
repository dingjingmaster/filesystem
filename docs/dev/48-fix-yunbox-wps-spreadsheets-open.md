# 云盒 WPS 表格文件打开错误修复记录

> 文档元数据
> - 文件编号：48
> - 文档类型：fix
> - 文件路径：docs/dev/48-fix-yunbox-wps-spreadsheets-open.md
> - 完成日期：2026-09-16
> - 需求级别：L2

## 1. 问题定义

- 问题现象：Kylin V10 SP1 云盒环境中，通过文件管理器打开 `.et` 文件报错，`wpsoffice /prometheus /yunbox/as.et` 不能稳定拉起 WPS Spreadsheets。
- 预期行为：`.et`、XLS/XLSX 等表格家族文件在云盒中使用 WPS Spreadsheets 组件打开；此前 Writer 在云盒/沙盒中依赖 `wpsoffice /prometheus` 的兼容行为不能退化。
- 影响范围：WPS Office 外部应用启动命令生成逻辑，限定在 `FILESYSTEM_HOOK_MODE=yunbox` 且目标应用为 WPS Spreadsheets 时。
- 不包含：修改远端系统 MIME 配置、修改 WPS 安装、修改云盒 hook 或系统脚本。

## 2. 证据与根因

- 复现方式：远程只读取证 `192.168.122.42`，宿主直接打开 `/home/dingjing/桌面/as.et` 时为 `/usr/bin/et` 和 `office6/et`；云盒 rootfs 内 `/yunbox/as.et` 存在且为 Excel/OLE 文件。
- 证据等级：E2。
- 关键日志/堆栈/输入：
  - `gio mime application/wps-office.et` 默认应用为 `wps-office-et.desktop`。
  - `/usr/share/applications/wps-office-et.desktop` 中 `Exec=/usr/bin/et %F`。
  - `/proc/<pid>/root/yunbox/as.et` 解密后为 OLE，系统 `file --mime-type` 识别为 `application/vnd.ms-excel`。
  - 在同一云盒 rootfs 中，`wpsoffice /prometheus /yunbox/as.et` 退出码为 `255`；直接 `office6/et /yunbox/as.et` 进入 WPS 表格打开流程并输出 `[Open][BEGIN]`/`[Open][END]`。
- 根因：文件管理器对 WPS 表格仍优先生成 `wpsoffice /prometheus <path>`。在 Kylin 云盒环境中，该通用派发入口不能稳定处理 `/yunbox/as.et`，而组件入口 `office6/et <path>` 可直接打开。
- 相关代码路径：`crates/filesystem-gui/src/apps.rs` 的 `build_exec_commands`、WPS 家族识别与命令生成逻辑。

## 3. 修复方案

- 最小修复点：在 `FILESYSTEM_HOOK_MODE=yunbox` 且目标应用识别为 WPS Spreadsheets 时，绕过通用 `wpsoffice /prometheus` 派发器，优先直启 `office6/et <path>`，并保留原 `.desktop` 命令作为 fallback。
- 代码逻辑改动：将云盒 WPS 组件命令生成分支扩展为 Presentation -> `wpp`、Spreadsheets -> `et`；Writer 继续沿用原 `wpsoffice /prometheus` 优先逻辑。
- 影响的使用场景：云盒内打开 `.et`、XLS/XLSX 等由 WPS Spreadsheets 处理的表格文件。
- 不影响的使用场景：普通环境 WPS Spreadsheets 打开、云盒/沙盒内 WPS Writer 的 Prometheus 兼容入口、非 WPS 应用打开。

## 4. 修复执行计划

| 步骤 | 修改内容 | 验证方式 | 状态 |
|------|----------|----------|------|
| 1 | 增加 `yunbox + WPS Spreadsheets` 命令生成回归测试，并保留 Writer Prometheus 行为测试 | `cargo test -p filesystem-gui yunbox_wps_ -- --nocapture` | 完成 |
| 2 | 调整 WPS 命令生成逻辑：云盒 Spreadsheets 优先 `office6/et`，Writer 不变 | `cargo test -p filesystem-gui yunbox_wps_ -- --nocapture` | 完成 |
| 3 | 执行格式化、WPS 相关回归和 GUI crate 测试 | `cargo fmt --check`、相关 `cargo test` | 完成 |

## 5. 风险摘要

| 项 | 结论 |
|----|------|
| 风险矩阵 | L2 |
| 命令权限 | C0/C1 |
| 高风险门禁 | 是，Rust 行为逻辑变更；通过定向回归测试覆盖 |
| 破坏性操作 | 否 |
| 用户已有修改 | 是，包含同日 47 号 `.dps` 修复的未提交改动；本次在其基础上增量修改 |
| 用户确认 | 用户要求修复 |
| 副作用/风险 | 若某 WPS 安装没有 `/opt/kingsoft/wps-office/office6/et`，仍保留原 `/usr/bin/et` desktop 命令 fallback |

## 6. 验证

- 验证环境：本地开发环境 `/data/code/andsec-dev/filesystem`；远端证据环境为 Kylin V10 SP1 `192.168.122.42`。
- 回归/相关验证：
  - `cargo test -p filesystem-gui yunbox_wps_ -- --nocapture`
  - `cargo test -p filesystem-gui wps -- --nocapture`
  - `cargo test -p filesystem-gui build_exec_command -- --nocapture`
  - `cargo fmt --check`
  - `git diff --check`
  - `cargo test -p filesystem-gui`
- 结果：定向新增测试、WPS 相关回归、Exec 命令生成回归、格式检查、diff 空白检查和 GUI crate 全量单测通过。
- 未执行验证项：远端云盒 GUI 实机复测未执行；当前修复以远端命令取证和本地命令生成自动化测试覆盖。
- 残余风险：不同 WPS 安装路径如果缺少标准 `office6/et`，会依赖 `.desktop` fallback。

## 7. 修复总结

- 最终结果：已完成。云盒模式下 WPS Spreadsheets 不再优先走 `wpsoffice /prometheus`，而是先尝试 `office6/et <path>`，并保留原 `.desktop` 命令 fallback；Writer 仍保留 Prometheus 首选入口。
- 计划偏差：无。
- 后续建议：如后续需要实机验证，可在 Kylin 云盒中确认 `.et` 进程为 `office6/et /yunbox/as.et`。
