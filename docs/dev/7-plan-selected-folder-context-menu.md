# 选中文件夹右键菜单开发计划

> 文档元数据
> - 文件编号：7
> - 文档类型：plan
> - 文件路径：docs/dev/7-plan-selected-folder-context-menu.md
> - 文档版本：v1.0.0
> - 最后更新：2026-06-25
> - 关联需求：左键选中文件夹后，右键显示文件夹菜单，支持打开、复制、剪切、重命名、删除、终端打开和属性
> - 关联调研：docs/dev/7-research-selected-folder-context-menu.md

## 1. 目标与成功标准

- 任务目标：实现单个选中文件夹右键菜单和相关文件操作。
- 成功标准：
  - 左键选中文件夹后，在该文件夹上右键显示文件夹菜单。
  - 菜单项按需求顺序显示：打开、复制、剪切、重命名、删除、在终端打开、属性，中间带分割线。
  - 打开进入该文件夹，效果与双击一致。
  - 复制/剪切记录内部剪贴板；空白菜单粘贴时优先复制/移动内部剪贴板中的文件夹。
  - 剪切中的文件夹视觉变淡，粘贴成功或被其它复制/剪切覆盖后恢复。
  - 重命名复用现有重命名覆盖层和提交流程。
  - 删除前显示确认弹窗，确定后后台递归删除，取消不删除。
  - 在终端打开以选中文件夹作为 working directory。
  - 属性弹窗显示选中文件夹属性。
- 前置条件：基于当前模块拆分后的 GUI 代码继续修改。
- 非目标：文件右键菜单、多选批量操作、回收站、系统剪贴板 MIME 写入。

## 2. 修改边界

- 最大修改范围：
  - `crates/filesystem-core/src/lib.rs`
  - `crates/filesystem-gui/src/model.rs`
  - `crates/filesystem-gui/src/app.rs`
  - `crates/filesystem-gui/src/tasks.rs`
  - `crates/filesystem-gui/src/components.rs`
  - `crates/filesystem-gui/src/style.rs`
  - `docs/dev/README.md`
  - `docs/overview-product-dev.md`
  - 本任务 Research/Plan/Summary 文档
- 禁止触碰范围：Cargo 依赖、图标资源、无关 UI 布局和未授权用户数据。
- 影响模块/文件：
  - core 新增递归删除 API 和临时目录测试。
  - GUI model 新增菜单状态、内部剪贴板状态和删除确认状态。
  - GUI app 调整右键判断、菜单 overlay、粘贴优先级、删除确认和属性目标。
  - GUI style/components 支持剪切变淡显示和确认弹窗按钮。
- 依赖关系：GUI 调用 core 删除 API；复制/剪切继续复用 core `paste_paths`。

## 3. 安全门禁摘要

| 项 | 结论 |
|----|------|
| 风险矩阵结论 | L4 |
| 命令权限 | C0/C1 |
| 高风险开发门禁 | 是，文件系统写操作和递归删除能力 |
| 破坏性操作 | 代码能力包含删除；验证只允许在测试临时目录内删除 |
| 用户确认事项 | 无；运行时删除需要用户弹窗确认 |
| 止损/回滚方案 | 若验证失败，停止并修复本次相关 diff；必要时回退本次新增菜单和删除 API |

## 4. Bug 修复计划（按需）

- 不适用。

## 5. 执行计划

| 步骤 | 修改内容 | 验证方式 | 状态 |
|------|----------|----------|------|
| 1 | 建立 Research/Plan 文档并更新任务索引 | 人工检查文档编号和边界 | 完成 |
| 2 | 在 core 增加 `delete_entry` 并补临时目录递归删除测试 | `cargo test -p filesystem-core` 通过，21 个测试通过 | 完成 |
| 3 | 在 GUI model/app/tasks 增加文件夹菜单、内部剪贴板、删除确认和目标属性逻辑 | `cargo check -p filesystem-gui` 通过 | 完成 |
| 4 | 调整剪切变淡样式和菜单/确认弹窗 UI | `cargo test -p filesystem-gui` 通过，11 个测试通过 | 完成 |
| 5 | 更新长期开发概览和 Summary | 已更新 `docs/overview-product-dev.md` 和 Summary | 完成 |
| 6 | 完整验证 | `cargo fmt --check`、`cargo check -p filesystem-gui`、`cargo test -p filesystem-core`、`cargo test -p filesystem-gui`、`make test`、`git diff --check` 均通过 | 完成 |

## 6. 验证计划

- 基础验证：
  - `cargo fmt --check`
  - `cargo check -p filesystem-gui`
  - `cargo test -p filesystem-core`
  - `cargo test -p filesystem-gui`
  - `make test`
  - `git diff --check`
- 高风险验证：
  - 递归删除只通过 core 临时目录测试覆盖。
  - 粘贴移动/复制复用既有 `paste_paths` 测试，并通过 GUI 编译验证消息路径。
  - 删除确认弹窗事件隔离通过代码审查和 GUI 编译验证；真实点击需人工确认。
- 验证环境：本地 Linux workspace。
- 不可执行验证项：不对真实用户目录执行删除；不做真实 X11/Wayland GUI smoke test。
- 残余风险：没有回收站，用户确认后会真实递归删除目标文件夹；真实 GUI 视觉和交互仍需人工验证。

## 7. 验证结果

- `cargo fmt --check`：通过。
- `cargo check -p filesystem-gui`：通过。
- `cargo test -p filesystem-core`：通过，21 个测试通过。
- `cargo test -p filesystem-gui`：通过，11 个测试通过。
- `make test`：通过。
- `git diff --check`：通过。

## 8. 变更记录（按需）

| 日期 | 变更 | 原因 |
|------|------|------|
| 2026-06-25 | 创建计划 | 响应选中文件夹右键菜单需求 |
| 2026-06-25 | 完成实现并记录验证结果 | 收敛任务状态 |
