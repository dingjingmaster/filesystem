# Desktop Exec 字段码展开修复记录

> 文档元数据
> - 文件编号：17
> - 文档类型：fix
> - 文件路径：docs/dev/17-fix-desktop-exec-field-codes.md
> - 完成日期：2026-06-26
> - 需求级别：L2

## 1. 问题定义

- 问题现象：在 UOS/Deepin 环境运行 `filesystem-gui` 后，打开文件时可能出现类似无法解析 `%2` 中符号的错误。
- 预期行为：按 Desktop Entry 规范展开 `.desktop` `Exec` 字段码后再启动外部应用，不应把未知或废弃 `%x` 字段码原样传给应用。
- 影响范围：普通文件双击、文件菜单“用 xxx 打开”、打开方式弹窗确认打开。
- 不包含：改用 `xdg-open`、DBus、portal 或桌面 MIME 服务；写入系统默认应用配置。

## 2. 证据与根因

- 复现方式：远程登录 UOS/Deepin 测试环境确认运行进程、默认应用和 `.desktop` `Exec` 数据；直接启动浏览器/播放器的 `%U` 样例未稳定复现原始弹窗；通过单元测试覆盖 `--token=%2`、`%d` 等字段码输入。
- 证据等级：原始远端问题为 E1，缺少稳定 GUI 复现和完整错误日志；字段码展开缺陷已通过 E3 单元测试覆盖。
- 关键日志/堆栈/输入：远端 `filesystem-gui` 进程使用 `DISPLAY=:0`、`XDG_CURRENT_DESKTOP=Deepin`；默认应用包含 `deepin-editor %F`、`deepin-movie %U`、`org.deepin.browser %U` 等标准入口；代码构造 Exec 时只替换少数字段码。
- 根因：`build_exec_command` 原实现用字符串替换处理 `%c/%f/%F/%u/%U`，只跳过完整 token `%i/%k`；字段码嵌入参数内部或遇到废弃/未知字段码时，可能保留字面 `%x` 并传给外部应用。
- 相关代码路径：`crates/filesystem-gui/src/apps.rs`

## 3. 修复方案

- 最小修复点：把 Exec token 展开改为逐字符解析。
- 代码逻辑改动：`%%` 输出字面 `%`；`%c` 输出应用名；`%f/%F` 输出本地路径；`%u/%U` 输出 `file://` URI；`%i/%k`、废弃 `%d/%D/%n/%N/%v/%m` 和未知字段码移除；空 token 不传给外部应用。
- 影响的使用场景：更兼容发行版或第三方 `.desktop` 中带废弃/未知字段码的打开命令。
- 不影响的使用场景：标准 `%f/%F/%u/%U` 打开方式、无字段码时追加本地路径、带空格路径的 file URI 编码。

## 4. 修复执行计划

| 步骤 | 修改内容 | 验证方式 | 状态 |
|------|----------|----------|------|
| 1 | 远端确认运行环境、默认应用和 Exec 字段 | SSH 只读检查、手动样例启动 | 已完成 |
| 2 | 修正 Exec 字段码展开逻辑 | `cargo test -p filesystem-gui apps::tests` | 已完成 |
| 3 | 增加未知字段码和 `%%` 回归测试 | `cargo test -p filesystem-gui apps::tests` | 已完成 |
| 4 | 完整相关验证 | `cargo fmt --check`、`cargo test -p filesystem-gui`、`git diff --check` | 已完成 |

## 5. 风险摘要

| 项 | 结论 |
|----|------|
| 风险矩阵 | L2 |
| 命令权限 | C0/C1；远端测试仅创建 `/tmp` 样例文件并启动用户要求验证的 GUI 应用 |
| 高风险门禁 | 是，影响外部应用启动参数 |
| 破坏性操作 | 否 |
| 用户已有修改 | 是，基于当前未提交渲染和文件操作改动继续增量修改，未回退无关内容 |
| 用户确认 | 用户提供远端测试环境并要求尝试 |
| 副作用/风险 | 原始 UOS 弹窗未稳定复现，修复覆盖的是明确存在的 Exec 字段码兼容缺陷 |

## 6. 验证

- 验证环境：本地 Linux workspace；远端 UOS/Deepin 测试环境用于只读调查和样例启动。
- 回归/相关验证：`cargo test -p filesystem-gui apps::tests`、`cargo fmt --check`、`cargo test -p filesystem-gui`、`git diff --check`。
- 结果：均通过；`filesystem-gui` 44 个单元测试通过。
- 未执行验证项：未在部署新二进制后的 UOS 图形会话中进行真实 GUI 双击打开复验。
- 残余风险：如果原始弹窗来自某个应用不兼容 `file://` URI 而不是 Exec 字段码残留，仍需基于具体文件类型和完整错误日志继续定位。

## 7. 修复总结

- 最终结果：已完成，`.desktop` Exec 字段码按逐字符方式展开，未知或废弃 `%x` 字段码不再原样传给外部应用。
- 计划偏差：远端无法通过命令行稳定复现用户看到的弹窗，改为修复代码中可明确导致 `%x` 外泄的兼容问题。
- 后续建议：在能部署新二进制到 UOS 环境后，用文本、HTML、图片、PDF、音视频样例分别做真实 GUI 打开验证。
