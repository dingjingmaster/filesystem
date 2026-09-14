# 环境变量隐藏终端菜单入口轻量任务记录

> 文档元数据
> - 文件编号：45
> - 文档类型：task
> - 文件路径：docs/dev/45-task-hide-terminal-menu-entry.md
> - 文档版本：v1.0.0
> - 最后更新：2026-09-14
> - 需求级别：L2
> - 关联需求：增加环境变量，开启后右键菜单不显示终端入口且不保留多余分割线

## 1. 目标

- 要解决的问题：文件管理器右键菜单固定显示“在终端打开”，缺少按启动环境隐藏该入口的能力。
- 成功标准：设置 `FILESYSTEM_HIDE_TERMINAL_ENTRY=1/true/yes/on` 时，空白处和文件夹右键菜单不显示“在终端打开”，菜单高度和分割线数量同步减少，不出现多余分割线。

## 2. 背景与边界

- 背景：空白处菜单和文件夹菜单各自构造终端入口；菜单高度用 item/separator 数量计算，隐藏入口时必须同步调整高度。
- 包含：运行时环境变量配置；空白处和文件夹右键菜单终端入口隐藏；相关高度/分割线单元测试；文档更新。
- 不包含：删除终端打开消息或任务实现；隐藏其它菜单项；新增命令行参数。
- 关键假设：环境变量名采用 `FILESYSTEM_HIDE_TERMINAL_ENTRY`，启用值沿用 `1`、`true`、`yes`、`on`。
- 非目标：不改变默认右键菜单行为。
- 最大修改范围：`crates/filesystem-gui/src/config.rs`、`crates/filesystem-gui/src/app.rs`、`docs/overview-product.md`、`docs/overview-product-dev.md`、`docs/dev/README.md`、本任务文档。
- 禁止触碰范围：文件操作、终端启动实现、外部应用启动和 root scope 路径限制逻辑。

## 3. 风险门禁

| 项 | 结论 |
|----|------|
| 风险矩阵 | L2：环境变量改变右键菜单可见行为，范围局限在 GUI 菜单 |
| 高风险开发门禁 | 是：涉及配置/启动环境行为，按 L2 执行 |
| 破坏性操作 | 否 |
| 用户已有修改 | 否：编码前 `git status --short` 为空 |
| 底层/系统风险 | 否：不涉及 Rust unsafe、并发、内存、ABI |
| 命令权限 | C0/C1：只读检查、定向文件修改、定向 cargo test |
| 用户确认事项 | 用户已确认环境变量方案，并要求隐藏后去掉多余 split 分割线 |
| 回滚/止损方式 | 回退本次相关文件即可恢复默认始终显示终端菜单入口 |

## 4. 方案

- 推荐方案：在 `RuntimeConfig` 增加 `hide_terminal_entry`，启动时读取 `FILESYSTEM_HIDE_TERMINAL_ENTRY`；菜单构造和高度计算统一引用该字段，隐藏时同时减少对应 item/separator。
- 取舍理由：复用现有运行时配置入口，不新增 CLI 框架；保留终端启动代码，降低行为改动范围。
- 风险与应对：菜单高度和渲染分支容易不同步，通过高度测试覆盖隐藏后 separator 数量。

## 5. 执行计划

| 步骤 | 修改内容 | 验证方式 | 状态 |
|------|----------|----------|------|
| 1 | 补充环境变量和菜单高度红灯测试 | `cargo test -p filesystem-gui terminal_entry -- --nocapture` | 完成 |
| 2 | 实现配置读取和菜单隐藏分支 | `cargo test -p filesystem-gui terminal_entry -- --nocapture` | 完成 |
| 3 | 更新概览文档、本地索引和任务总结 | `git diff --check` | 完成 |

## 6. 实现记录

- 修改文件：`crates/filesystem-gui/src/config.rs`、`crates/filesystem-gui/src/app.rs`、`docs/overview-product.md`、`docs/overview-product-dev.md`、`docs/dev/README.md`、`docs/dev/45-task-hide-terminal-menu-entry.md`。
- 关键决策：`RuntimeConfig.hide_terminal_entry` 只控制菜单显示，不删除 `ContextOpenTerminal`、`FolderOpenTerminal` 和终端启动任务；`FILESYSTEM_HIDE_TERMINAL_ENTRY` 与 `FILESYSTEM_NO_CONFIG` 独立，禁用 `filesystem.ini` 时仍可隐藏终端入口。
- 计划偏差：无。
- 安全门禁执行结果：未执行破坏性操作；未改文件操作、终端启动实现、外部应用启动或 root scope 路径限制逻辑。
- L2 审视结论：产品侧满足隐藏空白处和文件夹菜单终端入口且不留下多余分割线；工程侧菜单高度和渲染分支共享同一配置字段；架构侧未引入新依赖或额外进程状态。

## 7. 验证记录

- 验证环境：本地
- 系统信息（OS/内核/架构/编译器/运行时，按需）：Linux gentoo-pc 7.1.4-gentoo-dingjing x86_64；rustc 1.96.1；cargo 1.96.1。

| 验证项 | 命令/步骤 | 结果 | 备注 |
|--------|-----------|------|------|
| 红灯测试 | `cargo test -p filesystem-gui terminal_entry -- --nocapture` | 通过 | 新增测试先因缺少 `hide_terminal_entry` 默认值和加载函数参数失败，确认覆盖待实现分支 |
| 定向回归 | `cargo test -p filesystem-gui terminal_entry -- --nocapture` | 通过 | 5 passed，覆盖环境变量、`--no-config` 组合和菜单高度 |
| GUI crate 回归 | `cargo test -p filesystem-gui` | 通过 | 158 passed |
| 格式检查 | `cargo fmt --check` | 通过 | 已运行 `cargo fmt` 修正格式 |
| 空白检查 | `git diff --check` | 通过 | 无空白错误 |

- 未执行验证项：未启动真实 X11/Wayland GUI 做右键菜单人工截图验证。
- 残余风险：自动化通过菜单高度验证分割线数量，未对实际像素渲染截图做人工确认；风险较低。

## 8. 总结

- 最终结果：完成 `FILESYSTEM_HIDE_TERMINAL_ENTRY=1/true/yes/on` 隐藏空白处和文件夹右键菜单“在终端打开”入口，并同步减少对应分割线；默认行为不变。
- 遗留风险：真实 GUI 人工 smoke test 未执行。
- 后续建议：如果后续继续增加菜单裁剪项，可考虑集中抽象菜单 item/separator 描述，进一步减少高度和渲染分支重复。
