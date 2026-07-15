# 自动刷新 model/view 优化开发结论摘要

> 文档元数据
> - 文件编号：31
> - 文档类型：summary
> - 文件路径：docs/dev/31-summary-auto-refresh-model-view.md
> - 文档版本：v1.0.0
> - 完成日期：2026-07-14
> - 关联需求：编译导致频繁刷新文件夹、界面闪烁；按目录数据刷新和可视范围刷新方向优化。
> - 关联调研：docs/dev/31-research-auto-refresh-model-view.md
> - 关联计划：docs/dev/31-plan-auto-refresh-model-view.md

## 1. 最终结果

- 原始需求：分析并修复编译导致当前文件夹频繁刷新、界面闪烁的问题；用户否定尾部去抖后，要求按目录数据刷新、可视范围刷新和切换目录全刷新方向完成优化。
- 最终方案：自动刷新不再复用清空式 `reload()`；watcher 传递事件 kind 和 paths，普通文件内容/metadata 变化刷新单个条目，新增/删除/重命名/rescan 等结构或不确定事件走后台快照合并；快照合并保留旧界面、选择和滚动，若当前虚拟化可视范围发生变化则补装饰当前范围内所有条目，否则只补装饰当前范围内变化条目；切换目录仍走原全量加载。
- 完成状态：完成。
- 需求变更：尾部去抖方案被明确排除；实现改为事件级 model/view 风格更新。

## 2. 关键改动

- 修改文件：
  - `crates/filesystem-core/src/lib.rs`
  - `crates/filesystem-gui/src/icons.rs`
  - `crates/filesystem-gui/src/model.rs`
  - `crates/filesystem-gui/src/tasks.rs`
  - `crates/filesystem-gui/src/app.rs`
  - `docs/overview-product.md`
  - `docs/overview-product-dev.md`
  - `docs/dev/README.md`
  - `docs/dev/31-research-auto-refresh-model-view.md`
  - `docs/dev/31-plan-auto-refresh-model-view.md`
  - `docs/dev/31-summary-auto-refresh-model-view.md`
- 代码逻辑改动：
  - core 新增 `entry_for_path`，复用原 `FileEntry` 元数据和软链接识别逻辑，供 GUI 单项刷新。
  - GUI 新增 `CurrentFolderChange`、`AutoRefreshRequest` 和自动刷新结果消息。
  - watcher 将 notify 事件分类为 `Entry`、`Structure` 或 `Rescan`，只保留当前目录直接子项路径；嵌套子目录路径和目录子项 modify 事件被忽略。
  - 文件修改事件通过后台单项读取和装饰更新对应 `DisplayEntry`，不清空列表、不重置滚动、不清选择。
  - 结构或不确定事件通过 single-flight 后台快照同步，扫描中再次变脏只标记 dirty，完成后立即补一轮。
  - 快照同步按 path 合并，保留仍存在条目的图标/MIME/角标和选择状态，移除消失路径；若可视范围加缓冲区内的路径集合变化，则补装饰当前范围内所有条目，否则只补装饰当前范围内变化条目。
- 影响的使用场景：编译、高频保存、批量创建/删除/重命名当前目录直接子项时，自动刷新不再反复清空整个文件视图。
- 不影响的使用场景：用户切换目录、后退/前进、地址栏跳转仍按原有全量加载；不递归监听子目录内部变化；不硬编码忽略构建目录。
- 计划偏差：无。

## 3. 安全门禁结果

| 项 | 结论 |
|----|------|
| 风险矩阵 | L3 |
| 命令权限 | C0/C1 |
| 高风险项 | 有，文件系统事件监听、异步状态机和性能关键路径；通过事件分类、single-flight 和单元测试约束风险 |
| 破坏性操作 | 无 |
| 用户已有修改 | 无，编码前 `git status --short` 无输出 |
| 用户确认事项 | 无 |
| 副作用/风险 | 未做真实图形会话编译风暴人工验证；不同 notify 后端的事件细节仍可能有差异 |

## 4. 验证结果

- 验证环境：Linux gentoo-pc 7.0.11-gentoo-dingjing x86_64；rustc 1.95.0；cargo 1.95.0。
- 执行验证：
  - `cargo fmt --check`：通过
  - `cargo test -p filesystem-core`：通过，34 个测试通过
  - `cargo test -p filesystem-gui`：通过，116 个测试通过
  - `cargo check -p filesystem-gui`：通过
  - `git diff --check`：通过
- 结果：自动化验证通过。
- 未执行验证项：未在真实 X11/Wayland GUI 会话中启动编译风暴手工观察闪烁改善。
- 残余风险：notify 在网络文件系统、伪文件系统、大量事件溢出或其它后端下可能发出不完整/不一致事件；实现已对 `need_rescan` 和不确定事件保留快照校准。

## 5. Bug 修复验证

- 原问题复现：未运行真实 GUI 复现；静态证据显示旧路径中高频事件会周期性调用清空式 `reload()`，导致 `entries.clear()` 后分批重建。
- 证据等级：E1。
- 根因：自动刷新复用切换目录的清空加载路径，而不是只更新受影响的数据模型。
- 回归/相关验证：新增 core 单项读取测试、watcher 事件分类测试、GUI 自动刷新状态机测试，覆盖嵌套子目录忽略、目录 modify 忽略、结构变化不清空、single-flight dirty、单项更新不清空和快照合并保留状态。

## 6. 后续事项

- 技术债：真实 GUI 编译风暴和不同文件系统组合仍需人工确认。
- 后续建议：如果仍有高频结构变化导致可见区域频繁变动，可继续细化为行级插入/删除动画或更完整的 model/view diff。
- 关联文档/提交：未提交；当前无用户提交授权。
