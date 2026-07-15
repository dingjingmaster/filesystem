# 列表视图所有者用户名轻量总结

> 文档元数据
> - 文件编号：33
> - 文档类型：summary
> - 文件路径：docs/dev/33-summary-list-owner-username.md
> - 完成日期：2026-07-14
> - 需求级别：L1

## 1. 修改目标

- 原始需求：列表视图“所有者”列不要显示用户 id，要显示用户名。
- 成功标准：列表视图所有者列优先把 UID 显示为本机用户名；无法解析用户名时保留 UID 作为兜底。

## 2. 修改结果

- 修改文件：`crates/filesystem-gui/src/utils.rs`、`docs/overview-product.md`、`docs/overview-product-dev.md`、`docs/dev/README.md`、`docs/dev/33-summary-list-owner-username.md`。
- 行为变化：GUI 列表视图通过缓存解析 `/etc/passwd`，把 `FileEntry.owner` 中的 UID 映射为用户名；未知 UID 或读取失败时显示原 UID。
- 不适用项：未修改 core `FileEntry.owner` 数据结构，属性弹窗 owner/group 标签仍按原有 UID/GID 格式显示。
- 安全门禁执行结果：未执行破坏性操作；修改前检查工作区，存在既有未提交改动，本次只叠加列表所有者显示逻辑和相关文档。
- 风险矩阵结论：L1，局部 UI 显示修正，无公共接口、数据结构、依赖或架构变化。
- 涉及命令权限级别：C0/C1。
- 是否覆盖用户已有修改：否；本次只修改 `utils.rs` 的所有者显示路径和相关文档。

## 3. 验证

- 验证环境：Linux 7.0.11-gentoo-dingjing x86_64 GNU/Linux；rustc 1.95.0；cargo 1.95.0。
- 命令/步骤：`cargo fmt --check`；`cargo test -p filesystem-gui`。
- 结果：通过；GUI crate 118 个测试通过，新增 `/etc/passwd` 解析和未知 UID 兜底测试。
- 未执行验证项：未在真实 GUI 会话中打开列表视图人工确认所有者列显示当前用户名。
- 未覆盖风险：`/etc/passwd` 缺失、特殊 NSS/LDAP 用户或运行中用户数据库变化时可能无法解析用户名；当前按 UID 兜底。
- Bug 证据等级（按需）：不适用。
- Bug 修复验证（按需）：不适用。
