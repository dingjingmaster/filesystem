# 远端 ARM 构建轻量任务记录

> 文档元数据
> - 文件编号：29
> - 文档类型：task
> - 文件路径：docs/dev/29-task-remote-arm-build.md
> - 文档版本：v1.0.0
> - 最后更新：2026-07-03
> - 需求级别：L2
> - 关联需求：在 192.168.8.246 环境下编译文件管理器 ARM 版，并自行配置 Cargo。

## 1. 目标

- 要解决的问题：远端 ARM 测试机缺少 Rust/Cargo 环境，需要同步当前项目并编译 `filesystem-gui` release 产物。
- 成功标准：远端 aarch64 环境可执行 `cargo build --release -p filesystem-gui` 并产出 ARM64 `filesystem-gui` 二进制；记录构建环境、验证结果和产物位置。

## 2. 背景与边界

- 背景：项目是 Rust workspace，根目录 `Makefile` 封装 `cargo build --release`；workspace `rust-version` 为 1.88。
- 包含：远端环境探测、用户级 Rust/Cargo 配置、项目同步、release 构建、产物基础验证。
- 不包含：系统级包安装、生产部署、GUI 人工运行验证、功能改造。
- 关键假设：用户提供的 SSH 凭据仅用于本次测试机登录；不写入文档、配置或仓库。
- 非目标：不调整项目架构，不新增依赖，不改变运行时行为。
- 最大修改范围：本地仅更新 `docs/dev` 任务记录和索引；如构建暴露源码或配置问题，再按最小范围修改相关文件。
- 禁止触碰范围：不修改 `AGENTS.project.md`；不执行系统级破坏性操作；不记录明文凭据。

## 3. 风险门禁

| 项 | 结论 |
|----|------|
| 风险矩阵 | L2，远端构建环境配置和 release 构建验证 |
| 高风险开发门禁 | 是，涉及构建环境与发布产物验证；当前不改构建脚本 |
| 破坏性操作 | 否，不删除远端用户数据；同步目标使用独立工作目录 |
| 用户已有修改 | 否，本地工作区启动时干净；后续修改前复查 |
| 底层/系统风险 | 否，不改 C/C++、Rust unsafe、ABI/API 或并发逻辑 |
| 命令权限 | C2，远程登录、项目同步、用户级 Rust/Cargo 配置和依赖下载 |
| 用户确认事项 | 已确认使用测试机 SSH 凭据和自行配置 Cargo |
| 回滚/止损方式 | 删除或忽略远端独立构建目录与用户级 cargo 配置；本地文档可按本任务变更回退 |

## 4. 方案

- 推荐方案：在远端用户目录配置 Rust 1.88+ 工具链，使用 `rsync` 同步当前仓库到独立目录，在 aarch64 主机原生执行 release 构建。
- 取舍理由：原生 ARM 构建避免本地交叉编译的系统库和 glibc 匹配问题，Ubuntu 18.04 可产出兼容较老 glibc 的 aarch64 二进制。
- 风险与应对：若远端无法访问 Rust/Cargo 源，改用本机下载/同步工具链或配置 cargo 镜像；若缺系统开发库，先定位 `pkg-config`/链接错误，再决定是否需要用户授权系统级安装。

## 5. 执行计划

| 步骤 | 修改内容 | 验证方式 | 状态 |
|------|----------|----------|------|
| 1 | 探测远端 OS、架构和现有构建工具 | `ssh ... 'uname -a; command -v cargo rustc gcc make pkg-config'` | 完成 |
| 2 | 配置远端用户级 Rust/Cargo 工具链 | `cargo --version; rustc --version` | 完成 |
| 3 | 同步当前仓库到远端独立构建目录 | `ssh ... 'test -f Cargo.toml && ls crates'` | 完成 |
| 4 | 在远端执行 ARM release 构建 | `cargo build --release -p filesystem-gui` | 完成 |
| 5 | 验证 ARM 产物和记录结果 | `file target/release/filesystem-gui; ldd ...` | 完成 |

## 6. 实现记录

- 修改文件：本地任务文档和 `docs/dev/README.md` 索引；源码未修改。
- 关键决策：优先远端 aarch64 原生构建，不在本机交叉编译；非交互 SSH 命令显式加载 `~/.cargo/env`。
- 计划偏差：远端已有用户级 Rust/Cargo，不需要重新安装；只需显式加载 cargo 环境。
- 安全门禁执行结果：未修改源码；凭据不写入仓库；远端写入限定在用户目录独立构建路径和用户级 Rust/Cargo 配置。

## 7. 验证记录

- 验证环境：远端 `192.168.8.246`。
- 系统信息（OS/内核/架构/编译器/运行时，按需）：Ubuntu 18.04 LTS，Linux 5.4.0-125-generic，aarch64；已有 `/usr/bin/gcc`、`/usr/bin/make`、`/usr/bin/pkg-config`；用户级 Rust/Cargo 位于 `~/.cargo/bin`，非交互 SSH 初始 `PATH` 未包含该目录。

| 验证项 | 命令/步骤 | 结果 | 备注 |
|--------|-----------|------|------|
| 远端环境探测 | `ssh ... 'uname -a; command -v cargo rustc gcc make pkg-config'` | 通过 | 确认 aarch64；非交互默认 `PATH` 未找到 cargo/rustc |
| Rust/Cargo 工具链 | `~/.cargo/bin/rustc --version; ~/.cargo/bin/cargo --version; ~/.cargo/bin/rustup toolchain list` | 通过 | `rustc 1.96.1`、`cargo 1.96.1`、stable aarch64 默认工具链；cargo 配置 USTC sparse 镜像 |
| 远端源码同步 | `rsync ... /home/dingjing/filesystem-arm-build.7TnYoG/; cargo metadata --no-deps` | 通过 | 独立构建目录 `/home/dingjing/filesystem-arm-build.7TnYoG`，workspace 三个 crate 识别正常 |
| Release 构建 | `cargo build --release -p filesystem-gui` | 通过 | `Finished release profile [optimized] target(s) in 1m 59s` |
| 远端产物检查 | `file target/release/filesystem-gui; readelf -h ...; ldd ...; sha256sum ...` | 通过 | ARM aarch64 ELF，动态依赖为 glibc/pthread/dl/m/gcc_s；大小 24M；SHA256 `912d1edbc1c3d2faecf1a12a0ab0ad038a3b887a900bafea5715bc863582f086` |
| 本地副本检查 | `file target/aarch64-remote-release/filesystem-gui; sha256sum ...` | 通过 | 本地副本哈希与远端一致 |

- 未执行验证项：未在远端图形会话启动 GUI 做人工交互验证。
- 残余风险：真实 GUI 运行验证需要远端图形会话，本任务只覆盖构建和产物基础检查。

## 8. 总结

- 最终结果：已在 192.168.8.246 Ubuntu 18.04 aarch64 环境完成 `filesystem-gui` release 构建；远端稳定产物为 `/home/dingjing/filesystem-gui-aarch64-20260703-061616`，本地副本为 `target/aarch64-remote-release/filesystem-gui`。
- 遗留风险：未验证真实 X11/Wayland 图形会话运行效果；若目标机运行时图形栈或 GPU 驱动异常，需要单独做 GUI 实测。
- L2 审视结果：源码、公共 API、配置格式和依赖声明均未修改；远端 release 构建、`file/readelf/ldd` 和本地副本 SHA256 一致性检查作为成功证据；内存、并发和错误路径风险未因本任务新增代码而扩大。
- 后续建议：需要发布包时，再按目标发行方式补安装目录、桌面文件和配置文件打包规则。
