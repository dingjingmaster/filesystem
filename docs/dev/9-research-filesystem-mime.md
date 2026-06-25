# filesystem-mime 调研报告

> 文档元数据
> - 文件编号：9
> - 文档类型：research
> - 文件路径：docs/dev/9-research-filesystem-mime.md
> - 文档版本：v1.0.0
> - 最后更新：2026-06-25
> - 关联需求：实现少依赖 `filesystem-mime`，默认集成常见文件类型识别，无法识别时尝试解析系统 `/usr/share/mime` 这类 shared-mime-info 数据文件。

## 1. 问题与边界

- 问题描述：现有 GUI 的文件类型识别分散在 `apps.rs` 和 `icons.rs`，主要依赖扩展名；部分文本文件、Office 文档或真实内容与扩展名不一致的文件，会导致默认打开方式和图标选择不准确。
- 调研目的：确认在不调用 `file` 命令、不依赖 libmagic/桌面服务、不新增第三方 crate 的前提下，如何提供可维护的 MIME 识别模块。
- 包含：内置常见扩展名、常见无扩展名文本名、常见二进制签名、结构化文本识别、shared-mime-info `globs2` 和 `magic` 只读 fallback。
- 不包含：完整实现 libmagic、完整实现 shared-mime-info 复杂嵌套 magic 规则、写入系统 MIME 数据库、写入默认应用配置。
- 非目标：不通过 DBus/GVFS/portal/xdg-open 或桌面 MIME 服务完成类型识别。
- 禁止触碰范围：不修改系统 `/usr/share/mime`，不执行删除或覆盖用户文件的验证。

## 2. 当前证据

- 现有实现/现状：`apps.rs` 自带扩展名到 MIME 的局部映射；`icons.rs` 通过扩展名选择主题 MIME 图标；`DisplayEntry` 不缓存 MIME 信息；文件属性类型标签依赖旧映射。
- 已知约束：所有可能阻塞 UI 的文件读取必须放到 iced 后台任务；文件双击和打开方式选择不能在 UI 线程读取文件内容。
- 关键日志/数据/用户反馈：用户要求整合类似 `file` 的能力，但不希望使用 `file` 命令；用户明确要求少依赖，默认可识别，系统存在 `/usr/share/mime` 时可尝试解析。
- Bug 证据等级（按需）：不适用，属于功能增强。
- 证据不足项：不同发行版 shared-mime-info 的 `magic` 复杂规则覆盖度需要后续人工样本验证。

## 3. 安全门禁摘要

| 项 | 结论 |
|----|------|
| 风险矩阵初判 | L3，跨 MIME、GUI、图标、打开方式和文档边界 |
| 命令权限 | C0/C1，只读探查与本仓库内代码/文档修改、测试 |
| 高风险开发门禁 | 是，涉及文件系统内容读取和 GUI 后台任务边界 |
| 破坏性操作 | 否 |
| 用户已有修改 | 有，基于当前已修改工作区继续增量修改，未回退无关内容 |
| 用户确认事项 | 无 |

## 4. 候选方案

| 方案 | 核心思路 | 优点 | 风险/代价 | 适用条件 |
|------|----------|------|-----------|----------|
| A | 调用 `file` 命令或绑定 libmagic | 识别覆盖度高 | 依赖外部命令/库，与少依赖和独立运行目标冲突 | 不适合当前目标 |
| B | 引入现成 MIME crate | 实现成本低 | 增加依赖，覆盖能力和系统数据库解析边界需再审 | 不符合“越少依赖越少”的优先级 |
| C | 新增无第三方依赖 `filesystem-mime` crate | 模块边界清晰，默认规则可控，可按需解析系统 MIME 数据文件 | 需要自行维护常见规则；shared-mime-info magic 先只覆盖保守子集 | 推荐 |

## 5. 推荐结论

- 推荐方案：采用方案 C，新增 `crates/filesystem-mime`，提供 `detect_path`、`detect_name`、`detect_bytes` 和 `MimeInfo`。
- 取舍理由：把文件类型识别集中到单独 crate，GUI 只消费缓存的 MIME；内置规则覆盖常见办公、媒体、归档、源码和无扩展名文本文件；系统 fallback 只读解析本地 shared-mime-info 数据，不调用桌面服务。
- 需要进入 Plan 的关键约束：内容读取上限固定；目录装饰后台执行；打开方式、图标和属性页使用缓存 MIME；系统 magic 复杂子规则不做激进匹配。
- 需要用户确认的问题：无。
- 后续验证方向：单元测试覆盖内置规则、签名优先级、zip office 检测、`globs2` 和 magic 解析；workspace 测试确认 GUI 集成未回归。

## 6. 参考资料（按需）

- 本机 `/usr/share/mime/globs2`：shared-mime-info 扩展名/通配规则。
- 本机 `/usr/share/mime/magic`：shared-mime-info `MIME-Magic` 文件，作为保守 fallback。
