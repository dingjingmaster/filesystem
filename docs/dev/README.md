# 需求与任务索引

> 记录 L1+ 新增任务、问题、调研、计划、总结或评审文档。本索引是本地上下文，默认不要求提交。新需求/问题优先读取本索引，再按相关性最多展开 3-5 个 Summary、task 或 fix 文档，避免加载完整历史上下文。

## 编号规则

- 文件编号使用从 `1` 开始递增的正整数，不要求固定位数。
- 编号全局只在 `docs/dev/` 下递增，不按类型分别编号。
- 新增任务、问题修复、调研、计划、总结、评审或需求变更文档前，先检查本索引和 `docs/dev/` 现有文件名，取最大编号 + 1。
- 同一需求的多份文档使用同一编号，例如 `2-research-xxx.md`、`2-plan-xxx.md`、`2-summary-xxx.md`。
- 编号一旦分配不得复用；取消、废弃、拆分、合并也要在索引中保留记录并标注状态。
- 文件命名格式：`N-[type]-[slug].md`，其中 `type` 可取 `summary`、`task`、`fix`、`research`、`plan`、`review`。

## 索引

| ID | 日期 | 级别 | 类型 | 文档 | 状态 | 摘要 |
|----|------|------|------|------|------|------|
| 1 | 2026-06-24 | L4 | research | [1-research-cosmic-files.md](1-research-cosmic-files.md) | 已完成 | 调研 cosmic-files 依赖、模块结构和作为独立少依赖文件管理器基础的可行性。 |
| 1 | 2026-06-24 | L4 | plan | [1-plan-local-linux-file-manager.md](1-plan-local-linux-file-manager.md) | 部分完成 | 明确仅支持 Linux 本地文件系统、GUI only、固定 wgpu 渲染的架构计划；已落地暗色只读 GUI、800x600 最小窗口、历史导航、地址栏文件名正则搜索、流式图标视图、列表视图、本地图标策略、单击选中/框选多选/双击打开。 |
| 1 | 2026-06-24 | L4 | summary | [1-summary-local-linux-file-manager.md](1-summary-local-linux-file-manager.md) | 已完成 | 总结首个 Rust workspace、只读 core 扫描/文件名正则搜索库、wgpu 暗色 iced GUI、800x600 最小窗口、流式图标视图、列表视图、本地图标策略、单击选中/框选多选/双击打开、验证结果和未覆盖风险。 |
| 2 | 2026-06-25 | L4 | research | [2-research-context-menu-file-ops-properties.md](2-research-context-menu-file-ops-properties.md) | 已完成 | 调研文件视图空白区右键菜单、新建/粘贴/终端/属性弹窗实现边界，明确首版剪贴板只解析文本路径或 file URI。 |
| 2 | 2026-06-25 | L4 | plan | [2-plan-context-menu-file-ops-properties.md](2-plan-context-menu-file-ops-properties.md) | 已完成 | 规划 core 写操作、GUI 右键菜单、内联重命名、属性弹窗、终端启动和验证步骤。 |
| 2 | 2026-06-25 | L4 | summary | [2-summary-context-menu-file-ops-properties.md](2-summary-context-menu-file-ops-properties.md) | 已完成 | 总结空白区右键菜单、新建/重命名、文本路径粘贴、全选、终端打开、属性查看、验证结果和残余风险。 |
| 3 | 2026-06-25 | L4 | research | [3-research-properties-permission-edit.md](3-research-properties-permission-edit.md) | 已完成 | 调研属性弹窗事件隔离、拖拽、按钮统一、对齐和当前文件夹权限修改方案。 |
| 3 | 2026-06-25 | L4 | plan | [3-plan-properties-permission-edit.md](3-plan-properties-permission-edit.md) | 已完成 | 规划 core 权限修改 API、GUI 属性弹窗交互修正、权限保存后台任务和验证步骤。 |
| 3 | 2026-06-25 | L4 | summary | [3-summary-properties-permission-edit.md](3-summary-properties-permission-edit.md) | 已完成 | 总结属性弹窗事件隔离、拖拽、关闭按钮统一、对齐和当前文件夹权限修改实现。 |
| 4 | 2026-06-25 | L4 | research | [4-research-gui-module-refactor.md](4-research-gui-module-refactor.md) | 已完成 | 调研 GUI 单文件实现的维护问题，确定按 app/model/tasks/icons/components/utils/style 等职责拆分且不改变行为。 |
| 4 | 2026-06-25 | L4 | plan | [4-plan-gui-module-refactor.md](4-plan-gui-module-refactor.md) | 已完成 | 规划 GUI 模块拆分、可见性调整、测试迁移、开发概览更新和完整验证。 |
| 4 | 2026-06-25 | L4 | summary | [4-summary-gui-module-refactor.md](4-summary-gui-module-refactor.md) | 已完成 | 总结 GUI crate 拆分为 app/model/tasks/icons/components/utils/config/style 模块、验证结果和残余人工 GUI 验证风险。 |
| 5 | 2026-06-25 | L1 | summary | [5-summary-properties-vertical-alignment.md](5-summary-properties-vertical-alignment.md) | 已完成 | 调整属性弹窗标题栏、概要区、属性行、权限行和底部按钮内容的垂直居中布局。 |
| 6 | 2026-06-25 | L1 | summary | [6-summary-properties-back-icon.md](6-summary-properties-back-icon.md) | 已完成 | 将设置自定义权限页返回按钮从文本箭头改为 `icons/left.svg`，并使用居中图标按钮布局。 |
| 7 | 2026-06-25 | L4 | research | [7-research-selected-folder-context-menu.md](7-research-selected-folder-context-menu.md) | 已完成 | 调研单个选中文件夹右键菜单、内部复制/剪切、递归删除确认、终端打开和属性目标切换方案。 |
| 7 | 2026-06-25 | L4 | plan | [7-plan-selected-folder-context-menu.md](7-plan-selected-folder-context-menu.md) | 已完成 | 规划 core 删除 API、GUI 文件夹菜单、内部剪贴板、剪切变淡、删除确认弹窗和验证步骤。 |
| 7 | 2026-06-25 | L4 | summary | [7-summary-selected-folder-context-menu.md](7-summary-selected-folder-context-menu.md) | 已完成 | 总结选中文件夹右键菜单、内部复制/剪切、剪切变淡、删除确认、后台递归删除和验证结果。 |
| 8 | 2026-06-25 | L4 | research | [8-research-selected-file-context-menu.md](8-research-selected-file-context-menu.md) | 已完成 | 调研单个选中文件右键菜单、默认应用/打开方式本地解析、文件属性和文件操作方案。 |
| 8 | 2026-06-25 | L4 | plan | [8-plan-selected-file-context-menu.md](8-plan-selected-file-context-menu.md) | 已完成 | 规划 core 文件属性 API、GUI 应用注册表、文件菜单、打开方式弹窗、文件属性弹窗和验证步骤。 |
| 8 | 2026-06-25 | L4 | summary | [8-summary-selected-file-context-menu.md](8-summary-selected-file-context-menu.md) | 已完成 | 总结选中非文件夹条目右键菜单、默认应用/打开方式本地解析、无默认应用时首选候选应用打开、常见无扩展名文本文件匹配、文件属性、文件操作和验证结果。 |
| 9 | 2026-06-25 | L3 | research | [9-research-filesystem-mime.md](9-research-filesystem-mime.md) | 已完成 | 调研少依赖文件类型识别模块，明确不调用 `file` 命令、不依赖桌面 MIME 服务，并只读 fallback 解析 shared-mime-info 数据。 |
| 9 | 2026-06-25 | L3 | plan | [9-plan-filesystem-mime.md](9-plan-filesystem-mime.md) | 已完成 | 规划新增无第三方依赖 `filesystem-mime` crate、GUI 缓存 MIME 接入、系统 `globs2`/`magic` fallback 和验证步骤。 |
| 9 | 2026-06-25 | L3 | summary | [9-summary-filesystem-mime.md](9-summary-filesystem-mime.md) | 已完成 | 总结 `filesystem-mime` 内置内容/名称识别、shared-mime-info fallback、GUI 图标/打开方式/属性统一使用缓存 MIME、验证结果和残余风险。 |
| 10 | 2026-06-25 | L2 | fix | [10-fix-zip-container-mime.md](10-fix-zip-container-mime.md) | 已完成 | 修复 `.odt` 等 ZIP 容器文档被通用 zip MIME 抢占的问题，补 ODT、普通 ZIP 和强内容签名回归测试。 |
| 11 | 2026-06-25 | L2 | fix | [11-fix-text-editor-fallback.md](11-fix-text-editor-fallback.md) | 已完成 | 修复 `.desktop/.c/.cpp/.md` 等文本类文件打开兜底，支持 `text/plain` 和 `TextEditor` 分类 fallback，过滤 `Terminal=true` 应用。 |
| 12 | 2026-06-25 | L3 | research | [12-research-symlink-badge-open.md](12-research-symlink-badge-open.md) | 已完成 | 调研软链接目标类型、断链状态、图标角标、菜单分类和双击目标打开方案。 |
| 12 | 2026-06-25 | L3 | plan | [12-plan-symlink-badge-open.md](12-plan-symlink-badge-open.md) | 已完成 | 规划 core `SymlinkTarget`、GUI 角标渲染、有效类型打开路径、断链弹窗和验证步骤。 |
| 12 | 2026-06-25 | L3 | summary | [12-summary-symlink-badge-open.md](12-summary-symlink-badge-open.md) | 已完成 | 总结软链接角标、断链角标、目标路径打开、属性跟随和验证结果。 |
| 13 | 2026-06-25 | L3 | research | [13-research-large-directory-performance.md](13-research-large-directory-performance.md) | 已完成 | 调研大目录打开卡顿根因，明确分批加载、基础条目先显示、后台装饰、图标缓存和视图虚拟化方案。 |
| 13 | 2026-06-25 | L3 | plan | [13-plan-large-directory-performance.md](13-plan-large-directory-performance.md) | 已完成 | 规划 core 分批 scanner、GUI stream 事件、MIME/图标后台装饰、缓存、虚拟化和验证步骤。 |
| 13 | 2026-06-25 | L3 | summary | [13-summary-large-directory-performance.md](13-summary-large-directory-performance.md) | 已完成 | 总结大目录分批显示、后台装饰、图标缓存、图标/列表视图虚拟化、验证结果和残余风险。 |
| 14 | 2026-06-26 | L4 | research | [14-research-file-operation-progress.md](14-research-file-operation-progress.md) | 已完成 | 调研复制/剪切粘贴进度显示、跨文件系统剪切退化、取消和进度 UI 边界。 |
| 14 | 2026-06-26 | L4 | plan | [14-plan-file-operation-progress.md](14-plan-file-operation-progress.md) | 已完成 | 规划 core 带进度粘贴 API、GUI 文件操作 stream、侧栏圆形总进度和详情条形进度。 |
| 14 | 2026-06-26 | L4 | summary | [14-summary-file-operation-progress.md](14-summary-file-operation-progress.md) | 已完成 | 总结复制/剪切进度、跨文件系统剪切复制后删除、详情失焦隐藏、取消入口、最多 3 个复制线程和验证结果。 |
| 15 | 2026-06-26 | L2 | task | [15-task-rust-edition-2021.md](15-task-rust-edition-2021.md) | 已完成 | 将 workspace Rust edition 从 2024 调整为 2021，并将 `rust-version` 修正为 1.88 以匹配当前依赖 MSRV。 |
| 16 | 2026-06-26 | L2 | task | [16-task-renderer-selection.md](16-task-renderer-selection.md) | 已完成 | GUI 同时支持 `wgpu` 和 `tiny-skia`，默认优先 wgpu，探测失败或显式 `ICED_BACKEND=tiny-skia` 时使用 tiny-skia。 |
| 17 | 2026-06-26 | L2 | fix | [17-fix-desktop-exec-field-codes.md](17-fix-desktop-exec-field-codes.md) | 已完成 | 修复 `.desktop` Exec 字段码展开，避免未知或废弃 `%x` 字段码原样传给外部应用。 |
| 18 | 2026-06-26 | L2 | fix | [18-fix-uos-software-renderer-refresh.md](18-fix-uos-software-renderer-refresh.md) | 已完成 | UOS/虚拟机软件渲染环境下自动避开 wgpu，回退 tiny-skia 以规避交互刷新异常。 |
| 19 | 2026-06-26 | L2 | task | [19-task-shortcuts-multi-select-menu.md](19-task-shortcuts-multi-select-menu.md) | 已完成 | 为选中文件/文件夹新增复制、剪切、删除快捷键，为空选择新增粘贴快捷键，并为多选新增复制/剪切/删除右键菜单。 |
| 20 | 2026-06-27 | L2 | task | [20-task-click-range-selection.md](20-task-click-range-selection.md) | 已完成 | 为文件视图新增 Ctrl 点击多点选择和 Shift 点击范围选择。 |
| 21 | 2026-06-29 | L2 | task | [21-task-filesystem-ini-config.md](21-task-filesystem-ini-config.md) | 已完成 | 启动时读取可执行文件同级 `filesystem.ini`，支持覆盖左侧标题栏名称和右键菜单终端命令。 |
| 22 | 2026-06-29 | L2 | task | [22-task-blank-menu-custom-commands.md](22-task-blank-menu-custom-commands.md) | 已完成 | 通过 `filesystem.ini` 的 `[blank-menu.*]` section 为空白处右键菜单新增自定义命令。 |
| 23 | 2026-06-29 | L2 | task | [23-task-current-folder-auto-refresh.md](23-task-current-folder-auto-refresh.md) | 已完成 | 监听当前打开文件夹的直接子项变化，并在新增、删除、重命名或修改时自动刷新。 |
| 24 | 2026-06-29 | L2 | task | [24-task-open-with-default-app.md](24-task-open-with-default-app.md) | 已完成 | 打开方式弹窗支持勾选设为默认打开方式，写入当前桌面优先的用户级 MIME apps 配置并同步当前进程默认应用。 |
| 25 | 2026-06-29 | L3 | fix | [25-fix-wps-docx-open.md](25-fix-wps-docx-open.md) | 已完成 | 远端 GUI 双击和 strace 取证修复 WPS 默认打开差异：支持无 MimeType 默认应用、按 GIO 规则处理 desktop-specific mimeapps、保留 WPS 扩展名 MIME、WPS `%U` 传本地路径并回收外部应用 wrapper 子进程。 |
