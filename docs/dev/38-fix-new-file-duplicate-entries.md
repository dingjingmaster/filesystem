# 新建文件后列表重复修复记录

> 文档元数据
> - 文件编号：38
> - 文档类型：fix
> - 文件路径：docs/dev/38-fix-new-file-duplicate-entries.md
> - 完成日期：2026-07-15
> - 需求级别：L2

## 1. 问题定义

- 问题现象：用户反馈新建文件后，文件视图显示了双倍的数据。
- 预期行为：新建文件或模板文件后，当前目录列表中每个路径只显示一次，并进入新文件重命名状态。
- 影响范围：当前文件夹自动刷新与新建成功后的目录 reload 交错时的 GUI 列表数据。
- 不包含：不修改 core 文件创建、模板复制、重命名语义；不改变 watcher 事件分类。

## 2. 证据与根因

- 复现方式：通过代码路径和状态机测试模拟新建触发 reload 后，自动刷新快照先写入列表，随后 reload 批次继续到达。
- 证据等级：E3。起始证据为用户反馈 E1；补充状态机回归测试后，修复前可稳定复现失败。
- 关键日志/堆栈/输入：修复前 `cargo test -p filesystem-gui directory_batch_deduplicates_existing_paths_from_auto_refresh` 失败，断言左侧为 `["alpha.txt", "alpha.txt", "bravo.txt", "bravo.txt"]`，期望为 `["alpha.txt", "bravo.txt"]`。
- 根因：`DirectoryLoadEvent::Batch` 直接 `self.entries.extend(entries)`；当 `AutoRefreshSnapshotLoaded` 已经把同一路径写入 `self.entries` 后，后续 reload 批次会再次追加同一路径，导致显示重复。
- 相关代码路径：`crates/filesystem-gui/src/app.rs` 的 `CreateFinished` / `TemplateCreateFinished` / `current_folder_changed` / `directory_event` / `apply_auto_refresh_snapshot`。

## 3. 修复方案

- 最小修复点：目录加载批次合并时按路径 upsert，而不是无条件 append。
- 代码逻辑改动：新增批次合并辅助逻辑；已有路径替换为最新批次数据，不存在路径才追加，随后保持现有排序和装饰任务。
- 影响的使用场景：新建文件、模板新建、外部结构变化与目录 reload/自动刷新交错时，列表不会重复显示同一路径。
- 不影响的使用场景：目录切换、搜索、文件创建 API、自动刷新事件分类和装饰逻辑不变。

## 4. 修复执行计划

| 步骤 | 修改内容 | 验证方式 | 状态 |
|------|----------|----------|------|
| 1 | 增加目录批次与自动刷新快照交错的回归测试 | `cargo test -p filesystem-gui directory_batch_deduplicates_existing_paths_from_auto_refresh` | 完成 |
| 2 | 将目录加载批次改为按路径合并 | `cargo test -p filesystem-gui directory_batch_deduplicates_existing_paths_from_auto_refresh` | 完成 |
| 3 | 执行相关 GUI 测试与格式检查 | `cargo test -p filesystem-gui`、`cargo fmt --check`、`git diff --check` | 完成 |

## 5. 风险摘要

| 项 | 结论 |
|----|------|
| 风险矩阵 | L2 |
| 命令权限 | C0/C1 |
| 高风险门禁 | 是，涉及文件系统事件监听后的异步 GUI 状态合并；限定为 GUI 内存列表合并，不改真实文件写入 |
| 破坏性操作 | 否 |
| 用户已有修改 | 否，编码前 `git status --short` 无输出 |
| 用户确认 | 无 |
| 副作用/风险 | 目录批次中的同一路径会保留一条最新数据；未覆盖真实 GUI 会话中的所有 notify 后端事件顺序 |

## 6. 验证

- 验证环境：Linux 7.0.11-gentoo-dingjing x86_64 GNU/Linux；rustc 1.95.0；cargo 1.95.0。
- 回归/相关验证：
  - 修复前 `cargo test -p filesystem-gui directory_batch_deduplicates_existing_paths_from_auto_refresh`：失败，复现重复路径。
  - 修复后 `cargo test -p filesystem-gui directory_batch_deduplicates_existing_paths_from_auto_refresh`：通过。
  - `cargo test -p filesystem-gui`：通过，125 个测试通过。
  - `cargo fmt --check`：通过。
  - `git diff --check`：通过。
- 结果：自动化验证通过。
- 未执行验证项：未启动真实 X11/Wayland GUI 手工右键新建观察；自动化测试已覆盖触发重复的状态机交错顺序。
- 残余风险：不同 notify 后端的真实事件顺序仍可能存在差异；本修复在目录批次合并层按路径去重，覆盖同一路径重复追加这一类后果。

## 7. 修复总结

- 最终结果：目录 reload 批次不再无条件追加；同一路径已存在时替换为批次中的最新条目，新建文件后不会因自动刷新快照交错显示双倍数据。
- 计划偏差：无。
- 审视结果：未执行破坏性操作；只改 GUI 内存列表合并和本地任务文档；不改变 ABI/API/协议/配置；未引入新依赖；回归测试覆盖用户问题路径。
- 后续建议：如真实 GUI 仍出现重复，应继续收集具体路径和事件顺序，优先检查搜索模式或其它入口是否存在独立追加路径。
