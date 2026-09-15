# 文件管理器外部应用 Hook 模式轻量任务记录

> 文档元数据
> - 文件编号：46
> - 文档类型：task
> - 文件路径：docs/dev/46-task-filesystem-hook-mode.md
> - 文档版本：v1.0.0
> - 最后更新：2026-09-15
> - 需求级别：L2
> - 关联需求：支持 `FILESYSTEM_HOOK_MODE=sandbox|yunbox`，区分沙盒和云盒外部应用 hook 注入

## 1. 目标

- 要解决的问题：文件管理器在打开外部应用前原本统一移除 `LD_PRELOAD`，云盒模式下由 `fileboxd` 传入的 `BOXFlLESO=/usr/local/andsec/hook/yun/fileman.so` 无法继续传给 WPS 等外部应用。
- 成功标准：默认模式继续清理 `LD_PRELOAD`；`FILESYSTEM_HOOK_MODE=sandbox` 时只注入沙盒 `hook-connect.so`；`FILESYSTEM_HOOK_MODE=yunbox` 时只注入 `BOXFlLESO` 指向的云盒 hook。

## 2. 背景与边界

- 背景：云盒打开链路使用 `sandbox-nemo --root /yunbox` 作为文件管理器，后续双击 WPS 文档时仍由文件管理器启动外部应用。
- 包含：外部应用启动环境中的 `LD_PRELOAD` 模式选择；相关回归测试和文档。
- 不包含：修改 WPS、云盒 hook 实现、沙盒 hook 实现或打包安装路径。
- 关键假设：云盒入口会设置 `FILESYSTEM_HOOK_MODE=yunbox` 和 `BOXFlLESO`；普通沙盒入口设置 `FILESYSTEM_HOOK_MODE=sandbox`。
- 非目标：合并两个 hook；本轮明确保持沙盒和云盒 hook 互斥。
- 最大修改范围：`crates/filesystem-gui/src/apps.rs`、本地文档和开发概览。
- 禁止触碰范围：不改 `AGENTS.project.md`，不执行提交/推送/部署。

## 3. 风险门禁

| 项 | 结论 |
|----|------|
| 风险矩阵 | L2，单模块启动环境行为变化 |
| 高风险开发门禁 | 否，未涉及 unsafe、系统调用或公共协议 |
| 破坏性操作 | 否 |
| 用户已有修改 | 否，目标仓库开始时工作区干净 |
| 底层/系统风险 | 否；变更为 Rust safe code 的子进程环境构造 |
| 命令权限 | C1，工作区内修改和定向测试 |
| 用户确认事项 | 已确认按 `FILESYSTEM_HOOK_MODE=sandbox|yunbox` 实现 |
| 回滚/止损方式 | 回退 `apps.rs` 中 hook 模式分支即可恢复默认移除 `LD_PRELOAD` 行为 |

## 4. 方案

- 推荐方案：`sanitize_open_command_environment()` 先移除继承的 `LD_PRELOAD`，再按 `FILESYSTEM_HOOK_MODE` 设置子应用 hook：`sandbox` 使用固定沙盒 hook，`yunbox` 使用非空 `BOXFlLESO`；其它值或缺失时保持无 `LD_PRELOAD`。
- 取舍理由：避免云盒和沙盒 hook 混合导致符号拦截顺序和行为互相污染；边界清晰，默认行为兼容。
- 风险与应对：若入口未设置 `FILESYSTEM_HOOK_MODE`，仍不会注入 hook；由定向测试覆盖三种模式，后续 sec_linux 侧需设置该变量。

## 5. 执行计划

| 步骤 | 修改内容 | 验证方式 | 状态 |
|------|----------|----------|------|
| 1 | 为外部应用启动环境补 `sandbox/yunbox` 模式测试 | `cargo test -p filesystem-gui spawn_open_command_ -- --test-threads=1 --nocapture` | 完成 |
| 2 | 实现 `FILESYSTEM_HOOK_MODE` 分流 | 同上 | 完成 |
| 3 | 更新本地文档和开发概览 | 人工阅读、`git diff --check` | 完成 |

## 6. 实现记录

- 修改文件：`crates/filesystem-gui/src/apps.rs`、`docs/dev/46-task-filesystem-hook-mode.md`、`docs/dev/README.md`、`docs/overview-product-dev.md`。
- 关键决策：默认仍清理 `LD_PRELOAD`；`sandbox` 和 `yunbox` 互斥，不拼接两个 hook。
- 计划偏差：无。
- 安全门禁执行结果：只执行工作区内修改和定向验证；未提交。

## 7. 验证记录

- 验证环境：本地 `/data/code/andsec-dev/filesystem`。
- 系统信息：Rust/Cargo 项目，使用当前工具链和现有 workspace 依赖。

| 验证项 | 命令/步骤 | 结果 | 备注 |
|--------|-----------|------|------|
| Hook 模式定向回归 | `cargo test -p filesystem-gui spawn_open_command_ -- --test-threads=1 --nocapture` | 通过 | 3 passed，覆盖默认、sandbox、yunbox 三种模式 |
| 外部应用邻近回归 | `cargo test -p filesystem-gui apps:: -- --test-threads=1` | 通过 | 53 passed |
| 格式检查 | `cargo fmt -- --check` | 通过 | 无格式差异 |

- 未执行验证项：未在真实云盒里启动 WPS 验证，需要 sec_linux 侧设置 `FILESYSTEM_HOOK_MODE=yunbox` 后联调。
- 残余风险：当前只完成文件管理器支持；云盒启动方尚需传入新环境变量。

## 8. 总结

- 最终结果：文件管理器支持 `FILESYSTEM_HOOK_MODE=sandbox|yunbox` 控制外部应用 `LD_PRELOAD`，默认仍清理继承 hook。
- 遗留风险：真实云盒 WPS 剪贴板加密还依赖 sec_linux/fileboxd 设置新变量并部署云盒 hook。
- 后续建议：在 sec_linux 的 `fileboxd` 云盒文件管理器环境中设置 `FILESYSTEM_HOOK_MODE=yunbox`，普通沙盒入口设置 `sandbox`。
