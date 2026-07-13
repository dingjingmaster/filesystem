# 右键菜单底部遮挡修复轻量总结

> 文档元数据
> - 文件编号：30
> - 文档类型：summary
> - 文件路径：docs/dev/30-summary-context-menu-bottom-clamp.md
> - 完成日期：2026-07-13
> - 需求级别：L1

## 1. 修改目标

- 原始需求：修复文件管理器底部右键时菜单被遮掉一部分，导致菜单项不能显示的问题。
- 成功标准：右键菜单靠近底部打开时根据菜单高度向上调整，菜单项保持可见；不改变既有菜单项和点击行为。

## 2. 修改结果

- 修改文件：`crates/filesystem-gui/src/app.rs`、`crates/filesystem-gui/src/components.rs`、`crates/filesystem-gui/src/config.rs`、`crates/filesystem-gui/src/utils.rs`、`docs/overview-product.md`、`docs/overview-product-dev.md`、`docs/dev/README.md`。
- 行为变化：右键菜单在保持原有横向钳制和固定宽度的基础上，按实际菜单项、分割线、内边距估算高度，并在浏览区域内钳制 y 坐标；空白菜单展开模板子菜单时按主菜单和子菜单的较大高度计算，避免底部触发时被裁剪。
- 不适用项：未改菜单项内容、文件操作语义、快捷键、右键命中规则或窗口布局。
- 安全门禁执行结果：已检查工作区状态；未执行破坏性操作；未执行需用户确认的 C2/C3 命令；保留既有未提交的 29 号任务文档和索引改动。
- 风险矩阵结论：L1，单模块 GUI 低风险行为修正；不涉及公共接口、数据结构、配置格式、依赖、部署或架构变化。
- 涉及命令权限级别：C0 只读检查；C1 工作区内代码/文档修改、格式化和本地验证。
- 是否覆盖用户已有修改：否。`docs/dev/README.md` 原有 29 号索引改动保留，本次只追加 30 号索引行。
- L1 审视结果：安全门禁、工作区保护、E1 相关验证、高级工程师简洁性/性能风险/风格一致性检查通过。

## 3. 验证

- 验证环境：Linux 7.0.11-gentoo-dingjing x86_64；rustc 1.95.0；cargo 1.95.0。
- 命令/步骤：
  - `cargo test -p filesystem-gui context_menu`
  - `cargo check -p filesystem-gui`
  - `cargo test -p filesystem-gui`
- 结果：全部通过；`filesystem-gui` 完整单元测试 110 passed。
- 未执行验证项：未在真实图形会话中人工右键实测。
- 未覆盖风险：若菜单本身高度超过整个浏览区域，目前只能从顶部开始显示，超高菜单仍需要后续滚动菜单能力解决。
- Bug 证据等级（按需）：E1。用户提供明确触发位置和可见现象，代码定位确认原实现只钳制 x 坐标。
- Bug 修复验证（按需）：新增/更新右键菜单定位单元测试覆盖 y 轴底部钳制、菜单高于浏览区域时回到顶部、模板子菜单较高时按子菜单高度计算。
