# File 开发概览

> 文档元数据
> - 文档版本：v1.0.0
> - 最后更新：2026-06-24
> - 更新来源：docs/dev/1-research-cosmic-files.md、docs/dev/1-plan-local-linux-file-manager.md、docs/dev/1-summary-local-linux-file-manager.md
> - 关联产品文档：docs/overview-product.md

## 1. 技术栈

| 类别 | 技术/版本 | 用途 | 备注 |
|------|-----------|------|------|
| 语言 | Rust，edition 2024 | 核心逻辑和 GUI | workspace `rust-version` 为 1.85 |
| 构建系统 | Cargo workspace + Makefile | 多 crate 构建与 feature 管理 | `make` 封装常用构建/测试入口 |
| 运行平台 | Linux 本地文件系统 | 目标运行平台 | 不支持网络文件系统服务集成 |
| 关键依赖 | `iced 0.14`、`resvg 0.45` | GUI、窗口事件和 SVG 窗口图标渲染 | `iced` 关闭 default features；`resvg` 复用 iced SVG 渲染路径 |
| 渲染 | `iced/wgpu` | GPU 渲染路径 | 固定启用，不保留 `tiny-skia` |
| 窗口后端 | `iced/x11`、`iced/wayland` | 同一二进制支持 X11/Wayland | 由 winit 运行时选择 |

## 2. 架构边界

- 模块划分：
  - `crates/filesystem-core`：本地文件系统模型和只读扫描 API，不依赖 GUI 或外部 crate。
  - `crates/filesystem-gui`：iced 图形入口，使用无系统边框窗口，持有当前目录、条目列表、访问历史栈、侧边栏导航、可编辑地址栏、窗口控制/缩放消息和显示状态。
- 进程/线程边界：当前只运行单 GUI 进程；iced 需要 `thread-pool` executor feature，但本轮没有后台任务。
- 客户端/服务端/驱动边界：无服务端、无内核模块、无桌面服务客户端。
- 数据流：GUI 状态触发 `scan_dir`，core 返回 `DirectoryListing`，GUI 渲染条目或错误状态。
- 控制流：启动时把 `icons/fs.svg` 渲染为 128x128 RGBA 窗口 icon，并设置 Linux application_id 为 `File`；用户点击目录、侧边栏主文件夹/根目录/家目录常见路径或编辑地址栏回车后，成功切换目录会写入后退栈并清空前进栈；后退/前进按钮从对应历史栈切换路径并维护反向栈；隐藏文件开关只重新扫描当前路径；窗口拖拽/关闭/最小化/最大化通过 iced `window` task 执行；四边和四角 resize 命中区调用 `window::drag_resize`，由窗口管理器接管实际缩放。
- 外部依赖：允许后续配置外部二进制，但不能依赖 DBus/GVFS/portal/XDG MIME/通知/桌面配置服务。

## 3. 关键接口

| 接口/协议/ABI | 调用方 | 提供方 | 兼容约束 | 说明 |
|---------------|--------|--------|----------|------|
| `scan_dir(path, ScanOptions)` | `filesystem-gui` | `filesystem-core` | 只读；不跟随符号链接判断类型 | 返回排序后的本地目录条目 |
| `iced` feature 集 | 构建者 | `filesystem-gui` | 固定启用 `thread-pool`、`svg`、`wgpu`、`x11`、`wayland` | wgpu 渲染、SVG 图标和双窗口后端 |
| `load_window_icon()` | `filesystem-gui` | `resvg`/`tiny-skia` | 直接依赖不额外启用 `resvg` default features；输出非预乘 RGBA | 把 `icons/fs.svg` 转换为 iced/winit 窗口 icon |

## 4. 数据与配置

- 核心数据结构：
  - `EntryKind`：`Directory`、`File`、`Symlink`、`Other`。
  - `FileEntry`：名称、路径、类型、隐藏状态、大小、修改时间。
  - `DirectoryListing`：当前路径和条目列表。
  - `ScanOptions`：当前仅包含 `show_hidden`。
- 配置文件/参数：暂无持久配置。
- 持久化数据：暂无。
- 编译期资源：窗口控制按钮使用 `icons/min.svg`、`icons/max.svg`、`icons/close.svg`；地址栏历史按钮使用 `icons/left.svg`、`icons/right.svg`；窗口/任务栏图标使用 `icons/fs.svg`；侧边栏导航使用 `icons/home.svg`、`icons/root.svg`、`icons/download.svg`、`icons/picture.svg`、`icons/desktop.svg`、`icons/document.svg`、`icons/music.svg`、`icons/videos.svg`；这些 SVG 均通过 `include_bytes!` 编入 GUI 二进制。
- 外框圆角：不使用透明窗口、内容裁剪或自绘方式模拟；只有 iced/winit 在 Linux 暴露窗口管理器原生圆角接口后才设置。
- 迁移/兼容规则：暂无。
- 敏感信息处理：不读取系统账号密钥；路径和错误只显示在本地 GUI。

## 5. 高风险区域

