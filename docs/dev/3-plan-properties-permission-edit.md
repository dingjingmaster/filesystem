# 属性弹窗与权限修改开发计划

> 文档元数据
> - 文件编号：3
> - 文档类型：plan
> - 文件路径：docs/dev/3-plan-properties-permission-edit.md
> - 文档版本：v1.0.0
> - 创建日期：2026-06-25
> - 关联调研：docs/dev/3-research-properties-permission-edit.md

## 1. 目标与成功标准

- 属性弹窗显示后，点击、右键、滚轮等鼠标事件不再传递到文件管理器底层视图。
- 属性弹窗标题区域可拖拽移动，窗口 resize 后位置保持在当前窗口可见范围内。
- 属性弹窗关闭按钮使用 `icons/close.svg`，并复用主窗口关闭按钮的尺寸、padding 和 hover 效果。
- 属性弹窗内展示信息水平左对齐、垂直居中；通用属性行不再用右侧贴边展示值。
- 权限页允许修改当前文件夹 owner/group/other 访问位，点击“更改”后后台执行权限修改并刷新属性。
- 权限页移除“更改内容文件的权限”入口和内容权限页，保留“更改”并增加“取消”按钮；权限页所有行和按钮垂直居中。

## 2. 修改边界

- 修改 `crates/filesystem-core/src/lib.rs`：新增 `set_permissions` 和临时目录测试。
- 修改 `crates/filesystem-gui/src/main.rs`：新增属性弹窗拖拽、事件隔离、权限编辑状态和后台保存任务。
- 更新 `docs/dev/README.md`、本任务文档、`docs/overview-product.md`、`docs/overview-product-dev.md`。
- 不修改 `/data/source/cosmic-files`，不引入桌面服务依赖，不实现递归权限修改。

## 3. 执行计划

| 步骤 | 修改内容 | 验证方式 | 状态 |
|------|----------|----------|------|
| 1 | core 新增权限修改 API 和测试 | `cargo test -p filesystem-core`：通过，19 个测试 | 完成 |
| 2 | GUI 属性弹窗事件隔离、拖拽、关闭按钮和对齐调整 | `cargo check -p filesystem-gui`、`cargo test -p filesystem-gui`：通过 | 完成 |
| 3 | GUI 权限页增加待保存 mode、访问级别切换、取消待保存改动、后台保存和错误展示；移除内容权限页 | `cargo check -p filesystem-gui`、`cargo test -p filesystem-gui`：通过，11 个 GUI 测试 | 完成 |
| 4 | 更新产品/开发文档和任务总结 | `git diff --check`：通过 | 完成 |
| 5 | 运行完整验证 | `cargo fmt --check`、`make test`、`git diff --check`：均通过 | 完成 |

## 4. 验证计划

- `cargo fmt --check`
- `cargo test -p filesystem-core`
- `cargo check -p filesystem-gui`
- `cargo test -p filesystem-gui`
- `make test`
- `git diff --check`

## 5. 安全门禁

- 权限修改属于文件系统写操作，core 测试只作用于临时目录。
- GUI 保存权限必须通过后台任务执行，UI 线程只更新状态。
- 不执行 C2/C3 命令，不调用 shell，不修改真实用户目录做验证。
