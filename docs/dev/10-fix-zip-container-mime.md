# ZIP 容器 MIME 误判修复记录

> 文档元数据
> - 文件编号：10
> - 文档类型：fix
> - 文件路径：docs/dev/10-fix-zip-container-mime.md
> - 完成日期：2026-06-25
> - 需求级别：L2

## 1. 问题定义

- 问题现象：新增 `filesystem-mime` 后，`.odt` 被识别为 `application/zip`，导致无法匹配 Office/LibreOffice 这类文档应用。
- 预期行为：ODT/ODS/ODP/DOCX/XLSX/PPTX/EPUB/JAR/APK 这类 ZIP 容器专用格式应识别为专用 MIME；普通 zip 仍识别为 `application/zip`；强内容签名如 PDF 不能被扩展名覆盖。
- 影响范围：文件双击默认打开、打开方式列表、文件图标和文件属性类型标签。
- 不包含：完整实现 ZIP 中央目录解析或 shared-mime-info 全量 magic 语义。

## 2. 证据与根因

- 复现方式：构造 ODT 风格 ZIP，首个 entry 为 `mimetype`，内容为 `application/vnd.oasis.opendocument.text`，后续紧跟下一个 `PK` local header。
- 证据等级：E3，已补单元测试覆盖回归。
- 关键日志/堆栈/输入：`detect_bytes("a.odt", bytes)` 返回通用 zip。
- 根因：`zip_mimetype_entry()` 没有根据 ZIP local header 的 compressed size 截断 `mimetype` 内容，导致读取到后续 `PK` header 后匹配失败；同时 ZIP 内容识别失败时直接退为 `application/zip`，没有给 ZIP 容器专用扩展名留 fallback。
- 相关代码路径：`crates/filesystem-mime/src/lib.rs`

## 3. 修复方案

- 最小修复点：修正 `zip_mimetype_entry()` 的内容截断；ZIP 内部识别失败时，仅对 ZIP 容器家族扩展名使用专用 MIME fallback。
- 代码逻辑改动：新增 `zip_container_extension_mime()`；新增 `le_u32()` 读取 compressed size；强签名仍优先于扩展名。
- 影响的使用场景：ODT/ODS/ODP/DOCX/XLSX/PPTX/EPUB/JAR/APK 的打开方式和属性类型更准确。
- 不影响的使用场景：PDF/图片/ELF 等强签名识别、普通 `.zip` 文件识别。

## 4. 修复执行计划

| 步骤 | 修改内容 | 验证方式 | 状态 |
|------|----------|----------|------|
| 1 | 修正 ZIP `mimetype` 截取逻辑 | `cargo test -p filesystem-mime` | 已完成 |
| 2 | 增加 ZIP 容器专用扩展名 fallback | `cargo test -p filesystem-mime` | 已完成 |
| 3 | 补 ODT、普通 ZIP、PDF 强签名回归测试 | `cargo test -p filesystem-mime` | 已完成 |
| 4 | 跑 GUI 和 workspace 验证 | `cargo check -p filesystem-gui`、`cargo test -p filesystem-gui`、`make test` | 已完成 |

## 5. 风险摘要

| 项 | 结论 |
|----|------|
| 风险矩阵 | L2 |
| 命令权限 | C0/C1 |
| 高风险门禁 | 是，涉及文件内容识别和默认打开方式链路 |
| 破坏性操作 | 否 |
| 用户已有修改 | 是，基于当前未提交的 MIME 功能改动继续增量修改，未回退无关内容 |
| 用户确认 | 无 |
| 副作用/风险 | ZIP 内容截断依赖 local header size；带 data descriptor 的复杂 ZIP 仍可能退化到扩展名或通用 zip |

## 6. 验证

- 验证环境：本地 Linux workspace。
- 回归/相关验证：`cargo fmt --check`、`cargo test -p filesystem-mime`、`cargo check -p filesystem-gui`、`cargo test -p filesystem-gui`、`make test`、`git diff --check`。
- 结果：均通过。`make test` 覆盖 core 23 个、GUI 28 个、MIME 10 个单元测试。
- 未执行验证项：未用真实 LibreOffice 图形环境人工双击 `.odt` 样本。
- 残余风险：完整 ZIP 中央目录和 data descriptor 场景未实现。

## 7. 修复总结

- 最终结果：已完成，`.odt` 等 ZIP 容器文档不再被通用 zip MIME 抢占，普通 zip 和强内容签名优先级保持不变。
- 计划偏差：无。
- 后续建议：后续如遇更多容器格式误判，按“强签名优先、容器专用规则次之、通用容器最后”的规则扩展。
