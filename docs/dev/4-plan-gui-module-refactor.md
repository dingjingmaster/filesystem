# GUI 模块拆分开发计划

> 文档元数据
> - 文件编号：4
> - 文档类型：plan
> - 文件路径：docs/dev/4-plan-gui-module-refactor.md
> - 文档版本：v1.0.0
> - 最后更新：2026-06-25
> - 关联需求：按模块划分拆分代码，确保功能不受影响，拆分模块要考虑使用设计模式并可持续维护
> - 关联调研：docs/dev/4-research-gui-module-refactor.md

## 1. 目标与成功标准

- 任务目标：把 `filesystem-gui` 从单文件实现拆成职责清晰的模块，同时保持现有功能、依赖和构建行为不变。
- 成功标准：
  - `main.rs` 只保留模块声明和 iced 应用入口。
  - GUI 代码按 `app`、`model`、`config`、`tasks`、`icons`、`components`、`utils`、`style` 拆分。
  - 后台任务、图标解析、组件构造、纯工具函数和样式均从 `app` 状态机中分离。
  - 现有测试通过，并保持覆盖原有纯函数/布局/权限/重命名边界。
- 前置条件：工作区无需要保护的用户未提交改动；不需要外部网络或系统权限。
- 非目标：不新增功能、不调整 UI 视觉、不修改 `filesystem-core` 行为、不改变 Cargo 依赖。

## 2. 修改边界

- 最大修改范围：
  - `crates/filesystem-gui/src/main.rs`
  - 新增 `crates/filesystem-gui/src/{app,model,config,tasks,icons,components,utils,style}.rs`
  - `docs/dev/README.md`
  - 本任务的 Research/Plan/Summary 文档
  - 如需沉淀长期架构事实，更新 `docs/overview-product-dev.md`
- 禁止触碰范围：`filesystem-core` 逻辑、Cargo feature、图标资源、Makefile、无关历史文档。
- 影响模块/文件：仅 GUI crate 内部模块边界和文档。
- 依赖关系：
  - `main` 依赖 `app` 和 `config`。
  - `app` 依赖 `model`、`tasks`、`components`、`utils`、`style`、`config`。
  - `tasks` 依赖 `filesystem-core` 和 `icons`，负责后台 I/O 任务适配。
  - `icons` 依赖 `filesystem-core::FileEntry` 和 GUI display model，负责主题图标策略。
  - `components` 依赖 `model`、`utils`、`style`、`config`，负责 iced widget 工厂。
  - `utils` 依赖 core 数据类型和少量 GUI 几何类型，保持纯函数为主。
  - `style` 只依赖 iced 样式类型和 `Message`。

## 3. 安全门禁摘要

| 项 | 结论 |
|----|------|
| 风险矩阵结论 | L4 |
| 命令权限 | C0/C1；不执行 C2/C3 |
| 高风险开发门禁 | 是，架构重构和 Rust GUI 状态边界调整 |
| 破坏性操作 | 否 |
| 用户确认事项 | 无 |
| 止损/回滚方案 | 若编译或测试无法在合理范围内恢复，停止并保留 diff 说明；可通过回退本次新增模块和 `main.rs` 改动恢复单文件结构 |

## 4. Bug 修复计划（按需）

- 不适用。

## 5. 执行计划

| 步骤 | 修改内容 | 验证方式 | 状态 |
|------|----------|----------|------|
| 1 | 建立本地 Research/Plan 文档并更新任务索引 | 人工检查文档和索引编号 | 完成 |
| 2 | 按职责拆分 GUI 源码，调整 `pub(crate)` 可见性和 imports | `cargo check -p filesystem-gui` 通过 | 完成 |
| 3 | 迁移原有 GUI 单元测试到对应模块 | `cargo test -p filesystem-gui` 通过，11 个测试通过 | 完成 |
| 4 | 更新长期开发概览中的 GUI 模块边界事实 | 已更新 `docs/overview-product-dev.md` | 完成 |
| 5 | 执行完整验证并记录结果 | `cargo fmt --check`、`cargo check -p filesystem-gui`、`cargo test -p filesystem-core`、`cargo test -p filesystem-gui`、`make test`、`git diff --check` 均通过 | 完成 |

## 6. 验证计划

- 基础验证：
  - `cargo fmt --check`
  - `cargo check -p filesystem-gui`
  - `cargo test -p filesystem-core`
  - `cargo test -p filesystem-gui`
  - `make test`
  - `git diff --check`
- 高风险验证：
  - API/消息签名：由 `cargo check` 验证模块可见性和 iced `Task<Message>` 类型一致。
  - 错误路径：由现有 core/gui 测试覆盖文件名限制、权限切换、菜单定位和布局计算。
  - 并发/UI 阻塞：人工审查所有既有 `Task::perform` 调用仍位于后台任务路径。
- 验证环境：本地 Linux workspace。
- 不可执行验证项：本次不启动真实 X11/Wayland GUI smoke test。
- 残余风险：拆分本身不改变运行逻辑，但真实图形会话仍需人工确认视觉完全一致。

## 7. 验证结果

- `cargo fmt --check`：通过。
- `cargo check -p filesystem-gui`：通过。
- `cargo test -p filesystem-core`：通过，19 个测试通过。
- `cargo test -p filesystem-gui`：通过，11 个测试通过。
- `make test`：通过。
- `git diff --check`：通过。

## 8. 变更记录（按需）

| 日期 | 变更 | 原因 |
|------|------|------|
| 2026-06-25 | 创建计划 | 响应 GUI 模块化重构需求 |
| 2026-06-25 | 完成模块拆分并记录验证结果 | 收敛任务状态 |
