# 空白处右键菜单自定义命令轻量任务记录

> 文档元数据
> - 文件编号：22
> - 文档类型：task
> - 文件路径：docs/dev/22-task-blank-menu-custom-commands.md
> - 文档版本：v1.0.0
> - 最后更新：2026-06-29
> - 需求级别：L2
> - 关联需求：通过 `filesystem.ini` 的 `[blank-menu.*]` section 为文件视图空白处右键菜单添加自定义命令

## 1. 目标

- 要解决的问题：空白处右键菜单当前只支持内置操作，用户希望通过同一个 `filesystem.ini` 增加自定义菜单项，并固定插入到“在终端打开”和“属性”之间。
- 成功标准：
  - 支持读取 `[blank-menu.<id>]` section。
  - 每个 section 支持 `label=xxx`、`command=xxx` 和多个 `arg=xxx`，多个 `arg` 按配置顺序传入。
  - 菜单项按配置文件 section 出现顺序显示。
  - 新增菜单项只出现在文件视图空白处右键菜单中，位置为“在终端打开”后的分割线与“属性”前的分割线之间。
  - 点击自定义菜单项后直接以 argv 启动命令，不经 shell，并支持把参数中的 `{cwd}` 替换为当前目录。
  - 未配置 `[blank-menu.*]` 时保持现有菜单布局。

## 2. 背景与边界

- 背景：现有 `filesystem.ini` 已支持顶层 `name` 和 `terminal`，运行时配置由 `crates/filesystem-gui/src/config.rs` 解析并存入 `RuntimeConfig`。
- 包含：INI section 解析扩展、空白处右键菜单动态项、后台启动自定义命令、`{cwd}` 参数替换、单元测试、产品/开发概览和本地索引更新。
- 不包含：文件夹/文件/多选右键菜单自定义项、菜单图标、子菜单、热加载、命令等待完成、脚本输出展示、shell 语法解析。
- 关键假设：`command` 是可直接执行的程序路径或当前进程可解析的命令；`arg` 不做 shell 拆分，一个 `arg=` 对应一个 argv 参数。
- 非目标：不改变内置菜单项功能和其它右键菜单布局。
- 最大修改范围：`crates/filesystem-gui/src/config.rs`、`crates/filesystem-gui/src/model.rs`、`crates/filesystem-gui/src/app.rs`、`crates/filesystem-gui/src/tasks.rs`、`configs/filesystem.ini`、产品/开发概览、本任务文档和 `docs/dev/README.md`。
- 禁止触碰范围：不修改 `AGENTS.project.md`；不执行真实用户脚本；不引入新依赖。

## 3. 风险门禁

| 项 | 结论 |
|----|------|
| 风险矩阵 | L2，扩展局部配置格式和单 GUI 菜单行为 |
| 高风险开发门禁 | 是，涉及配置格式和外部命令启动；按 L2 执行并补充解析/替换自动化验证 |
| 破坏性操作 | 否 |
| 用户已有修改 | 否；编码前 `git status --short` 为空 |
| 底层/系统风险 | 否；不涉及 unsafe、内存生命周期、并发、ABI/API 或构建链接 |
| 命令权限 | C0/C1；不需要 C2/C3 |
| 用户确认事项 | 无 |
| 回滚/止损方式 | 回退本次 GUI 和文档变更即可恢复原空白菜单行为 |

## 4. 方案

- 推荐方案：在 `RuntimeConfig` 中新增空白菜单自定义命令列表；解析 `[blank-menu.*]` section 时收集 label、command 和重复 arg；空白菜单渲染时在“在终端打开”和“属性”之间插入动态项；点击后后台 spawn 配置命令并替换 `{cwd}`。
- 取舍理由：沿用已有配置读取路径，不新增依赖；一个 `arg=` 对应一个 argv 参数，可避免 shell 注入和引号拆分歧义。
- 风险与应对：只有 label 和 command 非空的 section 才生成菜单项；外部命令启动失败时写入状态栏错误；测试覆盖 section 顺序、重复 arg 和 `{cwd}` 替换。

## 5. 执行计划

| 步骤 | 修改内容 | 验证方式 | 状态 |
|------|----------|----------|------|
| 1 | 扩展配置模型和 INI section 解析，支持 `[blank-menu.*]` | `cargo test -p filesystem-gui config` | 完成 |
| 2 | 在空白处右键菜单指定位置插入自定义项并接入点击消息 | `cargo check -p filesystem-gui`、`cargo test -p filesystem-gui` | 完成 |
| 3 | 实现自定义命令 argv 启动和 `{cwd}` 参数替换 | `cargo test -p filesystem-gui tasks`、`cargo test -p filesystem-gui` | 完成 |
| 4 | 更新产品概览、开发概览和本地 dev 索引 | 人工检查文档一致性、`git diff --check` | 完成 |
| 5 | 运行格式、GUI 测试和编译检查命令 | `cargo fmt --check`、`cargo test -p filesystem-gui`、`cargo check -p filesystem-gui`、`git diff --check` | 完成 |

