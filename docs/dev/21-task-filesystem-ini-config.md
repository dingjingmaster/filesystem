# filesystem.ini 启动配置轻量任务记录

> 文档元数据
> - 文件编号：21
> - 文档类型：task
> - 文件路径：docs/dev/21-task-filesystem-ini-config.md
> - 文档版本：v1.0.0
> - 最后更新：2026-06-29
> - 需求级别：L2
> - 关联需求：启动时读取同级 `filesystem.ini`，支持覆盖左侧标题栏名称和右键菜单终端命令

## 1. 目标

- 要解决的问题：文件管理器的左侧标题和“在终端打开”命令当前写死在代码里，无法通过部署目录旁的配置文件调整。
- 成功标准：
  - 启动时检查可执行文件同级目录是否存在 `filesystem.ini`。
  - 不存在配置文件时保持现有默认行为。
  - 配置文件存在且包含非空 `name=xxx` 时，左侧标题栏显示该名称。
  - 配置文件存在且包含非空 `terminal=xxx` 时，“在终端打开”使用该终端路径启动。
  - 配置文件存在但不包含上述选项时，不影响默认行为。

## 2. 背景与边界

- 背景：现有 GUI 应用名常量在 `config.rs`，终端启动逻辑在 `tasks.rs` 中按 `terminator`、`mate-terminal`、`gnome-terminal` 顺序查找 PATH。
- 包含：GUI 启动配置加载、INI 简单解析、左侧标题栏显示、右键终端打开命令选择、单元测试、产品/开发概览和本地索引更新。
- 不包含：系统级配置目录、热加载、配置写回、默认应用配置、终端参数模板、图形自动化测试。
- 关键假设：`filesystem.ini` 的“同级目录”指可执行文件所在目录；`terminal` 配置为可直接执行的终端程序路径。
- 非目标：不改变 Linux `application_id`、窗口 icon、文件打开方式解析和文件操作逻辑。
- 最大修改范围：`crates/filesystem-gui/src/config.rs`、`crates/filesystem-gui/src/app.rs`、`crates/filesystem-gui/src/tasks.rs`、产品/开发概览、本任务文档和 `docs/dev/README.md`。
- 禁止触碰范围：不修改 `AGENTS.project.md`；不执行真实用户目录删除验证；不引入新依赖。

## 3. 风险门禁

| 项 | 结论 |
|----|------|
| 风险矩阵 | L2，新增局部配置格式和单 GUI crate 行为，不改变架构或公共 crate API |
| 高风险开发门禁 | 是，涉及配置格式；按 L2 执行并补充自动化解析验证 |
| 破坏性操作 | 否 |
| 用户已有修改 | 是；编码前发现 docs/dev/README.md、docs/overview-product.md、docs/overview-product-dev.md 和 docs/dev/20-task-click-range-selection.md 已有文档改动，本次只在其基础上追加相关内容 |
| 底层/系统风险 | 否；不涉及 unsafe、内存生命周期、并发、ABI/API 或构建链接 |
| 命令权限 | C0/C1；不需要 C2/C3 |
| 用户确认事项 | 无 |
| 回滚/止损方式 | 回退本次 GUI 和文档变更即可恢复硬编码默认行为 |

## 4. 方案

- 推荐方案：在 `config.rs` 新增运行时配置结构和无依赖 INI 解析；`FileManager::new` 启动时加载一次并存入状态；侧栏标题从运行时配置读取；终端打开任务优先使用配置的终端路径，未配置时保留原终端 fallback 列表。
- 取舍理由：需求只需要两个简单键值，不引入 INI 依赖；配置只在启动读取，避免增加热加载状态复杂度。
- 风险与应对：配置文件读取失败或配置项为空时保持默认行为；解析测试覆盖注释、section、大小写和空值忽略。

## 5. 执行计划

| 步骤 | 修改内容 | 验证方式 | 状态 |
|------|----------|----------|------|
| 1 | 新增运行时配置读取和 INI 解析，覆盖左侧标题栏名称 | `cargo test -p filesystem-gui config`、`cargo check -p filesystem-gui` | 完成 |
| 2 | 让终端打开逻辑优先使用配置的终端路径，未配置时保留默认 fallback | `cargo test -p filesystem-gui`、`cargo check -p filesystem-gui` | 完成 |
| 3 | 更新产品概览、开发概览和本地 dev 索引 | 人工检查文档一致性、`git diff --check` | 完成 |
| 4 | 运行格式、GUI 测试和编译检查命令 | `cargo fmt --check`、`cargo test -p filesystem-gui`、`cargo check -p filesystem-gui`、`git diff --check` | 完成 |

