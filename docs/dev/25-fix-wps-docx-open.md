# WPS 文档和 WPS 原生格式打开兼容修复记录

> 文档元数据
> - 文件编号：25
> - 文档类型：fix
> - 文件路径：docs/dev/25-fix-wps-docx-open.md
> - 完成日期：2026-06-29
> - 需求级别：L3（由 L2 升级；真实远端取证后同时涉及 `filesystem-mime`、`filesystem-gui` 和 MIME apps 写入/解析规则）

## 1. 问题定义

- 问题现象：用户反馈 `192.168.122.64` 虚拟机上，本文件管理器可以用 WPS 表格打开 `*.et`，但无法使用 WPS 文档打开 `*.docx`；第一轮和第二轮修复后仍不行，且 `*.wps` 也应使用 WPS 打开；用户补充通过 GUI 文件视图双击 `*.wps/*.docx` 仍打不开。
- 预期行为：远端 Nautilus/GIO 能打开的 `*.docx`、`*.wps`，本文件管理器也应按同一套本地 `.desktop` 和 MIME apps 规则选择 WPS 并启动；外部应用 wrapper 退出后不应在本进程下留下 zombie。
- 影响范围：普通文件双击默认打开、文件菜单“用 xxx 打开”、打开方式候选列表和默认应用匹配。
- 不包含：修改系统 `mimeapps.list`、安装或配置 WPS、远程虚拟机写操作。

## 2. 证据与根因

- 复现方式：使用用户提供的远端账号信息登录 `192.168.122.64` 后取证；远端 `xdg-mime query filetype /home/dingjing/Desktop/WPS文字文档\ 2.docx` 返回 `application/wps-office.docx`，`gio mime application/vnd.openxmlformats-officedocument.wordprocessingml.document` 的默认应用为 `wps-office-prometheus.desktop`；当前运行的 `/home/dingjing/filesystem-gui` 下存在多条 `[wps] <defunct>` 子进程。安装 `xdotool` 后对真实 GUI 文件视图双击 `aaa.wps` 和 `ss.docx`，`strace` 显示当前远端二进制执行 `/usr/bin/wps file:///home/dingjing/Desktop/aaa.wps`、`/usr/bin/wps file:///home/dingjing/Desktop/ss.docx`，WPS wrapper 后续只执行 `wpsoffice /home/...` 且未弹出窗口；对比 `gio open /home/dingjing/Desktop/ss.docx`，GIO 执行 `/usr/bin/wps /home/dingjing/Desktop/ss.docx`，WPS wrapper 后续带 `/prometheus` 和 `/from_prome` 参数。
- 证据等级：E2。远端真实环境可登录取证；GIO/xdg-open/WPS wrapper 启动命令可在同一图形会话环境返回成功；本地回归测试覆盖对应差异。
- 关键日志/堆栈/输入：远端 `DISPLAY=:0`、`XDG_CURRENT_DESKTOP=ubuntu:GNOME`；`/usr/share/applications/wps-office-prometheus.desktop` 存在 `Exec=/usr/bin/wps %F` 但没有 `MimeType`；`/usr/share/applications/wps-office-wps.desktop` 存在 `Exec=/usr/bin/wps %U` 并声明 `application/wps-office.docx`、标准 OOXML MIME 等；`gio open /home/dingjing/Desktop/WPS文字文档\ 2.docx` 返回 0，同时警告 `~/.config/ubuntu-mimeapps.list` 不允许 `[Added Associations]`；远端样例 `*.wps/*.docx` 文件头为 `18 44 52 4d ...`，不是标准 zip OOXML，`file --mime-type` 返回 `application/octet-stream`，但 GIO fast content type 返回 `application/wps-office.docx` 或 `application/wps-office.wps`。
- 根因：Nautilus/GIO 会把只出现在 `[Default Applications]` 的 `wps-office-prometheus.desktop` 当成可启动默认应用，即使该 desktop 文件没有 `MimeType`；本项目早期会过滤无 `MimeType` 且非 TextEditor 的 app，导致默认应用被忽略。第二轮修复又无差别合并 desktop-specific `ubuntu-mimeapps.list` 的 `[Added Associations]`/`[Removed Associations]`，与 GIO 规则不一致；默认应用写入也会把 Added Associations 写进 desktop-specific 文件，产生 GIO 警告。真实 GUI 双击暴露两个额外问题：后台内容识别可能让远端 WPS 自有二进制头掉到 `application/octet-stream` 并覆盖扩展名 MIME；WPS Writer 的 desktop 文件使用 `%U`，按标准展开成 `file://` URI 后 WPS wrapper 与 GIO 的本地路径启动路径不同，导致不弹出文档窗口。另一个明确问题是 `Command::spawn()` 后直接丢弃 `Child`，WPS wrapper 退出后成为 zombie。
- 相关代码路径：`crates/filesystem-mime/src/lib.rs`、`crates/filesystem-gui/src/apps.rs`。