## 6. 实现记录

- 修改文件：
  - `crates/filesystem-gui/src/config.rs`
  - `crates/filesystem-gui/src/model.rs`
  - `crates/filesystem-gui/src/app.rs`
  - `crates/filesystem-gui/src/tasks.rs`
  - `configs/filesystem.ini`
  - `docs/overview-product.md`
  - `docs/overview-product-dev.md`
  - `docs/dev/README.md`
  - `docs/dev/22-task-blank-menu-custom-commands.md`
- 关键决策：
  - `[blank-menu.*]` 只影响空白处右键菜单，不影响选中文件夹、选中文件或多选菜单。
  - 只有同时配置非空 `label` 和 `command` 的 section 会生成菜单项；多个 `arg=` 按配置顺序保留。
  - 自定义项按配置文件 section 出现顺序插入到“在终端打开”后的分割线和“属性”前的分割线之间。
  - 自定义命令用 `Command::new(command).args(args).current_dir(cwd).spawn()` 启动，不经 shell，不等待命令结束；`arg` 中的 `{cwd}` 做字符串替换。
  - `configs/filesystem.ini` 保留现有 window 示例，并增加一个 blank-menu 自定义脚本示例。
- 计划偏差：无。
- 安全门禁执行结果：未执行破坏性操作；未执行 C2/C3 命令；未修改 `AGENTS.project.md`；本轮未主动执行提交命令；未执行真实用户脚本。

## 7. 验证记录

- 验证环境：本地 Linux workspace。
- 系统信息（OS/内核/架构/编译器/运行时，按需）：Linux 7.0.11-gentoo-dingjing x86_64 GNU/Linux；rustc 1.93.1 (01f6ddf75 2026-02-11) (gentoo)。

| 验证项 | 命令/步骤 | 结果 | 备注 |
|--------|-----------|------|------|
| 配置解析测试 | `cargo test -p filesystem-gui config` | 通过 | 6 个配置测试通过，覆盖 blank-menu section 顺序、重复 arg、无效项过滤和既有顶层/`[window]` name/terminal |
| 任务函数测试 | `cargo test -p filesystem-gui tasks` | 通过 | 覆盖 `{cwd}` 参数替换 |
| GUI 单元测试 | `cargo test -p filesystem-gui` | 通过 | 63 个测试通过 |
| GUI 编译检查 | `cargo check -p filesystem-gui` | 通过 | GUI crate 编译检查通过 |
| 格式检查 | `cargo fmt --check` | 通过 | Rust 格式符合项目设置 |
| diff 空白检查 | `git diff --check` | 通过 | 无尾随空白或 diff whitespace 问题 |

- 未执行验证项：未启动真实 X11/Wayland GUI 做空白菜单渲染和真实外部命令启动 smoke test。
- 残余风险：用户配置的 `command` 是否可执行、脚本行为是否安全以及终端/脚本是否按预期处理 argv 仍取决于本机环境；自动化测试未真实拉起外部程序。

## 8. 总结

- 最终结果：完成 `[blank-menu.*]` 配置解析、空白处右键菜单动态项插入和自定义命令启动；新增项固定显示在“在终端打开”和“属性”之间，`{cwd}` 会替换为当前目录。
- 遗留风险：真实图形会话菜单显示和真实外部命令启动需要人工 smoke test。
- 后续建议：如后续需要文件夹/文件菜单自定义项、图标、子菜单、命令等待结果或 shell 模板，应单独设计配置语法和安全边界。

## 9. L2 审视

| 角色 | 结论 |
|------|------|
| 安全审查员 | 通过。未执行破坏性操作或 C2/C3 命令；本次是需求实现，不是 bug 修复；变更不涉及 unsafe、指针、并发、ABI/API 或构建链接；外部命令启动不经 shell，`arg` 不做 shell 拆分，配置无效项会被忽略。 |
| 高级产品 | 通过。自定义命令只加到空白处右键菜单，且位置符合“在终端打开”和“属性”之间的要求；未配置时保持原菜单布局。 |
| 高级架构师 | 通过。沿用 `RuntimeConfig` 和 `tasks.rs` 系统适配边界，没有新增依赖或公共 crate API；配置解析、菜单状态和命令执行职责清晰。 |
| 高级工程师 | 通过。实现保持局部；`cargo test -p filesystem-gui config` 覆盖 section 解析，`cargo test -p filesystem-gui tasks` 覆盖 `{cwd}` 替换，`cargo test -p filesystem-gui`、`cargo check -p filesystem-gui`、`cargo fmt --check` 和 `git diff --check` 均通过。 |
