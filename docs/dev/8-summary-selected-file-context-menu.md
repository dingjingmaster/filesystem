# 选中文件右键菜单开发结论摘要

> 文档元数据
> - 文件编号：8
> - 文档类型：summary
> - 文件路径：docs/dev/8-summary-selected-file-context-menu.md
> - 文档版本：v1.0.0
> - 完成日期：2026-06-25
> - 关联需求：左键选中文件后，右键显示文件菜单，支持默认应用打开、打开方式、复制、剪切、重命名、删除和属性
> - 关联调研：docs/dev/8-research-selected-file-context-menu.md
> - 关联计划：docs/dev/8-plan-selected-file-context-menu.md

## 1. 最终结果

- 原始需求：当用户左键选中文件后，右键显示文件菜单，菜单支持用默认应用打开、打开方式、复制、剪切、重命名、删除和属性；除文件夹外其它条目都应视作文件。
- 最终方案：新增单个选中非文件夹条目的文件菜单、本地 `.desktop`/`mimeapps.list` 解析、打开方式弹窗、默认应用打开、文件属性摘要和文件删除确认；普通文件双击也复用默认应用打开逻辑；补齐 `Makefile` 等常见无扩展名文本文件的 MIME 识别和文本应用匹配；没有默认应用但存在打开方式候选时，自动选择最匹配的候选应用打开。
- 完成状态：完成。
- 需求变更：无。

## 2. 关键改动

- 修改文件：
  - `crates/filesystem-core/src/lib.rs`
  - `crates/filesystem-gui/src/apps.rs`
  - `crates/filesystem-gui/src/main.rs`
  - `crates/filesystem-gui/src/model.rs`
  - `crates/filesystem-gui/src/app.rs`
  - `crates/filesystem-gui/src/tasks.rs`
  - `crates/filesystem-gui/src/components.rs`
  - `docs/overview-product.md`
  - `docs/overview-product-dev.md`
  - `docs/dev/README.md`
  - `docs/dev/8-research-selected-file-context-menu.md`
  - `docs/dev/8-plan-selected-file-context-menu.md`
- 代码逻辑改动：
  - core 新增 `FileProperties` 和 `file_properties`，用于读取普通文件属性。
  - GUI 新增 `apps` 模块，递归扫描本地 applications 目录，解析 `.desktop`、`Icon`、通用/桌面专用 `mimeapps.list`、MIME 扩展名映射、常见无扩展名文本文件映射和 Exec field code。
  - GUI 应用图标复用本地图标主题查找；无图标或找不到图标时回退到 `icons/app.svg`。
  - GUI 启动时后台加载应用注册表；文件菜单和双击非目录条目优先使用默认应用，没有默认应用时按 MIME 匹配分数选择打开方式列表中的首选候选应用，再通过 `Command::spawn` 直接启动；没有任何候选应用时才提示用户走“打开方式”。
  - 默认应用查询补齐 `XDG_CURRENT_DESKTOP` 对应的桌面专用文件，如 `gnome-mimeapps.list`，并读取 `XDG_CONFIG_DIRS`。
  - `Makefile`/`GNUmakefile`/`.mk` 识别为 `text/x-makefile`，`Dockerfile`、`README`、`LICENSE`、`.gitignore` 等识别为 `text/plain`；支持 `text/plain` 的应用可打开其它 `text/*` 文件，默认应用在专有文本 MIME 无配置时回落到 `text/plain`。
  - 打开方式候选按默认应用、精确 MIME、文本兜底、类型通配、全局通配和名称排序，`doc/docx` 等文档在缺少默认应用时可选中最匹配的文档应用。
  - GUI 新增 `ContextMenuState::File`、`OpenWithDialog`、文件属性状态和文件操作消息。
  - 文件复制/剪切复用内部 `ClipboardState`，重命名复用现有内联重命名流程，删除复用确认弹窗和后台 `delete_entry`。
- 影响的使用场景：单个选中非文件夹条目右键操作、普通文件双击打开、打开方式选择、文件属性查看、文件复制/剪切/重命名/删除。
- 不影响的使用场景：空白处右键菜单、文件夹菜单、目录导航、搜索、框选、多选状态。
- 计划偏差：补充实现了非目录条目双击默认应用打开，以满足既有“文件和文件夹双击打开、单击选中”的交互规则。

## 3. 安全门禁结果

| 项 | 结论 |
|----|------|
| 风险矩阵 | L4 |
| 命令权限 | C0/C1 |
| 高风险项 | 有，涉及文件删除入口和外部应用启动 |
| 破坏性操作 | 未执行真实用户目录删除；删除相关验证仅使用既有临时目录测试 |
| 用户已有修改 | 编码前工作区干净；本次未回退用户修改 |
| 用户确认事项 | 无；运行时删除仍必须用户在确认弹窗中确认 |
| 副作用/风险 | 文件删除无回收站；外部应用启动依赖本地 `.desktop` 的 Exec 声明质量 |

## 4. 验证结果

- 验证环境：本地 Linux workspace。
- 执行验证：
  - `cargo fmt --check`
  - `cargo check -p filesystem-gui`
  - `cargo test -p filesystem-core`
  - `cargo test -p filesystem-gui`
  - `make test`
  - `git diff --check`
- 结果：全部通过；core 23 个测试通过，GUI 27 个测试通过，`make test` 聚合测试通过。
- 未执行验证项：未真实启动 GUI 外部应用；未在 X11/Wayland 会话中人工检查打开方式和文件属性视觉。
- 残余风险：不同发行版 `.desktop` 和 `mimeapps.list` 兼容性仍有限；没有任何应用声明支持该 MIME 时仍不能直接双击打开；任意无扩展名文本文件不会读取内容嗅探，仍可能使用未知类型保守回退。

## 5. Bug 修复验证（按需）

- 证据等级：E1。用户反馈 `Makefile` 等实际文本文件在“打开方式”中没有可用应用。
- 根因：MIME 识别只覆盖扩展名，`Makefile` 这类无扩展名文本文件落到 `application/octet-stream`，导致声明 `text/plain` 的编辑器被过滤掉。
- 修复：补齐常见无扩展名文本文件映射；`text/plain` 应用可匹配其它 `text/*` 文件；专有文本 MIME 默认应用缺失时回落到 `text/plain` 默认应用。
- 回归验证：新增 GUI 单测覆盖 `Makefile`、`GNUmakefile`、`.mk`、`Dockerfile`、`.gitignore` 的 MIME 映射，覆盖 `text/plain` 应用打开 `text/x-makefile`，并确认未知二进制类型仍不误用文本编辑器默认应用。
- 证据等级：E1。用户反馈 `doc/docx` 等文档没有默认应用时双击不能打开，但“打开方式”列表已有候选应用。
- 根因：双击路径只查询默认应用，不会从打开方式候选列表中挑选首选应用。
- 修复：新增首选应用选择规则，默认应用优先；无默认应用时从候选列表按 MIME 匹配质量选择首个应用，并让打开方式弹窗默认选中同一候选。
- 回归验证：新增 GUI 单测覆盖 `docx` 在无默认应用时从泛用应用、精确文档应用和全局通配应用中选中精确文档应用。

## 6. 后续事项（按需）

- 技术债：当前 MIME 识别是扩展名和常见文件名映射，不读取文件内容，也不是完整 MIME 数据库。
- 后续建议：如果要支持“总是用于该文件类型”，需要单独设计写入 `mimeapps.list` 的权限边界、冲突处理和验证。
