# Rust Edition 2021 轻量任务记录

> 文档元数据
> - 文件编号：15
> - 文档类型：task
> - 文件路径：docs/dev/15-task-rust-edition-2021.md
> - 文档版本：v1.0.0
> - 最后更新：2026-06-26
> - 需求级别：L2
> - 关联需求：将当前项目 Rust workspace edition 从 2024 调整到 2021，并确认低版本系统构建所需 Rust/Cargo 版本。

## 1. 目标

- 要解决的问题：项目当前声明 Rust edition 2024，用户要求改为 2021；远端 Ubuntu 18.04 系统自带 Cargo/Rust 1.65 无法解析当前 `Cargo.lock`。
- 成功标准：workspace package edition 为 2021；workspace `rust-version` 与当前依赖 MSRV 一致；各 crate 继续继承 workspace 配置；项目可通过格式、编译和测试验证。

## 2. 背景与边界

- 背景：`Cargo.toml` 通过 `[workspace.package]` 统一声明 edition，三个 crate 均使用 `edition.workspace = true`。
- 包含：修改 workspace edition、同步当前实际 MSRV、同步开发概览和本地任务索引。
- 不包含：降低依赖版本；为 Rust 2018 做兼容。
- 关键假设：用户说“改到 2021”指 Rust edition 2021。
- 非目标：不改变运行时行为和文件操作逻辑。
- 最大修改范围：`Cargo.toml`、`docs/overview-product-dev.md`、`docs/dev/README.md` 和本任务记录。
- 禁止触碰范围：不改无关源码，不回退已有文件操作进度实现。

## 3. 风险门禁

| 项 | 结论 |
|----|------|
| 风险矩阵 | L2，构建配置变更 |
| 高风险开发门禁 | 是，涉及构建配置 |
| 破坏性操作 | 否 |
| 用户已有修改 | 否，编码前工作区无未提交修改 |
| 底层/系统风险 | 否 |
| 命令权限 | C0/C1 |
| 用户确认事项 | 无，用户明确要求改到 2021 |
| 回滚/止损方式 | 将 workspace edition 改回 2024 并恢复文档 |

## 4. 方案

- 推荐方案：把 workspace package `edition` 从 `2024` 改为 `2021`，并把 `rust-version` 修正为 `"1.88"`。
- 取舍理由：edition 与 MSRV 是两个概念；远端用 Rust 1.85 编译当前 lockfile 时，Cargo 明确报告 `iced 0.14.0`、`iced_program 0.14.0`、`wgpu 27.0.1` 需要 `rustc 1.88`，继续声明 1.85 会误导低版本系统构建。
- 风险与应对：如果代码含 2024-only 语义，编译或测试会暴露；用 workspace 全量测试验证，并在远端 Ubuntu 18.04 上用 Rust 1.88 复核 GUI 编译。

## 5. 执行计划

| 步骤 | 修改内容 | 验证方式 | 状态 |
|------|----------|----------|------|
| 1 | 修改 workspace edition 为 2021，并同步 `rust-version = "1.88"` 与开发概览 | `cargo check -p filesystem-gui` | 完成 |
| 2 | 更新本地任务文档和索引 | `git diff --check` | 完成 |
| 3 | 运行格式、编译和测试验证 | `cargo fmt --check`、`cargo test`、`make test` | 完成 |
| 4 | 在远端 Ubuntu 18.04 安装用户态 Rust/Cargo 1.88 并复核 GUI 编译 | `cargo check -p filesystem-gui` | 完成 |

## 6. 实现记录

- 修改文件：
  - `Cargo.toml`
  - `docs/overview-product-dev.md`
  - `docs/dev/README.md`
  - `docs/dev/15-task-rust-edition-2021.md`
- 关键决策：edition 改为 2021；MSRV 修正为 Rust/Cargo 1.88，匹配当前 GUI 依赖要求。
- 计划偏差：远端低版本验证发现 1.85 不能满足当前依赖，因此将原记录中的 1.85 修正为 1.88。
- 远端处理：在用户目录通过 rustup 安装 Rust/Cargo 1.88.0，并设为该用户默认工具链；未使用 root 修改系统 `/usr/bin/rustc` 或 `/usr/bin/cargo`。
- 安全门禁执行结果：未执行破坏性操作；未修改无关源码。

## 7. 验证记录

- 验证环境：本地 Linux workspace。
- 系统信息：本地使用当前 Rust/Cargo 工具链；远端验证环境为 Ubuntu 18.04.6 LTS、x86_64。

| 验证项 | 命令/步骤 | 结果 | 备注 |
|--------|-----------|------|------|
| 格式检查 | `cargo fmt --check` | 通过 | edition 2021 下已重新格式化 import 排序 |
| GUI 编译 | `cargo check -p filesystem-gui` | 通过 |  |
| core 测试 | `cargo test -p filesystem-core` | 通过 | 31 个测试通过 |
| GUI 测试 | `cargo test -p filesystem-gui` | 通过 | 37 个测试通过 |
| workspace 测试 | `cargo test` | 通过 | core 31、GUI 37、MIME 10 个测试通过 |
| 项目测试入口 | `make test` | 通过 | 调用 `cargo test` |
| diff 空白检查 | `git diff --check` | 通过 |  |
| 远端旧工具链验证 | `cargo check -p filesystem-gui` | 失败 | Ubuntu 18.04 系统 Cargo/Rust 1.65 无法解析 lockfile v4 |
| 远端 Rust 1.85 验证 | `cargo check -p filesystem-gui` | 失败 | 当前依赖要求 `rustc 1.88` |
| 远端 Rust 1.88 验证 | `cargo check -p filesystem-gui` | 通过 | Ubuntu 18.04 用户态 rustup 工具链，Rust/Cargo 1.88.0 |

- 未执行验证项：低于 Rust 1.88 的可用依赖降级方案未实现。
- 残余风险：如果必须支持 Rust 1.65/1.85，需要单独做依赖版本降级和 lockfile 兼容任务。

## 8. 总结

- 最终结果：workspace Rust edition 已改为 2021，`rust-version` 已修正为 1.88，各 crate 继续继承 workspace 配置，当前代码在 edition 2021 下通过本地编译和测试。
- 遗留风险：这不是低版本 MSRV 降级；Ubuntu 18.04 这类系统需要通过 rustup 使用 Rust/Cargo 1.88 或更高版本构建。
- 后续建议：如需支持更低 Rust/Cargo 版本，应单独做依赖降级和 lockfile 兼容任务。
