# UOS 软件渲染刷新问题修复记录

> 文档元数据
> - 文件编号：18
> - 文档类型：fix
> - 文件路径：docs/dev/18-fix-uos-software-renderer-refresh.md
> - 完成日期：2026-06-26
> - 需求级别：L2

## 1. 问题定义

- 问题现象：UOS 测试环境中切换页面、显示属性弹窗、打开右键菜单等交互存在刷新异常；多次在不同位置打开右键菜单后，旧菜单位置会留下黑色阴影残影。
- 预期行为：默认仍优先使用 wgpu；当图形环境只有软件 rasterizer 或 CPU adapter 时，应自动回退到 tiny-skia 软件渲染；动态 overlay 关闭或移动后不应留下旧阴影残影。
- 影响范围：GUI 启动时的渲染后端选择；右键菜单、工具栏菜单和属性弹窗等动态 overlay 样式。
- 不包含：重写具体页面、属性弹窗或右键菜单布局；强制所有环境默认 tiny-skia。

## 2. 证据与根因

- 复现方式：远程登录 UOS/Deepin 测试环境检查运行进程和图形栈；当前 `filesystem-gui` 进程未设置 `ICED_BACKEND`，因此 wgpu 探测通过后使用默认后端；默认 wgpu 下右键菜单截图出现斜向条纹和窗口内容错位；用 `ICED_BACKEND=tiny-skia` 启动后单次右键菜单和鼠标移动 hover 截图正常；继续在文件、空白区、靠右下等多个坐标多次打开右键菜单后，旧菜单位置留下黑色阴影残影。
- 证据等级：E1，用户可见刷新异常和远端图形栈证据明确，但未形成自动化 GUI 复现脚本。
- 关键日志/堆栈/输入：远端 `glxinfo -B` 显示 `OpenGL renderer string: llvmpipe (LLVM 7.0, 256 bits)`，`Accelerated: no`；远端截图文件保存在测试机 `/tmp/filesystem-menu-open.png`、`/tmp/filesystem-menu-hover.png`、`/tmp/filesystem-menu-tiny-open.png`、`/tmp/filesystem-menu-tiny-hover.png` 和 `/tmp/filesystem-menu-series/`。
- 根因：原 wgpu 探测只验证 adapter/device/pipeline 可创建，无法排除 llvmpipe、SwiftShader 等软件 rasterizer；这类环境可能“能跑 wgpu”，但实际窗口呈现/刷新不可靠。强制 tiny-skia 后仍存在多次菜单残影，原因是动态 overlay 使用带 blur 的 `Shadow`，阴影绘制范围超出 widget 本体 bounds，局部 damage 清理旧位置时可能漏掉阴影区域。
- 相关代码路径：`crates/filesystem-gui/src/renderer.rs`、`crates/filesystem-gui/src/style.rs`

## 3. 修复方案

- 最小修复点：在 wgpu adapter 探测后检查 `AdapterInfo`；移除动态 overlay 的模糊阴影。
- 代码逻辑改动：`DeviceType::Cpu` 直接视为不支持 wgpu；adapter 名称、driver 或 driver_info 包含 `llvmpipe`、`lavapipe`、`softpipe`、`software rasterizer`、`swiftshader` 时视为不适合 wgpu，自动返回 tiny-skia；用户显式 `ICED_BACKEND=wgpu` 时仍尊重用户选择。
- 影响的使用场景：UOS/虚拟机/软件渲染环境会自动回退 tiny-skia，减少菜单、弹窗、页面切换刷新异常；多次打开/关闭菜单和属性弹窗时不再依赖模糊阴影 damage 清理。
- 不影响的使用场景：硬件 GPU 环境继续默认优先 wgpu；显式 `ICED_BACKEND=tiny-skia` 或其他后端配置保持原行为。

## 4. 修复执行计划

| 步骤 | 修改内容 | 验证方式 | 状态 |
|------|----------|----------|------|
| 1 | 远端确认 UOS 图形栈与当前后端 | `glxinfo -B`、进程环境检查、wgpu/tiny-skia 截图对比 | 已完成 |
| 2 | wgpu 探测排除 CPU/软件 rasterizer adapter | `cargo test -p filesystem-gui renderer::tests` | 已完成 |
| 3 | 移除动态 overlay 模糊阴影 | 多坐标右键菜单截图复现结论、代码检查 | 已完成 |
| 4 | 完整 GUI crate 验证 | `cargo fmt --check`、`cargo test -p filesystem-gui`、`git diff --check` | 已完成 |

## 5. 风险摘要

| 项 | 结论 |
|----|------|
| 风险矩阵 | L2 |
| 命令权限 | C0/C1；远端仅只读检查图形栈并短时启动用户态 GUI 进程 |
| 高风险门禁 | 是，涉及渲染后端选择和构建行为 |
| 破坏性操作 | 否 |
| 用户已有修改 | 是，基于当前未提交渲染后端选择改动继续增量修改，未回退无关内容 |
| 用户确认 | 用户指定 UOS 测试环境并要求排查 |
| 副作用/风险 | 某些软件 Vulkan/OpenGL adapter 将不再默认走 wgpu；如用户确需强制 wgpu，可显式设置 `ICED_BACKEND=wgpu`；菜单和属性弹窗少了模糊阴影，视觉层级主要依赖边框和背景色 |

## 6. 验证

- 验证环境：本地 Linux workspace；远端 UOS/Deepin 测试环境用于只读调查和 tiny-skia 启动确认。
- 回归/相关验证：远端 wgpu/tiny-skia 截图对比、远端多坐标右键菜单截图、`cargo test -p filesystem-gui renderer::tests`、`cargo fmt --check`、`cargo test -p filesystem-gui`、`git diff --check`。
- 结果：远端 wgpu 下复现斜向条纹和内容错位；强制 tiny-skia 后单次菜单正常，但多坐标多次打开菜单仍复现旧阴影残影；本地自动化验证均通过，`filesystem-gui` 47 个单元测试通过。
- 未执行验证项：未在重新编译并部署到 UOS 后做真实交互复验。
- 残余风险：如果刷新异常来自 UOS 窗口管理器或 iced/winit 非渲染后端路径，仍需进一步使用 tiny-skia 新二进制做人工对比验证。

## 7. 修复总结

- 最终结果：已完成，软件 adapter 会自动回退 tiny-skia，菜单、工具栏菜单和属性弹窗移除模糊阴影以避免旧阴影残影。
- 计划偏差：最初只按 wgpu 软件 rasterizer 问题处理；多坐标菜单复现后确认还存在 overlay 阴影 damage 残留问题。
- 后续建议：目标系统重新编译后，先不设置环境变量启动，确认 `llvmpipe` 环境自动使用 tiny-skia；再在多个坐标反复打开右键菜单、切换属性弹窗做人工复验。
