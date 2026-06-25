# File 开发概览

> 文档元数据
> - 文档版本：v1.0.0
> - 最后更新：2026-06-25
> - 更新来源：docs/dev/1-research-cosmic-files.md、docs/dev/1-plan-local-linux-file-manager.md、docs/dev/1-summary-local-linux-file-manager.md、docs/dev/2-summary-context-menu-file-ops-properties.md
> - 关联产品文档：docs/overview-product.md

## 1. 技术栈

| 类别 | 技术/版本 | 用途 | 备注 |
|------|-----------|------|------|
| 语言 | Rust，edition 2024 | 核心逻辑和 GUI | workspace `rust-version` 为 1.85 |
| 构建系统 | Cargo workspace + Makefile | 多 crate 构建与 feature 管理 | `make` 封装常用构建/测试入口 |
| 运行平台 | Linux 本地文件系统 | 目标运行平台 | 不支持网络文件系统服务集成 |
| 关键依赖 | `iced 0.14`、`resvg 0.45`、`regex 1`、`libc 0.2` | GUI、窗口事件、SVG 窗口图标渲染、文件名正则搜索、Linux 文件系统容量查询 | `iced` 关闭 default features；启用 `advanced` 仅用于访问 text wrapping 类型；`resvg` 复用 iced SVG 渲染路径；`regex` 和 `libc` 只在 core 中使用 |
| 渲染 | `iced/wgpu` | GPU 渲染路径 | 固定启用，不保留 `tiny-skia` |
| 窗口后端 | `iced/x11`、`iced/wayland` | 同一二进制支持 X11/Wayland | 由 winit 运行时选择 |

## 2. 架构边界

- 模块划分：
  - `crates/filesystem-core`：本地文件系统模型、只读扫描/搜索 API、首批写操作 API、剪贴板文本路径解析、当前文件夹属性统计和权限修改 API，不依赖 GUI。
  - `crates/filesystem-gui`：iced 图形入口，使用无系统边框窗口，持有当前目录、条目列表、访问历史栈、侧边栏导航、可编辑地址栏、视图模式、菜单状态、右键菜单状态、内联重命名状态、属性弹窗状态、属性弹窗位置/拖拽/权限编辑状态、选中路径集合、框选拖拽状态、主文件区宽度、窗口尺寸、窗口控制/缩放消息和显示状态。