| 风险区域 | 关注点 | 验证方式 | 关联文档 |
|----------|--------|----------|----------|
| 文件系统写操作 | 复制、移动、删除、覆盖冲突、失败清理 | 后续只能在临时目录集成测试中逐步启用 | docs/dev/1-plan-local-linux-file-manager.md |
| 权限/系统调用 | 不可读目录、符号链接、特殊文件 | 当前覆盖缺失路径和符号链接；权限专项待补 | docs/dev/1-summary-local-linux-file-manager.md |
| GUI 后端 | X11/Wayland 会话差异 | 构建已验证；真实开窗 smoke test 待补 | docs/dev/1-summary-local-linux-file-manager.md |
| 外部命令 | shell 注入、缺命令降级、参数传递 | 后续使用 argv 直接调用并配 mock 命令测试 | docs/dev/1-plan-local-linux-file-manager.md |
| 渲染后端 | wgpu 驱动栈复杂度；Wayland CSD 传递依赖 `tiny-skia` | 默认构建使用 wgpu；`tiny-skia` renderer 已移除；需真实图形会话验证 | docs/dev/1-plan-local-linux-file-manager.md |

## 6. 构建与验证

- 构建命令：
  - Release：`make` 或 `make release`
  - Debug：`make debug`
  - 默认 GUI 快速检查：`cargo check -p filesystem-gui`
- 单元测试：
  - `make test`
  - `cargo test -p filesystem-core`
  - `cargo test`
- 静态检查：
  - `cargo fmt --check`
  - 后续可加入 `cargo clippy`
- 高风险验证：
  - 写操作尚未实现；后续所有破坏性测试只能作用于测试创建的临时目录。
- 最小人工验证步骤：
  - 在 X11 会话启动 GUI，确认窗口无系统边框，侧栏顶部和地址栏右侧空白区可拖拽，双击可最大化/还原，窗口按钮可关闭/最小化/最大化，四边和四角可拖动缩放，侧边栏、后退/前进按钮、可编辑地址栏和网格视图渲染正常，目录可进入并可通过后退/前进访问历史位置。
  - 在 Wayland 会话启动同一个二进制，确认行为一致。

## 7. 发布与回滚

- 产物：Cargo binary `filesystem-gui`。
- 安装/部署方式：未定义。
- 配置变更：暂无。
- 升级步骤：未定义。
- 回滚步骤：回退本次新增 workspace 与文档即可回到空实现状态。
- 止损条件：若 iced/winit 运行时依赖超出无桌面服务边界，保留 core，替换 GUI 层。

## 8. 观测与排障

- 关键日志：暂无结构化日志。
- 指标/告警：暂无。
- 常见故障：
  - 没有 `DISPLAY` 或 `WAYLAND_DISPLAY` 时 GUI 无法启动。
  - 目录不可读时 GUI 显示 core 返回的错误。
  - wgpu 构建可能受 GPU 驱动栈影响。
- 排障入口：
  - 依赖边界：`cargo tree -p filesystem-core`、`cargo tree -p filesystem-gui`
  - 构建后端：`filesystem-gui` 固定启用 `iced/wgpu`

## 9. 文档索引

- 需求与任务索引：docs/dev/README.md
- 产品概览：docs/overview-product.md
- 按需片段模板：.dj-agent/fragments/
- 关键任务文档：
  - docs/dev/1-research-cosmic-files.md
  - docs/dev/1-plan-local-linux-file-manager.md
  - docs/dev/1-summary-local-linux-file-manager.md

## 10. 变更记录

| 日期 | 变更 | 影响 | 关联文档 |
|------|------|------|----------|
| 2026-06-24 | 创建开发概览，记录 workspace、依赖、feature 和验证入口 | 建立长期开发事实 | docs/dev/1-summary-local-linux-file-manager.md |
| 2026-06-24 | 记录 GUI 暗色文件管理器布局和本地导航控制流 | 更新 GUI 架构事实 | docs/dev/1-summary-local-linux-file-manager.md |
| 2026-06-24 | 记录无系统边框窗口和侧边栏常见家目录探测规则 | 更新 GUI 架构事实 | docs/dev/1-summary-local-linux-file-manager.md |
| 2026-06-24 | 记录自绘窗口控制消息和 iced window task 控制流 | 更新 GUI 架构事实 | docs/dev/1-summary-local-linux-file-manager.md |
| 2026-06-24 | 记录可编辑地址栏状态和路径解析控制流 | 更新 GUI 架构事实 | docs/dev/1-summary-local-linux-file-manager.md |
| 2026-06-24 | 记录四边/四角 `window::drag_resize` 和外框圆角不伪造策略 | 更新 GUI 架构事实 | docs/dev/1-summary-local-linux-file-manager.md |
| 2026-06-24 | 新增根目录 `Makefile`，记录 `make`、`make debug`、`make test` 项目级入口 | 更新构建与验证入口 | docs/dev/1-summary-local-linux-file-manager.md |
| 2026-06-24 | 记录应用名 `File`/`文件`、Linux application_id 和 `icons/fs.svg` 窗口 icon 渲染路径 | 更新 GUI 架构事实 | docs/dev/1-summary-local-linux-file-manager.md |
| 2026-06-24 | 记录后退/前进历史栈、按钮资源和刷新按钮移除 | 更新 GUI 控制流 | docs/dev/1-summary-local-linux-file-manager.md |
| 2026-06-24 | 记录侧边栏导航 SVG 资源和对齐规则 | 更新 GUI 资源事实 | docs/dev/1-summary-local-linux-file-manager.md |
