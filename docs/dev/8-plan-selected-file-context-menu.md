# 选中文件右键菜单开发计划

> 文档元数据
> - 文件编号：8
> - 文档类型：plan
> - 文件路径：docs/dev/8-plan-selected-file-context-menu.md
> - 文档版本：v1.0.0
> - 最后更新：2026-06-25
> - 关联需求：左键选中文件后，右键显示文件菜单，支持默认应用打开、打开方式、复制、剪切、重命名、删除和属性
> - 关联调研：docs/dev/8-research-selected-file-context-menu.md

## 1. 目标与成功标准

- 任务目标：实现单个选中非文件夹条目的右键文件菜单、打开方式弹窗和文件属性弹窗。
- 成功标准：
  - 单个非文件夹条目已选中且右键命中该条目时显示文件菜单。
  - 文件菜单项为：`用 xxx 打开`、`打开方式`、复制、剪切、重命名、删除、属性，中间按需求分割。
  - `用 xxx 打开` 使用默认应用名称；无默认应用时显示保守文案并提示无默认应用，用户仍可通过“打开方式”选择可用应用。
  - 打开方式弹窗列出匹配 MIME 的本地应用，默认应用预选；每个应用显示图标和名称，名称左对齐、垂直居中，选中对钩右对齐。
  - 复制/剪切/重命名/删除复用文件夹菜单语义；删除前确认。
  - 文件属性弹窗显示文件名、类型、大小、上级文件夹、访问/修改/创建时间、权限和是否可执行。
- 前置条件：沿用现有模块化 GUI 和 core 文件操作 API。
- 非目标：持久化默认应用、回收站、多选菜单、完整 MIME 数据库。

## 2. 修改边界

- 最大修改范围：
  - `crates/filesystem-core/src/lib.rs`
  - `crates/filesystem-gui/src/main.rs`
  - `crates/filesystem-gui/src/model.rs`
  - `crates/filesystem-gui/src/app.rs`
  - `crates/filesystem-gui/src/tasks.rs`
  - 新增 `crates/filesystem-gui/src/apps.rs`
  - `crates/filesystem-gui/src/components.rs`
  - `crates/filesystem-gui/src/style.rs`
  - `crates/filesystem-gui/src/utils.rs`
  - `docs/dev/README.md`
  - `docs/overview-product-dev.md`
  - 本任务 Research/Plan/Summary 文档
- 禁止触碰范围：Cargo 依赖、图标资源、无关 UI 重构。
- 影响模块/文件：
  - core 新增 `FileProperties` 和 `file_properties`。
  - GUI 新增应用注册表解析和外部应用启动适配。
  - GUI 新增文件菜单、打开方式弹窗和文件属性摘要。
- 依赖关系：GUI app 使用 apps 模块提供的本地应用注册表、默认应用查询、可选应用列表和打开任务。

## 3. 安全门禁摘要

| 项 | 结论 |
|----|------|
| 风险矩阵结论 | L4 |
| 命令权限 | C0/C1 |
| 高风险开发门禁 | 是，文件系统写操作和外部程序启动 |
| 破坏性操作 | 代码能力包含删除；验证只允许临时目录删除 |
| 用户确认事项 | 无；运行时删除需要确认弹窗 |
| 止损/回滚方案 | 若验证失败，停止并修复或回退本次新增文件菜单、应用解析和文件属性 API |

## 4. Bug 修复计划（按需）

- 不适用。

## 5. 执行计划

| 步骤 | 修改内容 | 验证方式 | 状态 |
|------|----------|----------|------|
| 1 | 建立 Research/Plan 文档并更新任务索引 | 人工检查文档编号和边界 | 已完成 |
| 2 | core 增加文件属性 API 和测试 | `cargo test -p filesystem-core` | 已完成 |
| 3 | 新增 apps 模块，后台加载本地应用注册表并实现直接启动 | `cargo test -p filesystem-gui`、`cargo check -p filesystem-gui` | 已完成 |
| 4 | GUI 增加文件菜单、打开方式弹窗、文件属性弹窗和文件删除确认文案 | `cargo check -p filesystem-gui` | 已完成 |
| 5 | 更新长期开发概览和 Summary | 人工检查文档一致性 | 已完成 |
| 6 | 完整验证 | `cargo fmt --check`、`cargo check -p filesystem-gui`、`cargo test -p filesystem-core`、`cargo test -p filesystem-gui`、`make test`、`git diff --check` | 已完成 |

## 6. 验证计划

- 基础验证：
  - `cargo fmt --check`
  - `cargo check -p filesystem-gui`
  - `cargo test -p filesystem-core`
  - `cargo test -p filesystem-gui`
  - `make test`
  - `git diff --check`
- 高风险验证：
  - 文件属性 API 用临时目录测试覆盖。
  - 删除仍复用已有临时目录测试，不对真实用户目录执行。
  - 外部应用启动不在自动化中真实打开 GUI 程序，只验证解析函数和编译路径。
- 验证环境：本地 Linux workspace。
- 不可执行验证项：不真实启动 GUI 应用做打开文件测试，不真实开窗检查打开方式/属性视觉。
- 残余风险：不同发行版 `.desktop` 和 MIME 配置兼容性有限；真实应用启动需人工确认。
- 验证结果：`cargo fmt --check`、`cargo check -p filesystem-gui`、`cargo test -p filesystem-core`、`cargo test -p filesystem-gui`、`make test`、`git diff --check` 均通过。

## 7. 变更记录（按需）

| 日期 | 变更 | 原因 |
|------|------|------|
| 2026-06-25 | 创建计划 | 响应选中文件右键菜单需求 |