- 进程/线程边界：当前只运行单 GUI 进程；目录扫描、文件名正则搜索、新建、重命名、粘贴复制/移动、当前文件夹属性统计、当前文件夹权限保存和终端探测/启动通过 iced `Task::perform` 交给 `thread-pool` executor 执行，UI 线程只处理状态更新和渲染；后续删除等可能阻塞 UI 的文件操作必须沿用后台任务模型。
- 客户端/服务端/驱动边界：无服务端、无内核模块、无桌面服务客户端。
- 数据流：GUI 状态发起后台 `scan_dir` 或 `search_file_names` 任务，core 返回 `DirectoryListing` 或 `SearchResults`，GUI 后台装饰步骤把条目转换为带 `EntryIcon` 的 `DisplayEntry` 后再发送完成消息；GUI 收到完成消息后渲染条目或错误状态；每个后台请求带自增 ID，过期结果会被丢弃。
- 控制流：启动时把 `icons/fs.svg` 渲染为 128x128 RGBA 窗口 icon，设置 Linux application_id 为 `File`，并把窗口最小尺寸设置为 800x600；GUI 订阅 `window::resize_events()`，窗口大小变化后更新主文件区宽度和窗口尺寸，并把属性弹窗位置钳制在可见范围内；用户双击目录、点击侧边栏主文件夹/根目录/家目录常见路径或在地址栏输入绝对路径回车后，成功切换目录会写入后退栈并清空前进栈；右侧主区域条目单击会清空旧选择并更新 `selected_paths` 为单个路径，双击才发送打开消息；从文件视图空白区域按下并拖拽时记录 `SelectionDrag`，结合当前 `ViewMode` 的布局公式和 `scrollable` 绝对滚动偏移计算命中的条目矩形，并更新 `selected_paths` 为多个路径；文件视图空白处右键打开 `context_menu` 覆盖层，菜单点击后发起后台新建/粘贴/终端/属性任务或同步执行全选；新建成功后设置 `RenameState`，其中保存 `text_editor::Content` 并对默认名称执行 `SelectAll`，图标视图和列表视图主体仍按普通条目固定布局渲染；`rename_overlay` 通过 `stack` 覆盖在文件视图上层，根据 `entry_content_rect` 和当前滚动偏移定位固定 widget id 的 `text_editor`，目录刷新完成后通过 iced widget operation 聚焦；重命名 editor 拦截 Enter 作为提交，不插入换行，按 1.5 倍行高渲染，宽度随字符估算最多扩展到基础宽度 3 倍，继续输入后使用 `Wrapping::WordOrGlyph` 换行并按估算行数增高；编辑框动态宽高不参与图标网格或列表行布局测量，因此不会挤压其它文件/文件夹位置；点击文件视图/工具栏/侧边栏等外部区域时统一调用后台 `rename_entry`，空名保持编辑状态，名称未变则退出编辑；粘贴先用 `iced::clipboard::read()` 读取标准剪贴板文本，再通过 core `parse_clipboard_paths` 解析本地路径并后台调用 `paste_paths`；属性弹窗打开后后台调用 `folder_properties`，结果以概要和权限页展示；属性弹窗使用全窗口 `mouse_area` 事件层阻止点击、右键和滚轮事件落到底层文件视图，标题区域拖拽更新弹窗坐标，关闭按钮复用 `icons/close.svg` 和 `style::close_button`；权限页维护待保存 mode，owner/group/other 访问行只修改本地待保存状态，点击“取消”恢复为当前属性 mode，点击“更改”后后台调用 `set_permissions` 并重新读取 `folder_properties`；地址栏输入不是绝对路径时，按正则在当前目录树递归搜索文件/目录名并把匹配项渲染到主区域；目录加载、搜索和文件主题图标解析都在后台执行；后退/前进按钮从对应历史栈切换路径并维护反向栈；隐藏文件开关位于地址栏右侧菜单中，在目录模式下重新扫描当前路径，在搜索模式下用当前正则重新搜索；地址栏右侧菜单切换 `ViewMode`，图标视图按主文件区宽度和固定 tile 尺寸计算列数后流式渲染网格，列表视图渲染名称、大小、所有者、修改时间列，两个视图都渲染条目图标；主内容区域用 `stack` 叠加透明菜单层，菜单和子菜单覆盖在文件视图上方，不参与工具栏/文件视图 column 排版；窗口拖拽/关闭/最小化/最大化通过 iced `window` task 执行；四边和四角 resize 命中区调用 `window::drag_resize`，由窗口管理器接管实际缩放。
- GUI 右键菜单规则：`BrowserRightPressed` 同时判断 `selected_paths` 是否为空和指针是否命中条目；当前无选中项时，条目右键等价于空白处右键；`context_menu_overlay` 使用主文件区宽度钳制菜单 x 坐标并保持固定宽度，菜单项内容水平左对齐、垂直居中。
- 外部依赖：允许后续配置外部二进制，但不能依赖 DBus/GVFS/portal/XDG MIME/通知/桌面配置服务。

## 3. 关键接口

