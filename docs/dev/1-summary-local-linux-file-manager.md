# Linux 本地图形文件管理器首版实现摘要

> 文档元数据
> - 文件编号：1
> - 文档类型：summary
> - 文件路径：docs/dev/1-summary-local-linux-file-manager.md
> - 文档版本：v1.0.0
> - 完成日期：2026-06-24
> - 关联需求：仅支持 Linux 本地文件系统，图形界面 only，允许外部二进制依赖，但完全不依赖系统桌面服务的 Rust 文件管理器。
> - 关联调研：docs/dev/1-research-cosmic-files.md
> - 关联计划：docs/dev/1-plan-local-linux-file-manager.md

## 1. 最终结果

- 原始需求：基于 cosmic-files 调研结论，开始实现一个更少依赖、GUI only、Linux 本地文件系统的 Rust 文件管理器。
- 最终方案：新建独立 Rust workspace，拆分 `filesystem-core` 与 `filesystem-gui`；core 做本地目录只读扫描和文件名正则搜索，GUI 直接使用 `iced` 和 `wgpu`；界面采用无系统边框暗色文件管理器布局。
- 完成状态：部分完成。已完成最小只读浏览骨架、当前目录树文件名正则搜索、800x600 最小窗口、流式图标视图、列表视图、单击选中和框选多选；写操作、外部二进制适配和真实图形会话 smoke test 待后续。
- 需求变更：用户反馈 tiny-skia 版本界面卡顿，要求改用 wgpu 并去掉 tiny-skia；`iced` 0.14 关闭 default features 后必须显式启用 executor，本次选择 `iced/thread-pool`。

## 2. 关键改动

- 修改文件：
  - `Cargo.toml`
  - `Cargo.lock`
  - `Makefile`
  - `.gitignore`
  - `crates/filesystem-core/Cargo.toml`
  - `crates/filesystem-core/src/lib.rs`
  - `crates/filesystem-gui/Cargo.toml`
  - `crates/filesystem-gui/src/main.rs`
  - `docs/overview-product.md`
  - `docs/overview-product-dev.md`
  - `docs/dev/README.md`
  - `docs/dev/1-plan-local-linux-file-manager.md`
  - `docs/dev/1-summary-local-linux-file-manager.md`
