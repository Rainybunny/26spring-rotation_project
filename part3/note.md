**ClickHouse** `32dcbfb6273fffacda1ec09c8cbf737f82ca0d04`

核心差异: 将 `iterateMetadataFiles` 调用时, lambda 函数参数中对变量 `context` 的捕获方式由复制改为引用.

优化策略: 通过引用传递避免不必要的对象复制.

---

**folly** `2e29c4715e0957e44e8e3fdf8e37ca7f4526b6f3`

核心差异: 将 mutex 上锁方式由 `std::unique_lock` 改为 `std::lock_guard` (不改变锁范围与锁逻辑).

优化策略: <TODO>

---

**mysql-server** `aea6bd5081a4785593512467f504bcf8242ce5be`

核心差异: 在循环 `while (!ok)` 中, 每当设置变量 `ok = false` (若干种条件满足了其一) 就立即 `continue` 或 `break` (内层循环时), 消除无效的条件判断.

优化策略: <TODO>

---

**xgboost** `5c8ccf4455e3eef3f863573958853bd6240800bd`

核心差异:

1. 在多线程任务时, 每个线程需要 `discard_size` 个采样, 于是将线程 `tid` 遗弃的样本数量设置为 `discard_size * tid` (而非之前的 `2 * discard_size + tid`).

2. 生成 boolean 采样时, 由 "线程内 `mt19937` 生成 + `bernoulli_distribution` 采样" 改为 "线程内 `mt19937` 生成 + 与预处理好的整数阈值比较".

优化策略: <TODO>

---

**opencv** `12b8d542b7465f495681d3dc0c50cd27f7e0ee94`

核心差异: 调整指令集调用的顺序, 将原本分离进行的多个 `vx_load` 和多个 `v_muladd` 交错执行, 更利于指令多发射和流水化.

优化策略: <TODO>