| 接口/协议/ABI | 调用方 | 提供方 | 兼容约束 | 说明 |
|---------------|--------|--------|----------|------|
| `scan_dir(path, ScanOptions)` | `filesystem-gui` | `filesystem-core` | 只读；不跟随符号链接判断类型；条目元数据包含大小、UID 所有者和修改时间 | 返回排序后的本地目录条目 |
| `search_file_names(root, query, ScanOptions)` | `filesystem-gui` | `filesystem-core` | 只读；按 Rust `regex` 语法匹配文件/目录名；不跟随符号链接目录递归；结果条目保留元数据 | 返回当前目录树下排序后的匹配项 |
| `create_file` / `create_folder` / `rename_entry` | `filesystem-gui` | `filesystem-core` | 只作用于当前目录；重命名不覆盖已有路径；新建自动选择唯一名称；GUI 空名不提交，名称未变只退出编辑；文件名过长错误可识别 | 支持右键菜单新建和内联重命名 |
| `child_path_limits(parent)` | `filesystem-gui` | `filesystem-core` | 使用 Linux `pathconf` 查询当前目录 `_PC_NAME_MAX` 和 `_PC_PATH_MAX`；查询不到的限制返回 `None` | 支持重命名编辑时阻止超长名称/路径 |
| `paste_paths(sources, destination, action)` | `filesystem-gui` | `filesystem-core` | 复制递归复制文件/目录；移动首版只使用 `fs::rename`；目标存在时自动唯一命名 | 支持从文本剪贴板解析出的本地路径粘贴 |
| `parse_clipboard_paths(contents)` | `filesystem-gui` | `filesystem-core` | 只解析标准剪贴板文本中的绝对路径、`file://` URI、`copy`/`cut`/`move` 标记 | 不读取专用剪贴板 MIME target |
| `folder_properties(path)` | `filesystem-gui` | `filesystem-core` | 后台递归统计当前文件夹条目数和文件大小；使用 Linux `statvfs` 查询剩余空间 | 支持属性弹窗查看 |
| `set_permissions(path, mode)` | `filesystem-gui` | `filesystem-core` | mode 必须在 `0o0000..=0o7777`；只修改当前路径权限，不递归修改内容、不修改 owner/group/ACL | 支持属性弹窗权限页修改当前文件夹权限 |
| `Task::perform(...)` 后台任务 | `filesystem-gui` | `iced` thread-pool executor | 所有可能阻塞 UI 的 I/O、复制、移动、删除都必须通过后台任务发起 | UI 线程不直接执行耗时文件系统操作 |
| `iced` feature 集 | 构建者 | `filesystem-gui` | 固定启用 `advanced`、`thread-pool`、`svg`、`wgpu`、`x11`、`wayland` | wgpu 渲染、SVG 图标、text wrapping 类型和双窗口后端 |
| `load_window_icon()` | `filesystem-gui` | `resvg`/`tiny-skia` | 直接依赖不额外启用 `resvg` default features；输出非预乘 RGBA | 把 `icons/fs.svg` 转换为 iced/winit 窗口 icon |
| `IconResolver` | `filesystem-gui` 后台任务 | 本地图标主题文件 | 只查找并读取本地 SVG 图标文件，不调用桌面服务；找不到就回退本地 `icons/file.svg` | 根据扩展名映射常见 MIME 图标名，完成消息携带已读入内存的 SVG |

## 4. 数据与配置

- 核心数据结构：
  - `EntryKind`：`Directory`、`File`、`Symlink`、`Other`。
  - `FileEntry`：名称、路径、类型、隐藏状态、大小、UID 所有者、修改时间。
  - `DirectoryListing`：当前路径和条目列表。
  - `SearchResults`：搜索根路径、关键词和结果条目列表。
  - `DisplayEntry`/`EntryIcon`：GUI 层显示条目和图标来源，图标来源为编译期 SVG 或后台读入内存的本地图标主题 SVG。
  - `ClipboardPaths`/`PasteAction`：从剪贴板文本解析出的粘贴动作和本地路径列表。
  - `FolderProperties`：当前文件夹属性弹窗所需路径、名称、父目录、递归条目数、总大小、剩余空间、UID/GID、mode、修改时间和创建时间。
  - `ScanOptions`：当前仅包含 `show_hidden`。
- 配置文件/参数：暂无持久配置。
- 持久化数据：暂无。
- 编译期资源：窗口控制按钮使用 `icons/min.svg`、`icons/max.svg`、`icons/close.svg`；地址栏历史按钮使用 `icons/left.svg`、`icons/right.svg`；菜单按钮使用 `icons/menu.svg`；窗口/任务栏图标使用 `icons/fs.svg`；条目回退图标使用 `icons/folder.svg` 和 `icons/file.svg`；侧边栏导航使用 `icons/home.svg`、`icons/root.svg`、`icons/download.svg`、`icons/picture.svg`、`icons/desktop.svg`、`icons/document.svg`、`icons/music.svg`、`icons/videos.svg`；这些 SVG 均通过 `include_bytes!` 编入 GUI 二进制。
- 外框圆角：不使用透明窗口、内容裁剪或自绘方式模拟；只有 iced/winit 在 Linux 暴露窗口管理器原生圆角接口后才设置。
- 迁移/兼容规则：暂无。
- 敏感信息处理：不读取系统账号密钥；路径和错误只显示在本地 GUI。

