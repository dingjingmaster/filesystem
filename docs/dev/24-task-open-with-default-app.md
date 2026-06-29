# 打开方式设为默认应用轻量任务记录

> 文档元数据
> - 文件编号：24
> - 文档类型：task
> - 文件路径：docs/dev/24-task-open-with-default-app.md
> - 文档版本：v1.0.0
> - 最后更新：2026-06-29
> - 需求级别：L2
> - 关联需求：打开方式中选择应用后支持设为默认打开方式

## 1. 目标

- 要解决的问题：现有打开方式弹窗只能临时选择应用打开文件，无法像 Nautilus/Nemo 一样把选中的应用保存为该文件类型默认应用。
- 成功标准：打开方式弹窗提供“设为默认打开方式”开关；用户确认打开后写入用户级 MIME apps 配置文件；后续默认应用打开和候选排序立即使用新默认应用；写入失败有状态反馈且不阻断本次打开。

## 2. 背景与边界

- 背景：项目已具备本地 `.desktop` 解析、`mimeapps.list` 默认应用读取、打开方式候选排序和外部应用启动。
- 包含：按当前桌面优先写入 `$XDG_CONFIG_HOME/<desktop>-mimeapps.list` 或 `$HOME/.config/<desktop>-mimeapps.list`，没有桌面名时回退通用 `mimeapps.list`；更新 `[Default Applications]` 和 `[Added Associations]`；打开成功后同步内存中的默认应用映射。
- 不包含：系统级配置写入、调用 `xdg-mime`/DBus/portal、跨桌面专用默认文件写入、为未知 MIME 新增应用选择器。
- 关键假设：选中的应用来自已解析 `.desktop` 注册表，应用 ID 可直接写入 `mimeapps.list`。
- 非目标：替换现有默认应用读取优先级；引入新的桌面服务依赖。
- 最大修改范围：`filesystem-gui` 的 app/model/tasks/apps 模块和长期文档。
- 禁止触碰范围：core 文件操作语义、MIME 识别规则、系统级 `/etc` 或 `/usr/share` 配置写入。

## 3. 风险门禁

| 项 | 结论 |
|----|------|
| 风险矩阵 | L2：新增用户级持久化配置写入，但作用域限定为当前用户配置文件 |
| 高风险开发门禁 | 否 |
| 破坏性操作 | 否；运行时只在用户显式勾选后写用户级 MIME apps 配置文件 |
| 用户已有修改 | 否；实施前工作区无冲突，按 `git status --short` 检查 |
| 底层/系统风险 | 否；Rust 安全代码，无 unsafe、ABI 或内核改动 |
| 命令权限 | C0/C1；仅本地编译测试 |
| 用户确认事项 | 无；用户已要求实现 |
| 回滚/止损方式 | 回退本次代码变更即可；运行时若写入失败只显示状态，不阻断打开文件 |

## 4. 方案

- 推荐方案：在打开方式弹窗增加复选框，确认打开时先启动所选应用，再尝试写用户级 MIME apps 配置文件，成功后更新内存注册表默认应用。
- 取舍理由：复用现有打开方式弹窗和候选排序，不引入桌面服务；写入用户配置符合 freedesktop MIME Apps 规范的默认应用和新增关联格式。
- 风险与应对：`mimeapps.list` 可能不存在或缺少 section，使用纯函数补齐 section 并保留无关行；权限或路径错误时返回状态消息。

## 5. 执行计划

| 步骤 | 修改内容 | 验证方式 | 状态 |
|------|----------|----------|------|
| 1 | 增加 `mimeapps.list` 用户级写入函数和单元测试 | `cargo test -p filesystem-gui` | 已完成 |
| 2 | 打开方式弹窗增加“设为默认打开方式”状态和 UI | `cargo check -p filesystem-gui` | 已完成 |
| 3 | 打开文件任务返回默认应用写入结果并同步内存默认映射 | `cargo test -p filesystem-gui` | 已完成 |
| 4 | 更新产品/开发概览和任务索引 | 文档审查 | 已完成 |

## 6. 实现记录

- 修改文件：`crates/filesystem-gui/src/apps.rs`、`crates/filesystem-gui/src/model.rs`、`crates/filesystem-gui/src/tasks.rs`、`crates/filesystem-gui/src/app.rs`、`docs/overview-product.md`、`docs/overview-product-dev.md`、`docs/dev/README.md`。
- 关键决策：只写用户级 MIME apps 配置文件，并与读取优先级一致优先写当前桌面专用文件；新默认应用写到列表首位并保留已有其它应用作为 fallback；打开成功后再尝试保存默认应用。
- 计划偏差：无。
- 安全门禁执行结果：未使用破坏性命令；未写工作区外文件；运行时写用户配置仅由用户显式勾选触发。

## 7. 验证记录

- 验证环境：本地 workspace。
- 系统信息：Rust/Cargo 当前工具链，Linux 本地构建环境。

| 验证项 | 命令/步骤 | 结果 | 备注 |
|--------|-----------|------|------|
| 格式检查 | `cargo fmt --check` | 通过 | rustfmt 排版一致 |
| 编译检查 | `cargo check -p filesystem-gui` | 通过 | GUI crate 编译通过 |
| 单元测试 | `cargo test -p filesystem-gui` | 通过 | 76 passed |

- 未执行验证项：未在真实图形会话中手动打开文件并检查系统文件管理器是否读取同一默认应用。
- 残余风险：不同桌面环境可能有各自的 `*-mimeapps.list`；本次只更新当前桌面的用户级文件或通用回退文件，不主动同步其它桌面的默认应用。

## 8. 总结

- 最终结果：打开方式弹窗支持勾选“设为默认打开方式”，并在打开成功后保存当前 MIME 的默认应用。
- 遗留风险：真实桌面会话下的系统级联动仍需人工验证。
- 后续建议：如需跨桌面统一默认应用，可后续评估是否同步多个桌面专用 `mimeapps.list`。
