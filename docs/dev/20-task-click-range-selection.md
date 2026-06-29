# Ctrl/Shift 点击多选轻量任务记录

> 文档元数据
> - 文件编号：20
> - 文档类型：task
> - 文件路径：docs/dev/20-task-click-range-selection.md
> - 文档版本：v1.0.0
> - 最后更新：2026-06-27
> - 需求级别：L2
> - 关联需求：支持按住 Ctrl 点击多个文件累加选择，以及按住 Shift 点击选择两个文件之间的范围

## 1. 目标

- 要解决的问题：现有 GUI 支持单击单选和空白区域框选，但不支持文件管理器常见的 Ctrl 多点选择和 Shift 范围选择。
- 成功标准：
  - 普通单击文件/文件夹仍清空旧选择并只选中当前条目。
  - 已选中一个条目后，按住 `Ctrl` 点击其它条目可以把它们加入当前选择；再次 `Ctrl` 点击已选条目可取消该条目选择。
  - 已选中一个条目后，按住 `Shift` 点击另一个条目，可以按当前显示顺序选中锚点、目标和二者之间所有条目。
  - `Ctrl+Shift` 点击范围时追加到现有选择，不破坏已有多选。
  - 图标视图、列表视图和搜索结果使用同一选择规则。

## 2. 背景与边界

- 背景：历史任务已实现 `selected_paths` 多选集合、空白区域框选、多选右键菜单和选中集合快捷键。
- 包含：GUI 选择状态、键盘修饰键事件订阅、条目点击选择逻辑、选择语义单元测试、产品/开发概览和本地索引更新。
- 不包含：键盘方向键范围选择、跨窗口选择、文件系统写操作语义变更、UI 自动化测试。
- 关键假设：Shift 范围选择以当前 `entries` 显示顺序为准；普通点击或 Ctrl 点击会更新后续 Shift 范围选择的锚点。
- 非目标：不改变双击打开、框选、多选右键菜单、复制/剪切/删除快捷键和文件操作后台任务。
- 最大修改范围：`crates/filesystem-gui`、`docs/overview-product.md`、`docs/overview-product-dev.md`、本任务文档和 `docs/dev/README.md`。
- 禁止触碰范围：不修改 `AGENTS.project.md`；不执行真实用户目录删除验证；不做无关重构。

## 3. 风险门禁

| 项 | 结论 |
|----|------|
| 风险矩阵 | L2，单 GUI 模块交互增强，不改变 core/API/数据模型 |
| 高风险开发门禁 | 否；不涉及文件系统写操作、并发、unsafe、ABI/API 或构建链接 |
| 破坏性操作 | 否 |
| 用户已有修改 | 编码前 `git status --short` 为空 |
| 底层/系统风险 | 否 |
| 命令权限 | C0/C1；不需要 C2/C3 |
| 用户确认事项 | 无 |
| 回滚/止损方式 | 回退本次 GUI 与文档变更即可恢复原选择行为 |

## 4. 方案

- 推荐方案：在 GUI 层维护当前 Ctrl/Shift 修饰键状态和选择锚点；条目点击时复用现有 `selected_paths` 集合，普通点击单选，Ctrl 点击切换单项，Shift 点击按 `entries` 显示顺序选择闭区间，Ctrl+Shift 追加范围。
- 取舍理由：不新增依赖，不改 core；范围计算基于显示顺序，能同时覆盖图标视图、列表视图和搜索结果。
- 风险与应对：修饰键状态依赖 iced 键盘事件订阅，已通过单元测试覆盖事件映射；真实 X11/Wayland 图形会话仍需人工 smoke test。

## 5. 执行计划

| 步骤 | 修改内容 | 验证方式 | 状态 |
|------|----------|----------|------|
| 1 | 扩展 GUI 选择状态和条目点击逻辑，支持 Ctrl/Shift 多选 | `cargo test -p filesystem-gui`、`cargo check -p filesystem-gui` | 完成 |
| 2 | 更新产品概览、开发概览和本地 dev 索引 | 人工检查文档一致性、`git diff --check` | 完成 |
| 3 | 运行格式、GUI 测试和编译检查命令 | `cargo fmt --check`、`cargo test -p filesystem-gui`、`cargo check -p filesystem-gui`、`git diff --check` | 完成 |

