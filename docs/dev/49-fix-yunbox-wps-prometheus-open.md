# 云盒 WPS Prometheus 打开修复记录

> 文档元数据
> - 文件编号：49
> - 文档类型：fix
> - 文件路径：docs/dev/49-fix-yunbox-wps-prometheus-open.md
> - 完成日期：2026-09-18
> - 需求级别：L2

## 1. 问题定义

- 问题现象：在 192.168.122.65 云盒文件管理器中点击 `.et` 文件没有反应。
- 预期行为：云盒内 WPS 表格、演示和文字文件点击后能拉起 WPS 窗口并打开 `/yunbox/...` 路径。
- 影响范围：`FILESYSTEM_HOOK_MODE=yunbox` 下 WPS Office 文件的默认应用打开命令生成。
- 不包含：不调整非云盒模式、非 WPS 应用、云盒挂载和权限校验逻辑。

## 2. 证据与根因

- 复现方式：在 65 已打开的云盒会话中点击 `/yunbox/WPS表格工作表 2.et`，并手工在同一 chroot 中启动 WPS。
- 证据等级：E2。
- 关键日志/输入：
  - `office6/et /yunbox/*.et` 返回码为 0，但不留下 `et` 进程。
  - `strace` 显示 `office6/et` 内部尝试 `execve("/et", ["/et", "/prometheus", "/yunbox/..." ...]) = -1 ENOENT`，随后 `exit_group(0)`。
  - `wpsoffice /prometheus /yunbox/*.et` 不会秒退，能连接 X11 并进入正常 WPS 初始化流程。
- 根因：前序修复把云盒 WPS 表格/演示改为直启 `office6/et`/`office6/wpp`，但 65 的 WPS 组件入口会走异常的 `/et /prometheus` 路径并以成功码退出；文件管理器只在 `spawn()` 失败时 fallback，因此表现为点击无反应。
- 相关代码路径：`crates/filesystem-gui/src/apps.rs`

## 3. 修复方案

- 最小修复点：移除云盒模式下 WPS 表格/演示直启组件入口的特殊分支，恢复 WPS 家族统一优先 `wpsoffice /prometheus <path>`。
- 代码逻辑改动：
  - `build_exec_commands()` 不再优先调用云盒组件命令生成。
  - 删除 `yunbox_wps_component_command()`、`yunbox_wps_component_name()` 和 `wps_component_executable()`。
  - 云盒 WPS 表格/演示测试期望改为 Prometheus 入口，`.desktop` 命令仍保留 fallback。
- 影响的使用场景：云盒内 WPS Writer、Spreadsheets、Presentation 文件打开。
- 不影响的使用场景：普通模式和沙盒模式仍沿用同一 WPS Prometheus 优先逻辑；非 WPS 文件不变。

## 4. 修复执行计划

| 步骤 | 修改内容 | 验证方式 | 状态 |
|------|----------|----------|------|
| 1 | 现场确认 `office6/et` 秒退和 `wpsoffice /prometheus` 可进入初始化 | 65 `strace` | 已完成 |
| 2 | 撤销云盒 WPS 组件入口特殊分支 | `cargo test -p filesystem-gui yunbox_wps_ -- --test-threads=1 --nocapture` | 已完成 |
| 3 | 构建并部署目标二进制到 65 | 202 release 构建、65 sha256 校验 | 已完成 |

## 5. 风险摘要

| 项 | 结论 |
|----|------|
| 风险矩阵 | L2 |
| 命令权限 | C3，按用户要求在 65 测试环境替换文件管理器二进制 |
| 高风险门禁 | 否 |
| 破坏性操作 | 已备份旧 `/usr/local/andsec/sandbox/bin/sandbox-nemo.bin` |
| 用户已有修改 | 有，基于前序云盒 WPS 打开修复继续修改 |
| 用户确认 | 有，用户要求“解决” |
| 副作用/风险 | 之前 Kylin 环境中 Prometheus 对 `.et/.dps` 的问题可能与 65 的 WPS 版本不同；本次以 65 现场问题为准，保留 `.desktop` fallback |

## 6. 验证

- 验证环境：本地 `/data/code/andsec-dev/filesystem`；202 适配环境 `~/code/andsec-dev/filesystem`；65 现场云盒。
- 回归/相关验证：
  - 本地：`cargo test -p filesystem-gui yunbox_wps_ -- --test-threads=1 --nocapture`
  - 本地：`cargo fmt --check`
  - 本地：`git diff --check`
  - 202：`cargo fmt --check`
  - 202：`cargo test -p filesystem-gui yunbox_wps_ -- --test-threads=1 --nocapture`
  - 202：`cargo build -p filesystem-gui --release`
  - 202：`git diff --check`
- 结果：均通过；65 已替换为 sha256 `7b77dd85cde20f0b8e490279f9ddfd09a55b12ba7859245bfdb1f8e9dae14f8f` 的新 `sandbox-nemo.bin`。
- 未执行验证项：未强制关闭 65 当前已打开云盒窗口；需要用户关闭后重新打开 `.ias` 加载新二进制再点击验证。
- 残余风险：如果其它发行版 WPS Prometheus 仍存在组件派发错误，需要按发行版/WPS 版本继续做兼容分支。

## 7. 修复总结

- 最终结果：云盒 WPS 打开命令回到 `wpsoffice /prometheus <path>`，避免 `office6/et` 在 65 上成功退出但无窗口。
- 计划偏差：原先“直启组件入口绕开融合模式”的判断被 65 `strace` 反证，已撤销。
- 后续建议：65 关闭当前云盒窗口后重新打开 `.ias`，再点击 `.et` 做人工确认。
