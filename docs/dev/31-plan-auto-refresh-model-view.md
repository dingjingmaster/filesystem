# 自动刷新 model/view 优化开发计划

> 文档元数据
> - 文件编号：31
> - 文档类型：plan
> - 文件路径：docs/dev/31-plan-auto-refresh-model-view.md
> - 文档版本：v1.0.0
> - 最后更新：2026-07-14
> - 关联需求：编译导致频繁刷新文件夹、界面闪烁；按目录数据刷新和可视范围刷新方向优化。
> - 关联调研：docs/dev/31-research-auto-refresh-model-view.md

## 1. 目标与成功标准

- 任务目标：将当前文件夹自动刷新从全量清空 reload 优化为事件级数据更新和结构变化快照同步，减少编译等高频事件导致的界面闪烁。
- 成功标准：
  - 普通文件写入/metadata 变化只更新对应条目，不清空整个文件夹列表。
  - 当前目录直接子项新增、删除、重命名或不确定事件触发后台快照同步，但不清空旧界面。
  - 当前目录下子目录内部变化不触发父目录数据刷新。
  - 切换目录仍走现有全量刷新流程。
  - 自动化测试覆盖事件分类、单项更新和结构变化同步边界。
- 前置条件：沿用现有 `notify`、`Task`、目录扫描、装饰和虚拟化视图。
- 非目标：不新增配置项；不递归监听；不硬编码忽略构建目录；不重构整体 UI。

## 2. 修改边界

- 最大修改范围：`filesystem-core` 单项条目读取 API；`filesystem-gui` 的模型消息、watcher 事件转换、自动刷新状态机、相关单元测试；本地任务文档和索引。
- 禁止触碰范围：不修改 `AGENTS.project.md`；不修改文件写操作语义；不引入新依赖；不调整无关样式。
- 影响模块/文件：
  - `crates/filesystem-core/src/lib.rs`
  - `crates/filesystem-gui/src/model.rs`
  - `crates/filesystem-gui/src/tasks.rs`
  - `crates/filesystem-gui/src/app.rs`
  - `docs/dev/*`
- 依赖关系：GUI 单项刷新依赖 core 暴露按 path 读取 `FileEntry` 的 API；结构变化快照复用现有目录扫描基础逻辑。

## 3. 安全门禁摘要

| 项 | 结论 |
|----|------|
| 风险矩阵结论 | L3 |
| 命令权限 | C0/C1 |
| 高风险开发门禁 | 是，文件系统事件监听、异步状态机、性能关键路径 |
| 破坏性操作 | 否 |
| 用户确认事项 | 无 |
| 止损/回滚方案 | 回退本次 core API、GUI 自动刷新消息/状态机和文档即可恢复原 reload 自动刷新行为 |

## 4. Bug 修复计划

- 复现方式：未运行真实编译复现；通过现有代码路径确认高频事件会周期性触发 `reload()` 并清空 `entries`。
- 证据等级：E1。
- 根因假设：自动刷新复用切换目录的清空加载路径，导致高频文件事件下 UI 反复空白和重建。
- 最小修复点：自动刷新路径改为单项更新/后台快照同步，不再调用清空式 `reload()`。
- 回归/相关验证：新增状态机单元测试并执行相关 Rust 测试和编译检查。

## 5. 执行计划

| 步骤 | 修改内容 | 验证方式 | 状态 |
|------|----------|----------|------|
| 1 | 增加 core 单路径 `FileEntry` 读取能力，供 GUI 刷新单个条目 | `cargo test -p filesystem-core` | 完成 |
| 2 | 将 watcher 消息从 `cwd` 扩展为事件 kind + paths，并过滤非当前目录直接子项 | `cargo test -p filesystem-gui` | 完成 |
| 3 | 实现自动刷新状态机：文件修改单项刷新；结构/不确定事件后台快照同步；切换目录保持全量刷新 | `cargo test -p filesystem-gui`、`cargo check -p filesystem-gui` | 完成 |
| 4 | 更新长期开发/产品事实和本地索引，总结验证结果 | 文档阅读、`git diff --check` | 完成 |

## 6. 验证计划

- 基础验证：
  - `cargo fmt --check`
  - `cargo test -p filesystem-core`
  - `cargo test -p filesystem-gui`
  - `cargo check -p filesystem-gui`
  - `git diff --check`
- 高风险验证：通过单元测试覆盖事件分类、当前目录边界、过期请求丢弃和状态保持；人工审查 async task 错误路径。
- 验证环境：本地 Linux workspace；Linux gentoo-pc 7.0.11-gentoo-dingjing x86_64；rustc 1.95.0；cargo 1.95.0。
- 不可执行验证项：本轮未启动真实 GUI 编译风暴手工验证，避免额外图形会话依赖；在 Summary 记录残余风险。
- 残余风险：不同 notify 后端事件细节可能仍需真实图形会话和文件系统组合验证。

## 7. 变更记录

| 日期 | 变更 | 原因 |
|------|------|------|
| 2026-07-14 | 创建计划，按事件级数据更新和结构变化后台同步实施 | 用户确认按 model/view 方向优化 |