## 5. 高风险区域

| 风险区域 | 关注点 | 验证方式 | 关联文档 |
|----------|--------|----------|----------|
| 文件系统写操作 | 复制、移动、删除、覆盖冲突、失败清理 | 后续只能在临时目录集成测试中逐步启用 | docs/dev/1-plan-local-linux-file-manager.md |
| 权限/系统调用 | 不可读目录、符号链接、特殊文件 | 当前覆盖缺失路径和符号链接；权限专项待补 | docs/dev/1-summary-local-linux-file-manager.md |
| GUI 后端 | X11/Wayland 会话差异 | 构建已验证；真实开窗 smoke test 待补 | docs/dev/1-summary-local-linux-file-manager.md |
| 文件名正则递归搜索 | 后台任务会避免阻塞 UI，但大目录仍可能占用线程池；无效正则会返回 `InvalidInput` 错误 | 当前覆盖递归正则命中、隐藏过滤、空关键词和无效正则；大目录与权限专项待补 | docs/dev/1-summary-local-linux-file-manager.md |
| 首批写操作 | 新建、重命名、粘贴复制/移动、当前文件夹权限修改必须避免覆盖/越界、限制测试范围，并保持 UI 状态一致 | core 临时目录测试覆盖唯一命名、防覆盖重命名、文件名过长识别、目录路径限制查询、递归复制、同文件系统移动、剪贴板文本解析和权限 mode 修改 | docs/dev/2-plan-context-menu-file-ops-properties.md；docs/dev/3-summary-properties-permission-edit.md |
| 属性统计 | 递归统计大目录可能耗时，权限错误可能中断统计 | 后台任务执行；当前测试覆盖临时目录条目数和大小统计 | docs/dev/2-plan-context-menu-file-ops-properties.md |
| 外部命令 | shell 注入、缺命令降级、参数传递 | 终端启动已按 PATH 顺序直接传 argv 调用；文件打开/压缩等后续外部命令仍需 mock 命令测试 | docs/dev/2-plan-context-menu-file-ops-properties.md |
| 渲染后端 | wgpu 驱动栈复杂度；Wayland CSD 传递依赖 `tiny-skia` | 默认构建使用 wgpu；`tiny-skia` renderer 已移除；需真实图形会话验证 | docs/dev/1-plan-local-linux-file-manager.md |

## 6. 构建与验证

- 构建命令：
  - Release：`make` 或 `make release`
  - Debug：`make debug`
  - 默认 GUI 快速检查：`cargo check -p filesystem-gui`
- 单元测试：
  - `make test`
  - `cargo test -p filesystem-core`
  - `cargo test -p filesystem-gui`
  - `cargo test`
- 静态检查：
  - `cargo fmt --check`
  - 后续可加入 `cargo clippy`
- 高风险验证：
  - 首批写操作测试只作用于测试创建的临时目录；权限修改测试只修改临时目录 mode；后续删除等破坏性操作仍必须只在临时目录或明确授权范围内验证。
