# System Architecture & Design

## 1. Overview and Mission

The **Limit Order Book Simulator & Market-Making Engine** is a high-performance C++17 modular simulation system designed to research exchange matching semantics and quantitative market-making strategies under deterministic microstructural conditions.

## 2. Module Boundaries & Data Ownership

```mermaid
graph TD
    MD[Market Data / Historical Replay] --> |Normalized Events| SIM[Event-Driven Simulator]
    GEN[Synthetic Market Generator] --> |Generated Orders| SIM
    
    SIM --> |Order Requests| ME[Matching Engine]
    ME --> |Authoritative State| OB[Order Book]
    OB --> |L2/L3 Snapshots & BBO| MMS[Market-Making Strategy]
    
    MMS --> |Reservation Price & Quotes| SIM
    ME --> |Trades & Execution Reports| SIM
    
    SIM --> |Trade & Fill Flow| ACC[Portfolio Accounting]
    SIM --> |Metrics & Decision Flow| MET[Analytics & Reporting]
    
    ACC --> |Cash, Inventory, Drawdown| MET
    MET --> |JSON, Markdown, CSV| OUT[Artifacts & Reports]
```

### Module Responsibilities

1. **`lob::core`**: Fundamental types (`Price`, `Quantity`, `OrderId`, `TradeId`, `Timestamp`), error category, and centralized YAML configuration.
2. **`lob::book`**: Authoritative resting order state. Combines a cache-conscious contiguous array (`OrderArena`) with doubly-linked FIFO price queues and ordered price indices (`std::map`).
3. **`lob::matching`**: Exchange matching logic enforcing deterministic price-time priority, limit and market order execution, partial fills, GTC/IOC/FOK enforcement, and invariant validation.
4. **`lob::market_data`**: CSV parsers, normalizers, data quality validators, and a 6-scenario pseudorandom synthetic market generator.
5. **`lob::strategy`**: Implementation of the Avellaneda–Stoikov market-making model, rolling realized volatility estimation, inventory controls, quote replacement thresholding, and top-of-book liquidity imbalance extensions.
6. **`lob::simulation`**: Deterministic simulation coordinator driving the event loop, routing strategy orders to the matching engine, and maintaining simulation time.
7. **`lob::analytics`**: Mark-to-market portfolio accounting, transaction fees/rebates, cash and inventory ledgers, adverse selection post-fill markouts, and automated reporting.

## 3. Determinism and Latency Separation

The matching engine is strictly single-threaded and deterministic. Given identical inputs and random seeds, runs produce byte-for-byte identical trade ledgers and execution reports. Wall-clock timing is isolated to benchmark harnesses and never pollutes engine matching decisions.
