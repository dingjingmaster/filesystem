# 属性弹窗与权限修改调研

> 文档元数据
> - 文件编号：3
> - 文档类型：research
> - 文件路径：docs/dev/3-research-properties-permission-edit.md
> - 文档版本：v1.0.0
> - 创建日期：2026-06-25
> - 关联需求：属性弹窗事件隔离、可拖拽、关闭按钮统一、信息左对齐/垂直居中、权限页允许修改当前文件夹权限。

## 1. 背景与目标

- 当前属性弹窗作为 `stack` 顶层元素展示，但没有专门的事件隔离层、拖拽状态或权限写操作。
- 当前权限页只读取 `FolderProperties.mode` 并展示 owner/group/other 访问说明，不允许修改。
- 用户要求属性弹窗上的点击不投递到底层文件管理器，弹窗可拖拽，关闭按钮与主窗口关闭按钮一致，弹窗信息按水平左对齐、垂直居中排列，并允许修改权限。

## 2. 现状结论

- `filesystem-core` 已提供 `folder_properties`，但没有 chmod API；新增权限修改需要补 core 公共函数并用临时目录测试验证。
- `filesystem-gui` 的 `properties_overlay` 当前居中绘制，不保存位置；需要记录窗口尺寸、弹窗位置和拖拽状态。
- iced `mouse_area` 先让内部 widget 处理事件，若未捕获再自己发布消息并捕获事件；适合用全屏属性遮罩吃掉底层 press/release/move/scroll，同时保留内部按钮可点击。
- 关闭按钮可直接复用已有 `window_icon(include_bytes!("../../../icons/close.svg"))` 和 `style::close_button`。

## 3. 推荐方案

- core 新增 `set_permissions(path, mode)`，仅校验 mode 在 `0o0000..=0o7777`，通过 `std::fs::set_permissions` 和 `PermissionsExt::from_mode` 修改路径权限。
- GUI 属性弹窗新增：
  - `PropertiesDialog.position`、`permissions_mode`、`saving_permissions`、`permission_error`。
  - `properties_pointer` 和 `properties_drag`，用全屏 `mouse_area` 跟踪指针，标题区域 press 开始拖拽，release 结束拖拽。
  - 全屏属性遮罩处理左/右/中键和滚轮事件，避免底层文件视图收到点击、右键或滚动。
  - 权限页以 owner/group/other 三行可点击访问级别按钮修改待保存 mode，点击“更改”后台保存并重新读取属性。
- 首版仅修改当前文件夹本身权限，不递归修改内容权限；按后续反馈移除“更改内容文件的权限”入口和参考页。

## 4. 风险与边界

- 风险矩阵：L4。涉及文件系统权限写操作和 GUI 状态流转，运行时可能改变用户目录权限。
- 命令权限：实现和验证只修改源码/文档、在临时目录跑测试，不执行系统 `chmod` 命令，不修改真实用户目录。
- 失败策略：保存失败时保持属性弹窗打开，显示错误并保留用户选择；保存成功后重新读取属性。
- 未覆盖：不实现 chown/chgrp、不实现递归 chmod、不实现 ACL 或扩展属性。
