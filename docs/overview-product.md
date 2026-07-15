# File 产品概览

> 文档元数据
> - 文档版本：v1.0.0
> - 最后更新：2026-07-15
> - 更新来源：docs/dev/1-research-cosmic-files.md、docs/dev/1-plan-local-linux-file-manager.md、docs/dev/1-summary-local-linux-file-manager.md、docs/dev/2-summary-context-menu-file-ops-properties.md、docs/dev/3-summary-properties-permission-edit.md、docs/dev/7-summary-selected-folder-context-menu.md、docs/dev/8-summary-selected-file-context-menu.md、docs/dev/9-summary-filesystem-mime.md、docs/dev/11-fix-text-editor-fallback.md、docs/dev/12-summary-symlink-badge-open.md、docs/dev/13-summary-large-directory-performance.md、docs/dev/14-summary-file-operation-progress.md、docs/dev/17-fix-desktop-exec-field-codes.md、docs/dev/19-task-shortcuts-multi-select-menu.md、docs/dev/20-task-click-range-selection.md、docs/dev/21-task-filesystem-ini-config.md、docs/dev/22-task-blank-menu-custom-commands.md、docs/dev/23-task-current-folder-auto-refresh.md、docs/dev/24-task-open-with-default-app.md、docs/dev/25-fix-wps-docx-open.md、docs/dev/26-fix-wps-sandbox-prometheus-open.md、docs/dev/27-task-template-file-menu.md
> - 本次更新来源：docs/dev/37-fix-properties-group-name.md

## 1. 产品定位

- 产品名称：英文 `File`，中文默认 `文件`；左侧标题栏名称可由可执行文件同级 `filesystem.ini` 的 `name` 配置覆盖。
- 目标用户：需要轻量、独立、图形化 Linux 本地文件管理器的用户。
- 核心问题：常见桌面文件管理器依赖 DBus/GVFS/portal/MIME/通知/桌面配置等服务，不适合独立环境运行。
- 核心价值：提供只依赖 Linux 本地文件系统和必要图形协议的 GUI 文件浏览与后续文件操作能力。
- 非目标：不支持 GVFS、SFTP、SMB、MTP、网络邻居、云盘、桌面图标或桌面 MIME 服务集成。

## 2. 功能边界

- 核心功能：本地目录浏览、当前打开文件夹自动刷新、当前目录树文件名正则搜索、访问历史后退/前进、隐藏文件过滤、文件框选多选、Ctrl/Shift 点击多选、空白区右键菜单、空白区右键菜单自定义命令、XDG 模板文件新建、选中文件夹右键菜单、选中文件右键菜单、多选右键菜单、文件操作快捷键、新建文件/文件夹与内联重命名、文本剪贴板路径粘贴、文件/文件夹复制剪切粘贴、复制/剪切粘贴进度显示与取消、文件/文件夹删除确认、当前目录或目标文件夹终端打开、可执行文件同级 `filesystem.ini` 启动配置、当前文件夹属性查看、当前文件夹权限修改、普通文件默认应用打开、打开方式选择、用户级默认打开方式设置、普通文件属性查看、软链接角标、软链接目标打开和断链提示、无系统边框窗口、窗口拖拽/关闭/最小化/最大化/边缘缩放、800x600 最小窗口尺寸、侧边栏本地导航、可编辑地址栏、分批显示大目录、流式图标视图、列表视图、基于内置 MIME 与系统 shared-mime-info fallback 的文件类型识别、基础文件类型与元数据展示；后续逐步补充更多安全写操作和外部二进制适配。
- 不支持功能：命令行/TUI、系统桌面服务、网络文件系统、桌面注册接口。
- 关键对象：本地路径、目录条目、文件类型、隐藏状态、文件元数据、软链接目标。
- 关键状态：当前目录、目录条目列表、运行时配置、选中路径集合、选择范围锚点、框选拖拽状态、搜索关键词、搜索根目录、隐藏文件显示开关、视图模式、自动刷新快照状态、状态消息、提示弹窗。

## 3. 关键场景

