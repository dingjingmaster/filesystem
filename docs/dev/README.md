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
| 1 | 2026-06-24 | L4 | plan | [1-plan-local-linux-file-manager.md](1-plan-local-linux-file-manager.md) | 部分完成 | 明确仅支持 Linux 本地文件系统、GUI only、固定 wgpu 渲染的架构计划；已落地暗色只读 GUI。 |
| 1 | 2026-06-24 | L4 | summary | [1-summary-local-linux-file-manager.md](1-summary-local-linux-file-manager.md) | 已完成 | 总结首个 Rust workspace、只读 core 扫描库、wgpu 暗色 iced GUI、验证结果和未覆盖风险。 |
