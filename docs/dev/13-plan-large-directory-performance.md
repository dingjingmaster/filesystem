# 大目录打开性能优化开发计划

> 文档元数据
> - 文件编号：13
> - 文档类型：plan
> - 文件路径：docs/dev/13-plan-large-directory-performance.md
> - 文档版本：v1.0.0
> - 最后更新：2026-06-25
> - 关联需求：按先显示、缓存、虚拟化、分批加载四点优化大目录打开卡顿。
> - 关联调研：docs/dev/13-research-large-directory-performance.md

## 1. 目标与成功标准

- 任务目标：优化大目录打开路径，使 UI 不等待完整目录装饰和全部条目 widget 构建后才响应。
- 成功标准：目录扫描按批次更新 UI；文件先显示基础图标和名称，MIME/主题图标后台装饰；主题图标解析有进程内缓存；图标视图和列表视图只构建可视区域附近条目。
- 前置条件：保持现有 Rust workspace、iced/wgpu GUI 和后台任务模型。
- 非目标：不新增持久索引、不引入桌面服务、不改变搜索和文件操作语义。

## 2. 修改边界

- 最大修改范围：`filesystem-core` 扫描 API、`filesystem-gui` 的 model/tasks/icons/app/config、产品/开发文档和本地任务文档。
- 禁止触碰范围：不改系统配置；不执行真实删除/复制用户数据；不引入外部服务。
- 影响模块/文件：`crates/filesystem-core/src/lib.rs`、`crates/filesystem-gui/src/{app.rs,config.rs,icons.rs,model.rs,tasks.rs}`。
- 依赖关系：继续使用 iced `Task` 和 `stream::channel`；不新增 crate。

## 3. 安全门禁摘要

| 项 | 结论 |
|----|------|
| 风险矩阵结论 | L3 |
| 命令权限 | C0/C1 |
| 高风险开发门禁 | 是，文件系统扫描、后台任务、性能关键路径 |
| 破坏性操作 | 否 |
| 用户确认事项 | 无 |
| 止损/回滚方案 | 回退本次扫描/装饰/虚拟化改动即可恢复原完成后一次性加载模式 |

## 4. Bug 修复计划（按需）

- 复现方式：未提供固定复现目录；基于用户反馈和代码路径分析。
- 证据等级：E1。
- 根因假设：虽然扫描在后台，但 UI 收到结果前仍等待整目录扫描、MIME/图标装饰；渲染时对全部条目构建 widget。
- 最小修复点：分批扫描返回基础条目；后台装饰；图标缓存；可视区域虚拟化。
- 回归/相关验证：编译测试和人工大目录打开体验验证。

## 5. 执行计划

| 步骤 | 修改内容 | 验证方式 | 状态 |
|------|----------|----------|------|
| 1 | 新增 core 分批扫描 API 和 GUI 目录加载事件模型 | `cargo check -p filesystem-gui` | 完成 |
| 2 | 基础条目先显示，MIME/图标后台装饰，主题图标和应用图标缓存 | `cargo test -p filesystem-gui` | 完成 |
| 3 | 图标视图和列表视图虚拟化，只渲染可视区域附近条目 | `cargo check -p filesystem-gui`，人工审查布局公式 | 完成 |
| 4 | 更新总文档、任务索引和 Summary | 文档链接检查、人工审查 | 完成 |
| 5 | 运行格式化和项目验证 | `cargo fmt --check`、`cargo test -p filesystem-core`、`cargo test -p filesystem-mime`、`cargo check -p filesystem-gui`、`cargo test -p filesystem-gui`、`make test`、`git diff --check` | 完成 |

## 6. 验证计划

- 基础验证：运行 Rust fmt、相关 crate 测试、GUI 编译、全项目 make test 和 diff whitespace 检查。
- 高风险验证（按需）：人工审查后台任务请求 ID 丢弃过期结果、错误路径状态恢复、虚拟化滚动高度保持。
- 验证环境：本地 Linux workspace，Rust workspace。
- 不可执行验证项：当前自动化不真实打开 GUI，也不测 X11/Wayland 大目录帧时间。
- 残余风险：极端慢文件系统下 `read_dir` 单次 `next` 仍可能慢；真实视觉顺滑度需要图形会话人工确认。

## 7. 变更记录（按需）

| 日期 | 变更 | 原因 |
|------|------|------|
| 2026-06-25 | 创建计划 | 大目录打开卡顿专项优化 |
