# 地址栏粘贴长度限制轻量总结

> 文档元数据
> - 文件编号：41
> - 文档类型：summary
> - 文件路径：docs/dev/41-summary-address-bar-paste-limit.md
> - 完成日期：2026-07-16
> - 需求级别：L1

## 1. 修改目标

- 原始需求：地址栏粘贴时设置全局 4096 字节上限；超过上限时拒绝本次输入，保留原地址栏内容，并在状态栏提示“路径或搜索内容过长”。
- 成功标准：地址栏接受不超过 4096 字节的输入；超过 4096 字节的粘贴不会替换现有内容；状态栏显示指定提示。

## 2. 修改结果

- 修改文件：`crates/filesystem-gui/src/config.rs`、`crates/filesystem-gui/src/model.rs`、`crates/filesystem-gui/src/app.rs`、`docs/dev/README.md`、`docs/overview-product.md`、`docs/overview-product-dev.md`。
- 行为变化：地址栏输入统一受 `ADDRESS_BAR_MAX_INPUT_BYTES = 4096` 保护；地址栏粘贴走独立 `PathPasted` 消息，超限时拒绝并保留旧值。
- 不适用项：未改变文件粘贴菜单或空选择 `Ctrl+V` 的剪贴板文件路径解析规则。
- 安全门禁执行结果：修改前已检查工作区状态；仅改当前需求相关文件；未执行破坏性操作。
- 风险矩阵结论：L1，局部 GUI 状态机行为修正，不改变公共接口、数据结构或架构。
- 涉及命令权限级别：C0 只读检查，C1 工作区内源码与本地文档修改、定向测试。
- 是否覆盖用户已有修改：否。

## 3. 验证

- 验证环境：本地工作区。
- 命令/步骤：`cargo test -p filesystem-gui`、`cargo fmt --check`、`git diff --check`。
- 结果：`cargo test -p filesystem-gui` 通过，141 个测试通过；`cargo fmt --check` 通过；`git diff --check` 通过。
- 未执行验证项：真实图形会话手工粘贴待按需验证。
- 未覆盖风险：Iced/平台剪贴板实际事件投递差异需要真实 X11/Wayland 会话确认。
