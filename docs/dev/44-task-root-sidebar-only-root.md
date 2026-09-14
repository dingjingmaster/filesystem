# root 模式侧边栏只显示根目录轻量任务记录

> 文档元数据
> - 文件编号：44
> - 文档类型：task
> - 文件路径：docs/dev/44-task-root-sidebar-only-root.md
> - 文档版本：v1.0.0
> - 最后更新：2026-09-14
> - 需求级别：L2
> - 关联需求：调整 `--root` 模式侧边栏，只显示根目录；普通模式仍显示家目录特殊文件夹

## 1. 目标

- 要解决的问题：`--root` 模式当前隐藏家目录特殊文件夹，但仍显示“主文件夹”，不符合“只显示根目录”的侧边栏要求。
- 成功标准：普通启动侧边栏显示主文件夹、根目录和已存在的家目录特殊文件夹；`--root` 启动侧边栏只显示根目录，不显示主文件夹和家目录特殊文件夹。

## 2. 背景与边界

- 背景：`beba4c0` 引入 `--root` 后让 `HomeShortcutsLoaded` 在 root scope 下丢弃特殊文件夹，但侧边栏基础导航仍固定渲染 Home+Root。
- 包含：调整侧边栏基础导航分支；补充单元测试；更新相关产品/开发文档和本地索引。
- 不包含：改变 root 路径限制、地址栏真实路径显示、外部应用打开策略。
- 关键假设：root 模式下根目录入口仍指向 `--root <dir>` 指定目录。
- 非目标：不重新设计侧边栏布局。
- 最大修改范围：`crates/filesystem-gui/src/app.rs`、`docs/overview-product.md`、`docs/overview-product-dev.md`、`docs/dev/README.md`、本任务文档。
- 禁止触碰范围：文件操作、MIME、配置文件读取和 root scope 路径判定逻辑。

## 3. 风险门禁

| 项 | 结论 |
|----|------|
| 风险矩阵 | L2：启动参数影响导航 UI 显示，范围局限在 GUI 侧边栏 |
| 高风险开发门禁 | 是：涉及启动参数关联行为，按 L2 执行 |
| 破坏性操作 | 否 |
| 用户已有修改 | 否：编码前 `git status --short` 为空 |
| 底层/系统风险 | 否：不涉及 Rust unsafe、并发、内存、ABI |
| 命令权限 | C0/C1：只读检查、定向文件修改、定向 cargo test |
| 用户确认事项 | 用户已明确要求修改 `--root` 侧边栏行为 |
| 回滚/止损方式 | 回退本次相关文件即可恢复旧 root 侧边栏行为 |

## 4. 方案

- 推荐方案：新增侧边栏基础导航 kind 计算；普通模式返回 `[Home, Root]`，root 模式返回 `[Root]`，sidebar 渲染统一消费该列表。
- 取舍理由：保留现有 HomeShortcutsLoaded root 模式丢弃逻辑，只修正基础导航显示；不引入新状态或依赖。
- 风险与应对：测试直接覆盖普通模式和 root 模式的导航 kind，避免误删普通启动入口。

## 5. 执行计划

| 步骤 | 修改内容 | 验证方式 | 状态 |
|------|----------|----------|------|
| 1 | 补充 root/普通侧边栏基础导航红灯测试 | `cargo test -p filesystem-gui sidebar_nav -- --nocapture` | 完成 |
| 2 | 实现 root 模式只返回根目录入口 | `cargo test -p filesystem-gui sidebar_nav -- --nocapture` | 完成 |
| 3 | 更新长期文档、本地索引和任务总结 | `git diff --check` | 完成 |

## 6. 实现记录

- 修改文件：`crates/filesystem-gui/src/app.rs`、`docs/overview-product.md`、`docs/overview-product-dev.md`、`docs/dev/README.md`、`docs/dev/44-task-root-sidebar-only-root.md`。
- 关键决策：新增 `sidebar_nav_kinds()` 作为侧边栏基础导航单一分支；普通模式返回 `[Home, Root]`，root 模式返回 `[Root]`；保留 root 模式丢弃 `HomeShortcutsLoaded` 的逻辑，确保特殊文件夹也不显示。
- 计划偏差：无。
- 安全门禁执行结果：未执行破坏性操作；未修改 root scope 路径判定、文件操作或外部应用逻辑；本次只调整侧边栏显示分支。
- L2 审视结论：产品侧满足 root 模式只显示根目录、普通模式保留家目录特殊文件夹；工程侧通过红绿测试和 GUI crate 全量回归；架构侧未引入新依赖或额外状态。

## 7. 验证记录

- 验证环境：本地
- 系统信息（OS/内核/架构/编译器/运行时，按需）：Linux gentoo-pc 7.1.4-gentoo-dingjing x86_64；rustc 1.96.1；cargo 1.96.1。

| 验证项 | 命令/步骤 | 结果 | 备注 |
|--------|-----------|------|------|
| 红灯测试 | `cargo test -p filesystem-gui sidebar_nav -- --nocapture` | 通过 | 新增测试先因缺少 `sidebar_nav_kinds()` 失败，证明测试覆盖待实现分支 |
| 定向回归 | `cargo test -p filesystem-gui sidebar_nav -- --nocapture` | 通过 | 2 passed，覆盖普通和 root 侧边栏基础入口 |
| GUI crate 回归 | `cargo test -p filesystem-gui` | 通过 | 153 passed |
| 格式检查 | `cargo fmt --check` | 通过 | 已运行 `cargo fmt` 修正格式 |
| 空白检查 | `git diff --check` | 通过 | 无空白错误 |

- 未执行验证项：未启动真实 X11/Wayland GUI 做人工侧边栏截图验证。
- 残余风险：自动化覆盖状态分支，未覆盖真实桌面渲染截图；风险较低。

## 8. 总结

- 最终结果：普通模式侧边栏保留主文件夹、根目录和家目录特殊文件夹；`--root` 模式侧边栏基础入口只保留根目录，且继续不加载家目录特殊文件夹。
- 遗留风险：真实 GUI 人工 smoke test 未执行。
- 后续建议：如继续裁剪 root 模式 UI，可集中在侧边栏导航分支上扩展，避免影响路径访问限制。