| 场景 | 用户目标 | 成功标准 | 异常/边界 |
|------|----------|----------|-----------|
| 浏览本地目录 | 查看当前目录内容并进入子目录 | GUI 以暗色文件管理器布局展示目录条目，目录优先排序；普通单击条目只选中单项，按住 `Ctrl` 单击可切换多个条目，按住 `Shift` 单击可选中锚点到目标之间的范围，空白区域拖拽可框选多个条目，双击目录才进入；可通过后退/前进访问历史位置；可在图标视图和列表视图间切换，图标视图按窗口宽度流式重排且文件图标大小保持一致，列表列出名称、大小、所有者、修改时间；当前打开文件夹直接普通文件内容或元数据变化时更新对应条目，直接子项新增、删除、重命名或不确定事件时后台校准目录列表 | 路径不存在或不可读时展示错误状态；纯读取访问不要求刷新；当前目录下子目录内部变化不刷新父目录列表 |
| 搜索当前目录树 | 通过地址栏输入正则表达式查找当前目录及子目录中的文件名 | 输入不是绝对路径时，按正则递归搜索当前目录树下的文件/目录名并列出匹配项；结果项显示相对路径 | 空关键词不搜索；无效正则展示错误；隐藏文件是否参与搜索由隐藏文件开关控制 |
| 空白区右键操作 | 在当前目录执行常见操作 | 图标视图和列表视图空白处右键弹出菜单；当前没有选中任何文件/文件夹时，在条目上右键也弹出同一菜单；菜单固定宽度，靠近主文件区右侧时向左钳制显示，菜单项水平左对齐、垂直居中；支持新建文件、新建文件夹、新建文档、粘贴、全选、在终端打开和属性；新建文件、文件夹或模板文件后默认名称全选，重命名编辑框作为覆盖层显示，不挤压文件布局，1.5 倍行高，长名称最多扩宽 3 倍后换行增高，编辑时拒绝超过当前目录名称/路径字节限制的输入，回车或点击外部区域提交重命名 | 粘贴首版只解析标准剪贴板文本中的本地路径或 `file://` URI；属性权限页只修改当前文件夹本身权限，不递归修改内容权限；限制未知且系统报文件名过长时保留默认名 |
| 选中文件/文件夹右键操作 | 对选中目标执行上下文操作 | 单个文件夹已选中且右键命中该文件夹时显示文件夹菜单，支持打开、复制、剪切、重命名、删除、在终端打开和属性；单个非文件夹条目已选中且右键命中该条目时显示文件菜单，支持用默认应用打开、打开方式、复制、剪切、重命名、删除和属性；选中多个文件/文件夹后右键显示多选菜单，支持复制、剪切和删除；文件夹和文件删除前均弹出确认 | 删除不进入回收站；剪切状态在当前视图中变淡显示；打开方式候选来自本地 `.desktop` `MimeType`、用户/系统 `mimeapps.list` 关联和内置兼容规则 |
| 选中项快捷操作 | 用键盘执行常见文件操作 | 选中一个或多个文件/文件夹后，`Ctrl+C` 复制选中集合，`Ctrl+X` 剪切选中集合，`Del` 弹出删除确认；没有选中任何文件/文件夹时，`Ctrl+V` 执行粘贴 | 快捷键只在输入控件未捕获按键时生效；删除不进入回收站，确认后才执行 |
| 复制/剪切粘贴进度 | 了解文件操作整体进度并管理单次粘贴 | 用户发起复制或剪切粘贴后，侧栏标题“文件”居中显示，标题右侧显示右对齐圆形总进度，按顺时针填充；总进度完成后中心显示对钩并延时 3 秒消失；点击圆形区域在其下方展开或隐藏详情，详情作为上层 popup 可覆盖到右侧内容上方且不改变侧栏或主区域布局，点击 popup 外部区域会自动隐藏详情；每次粘贴显示独立条形进度、当前文件、速度和 `x` 按钮，不超过 5 条详情时不显示滚动条 | `x` 对运行中的粘贴表示取消该次操作并立即移除对应详情条；对失败、取消或已结束条目表示关闭详情；同一时间最多 3 个文件操作复制线程，更多操作显示为排队并在前序操作结束后启动；跨文件系统剪切会复制成功后删除源路径 |
| 切换隐藏文件 | 查看或隐藏点号开头文件 | 在地址栏右侧菜单中切换后刷新列表 | 不读取桌面配置服务 |
| 软链接浏览与打开 | 识别文件/文件夹软链接并处理断链 | 有效文件夹软链接显示文件夹图标加 `icons/symbol.svg` 角标，双击进入目标文件夹；有效文件软链接显示文件图标加 `icons/symbol.svg` 角标，双击打开目标文件；断链显示 `icons/symbol-disconnect.svg` 角标，双击或打开时弹窗提示“软连接 xxx 损坏” | 复制、剪切、重命名、删除仍作用于软链接路径本身；搜索不递归跟随目录软链接 |
| 打开文件 | 用本地应用处理普通文件 | 双击普通文件或选择“用 xxx 打开”时，优先使用本地配置解析到的默认应用启动；没有默认应用但打开方式列表有候选时，自动选择最匹配的候选应用；启动外部应用前会按 `.desktop` `Exec` 字段码展开参数，并丢弃未知或废弃字段码，WPS Office 本地文件的 `%u/%U` 兼容展开为本地路径，WPS Writer/表格/演示的 `wps`/`et`/`wpp` 启动器优先走 `wpsoffice /prometheus` 并保留原 `.desktop` 命令 fallback；文件类型优先来自后台缓存的内容签名、内置名称/扩展名和系统 shared-mime-info fallback，系统 magic 返回 `application/octet-stream` 等泛型结果时不覆盖明确扩展名 MIME；打开候选合并 `.desktop` `MimeType`、`mimeapps.list` 的 `[Default Applications]` 和通用 `mimeapps.list` 的 `[Added Associations]`，默认应用即使对应 `.desktop` 没有 `MimeType` 也可作为候选，并按通用 `mimeapps.list` 的 `[Removed Associations]` 移除关联；`Makefile`、`Dockerfile`、`README`、`LICENSE`、`.gitignore`、`.desktop`、`.c`、`.cpp`、`.md` 等文本类文件可退到文本编辑器打开；内置识别 WPS Writer/表格/演示常见原生格式、UOF、OOXML 模板和宏格式，标准 Office MIME 与同类 WPS Office MIME 在打开候选匹配中互认；“打开方式”弹窗列出可处理该文件类型的应用并默认选中首选应用，应用行左侧显示 APP 图标和名称，右侧显示选中对钩，可勾选“设为默认打开方式”把所选应用保存为当前 MIME 的用户级默认应用 | 不使用 `file` 命令、`xdg-open`、DBus、GVFS、portal 或桌面 MIME 服务；默认应用只写用户级 MIME apps 配置，有当前桌面名时桌面专用文件保存 `[Default Applications]`、通用 `mimeapps.list` 保存 `[Added Associations]`，无桌面名时通用文件保存两者，不写系统配置；APP 无图标或图标找不到时使用 `icons/app.svg`；当前不把需要终端的应用作为打开候选 |

