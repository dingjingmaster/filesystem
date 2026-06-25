# 软链接角标与打开行为开发计划

> 文档元数据
> - 文件编号：12
> - 文档类型：plan
> - 文件路径：docs/dev/12-plan-symlink-badge-open.md
> - 文档版本：v1.0.0
> - 最后更新：2026-06-25
> - 关联需求：文件/文件夹软链接显示角标，断链显示断开角标，双击有效软链接打开目标，双击断链弹窗提示。
> - 关联调研：docs/dev/12-research-symlink-badge-open.md

## 1. 目标与成功标准

- 任务目标：让本地文件视图正确展示和打开文件/目录软链接，并对断链给出明确弹窗反馈。
- 成功标准：有效软链接显示 `icons/symbol.svg` 角标，断链显示 `icons/symbol-disconnect.svg`；目录软链接双击进入目标目录；文件软链接双击用默认应用打开目标文件；断链双击弹窗提示“软连接 xxx 损坏”；右键菜单按有效目标类型分类。
- 前置条件：`icons/symbol.svg` 和 `icons/symbol-disconnect.svg` 已在项目根 `icons/` 下提供。
- 非目标：不递归搜索目录软链接，不实现创建软链接入口。

## 2. 修改边界

- 最大修改范围：`filesystem-core` 条目模型与属性读取，`filesystem-gui` 显示模型、图标组件、打开路径、菜单分类和提示弹窗，相关文档。
- 禁止触碰范围：不改变复制/剪切/重命名/删除对软链接路径本身的操作语义；不新增桌面服务依赖。
- 影响模块/文件：`crates/filesystem-core/src/lib.rs`，`crates/filesystem-gui/src/{app,components,icons,model,utils}.rs`，`docs/*`。
- 依赖关系：无新增 crate；新增 SVG 继续通过 `include_bytes!` 编入 GUI。

## 3. 安全门禁摘要

| 项 | 结论 |
|----|------|
| 风险矩阵结论 | L3 |
| 命令权限 | C0/C1 |
| 高风险开发门禁 | 是，文件系统符号链接与打开关键链路 |
| 破坏性操作 | 否 |
| 用户确认事项 | 无 |
| 止损/回滚方案 | 回退 12 号相关代码和文档即可恢复原软链接行为 |

## 4. Bug 修复计划（按需）

- 复现方式：不适用。
- 证据等级：不适用。
- 根因假设：不适用。
- 最小修复点：不适用。
- 回归/相关验证：新增 core 与 GUI 单测。

## 5. 执行计划

| 步骤 | 修改内容 | 验证方式 | 状态 |
|------|----------|----------|------|
| 1 | core 增加 `SymlinkTarget`，扫描时记录目标类型、断链状态和 canonical path | `cargo test -p filesystem-core` | 已完成 |
| 2 | GUI `DisplayEntry` 增加角标，图标视图/列表视图叠加普通链接或断链角标 | `cargo test -p filesystem-gui` | 已完成 |
| 3 | GUI 打开路径按有效目标类型处理，断链显示阻塞弹窗 | `cargo check -p filesystem-gui` | 已完成 |
| 4 | 让文件/目录属性跟随有效软链接目标读取 | `cargo test -p filesystem-core` | 已完成 |
| 5 | 更新总览和本地任务索引 | 文档阅读、`git diff --check` | 已完成 |

## 6. 验证计划

- 基础验证：`cargo fmt --check`、`cargo test -p filesystem-core`、`cargo test -p filesystem-mime`、`cargo check -p filesystem-gui`、`cargo test -p filesystem-gui`、`make test`、`git diff --check`。
- 高风险验证：core 临时目录覆盖文件软链接、目录软链接、断链、属性跟随；GUI 单测覆盖角标状态。
- 验证环境：本地 Linux workspace。
- 不可执行验证项：真实 GUI 中角标视觉位置、X11/Wayland 双击打开目标路径需人工验证。
- 残余风险：特殊权限导致 `metadata` 不可访问时按断链处理。

## 7. 变更记录（按需）

| 日期 | 变更 | 原因 |
|------|------|------|
| 2026-06-25 | 创建计划并按实现结果更新状态 | 当前需求为跨 core/GUI 的 L3 功能 |
