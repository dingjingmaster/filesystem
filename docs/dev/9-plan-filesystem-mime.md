# filesystem-mime 开发计划

> 文档元数据
> - 文件编号：9
> - 文档类型：plan
> - 文件路径：docs/dev/9-plan-filesystem-mime.md
> - 文档版本：v1.0.0
> - 最后更新：2026-06-25
> - 关联需求：实现少依赖 `filesystem-mime`，默认集成常见文件类型识别，无法识别时尝试解析系统 `/usr/share/mime` 这类 shared-mime-info 数据文件。
> - 关联调研：docs/dev/9-research-filesystem-mime.md

## 1. 目标与成功标准

- 任务目标：新增独立 MIME 识别模块，替代 GUI 内部分散的扩展名识别，并让默认打开方式、打开方式列表、文件图标和文件属性类型标签都基于同一份 MIME 结果。
- 成功标准：不调用 `file` 命令、不新增第三方依赖；常见无扩展名文本、源码、图片、音频、视频、归档、PDF、Office/ODF/EPUB 等能默认识别；不可识别时尝试解析 shared-mime-info `magic` 和 `globs2`；所有内容读取发生在后台目录装饰任务中。
- 前置条件：保留现有 `filesystem-core` 扫描结果和 iced 后台任务模型。
- 非目标：完整实现 shared-mime-info 的全部 magic 嵌套/掩码语义；写入默认应用或系统 MIME 数据库。

## 2. 修改边界

- 最大修改范围：workspace 配置、新增 `filesystem-mime` crate、GUI 的 model/icons/apps/app 集成、相关文档和测试。
- 禁止触碰范围：不改系统 MIME 数据文件，不改桌面服务配置，不引入外部命令调用。
- 影响模块/文件：`Cargo.toml`、`Cargo.lock`、`crates/filesystem-mime`、`crates/filesystem-gui/src/{model,icons,apps,app}.rs`、总览文档和 9 号任务文档。
- 依赖关系：`filesystem-gui` 新增本地 path dependency `filesystem-mime`；`filesystem-mime` 无第三方依赖。

## 3. 安全门禁摘要

| 项 | 结论 |
|----|------|
| 风险矩阵结论 | L3 |
| 命令权限 | C0/C1 |
| 高风险开发门禁 | 是，文件系统读取、共享数据结构和 GUI 后台任务边界变化 |
| 破坏性操作 | 否 |
| 用户确认事项 | 无 |
| 止损/回滚方案 | 回退新增 crate、GUI MIME 接入和文档即可恢复旧扩展名映射 |

## 4. Bug 修复计划（按需）

- 复现方式：不适用。
- 证据等级：不适用。
- 根因假设：不适用。
- 最小修复点：不适用。
- 回归/相关验证：不适用。

## 5. 执行计划

| 步骤 | 修改内容 | 验证方式 | 状态 |
|------|----------|----------|------|
| 1 | 新增 `filesystem-mime` crate，提供 `MimeInfo`、`MimeSource`、`detect_path`、`detect_name`、`detect_bytes` | `cargo test -p filesystem-mime` | 已完成 |
| 2 | 内置常见名称、扩展名、内容签名、结构化文本、zip/OLE 文档识别 | `cargo test -p filesystem-mime` | 已完成 |
| 3 | 解析 shared-mime-info `globs2` 和保守解析 `MIME-Magic` 独立规则 | `cargo test -p filesystem-mime` | 已完成 |
| 4 | GUI 后台装饰条目时缓存 MIME，并让图标、打开方式、默认打开和属性标签使用缓存 MIME | `cargo check -p filesystem-gui`、`cargo test -p filesystem-gui` | 已完成 |
| 5 | 更新任务文档、产品概览和开发概览 | 人工阅读、`git diff --check` | 进行中 |

## 6. 验证计划

- 基础验证：`cargo fmt --check`、`cargo test -p filesystem-mime`、`cargo check -p filesystem-gui`、`cargo test -p filesystem-gui`、`cargo test -p filesystem-core`、`make test`、`git diff --check`。
- 高风险验证（按需）：确认 UI 线程不直接读取文件内容；确认系统 MIME fallback 只读本地数据文件；确认外部应用启动仍直接 argv 调用，不走 shell 或桌面服务。
- 验证环境：本地 Linux workspace。
- 不可执行验证项：未在真实图形会话中人工打开大量不同格式样本验证图标和默认应用选择。
- 残余风险：shared-mime-info magic 当前只支持独立顶层规则，带子规则、复杂掩码或更复杂条件的系统规则会被保守跳过。

## 7. 变更记录（按需）

| 日期 | 变更 | 原因 |
|------|------|------|
| 2026-06-25 | 将系统 fallback 从仅 `globs2` 扩展为保守解析 `MIME-Magic` 独立规则 | 更贴近用户要求的 `/usr/share/mime` 解析能力 |
