# Case Study

- 状态顺序 `ccs | dio | rag | rgn`.
- `full_02_strict` 实际有 5 列 (`rgn2` 为 `rgn` 重复试验); 本文按前 4 列讨论.

## 1 ARRR (只有 CCS 成功)

### 1.1 `tensorflow/5e0...8b7`

- 目标优化手段: 将一个循环迭代器定义由 `auto it : ...` 变为 `const auto& it : ...`, 以避免不必要的复制.

- `ccs`: `Selected Strategy: "Use pass-by-reference in range-based for loops with llvm::enumerate to avoid unnecessary object copying."`, 完全契合目标; 且两个示例都是来自 `tensorflow` 的, 第一个示例 `before_func` 和 `after_func` 完全一致 (知识库的问题); 第二个示例正好是对 `for (auto en : llvm::enumerate(tokens))` 的优化.
- `dio`: 没有发现这一优化, 尝试减小一个 `std::string` 常量传递的代价, 被认为效果很小且与目标思路不同.
- `rag`: few-shot 中存在 `Reducing temporary object creation`, 但优化器最终仍然选择了和 `dio` 一样的优化策略.
- `rgn`: `Example 1 shows optimization by using "const auto&" in loops to avoid copies.` 但最终还是选择了和 `dio` 一样的优化策略.

### 1.2 `folly/2e2...6f3`

