# 文本文件打开兜底修复记录

> 文档元数据
> - 文件编号：11
> - 文档类型：fix
> - 文件路径：docs/dev/11-fix-text-editor-fallback.md
> - 完成日期：2026-06-25
> - 需求级别：L2

## 1. 问题定义

- 问题现象：`.desktop`、`.c`、`.cpp`、`.md` 等文本类文件可能无法用合适的文本编辑器打开，或误选需要终端的编辑器导致 GUI 下看起来没有打开。
- 预期行为：文本类文件优先匹配精确 MIME 应用；没有精确应用时可退到 `text/plain` 默认应用或 `TextEditor` 分类应用；未知二进制不能被文本编辑器兜底打开。
- 影响范围：普通文件双击、文件菜单“用 xxx 打开”、打开方式列表。
- 不包含：为 `Terminal=true` 应用自动创建终端窗口；修改系统默认应用配置。

## 2. 证据与根因

- 复现方式：代码阅读和本机 `.desktop` 数据验证。`vim.desktop` 声明 `Terminal=true` 且支持 C/C++ MIME；`application/x-desktop` 是文本配置但不属于 `text/*`；部分 `MimeType=` 项有前导空格。
- 证据等级：E3，已补应用匹配、解析和 URI field code 单元测试。
- 关键日志/堆栈/输入：不适用。
- 根因：应用匹配只对 `text/*` 回退 `text/plain`；未把 `application/x-desktop`、`application/json` 等文本型 application MIME 纳入文本编辑器 fallback；未解析 `Categories=TextEditor`；未过滤需要终端的 app；`MimeType=` 项没有 trim；`%u/%U` 没按 file URI 替换。
- 相关代码路径：`crates/filesystem-gui/src/apps.rs`、`crates/filesystem-mime/src/lib.rs`

## 3. 修复方案

- 最小修复点：扩展文本可编辑 MIME 判定；解析 `TextEditor` 分类和 `Terminal` 标记；过滤 `Terminal=true` 应用；trim `MimeType=` 项；修正 `%u/%U` 为 file URI；把 `.desktop` 内置识别为 `application/x-desktop`。
- 代码逻辑改动：精确 MIME 分数最高，`text/plain` fallback 次之，`TextEditor` 分类 fallback 只对文本类 MIME 生效；`application/octet-stream` 不使用文本编辑器兜底。
- 影响的使用场景：`.desktop`、`.c`、`.cpp`、`.md`、JSON/XML/SVG 等文本类文件更容易打开到 GUI 文本编辑器。
- 不影响的使用场景：Office、图片、音视频、压缩包和未知二进制文件打开规则。

## 4. 修复执行计划

| 步骤 | 修改内容 | 验证方式 | 状态 |
|------|----------|----------|------|
| 1 | 扩展文本类 MIME fallback 和 `TextEditor` 分类匹配 | `cargo test -p filesystem-gui apps::tests` | 已完成 |
| 2 | 过滤 `Terminal=true` 应用，trim `MimeType` 项 | `cargo test -p filesystem-gui apps::tests` | 已完成 |
| 3 | `%u/%U` 替换为 file URI | `cargo test -p filesystem-gui apps::tests` | 已完成 |
| 4 | `.desktop` 内置 MIME 识别 | `cargo test -p filesystem-mime` | 已完成 |
| 5 | 完整验证 | `make test`、`git diff --check` | 已完成 |

## 5. 风险摘要

| 项 | 结论 |
|----|------|
| 风险矩阵 | L2 |
| 命令权限 | C0/C1 |
| 高风险门禁 | 是，影响默认打开方式选择 |
| 破坏性操作 | 否 |
| 用户已有修改 | 是，基于当前未提交 MIME 功能继续增量修改，未回退无关内容 |
| 用户确认 | 无 |
| 副作用/风险 | `Terminal=true` 应用暂不出现在打开候选中，后续如需支持应通过终端启动封装 |

## 6. 验证

- 验证环境：本地 Linux workspace。
- 回归/相关验证：`cargo fmt --check`、`cargo test -p filesystem-gui apps::tests`、`cargo test -p filesystem-mime`、`cargo check -p filesystem-gui`、`cargo test -p filesystem-gui`、`make test`、`git diff --check`。
- 结果：均通过。`make test` 覆盖 core 23 个、GUI 28 个、MIME 10 个单元测试。
- 未执行验证项：未在真实 GUI 会话中双击 `.desktop/.c/.cpp/.md` 样本。
- 残余风险：不同发行版编辑器 `.desktop` 声明差异仍需人工样本验证。

## 7. 修复总结

- 最终结果：已完成，文本类文件可以退到 `text/plain` 或 `TextEditor` 分类应用，未知二进制不会走文本编辑器兜底。
- 计划偏差：无。
- 后续建议：如用户希望 Vim 等终端应用也可用，后续按终端启动策略显式支持 `Terminal=true`。
