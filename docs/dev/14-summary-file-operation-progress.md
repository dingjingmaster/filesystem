# 文件复制剪切进度开发结论摘要

> 文档元数据
> - 文件编号：14
> - 文档类型：summary
> - 文件路径：docs/dev/14-summary-file-operation-progress.md
> - 文档版本：v1.0.0
> - 完成日期：2026-06-26
> - 关联需求：文件复制、剪切和粘贴时显示总体圆形进度；点击展开每次粘贴的条形进度、当前文件、速度和取消按钮；跨文件系统剪切退化为复制后删除。
> - 关联调研：docs/dev/14-research-file-operation-progress.md
> - 关联计划：docs/dev/14-plan-file-operation-progress.md

## 1. 最终结果

- 原始需求：
  - 判断现有剪切是否同文件系统用正常剪切、跨文件系统复制后删除。
  - 复制/剪切粘贴时提供整体进度，点击显示每次粘贴的详情、当前文件和进度。
  - 进度入口指定为侧栏“文件”右侧的圆形总进度；点击展开详情；每次粘贴详情包含条形进度、当前文件、速度和 `x` 取消按钮。
  - 验收补充：详情 popup 失焦自动隐藏，少量操作不显示滚动条，文件操作复制线程最多并发 3 个。
- 最终方案：
  - core 新增带进度的粘贴 API，复制按文件分块汇报进度，目录先统计总量；剪切先尝试 `rename`，跨文件系统 `EXDEV` 时复制成功后删除源路径。
  - GUI 每次粘贴创建独立 operation id 和取消 token，最多 3 个 operation 启动 `Task::stream` 复制线程，其余显示为排队状态；侧栏标题居中，标题右侧显示聚合圆形进度，展开后以窗口上层 popup 按每次粘贴显示详情。
- 完成状态：完成。
- 需求变更：用户补充明确进度入口、详情展开位置、详情内容和取消按钮语义；实现按该形态落地。

## 2. 关键改动

- 修改文件：
  - `crates/filesystem-core/src/lib.rs`
  - `crates/filesystem-gui/src/model.rs`
  - `crates/filesystem-gui/src/tasks.rs`
  - `crates/filesystem-gui/src/app.rs`
  - `crates/filesystem-gui/src/components.rs`
  - `crates/filesystem-gui/src/style.rs`
  - `docs/overview-product.md`
  - `docs/overview-product-dev.md`
  - `docs/dev/README.md`
  - `docs/dev/14-research-file-operation-progress.md`
  - `docs/dev/14-plan-file-operation-progress.md`
- 代码逻辑改动：
  - core 新增 `PasteProgress`、`PasteProgressEvent`、`PasteProgressPhase` 和 `FileOperationCancelToken`。
  - core 新增 `paste_paths_with_progress`；旧 `paste_paths` 复用新实现但忽略进度事件。
  - core 复制路径改为预扫描统计总字节/条目、分块读写并汇报当前文件进度；取消会停止后续操作并清理当前未完成目标。
  - core 剪切路径保留同文件系统 `rename` 快路径；跨文件系统 `EXDEV` 时执行复制后删除源路径。
  - GUI 新增 `FileOperationState` 和 `FileOperationStatus`，支持多次粘贴并发显示、排队启动、速度采样、展开/隐藏详情、取消和完成 3 秒后移除。
  - 侧栏标题“文件”居中显示；标题右侧新增动态 SVG 圆形总进度按钮。
  - 详情面板作为窗口上层 popup 覆盖在标题下方，加宽到可覆盖右侧内容上方，显示每次粘贴的条形进度、当前文件、速度和 `x`，不挤压侧栏或右侧主界面布局。
  - 详情 popup 展开后点击 popup 外部区域会自动隐藏；详情不超过 5 条时不使用 scrollable，超过 5 条才显示滚动区域。
  - 运行中点击子进度条 `x` 会立即取消对应 token 并移除该详情条；后台结束事件回来后只触发目录刷新，不再恢复该详情条。
  - GUI 通过 `active_file_operation_ids` 控制同时最多 3 个实际复制线程；排队 operation 仍显示详情条和总进度占位，前序操作结束后自动启动下一条。
  - GUI 文件操作 stream 使用独立复制线程和标准通道桥接，stream 每收到一个 core 进度事件就 `await` 发送给 UI，避免复制结束后才批量刷新。
