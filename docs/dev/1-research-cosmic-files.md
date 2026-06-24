# cosmic-files 调研报告

> 文档元数据
> - 文件编号：1
> - 文档类型：research
> - 文件路径：docs/dev/1-research-cosmic-files.md
> - 文档版本：v1.0.0
> - 最后更新：2026-06-24
> - 关联需求：基于 `/data/source/cosmic-files` 评估实现独立环境可运行、尽量少依赖的 Rust 文件管理器。

## 1. 问题与边界

- 问题描述：用户希望做一个独立环境可运行、依赖尽量少的文件管理器，计划使用 Rust，并先评估已有开源项目 `cosmic-files`。
- 调研目的：判断 `cosmic-files` 的依赖重量、模块边界、可复用部分，以及后续是否适合作为直接基础。
- 包含：项目结构、直接依赖、默认 feature、关键模块、系统集成、文件操作核心、风险和推荐路线。
- 不包含：构建验证、代码迁移、源码改造、依赖 vendor、UI 方案定稿。
- 非目标：不在本阶段实现新文件管理器，不修改 `/data/source/cosmic-files`。
- 禁止触碰范围：不写入 `/data/source/cosmic-files`；不执行删除、覆盖、安装依赖、网络下载或系统级操作。

## 2. 当前证据

- 现有实现/现状：
  - 源仓库：`/data/source/cosmic-files`
  - Git remote：`https://github.com/pop-os/cosmic-files.git`
  - 当前提交：`c653b69`，tag `epoch-1.1.0`
  - 许可证：GPL-3.0-only
  - 主程序 crate：`cosmic-files`
  - workspace 成员：`cosmic-files-applet`
  - `Cargo.lock` 包条目数量：901
  - `rust-toolchain.toml` 指定 Rust `1.93.0`
- 已知约束：
  - 默认 feature 包含 `desktop`、`gvfs`、`wayland`、`wgpu`、`notify`、`io-uring`、`bzip2`、`lzma-rust2`、`dbus-config`。
  - UI 和配置强依赖 `libcosmic`、COSMIC/iced 体系。
  - 远程位置和挂载依赖可选 `gio`/`glib`/GVFS。
  - MIME 应用、缩略图、最近文件、DBus applet、桌面图标等均偏 Linux desktop/freedesktop 集成。
  - 锁文件含多个 Git 依赖，包括 `libcosmic`、`cosmic-mime-apps`、`trash-rs` cosmic branch、`xdg-mime-rs`、COSMIC fork 的 `winit`/clipboard 相关库。
- 关键模块：
  - `src/app.rs`：COSMIC 应用主体、动作调度、窗口/订阅/通知/菜单。
  - `src/tab.rs`：目录扫描、搜索、排序、Item/Location 模型、缩略图和大量视图逻辑，业务与 UI 混合较深。
  - `src/operation/`：复制、移动、删除、压缩、解压、新建、重命名、权限设置、可暂停/取消的 controller。
  - `src/mounter/`：挂载/网络位置抽象，GVFS 实现位于 `src/mounter/gvfs.rs`。
  - `src/trash.rs`：基于 `trash` crate 的回收站适配。
  - `src/mime_app.rs`、`src/thumbnailer.rs`、`src/thumbnail_cacher.rs`：freedesktop MIME 应用、缩略图器和缩略图缓存。
- Bug 证据等级：不适用。本阶段不是 bug 修复。
- 证据不足项：
  - 未运行 `cargo check`，因为本阶段只读分析，避免在源仓库写入构建产物。
  - 已确认目标运行环境范围：只做 Linux 本地文件系统。
  - 已确认“独立环境”的关键含义：允许存在外部二进制依赖，但运行时完全不依赖系统桌面服务。
  - 已确认前端形态：只做图形界面，不做命令行/TUI。
  - 已确认渲染策略曾为默认 `tiny-skia`、高级构建再开启 `wgpu`；后续因界面卡顿，已按用户要求改为 wgpu-only。

## 3. 安全门禁摘要

