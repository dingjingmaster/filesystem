# 逻辑根目录启动参数任务记录

> 文档元数据
> - 文件编号：42
> - 文档类型：task
> - 文件路径：docs/dev/42-task-root-scope.md
> - 文档版本：v1.0.0
> - 最后更新：2026-09-09
> - 需求级别：L2
> - 关联需求：为云盒打开场景增加 `--root` 参数，限制文件管理器不能跳出挂载目录

## 1. 目标

- 要解决的问题：云盒打开时只切换进程当前目录不足以限制文件管理器访问范围，地址栏、根目录按钮、历史导航或软链接目标仍可能跳出云盒挂载目录。
- 成功标准：
  - 支持 `filesystem-gui --root <dir>`。
  - 启动后默认打开 `<dir>`。
  - 根目录按钮指向 `<dir>`。
  - 目录访问、地址栏绝对路径、历史导航、软链接目标和打开文件入口不得跳出 `<dir>`。
  - 指定 `--root` 后，点击 root 外目录自动跳回 `<dir>`，侧边栏不显示下载、图片、桌面、文档、音乐、视频等家目录快捷入口。

## 2. 背景与边界

- 背景：云盒打开由外部 launch 挂载云盒后启动文件管理器，文件管理器需要提供逻辑根目录能力，而不是依赖真实 `chroot`。
- 包含：参数解析、逻辑根 canonicalize、导航/打开入口限制、root 外目录跳回 root、侧边栏常见目录快捷入口隐藏、右键打开方式/终端/属性/重命名/删除防御检查、相关单元测试。
- 不包含：真实 `chroot`；地址栏虚拟路径 `/...` 映射；外部应用的 DLP/DRM hook；云盒水印或在线授权。
- 关键假设：`--root` 传入的是已存在目录；云盒挂载目录由外部进程负责创建和清理。
- 非目标：不改变普通无 `--root` 启动行为。
- 最大修改范围：`crates/filesystem-gui/src/app.rs`、`main.rs`、新增 root 参数模块和文档。
- 禁止触碰范围：不修改 sandbox 运行时、系统文件和用户真实目录。

## 3. 风险门禁

| 项 | 结论 |
|----|------|
| 风险矩阵 | L2：新增启动参数和访问边界，影响用户导航行为 |
| 高风险开发门禁 | 否；未使用 `unsafe`，不做真实 chroot |
| 破坏性操作 | 否 |
| 用户已有修改 | 否；本轮开始时仅 filesystem 工作区内本次改动 |
| 底层/系统风险 | 否；仅 Rust 路径校验和 GUI 状态流转 |
| 命令权限 | C0/C1 |
| 用户确认事项 | 用户确认按短设计实现第一版逻辑根，不做地址栏虚拟路径 |
| 回滚/止损方式 | 回退本次代码即可恢复旧行为；无磁盘格式或用户数据迁移 |

## 4. 方案

- 推荐方案：新增 `RootScope` 解析 `--root`，启动时把 root canonicalize 后作为初始 cwd；所有访问入口调用统一 `path_allowed()`，实际路径 canonicalize 后必须位于 root 下。
- 取舍理由：逻辑根不破坏文件管理器依赖的系统库、桌面文件、字体、外部应用和 D-Bus 环境，比真实 `chroot` 更适合云盒挂载目录。
- 风险与应对：地址栏仍显示真实路径，后续如需隐藏真实挂载路径再做虚拟路径映射；不存在路径在 root 内也会因无法 canonicalize 被拒绝，第一版接受该保守行为。

## 5. 执行计划

| 步骤 | 修改内容 | 验证方式 | 状态 |
|------|----------|----------|------|
| 1 | 增加 root 参数解析和 `RootScope` 路径判定 | root 单元测试 | 已完成 |
| 2 | FileManager 启动和导航入口接入 root scope | app 单元测试 | 已完成 |
| 3 | 右键打开方式、终端、属性、重命名、删除补防御检查 | GUI 全包测试 | 已完成 |
| 4 | 更新文档 | diff 检查 | 已完成 |

## 6. 实现记录

- 修改文件：
  - `crates/filesystem-gui/src/root.rs`
  - `crates/filesystem-gui/src/main.rs`
  - `crates/filesystem-gui/src/app.rs`
  - `docs/dev/README.md`
  - `docs/overview-product.md`
  - `docs/overview-product-dev.md`
- 关键决策：
  - `--root` 是逻辑根，不调用 `chroot()`。
  - `NavKind::Root` 在 root scope 下指向配置 root；Home 不在 root 内时也回到 root。
  - 目录导航、历史导航和目录软链接目标跳出 root 时回到配置 root；文件打开、终端、属性、重命名、删除等非导航入口继续拒绝越界。
  - `--root` 模式忽略 XDG 家目录快捷入口，侧边栏只保留主文件夹和根目录。
- 计划偏差：无。
- 安全门禁执行结果：未执行破坏性命令；只改工作区内源码和文档。

## 7. 验证记录

- 验证环境：本地 `/data/code/filesystem`。
- 系统信息：Linux / Rust cargo workspace。

| 验证项 | 命令/步骤 | 结果 | 备注 |
|--------|-----------|------|------|
| RED 测试 | `cargo test -p filesystem-gui root_scope --no-default-features` | 预期失败 | 缺少 `crate::root`、`root_scope` 和 `nav_kind_path` |
| root 定向测试 | `cargo test -p filesystem-gui root --no-default-features` | 通过 | 7 passed |
| root 定向测试 | `cargo test -p filesystem-gui root_scope --no-default-features` | 通过 | 3 passed |
| GUI 全包测试 | `cargo test -p filesystem-gui --no-default-features` | 通过 | 149 passed |
| 格式检查 | `cargo fmt --check` | 通过 | 无格式差异 |
| GUI 构建 | `cargo build -p filesystem-gui --no-default-features` | 通过 | dev profile 构建通过 |
| diff 空白检查 | `git diff --check` | 通过 | 无空白错误 |

- 未执行验证项：真实 GUI 会话下 `--root` 启动和云盒挂载目录端到端人工验证未执行。
- 残余风险：地址栏仍显示真实路径；外部应用打开后是否受云盒 DRM 限制依赖后续 hook/launch/fileboxd 策略。

## 8. 总结

- 最终结果：`filesystem-gui --root <dir>` 已支持逻辑根目录，主要导航和打开入口会限制在 root；目录越界导航会回到 root，家目录常见快捷入口在 root 模式下隐藏。
- 遗留风险：真实云盒场景仍需联调外部应用打开、复制导出策略和地址栏显示形式。
- 后续建议：云盒集成稳定后，再按产品要求决定是否增加虚拟路径显示和云盒专用菜单裁剪。
