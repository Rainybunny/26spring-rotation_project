这是关于本人对轮转项目完成情况的记录和说明. 对于轮转项目本身的介绍请见 `TASK_README.md` 文件.

# Part 1/2

不必多言. Part 2 中的 prompt 非常简陋, 但我觉得它已经足够了.

# Part 3 - 使用大模型自动优化代码

本部分实现了基于大模型的 C/C++ 代码自动优化工具，并对比了 Direct Prompt 和 RAG 两种方法的实现。

## 3.1 & 3.3 - 基础设施与评估 (Infrastructure & Evaluation)

### 基础工具 (`utils.py`)
- **数据封装**：
    - `CommitInfo` 类：解析 benchmark 和知识库中的提交信息，自动提取修改前后的文件内容及函数片段（`before_func`/`after_func`）。
    - `Mission` 类：继承自 `CommitInfo`，用于管理单个优化任务的生命周期，包括生成目标路径、存储优化结果和评测理由。
- **模型配置**：统一配置了基于 LangChain 的 ChatOpenAI 接口和 OpenAIEmbeddings，用于生成和向量检索。

### 评估机制 (`judge.py`)
为了自动化评估优化结果，实现了一个并发评测系统：
1.  **并发执行**：使用 `ThreadPoolExecutor` 并发运行优化任务和评测任务，提高大规模测试的效率。
2.  **LLM Judge (Strict Mode)**：默认使用 `llm_judge_strict`，这是一个更为严格的评测标准。
    -   **Prompt 准则**：
        1.  **VALIDITY**：代码必须是有效的函数替换，保持正确性。
        2.  **PERFORMANCE**：必须提升性能。
        3.  **STRATEGY MATCH**：**（关键）** 优化的策略必须与 Reference Solution (Ground Truth) 一致。如果参考答案用了某种算法改进，模型也必须用同样的策略，否则判负。
    -   **输入**：
        -   原始完整代码。
        -   模型生成的优化代码。
        -   **参考答案（Ground Truth）**的优化代码。

## 3.2 - Direct Prompt 方法 (Baseline)

实现了 `optimizer_direct_io.py` 作为基准方法，采用直接提示（Direct Prompting）策略。

### Prompt 构建详情
该方法的 Prompt 设计包含以下关键部分（代码复用于 `utils.py`）：

1.  **角色设定 (`system_prompt_role_prelog`)**：
    -   设定为 "Expert C/C++ optimization engineer"。
    -   明确目标：最大化运行时效率，最小化资源使用。
    -   **核心约束**：严禁改变代码行为（Behavior Preservation），不关注代码风格。

2.  **思维链 (Chain of Thought) 指令**：
    -   强制模型按步骤执行：
        1.  **瓶颈分析**：分析原始代码。
        2.  **方案提出**：优先考虑简单、高影响力的修改。
        3.  **方案选择**：必须只选择**一个**优化点。
        4.  **代码应用**：实施优化。

3.  **输入内容**：
    -   通过 `mission.show_goal()` 传入：
        -   **Full Source Code**：提供完整的上下文。
        -   **Target Function**：明确指出需要优化的特定函数。

4.  **输出格式规范 (`system_prompt_final_output_format`)**：
    -   要求首先输出思考过程。
    -   最后在一个 Markdown 代码块中输出完整的优化后函数，便于用于正则匹配提取。

## 3.4 - RAG (Retrieval-Augmented Generation) 方法

实现了 `optimizer_rag.py`，利用 SemOpt 知识库中的历史优化案例来增强模型的优化能力。

### 1. 知识库构建 (Knowledge Base)
-   **向量数据库**：使用 `ChromaDB` 存储和检索。
-   **构建流程**：
    -   遍历 `knowledge_base/` 目录下的所有仓库（filtered by `repo_list`）。
    -   提取每个优化 commit 的 **原始函数代码 (`before_func`)** 作为文档进行 Embedding。
    -   存储元数据（Repo Name, Commit Hash），用于后续过滤。
    -   实现了分批写入（Batch Processing）以增强稳定性。

### 2. Prompt 构建详情
在 Baseline 的基础上，RAG 方法在 User Prompt 中动态注入了检索到的上下文：

-   **检索与过滤**：
    -   基于 `mission.origin_func` 在向量库中检索 **Top-20** 相似案例。
    -   **防泄漏过滤**：排除同仓库的 Commit (`repo_name != mission.repo_name`)。
    -   **多样性过滤**：为了避免参考案例同质化，限制每个仓库最多只贡献一个案例。
    -   **质量过滤**：仅保留检索距离小于 **1.2** 的案例（距离越小越相似）。
    -   **数量限制**：最多保留 **4** 个案例。

-   **上下文注入 (`rag_context`)**：
    -   检索到的案例被格式化为 Few-Shot 示例，插入到 User Prompt 的开头：
        ```text
        Example 1 (Repo: ..., Commit: ...):
        Similarity distance: ...
        Original Function: ...
        Optimized Function: ...
        ```
    -   这些示例被插入到 User Prompt 的开头，作为优化的参考。

-   **指令增强**：
    -   **System Prompt**：增加了步骤，要求分析参考案例中的优化技术是否可迁移到当前任务。
    -   **User Prompt**：明确要求 "Based on the reference examples above, analyze if any of the optimization techniques... can be applied"。

## 3.5 - Claude Code Skill (Strategy Library) 方法

实现了 `optimizer_ccskill.py`，模拟了人类专家“查阅策略库 -> 选择策略 -> 应用策略”的流程。

### 1. 策略库构建
-   基于 SemOpt 的聚类结果 (`data/0_76.json`)。
-   使用 LLM 为每个簇生成简短的 **Strategy Summary**。
-   为每个策略关联具体的 **Reference Examples** (Repo & Commit Hash)。
-   策略库被持久化在 `data/strategies.json` 中。
-   **预计算 Embedding**：为了加速检索，预先计算所有 Strategy Summary 的向量表示并缓存。

### 2. 两阶段 Prompt 构建

**阶段一：策略检索与选择 (Strategy Retrieval & Selection)**
-   **预筛选 (Retrieval)**：
    -   为了避免将所有策略放入 Prompt 导致 Context 溢出或干扰，先计算待优化代码与所有策略 Summary 的相似度。
    -   检索出 **Top-12** 个最相关的候选策略。
-   **Prompt 输入**：
    -   **候选策略目录**：仅包含筛选出的 12 个策略的 ID、Summary 和描述。
    -   **待优化代码**。
-   **任务**：分析代码瓶颈，从候选目录中选出最合适的一个优化策略 ID (或 None)。

**阶段二：基于策略的优化 (Optimization)**
-   模型根据选中的策略 ID 获取详细信息（Summary + Examples）。
-   **Prompt 增强**：
    -   **System Prompt**：加入动态指令，要求解释为何该策略适用，以及如何适配该策略到当前代码。
    -   **User Prompt**：注入 **Selected Strategy Details**，展示具体的参考代码对（Source -> Target）。
    -   如果未选中策略，则回退到类似 Direct Prompt 的通用优化模式。

这种方法相比 RAG 的纯相似度匹配，引入了更高层的语义抽象（策略），能更好地处理跨仓库但逻辑相似的优化场景，同时通过两阶段检索减少了 Token 消耗。