- 代码逻辑改动：
  - `filesystem-core` 提供 `scan_dir` 和 `search_file_names`，使用 `std::fs::read_dir`、`symlink_metadata` 和 `regex`，按目录、符号链接、文件、其他类型排序，支持隐藏文件过滤和当前目录树文件名正则搜索。
  - `filesystem-gui` 提供 iced 窗口，支持进入目录、历史后退/前进和显示隐藏文件。
  - GUI 已从简单列表升级为参考截图风格的暗色界面：无系统边框窗口、左侧本地导航、顶部后退/前进按钮、可编辑地址栏、视图菜单、主区域文件视图、底部状态栏。
  - 地址栏右侧增加菜单按钮；菜单按钮使用 `icons/menu.svg`，靠近最小化按钮并保留间隔；菜单内包含“视图”子菜单，可选择“图标视图”和“列表视图”。
  - 菜单和视图子菜单作为覆盖层显示在文件视图上方，不再插入主布局流，因此打开菜单不会推动图标/列表位置。
  - 隐藏文件复选框已从工具栏移入地址栏右侧菜单。
  - 列表视图显示名称、大小、所有者、修改时间；所有者使用 Linux UID，不读取桌面服务或系统 MIME/portal 信息。
  - 图标视图和列表视图均显示条目图标；文件夹使用 `icons/folder.svg`，文件按扩展名优先查找本地图标主题中的 SVG MIME 图标，找不到时使用 `icons/file.svg`。
  - 窗口最小尺寸为 800x600；图标视图保存主文件区宽度并按固定 tile 尺寸计算列数，窗口变宽时增加每行条目，变窄时把列尾条目换到下一行，不缩放文件/文件夹图标。
  - 侧边栏只显示“主文件夹”“根目录”，以及家目录中实际存在的下载/图片/桌面/文档/音乐/视频等常见目录；导航项内容左对齐、垂直居中。
  - 侧边栏导航图标改为指定 SVG 资源：`icons/home.svg`、`icons/root.svg`、`icons/download.svg`、`icons/picture.svg`、`icons/desktop.svg`、`icons/document.svg`、`icons/music.svg`、`icons/videos.svg`。
  - 参考 cosmic-files 模式补充窗口拖拽、双击最大化/还原、最小化、最大化/还原和关闭操作；底层使用 iced `window` task。
  - 侧边栏顶部改为空白拖拽区，导航按钮下移；地址栏改为可编辑输入框，支持回车访问绝对路径。
  - 窗口控制按钮使用本地 SVG 资源：`icons/min.svg`、`icons/max.svg`、`icons/close.svg`，按钮 padding 为 6px，图标居中。
  - 窗口四边和四角加入 6px resize 命中区，调用 iced `window::drag_resize`，实际缩放交给窗口管理器处理。
  - 外框圆角不使用透明窗口或内容裁剪模拟；当前 Linux iced/winit 未提供窗口管理器原生圆角设置接口，因此暂不设置外框圆角。
  - 应用英文名为 `File`，中文名为 `文件`；窗口标题和 Linux application_id 使用 `File`，左侧拖拽标题栏显示 `文件`。
  - 窗口/任务栏图标使用 `icons/fs.svg`，启动时通过 `resvg` 渲染为 128x128 RGBA 后传给 iced/winit。
  - 地址栏左侧刷新按钮已移除，改为后退/前进两个 SVG 按钮：`icons/left.svg`、`icons/right.svg`；按钮无可用历史时置灰且不可点。
  - 成功进入目录、侧边栏跳转或地址栏跳转时写入后退历史并清空前进历史；后退/前进会维护反向历史栈。
  - 主区域文件和文件夹单击只更新选中状态；从空白区域拖拽会显示半透明框选矩形并选中命中的多个条目；双击才进入目录或触发普通文件打开提示。
  - 地址栏回车时只有绝对路径按路径跳转；所有非绝对路径输入都作为正则表达式在当前目录树搜索文件/目录名，搜索结果在主区域列出，图标视图副标题显示相对路径，列表视图名称列显示相对路径；隐藏文件开关会重新扫描或重新执行当前搜索。
  - `FileEntry` 增加 `owner` 字段，扫描目录和搜索结果都会保留 UID 所有者元数据，供列表视图展示。
  - 目录扫描、文件名正则搜索和主题图标解析改为 iced `Task::perform` 后台任务；GUI 使用自增请求 ID 丢弃过期结果，避免旧后台结果覆盖新导航/搜索状态。
  - GUI 依赖固定启用 `iced/wgpu`、`iced/x11`、`iced/wayland`、`iced/thread-pool`、`iced/svg`，不再保留 renderer 互斥 feature；`resvg` 仅用于窗口 icon SVG 渲染，直接依赖不额外启用 default features。
  - 根目录新增 `Makefile`：`make`/`make release` 编译 release，`make debug` 编译 debug，`make test` 执行 workspace 测试。
- 影响的使用场景：可以从当前工作目录启动 GUI，以图形文件管理器布局浏览本地目录，单击选择条目、框选多个条目、双击进入目录，在流式图标视图和列表视图之间切换，并通过地址栏正则搜索当前目录树文件名。
- 不影响的使用场景：不会调用 DBus/GVFS/portal/XDG MIME/通知/桌面配置服务；不会执行删除、移动、复制或打开文件动作。
- 计划偏差：权限错误测试未做专项覆盖；未在真实 X11/Wayland 会话开窗测试。

## 3. 安全门禁结果

| 项 | 结论 |
|----|------|
| 风险矩阵 | L4，新系统且长期涉及文件系统写操作；本轮只落只读骨架 |
| 命令权限 | C1，执行本地读写源码、Cargo 构建和测试；Cargo 依赖解析成功，没有额外权限提升 |
| 高风险项 | 文件系统操作被限制为只读扫描/文件名正则搜索；测试删除仅作用于由测试创建的 `/tmp/filesystem-core-test-*` 临时目录 |
| 破坏性操作 | 无真实用户文件破坏性操作 |
| 用户已有修改 | 工作区原有变更仅为未跟踪 `docs/`；本次在该文档体系内继续更新，没有回退用户修改 |
| 用户确认事项 | 已确认 GUI only、Linux 本地文件系统；渲染策略已从 tiny-skia 默认改为 wgpu only |
| 副作用/风险 | GUI 依赖 iced/winit/Wayland/X11 图形栈；真实图形会话兼容性尚未 smoke test |

## 4. 验证结果

