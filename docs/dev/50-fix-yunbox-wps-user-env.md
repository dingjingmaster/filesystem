# 云盒 WPS 用户环境修复记录

> 文档元数据
> - 文件编号：50
> - 文档类型：fix
> - 文件路径：docs/dev/50-fix-yunbox-wps-user-env.md
> - 完成日期：2026-09-20
> - 需求级别：L2

## 1. 问题定义

- 问题现象：云盒文件管理器中打开 WPS 文字、表格、演示文件不稳定，可能停在 WPS 锁定/初始化状态，表现为文件未真正打开。
- 预期行为：42 和 65 两套远端云盒系统中，`/yunbox` 内的 WPS 文字、表格、演示文件均能通过文件管理器打开到对应 WPS 组件。
- 影响范围：`FILESYSTEM_HOOK_MODE=yunbox` 下外部应用启动环境，重点是 WPS Office。
- 不包含：修改 WPS 授权、云盒 hook 二进制、系统 MIME 配置或 `fileboxd.bin` 启动链。

## 2. 证据与根因

- 复现/验证环境：
  - `192.168.122.42`：Kylin V10 SP1，云盒文件管理器路径 `/usr/local/andsec/sandbox/bin/sandbox-nemo.bin`。
  - `192.168.122.65`：Ubuntu/UKUI 环境，云盒文件管理器同路径。
- hook 结论：
  - `/usr/local/andsec/hook/yun/fileman.so` 会被文件管理器和 WPS 组件加载。
  - 动态符号显示该 hook 主要拦截 X11/XCB、剪贴板、打印和网络连接相关符号，不拦截 `open/openat/fopen/read` 等文件读取入口。
  - 因此 hook 需要保留加载，但不是 WPS 读文件失败的根因。
- 关键证据：
  - 42 中新拉起的云盒文件管理器可处于 `UID=0`，环境包含 `USER=root`、`HOME=/home/root`、`LOGNAME=root`、`PKEXEC_UID=1000`。
  - 在同类 root 用户环境下手工启动 WPS 可复现“文档已被其它应用程序锁定，是否以只读模式打开？”提示。
  - 同一命令在设置 `USER=dingjing LOGNAME=dingjing` 后不再出现锁定提示，WPS 组件窗口能正常出现。
  - 65 当前链路可处于普通用户态，但同样带 `PKEXEC_UID=1000`，需要兼容两种启动形态。
- 根因：云盒启动链可能由 `fileboxd/pkexec` 以 root 环境拉起文件管理器，文件管理器再启动 WPS 时默认继承 root 的 `USER/LOGNAME/HOME`。WPS 依赖这些用户环境判断锁文件、配置目录和组件状态，导致文档打开停在锁定/初始化状态。
- 相关代码路径：`crates/filesystem-gui/src/apps.rs`

## 3. 修复方案

- 最小修复点：仅在 `FILESYSTEM_HOOK_MODE=yunbox` 下，外部应用 `Command` 启动前归一化用户环境。
- 用户名推导顺序：
  - 优先读取 `PKEXEC_UID`，通过 `/etc/passwd` 反查非 root 用户名。
  - 找不到时退回 `HOME=/home/<user>` 推导。
- 注入环境：
  - `USER=<真实桌面用户>`
  - `LOGNAME=<真实桌面用户>`
  - `USERNAME=<真实桌面用户>`
- 保持原有行为：
  - 云盒 hook 仍从 `BOXFlLESO` 注入到 `LD_PRELOAD`。
  - WPS 家族仍优先走 `wpsoffice /prometheus <path>`，`.desktop` 命令保留 fallback。
  - 非云盒模式不做用户环境归一化。

## 4. 验证

- 本地验证：
  - `cargo test -p filesystem-gui spawn_open_command_ -- --test-threads=1 --nocapture`
  - `cargo test -p filesystem-gui yunbox_wps_ -- --test-threads=1 --nocapture`
- 202 x86_64 构建验证：
  - `cargo test -p filesystem-gui spawn_open_command_ -- --test-threads=1 --nocapture`
  - `cargo test -p filesystem-gui yunbox_wps_ -- --test-threads=1 --nocapture`
  - `cargo build -p filesystem-gui --release`
  - release 二进制最高 GLIBC 版本为 `2.27`，兼容 65。
- 部署验证：
  - 42、65 均部署 `/usr/local/andsec/sandbox/bin/sandbox-nemo.bin`，sha256 为 `d15447c0ac90691c362af6129fb74eb6eb6aa4bca527071283af74d11158e269`。
  - 42 验证 `/yunbox/asdasasd.wps`、`/yunbox/rrr.et`、`/yunbox/sa.dps` 均进入对应 `wps/et/wpp /from_prome` 组件；组件环境中 `USER/LOGNAME/USERNAME=dingjing`，`LD_PRELOAD=/usr/local/andsec/hook/yun/fileman.so`。
  - 65 验证 `/yunbox/WPS表格工作表 2.et`、`/yunbox/WPS文字文档.wps`、`/yunbox/WPS演示文稿.dps` 均进入对应 `et/wps/wpp /from_prome` 组件；组件环境中 `USER/LOGNAME/USERNAME=dingjing`，`LD_PRELOAD=/usr/local/andsec/hook/yun/fileman.so`。

## 5. 风险摘要

| 项 | 结论 |
|----|------|
| 风险矩阵 | L2 |
| 命令权限 | C3，按用户要求远程部署到 42/65 测试系统 |
| 高风险门禁 | 否 |
| 破坏性操作 | 已备份旧 `/usr/local/andsec/sandbox/bin/sandbox-nemo.bin` 后替换 |
| 用户已有修改 | 有，基于前序云盒 WPS 打开修复继续修改 |
| 副作用/风险 | 如果某环境缺少 `PKEXEC_UID` 且 `HOME` 也不是 `/home/<user>`，不会强行改用户环境，避免误改非云盒场景 |

## 6. 修复总结

- 最终结果：42 和 65 的云盒 WPS 文字、表格、演示文件均已实机验证可打开。
- 根因定位：不是 hook 拦截文件读取；hook 需要保留。问题点是云盒/pkexec/root 启动链向 WPS 传播了 root 用户环境。
- 后续建议：若后续修改 `fileboxd.bin` 或云盒启动链，仍需保留 `PKEXEC_UID` 到真实桌面用户的环境归一化规则。
