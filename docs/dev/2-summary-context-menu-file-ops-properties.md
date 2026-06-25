# 文件视图右键菜单、写操作与属性弹窗实现总结

> 文档元数据
> - 文件编号：2
> - 文档类型：summary
> - 文件路径：docs/dev/2-summary-context-menu-file-ops-properties.md
> - 文档版本：v1.0.0
> - 完成日期：2026-06-25
> - 关联需求：图标视图和列表视图空白处右键菜单；支持新建文件/文件夹、粘贴、全选、在终端打开、查看当前文件夹属性。
> - 关联调研：docs/dev/2-research-context-menu-file-ops-properties.md
> - 关联计划：docs/dev/2-plan-context-menu-file-ops-properties.md

## 1. 最终结果

- 完成状态：已完成首版。
- 用户可见结果：
  - 图标视图和列表视图空白处右键弹出菜单，包含“新建文件”“新建文件夹”“粘贴”“全选”“在终端打开”“属性”和分割线。
  - 新建文件/文件夹在当前目录创建唯一名称，并在图标/列表视图中进入内联重命名；默认名称进入全选状态，编辑框使用 1.5 倍行高，长名称最多扩展到基础宽度 3 倍，继续输入会换行并增高；回车或点击文件视图/工具栏/侧边栏等外部区域提交，目标存在时失败不覆盖。
  - 粘贴读取标准剪贴板文本，支持绝对路径、`file://` URI、`copy` 和 `cut`/`move` 文本标记；复制递归复制文件夹，剪切首版使用同文件系统 `rename`。
  - 全选会选中当前视图列出的全部条目。
  - 在终端打开按 `terminator`、`mate-terminal`、`gnome-terminal` 顺序查找 PATH，并用 `--working-directory` 在当前目录启动。
  - 属性弹窗展示当前文件夹概要、权限页和内容权限参考页；首版只查看，不修改权限。
- 计划偏差：无阻塞偏差；按调研结论保留剪贴板首版边界，不读取专用 MIME target。

## 2. 关键改动

- `filesystem-core`
  - 新增 `create_file`、`create_folder`、`rename_entry`、`paste_paths`、`parse_clipboard_paths`、`folder_properties`。
  - 新增 `PasteAction`、`ClipboardPaths`、`FolderProperties`。
  - 新增 `libc` 直接依赖，用于 Linux `statvfs` 查询当前文件夹所在文件系统剩余空间。
  - 测试从 8 个增加到 15 个，覆盖唯一命名、防覆盖重命名、递归复制、同文件系统移动、剪贴板文本解析和属性统计。
- `filesystem-gui`
  - 新增 `context_menu`、`RenameState`、`PropertiesDialog` 状态和对应消息。
  - 文件视图 `mouse_area` 增加右键消息；右键菜单用 `stack` 覆盖显示，不改变文件布局。
  - 新建成功后的重命名编辑框使用固定 widget id 和 `text_editor::Content`；目录刷新后用 iced widget operation 聚焦，默认名称在 content 内全选；编辑框用 `text_editor` 支持 wrapping、1.5 倍行高、动态宽度和动态高度。
  - 新建、重命名、粘贴、终端启动、属性统计均通过 `Task::perform` 后台执行。
  - 属性弹窗使用三页视图：概要、权限、内容权限参考页。
- 文档：
  - 新增 `docs/dev/2-research-*`、`docs/dev/2-plan-*`、`docs/dev/2-summary-*`。
  - 更新 `docs/overview-product.md` 和 `docs/overview-product-dev.md`。

## 3. 安全门禁结果

| 项 | 结论 |
|----|------|
| 风险矩阵 | L4，新增文件系统写操作、递归复制、移动、外部进程和属性递归统计 |
| 命令权限 | C1，只修改源码/文档并运行本地构建测试 |
| 高风险项 | 文件写操作已限制在 core API；自动化测试只使用临时目录 |
| 破坏性操作 | 未对真实用户目录执行测试性复制/移动/删除 |
| 用户已有修改 | 基于上一轮未提交 GUI/文档变更继续修改，没有回退 |
| 副作用/风险 | 运行时右键菜单新建/粘贴会真实修改当前目录；终端打开会启动外部进程 |

## 4. 验证结果

- 执行验证：
  - `cargo fmt --check`：通过。
  - `cargo test -p filesystem-core`：通过，15 个测试。
  - `cargo check -p filesystem-gui`：通过。
  - `cargo test -p filesystem-gui`：通过，3 个测试。
  - `cargo test`：通过。
  - `make test`：通过。
  - `git diff --check`：通过。
- 未执行验证：
  - 未在真实 X11/Wayland 图形会话中手工验证右键菜单定位、重命名编辑框焦点/全选/长名称换行视觉状态、真实剪贴板内容、属性弹窗视觉细节和终端开窗。
  - 未验证跨设备剪切；首版也不做复制后删除退化。
  - 未验证 GNOME/KDE/COSMIC 文件管理器专用剪贴板 MIME target。

## 5. 残余风险

- 如果外部应用复制文件时没有提供 iced 可读取的文本 fallback，粘贴会提示剪贴板没有本地路径。
- 大目录属性统计会在后台执行，但首版没有取消和进度。
- 权限弹窗首版只查看，不修改；截图中的“更改”类操作未执行 chmod/chown。
