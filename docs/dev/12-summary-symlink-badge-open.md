# 软链接角标与打开行为开发结论摘要

> 文档元数据
> - 文件编号：12
> - 文档类型：summary
> - 文件路径：docs/dev/12-summary-symlink-badge-open.md
> - 文档版本：v1.0.0
> - 完成日期：2026-06-25
> - 关联需求：文件/文件夹软链接显示角标，断链显示断开角标，双击有效软链接打开目标，双击断链弹窗提示。
> - 关联调研：docs/dev/12-research-symlink-badge-open.md
> - 关联计划：docs/dev/12-plan-symlink-badge-open.md

## 1. 最终结果

- 原始需求：软链接文件和文件夹需要右上角角标，断链使用断开角标；双击有效软链接打开目标，双击断链弹框提示损坏。
- 最终方案：core 扫描时记录软链接目标类型、断链状态和 canonical path；GUI 用有效目标类型决定图标、菜单分类和打开行为；图标视图和列表视图都叠加角标；断链打开或打开方式入口显示阻塞弹窗。
- 完成状态：完成。
- 需求变更：无。

## 2. 关键改动

- 修改文件：`crates/filesystem-core/src/lib.rs`、`crates/filesystem-gui/src/app.rs`、`components.rs`、`icons.rs`、`model.rs`、`utils.rs`、`icons/symbol.svg`、`icons/symbol-disconnect.svg`、产品/开发总览和 12 号任务文档。
- 代码逻辑改动：`FileEntry` 增加 `symlink_target`；有效目录软链接排序、菜单和双击按目录处理，有效文件软链接按文件处理；断链显示 `BrokenSymlink` 角标并弹出“软连接 xxx 损坏”；文件/目录属性读取跟随有效软链接目标。
- 影响的使用场景：目录浏览、图标视图、列表视图、右键菜单分类、双击打开、打开方式、属性页。
- 不影响的使用场景：复制/剪切/重命名/删除仍作用于软链接路径本身；搜索不递归跟随目录软链接。
- 计划偏差：无。

## 3. 安全门禁结果

| 项 | 结论 |
|----|------|
| 风险矩阵 | L3 |
| 命令权限 | C0/C1 |
| 高风险项 | 有，涉及文件系统符号链接和 GUI 打开关键链路 |
| 破坏性操作 | 无 |
| 用户已有修改 | 有，基于已提供未跟踪角标 SVG 增量接入，未回退无关修改 |
| 用户确认事项 | 无 |
| 副作用/风险 | 权限不可访问的软链接目标会按断链提示；真实 GUI 视觉需人工确认 |

## 4. 验证结果

- 验证环境：本地 Linux workspace，Rust workspace。
- 系统信息（按需）：不适用。
- 执行验证：`cargo fmt --check`；`cargo test -p filesystem-core`；`cargo test -p filesystem-mime`；`cargo check -p filesystem-gui`；`cargo test -p filesystem-gui`；`make test`；`git diff --check`。
- 结果：全部通过。`make test` 覆盖 core 26 个、GUI 30 个、MIME 10 个单元测试，GUI 编译通过。
- 未执行验证项：未真实启动 GUI 检查角标视觉和 X11/Wayland 双击行为。
- 残余风险：不同文件系统对权限错误和 canonical path 的表现可能不同；不可访问目标统一按断链处理。

## 5. Bug 修复验证（按需）

- 原问题复现：不适用。
- 证据等级：不适用。
- 根因：不适用。
- 回归/相关验证：新增 core 测试覆盖文件软链接、目录软链接、断链和属性跟随；新增 GUI 图标测试覆盖普通链接和断链角标。

## 6. 后续事项（按需）

- 技术债：后续如果需要专门展示“软链接自身属性”，应新增独立属性页模式，不复用当前目标属性读取。
- 后续建议：人工准备有效目录软链接、有效文件软链接和断链，在 X11/Wayland 会话确认角标位置、菜单分类、双击目标路径和断链弹窗。
- 关联文档/提交：本次未提交；默认不自动提交。
