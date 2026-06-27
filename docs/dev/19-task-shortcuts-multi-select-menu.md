# 快捷键与多选右键菜单轻量任务记录

> 文档元数据
> - 文件编号：19
> - 文档类型：task
> - 文件路径：docs/dev/19-task-shortcuts-multi-select-menu.md
> - 文档版本：v1.0.0
> - 最后更新：2026-06-26
> - 需求级别：L2
> - 关联需求：为选中文件/文件夹新增复制、剪切、删除快捷键，为空选择新增粘贴快捷键，并为多选新增复制/剪切/删除右键菜单

## 1. 目标

- 要解决的问题：现有 GUI 已有单文件/单文件夹右键复制、剪切、删除和空白菜单粘贴，但缺少键盘快捷方式；多选后右键也没有直接面向选中集合的复制、剪切和删除菜单。
- 成功标准：
  - 选中一个或多个文件/文件夹后，`Ctrl+C` 写入内部复制剪贴板。
  - 没有选中任何文件/文件夹时，`Ctrl+V` 执行与空白右键菜单“粘贴”一致的粘贴逻辑。
  - 选中一个或多个文件/文件夹后，`Ctrl+X` 写入内部剪切剪贴板，剪切条目继续变淡显示。
  - 选中一个或多个文件/文件夹后，`Del` 弹出删除确认；用户确认后才执行删除。
  - 选中多个文件/文件夹后右键弹出只包含“复制”“剪切”“删除”的多选菜单，并作用于当前选中集合。

## 2. 背景与边界

- 背景：历史任务已实现 `selected_paths` 多选状态、单文件/单文件夹菜单、内部 `ClipboardState`、粘贴进度和删除确认弹窗。
- 包含：GUI 消息、键盘事件订阅、多选右键菜单状态、选中集合复制/剪切/删除、删除确认弹窗支持多路径、任务文档与长期产品事实更新。
- 不包含：系统剪贴板专用 MIME 写入、回收站、撤销删除、批量属性/重命名、跨窗口剪贴板同步。
- 关键假设：快捷键只在键盘事件未被输入框或重命名编辑器捕获时触发；多选右键菜单作用于当前选中集合。
- 非目标：不改变 core 的复制、剪切、删除语义。
- 最大修改范围：`crates/filesystem-gui`、`docs/overview-product.md`、`docs/overview-product-dev.md`、本任务文档和 `docs/dev/README.md`。
- 禁止触碰范围：不修改 `AGENTS.project.md`；不执行真实用户目录删除验证；不做无关重构。

## 3. 风险门禁

| 项 | 结论 |
|----|------|
| 风险矩阵 | L2，单 GUI 模块功能增强，复用既有文件操作 API |
| 高风险开发门禁 | 是，涉及文件系统写操作入口和删除触发入口；删除仍由确认弹窗保护 |
| 破坏性操作 | 代码实现本身不执行删除；运行时删除必须用户确认 |
| 用户已有修改 | 编码前 `git status --short` 为空 |
| 底层/系统风险 | 无 Rust unsafe、ABI/API、依赖或构建变更 |
| 命令权限 | C0/C1；不需要 C2/C3 |
| 用户确认事项 | 无；运行时删除由 GUI 弹窗二次确认 |
| 回滚/止损方式 | 回退本次 GUI 与文档变更即可恢复原行为 |

## 4. 方案

- 推荐方案：在 GUI 层新增快捷键订阅，将 `Ctrl+C`、`Ctrl+X`、`Ctrl+V` 和 `Del` 映射到新的选中集合/快捷粘贴消息；新增 `ContextMenuState::Selection` 渲染多选菜单；复用内部剪贴板、粘贴进度和 core 删除 API。
- 取舍理由：复用现有单项菜单和后台任务模型，避免新增 core API 或桌面服务依赖；快捷键只处理 ignored 键盘事件，降低与文本输入冲突的风险。
- 风险与应对：多路径删除可能部分成功后失败，GUI 删除任务需记录已删除路径并刷新目录；删除确认弹窗需支持多路径文案。

