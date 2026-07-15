# 属性权限页所有者和用户组名称轻量总结

> 文档元数据
> - 文件编号：36
> - 文档类型：summary
> - 文件路径：docs/dev/36-summary-properties-owner-group-name.md
> - 完成日期：2026-07-15
> - 需求级别：L1

## 1. 修改目标

- 原始需求：文件夹右键菜单属性 -> 权限 -> 显示权限界面中，“所有者 UID xxx”改为“所有者 用户名(UID)”，“用户组 GID xxx”改为“用户组 组名(GID)”。
- 成功标准：属性权限页优先显示本机用户名和组名，并保留 UID/GID；无法解析名称时继续显示原 UID/GID 兜底。

## 2. 修改结果

- 修改文件：`crates/filesystem-gui/src/utils.rs`、`docs/overview-product.md`、`docs/overview-product-dev.md`、`docs/dev/README.md`、`docs/dev/36-summary-properties-owner-group-name.md`。
- 行为变化：属性权限页所有者值从 `UID xxx` 改为 `用户名(xxx)`；用户组值从 `GID xxx` 改为 `组名(xxx)`；未知 UID/GID 或读取失败时保留 `UID xxx` / `GID xxx`。
- 不适用项：未修改 core 的 `FolderProperties` / `FileProperties` 数据结构；未修改 owner/group；未改变权限保存逻辑；列表视图所有者列仍按既有规则只显示用户名或 UID 兜底。
- 安全门禁执行结果：修改前检查工作区，未发现既有未提交改动；本次未执行破坏性操作，未修改真实文件权限或属主。
- 风险矩阵结论：L1，局部 GUI 显示调整，无公共接口、数据结构、依赖或架构变化。
- 涉及命令权限级别：C0/C1。
- 是否覆盖用户已有修改：否。

## 3. 验证

- 验证环境：Linux 7.0.11-gentoo-dingjing x86_64 GNU/Linux；rustc 1.95.0；cargo 1.95.0。
- 命令/步骤：`cargo fmt --check`；`cargo test -p filesystem-gui`。
- 结果：通过；GUI crate 122 个测试通过，新增 `/etc/group` 解析和 `名称(id)` 显示格式测试。
- 未执行验证项：未在真实 GUI 会话中打开文件夹属性权限页人工确认显示效果。
- 未覆盖风险：系统不使用 `/etc/passwd` 或 `/etc/group` 承载账号/组信息时，名称可能无法解析；当前按 UID/GID 兜底。
- Bug 证据等级（按需）：不适用。
- Bug 修复验证（按需）：不适用。