## 4. 核心流程

```text
1. 启动 GUI。
2. 从当前工作目录扫描本地目录。
3. 展示无系统边框窗口、侧边栏、路径栏和当前视图模式下的文件条目。
4. 用户普通单击条目单选、按住 `Ctrl` 单击多点选择、按住 `Shift` 单击范围选择、拖拽框选多个条目、双击目录进入、后退/前进访问历史位置、在地址栏输入绝对路径跳转、输入非绝对路径正则搜索当前目录树文件名或在菜单中切换隐藏文件。
5. 扫描失败时在窗口状态区域展示错误。
```

## 5. 产品规则

- 权限规则：写操作只能由用户显式触发；新建、重命名、粘贴、删除和当前文件夹权限修改必须有明确边界和失败反馈，删除等破坏性操作必须先确认。
- 状态流转：目录切换后重新扫描并退出搜索模式；搜索成功更新条目为搜索结果，扫描失败清空条目并展示错误；当前打开文件夹直接普通文件内容或元数据变化时只更新对应条目；直接子项新增、删除、重命名或不确定事件时后台快照校准目录列表且不清空旧界面。
- 文件视图显示规则：目录加载开始时清空旧条目并把主文件区滚动位置复位到顶部；图标视图和列表视图中的长文件名使用中间省略，保留开头和结尾，鼠标悬停条目时显示完整名称。
- 列表视图显示规则：列表列出名称、大小、所有者、修改时间；所有者列优先显示本机用户名，无法解析用户名时显示 UID；修改时间按系统本地时区显示。
- 属性权限页显示规则：所有者值优先显示为 `用户名(UID)`，用户组值优先显示为 `组名(GID)`；无法解析名称时分别保留 `UID xxx` 或 `GID xxx` 兜底。
- 时间显示规则：列表视图修改时间、文件夹属性修改/创建时间和普通文件属性访问/修改/创建时间统一按系统本地时区显示为 `YYYY-MM-DD HH:MM`；无法转换时显示 `-`。
- 模板文件新建规则：空白菜单“新建文档”子菜单只显示模板文件名去掉最后一个扩展名后的名称；选择模板后复制创建的文件仍使用模板原始文件名和扩展名，并进入内联重命名。
- 异常处理：缺失路径、不可读路径和 metadata 读取失败必须显式反馈。
- 兼容约束：同一二进制应可在 X11 和 Wayland 会话运行，不分别编译。
- 用户可见行为：默认不显示隐藏文件；隐藏文件开关位于地址栏右侧菜单中；右侧主区域文件和文件夹普通单击只选中单项，按住 `Ctrl` 单击其它条目可切换加入/移除多选，按住 `Shift` 单击其它条目会按当前显示顺序选中锚点和目标之间的所有条目，空白区域拖拽可框选多个条目，双击目录进入目录、双击非目录条目尝试用默认应用或打开方式首选候选应用打开；图标视图和列表视图空白处右键菜单包含“新建文件”“新建文件夹”“新建文档”“粘贴”“全选”“在终端打开”“属性”，`filesystem.ini` 中配置的空白菜单自定义命令插入到“在终端打开”后的分割线与“属性”前的分割线之间，菜单叠加显示且不重排文件视图；单个选中文件夹右键菜单包含“打开”“复制”“剪切”“重命名”“删除”“在终端打开”“属性”；单个选中非文件夹条目的右键菜单包含“用 xxx 打开”“打开方式”“复制”“剪切”“重命名”“删除”“属性”；打开方式弹窗列出本地 `.desktop` 声明或 `mimeapps.list` 关联支持该 MIME 类型的应用并默认选中首选应用，MIME 类型由后台缓存的 `filesystem-mime` 识别结果提供，`Makefile` 等常见无扩展名文本文件按文本应用匹配；打开方式弹窗可勾选“设为默认打开方式”，打开成功后写入用户级 MIME apps 配置文件并让后续双击、右键“用 xxx 打开”和候选排序立即使用新默认应用，写入失败只在状态栏反馈且不阻断本次打开；新建文件/文件夹在当前目录创建唯一名称并进入内联重命名；如果 XDG 模板目录中存在模板文件，空白菜单“新建文档”可展开子菜单，选择模板后把该模板文件复制到当前目录并进入内联重命名，默认名称全选，编辑框作为文件视图上层覆盖层显示，不挤压其它文件/文件夹位置，使用 1.5 倍行高，长名称最多扩展到基础宽度 3 倍，继续输入会换行并增高，编辑时按当前目录 `NAME_MAX` 和 `PATH_MAX` 字节限制拒绝超限输入，回车或点击文件视图/工具栏/侧边栏等外部区域提交且不覆盖已有路径；如果限制未知且系统返回文件名过长，则退出重命名并保留新建时的默认名称；粘贴从标准剪贴板文本解析本地绝对路径或 `file://` URI，`copy` 文本标记按复制处理，`cut`/`move` 文本标记按剪切处理，同文件系统优先用 `rename` 移动，跨文件系统在 `rename` 返回跨设备错误时复制成功后删除源路径；复制/剪切粘贴期间侧栏标题“文件”居中显示，标题右侧显示右对齐圆形总进度，点击可展开或隐藏每次粘贴的条形进度、当前文件、速度和取消入口，详情作为上层 popup 可覆盖到右侧内容上方且不改变侧栏或主区域布局，点击 popup 外部区域会自动隐藏详情，不超过 5 条详情时不显示滚动条，运行中点 `x` 会取消对应操作并立即移除该详情条，同一时间最多 3 个文件操作复制线程，更多操作显示为排队并在前序操作结束后启动，全部完成后圆形中心显示对钩并延时 3 秒消失；“在终端打开”按 `terminator`、`mate-terminal`、`gnome-terminal` 顺序查找 PATH 并在当前目录或目标文件夹启动；“属性”弹窗可查看当前文件夹概要和权限页，也可查看普通文件的类型、大小、父目录、时间、权限和是否可执行；属性弹窗上层事件不传递到底层文件视图，标题区域可拖拽，关闭按钮复用 `icons/close.svg` 和主窗口关闭按钮样式，信息水平左对齐、垂直居中；权限页可修改当前文件夹 owner/group/other 访问权限，可取消待保存改动或后台保存，不提供“更改内容文件的权限”入口；窗口英文标题和 Linux app_id 使用 `File`；左侧拖拽标题栏显示 `文件`；窗口/任务栏图标使用 `icons/fs.svg`；窗口最小尺寸为 800x600；地址栏左侧只有后退和前进按钮，分别使用 `icons/left.svg` 与 `icons/right.svg`，无历史时置灰不可点；地址栏右侧菜单按钮使用 `icons/menu.svg`，位置靠近最小化按钮且保留拖拽空白，菜单提供“视图”子菜单，可选择图标视图或列表视图；菜单和子菜单叠加在文件显示区域上层，不改变图标/列表视图中的文件位置和布局；图标视图使用固定 tile 大小的流式布局，窗口变宽时增加每行列数、变窄时把列尾条目换到下一行，不缩放文件/文件夹图标；列表视图显示名称、大小、所有者、修改时间；图标视图和列表视图均显示条目图标，文件夹使用 `icons/folder.svg`，文件按 MIME 类型优先使用本地图标主题中的 SVG MIME 图标，找不到时使用 `icons/file.svg`；侧边栏只显示主文件夹、根目录和用户家目录中实际存在的常见目录，导航项使用本地 SVG 图标并保持左对齐、垂直居中；侧边栏顶部和地址栏右侧空白区可拖拽窗口；窗口四边和四角可拖动缩放；地址栏可编辑，回车时只有绝对路径按路径访问，其他输入一律作为正则表达式在当前目录树下搜索文件/目录名并列出匹配项。
- 启动配置规则：启动时检查可执行文件同级目录的 `filesystem.ini`；该 INI 文件中非空 `name=xxx` 会覆盖左侧拖拽标题栏显示，非空 `terminal=xxx` 会让“在终端打开”执行该终端路径并把当前目录或目标文件夹设为进程当前工作目录；`[blank-menu.*]` section 可通过 `label`、`command` 和多个 `arg` 为空白处右键菜单新增自定义命令，参数中的 `{cwd}` 会替换为当前目录；未配置时保持默认左侧标题 `文件` 和 `terminator`、`mate-terminal`、`gnome-terminal` PATH fallback，且不显示额外空白菜单项。
- 当前文件夹自动刷新规则：GUI 只监听当前打开文件夹本身的直接子项变化；切换目录后监听目标随当前目录变化；普通文件内容或元数据变化只刷新对应条目；直接子项新增、删除、重命名、事件溢出或不确定事件使用后台快照合并校准，期间保留旧界面、滚动和选择状态；当前目录下子目录内部变化不刷新父目录列表；不递归监听子目录内部变化，不保证网络文件系统、伪文件系统或纯读取访问产生刷新。
- 软链接规则：文件夹软链接按有效目标显示文件夹图标并加 `icons/symbol.svg` 角标，文件软链接按有效目标显示文件图标并加 `icons/symbol.svg` 角标；断链或目标不可访问时使用 `icons/symbol-disconnect.svg` 角标；双击有效文件夹软链接进入解析后的目标目录，双击有效文件软链接打开目标文件，双击断链或对断链执行打开入口时弹窗提示“软连接 xxx 损坏”。
- 右键菜单规则：当前没有选中任何文件/文件夹时，在图标视图或列表视图条目上右键等价于空白处右键；只有单个文件夹或单个非文件夹条目已选中且右键命中该条目时才显示对应目标菜单；选中多个文件/文件夹时右键显示只包含“复制”“剪切”“删除”的多选菜单；右键菜单固定宽度并在主文件区内钳制显示，靠近右边界时不压缩，靠近底部时向上显示以避免遮挡，菜单项水平左对齐、垂直居中。
- 打开方式列表规则：每个 APP 行左侧显示 APP 图标和名称，内容水平左对齐、垂直居中；选中对钩水平右对齐；候选应用由 `.desktop` `MimeType`、`mimeapps.list` 的 `[Default Applications]`、通用 `mimeapps.list` 的 `[Added Associations]` 和内置兼容规则共同决定，默认应用可补充无 `MimeType` 的 `.desktop` 支持 MIME，并按通用 `mimeapps.list` 的 `[Removed Associations]` 移除用户或系统取消的关联；默认应用排在最前，若没有默认应用则按精确 MIME、同类 WPS/标准 Office MIME 互认、`text/plain` 文本兜底、`TextEditor` 分类兜底、类型通配、全局通配和名称排序；WPS 原生 MIME 的精确匹配优先于同家族 alias；勾选“设为默认打开方式”后确认打开，有当前桌面名时把所选应用写入桌面专用 MIME apps 配置文件的 `[Default Applications]`，并写入通用 `mimeapps.list` 的 `[Added Associations]`；无桌面名时写入通用 `mimeapps.list` 的两个 section；APP 图标按 `.desktop` 的 `Icon` 字段查找本地图标，找不到时使用 `icons/app.svg`。
- 外框圆角策略：只接受窗口管理器原生支持的外框圆角；当前 Linux iced/winit 未提供可用接口时，不使用透明窗口或内容裁剪模拟圆角。

