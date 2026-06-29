# WPS Office 沙盒 Prometheus 入口修复记录

> 文档元数据
> - 文件编号：26
> - 文档类型：fix
> - 文件路径：docs/dev/26-fix-wps-sandbox-prometheus-open.md
> - 完成日期：2026-06-29
> - 需求级别：L2

## 1. 问题定义

- 问题现象：文件管理器放入 andsec 沙盒后，双击沙盒内 `ss.docx`、`*.xlsx`、`*.pptx`、`*.dps` 等 WPS Office 文档无法稳定打开；同一机器宿主桌面会话里的 WPS/GIO 打开链路此前可用。
- 预期行为：即使沙盒内 WPS 用户配置缺失，也应能从文件管理器打开 WPS Writer、WPS Spreadsheets 和 WPS Presentation 支持的本地文档。
- 影响范围：通过本地 `.desktop` Exec 启动 WPS Office 文档的路径，包括 Writer、表格和演示三类文档。
- 不包含：不修改 andsec 沙盒配置，不依赖 `xdg-open`、DBus、portal 或宿主路径映射。

## 2. 证据与根因

- 复现方式：在 `192.168.122.64` 上双击沙盒内文件管理器中的 `/home/dingjing/ss.docx`。
- 证据等级：E1，远端 `strace -f -e execve` 和 `/proc` 环境取证。
- 关键日志/输入：
  - 文件管理器进程位于 `/var/lib/andsec/sandbox/.sandbox` 根目录下，进程名为 `sandbox-nemo`。
  - 原始双击 Writer 文件执行 `/usr/bin/wps /home/dingjing/ss.docx`，随后进入 `/opt/kingsoft/wps-office/office6/wps /home/dingjing/ss.docx`，底层进程退出 `255`。
  - 沙盒内 `~/.config/Kingsoft/Office.conf` 只有 `[6.0]`，缺少宿主中存在的 `wpsoffice\Application%20Settings\AppComponentMode=prome_fushion`。
  - 临时补齐该配置后，真实双击改为执行 `/opt/kingsoft/wps-office/office6/wpsoffice /prometheus /home/dingjing/ss.docx`，随后可见窗口 `ss.docx - WPS 文字` 出现。
  - 恢复配置后，用文件管理器完整环境直接启动 `wpsoffice /prometheus /home/dingjing/ss.docx` 仍可打开窗口。
  - 远端 `/usr/bin/wps`、`/usr/bin/et`、`/usr/bin/wpp` 三个 wrapper 都读取同一个 `Office.conf` 模式键；缺少该键时分别进入旧 `office6/wps`、`office6/et`、`office6/wpp` 入口。
  - 远端 `wps-office-wps.desktop`、`wps-office-et.desktop`、`wps-office-wpp.desktop` 和 `/usr/share/mime/packages/custom-wps-office.xml` 声明了 WPS Writer、表格、演示三组格式，用户默认应用可能落在无 `MimeType` 的 `wps-office-prometheus.desktop` 或三组组件 `.desktop` 上。
- 根因：WPS wrapper 是否切到 Prometheus 入口依赖用户 `Office.conf`；沙盒内配置缺少该模式键，wrapper 走旧组件入口并失败。文件管理器原先完全依赖 `.desktop` Exec，因此继承了该配置差异。
- 相关代码路径：`crates/filesystem-gui/src/apps.rs`、`crates/filesystem-mime/src/lib.rs`

## 3. 修复方案

- 最小修复点：在 WPS Office 家族应用展开 `.desktop` Exec 后，若启动器是 `wps`、`et` 或 `wpp`，优先生成 `wpsoffice /prometheus <path>` 命令。
- 代码逻辑改动：
  - `open_file_with_app` 支持一组候选启动命令，首个启动失败时尝试 fallback。
  - WPS `/usr/bin/wps`、`/usr/bin/et`、`/usr/bin/wpp` 首选 `/opt/kingsoft/wps-office/office6/wpsoffice /prometheus <path>`。
  - 若 `.desktop` 指向自定义 `/.../office6/wps`、`/.../office6/et` 或 `/.../office6/wpp`，首选同目录 sibling `wpsoffice`。
  - 原 `.desktop` 展开的组件命令保留为 fallback，避免非标准安装中首选路径不存在时完全无法启动。
  - MIME/扩展名覆盖远端 WPS 声明的 Writer、表格和演示格式；另外按用户反馈补充 `*.wpp` 到演示家族兼容兜底。