- 目标优化手段: 在一个锁逻辑简单的上下文中, 将 `unique_lock` 锁替换为 `lock_guard` 锁.
- `ccs`: `Selected Strategy: Move lock acquisition outside of loops when thread safety is needed for the entire loop execution to reduce lock overhead and contention.`, 换成功能和开销都跟轻量的 `lock_guard` 满足了这一要求.
- `dio`: 尝试将对 `signaled_` 操作变得无锁, 但 `signaled_` 只是一个 `bool`, 尝试调用其 `store` method 的行为非法.
- `rag`: 同 `dio`.
- `rgn`: 尝试 double check 了 `_signaled` 为 `false` 的情况, 与目标优化手段不同; 且其正确性依赖于对 `wake()` 调用的上下文`, 不能作为原函数的一个通用替代方案.

## 2 RARR (只有 DIO 成功)

### 2.1 `react-native/d31...e32`

- 目标优化手段: 将 `std::string(lhs) == std::string(rhs)` 改为 C 字符串比较, 且保留 `lhs == rhs` 的同指针快速路径, 即 `lhs == rhs || strcmp(...) == 0`.

- `dio`: 输出为
	`if (lhs == rhs) return true; ... return std::strcmp(lhs, rhs) == 0;`
	和目标策略一致, 被判定通过.
- `ccs`: 仅替换成 `strcmp`，未保留同指针快速路径; judge 认为“思路不完整”.
- `rag`: 与 `ccs` 基本相同, 缺少 `lhs == rhs` 快速路径.
- `rgn`: 与 `ccs`/`rag` 相同, 同样因“缺少 pointer short-circuit”被拒.

### 2.2 `git/be9...ed3`

- 目标优化手段: 在 `len == namelen` 分支中, 将 `cache_name_compare(...)` 直接替换为 `memcmp(...)`.

- `dio`: 精确命中 `if (len == namelen && !memcmp(...))`, 与参考策略一致, 成功.
- `ccs`: 同时改了 `ce_namelen(ce)` 到 `ce->ce_namelen` 并加入 `ce->name == name` 分支; judge 判定为策略偏移.
- `rag`: 加了首字符预检 (`ce->name[0] != name[0]`) 但保留 `cache_name_compare`, 不是参考中的 `memcmp` 替换.
- `rgn`: 改写为 `len != namelen` 早返回并保留 `cache_name_compare`, 与目标策略不同.

## 3 RRAR (只有 RAG 成功)

### 3.1 `git/df7...2fc`

- 目标优化手段: 将逐字节 `putchar(0)` 改为大块零缓冲写出 (`static const char zeros[256*1024]` + `write`).

- `rag`: 采用“块写零缓冲”主思路 (32KB + `fwrite`), 被 judge 认定为同类策略并通过.
- `ccs`: 也是块写 (BUFSIZ + `fwrite`), 但 judge 认为与参考在 `write`/缓冲大小/静态缓冲细节不一致而拒绝.
- `dio`: 8KB 静态缓冲 + `fwrite`, 同样被判细节不一致.
- `rgn`: 32KB + `fwrite` + `memset`, 亦被判与参考策略不同.

> 该样本里四个方法都抓到了“批量写零”的核心, 但 judge 对“是否同策略”的尺度不稳定, 导致只有 `rag` 过.

### 3.2 `tensorflow/e87...22e`

- 目标优化手段: ELU 分支把 `exp(x) - 1` 改为 `expm1(x)` (逐分量替换).

- `rag`: 该轮被判通过, 但其产物并未实现 `expm1`; 从 `jr.log` 可见 judge 在该样本出现了“提案缺失/假设提案等于参考”的异常推理.
- `ccs`: 重构为静态字符串表, 未触及关键 `expm1` 替换, 被拒.
- `dio`: 把 ELU 改成向量 `select(exp(...) - 1, ...)`, 与参考“`expm1` 微调”是不同策略.
- `rgn`: 与 `dio` 同类 (向量化 `select`), 仍未命中 `expm1` 关键点.

> 该样本更像 judge 误判导致的“单方法成功”, 不是优化器真实能力的单点领先.

## 4 RRRA (只有 RGN 成功)

### 4.1 `git/a27...340`

- 目标优化手段: 用 `COPY_ARRAY` 替换两段手写拷贝, 且第二段 `COPY_ARRAY(ret + a_len, b, b_len + 1)` 一次性包含 `OPTION_END`.

- `rgn`: 提案使用 `memcpy` 批量拷贝并单独写回 `OPTION_END`; judge 认为与参考“同核心思路”并通过.
- `ccs`: 与 `rgn` 非常接近, 但被 judge 判为“实现细节偏离参考宏用法”.
- `dio`: 与 `ccs` 类似, 同样因“没用 `COPY_ARRAY` + `b_len+1` 一次拷贝”被拒.
- `rag`: 也走 `memcpy` 路线, 但写回终止项方式不同 (`.type = OPTION_END`), 被判策略不匹配.

### 4.2 `v8/c8e...37f`

- 目标优化手段: `IsEnqueued()` 增加 `if (jobs_.empty()) return false;` 再复用 `GetJobFor`.

- `rgn`: 该轮被判通过, 但产物实际上是手写 `equal_range` 扫描, 且去掉了 `IsScript` 检查, 与参考并不一致.
- `ccs`: 尝试 `ALWAYS_INLINE` + 手写查找, 偏离“empty fast path + 复用 GetJobFor”策略.
- `dio`: 直接 `jobs_.find(key)` 可能忽略同 key 多 job 的歧义校验 (`IsAssociatedWith`), 有行为风险.
- `rag`: 只加了 `IsScript` 早返回, 未命中参考中的 `jobs_.empty()` 快路径.

> 该样本再次显示 judge 存在“通过样本与参考不一致”的噪声; 解释 RRRA 时应把评估器不稳定性纳入结论.

## 5 总结

综合本次 case study 与 README 中的实现细节, 四种策略可以概括为: `dio`(纯 direct prompt) 提供最稳的通用 baseline, `rag`/`rgn` 提供检索增强但对检索分布与 judge 一致性更敏感, `ccs`(strategy library) 在“命中策略模式”时上限更高但依赖策略选择质量.

### 5.1 四种策略的优缺点

- `dio` (Direct I/O)
	- 优点: 流程简单、可解释性高、错误模式清晰; 在目标改动“局部且直接”时容易命中.
	- 缺点: 缺少外部先验, 容易停留在“通用微优化”层面; 遇到需要特定历史技巧的样本时上限较低.

- `rag` (跨仓库多样性约束检索)
	- 优点: 能把历史优化模式迁移到当前任务; 通过“同仓库排除 + 每仓库最多一例”控制同质化和潜在泄漏, 泛化更稳.
	- 缺点: 检索样本虽然更“干净”, 但也可能丢掉最贴近目标实现细节的案例; 在 strict strategy-match 下, 常出现“方向对但细节不完全同构”被拒.

- `rgn` (弱约束检索, 仅去掉同 commit)
	- 优点: 更容易检到与目标非常近的同仓库案例, 命中“代码形态相似策略”的概率更高, 在部分样本上会显著抬升通过率.
	- 缺点: 更依赖数据分布, 容易产生“看起来很会做这类 repo, 但跨 repo 迁移一般”的现象; 同时更容易放大评测噪声(例如通过样本与 reference 并不完全一致).

- `ccs` (Strategy Library / 两阶段)
	- 优点: 先选策略再生成, 对“需要抽象策略迁移”的任务有明显优势; 一旦策略选中正确, 生成质量上限高.
	- 缺点: 误选策略时代价较大(会系统性偏航); 两阶段链路更复杂, 对候选策略召回和选择 prompt 都敏感.

### 5.2 关键现象: `rag` vs `rgn` 的检索边界影响结果形态

根据实现定义:

- `rag`: 排除同 repo 的所有 commit, 并限制每个 repo 最多一个示例.
- `rgn`: 仅排除“当前 exact commit”, 允许同 repo 其他 commit 大量进入 few-shot.

这会带来一个很明显的 trade-off:

- `rag` 更像“跨项目迁移”设定, 结论更能反映方法的泛化能力.
- `rgn` 更像“同项目近邻检索”设定, 往往更容易命中局部代码习惯, 但结果中掺入了更强的数据分布偏置.

因此, 若目标是评估“真实可迁移的优化能力”, 应优先参考 `rag` 风格约束; 若目标是“同生态内尽可能提高命中率”, `rgn` 更有工程实用性.

### 5.3 对本实验结论的解释边界

- 本任务使用 strict judge, 且已在多个样本中观察到 judge 对“策略是否一致”的判定存在不稳定/噪声.
- 因此, 单个样本上的 A/R 不应被过度解读为优化器绝对优劣; 更合理的解读是“策略命中能力 + 检索分布 + 评测噪声”三者共同作用的结果.

