# 禁用 filesystem.ini 启动配置轻量任务记录

> 文档元数据
> - 文件编号：43
> - 文档类型：task
> - 文件路径：docs/dev/43-task-disable-runtime-config.md
> - 文档版本：v1.0.0
> - 最后更新：2026-09-14
> - 需求级别：L2
> - 关联需求：增加命令行参数和环境变量，使文件管理器启动时可跳过 `filesystem.ini`

## 1. 目标

- 要解决的问题：当前 GUI 启动总会尝试读取可执行文件同级 `filesystem.ini`，缺少显式禁用入口。
- 成功标准：传入 `--no-config` 或设置 `FILESYSTEM_NO_CONFIG` 为启用值时，启动使用默认 `RuntimeConfig`，不读取 `filesystem.ini`。

## 2. 背景与边界

- 背景：现有 `crates/filesystem-gui/src/config.rs` 通过 `current_exe()` 定位并读取同级 `filesystem.ini`，`FileManager::new()` 在启动时调用配置加载。
- 包含：新增禁用配置读取的命令行参数和环境变量；补充配置单元测试；更新本地上下文索引和长期产品/开发事实。
- 不包含：新增完整 CLI 解析框架；改变现有 `--root <dir>` 行为；运行时动态重载配置。
- 关键假设：参数名采用 `--no-config`；环境变量采用 `FILESYSTEM_NO_CONFIG`，值为 `1`、`true`、`yes`、`on` 时启用。
- 非目标：改变 `filesystem.ini` 支持的键或文件位置。
- 最大修改范围：`crates/filesystem-gui/src/config.rs`、`crates/filesystem-gui/src/app.rs`、`docs/overview-product.md`、`docs/overview-product-dev.md`、`docs/dev/README.md`、本任务文档。
- 禁止触碰范围：无关 GUI、文件操作、MIME、root scope 逻辑。

## 3. 风险门禁

| 项 | 结论 |
|----|------|
| 风险矩阵 | L2：启动配置行为新增私有参数入口，范围局限在 GUI 配置加载 |
| 高风险开发门禁 | 是：涉及配置/启动参数行为，按 L2 执行 |
| 破坏性操作 | 否 |
| 用户已有修改 | 否：编码前 `git status --short` 为空 |
| 底层/系统风险 | 否：不涉及 Rust unsafe、并发、内存、ABI |
| 命令权限 | C0/C1：只读检查、定向文件修改、定向 cargo test |
| 用户确认事项 | 已确认参数方案和环境变量触发需求 |
| 回滚/止损方式 | 回退本次相关文件即可恢复原有始终读取配置行为 |

## 4. 方案

- 推荐方案：在配置模块增加启动配置加载入口，先检查 `--no-config` 和 `FILESYSTEM_NO_CONFIG`；命中时返回 `RuntimeConfig::default()`，否则沿用原 `load_runtime_config()`。
- 取舍理由：逻辑靠近现有配置读取代码，不引入新依赖或全局 CLI 框架；`--root` 仍由 root 模块单独解析，改动范围最小。
- 风险与应对：参数解析重复扫描启动参数，但成本极低；通过单元测试覆盖 CLI 与环境变量跳过 loader，避免误读配置文件。

## 5. 执行计划

| 步骤 | 修改内容 | 验证方式 | 状态 |
|------|----------|----------|------|
| 1 | 在配置模块补充禁用配置读取的红灯测试 | `cargo test -p filesystem-gui runtime_config_ -- --nocapture` | 完成 |
| 2 | 实现 `--no-config` 与 `FILESYSTEM_NO_CONFIG` 跳过配置加载 | `cargo test -p filesystem-gui runtime_config_ -- --nocapture` | 完成 |
| 3 | 更新产品/开发概览与索引 | 文档人工检查、`git diff --check` | 完成 |

## 6. 实现记录

- 修改文件：`crates/filesystem-gui/src/config.rs`、`docs/overview-product.md`、`docs/overview-product-dev.md`、`docs/dev/README.md`、`docs/dev/43-task-disable-runtime-config.md`。
- 关键决策：新增 `DISABLE_RUNTIME_CONFIG_ARG = "--no-config"` 和 `DISABLE_RUNTIME_CONFIG_ENV = "FILESYSTEM_NO_CONFIG"`；`load_runtime_config()` 先检查启动参数和环境变量，命中后直接返回 `RuntimeConfig::default()`，否则再读取同级 `filesystem.ini`。
- 计划偏差：无。
- 安全门禁执行结果：未执行破坏性操作；未修改无关模块；`--root` 仍由 `RootScope` 独立解析；未引入新依赖、持久化数据或并发/内存风险。
- L2 审视结论：安全侧通过测试闭包 `panic!` 证明禁用入口不会调用配置 loader；产品侧满足显式禁用配置读取需求且未改变默认行为；架构侧保持逻辑在 config 模块内；工程侧通过定向和 GUI crate 全量测试。

## 7. 验证记录

- 验证环境：本地
- 系统信息（OS/内核/架构/编译器/运行时，按需）：Linux gentoo-pc 7.1.4-gentoo-dingjing x86_64；rustc 1.96.1；cargo 1.96.1。

| 验证项 | 命令/步骤 | 结果 | 备注 |
|--------|-----------|------|------|
| 红灯测试 | `cargo test -p filesystem-gui runtime_config_ -- --nocapture` | 通过 | 新增测试先因缺少 `load_runtime_config_with_options` 失败，确认覆盖的是待实现行为 |
| 配置模块回归 | `cargo test -p filesystem-gui runtime_config_ -- --nocapture` | 通过 | 8 passed，覆盖配置解析、`--no-config` 和 `FILESYSTEM_NO_CONFIG` |
| GUI crate 回归 | `cargo test -p filesystem-gui` | 通过 | 151 passed |
| 格式检查 | `cargo fmt --check` | 通过 | 已先运行 `cargo fmt` 修正 `config.rs` 格式 |
| 空白检查 | `git diff --check` | 通过 | 无空白错误 |

- 未执行验证项：未启动真实 X11/Wayland GUI 做人工 smoke test。
- 残余风险：未覆盖真实桌面会话中通过启动器传参/注入环境变量的人工验证；单元测试已覆盖配置加载控制流。

## 8. 总结

- 最终结果：完成 `--no-config` 和 `FILESYSTEM_NO_CONFIG=1/true/yes/on` 禁用读取同级 `filesystem.ini`；未设置时保持原有读取和默认 fallback 行为。
- 遗留风险：真实 GUI 启动器传参和环境变量注入方式因桌面环境不同需人工确认。
- 后续建议：如后续继续增加启动参数，可考虑集中 CLI 解析，避免各模块重复扫描 `std::env::args_os()`。
