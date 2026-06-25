# 大目录打开性能优化开发结论摘要

> 文档元数据
> - 文件编号：13
> - 文档类型：summary
> - 文件路径：docs/dev/13-summary-large-directory-performance.md
> - 文档版本：v1.0.0
> - 完成日期：2026-06-25
> - 关联需求：按先显示、缓存、虚拟化、分批加载四点优化大目录打开卡顿。
> - 关联调研：docs/dev/13-research-large-directory-performance.md
> - 关联计划：docs/dev/13-plan-large-directory-performance.md

## 1. 最终结果

- 原始需求：解释并优化文件夹中大量文件时打开会卡一瞬间的问题。
- 最终方案：目录扫描改为 core `DirectoryScanner` 分批读取，GUI 使用 iced stream 分批接收基础条目；MIME 内容识别和主题图标解析改为后续后台装饰任务；主题图标和 APP 图标使用进程内缓存；图标视图和列表视图只构建可视区域附近条目，并用占位高度保持滚动尺寸。
- 完成状态：完成。
- 需求变更：无。

## 2. 关键改动

- 修改文件：`crates/filesystem-core/src/lib.rs`、`crates/filesystem-gui/src/app.rs`、`config.rs`、`icons.rs`、`model.rs`、`tasks.rs`、产品/开发总览和 13 号任务文档。
- 代码逻辑改动：新增 `DirectoryScanner` 和 `scan_dir_incremental`；`Message::DirectoryEvent` 支持 Started/Batch/Finished/Failed；`DisplayEntry` 先使用名称识别和本地回退图标，后台 `EntryDecoration` 再替换 MIME、主题图标和角标；`IconResolver` 的主题路径和 icon bytes 使用 `OnceLock`/`Mutex<HashMap<...>>` 缓存；图标/列表视图根据滚动偏移和窗口高度做行级虚拟化。
- 影响的使用场景：打开大目录、切换目录、搜索结果展示后的图标装饰、图标视图和列表视图滚动。
- 不影响的使用场景：右键菜单、双击打开、框选、重命名覆盖层、复制/剪切/删除语义不变。
- 计划偏差：为避免 stream 缓冲满时丢批次，额外把 core 分批扫描实现成可逐批 `next_batch` 的 `DirectoryScanner`。

## 3. 安全门禁结果

| 项 | 结论 |
|----|------|
| 风险矩阵 | L3 |
| 命令权限 | C0/C1 |
| 高风险项 | 有，涉及文件系统扫描、后台任务、性能关键路径和共享数据结构 |
| 破坏性操作 | 无 |
| 用户已有修改 | 无关用户修改未发现；本轮只基于当前需求增量修改 |
| 用户确认事项 | 无 |
| 副作用/风险 | 分批加载时首屏条目会先显示回退图标，随后再替换为主题图标；真实大目录帧时间仍需 GUI 会话人工确认 |

## 4. 验证结果

- 验证环境：本地 Linux workspace，Rust workspace。
- 系统信息（按需）：不适用。
- 执行验证：`cargo fmt --check`；`cargo check -p filesystem-gui`；`cargo test -p filesystem-core`；`cargo test -p filesystem-mime`；`cargo test -p filesystem-gui`；`make test`；`git diff --check`。
- 结果：全部通过。`make test` 覆盖 core 27 个、GUI 30 个、MIME 10 个单元测试。
- 未执行验证项：未真实启动 GUI 测量大目录首屏时间、滚动帧时间和 X11/Wayland 视觉效果。
- 残余风险：极端慢文件系统单次读取目录项仍可能慢；用户在后台装饰完成前双击无扩展名或需内容签名识别的文件时，可能先按名称识别结果选择应用。

## 5. Bug 修复验证（按需）

- 原问题复现：用户反馈大目录打开卡顿；代码路径显示旧实现等待完整扫描、装饰和全量 widget 构建。
- 证据等级：E1。
- 根因：完成消息晚于全量扫描和 MIME/图标装饰，渲染时为全部条目构建 UI 节点。
- 回归/相关验证：新增 core 测试覆盖 `DirectoryScanner` 分批读取、隐藏文件过滤和计数；GUI/MIME/core 现有测试全部通过。

## 6. 后续事项（按需）

- 技术债：搜索仍是递归完成后一次性返回结果，当前只把结果显示和装饰拆开；如搜索大树也卡，需要再做搜索流式化。
- 后续建议：准备包含数千到数万条目的本地目录，在 X11 和 Wayland 下确认首屏速度、滚动流畅度、图标装饰渐进替换和框选命中。
- 关联文档/提交：本次未提交；默认不自动提交。
