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
1. **并发执行**：使用 `ThreadPoolExecutor` 并发运行优化任务和评测任务，提高大规模测试的效率。
2. **LLM Judge**：使用大模型作为裁判（Judge），而不是依赖单元测试（因环境配置复杂）。Judge 的 Prompt 包含以下准则：
    - 检查代码是否为有效的函数替换。
    - 分析优化后的代码是否在保持正确性的前提下提升了性能（运行效率或资源消耗）。
    - 输出 `VERDICT: TRUE/FALSE` 及详细理由。

## 3.2 - Direct Prompt 方法 (Baseline)

实现了 `optimizer_direct_io.py` 作为基准方法，采用直接提示（Direct Prompting）策略。

- **核心策略**：CoT (Chain of Thought)。
- **Prompt 设计**：
    - 角色设定为 "Expert C/C++ optimization engineer"。
    - 强制模型分四步思考：
        1. 分析原始代码瓶颈（Bottlenecks），关注不必要的拷贝、内存分配和循环逻辑。
        2. 提出优化方案。
        3. **主要约束**：必须选择**基于单一修改点**的高影响力优化，避免大幅度重构。
        4. 应用优化。
    - 输出格式规范：要求将思考过程与最终代码块分离，便于解析。

## 3.4 - RAG (Retrieval-Augmented Generation) 方法

实现了 `optimizer_rag.py`，利用 SemOpt 知识库中的历史优化案例来增强模型的优化能力。

### 1. 知识库构建 (Knowledge Base)
- **向量数据库**：使用 `ChromaDB` 存储和检索。
- **构建流程**：
    - 遍历 `knowledge_base/` 目录下的所有仓库（filtered by `repo_list`）。
    - 提取每个优化 commit 的 **原始函数代码 (`before_func`)** 作为文档进行 Embedding。
    - 存储元数据（Repo Name, Commit Hash），用于后续过滤。
    - 实现了分批写入（Batch Processing）以增强稳定性。

### 2. 检索增强流程
- **相似度检索**：针对待优化的 `mission.origin_func`，并在向量数据库中检索 Top-5 相似案例。
- **过滤策略**：
    - 过滤掉来自 **同一个仓库** 的案例（`x[1]['repo_name'] != mission.repo_name`），防止数据泄露（Test leakage）。
    - 选取相似度阈值（0.8）以上的案例，保留 Top-3。
- **Prompt 增强**：
    - 将检索到的 "原始代码 -> 优化后代码" 对作为 Few-Shot Examples 注入到 User Prompt 中。
    - 指令模型分析这些参考案例中的优化模式（Pattern），并判断是否可迁移到当前任务。
    - 这种方法能引导模型学习特定的代码变换模式，而不仅仅依赖通用的训练知识。
