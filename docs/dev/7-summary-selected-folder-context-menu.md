# 选中文件夹右键菜单开发结论摘要

> 文档元数据
> - 文件编号：7
> - 文档类型：summary
> - 文件路径：docs/dev/7-summary-selected-folder-context-menu.md
> - 文档版本：v1.0.0
> - 完成日期：2026-06-25
> - 关联需求：左键选中文件夹后，右键显示文件夹菜单，支持打开、复制、剪切、重命名、删除、终端打开和属性
> - 关联调研：docs/dev/7-research-selected-folder-context-menu.md
> - 关联计划：docs/dev/7-plan-selected-folder-context-menu.md

## 1. 最终结果

- 原始需求：当用户左键选中文件夹后，右键显示文件夹菜单，菜单支持打开、复制、剪切、重命名、删除、在终端打开和属性。
- 最终方案：新增单个选中文件夹菜单、GUI 内部复制/剪切状态、剪切变淡显示、删除确认弹窗、后台递归删除和目标文件夹属性/终端入口。
- 完成状态：完成。
- 需求变更：无。

## 2. 关键改动

- 修改文件：
  - `crates/filesystem-core/src/lib.rs`
  - `crates/filesystem-gui/src/model.rs`
  - `crates/filesystem-gui/src/app.rs`
  - `crates/filesystem-gui/src/tasks.rs`
  - `crates/filesystem-gui/src/components.rs`
  - `crates/filesystem-gui/src/style.rs`
  - `crates/filesystem-gui/src/utils.rs`
  - `docs/overview-product-dev.md`
  - `docs/dev/README.md`
  - `docs/dev/7-research-selected-folder-context-menu.md`
  - `docs/dev/7-plan-selected-folder-context-menu.md`
- 代码逻辑改动：
  - core 新增 `delete_entry`，目录递归删除，文件/符号链接只删除路径本身。
  - GUI 新增 `ContextMenuState` 区分空白菜单和单个选中文件夹菜单。
  - GUI 新增 `ClipboardState`，文件夹复制/剪切写入内部剪贴板；空白菜单粘贴优先使用内部剪贴板。
  - GUI 新增 `DeleteConfirm` 和删除确认 overlay，确认后后台调用删除任务。
  - 文件夹菜单支持打开、复制、剪切、重命名、删除、在终端打开、属性。
  - 剪切中的文件夹在图标视图和列表视图中使用变淡样式；粘贴成功、删除或重命名该路径后清理内部剪贴板。
- 影响的使用场景：单个选中文件夹右键操作、内部复制/剪切粘贴、文件夹删除确认、目标文件夹属性和终端打开。
- 不影响的使用场景：空白处右键菜单、系统剪贴板文本路径粘贴 fallback、现有双击打开、现有重命名提交流程。
- 计划偏差：无。

## 3. 安全门禁结果

| 项 | 结论 |
|----|------|
| 风险矩阵 | L4 |
| 命令权限 | C0/C1 |
| 高风险项 | 有，新增递归删除能力和文件系统写操作 GUI 入口 |
| 破坏性操作 | 未执行真实用户目录删除；删除测试只在临时目录内执行 |
| 用户已有修改 | 当前工作区已有模块拆分和属性页改动，本次基于现状追加，未回退 |
| 用户确认事项 | 无；运行时删除仍必须用户在确认弹窗中确认 |
| 副作用/风险 | 没有回收站，用户确认后会真实递归删除目标文件夹 |

## 4. 验证结果

- 验证环境：本地 Linux workspace。
- 执行验证：
  - `cargo fmt --check`
  - `cargo check -p filesystem-gui`
  - `cargo test -p filesystem-core`
  - `cargo test -p filesystem-gui`
  - `make test`
  - `git diff --check`
- 结果：全部通过；core 21 个测试通过，GUI 11 个测试通过，`make test` 聚合测试通过。
- 未执行验证项：未对真实用户目录执行删除；未启动真实 X11/Wayland GUI 做人工 smoke test。
- 残余风险：真实 GUI 点击路径和视觉效果需人工确认；删除功能无回收站。

## 5. Bug 修复验证（按需）

- 不适用。

## 6. 后续事项（按需）

- 后续建议：实现文件右键菜单、多选批量菜单或回收站前，需单独设计删除安全策略和验证边界。