- 影响的使用场景：
  - 内部复制/剪切后粘贴。
  - 系统文本剪贴板路径粘贴。
  - 大文件、目录、多次粘贴和跨文件系统剪切。
- 不影响的使用场景：
  - 新建、重命名、删除确认、属性、打开方式、MIME、目录分批加载和虚拟化浏览。
- 计划偏差：
  - 取消时只清理当前未完成目标；已完整复制或已经移动成功的条目保留，避免剪切场景产生二次数据风险。
  - 验收阶段将详情面板从参与侧栏排版的内容改为覆盖层 popup，并修正进度事件发送方式以实现复制过程中真实刷新。
  - 验收优化阶段将标题改为居中、加宽详情 popup，并调整取消入口为点击后立即删除详情条。
  - 验收优化阶段新增详情失焦自动隐藏、少量详情不显示滚动条，以及最多 3 个复制线程的 GUI 队列调度。

## 3. 安全门禁结果

| 项 | 结论 |
|----|------|
| 风险矩阵 | L4 |
| 命令权限 | C0/C1 |
| 高风险项 | 有，涉及文件系统写操作、复制后删除、取消清理和异步任务状态 |
| 破坏性操作 | 未对真实用户目录执行测试性复制/移动/删除；运行时跨文件系统剪切会在复制成功后删除用户选择的源路径 |
| 用户已有修改 | 无，编码前工作区干净 |
| 用户确认事项 | 无阻塞；进度 UI 形态已由用户补充确认 |
| 副作用/风险 | 文件操作由用户触发后真实修改目标目录；取消不回滚已完成条目 |

## 4. 验证结果

- 验证环境：本地 Linux workspace。
- 系统信息（按需）：未执行真实图形会话和真实跨文件系统手工验证。
- 执行验证：
  - `cargo fmt --check`
  - `cargo test -p filesystem-core`
  - `cargo check -p filesystem-gui`
  - `cargo test -p filesystem-gui`
  - `cargo test`
  - `make test`
  - `git diff --check`
- 结果：
  - core 31 个测试通过，覆盖递归复制、同文件系统移动、进度事件、取消清理、同次粘贴唯一目标和跨设备错误识别。
  - GUI 37 个测试通过，覆盖文件操作进度百分比、完成态、运行态文案/速度、排队态文案、最多 3 个复制线程启动、完成后启动下一条排队操作和运行中取消后立即删除详情条。
  - workspace 和 `make test` 聚合验证通过。
- 未执行验证项：
  - 未在真实 X11/Wayland 会话人工确认圆形进度、详情面板位置、展开/隐藏和取消点击视觉效果。
  - 未用真实两个不同文件系统执行跨设备剪切；自动化覆盖 `EXDEV` 识别和复制/移动基础路径。
- 残余风险：
  - 大目录预扫描阶段需先统计总量，取消已接入但百分比在统计完成后才准确。
  - 复制/剪切过程中已完成的条目不会因取消而回滚。

## 5. Bug 修复验证（按需）

- 原问题复现：
  - `PasteAction::Cut` 旧实现只调用 `fs::rename`，跨文件系统会返回 `EXDEV` 并失败。
- 证据等级：E2。
- 根因：
  - 移动逻辑没有识别跨设备错误并退化为复制后删除。
- 回归/相关验证：
  - 旧同文件系统移动测试保持通过。
  - 新增 `EXDEV` 识别测试。
  - 新增进度和取消相关测试。

## 6. 后续事项（按需）

- 技术债：
  - 真实图形会话视觉 smoke test 仍需人工执行。
  - 真实跨文件系统剪切需要在具备两个挂载点的测试环境中补充人工或集成验证。
- 后续建议：
  - 如果后续要支持暂停/恢复、失败重试或后台任务历史，需要单独设计任务持久化和更完整的失败恢复策略。