## 6. 非功能要求（按需）

- 性能：目录打开分批显示基础条目，避免等待整目录扫描和 MIME/主题图标装饰完成；MIME 内容识别最多读取文件前 256 KiB 且在后台装饰任务执行；图标视图和列表视图只渲染可视区域附近条目，保持滚动高度和交互坐标一致；大目录真实帧时间仍需图形会话专项验证。
- 可用性：GUI 是唯一交互形态，不提供命令行/TUI。
- 安全：不依赖系统桌面服务；文件类型识别不调用 `file` 命令或 libmagic，只读本地文件和 shared-mime-info 数据；后续外部命令必须使用 argv 直接调用，不经 shell。
- 兼容性：使用 `wgpu` 渲染；同一二进制启用 X11 和 Wayland。

## 7. 文档索引

- 需求与任务索引：docs/dev/README.md
- 开发概览：docs/overview-product-dev.md
- 关键任务文档：
  - docs/dev/1-research-cosmic-files.md：cosmic-files 调研。
  - docs/dev/1-plan-local-linux-file-manager.md：本地 Linux GUI 文件管理器计划。
  - docs/dev/1-summary-local-linux-file-manager.md：首版只读骨架实现总结。
  - docs/dev/2-summary-context-menu-file-ops-properties.md：空白区右键菜单、首批写操作和属性弹窗实现总结。
  - docs/dev/3-summary-properties-permission-edit.md：属性弹窗交互修正和当前文件夹权限修改实现总结。
  - docs/dev/7-summary-selected-folder-context-menu.md：选中文件夹右键菜单实现总结。
  - docs/dev/8-summary-selected-file-context-menu.md：选中文件右键菜单和打开方式实现总结。
  - docs/dev/9-summary-filesystem-mime.md：少依赖文件类型识别模块实现总结。
  - docs/dev/11-fix-text-editor-fallback.md：文本文件打开兜底修复记录。
  - docs/dev/12-summary-symlink-badge-open.md：软链接角标和打开行为实现总结。
  - docs/dev/13-summary-large-directory-performance.md：大目录分批显示、后台装饰、图标缓存和视图虚拟化实现总结。
  - docs/dev/14-summary-file-operation-progress.md：复制/剪切粘贴进度、取消和跨文件系统剪切语义实现总结。
  - docs/dev/17-fix-desktop-exec-field-codes.md：Desktop Exec 字段码展开兼容修复记录。
  - docs/dev/19-task-shortcuts-multi-select-menu.md：快捷键和多选右键菜单实现记录。
  - docs/dev/20-task-click-range-selection.md：Ctrl/Shift 点击多选实现记录。
  - docs/dev/21-task-filesystem-ini-config.md：同级 filesystem.ini 启动配置实现记录。
  - docs/dev/22-task-blank-menu-custom-commands.md：空白处右键菜单自定义命令实现记录。
  - docs/dev/23-task-current-folder-auto-refresh.md：当前打开文件夹自动刷新实现记录。
  - docs/dev/24-task-open-with-default-app.md：打开方式设为默认应用实现记录。
  - docs/dev/25-fix-wps-docx-open.md：WPS 文档和 WPS 原生格式打开兼容修复记录。
  - docs/dev/26-fix-wps-sandbox-prometheus-open.md：WPS Office 沙盒 Prometheus 入口修复记录。
  - docs/dev/27-task-template-file-menu.md：空白菜单模板文件创建实现记录。
  - docs/dev/31-summary-auto-refresh-model-view.md：自动刷新 model/view 优化实现总结。
  - docs/dev/33-summary-list-owner-username.md：列表视图所有者列用户名显示记录。
  - docs/dev/34-fix-local-time-format.md：文件时间按本地时区显示修复记录。
  - docs/dev/36-summary-properties-owner-group-name.md：属性权限页所有者和用户组名称显示记录。
  - docs/dev/37-fix-properties-group-name.md：属性权限页用户组名称解析补强记录。

