这是关于本人对轮状项目完成情况的记录和说明. 对于轮状项目本身的介绍请见 `TASK_README.md` 文件.

# Part 1/2

不必多言. Part 2 中的 prompt 非常简陋, 但我觉得它已经足够了.

# Part 3

## 基本框架

我们定义了 `Mission` 类来表示一个优化任务. 它包含:

- `lang`: 代码语言 (`c` 或 `cpp`), 用以方便提示词的代码块标记;
- `output_file`: 输出文件路径;
- `origin`: 原始代码字符串;
- `target`: benchmark 中给出的优化后代码字符串;
- `origin_func`: 原始代码中待优化的函数字符串;
- `target_func`: benchmark 中给出的优化后的函数字符串;
- `output`: 由优化器的优化后函数字符串;
- `opt_reason`: 优化器的优化理由字符串;
- `result`: 由测评器判定的优化结果 (成功或失败);
- `judge_reason`: 测评器的判定理由字符串.

在 `optimizer_direct_io.py` 中, 我们给出了基于 prompt 的 CoT 优化器实现.