- 影响的使用场景：WPS Writer、表格、演示家族文档双击、文件菜单默认应用打开和打开方式弹窗启动。
- 不影响的使用场景：非 WPS 应用、非 `wps`/`et`/`wpp` 启动器、WPS PDF 独立 `wpspdf` 启动器。
- 覆盖的主要扩展名：
  - Writer：`doc`、`docm`、`docx`、`dot`、`dotm`、`dotx`、`rtf`、`uof`、`uot`、`uot3`、`uott`、`uott3`、`wps`、`wpsx`、`wpss`、`wpt`、`wptx`、`wpso`。
  - 表格：`et`、`etx`、`eto`、`ets`、`ett`、`ettx`、`uos`、`uos3`、`uost3`、`xls`、`xlsm`、`xlsx`、`xlt`、`xltm`、`xltx`。
  - 演示：`dps`、`dpsx`、`dpss`、`dpt`、`dptx`、`dpso`、`pot`、`potm`、`potx`、`pps`、`ppsm`、`ppsx`、`ppt`、`pptm`、`pptx`、`uop`、`uop3`、`uopt3`、`wpp`。

## 4. 修复执行计划

| 步骤 | 修改内容 | 验证方式 | 状态 |
|------|----------|----------|------|
| 1 | 远端沙盒内对比文件管理器、GIO、WPS wrapper 和 `Office.conf` 行为 | `strace`、`ps`、`xdotool`、`/proc` 只读取证 | 已完成 |
| 2 | 在 `apps.rs` 增加 WPS Prometheus 首选命令和 fallback 启动 | 单元测试和 GUI crate 全量测试 | 已完成 |
| 3 | 扩展 WPS Office MIME/扩展名识别和等价匹配 | MIME 与 GUI 单元测试 | 已完成 |
| 4 | 更新本地上下文与长期概览 | 文档检查 | 已完成 |

## 5. 风险摘要

| 项 | 结论 |
|----|------|
| 风险矩阵 | L2 |
| 命令权限 | C3，已使用用户提供的远端环境和 sudo 只读/临时诊断 |
| 高风险门禁 | 否 |
| 破坏性操作 | 否；远端临时 WPS 配置实验已备份并恢复 |
| 用户已有修改 | 本轮扩展开始时已有同一问题的未提交修改；已基于现状继续合并，未发现无关冲突 |
| 用户确认 | 有，用户要求远端调试该沙盒问题 |
| 副作用/风险 | WPS 非标准安装下首选 Prometheus 路径可能不存在；代码保留原 `.desktop` 命令作为 fallback；`*.wpp` 是按用户环境反馈加入的演示兼容兜底，远端 WPS XML 未声明该扩展 |

## 6. 验证

- 验证环境：本地 `/data/code/filesystem`；远端 `192.168.122.64` Ubuntu 18.04 X11 andsec 沙盒。
- 回归/相关验证：
  - `cargo fmt --check`
  - `cargo test -p filesystem-mime`
  - `cargo test -p filesystem-gui build_exec`
  - `cargo test -p filesystem-gui wps_office_alias_table_covers_common_office_formats`
  - `cargo test -p filesystem-gui`
  - `cargo check -p filesystem-gui`
- 结果：均通过。
- 未执行验证项：未在远端重新编译新二进制并替换运行；远端旧系统 glibc 与本机构建产物此前存在不兼容。
- 残余风险：真实沙盒验证仍需在目标机上用该机环境重新编译或使用兼容 glibc 的产物后手工双击确认。

## 7. 修复总结

- 最终结果：WPS Writer、表格和演示文档启动不再依赖沙盒内 `Office.conf` 是否含 Prometheus 模式，文件管理器会优先直接使用 WPS Prometheus 入口。
- 计划偏差：原先怀疑 X11/DBus runtime 缺失；远端取证显示真实阻断是 WPS wrapper 的配置分支。
- 后续建议：在目标 VM 内重新编译二进制后，用沙盒文件管理器双击 `*.docx`、`*.wps`、`*.xlsx`、`*.pptx`、`*.dps` 和 `*.wpp` 做最终人工回归。
