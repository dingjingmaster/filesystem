# 自定义权限返回按钮图标轻量总结

> 文档元数据
> - 文件编号：6
> - 文档类型：summary
> - 文件路径：docs/dev/6-summary-properties-back-icon.md
> - 完成日期：2026-06-25
> - 需求级别：L1

## 1. 修改目标

- 原始需求：设置自定义权限页面的回退按钮使用 `icons/left.svg`，并垂直和水平居中。
- 成功标准：权限页返回按钮不再使用文本箭头，改用本地图标资源，图标在按钮内居中显示。

## 2. 修改结果

- 修改文件：
  - `crates/filesystem-gui/src/app.rs`
  - `docs/dev/README.md`
- 行为变化：设置自定义权限页返回按钮改用 `icons/left.svg`，通过已有 `toolbar_icon` 组件水平、垂直居中，按钮 padding 为 6px。
- 不适用项：未修改属性页状态切换、权限保存逻辑或文件系统逻辑。
- 安全门禁执行结果：未执行破坏性操作；基于当前未提交模块拆分和属性页对齐改动继续追加。
- 风险矩阵结论：L1。
- 涉及命令权限级别：C0/C1。
- 是否覆盖用户已有修改：否。

## 3. 验证

- 验证环境：本地 Linux workspace。
- 命令/步骤：
  - `cargo check -p filesystem-gui`
  - `cargo test -p filesystem-gui`
  - `git diff --check`
- 结果：全部通过；GUI 11 个测试通过。
- 未执行验证项：未启动真实 GUI 做视觉确认。
- 未覆盖风险：实际像素级视觉效果仍需人工开窗确认。
- Bug 证据等级（按需）：不适用。
- Bug 修复验证（按需）：不适用。