## 3. 修复方案

- 最小修复点：在 MIME 识别层补 WPS 原生扩展名，在应用候选匹配层把 WPS Writer、WPS Spreadsheets、WPS Presentation 各自的 MIME 家族与标准 Office MIME 互认，并按 MIME apps 规范合并 Default/Added/Removed Associations。
- 代码逻辑改动：`detect_name()` 内置识别 `wps/wpt/et/ett/dps/dpt`；内容识别遇到系统 magic 的 `application/octet-stream` 这类泛型结果时继续回落到扩展名 MIME；ZIP/OLE 容器在 WPS/Office 扩展名明确时保留对应扩展名 MIME；`parse_desktop_app()` 保留无 `MimeType` 但有 `Exec` 的应用，后续由 `[Default Applications]` 给它补支持 MIME；`load_app_registry()` 读取桌面专用和通用 `mimeapps.list` 后，把 `[Default Applications]` 加入对应 app 的支持 MIME；只有非 desktop-specific 的通用 `mimeapps.list` 允许 `[Added Associations]`/`[Removed Associations]` 影响候选；默认应用写入时，桌面专用文件只写 `[Default Applications]`，通用 `mimeapps.list` 写 `[Added Associations]`；外部应用启动后用后台线程 wait 子进程，避免 wrapper 退出后留下 zombie；WPS Office desktop app 的 `%u/%U` 对本地文件按本地路径展开，以匹配远端 GIO/WPS wrapper 可弹窗路径。
- 影响的使用场景：WPS Writer 打开 DOC/DOCX/WPS/WPT，WPS Spreadsheets 打开 XLS/XLSX/ET/ETT，WPS Presentation 打开 PPT/PPTX/DPS/DPT，打开方式列表中同家族 MIME 的候选匹配，以及通过其它文件管理器写入 `mimeapps.list` 的候选/默认应用。
- 不影响的使用场景：Exec 字段码展开、文本编辑器兜底、通配 MIME 匹配、系统 `mimeapps.list` 写入位置和远程 VM 状态。

## 4. 修复执行计划

| 步骤 | 修改内容 | 验证方式 | 状态 |
|------|----------|----------|------|
| 1 | 补 WPS 原生扩展名和类型标签 | `cargo test -p filesystem-mime` | 已完成 |
| 2 | 将一对一 alias 改为 Writer/表格/演示 MIME 家族互认 | `cargo test -p filesystem-gui wps_office` | 已完成 |
| 3 | 补 `*.docx` 匹配只声明 `*.wps` MIME 的 WPS Writer、`*.wps` 精确匹配优先等回归测试 | `cargo test -p filesystem-gui wps_office` | 已完成 |
| 4 | 读取 `mimeapps.list` 的 Default/Added/Removed Associations，保留无 MimeType 默认应用，并补关联优先级回归测试 | `cargo test -p filesystem-gui mimeapps_`、`cargo test -p filesystem-gui higher_priority` | 已完成 |
| 5 | 按 GIO 规则忽略 desktop-specific mimeapps 的 Added/Removed，并拆分默认应用写入目标 | `cargo test -p filesystem-gui desktop_specific`、`cargo test -p filesystem-gui set_default_app` | 已完成 |
| 6 | 外部应用启动后后台 wait 子进程，避免 WPS wrapper zombie | `cargo test -p filesystem-gui` | 已完成 |
| 7 | 按远端真实 GUI 双击证据修正 WPS `%U` 展开和泛型 magic 覆盖扩展名问题 | `cargo test -p filesystem-gui build_exec_command`、`cargo test -p filesystem-gui double_click_wps_family_files_uses_wps_app`、`cargo test -p filesystem-mime binary_wps_content_keeps_extension_mime_when_magic_is_generic` | 已完成 |
| 8 | 更新本地修复记录、索引和长期开发/产品文档 | 文档审查、`git diff --check` | 已完成 |
| 9 | 执行完整相关验证 | `cargo fmt --check`、`cargo test -p filesystem-mime`、`cargo check -p filesystem-gui`、`cargo test -p filesystem-gui` | 已完成 |