## 6. 实现记录

- 修改文件：
  - `crates/filesystem-gui/src/config.rs`
  - `crates/filesystem-gui/src/app.rs`
  - `crates/filesystem-gui/src/tasks.rs`
  - `docs/overview-product.md`
  - `docs/overview-product-dev.md`
  - `docs/dev/README.md`
  - `docs/dev/21-task-filesystem-ini-config.md`
- 关键决策：
  - `filesystem.ini` 通过 `std::env::current_exe()` 定位到可执行文件同级目录，配置只在启动读取一次。
  - INI 解析不新增依赖，只识别非空 `name` 和 `terminal`，忽略空行、注释、section 行和未知键；键名大小写不敏感，重复键以最后一次为准。
  - `name` 只覆盖左侧标题栏显示，窗口标题前缀和 Linux `application_id` 继续使用 `File`。
  - 配置了 `terminal` 时用 `Command::new(path).current_dir(cwd).spawn()` 直接执行，不经 shell；未配置时保留 `terminator`、`mate-terminal`、`gnome-terminal` fallback。
- 计划偏差：无。
- 安全门禁执行结果：未执行破坏性操作；未执行 C2/C3 命令；未修改 `AGENTS.project.md`；本轮未主动执行提交命令；开始前已有的 `docs/dev/20-task-click-range-selection.md` 等文档改动未被回退或覆盖。

## 7. 验证记录

- 验证环境：本地 Linux workspace。
- 系统信息（OS/内核/架构/编译器/运行时，按需）：Linux 7.0.11-gentoo-dingjing x86_64 GNU/Linux；rustc 1.93.1 (01f6ddf75 2026-02-11) (gentoo)。

| 验证项 | 命令/步骤 | 结果 | 备注 |
|--------|-----------|------|------|
| 配置解析测试 | `cargo test -p filesystem-gui config` | 通过 | 3 个配置解析测试通过，覆盖默认值、`name`、`terminal`、大小写键和引号值 |
| GUI 单元测试 | `cargo test -p filesystem-gui` | 通过 | 60 个测试通过 |
| GUI 编译检查 | `cargo check -p filesystem-gui` | 通过 | GUI crate 编译检查通过 |
| 格式检查 | `cargo fmt --check` | 通过 | Rust 格式符合项目设置 |
| diff 空白检查 | `git diff --check` | 通过 | 无尾随空白或 diff whitespace 问题 |

- 未执行验证项：未启动真实 X11/Wayland GUI 做 `filesystem.ini` 标题覆盖和外部终端打开 smoke test。
- 残余风险：配置终端的真实启动效果仍取决于用户填写的终端程序是否可执行以及是否接受从进程当前目录继承工作目录；自动化测试未真实拉起外部终端。

## 8. 总结

- 最终结果：完成启动时读取可执行文件同级 `filesystem.ini`；非空 `name` 覆盖左侧标题栏显示，非空 `terminal` 覆盖右键“在终端打开”的终端路径；未配置时保持原默认标题和终端 fallback。
- 遗留风险：真实图形会话和真实终端程序启动需要人工 smoke test。
- 后续建议：如果后续需要支持终端参数模板或系统级配置目录，应单独设计配置语法和兼容策略。

## 9. L2 审视

| 角色 | 结论 |
|------|------|
| 安全审查员 | 通过。未执行破坏性操作或 C2/C3 命令；本次是需求实现，不是 bug 修复；变更不涉及 unsafe、指针、并发、ABI/API 或构建链接；配置格式新增已记录，解析失败和空值保持默认行为；外部终端启动不经 shell。 |
| 高级产品 | 通过。`name` 和 `terminal` 两个配置项覆盖用户要求的左侧标题栏和右键终端打开行为；未配置时保持现有默认体验。 |
| 高级架构师 | 通过。没有新增依赖或跨 crate 公共 API；配置读取集中在 GUI `config.rs`，终端启动仍在 `tasks.rs`，模块边界清晰。 |
| 高级工程师 | 通过。实现保持局部；`cargo test -p filesystem-gui config` 覆盖配置解析，`cargo test -p filesystem-gui`、`cargo check -p filesystem-gui`、`cargo fmt --check` 和 `git diff --check` 均通过。 |
