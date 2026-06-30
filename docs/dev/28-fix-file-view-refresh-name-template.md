# 文件视图刷新、长文件名和模板菜单修复记录

> 文档元数据
> - 文件编号：28
> - 文档类型：fix
> - 文件路径：docs/dev/28-fix-file-view-refresh-name-template.md
> - 文档版本：v1.0.0
> - 最后更新：2026-06-29
> - 需求级别：L2
> - 关联需求：修复目录回退残影、长文件名显示和模板子菜单显示名

## 1. 目标

- 要解决的问题：
  - 从文件很多的目录回退到文件较少的目录后，主文件区可能残留上一目录的绘制内容，框选后才消失。
  - 长文件名在图标视图和列表视图中只做前缀截断，结尾和扩展名不可见。
  - “新建模板文件”子菜单显示完整模板文件名，用户要求菜单项不显示扩展名，创建后的文件仍保留扩展名。
- 成功标准：
  - 新目录开始加载时清空旧条目并重置主文件区滚动状态。
  - 长文件名按中间省略显示，保留开头和结尾；鼠标悬停条目时显示完整名称。
  - 模板子菜单显示去掉最后一个扩展名后的名称，复制创建时仍使用模板原始文件名。

## 2. 背景与边界

- 背景：文件视图使用虚拟化和 scrollable 绝对偏移构建可视条目；从大目录切到小目录时，旧滚动偏移和旧渲染状态可能导致视觉残留。
- 包含：目录加载 Started 阶段的视图状态复位；图标/列表文件名中间省略；条目 tooltip；模板菜单显示名修正。
- 不包含：重写虚拟化布局；调整重命名编辑器的长名称换行策略；模板目录递归扫描。
- 风险：真实 GUI 残影需要图形会话复测；自动化只能覆盖状态机和纯函数行为。

## 3. 实现记录

- `crates/filesystem-gui/src/app.rs`
  - 为主文件区 `scrollable` 增加固定 widget id。
  - 目录加载 `Started` 时同步清空旧条目，并通过 `operation::snap_to(..., RelativeOffset::START)` 把滚动状态归零。
  - 图标视图和列表视图文件项外层增加完整名称 tooltip。
- `crates/filesystem-gui/src/utils.rs`
  - 将 `short_name` 和 `short_list_text` 改为中间省略，尾部多保留一个字符以优先保留扩展名。
- `crates/filesystem-gui/src/tasks.rs`
  - 模板文件扫描结果的 `label` 改为 `file_stem()`，菜单不显示最后一个扩展名；源 `path` 不变，创建时仍按原文件名复制。
- `crates/filesystem-gui/src/style.rs`
  - 增加文件名 tooltip 容器样式。

## 4. 验证记录

| 验证项 | 命令/步骤 | 结果 | 备注 |
|--------|-----------|------|------|
| 中间省略定向测试 | `cargo test -p filesystem-gui middle_ellipsis` | 通过 | 覆盖短名、长名、极小长度 |
| 目录切换状态定向测试 | `cargo test -p filesystem-gui directory_started_clears_old_entries_and_resets_scroll` | 通过 | 覆盖旧条目清空和滚动归零 |
| 模板显示名定向测试 | `cargo test -p filesystem-gui template_file_label_removes_only_last_extension` | 通过 | 覆盖 `.docx`、`.tar.gz` 和无扩展名 |
| 格式检查 | `cargo fmt --check` | 通过 | 无格式差异 |
| GUI 编译检查 | `cargo check -p filesystem-gui` | 通过 | 类型检查通过 |
| core 全量测试 | `cargo test -p filesystem-core` | 通过 | 33 passed |
| GUI 全量测试 | `cargo test -p filesystem-gui` | 通过 | 104 passed |
| diff 空白检查 | `git diff --check` | 通过 | 无空白错误 |

- 未执行验证项：真实 GUI 回退残影和悬停 tooltip 人工验证。

## 5. 总结

- 最终结果：文件视图在目录切换开始时复位滚动和旧条目；长文件名用中间省略显示并支持悬停查看完整名称；模板子菜单显示去扩展名名称，创建后仍保留模板文件扩展名。
- 残余风险：渲染残影仍建议在真实 X11/Wayland 会话中按“大目录 -> 小目录 -> 回退”的路径复测。