## 5. 风险摘要

| 项 | 结论 |
|----|------|
| 风险矩阵 | L3；跨 `filesystem-mime` 和 `filesystem-gui`，但不改变公共接口、不引入新依赖 |
| 命令权限 | C0/C1 |
| 高风险门禁 | 是；文件类型识别和外部应用选择属于关键用户链路，按 L3 扩大验证 |
| 破坏性操作 | 否 |
| 用户已有修改 | 否；编码前工作区干净 |
| 用户确认 | 无 |
| 副作用/风险 | MIME apps 关联合并会让通过系统/其它文件管理器添加的应用进入候选列表；保留无 MimeType 的默认应用会让 `wps-office-prometheus.desktop` 这类 wrapper 能被默认应用使用；WPS MIME 家族互认是兼容性规则；若某发行版使用其它 WPS MIME 名称，仍需后续补充 |

## 6. 验证

- 验证环境：本地 Linux workspace。
- 回归/相关验证：`cargo fmt --check`、`cargo test -p filesystem-mime`、`cargo test -p filesystem-gui wps_office`、`cargo test -p filesystem-gui mimeapps_`、`cargo test -p filesystem-gui desktop_specific`、`cargo test -p filesystem-gui set_default_app`、`cargo test -p filesystem-gui user_association`、`cargo test -p filesystem-gui build_exec_command`、`cargo test -p filesystem-gui double_click_wps_family_files_uses_wps_app`、`cargo test -p filesystem-mime binary_wps_content_keeps_extension_mime_when_magic_is_generic`、`cargo check -p filesystem-gui`、`cargo test -p filesystem-gui`。
- 结果：通过；`filesystem-mime` 全量 12 个单元测试通过，GUI crate 全量 89 个单元测试通过。
- 未执行验证项：尚未把包含最终补丁的新二进制部署到 `192.168.122.64` 做 GUI 实测；本机 release 因 glibc 版本高于远端 Ubuntu 18.04 不能直接运行。已在远端对用户当前二进制完成真实 GUI 双击 + `strace` 取证，确认失败差异在 WPS `%U` 的 `file://` 参数展开。
- 残余风险：真实虚拟机上 WPS `.desktop` 的 MIME 声明可能存在更多私有 MIME 名称；当前修复覆盖常见 Writer/表格/演示 WPS 原生和标准 Office 格式。

## 7. 修复总结

- 最终结果：已完成。`*.wps/*.wpt/*.et/*.ett/*.dps/*.dpt` 可被内置识别为 WPS 原生格式；远端 WPS 自有二进制头的 `*.docx/*.wps` 不再被泛型 `application/octet-stream` 覆盖扩展名 MIME；WPS Office `%u/%U` 会按本地路径传给 WPS wrapper；打开方式候选会合并 `.desktop` `MimeType`、`mimeapps.list` Default、通用 `mimeapps.list` Added/Removed Associations 和 WPS/标准 Office MIME 家族兼容规则；已修正远端暴露的无 MimeType 默认应用、desktop-specific associations、WPS URI 启动路径和 zombie 子进程问题。
- 计划偏差：第一轮一对一 alias 不足；第二轮补 MIME 家族但仍缺真实远端证据；第三轮使用远端取证后确认失败点在默认应用 wrapper、MIME apps 规范细节和子进程回收。
- 后续建议：完整验证通过后，可将新二进制复制到远端非覆盖路径进行 GUI 实测，再决定是否替换 `/home/dingjing/filesystem-gui`。