## 6. 实现记录

- 修改文件：
  - `crates/filesystem-gui/src/model.rs`
  - `crates/filesystem-gui/src/app.rs`
  - `docs/overview-product.md`
  - `docs/overview-product-dev.md`
  - `docs/dev/README.md`
  - `docs/dev/20-task-click-range-selection.md`
- 关键决策：
  - `SelectionModifiers` 只记录 Ctrl/Shift，避免 Alt/Logo 影响文件选择语义。
  - Shift 范围选择基于 `entries` 当前显示顺序，和视图布局坐标解耦。
  - Ctrl 点击会更新选择锚点；Shift 点击成功选中范围时保留原锚点，便于继续调整范围。
  - `Ctrl+Shift` 追加范围到现有选择；单独 `Shift` 用范围替换当前选择。
- 计划偏差：无。
- 安全门禁执行结果：未执行破坏性操作；未执行 C2/C3 命令；未修改 `AGENTS.project.md`；本轮未主动执行提交命令。

## 7. 验证记录

- 验证环境：本地 Linux workspace。
- 系统信息（OS/内核/架构/编译器/运行时，按需）：Linux 7.0.11-gentoo-dingjing x86_64 GNU/Linux；rustc 1.93.1 (01f6ddf75 2026-02-11) (gentoo)。

| 验证项 | 命令/步骤 | 结果 | 备注 |
|--------|-----------|------|------|
| GUI 单元测试 | `cargo test -p filesystem-gui` | 通过 | 57 个测试通过；新增覆盖 Ctrl 点击切换、Shift 范围选择、Ctrl+Shift 追加范围和修饰键事件映射 |
| GUI 编译检查 | `cargo check -p filesystem-gui` | 通过 | 验证消息类型、订阅和选择状态编译通过 |
| 格式检查 | `cargo fmt --check` | 通过 | Rust 格式符合项目设置 |
| diff 空白检查 | `git diff --check` | 通过 | 无尾随空白 |

- 未执行验证项：未启动真实 X11/Wayland GUI 做人工 Ctrl/Shift 点击 smoke test。
- 残余风险：不同窗口管理器或输入法环境下修饰键事件顺序仍需真实图形会话确认。

## 8. 总结

- 最终结果：完成 Ctrl 点击多点选择和 Shift 点击范围选择；普通点击保持单选，Ctrl 点击切换单项，Shift 点击按显示顺序选择锚点到目标之间的闭区间，Ctrl+Shift 追加范围。
- 遗留风险：真实 X11/Wayland 图形会话下的修饰键事件顺序仍需人工 smoke test。
- 后续建议：如后续需要键盘方向键范围选择，应单独设计焦点条目和锚点更新规则。

## 9. L2 审视

| 角色 | 结论 |
|------|------|
| 安全审查员 | 通过。未执行破坏性操作或 C2/C3 命令；本次是需求实现，不是 bug 修复；变更在 GUI 状态机和文档范围内，不涉及 unsafe、指针、共享数据结构、ABI/API 或配置格式；选择逻辑只修改内存中的 `selected_paths`、选择锚点和修饰键状态，错误路径不涉及资源释放。 |
| 高级产品 | 通过。普通单击、Ctrl 点击、Shift 点击和 Ctrl+Shift 点击覆盖用户要求及常见文件管理器选择语义；未改变双击打开、框选、多选右键菜单和快捷键语义。 |
| 高级架构师 | 通过。没有新增依赖或 core API；范围选择基于当前 `entries` 显示顺序，与图标/列表布局解耦，职责保持在 GUI 层。 |
| 高级工程师 | 通过。实现保持局部；`cargo test -p filesystem-gui` 覆盖修饰键事件、Ctrl 切换、Shift 范围和 Ctrl+Shift 追加；`cargo check -p filesystem-gui`、`cargo fmt --check` 和 `git diff --check` 均通过。 |