## 8. 变更记录

| 日期 | 变更 | 影响 | 关联文档 |
|------|------|------|----------|
| 2026-06-24 | 创建产品概览，明确 Linux 本地 GUI 文件管理器边界 | 建立长期产品事实 | docs/dev/1-summary-local-linux-file-manager.md |
| 2026-06-24 | 记录暗色文件管理器界面形态：侧边栏、路径栏、网格文件图标 | 更新用户可见行为 | docs/dev/1-summary-local-linux-file-manager.md |
| 2026-06-24 | 记录无系统边框窗口和精简侧边栏规则 | 更新用户可见行为 | docs/dev/1-summary-local-linux-file-manager.md |
| 2026-06-24 | 记录窗口控制按钮使用本地 SVG 图标 | 更新用户可见行为 | docs/dev/1-summary-local-linux-file-manager.md |
| 2026-06-24 | 记录无边框窗口拖拽、关闭、最小化、最大化行为 | 更新用户可见行为 | docs/dev/1-summary-local-linux-file-manager.md |
| 2026-06-24 | 记录侧栏顶部拖拽区和可编辑地址栏行为 | 更新用户可见行为 | docs/dev/1-summary-local-linux-file-manager.md |
| 2026-06-24 | 记录四边/四角缩放和不伪造外框圆角策略 | 更新用户可见行为 | docs/dev/1-summary-local-linux-file-manager.md |
| 2026-06-24 | 记录应用英文名 `File`、中文名 `文件` 和窗口图标 `icons/fs.svg` | 更新用户可见行为 | docs/dev/1-summary-local-linux-file-manager.md |
| 2026-06-24 | 地址栏左侧改为后退/前进历史按钮并移除刷新按钮 | 更新用户可见行为 | docs/dev/1-summary-local-linux-file-manager.md |
| 2026-06-24 | 记录侧边栏导航项使用本地 SVG 图标并左对齐、垂直居中 | 更新用户可见行为 | docs/dev/1-summary-local-linux-file-manager.md |
| 2026-06-24 | 记录地址栏支持绝对路径跳转和当前目录树文件名正则搜索 | 更新用户可见行为 | docs/dev/1-summary-local-linux-file-manager.md |
| 2026-06-24 | 记录地址栏右侧视图菜单和图标/列表视图切换，列表展示名称、大小、所有者、修改时间 | 更新用户可见行为 | docs/dev/1-summary-local-linux-file-manager.md |
| 2026-06-24 | 记录菜单、文件夹、文件图标资源和文件主题图标回退策略 | 更新用户可见行为 | docs/dev/1-summary-local-linux-file-manager.md |
| 2026-06-25 | 记录隐藏文件开关移入菜单，以及主区域单击选中、双击打开/进入 | 更新用户可见行为 | docs/dev/1-summary-local-linux-file-manager.md |
| 2026-06-25 | 记录菜单和子菜单作为覆盖层显示，不重排文件视图 | 更新用户可见行为 | docs/dev/1-summary-local-linux-file-manager.md |
| 2026-06-25 | 记录文件视图支持空白区域拖拽框选多个条目 | 更新用户可见行为 | docs/dev/1-summary-local-linux-file-manager.md |
| 2026-06-25 | 记录窗口最小尺寸 800x600 和固定 tile 的流式图标视图 | 更新用户可见行为 | docs/dev/1-summary-local-linux-file-manager.md |
| 2026-06-25 | 记录空白区右键菜单、新建/重命名、文本路径粘贴、终端打开和属性查看 | 更新用户可见行为 | docs/dev/2-plan-context-menu-file-ops-properties.md |
| 2026-06-25 | 记录新建后默认名称全选，回车或点击外部区域提交重命名 | 更新用户可见行为 | docs/dev/2-summary-context-menu-file-ops-properties.md |
| 2026-06-25 | 记录重命名编辑框 1.5 倍行高、最多 3 倍宽度和超长换行增高 | 更新用户可见行为 | docs/dev/2-summary-context-menu-file-ops-properties.md |
| 2026-06-25 | 记录重命名编辑框作为覆盖层显示，不挤压文件视图布局 | 更新用户可见行为 | docs/dev/2-summary-context-menu-file-ops-properties.md |
| 2026-06-25 | 记录重命名编辑时按目录路径限制拒绝超限输入，限制未知且系统报过长时保留默认名 | 更新用户可见行为 | docs/dev/2-summary-context-menu-file-ops-properties.md |
| 2026-06-25 | 记录无选中项时条目右键等价空白菜单，以及右键菜单固定宽度、靠右钳制和菜单项对齐 | 更新用户可见行为 | docs/dev/2-summary-context-menu-file-ops-properties.md |
| 2026-06-25 | 记录属性弹窗事件隔离、拖拽、关闭按钮统一、信息对齐和当前文件夹权限修改 | 更新用户可见行为 | docs/dev/3-summary-properties-permission-edit.md |
| 2026-06-25 | 记录选中文件夹右键菜单、内部复制/剪切、删除确认和目标文件夹终端/属性入口 | 更新用户可见行为 | docs/dev/7-summary-selected-folder-context-menu.md |
| 2026-06-25 | 记录选中非文件夹条目的右键文件菜单、默认应用打开、无默认应用时首选候选应用打开、常见无扩展名文本文件打开方式匹配、带图标打开方式弹窗、文件属性和普通文件双击打开 | 更新用户可见行为 | docs/dev/8-summary-selected-file-context-menu.md |
| 2026-06-25 | 记录 `filesystem-mime` 内置识别和 shared-mime-info fallback，打开方式、图标和文件属性统一使用缓存 MIME | 更新文件类型识别和打开文件行为 | docs/dev/9-summary-filesystem-mime.md |
| 2026-06-25 | 记录文本类文件可退到文本编辑器打开，打开候选支持 `TextEditor` 分类兜底并排除需要终端的应用 | 更新打开文件行为 | docs/dev/11-fix-text-editor-fallback.md |
| 2026-06-25 | 记录软链接角标、断链角标、有效软链接目标打开和断链弹窗提示 | 更新软链接浏览与打开行为 | docs/dev/12-summary-symlink-badge-open.md |
| 2026-06-25 | 记录大目录分批显示、MIME/图标后台装饰和视图虚拟化 | 更新目录浏览性能行为 | docs/dev/13-summary-large-directory-performance.md |
| 2026-06-26 | 记录复制/剪切粘贴总进度圆形入口、每次粘贴详情进度、详情失焦隐藏、取消入口、最多 3 个复制线程和跨文件系统剪切退化语义 | 更新文件操作用户可见行为 | docs/dev/14-summary-file-operation-progress.md |
| 2026-06-26 | 记录打开文件前按 `.desktop` `Exec` 字段码展开参数，并丢弃未知或废弃字段码 | 更新打开文件兼容行为 | docs/dev/17-fix-desktop-exec-field-codes.md |
| 2026-06-26 | 记录选中项复制/剪切/粘贴/删除快捷键和多选右键菜单 | 更新文件操作用户可见行为 | docs/dev/19-task-shortcuts-multi-select-menu.md |
| 2026-06-27 | 记录 Ctrl 点击多点选择和 Shift 点击范围选择 | 更新文件选择用户可见行为 | docs/dev/20-task-click-range-selection.md |
| 2026-06-29 | 记录可执行文件同级 `filesystem.ini` 的 `name` 和 `terminal` 启动配置 | 更新启动配置和终端打开行为 | docs/dev/21-task-filesystem-ini-config.md |
| 2026-06-29 | 记录 `[blank-menu.*]` 配置为空白处右键菜单新增自定义命令，并插入到“在终端打开”和“属性”之间 | 更新空白菜单和启动配置行为 | docs/dev/22-task-blank-menu-custom-commands.md |
| 2026-06-29 | 记录当前打开文件夹直接子项变化后自动刷新 | 更新目录浏览用户可见行为 | docs/dev/23-task-current-folder-auto-refresh.md |
| 2026-06-29 | 记录打开方式弹窗可将选中应用设为当前 MIME 的用户级默认打开方式 | 更新打开文件用户可见行为 | docs/dev/24-task-open-with-default-app.md |
| 2026-06-29 | 记录无 `MimeType` 默认应用、desktop-specific mimeapps 规则、WPS 原生扩展名识别、WPS `%u/%U` 本地路径兼容，以及 WPS Writer/表格/演示 MIME 家族与标准 Office MIME 在打开候选中互认 | 更新打开文件兼容行为 | docs/dev/25-fix-wps-docx-open.md |
| 2026-06-29 | 记录 WPS Writer/表格/演示的 `wps`/`et`/`wpp` 启动器优先走 `wpsoffice /prometheus` 并保留原 `.desktop` 命令 fallback | 更新打开文件兼容行为 | docs/dev/26-fix-wps-sandbox-prometheus-open.md |
| 2026-06-29 | 记录空白处右键菜单模板入口子菜单，按 XDG 模板目录复制模板文件并进入重命名 | 更新空白菜单用户可见行为 | docs/dev/27-task-template-file-menu.md |
| 2026-07-13 | 记录右键菜单靠近底部时按菜单高度向上显示，避免菜单项被浏览区域裁剪 | 更新右键菜单用户可见行为 | docs/dev/30-summary-context-menu-bottom-clamp.md |
| 2026-07-14 | 当前文件夹自动刷新改为普通文件单项更新、结构变化后台快照合并，并忽略当前目录下子目录内部变化 | 更新目录浏览用户可见行为 | docs/dev/31-summary-auto-refresh-model-view.md |
| 2026-07-14 | 记录列表视图所有者列优先显示本机用户名，未知时显示 UID | 更新列表视图用户可见行为 | docs/dev/33-summary-list-owner-username.md |
| 2026-07-14 | 记录列表修改时间和属性弹窗文件时间统一按系统本地时区显示 | 更新文件时间显示规则 | docs/dev/34-fix-local-time-format.md |
| 2026-07-15 | 记录属性权限页所有者显示为 `用户名(UID)`、用户组显示为 `组名(GID)`，未知名称保留 UID/GID 兜底 | 更新属性权限页用户可见行为 | docs/dev/36-summary-properties-owner-group-name.md |
| 2026-07-15 | 修正属性权限页用户组名称解析，确保优先按系统 GID 解析组名后再显示 `组名(GID)` | 修复属性权限页用户组显示 | docs/dev/37-fix-properties-group-name.md |
