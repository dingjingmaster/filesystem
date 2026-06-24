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
- 最终方案：新建独立 Rust workspace，拆分 `filesystem-core` 与 `filesystem-gui`；core 只做本地目录只读扫描，GUI 直接使用 `iced` 和 `wgpu`；界面采用无系统边框暗色文件管理器布局。
- 完成状态：部分完成。已完成最小只读浏览骨架；写操作、外部二进制适配、选择/多选和真实图形会话 smoke test 待后续。
- 需求变更：用户反馈 tiny-skia 版本界面卡顿，要求改用 wgpu 并去掉 tiny-skia；`iced` 0.14 关闭 default features 后必须显式启用 executor，本次选择 `iced/thread-pool`。

## 2. 关键改动

- 修改文件：
  - `Cargo.toml`
  - `Cargo.lock`
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
  - `filesystem-core` 提供 `scan_dir`，使用 `std::fs::read_dir` 和 `symlink_metadata`，按目录、符号链接、文件、其他类型排序，支持隐藏文件过滤。
  - `filesystem-gui` 提供 iced 窗口，支持进入目录、返回上级、刷新和显示隐藏文件。
  - GUI 已从简单列表升级为参考截图风格的暗色界面：无系统边框窗口、左侧本地导航、顶部返回/刷新/路径栏、主区域网格文件图标、底部状态栏。
  - 侧边栏只显示“主文件夹”“根目录”，以及家目录中实际存在的下载/图片/桌面/文档/音乐/视频等常见目录。
  - 参考 cosmic-files 模式补充窗口拖拽、双击最大化/还原、最小化、最大化/还原和关闭操作；底层使用 iced `window` task。
  - 侧边栏顶部改为空白拖拽区，导航按钮下移；地址栏改为可编辑输入框，支持回车访问绝对路径、相对路径、`~` 和 `~/...`。
  - 窗口控制按钮使用本地 SVG 资源：`icons/min.svg`、`icons/max.svg`、`icons/close.svg`，按钮 padding 为 6px，图标居中。
  - 窗口四边和四角加入 6px resize 命中区，调用 iced `window::drag_resize`，实际缩放交给窗口管理器处理。
  - 外框圆角不使用透明窗口或内容裁剪模拟；当前 Linux iced/winit 未提供窗口管理器原生圆角设置接口，因此暂不设置外框圆角。
  - GUI 依赖固定启用 `iced/wgpu`、`iced/x11`、`iced/wayland`、`iced/thread-pool`、`iced/svg`，不再保留 renderer 互斥 feature。
- 影响的使用场景：可以从当前工作目录启动 GUI，以图形文件管理器布局浏览本地目录。
- 不影响的使用场景：不会调用 DBus/GVFS/portal/XDG MIME/通知/桌面配置服务；不会执行删除、移动、复制或打开文件动作。
- 计划偏差：未实现选择/多选；权限错误测试未做专项覆盖；未在真实 X11/Wayland 会话开窗测试。

## 3. 安全门禁结果

| 项 | 结论 |
|----|------|
| 风险矩阵 | L4，新系统且长期涉及文件系统写操作；本轮只落只读骨架 |
| 命令权限 | C1，执行本地读写源码、Cargo 构建和测试；Cargo 依赖解析成功，没有额外权限提升 |
| 高风险项 | 文件系统操作被限制为只读扫描；测试删除仅作用于由测试创建的 `/tmp/filesystem-core-test-*` 临时目录 |
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
  - `cargo test -p filesystem-core`：通过，4 个单元测试。
  - `cargo check -p filesystem-gui`：通过，默认 wgpu。
  - `cargo test`：通过。
  - `git diff --check`：通过。
  - `cargo tree -p filesystem-core`：仅 `filesystem-core` 自身。
  - `cargo tree -p filesystem-gui -i tiny-skia`：`tiny-skia <- sctk-adwaita <- winit <- iced_winit <- iced`，说明剩余 tiny-skia 来自 Wayland 客户端装饰，不是 iced tiny-skia renderer。
  - `Cargo.lock` 禁用依赖名检索：`gio`、`glib`、`gvfs`、`zbus`、`ashpd`、`notify-rust`、`xdg-mime`、`cosmic-config` 均无命中。
- 结果：当前只读骨架自动化验证通过。
- 未执行验证项：
  - 未在真实 X11 和 Wayland 会话分别启动 GUI。
  - 未做权限错误专项用例和大目录性能测试。
- 残余风险：
  - iced/winit/wgpu 会带来 GPU 驱动栈、Wayland/X11 协议和剪贴板相关依赖，但未引入已禁用的桌面服务 crate。
  - 如果要求依赖树中完全不存在 `tiny-skia` crate，需要后续 patch/fork `iced_winit` 或关闭 Wayland；当前只移除了 iced tiny-skia renderer。
  - 后续写操作必须继续限制在临时目录测试，补齐复制、移动、删除、覆盖冲突和失败清理验证。

## 5. Bug 修复验证（按需）

- 不适用。本次是新功能骨架实现，不是 bug 修复。

## 6. Code 阶段审视

| 角色 | 结论 |
|------|------|
| 安全审查员 | 只读扫描使用 `read_dir` 和 `symlink_metadata`，没有跟随符号链接执行写操作；测试清理路径限制为固定前缀临时目录；没有执行破坏性系统操作。 |
| 高级产品 | 本轮满足“可以开始实现”的最小闭环：已有 GUI 入口、本地目录浏览和接近常见文件管理器的暗色界面；未把外部命令、网络文件系统或桌面服务提前纳入。 |
| 高级架构师 | core 与 GUI 边界清晰；core 无外部依赖；renderer 通过 Cargo feature 隔离；需要后续把文件操作状态机放在 core/ops 层，避免 UI 直接执行复杂写操作。 |
| 高级工程师 | 实现保持小范围；窗口操作复用 iced 原生 task，未引入额外依赖；错误路径能向 UI 展示；自动化验证覆盖当前只读 API。未覆盖真实开窗、权限错误和大目录性能，是下一阶段风险。 |

## 7. 后续事项（按需）

- 技术债：
  - `filesystem-gui` 暂无 UI 自动化测试。
  - 文件图标为 iced 组件绘制的轻量近似，不是独立图标主题。
  - 权限错误专项测试未补。
- 后续建议：
  - 先补 GUI 状态层测试和真实 X11/Wayland smoke test。
  - 再实现选择/多选和只读预览。
  - 写操作应从新建/重命名开始，并只在临时目录做集成测试。
  - 外部二进制调用必须直接传 argv，不通过 shell。
