# Benchmarking Methodology & Performance Engineering

## 1. Principles of Low-Latency Measurement

Isolated matching engine microbenchmarks evaluate the inner data structure latency independent of peripheral costs:
- **Exclusion of File I/O & Logging**: File writes, disk syncs, and console output add orders-of-magnitude noise (hundreds of microseconds to milliseconds) and must never be measured inside an engine matching microbenchmark.
- **Pre-generated Deterministic Workloads**: Benchmark events are pre-allocated in vectors prior to measurement loops. Random number generation is excluded from the timed hot loop.
- **Dead-Code Elimination Prevention**: All output execution reports, trade vectors, and snapshot structures are passed to `benchmark::DoNotOptimize(...)`.
- **Cache Warmup & State Realism**: Fixtures pre-fill books with realistic bid and ask distributions across dozens of price levels.

## 2. Microbenchmark Workload Definitions

| Benchmark Name | Workload Description | Target Operation |
|---|---|---|
| `BM_LimitOrderInsertNonCrossing` | Inserts non-crossing limit orders into alternating bid/ask levels | Arena allocation, price level insertion, FIFO append |
| `BM_OrderCancellation` | Cancels pre-populated active orders across multiple price levels by Order ID | Hash map lookup, intrusive doubly-linked list unlinking, arena slot recycling |
| `BM_MarketOrderMatchingSingleLevel` | Aggressive taker IOC order executing against head resting order | Price queue traversal, trade generation, execution report creation, maker reduction |
| `BM_MarketOrderMatchingMultiLevel` | Aggressive taker order sweeping across 10 distinct price levels | Multi-level traversal, map level deallocation, multiple fills |
| `BM_MixedWorkload` | 50% limit insertions, 30% cancellations, 20% market executions | Combined realistic steady-state exchange workload |
| `BM_OrderArenaAllocationAndRelease` | Micro-allocation and free-list recycling | Memory pool throughput and generation increment |
| `BM_OrderBookL2Snapshot` | Aggregates 10-level price and quantity depth | Map iteration, aggregate reading |
| `BM_OrderBookL3Snapshot` | Traverses individual orders across 10 levels | FIFO queue traversal, queue position numbering |

## 3. Latency vs. Throughput Distinction

- **Throughput ($M\text{ ops/sec}$)**: Number of operations processed per second under sustained back-to-back load.
- **Internal Matching Latency ($\text{ns/op}$)**: Elapsed CPU time per single order processing event in the engine.
- Note: Core engine matching latency does *not* represent real-world network wire-to-wire latency (which includes TCP stack, network NICs, kernel bypass, and gateway serialization).