## 5. 执行计划

| 步骤 | 修改内容 | 验证方式 | 状态 |
|------|----------|----------|------|
| 1 | 扩展 GUI 模型、键盘订阅、多选菜单和选中集合复制/剪切/删除处理 | `cargo test -p filesystem-gui`、`cargo check -p filesystem-gui` | 完成 |
| 2 | 更新产品概览和本地 dev 索引 | 人工检查文档一致性、`git diff --check` | 完成 |
| 3 | 运行格式、GUI 测试和检查命令 | `cargo fmt --check`、`cargo test -p filesystem-gui`、`cargo check -p filesystem-gui`、`git diff --check` | 完成 |

## 6. 实现记录

- 修改文件：
  - `crates/filesystem-gui/src/model.rs`
  - `crates/filesystem-gui/src/tasks.rs`
  - `crates/filesystem-gui/src/app.rs`
  - `docs/overview-product.md`
  - `docs/overview-product-dev.md`
  - `docs/dev/README.md`
  - `docs/dev/19-task-shortcuts-multi-select-menu.md`
- 关键决策：
  - 快捷键通过 `event::listen_with` 订阅 ignored 键盘事件，避免抢占地址栏和重命名编辑器等输入控件。
  - `Ctrl+C`、`Ctrl+X` 和多选菜单复用内部 `ClipboardState`；空选择 `Ctrl+V` 复用现有粘贴路径和进度逻辑。
  - 删除确认从单路径扩展为路径集合，确认后由 GUI 后台任务逐个调用既有 core `delete_entry`；批量删除先处理更深路径，降低搜索结果中父子路径同时选中时的失败概率。
  - 多选右键新增 `ContextMenuState::Selection`，菜单只包含用户要求的“复制”“剪切”“删除”。
- 计划偏差：补充更新了 `docs/overview-product-dev.md`，因为键盘订阅和多选删除结果成为 GUI 长期控制流事实。
- 安全门禁执行结果：未执行真实用户目录删除；删除入口仍必须经 GUI 确认弹窗；未执行 C2/C3 命令；未修改 `AGENTS.project.md`；未提交 git。

## 7. 验证记录

- 验证环境：本地 Linux workspace。
- 系统信息（OS/内核/架构/编译器/运行时，按需）：Linux 7.0.11-gentoo-dingjing x86_64 GNU/Linux；rustc 1.93.1 (01f6ddf75 2026-02-11) (gentoo)。

| 验证项 | 命令/步骤 | 结果 | 备注 |
|--------|-----------|------|------|
| GUI 单元测试 | `cargo test -p filesystem-gui` | 通过 | 53 个测试通过；新增覆盖快捷键映射、captured/repeat 忽略、选中集合复制/剪切、空选择粘贴约束、删除确认和删除完成状态清理 |
| GUI 编译检查 | `cargo check -p filesystem-gui` | 通过 | 验证 iced 订阅、消息类型和后台任务类型 |
| 格式检查 | `cargo fmt --check` | 通过 | 已先执行 `cargo fmt` |
| diff 空白检查 | `git diff --check` | 通过 | 无尾随空白 |

- 未执行验证项：未启动真实 X11/Wayland GUI 做人工快捷键和右键菜单 smoke test；未对真实用户目录执行删除。
- 残余风险：不同窗口焦点/输入法环境下键盘事件 captured 状态仍需真实图形会话确认；删除功能仍无回收站。

## 8. 总结

- 最终结果：完成四个快捷键和多选右键菜单；多选复制/剪切/删除作用于当前选中路径集合，删除前弹出确认，确认后后台删除并刷新目录。
- 遗留风险：真实 GUI 交互需人工 smoke test；删除无回收站。
- 后续建议：如果后续要支持撤销或回收站，需要单独设计数据安全边界和验证策略。