- 最小人工验证步骤：
  - 在 X11 会话启动 GUI，确认窗口无系统边框，最小尺寸不能小于 800x600，侧栏顶部和地址栏右侧空白区可拖拽，双击可最大化/还原，窗口按钮可关闭/最小化/最大化，四边和四角可拖动缩放，侧边栏、后退/前进按钮、可编辑地址栏、靠近最小化按钮的菜单按钮、视图菜单、图标视图和列表视图渲染正常，拉宽窗口时图标视图增加每行条目、缩窄窗口时列尾条目换到下一行且文件/文件夹图标尺寸不变，打开菜单/子菜单时文件视图条目位置不移动，菜单内隐藏文件开关可用，列表视图显示名称/大小/所有者/修改时间，两个视图都显示文件夹和文件图标，主区域条目单击只选中、空白区域拖拽可框选多个条目、双击才进入目录或触发文件打开提示，空白区右键菜单不移动文件布局，新建文件/文件夹后默认名称全选并可内联重命名，重命名编辑框作为覆盖层显示，动态变宽/变高时不挤压其它条目，至少 1.5 倍行高，长名称最多扩宽 3 倍后换行增高，回车或点击文件视图/工具栏/侧边栏等外部区域可完成重命名，粘贴文本路径可复制到当前目录，全选可选中当前条目，在终端打开按可用终端启动，属性弹窗能显示当前文件夹概要和权限页，目录可进入并可通过后退/前进访问历史位置，地址栏可输入绝对路径跳转或输入非绝对路径正则搜索当前目录树文件名。
  - 属性弹窗打开后，确认点击/右键/滚轮不会触发底层文件视图选择、框选或滚动；标题区域可拖拽移动；关闭按钮图标和 hover 效果与主窗口关闭按钮一致；概要和权限页的信息左对齐、垂直居中；权限页没有“更改内容文件的权限”入口，修改当前文件夹权限后可取消或保存并刷新显示。
  - 在任一图形会话中确认当前无选中项时条目右键等价空白处菜单，右键菜单靠近主文件区右边界时不被压缩，菜单项水平左对齐且垂直居中。
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
  - docs/dev/2-summary-context-menu-file-ops-properties.md

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
| 2026-06-24 | 记录 `search_file_names` 文件名正则递归搜索 API 和地址栏搜索控制流 | 更新 core/GUI 接口事实 | docs/dev/1-summary-local-linux-file-manager.md |
| 2026-06-24 | 记录目录加载和文件名正则搜索使用 iced 后台任务，后续阻塞型文件操作不得在 UI 线程执行 | 更新 GUI 线程边界 | docs/dev/1-summary-local-linux-file-manager.md |
| 2026-06-24 | 记录 `ViewMode` 视图切换菜单、列表视图列和 `FileEntry.owner` UID 元数据 | 更新 GUI 状态与 core 数据事实 | docs/dev/1-summary-local-linux-file-manager.md |
| 2026-06-24 | 记录 `IconResolver` 后台主题图标解析、菜单图标和文件/文件夹回退图标资源 | 更新 GUI 资源与线程边界 | docs/dev/1-summary-local-linux-file-manager.md |
| 2026-06-25 | 记录 `selected_path` 状态、主区域单击选中/双击打开，以及隐藏文件开关移入菜单 | 更新 GUI 交互控制流 | docs/dev/1-summary-local-linux-file-manager.md |
| 2026-06-25 | 记录菜单使用透明 `stack` 覆盖层，不参与文件视图布局 | 更新 GUI 布局控制流 | docs/dev/1-summary-local-linux-file-manager.md |
| 2026-06-25 | 记录文件视图 `SelectionDrag` 框选状态和多选命中计算 | 更新 GUI 选择控制流 | docs/dev/1-summary-local-linux-file-manager.md |
| 2026-06-25 | 记录窗口最小尺寸、resize 订阅和流式图标列数计算 | 更新 GUI 布局控制流 | docs/dev/1-summary-local-linux-file-manager.md |
| 2026-06-25 | 记录右键菜单、core 首批写操作、文本剪贴板粘贴、属性统计和终端启动 | 更新 GUI 与 core 控制流 | docs/dev/2-plan-context-menu-file-ops-properties.md |
| 2026-06-25 | 记录新建后重命名输入框固定 id、默认全选和外部点击提交控制流 | 更新 GUI 交互控制流 | docs/dev/2-summary-context-menu-file-ops-properties.md |
| 2026-06-25 | 记录重命名编辑框切换到 `text_editor`、1.5 倍行高、最多 3 倍宽度和 wrapping 控制流 | 更新 GUI 交互控制流 | docs/dev/2-summary-context-menu-file-ops-properties.md |
| 2026-06-25 | 记录重命名编辑框覆盖层定位和不参与文件布局测量 | 更新 GUI 布局控制流 | docs/dev/2-summary-context-menu-file-ops-properties.md |
| 2026-06-25 | 记录 `child_path_limits` 和重命名超长输入保护 | 更新 core/GUI 接口事实 | docs/dev/2-summary-context-menu-file-ops-properties.md |
| 2026-06-25 | 记录右键菜单无选中项语义、固定宽度钳制和菜单项对齐 | 更新 GUI 交互与布局控制流 | docs/dev/2-summary-context-menu-file-ops-properties.md |
| 2026-06-25 | 记录 `set_permissions`、属性弹窗事件隔离/拖拽和权限编辑后台保存控制流 | 更新 core/GUI 接口与交互控制流 | docs/dev/3-summary-properties-permission-edit.md |