| 项 | 结论 |
|----|------|
| 风险矩阵初判 | L4，新系统/架构级取舍 |
| 命令权限 | 已执行 C0 只读检查；C1 仅新增本地 `docs/dev` 调研文档 |
| 高风险开发门禁 | 是，涉及文件系统操作、系统副作用、权限、删除、并发/异步 I/O、构建依赖 |
| 破坏性操作 | 否 |
| 用户已有修改 | 否，修改前 `git status --short` 为空 |
| 用户确认事项 | 已确认 Linux 本地文件系统、不依赖系统桌面服务、可依赖外部二进制、只做 GUI；渲染策略后续改为 wgpu-only |

## 4. 候选方案

| 方案 | 核心思路 | 优点 | 风险/代价 | 适用条件 |
|------|----------|------|-----------|----------|
| A | 直接 fork cosmic-files 并补齐/vendor 依赖 | 功能完整，已有文件操作、UI、桌面集成 | 依赖极重，COSMIC/Wayland/GVFS/DBus 耦合深；Git 依赖多；GPLv3 约束明确；很难达到少依赖 | 目标就是 COSMIC/Linux desktop 文件管理器，接受重依赖和 GPLv3 |
| B | 抽取/重写核心文件操作，另建轻量文件管理器 | 可以控制依赖，保留 cosmic-files 中成熟的操作经验 | 需要重建 UI、模型、错误处理、测试；需要重审删除/移动/回收站等高风险逻辑 | 目标是独立运行、少依赖、可长期维护 |
| C | 保留 cosmic-files 为参考实现，仅迁移部分思想 | 最低耦合，技术债少，可按目标平台设计 | 初期功能少，需要分阶段补齐 | MVP 优先，愿意先实现本地文件浏览/基础操作 |

## 5. 推荐结论

- 推荐方案：优先采用 C/B 混合。不要直接 fork 后做“减依赖”，而是新建轻量架构，把 `cosmic-files` 当参考实现；重点参考 `operation/` 的操作集合、冲突处理、进度/取消模型、缩略图缓存标准和 freedesktop 细节。
- 取舍理由：
  - `cosmic-files` 的产品目标是 COSMIC desktop 文件管理器，不是轻量独立文件管理器。
  - 直接基于它保留 UI 会带入 `libcosmic`、iced/wgpu/winit、COSMIC 配置、DBus、Wayland/GVFS、图像解码等大量依赖。
  - 可复用价值主要在文件管理经验，不在工程骨架。
- 需要进入 Plan 的关键约束：
  - 目标平台：第一阶段限定 Linux 本地文件系统。
  - 运行边界：允许调用外部二进制，但不得依赖系统桌面服务，例如 DBus/GVFS/portal/XDG MIME 服务/通知服务/桌面配置服务。
  - 明确 UI：只做 GUI，不做命令行/TUI；GUI 显示层可依赖 X11/Wayland，但不得依赖 DBus/GVFS/portal/XDG MIME/通知/桌面配置等桌面服务。
  - 渲染策略：当前实现固定使用 `wgpu + x11 + wayland`，不保留 `tiny-skia`。
  - 明确 MVP：本地目录浏览、复制/移动/重命名/新建、回收站或永久删除、搜索、打开文件、基础排序。
  - 明确哪些桌面集成后置：GVFS/网络挂载、DBus FileManager1、缩略图器、最近文件、MIME 默认应用、桌面图标。
- 需要用户确认的问题：
  - 是否接受 GPLv3 继承影响？
  - 是否必须保留 COSMIC 的 UI/体验，还是只借鉴功能和实现经验？
  - 是否接受 GPLv3 继承影响？
- 后续验证方向：
  - 用 `/tmp` 作为 `CARGO_TARGET_DIR` 跑 `cargo check --no-default-features` 和默认 feature 对比，确认最小可编译依赖。
  - 建立新项目依赖预算，先实现 local fs core 并补复制/移动/删除测试。
  - 对删除、覆盖、跨设备移动、符号链接、权限/mtime、取消和失败清理建立自动化回归。

## 6. 参考资料（按需）

- `/data/source/cosmic-files/Cargo.toml`
- `/data/source/cosmic-files/Cargo.lock`
- `/data/source/cosmic-files/src/app.rs`
- `/data/source/cosmic-files/src/tab.rs`
- `/data/source/cosmic-files/src/operation/`
- `/data/source/cosmic-files/src/mounter/`
- `/data/source/cosmic-files/src/trash.rs`
- `/data/source/cosmic-files/TESTING.md`
