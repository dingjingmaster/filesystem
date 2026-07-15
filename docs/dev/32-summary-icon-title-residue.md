# 图标视图标题换行残留轻量总结

> 文档元数据
> - 文件编号：32
> - 文档类型：summary
> - 文件路径：docs/dev/32-summary-icon-title-residue.md
> - 完成日期：2026-07-14
> - 需求级别：L1

## 1. 修改目标

- 原始需求：分析并修复图标视图切换目录时的显示残留；用户补充定位为标题超过一行时出现残留，单行标题不出现残留。
- 成功标准：保留图标视图标题多行显示能力，同时让 tile 高度和标题区域容纳两行标题，并让普通 tile 绘制不依赖透明背景透出父层，降低目录切换时旧多行标题像素残留风险。

## 2. 修改结果

- 修改文件：`crates/filesystem-gui/src/app.rs`、`crates/filesystem-gui/src/style.rs`、`docs/dev/README.md`、`docs/dev/32-summary-icon-title-residue.md`。
- 行为变化：图标 tile 高度从 128 调整为 164，标题区域预留两行高度；未选中的图标 tile 也绘制内容区不透明背景，避免旧帧文本区域残留；保留标题换行行为。
- 不适用项：未修改目录加载、自动刷新、文件扫描、图标解析或列表视图逻辑。
- 安全门禁执行结果：未执行破坏性操作；修改前检查工作区，存在与既有自动刷新任务相关的未提交改动，本次仅叠加显示层小改并保留既有改动。
- 风险矩阵结论：L1，局部 UI 显示修复，无公共接口、数据、依赖或架构变化。
- 涉及命令权限级别：C0/C1。
- 是否覆盖用户已有修改：否；`app.rs` 已有未提交自动刷新改动，本次只改图标标题文本属性。

## 3. 验证

- 验证环境：Linux 7.0.11-gentoo-dingjing x86_64 GNU/Linux；rustc 1.95.0；cargo 1.95.0。
- 命令/步骤：`cargo fmt --check`；`cargo test -p filesystem-gui`；`git diff --check -- crates/filesystem-gui/src/app.rs crates/filesystem-gui/src/style.rs`。
- 结果：通过；GUI crate 116 个测试通过。
- 未执行验证项：未在真实 X11/Wayland GUI 会话中手工切换“长标题多行目录 -> 短标题目录”确认残留消失。
- 未覆盖风险：不同渲染后端或窗口系统仍可能存在后端级缓存/裁剪问题；本次通过稳定两行标题布局和不透明 tile 背景规避已知触发条件。
- Bug 证据等级（按需）：E1，用户提供明确触发现象，但本地未运行真实 GUI 复现。
- Bug 修复验证（按需）：自动化验证覆盖编译和现有 GUI 状态/布局相关测试；真实视觉残留需人工复测。
