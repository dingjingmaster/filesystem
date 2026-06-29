# 当前文件夹自动刷新轻量任务记录

> 文档元数据
> - 文件编号：23
> - 文档类型：task
> - 文件路径：docs/dev/23-task-current-folder-auto-refresh.md
> - 文档版本：v1.0.0
> - 最后更新：2026-06-29
> - 需求级别：L2
> - 关联需求：文件管理器打开时监听当前打开文件夹，文件或文件夹发生变化后自动刷新。

## 1. 目标

- 要解决的问题：当前文件夹内容被外部进程新增、删除、重命名或修改后，GUI 需要自动刷新，避免用户看到过期列表。
- 成功标准：GUI 启动后监听当前 `cwd`；切换目录后监听目标随之切换；当前目录直接子项发生创建、删除、重命名、内容或元数据修改事件时触发刷新；刷新复用现有分批目录加载逻辑并丢弃过期请求。

## 2. 背景与边界

- 背景：现有刷新通过 `reload()` 手动或文件操作完成后触发，目录加载已是 iced stream 分批扫描。
- 包含：GUI 层文件事件监听、事件合并去抖、触发当前目录重新加载、监听错误状态提示、相关文档更新。
- 不包含：递归监听子目录内部变化；纯读取访问触发刷新；网络文件系统可靠性保证；新增手动刷新按钮。
- 关键假设：“增删改查”按文件管理器需要显示变化的场景处理，即新增、删除、重命名、内容修改和元数据修改；纯读取通常不改变目录展示，且文件系统事件不保证可观测。
- 非目标：不改变 core 目录扫描 API，不改变写操作语义，不引入桌面服务依赖。
- 最大修改范围：`filesystem-gui` 依赖、GUI 消息/状态、后台任务或订阅、产品/开发总览、本地任务索引。
- 禁止触碰范围：不改 `AGENTS.project.md`；不重构无关 UI；不修改 core 写操作实现。

## 3. 风险门禁

| 项 | 结论 |
|----|------|
| 风险矩阵 | L2 |
| 高风险开发门禁 | 是，涉及文件系统事件监听和异步订阅；限定在 GUI 层且不改 core 写操作 |
| 破坏性操作 | 否 |
| 用户已有修改 | 编码前 `git status --short` 未发现已有修改 |
| 底层/系统风险 | 有，Linux 文件事件监听；无 Rust unsafe、FFI 或 ABI/API 变更 |
| 命令权限 | C0/C1；若依赖解析需网络则按规则请求确认 |
| 用户确认事项 | 无 |
| 回滚/止损方式 | 回退新增依赖和 GUI 订阅/消息改动即可恢复原手动刷新行为 |

## 4. 方案

- 推荐方案：在 `filesystem-gui` 引入 `notify`，用 iced `Subscription::run_with` 按当前 `cwd` 创建非递归目录 watcher；watcher 收到目录变化事件后投递 `CurrentFolderChanged(path)`，状态机确认路径仍是当前目录后延时合并并调用现有 `reload()`。
- 取舍理由：复用现有目录加载、排序、装饰和过期请求丢弃逻辑，避免新增扫描路径；非递归监听只覆盖当前打开文件夹的直接列表，符合文件管理器当前视图并避免递归 watch 大目录。
- 风险与应对：Linux inotify 对部分伪文件系统、网络文件系统和极大事件量不保证完整，文档记录残余风险；事件可能连续触发，状态机侧做短延时合并，避免高频刷新。

## 5. 执行计划

| 步骤 | 修改内容 | 验证方式 | 状态 |
|------|----------|----------|------|
| 1 | 新增当前目录 watcher 订阅和消息，触发现有 `reload()` | `cargo check -p filesystem-gui` | 完成 |
| 2 | 增加必要单元测试覆盖路径校验和刷新触发边界 | `cargo test -p filesystem-gui` | 完成 |
| 3 | 更新产品/开发总览和本地索引 | 文档阅读、`git diff --check` | 完成 |

## 6. 实现记录

- 修改文件：`Cargo.lock`、`crates/filesystem-gui/Cargo.toml`、`crates/filesystem-gui/src/app.rs`、`crates/filesystem-gui/src/model.rs`、`crates/filesystem-gui/src/tasks.rs`、`docs/overview-product.md`、`docs/overview-product-dev.md`、`docs/dev/README.md`、`docs/dev/23-task-current-folder-auto-refresh.md`。
- 关键决策：新增 `notify` 作为 GUI 层依赖；使用 `Subscription::run_with(cwd, ...)` 让当前路径参与订阅身份，路径切换时自动重建 watcher；使用 `RecursiveMode::NonRecursive` 只监听当前目录直接子项；事件进入状态机后以 200ms 延时合并，再复用 `reload()` 或搜索重跑。
- 计划偏差：原计划描述“watcher 侧短延时合并”，实现调整为状态机侧 `CurrentFolderChanged` -> `CurrentFolderRefreshReady` 合并，便于忽略过期路径并复用 iced task。
- 安全门禁执行结果：未执行破坏性操作；新增依赖只用于 GUI 订阅；不改 core 写操作、扫描 API、ABI 或配置格式；纯读取访问不作为自动刷新触发条件。

## 7. 验证记录

- 验证环境：本地 Linux workspace。
- 系统信息（OS/内核/架构/编译器/运行时，按需）：Linux 7.0.11-gentoo-dingjing x86_64 GNU/Linux；rustc 1.93.1 (01f6ddf75 2026-02-11) (gentoo)；cargo 1.93.1 (083ac5135 2025-12-15) (gentoo)。

| 验证项 | 命令/步骤 | 结果 | 备注 |
|--------|-----------|------|------|
| 格式检查 | `cargo fmt --check` | 通过 | 初次发现 rustfmt 排版差异，已执行 `cargo fmt` 后通过 |
| GUI 编译检查 | `cargo check -p filesystem-gui` | 通过 | 新增 `notify` 及锁文件依赖解析完成 |
| GUI 测试 | `cargo test -p filesystem-gui` | 通过 | 68 个测试通过，新增当前文件夹自动刷新状态机测试 |
| diff 空白检查 | `git diff --check` | 通过 | 无空白错误 |

- 未执行验证项：未在真实 X11/Wayland GUI 会话中手工创建、删除、重命名和修改文件确认可视刷新。
- 残余风险：inotify/notify 对伪文件系统、网络文件系统、极大事件量或队列溢出不保证完整；只监听当前目录直接子项，不递归监听子目录内部变化；纯读取访问不保证触发事件。

## 8. 总结

- 最终结果：完成当前打开文件夹自动刷新；路径变化时 watcher 随 `cwd` 重建，当前目录直接子项新增、删除、重命名、内容修改或元数据修改会合并触发刷新。
- 遗留风险：真实图形会话和部分特殊文件系统仍需人工确认。
- 后续建议：如后续需要监听子目录内部变化，应单独评估递归 watch 的资源上限和大目录性能。