- 验证环境：
  - 本地 Linux 工作区 `/data/code/filesystem`
  - Rust `rustc 1.93.1 (01f6ddf75 2026-02-11) (gentoo)`
- 执行验证：
  - `cargo fmt --check`：通过。
  - `cargo test -p filesystem-core`：通过，8 个单元测试，覆盖 UID 所有者元数据存在性。
  - `cargo check -p filesystem-gui`：通过，默认 wgpu。
  - `cargo test -p filesystem-gui`：通过，3 个单元测试覆盖流式图标列数和窗口宽度换算。
  - `cargo test`：通过。
  - `make debug`：通过。
  - `make test`：通过。
  - `make`：通过，生成 release 构建。
  - `git diff --check`：通过。
  - `cargo tree -p filesystem-core`：仅 `filesystem-core` 自身。
  - `cargo tree -p filesystem-gui -i tiny-skia`：`tiny-skia <- sctk-adwaita <- winit <- iced_winit <- iced`，说明剩余 tiny-skia 来自 Wayland 客户端装饰，不是 iced tiny-skia renderer。
  - `Cargo.lock` 禁用依赖名检索：`gio`、`glib`、`gvfs`、`zbus`、`ashpd`、`notify-rust`、`xdg-mime`、`cosmic-config` 均无命中。
- 结果：当前只读浏览、文件名正则搜索、列表视图依赖的元数据、条目图标数据流、菜单覆盖层布局、单击/双击消息路径、框选多选命中路径、流式图标列数和窗口宽度换算完成编译或单元测试验证；core 自动化验证通过。
- 未执行验证项：
  - 未在真实 X11 和 Wayland 会话分别启动 GUI。
  - 未做权限错误专项用例、大目录扫描、大目录文件名正则搜索性能测试和后台任务压力验证。
- 残余风险：
  - iced/winit/wgpu 会带来 GPU 驱动栈、Wayland/X11 协议和剪贴板相关依赖，但未引入已禁用的桌面服务 crate。
  - 文件名正则搜索当前已放入后台任务，不再阻塞 UI；但大目录搜索仍可能长时间占用线程池，后续需要取消、进度和限流策略。
  - 如果要求依赖树中完全不存在 `tiny-skia` crate，需要后续 patch/fork `iced_winit` 或关闭 Wayland；当前只移除了 iced tiny-skia renderer。
  - 后续写操作必须继续限制在临时目录测试，补齐复制、移动、删除、覆盖冲突和失败清理验证。

## 5. Bug 修复验证（按需）

- 不适用。本次是新功能骨架实现，不是 bug 修复。

## 6. Code 阶段审视

| 角色 | 结论 |
|------|------|
| 安全审查员 | 只读扫描/文件名正则搜索使用 `read_dir` 和 `symlink_metadata`，没有跟随符号链接执行写操作；测试清理路径限制为固定前缀临时目录；没有执行破坏性系统操作。 |
| 高级产品 | 本轮满足“可以开始实现”的最小闭环：已有 GUI 入口、本地目录浏览和接近常见文件管理器的暗色界面；未把外部命令、网络文件系统或桌面服务提前纳入。 |
| 高级架构师 | core 与 GUI 边界清晰；core 无外部依赖；renderer 通过 Cargo feature 隔离；需要后续把文件操作状态机放在 core/ops 层，避免 UI 直接执行复杂写操作。 |
| 高级工程师 | 实现保持小范围；窗口操作复用 iced 原生 task，目录扫描、搜索和主题图标解析通过后台任务执行；新增 `regex` 依赖用于用户要求的正则匹配；视图切换、条目选择和框选多选只影响 GUI 状态和渲染分支；错误路径能向 UI 展示；自动化验证覆盖当前只读 API。未覆盖真实开窗、权限错误、大目录性能和后台任务压力，是下一阶段风险。 |

## 7. 后续事项（按需）

- 技术债：
  - `filesystem-gui` 暂无 UI 自动化测试。
  - 文件图标主题解析只覆盖本地图标主题中的 SVG MIME 图标，未覆盖 PNG/XPM 等主题资源。
  - 权限错误专项测试未补。
- 后续建议：
  - 先补 GUI 状态层测试和真实 X11/Wayland smoke test。
  - 再实现只读预览、多选后的批量操作入口和文件打开器。
  - 写操作应从新建/重命名开始，并只在临时目录做集成测试。
  - 外部二进制调用必须直接传 argv，不通过 shell。
