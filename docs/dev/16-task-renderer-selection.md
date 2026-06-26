# Renderer Selection 轻量任务记录

> 文档元数据
> - 文件编号：16
> - 文档类型：task
> - 文件路径：docs/dev/16-task-renderer-selection.md
> - 文档版本：v1.0.0
> - 最后更新：2026-06-26
> - 需求级别：L2
> - 关联需求：GUI 默认优先使用 wgpu；探测失败或 `ICED_BACKEND=tiny-skia` 时回退 tiny-skia。

## 1. 目标

- 要解决的问题：当前 GUI 只编译 `wgpu` 渲染器，在 Ubuntu 18.04 + Virtio GPU 等图形栈上可能因 wgpu shader pipeline 不兼容而无法启动。
- 成功标准：GUI 同时编译 `wgpu` 和 `tiny-skia`；未显式设置 `ICED_BACKEND` 时默认优先 wgpu，wgpu 探测失败时设置 `ICED_BACKEND=tiny-skia`；显式 `ICED_BACKEND=tiny-skia` 直接使用 tiny-skia。

## 2. 背景与边界

- 背景：iced 0.14 同时启用 `wgpu` 和 `tiny-skia` 后会编译 fallback compositor，但远端失败发生在 wgpu shader pipeline 创建阶段，不能只依赖 adapter 是否存在。
- 包含：新增启动前 renderer 选择逻辑、启用 `tiny-skia` feature、同步开发概览和本地任务索引。
- 不包含：升级系统 Mesa/GPU 驱动；改变 GUI 业务状态机；改变文件操作进度逻辑。
- 关键假设：用户要求“默认优先 wgpu”指未设置 `ICED_BACKEND` 时优先尝试 wgpu；显式后端配置由用户承担。
- 非目标：不实现系统版本识别；不为所有 wgpu shader 做完整兼容性测试。
- 最大修改范围：`crates/filesystem-gui/Cargo.toml`、`src/main.rs`、新增 `src/renderer.rs` 和相关文档。
- 禁止触碰范围：不修改文件复制/剪切/删除业务逻辑，不回退已有未提交修改。

## 3. 风险门禁

| 项 | 结论 |
|----|------|
| 风险矩阵 | L2，构建 feature 与启动路径变更 |
| 高风险开发门禁 | 是，涉及构建配置和运行时启动策略 |
| 破坏性操作 | 否 |
| 用户已有修改 | 是，保留现有未提交修改，仅追加本需求相关变更 |
| 底层/系统风险 | 否，不引入 Rust unsafe，不修改系统配置 |
| 命令权限 | C0/C1 |
| 用户确认事项 | 无，用户明确要求实现该策略 |
| 回滚/止损方式 | 移除 `tiny-skia` feature、删除 renderer 探测并恢复原 main 入口 |

## 4. 方案

- 推荐方案：同时启用 iced `wgpu` 与 `tiny-skia`；启动前读取 `ICED_BACKEND`，显式 `tiny-skia`/`tiny_skia` 直接规范化为 `tiny-skia`，其它显式值不覆盖；未设置时执行 wgpu 探测，探测失败才设置 `ICED_BACKEND=tiny-skia`。
- 取舍理由：探测通过时不强行设置 `ICED_BACKEND=wgpu`，保留 iced 自身“wgpu 初始化失败再 fallback tiny-skia”的机会；探测失败时提前设置 tiny-skia，避免 wgpu pipeline validation panic。
- 风险与应对：探测只覆盖最小 device 和包含 `unpack2x16float` 的 pipeline，不覆盖所有未来 shader；通过远端 Ubuntu 18.04 复测软件 fallback。

## 5. 执行计划

| 步骤 | 修改内容 | 验证方式 | 状态 |
|------|----------|----------|------|
| 1 | 启用 `tiny-skia` feature，并新增启动前 renderer 选择逻辑 | `cargo check -p filesystem-gui` | 完成 |
| 2 | 为环境选择逻辑添加纯单元测试 | `cargo test -p filesystem-gui renderer` | 完成 |
| 3 | 更新开发概览和本地任务索引 | `git diff --check` | 完成 |
| 4 | 在远端 Ubuntu 18.04 复测默认启动 fallback | `DISPLAY=:0 ... timeout 8s ./target/debug/filesystem-gui` | 完成 |

## 6. 实现记录

- 修改文件：
  - `crates/filesystem-gui/Cargo.toml`
  - `crates/filesystem-gui/src/main.rs`
  - `crates/filesystem-gui/src/renderer.rs`
  - `docs/overview-product-dev.md`
  - `docs/dev/README.md`
  - `docs/dev/16-task-renderer-selection.md`
- 关键决策：探测失败才写入 `ICED_BACKEND=tiny-skia`；用户显式设置其它非空 `ICED_BACKEND` 时不覆盖。
- 计划偏差：无。
- 安全门禁执行结果：未执行破坏性操作；未修改系统配置；未回退用户已有修改。

## 7. 验证记录

- 验证环境：本地 Linux workspace；远端验证环境待复测。
- 系统信息：本地使用当前 Rust/Cargo 工具链；远端目标为 Ubuntu 18.04.6 LTS、x86_64。

| 验证项 | 命令/步骤 | 结果 | 备注 |
|--------|-----------|------|------|
| GUI 编译 | `cargo check -p filesystem-gui` | 通过 | 本地通过 |
| renderer 单测 | `cargo test -p filesystem-gui renderer` | 通过 | 本地 5 个 renderer 测试通过；远端同命令通过 |
| GUI 测试 | `cargo test -p filesystem-gui` | 通过 | 本地 42 个测试通过 |
| 格式检查 | `cargo fmt --check` | 通过 |  |
| diff 空白检查 | `git diff --check` | 通过 |  |
| 远端构建 | `cargo build -p filesystem-gui` | 通过 | Ubuntu 18.04.6 LTS，Rust/Cargo 1.88.0 |
| 远端默认启动复测 | `DISPLAY=:0 XAUTHORITY=/run/user/1000/gdm/Xauthority timeout 8s ./target/debug/filesystem-gui` | 通过 | 退出码 124，表示运行 8 秒后被 timeout 结束；未再出现 wgpu shader panic |
| 远端显式 tiny-skia 启动复测 | `ICED_BACKEND=tiny-skia DISPLAY=:0 XAUTHORITY=/run/user/1000/gdm/Xauthority timeout 8s ./target/debug/filesystem-gui` | 通过 | 退出码 124，表示显式 tiny-skia 路径可运行 |

- 未执行验证项：未人工确认远端屏幕上的窗口内容。
- 残余风险：wgpu 探测无法覆盖所有驱动缺陷；显式 `ICED_BACKEND=wgpu` 仍可能在不兼容图形栈上失败。

## 8. 总结

- 最终结果：GUI 已同时支持 `wgpu` 和 `tiny-skia`；默认优先 wgpu，探测失败或显式 `ICED_BACKEND=tiny-skia` 时使用 tiny-skia；远端 Ubuntu 18.04 默认启动已不再因 wgpu shader pipeline 失败而崩溃。
- 遗留风险：显式强制 `ICED_BACKEND=wgpu` 时仍可能在低能力图形栈上失败；远端仅通过 timeout 运行验证，未做人工窗口内容确认。
- 后续建议：如未来需要可观测性，可在启动时输出实际选择的 renderer。
