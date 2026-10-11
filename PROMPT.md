# MASTER ENGINEERING PROMPT: Limit Order Book Simulator & Market-Making Engine

## 1. Your Role and Mission

Act as a **Senior C++ Systems Engineer, Quantitative Research Engineer, Low-Latency Trading Infrastructure Engineer, and Quantitative Developer**.

Your task is to design and implement a complete, production-quality educational research system called:

**Limit Order Book Simulator & Market-Making Engine**

The project must demonstrate practical knowledge of:

- C++17 systems programming and modern C++ engineering practices.
- Limit order books and exchange matching engines.
- Level 2 (L2) and Level 3 (L3) market data.
- Price-time priority and deterministic order matching.
- Low-latency data structures and memory management.
- Event-driven simulation and historical market-data replay.
- Quantitative market making using the Avellaneda–Stoikov model.
- Inventory management and execution risk.
- Backtesting, transaction costs, adverse selection, and drawdown analysis.
- Statistical evaluation of fill probabilities and trading performance.
- Reproducible performance benchmarking using Google Benchmark.
- Professional software testing, documentation, and reproducible research.

This is a resume project intended to be examined by quantitative developers, trading-system engineers, and quantitative researchers at demanding technical interviews.

Do not build a toy implementation that merely demonstrates a bid and an ask. Build a cohesive research-grade system with a realistic matching engine, a market-making strategy, a historical replay pipeline, rigorous evaluation, and reproducible performance measurements.

The final repository must be understandable, modular, testable, and defensible in a technical interview.

### Mandatory implementation principles

1. Use C++17 as the primary implementation language.
2. Use the Standard Template Library (STL) wherever appropriate.
3. Use CMake for build configuration.
4. Use Google Benchmark for performance measurements.
5. Use GoogleTest or an equivalently structured C++ testing framework for unit and integration tests.
6. Use deterministic event processing and reproducible simulation runs.
7. Separate the matching engine, strategy, market data, simulation, analytics, and benchmarking code.
8. Use strict compiler warnings and make the project work with a documented, supported compiler toolchain.
9. Avoid unnecessary dependencies, overengineering, and artificial abstractions.
10. Never fabricate benchmark results, market data, profitability, or experimental findings.
11. Document every important architectural decision and every non-obvious quantitative assumption.
12. Provide a complete README that explains the project from first principles through implementation, execution, testing, benchmarking, and interpretation of results.

You are responsible for the actual implementation, not just architecture diagrams, pseudocode, or a project skeleton. Continue through implementation, testing, benchmarking, debugging, and documentation.

If execution tools are available, compile and run the project, fix discovered problems, and capture real outputs. If a task cannot be executed in the available environment, explicitly identify the limitation rather than claiming success.

---

# 2. Project Objective and Scope

Build a discrete-event limit order book simulator with two primary capabilities.

### Capability A: Exchange-style matching engine

Implement an order book that accepts order events, maintains bid and ask liquidity, matches incoming orders, generates trades, and updates market state.

The engine must support:

- Limit orders.
- Market orders.
- Partial fills.
- Resting orders.
- Order cancellation.
- Order modification under explicitly defined exchange semantics.
- Order replacement.
- Order rejection.
- Order identifiers.
- Trade identifiers.
- Multiple price levels.
- Price-time priority.
- Bid and ask depth queries.
- Order-book snapshots.
- L2 market depth.
- L3 individual-order depth.
- Deterministic trade and event generation.
- A configurable tick size and lot size.
- Configurable price and quantity constraints.

### Capability B: Quantitative market-making simulator

Build an event-driven market-making strategy that interacts with the matching engine.

The strategy must:

- Estimate or receive volatility.
- Estimate market-making inventory.
- Calculate an inventory-adjusted reservation price.
- Calculate a theoretical optimal half-spread.
- Generate bid and ask quotes.
- Respect inventory limits.
- Incorporate market-data liquidity information through a clearly specified extension to the baseline model.
- Manage outstanding quotes and cancellations.
- Simulate executions and inventory changes.
- Measure spread capture and mark-to-market P&L.
- Evaluate adverse selection.
- Analyze fill probabilities.
- Compare strategy configurations using reproducible experiments.

The system must also replay historical tick-by-tick or order-book event data when a suitable dataset is supplied.

Historical replay and synthetic simulation must be separate, clearly labeled modes. Synthetic data must never be presented as historical market data.

---

# 3. System Architecture

Design the project as a set of independent modules with explicit responsibilities.

Use a modular monolithic executable/library architecture initially. Do not introduce distributed services, networking infrastructure, databases, or multithreaded matching unless they provide a concrete benefit to the stated requirements.

The matching engine must be single-threaded and deterministic by default. This provides a clear ordering of events and makes correctness, debugging, and performance measurements easier to establish.

A future multithreaded market-data ingestion or analytics layer may be documented, but it must not compromise deterministic matching.

## 3.1 Required module boundaries

### Module 1: Core types

Define fundamental types and shared interfaces.

Responsibilities:

- Price representation.
- Quantity representation.
- Order IDs.
- Trade IDs.
- Event sequence numbers.
- Timestamps.
- Side enumeration.
- Order type enumeration.
- Time-in-force enumeration.
- Order status.
- Execution reports.
- Trade records.
- Market-data events.
- Configuration structures.
- Validation results.
- Error handling.

Prefer strongly typed identifiers and enums over arbitrary strings.

Use integer price ticks and integer quantities for the matching engine.

Represent prices internally as integer tick units, not floating-point numbers. For example, a market with a tick size of 0.01 can represent a price of 101.25 as 10125 ticks.

Convert integer prices into decimal or floating-point representations only at reporting, strategy-modeling, or data-import boundaries where necessary.

Use fixed-width integer types such as `std::int64_t` for prices and quantities where appropriate.

Define safe handling for:

- Integer overflow.
- Invalid quantities.
- Invalid prices.
- Zero or negative quantities.
- Duplicate order IDs.
- Invalid timestamps.
- Unsupported order types.
- Malformed input records.

Never allow invalid state to silently enter the matching engine.

### Module 2: Order book

Maintain the current state of resting orders.

Responsibilities:

- Store active orders.
- Maintain bid-side price levels.
- Maintain ask-side price levels.
- Preserve FIFO ordering within each price level.
- Support insertion and removal.
- Support cancellation by order ID.
- Support modification and replacement.
- Expose L2 aggregated depth.
- Expose L3 order-level depth.
- Provide best bid, best ask, and mid-price.
- Calculate spread and depth statistics.
- Produce consistent snapshots.

The order book must not directly implement market-making decisions.

It must not depend on historical-data parsers, analytics output formats, or command-line argument handling.

### Module 3: Matching engine

Implement exchange matching semantics.

Responsibilities:

- Receive incoming order requests.
- Validate order requests.
- Apply price-time priority.
- Match incoming orders against the opposite side.
- Generate trade records.
- Generate execution reports.
- Update resting order quantities.
- Remove fully executed orders.
- Handle partially filled orders.
- Handle cancellations.
- Handle rejected orders.
- Maintain event sequence numbers.
- Notify downstream simulation components of trades and book changes.

The matching engine owns the authoritative book state.

### Module 4: Market-data ingestion

Implement parsers and adapters for external market data.

Responsibilities:

- Parse supported input formats.
- Validate input fields.
- Normalize timestamps.
- Convert prices into integer ticks.
- Convert quantities into integer units.
- Preserve source sequence information where available.
- Detect malformed records.
- Detect missing sequence numbers where applicable.
- Report data-quality problems.
- Produce normalized events for replay.

Provide a synthetic-data generator as a separate component.

### Module 5: Event-driven simulator

Implement a deterministic simulation loop.

Responsibilities:

- Process events in timestamp and sequence order.
- Maintain simulation time.
- Deliver market-data events.
- Update the strategy's market state.
- Generate strategy quote requests.
- Submit orders to the matching engine.
- Process execution reports.
- Update inventory and cash.
- Track outstanding quotes.
- Apply configurable fees and rebates.
- Produce event logs and simulation summaries.

Keep the simulation clock independent of wall-clock time.

### Module 6: Market-making strategy

Implement the Avellaneda–Stoikov baseline strategy and an optional liquidity-aware extension.

Responsibilities:

- Estimate volatility.
- Calculate reservation prices.
- Calculate optimal half-spreads.
- Generate bid and ask quote prices.
- Apply inventory constraints.
- Apply tick-size rounding.
- Respect quantity limits.
- Manage quote lifecycle.
- Determine when to cancel or replace quotes.
- Record decisions and their inputs.

The strategy must be able to interact with the engine only through defined interfaces, not by modifying internal order-book structures directly.

### Module 7: Analytics and reporting

Responsibilities:

- Calculate trading P&L.
- Calculate realized and unrealized P&L.
- Calculate spread capture.
- Calculate fill rates.
- Estimate empirical fill probabilities.
- Measure adverse selection.
- Measure inventory exposure.
- Calculate maximum drawdown.
- Summarize quote activity.
- Compare parameter configurations.
- Export machine-readable results.
- Generate experiment summaries.

### Module 8: Benchmarking

Use Google Benchmark to measure the performance of the core engine.

Responsibilities:

- Benchmark order insertion.
- Benchmark order cancellation.
- Benchmark matching.
- Benchmark partial fills.
- Benchmark full fills.
- Benchmark order-book depth queries.
- Benchmark mixed event workloads.
- Benchmark the complete matching path.
- Measure throughput.
- Report latency distributions using a separate measurement harness when necessary.

Separate microbenchmarks from end-to-end strategy and simulation benchmarks.

### Module 9: Application and CLI

Provide a clean command-line interface.

The application should support commands conceptually equivalent to:

- `run-simulation`
- `replay-data`
- `run-market-maker`
- `benchmark`
- `validate-data`
- `export-book`
- `summarize-results`

Choose a command-line implementation appropriate to a small C++17 project. A lightweight dependency is acceptable, but a dependency-free CLI is also valid.

Provide meaningful help output, argument validation, configuration loading, and clear error messages.

### Module 10: Configuration

Use version-controlled example configuration files.

Support configuration of:

- Initial cash.
- Instrument tick size.
- Lot size.
- Initial market state.
- Matching-engine settings.
- Strategy parameters.
- Volatility estimation window.
- Inventory limit.
- Order quantity.
- Quote update frequency.
- Latency assumptions.
- Transaction fees.
- Rebates.
- Replay settings.
- Output directory.
- Random seed.
- Benchmark workload size.

Separate strategy parameters from matching-engine parameters and market-data settings.

Reject invalid configurations early.

---

# 4. Limit Order Book Data Structures

This project must explicitly compare data-structure trade-offs rather than assuming that a particular container is always fastest.

Implement the primary matching engine around a cache-conscious design using contiguous storage for order records and a suitable FIFO representation for each price level.

## 4.1 Order representation

Define an order record containing fields such as:

- Order ID.
- Side.
- Price in integer ticks.
- Original quantity.
- Remaining quantity.
- Arrival sequence number.
- Timestamp or simulation arrival time.
- Order type.
- Time-in-force.
- Active/inactive status.
- Intrusive next-order index.
- Intrusive previous-order index.

Avoid storing redundant fields unless there is a documented performance or correctness reason.

Do not place owning strings or dynamically allocated containers inside every order record.

Use indices into contiguous arrays wherever appropriate.

## 4.2 Contiguous flat storage

Implement an order arena using `std::vector<Order>` or a suitably designed preallocated array.

Use stable integer handles rather than long-lived raw pointers to vector elements.

The implementation must account for vector reallocation invalidating pointers and references.

Provide a free-list or another explicit mechanism for reusing order slots when cancellation and execution remove orders.

Ensure that reused slots cannot accidentally be confused with stale order handles. Generation counters may be used if they simplify safe handle validation.

The design should aim to reduce unnecessary allocations and improve memory locality.

Document the trade-off between:

- Preallocating memory.
- Dynamically growing storage.
- Memory usage.
- Cache behavior.
- Allocation overhead.
- Implementation complexity.

Do not claim that contiguous storage automatically guarantees superior latency; validate the claim experimentally.

## 4.3 Price-level representation

Implement a price-level structure containing:

- Price.
- Aggregate remaining quantity.
- Number of resting orders.
- Head order index.
- Tail order index.
- Optional metadata needed for efficient traversal.

Each price level must preserve FIFO ordering among orders at the same price.

Use an intrusive doubly linked list implemented with indices into the order arena.

Insertion at the tail and removal of a known order should be O(1).

The aggregate quantity and order count must remain consistent with the underlying queue.

## 4.4 Price-level indexing

Choose a suitable indexing structure for the price levels.

A reasonable default is an ordered associative container keyed by integer price, with bids and asks stored separately.

Explain the complexity of insertion, deletion, best-price access, and traversal.

Evaluate alternatives only if there is a meaningful performance question to answer, such as:

- `std::map`.
- Hash-based indexing combined with an ordered price structure.
- A fixed or bounded tick-indexed array for markets with a known price range.

Do not add a custom balanced tree solely for resume appeal.

The final choice must be justified by correctness, simplicity, and measured performance.

## 4.5 L2 and L3 views

L2 depth aggregates all active orders at a price.

L3 depth preserves individual order information and queue position.

Provide functions conceptually equivalent to:

- `best_bid()`
- `best_ask()`
- `mid_price()`
- `spread_ticks()`
- `depth_at_price()`
- `get_l2_snapshot(depth)`
- `get_l3_snapshot(depth)`
- `total_bid_quantity(depth)`
- `total_ask_quantity(depth)`

An empty book must be handled safely. Do not fabricate a midpoint when either side is unavailable.

Define whether snapshots are returned as owning copies or read-only views. Prevent callers from mutating authoritative engine state through a snapshot interface.

---

# 5. Matching Engine Semantics

This is the most important correctness-critical component.

Implement a deterministic price-time-priority matching engine with exchange-like order lifecycle semantics.

## 5.1 Price-time priority

The matching rules must be:

1. A buy order may match sell orders whose prices are less than or equal to the buy limit price.
2. A sell order may match buy orders whose prices are greater than or equal to the sell limit price.
3. The best available opposite-side price has priority.
4. Within a price level, the earliest eligible resting order has priority.
5. Each execution quantity is the minimum of the incoming order's remaining quantity and the resting order's remaining quantity.
6. The resting order's price determines the execution price for the baseline continuous limit order book.
7. Fully filled orders are removed from the book.
8. Partially filled resting orders remain at their existing queue position.
9. An incoming limit order's unfilled remainder rests on the book only when permitted by its time-in-force.
10. A market order must never rest on the book.

Ensure that the implementation does not violate price-time priority after partial fills, cancellations, or modifications.

## 5.2 Supported order types

Implement the following:

**Limit order:** Executes immediately against eligible opposite-side orders and rests with any remaining quantity when permitted.

**Market order:** Executes against available opposite-side liquidity. Any unfilled remainder is canceled rather than resting.

**Cancel request:** Removes an active order by order ID. Cancelling a nonexistent or already completed order must produce a documented response.

**Modify request:** Support quantity reductions and other explicitly defined modifications. Specify whether increasing quantity or changing price loses time priority.

**Replace request:** Treat a price-changing replacement as cancellation plus a new order unless the configured exchange semantics explicitly specify otherwise.

At minimum, implement Good-Til-Cancelled and Immediate-or-Cancel time-in-force behavior. Add Fill-or-Kill only if it is implemented correctly and tested.

A Fill-or-Kill order must be rejected or canceled without partial execution when its full quantity cannot be filled under the specified execution rules.

Do not implement advanced order types superficially.

## 5.3 Validation and invariants

Validate incoming requests before mutating the book.

Check:

- Positive quantity.
- Valid price for limit orders.
- Valid order identifier.
- Permitted tick-size alignment.
- Permitted lot-size alignment.
- Valid order type.
- Valid time-in-force.
- No illegal self-crossing state caused by corrupted internal data.
- No arithmetic overflow during quantity aggregation.

Maintain these invariants:

- Every active order belongs to exactly one price level.
- Every active order is reachable through its level's FIFO queue.
- The order-ID index references the correct active order.
- No canceled or fully executed order remains active.
- Aggregate level quantity equals the sum of remaining quantities in that level.
- Aggregate level order count equals the number of active orders in that level.
- The best bid is never greater than the best ask after a fully processed matching event, unless a documented special state or event-processing policy explicitly permits otherwise.
- Trade quantities are strictly positive.
- Executed quantities never exceed an order's remaining quantity.
- Trade prices satisfy the incoming order's limit constraint.
- Every generated trade has a unique identifier and traceable event sequence.

Add debug-mode invariant checks that can be enabled in tests and development builds.

## 5.4 Execution reports

Generate structured execution reports for:

- Accepted orders.
- Rejected orders.
- Partial fills.
- Full fills.
- Cancellations.
- Expirations.
- Replacements.
- Other explicitly supported lifecycle events.

Reports should contain relevant order identifiers, remaining quantities, executed quantities, prices, timestamps, and reason codes.

The matching engine must distinguish an order request from an execution report.

## 5.5 Event determinism

Assign monotonically increasing sequence numbers to engine-generated events.

Given the same ordered input event stream and configuration, the matching engine must produce the same trades, execution reports, and final book state.

Avoid relying on unordered-container iteration order for decisions that affect execution.

Provide a deterministic event log for debugging and replay.

---

# 6. Matching Engine Correctness Tests

Create a comprehensive automated test suite.

At minimum, include tests for the following scenarios.

### Basic book behavior

- Insert a bid.
- Insert an ask.
- Verify best bid and best ask.
- Verify midpoint and spread.
- Verify L2 aggregation.
- Verify L3 queue order.
- Insert multiple orders at the same price.

### Matching behavior

- Exact-price match.
- Incoming buy crossing the best ask.
- Incoming sell crossing the best bid.
- Better-priced liquidity executing before worse-priced liquidity.
- FIFO execution within the same price level.
- Full fill.
- Partial fill.
- Multiple fills from one incoming order.
- Multiple price levels consumed by one market order.
- Residual limit quantity resting on the book.
- Market-order residual cancellation.
- No execution when the limit price does not cross.

### Order lifecycle

- Cancel an active order.
- Cancel a nonexistent order.
- Cancel an already filled order.
- Modify quantity downward.
- Increase quantity according to the documented priority policy.
- Replace an order at a new price.
- Cancel a partially filled order.
- Reuse freed order storage safely.
- Reject duplicate order identifiers.

### Edge cases

- Empty bid side.
- Empty ask side.
- Empty book.
- Zero quantity.
- Negative quantity.
- Invalid price.
- Tick-size mismatch.
- Lot-size mismatch.
- Extremely large valid quantities.
- Aggregate quantity overflow protection.
- Invalid event sequence.
- Malformed external records.

### Property-based and stress tests

Where practical, add randomized order-event generation and verify invariants after every operation.

Run stress tests with many price levels, high order churn, repeated cancellation, and large numbers of resting orders.

Create differential tests against a simple, deliberately unoptimized reference implementation.

For identical inputs, compare the optimized engine's trades and final book state with the reference implementation.

The reference implementation must be structurally simple enough to serve as an independent correctness oracle.

Correctness takes precedence over performance.

---

# 7. Avellaneda–Stoikov Market-Making Model

Implement the Avellaneda–Stoikov model as a distinct quantitative module.

Document the assumptions and limitations of the model before presenting results.

The baseline strategy should follow the standard reservation-price and optimal-spread framework, with explicit definitions of units and parameters.

## 7.1 Core variables

Define:

- \(S_t\): current reference mid-price.
- \(q_t\): signed inventory.
- \(\gamma\): inventory risk-aversion parameter.
- \(\sigma\): estimated volatility of the reference price.
- \(T\): strategy horizon.
- \(t\): current model time.
- \(\tau = T-t\): remaining horizon.
- \(k\): order-arrival intensity decay parameter in the baseline model.
- \(A\): baseline order-arrival intensity parameter, if used.
- \(\Delta_b\): bid quote distance from the reference price.
- \(\Delta_a\): ask quote distance from the reference price.

Define the units of every parameter and variable in the documentation and configuration.

The baseline model should assume a continuous reference price and a defined volatility convention. The implementation must explicitly handle conversion from continuous model outputs to discrete exchange ticks.

## 7.2 Reservation price

Implement the standard reservation-price expression:

\[
r_t = S_t-q_t\gamma\sigma^2\tau
\]

Interpretation:

- Positive inventory reduces the reservation price.
- Negative inventory increases the reservation price.
- Higher volatility increases the inventory-risk adjustment.
- Greater risk aversion increases the inventory-risk adjustment.
- A longer remaining horizon increases the inventory-risk adjustment.

Document the assumptions under which this expression is used.

Do not confuse reservation price with the actual bid or ask quote.

## 7.3 Baseline optimal half-spread

Implement the baseline Avellaneda–Stoikov optimal half-spread expression:

\[
\delta_t =
\frac{\gamma\sigma^2\tau}{2}
+
\frac{1}{\gamma}
\ln\left(1+\frac{\gamma}{k}\right)
\]

The precise form of the expression must match the chosen model formulation.

Define the conventions for \(A\), \(k\), and arrival intensity. If an alternative parameterization is used, derive and document it rather than mixing equations from different formulations.

Validate parameter domains.

Handle the small-\(\gamma\) limit numerically so the implementation does not suffer avoidable instability.

Do not silently accept invalid values of \(\gamma\), \(k\), \(\sigma\), or the time horizon.

## 7.4 Bid and ask quote construction

Construct continuous target quotes as:

\[
p_b^*=r_t-\delta_t
\]

\[
p_a^*=r_t+\delta_t
\]

Convert these into valid exchange prices using an explicit tick-rounding policy.

The policy must preserve the intended quoting semantics. Document how rounding affects:

- Effective spread.
- Quote symmetry.
- Minimum quote distance.
- Potential market crossing.
- Inventory-adjusted skew.

The resulting prices must comply with tick-size rules.

If the model produces an invalid or negative theoretical quote, apply a documented instrument-specific policy instead of allowing undefined behavior.

## 7.5 Volatility estimation

Implement a configurable volatility estimator based on historical mid-price observations or returns.

A reasonable baseline is rolling realized volatility computed from log returns:

\[
r_i=\ln\left(\frac{S_i}{S_{i-1}}\right)
\]

Estimate volatility using a clearly defined rolling window.

Document whether the estimator returns:

- Per-observation volatility.
- Per-second volatility.
- Per-minute volatility.
- Annualized volatility.

Use consistent units when passing volatility into the strategy equations.

Avoid mixing annualized volatility with a horizon expressed in seconds.

Support a minimum number of observations before the strategy starts quoting.

Define safe behavior during warm-up and when price observations are missing or invalid.

An EWMA estimator may be included as an optional alternative.

## 7.6 Inventory constraints

Implement configurable inventory limits:

\[
q_{\min}\leq q_t\leq q_{\max}
\]

The strategy must not intentionally increase exposure beyond configured limits.

At the upper inventory limit:

- Disable or reduce bid-side quoting.
- Continue offering inventory-reducing liquidity when permitted.

At the lower inventory limit:

- Disable or reduce ask-side quoting.
- Continue bidding to reduce short exposure when permitted.

Apply constraints to actual inventory and outstanding order exposure where relevant.

Document whether outstanding buy and sell orders count toward projected inventory risk.

Prevent the strategy from repeatedly submitting quotes that cannot be accepted because of known risk constraints.

## 7.7 Liquidity-aware extension

The resume description mentions order-book liquidity dynamics. Do not falsely imply that the classical Avellaneda–Stoikov model inherently accounts for every microstructure variable.

Implement a separately identifiable liquidity-aware extension.

Candidate inputs include:

- Bid depth.
- Ask depth.
- Spread.
- Top-of-book imbalance.
- Recent trade intensity.
- Short-horizon volatility.
- Available opposite-side liquidity.

For example, calculate a normalized top-of-book imbalance:

\[
I_t =
\frac{Q_b-Q_a}{Q_b+Q_a}
\]

where \(Q_b\) and \(Q_a\) are bid and ask quantities over a documented depth range.

Define safe behavior when the denominator is zero.

Use the imbalance to adjust the reference price or quote skew through an explicitly parameterized extension.

Keep the classical baseline and liquidity-aware extension independently configurable.

Do not change the baseline model silently.

Run controlled experiments to determine whether the extension improves performance after transaction costs.

Do not assume a more complicated strategy is automatically better.

## 7.8 Quote lifecycle management

Implement a quote-management layer.

It should:

- Track outstanding bid and ask orders.
- Avoid duplicate live quotes at the same intended price unless explicitly supported.
- Cancel stale quotes.
- Replace quotes when strategy targets change sufficiently.
- Apply configurable quote update thresholds.
- Avoid excessive cancellation and replacement churn.
- Handle partial fills correctly.
- Recompute inventory after executions.
- Account for outstanding orders when enforcing risk limits.
- Record why each quote was placed, retained, modified, or canceled.

Model decisions must be logged with enough information to reconstruct the reservation price, half-spread, inventory, volatility, and liquidity inputs used at that moment.

---

# 8. Historical Market-Data Replay

Implement a historical replay pipeline that supports normalized market events.

The replay system must be capable of consuming historical tick-level or order-book message data, subject to the actual semantics of the available dataset.

Do not claim support for a proprietary feed format unless the parser is genuinely implemented and tested.

## 8.1 Supported initial format

Implement a clearly documented CSV format with fields appropriate to the selected replay mode.

For a trade-and-quote or top-of-book dataset, include fields such as:

- Timestamp.
- Event type.
- Symbol.
- Sequence number, if available.
- Trade price and quantity, where applicable.
- Bid price and quantity, where applicable.
- Ask price and quantity, where applicable.

For an order-level event stream, support fields such as:

- Timestamp.
- Event sequence.
- Event type.
- Order ID.
- Side.
- Price.
- Quantity.
- Reference order ID, where required.

Do not force incompatible data formats into a single ambiguous schema.

Define separate schemas or adapters for quote/trade replay and order-level reconstruction.

## 8.2 Important market-data distinction

A historical trade-and-quote feed does not necessarily contain the full order lifecycle.

Do not pretend that L2 snapshots or best bid/ask records provide enough information to reconstruct a complete L3 book.

Likewise, a historical order-level feed is not automatically a full exchange audit trail.

Document what can and cannot be reconstructed from each dataset.

## 8.3 Replay engine

The replay engine must:

- Read input incrementally where practical.
- Avoid loading enormous files entirely into memory without justification.
- Validate records.
- Preserve event ordering.
- Handle equal timestamps using sequence information where available.
- Report missing or duplicate sequence numbers.
- Support a configurable time range.
- Support filtering by symbol.
- Produce replay statistics.
- Record skipped or invalid records.
- Allow deterministic replay.

When source sequence information is unavailable, document the ordering convention and its limitations.

## 8.4 Strategy interaction with historical data

Clearly distinguish the historical external market from the strategy's own simulated orders.

The historical feed should supply reference-market information. The strategy's own simulated orders should be tracked separately.

Do not automatically insert historical public orders into the strategy's own book and then claim that the resulting fills reproduce actual exchange executions.

Implement a documented simulation mode that is consistent with the data available.

Possible modes include:

1. **Synthetic exchange simulation:** All background liquidity and agent orders exist within the simulator.
2. **Historical reference replay:** Historical quotes and trades drive the reference market, while strategy execution is simulated using an explicit fill model.
3. **Historical order-book reconstruction:** When suitable order-level events are available, reconstruct the observed book according to the documented feed semantics.

The third mode should only be enabled when the input data supports it.

## 8.5 Data-quality reports

Generate a report covering:

- Input file and schema.
- Record count.
- Valid record count.
- Rejected record count.
- Duplicate events.
- Out-of-order timestamps.
- Sequence gaps.
- Missing values.
- Invalid prices.
- Invalid quantities.
- Timestamp range.
- Symbol coverage.
- Parsing errors.

Do not silently discard malformed records.

---

# 9. Execution Simulation and Fill Modeling

A major requirement is realistic interpretation of order execution.

A matching engine can match orders against its own simulated book. However, a strategy replaying historical quotes does not automatically know whether a hypothetical order would have executed at a real exchange.

Implement distinct execution models with explicit assumptions.

## 9.1 Exact matching-engine mode

In synthetic simulation, strategy orders interact directly with the simulated exchange.

The exchange matching engine determines fills exactly according to the implemented matching rules.

## 9.2 Historical fill-model mode

When historical reference data is used, implement a separate execution-model interface.

A simple baseline may determine whether a hypothetical quote becomes marketable against subsequently observed prices.

However, the model must not claim to reproduce queue-level executions when the dataset lacks queue information.

Where appropriate, support a conservative touch-or-trade fill model, with explicit assumptions about whether a trade at the quote price is sufficient evidence for a fill.

Document:

- How a fill is triggered.
- Whether partial fills are possible.
- How execution quantity is determined.
- How queue position is approximated.
- How latency is modeled.
- How the strategy's own quotes affect simulated liquidity.
- What assumptions can cause optimistic bias.

Do not use future information to decide whether an order was filled before it could realistically have reached the market.

## 9.3 Queue-position modeling

Where L3 information is available, design an optional queue-aware model.

Track estimated queue position and changes in displayed quantity ahead of the simulated order.

Be explicit about uncertainties arising from:

- Hidden liquidity.
- Undisclosed orders.
- Market participants joining or leaving queues.
- Order modifications.
- Feed gaps.
- Unknown exchange-specific priority rules.

Do not label an estimated queue position as an exact position unless the input and matching rules justify that claim.

## 9.4 Latency modeling

Support configurable event delays for:

- Market-data reception.
- Strategy computation.
- Order submission.
- Exchange processing.
- Execution-report reception.

Use a deterministic simulation-time model.

A zero-latency configuration can be used as a baseline, but it must be labeled as idealized.

Add experiments showing how results change under different latency assumptions.

Do not use the measured C++ matching-engine benchmark as though it were total real-world order-routing latency.

---

# 10. Backtesting and Accounting

Implement a transparent backtesting and accounting framework.

Every simulation run must have an initial state, a deterministic event stream or random seed, a strategy configuration, and a defined terminal condition.

## 10.1 Cash and inventory

Maintain:

- Cash balance.
- Signed inventory.
- Average acquisition cost or another clearly specified accounting basis.
- Realized P&L.
- Unrealized P&L.
- Fees.
- Rebates.
- Trade count.
- Executed volume.

For a trade of quantity \(v\) at price \(p\):

- A buy increases inventory by \(v\) and decreases cash by \(pv\).
- A sell decreases inventory by \(v\) and increases cash by \(pv\).

Include transaction fees and rebates with a consistent sign convention.

Use a consistent accounting currency and quantity unit.

Use integer quantities and prices where appropriate for exchange state. Use sufficiently precise numerical types for financial reporting, and document rounding behavior.

## 10.2 Mark-to-market equity

Calculate equity using a documented valuation convention.

A baseline mid-price mark is:

\[
E_t=C_t+q_tS_t
\]

where \(C_t\) is cash, \(q_t\) is signed inventory, and \(S_t\) is the chosen reference price.

This is a mark-to-market convention, not necessarily the liquidation value of the position.

For liquidation-sensitive reporting, add an optional liquidation estimate that accounts for the cost of crossing the spread and, where supported, available depth.

## 10.3 Required performance metrics

Calculate at least the following:

**Trading performance**
- Gross P&L.
- Net P&L after transaction costs.
- Realized P&L.
- Unrealized P&L.
- Total return relative to a clearly defined capital base.
- P&L by trade and by time interval.

**Execution performance**
- Number of orders submitted.
- Number of orders canceled.
- Number of orders rejected.
- Number of partial fills.
- Number of completed fills.
- Executed volume.
- Fill rate.
- Quote-to-fill ratio.
- Average order lifetime.
- Average time to fill, when observable.
- Average spread captured per executed unit.

**Inventory risk**
- Mean absolute inventory.
- Maximum absolute inventory.
- Inventory time-weighted average.
- Inventory distribution.
- Time spent near risk limits.
- Inventory turnover.
- Inventory concentration by side.

**Market-making performance**
- Bid-side fill statistics.
- Ask-side fill statistics.
- Quoted spread.
- Realized spread.
- Effective spread, where calculable.
- Inventory-adjusted P&L.
- Adverse selection.
- P&L contribution from spread capture.
- P&L contribution from inventory mark-to-market changes.

**Risk metrics**
- Maximum drawdown.
- Drawdown duration.
- Peak-to-trough equity decline.
- Worst time interval.
- Worst trade.
- Volatility of equity returns.
- Exposure-related loss statistics.

**Data and operational metrics**
- Processed events per second.
- Simulation duration.
- Number of invalid events.
- Number of dropped or skipped events.
- Peak memory usage when measurable.

For each metric, document the exact formula and relevant limitations.

## 10.4 Drawdown

Define maximum drawdown using the equity curve:

\[
DD_t =
\frac{E_t-\max_{u\leq t}E_u}
{\max_{u\leq t}E_u}
\]

Maximum drawdown is the most negative value of this expression under the chosen convention.

Also report the absolute currency drawdown.

Handle zero or negative equity denominators carefully and document the behavior.

## 10.5 Adverse selection

Measure post-fill markouts.

For a fill at time \(t\), define:

\[
M_h=\epsilon(P_{t+h}-P_{\text{fill}})
\]

where \(\epsilon=+1\) for a buy and \(\epsilon=-1\) for a sell.

Under this convention, positive markout indicates favorable price movement for the position, while negative markout indicates adverse price movement.

Compute markouts over several configurable horizons, provided sufficient future observations exist.

Report sample counts and missing-horizon handling.

Separate immediate spread capture from subsequent price movement.

Do not treat spread capture and markout as interchangeable measures.

## 10.6 Empirical fill probabilities

Estimate fill probabilities by quote conditions.

Possible conditioning variables include:

- Quoted distance from mid-price.
- Spread regime.
- Volatility regime.
- Inventory bucket.
- Bid/ask side.
- Top-of-book imbalance.
- Time of day.
- Quote lifetime.

Report the denominator, number of opportunities, and number of observed fills for each bucket.

Do not claim a statistically meaningful relationship from tiny sample sizes.

Where practical, include confidence intervals or sample-size warnings.

Avoid survivorship bias and look-ahead bias when constructing observations.

---

# 11. Synthetic Market Simulation

Create a synthetic market-data generator for testing the full system without external datasets.

Use a seeded pseudorandom number generator.

The same seed and configuration must produce the same event sequence.

Provide multiple scenarios.

### Scenario A: Stable market

- Relatively stable reference price.
- Moderate spread.
- Reasonably balanced background liquidity.
- Low-to-moderate volatility.

### Scenario B: High volatility

- Larger reference-price fluctuations.
- More rapid market-state changes.
- Higher potential inventory risk.

### Scenario C: Liquidity imbalance

- Different bid and ask depths.
- Changing order-book imbalance.
- Asymmetric market pressure.

### Scenario D: Sparse liquidity

- Fewer resting orders.
- Wider spread.
- Greater price impact from market orders.

### Scenario E: Toxic flow

- Background trade direction correlated with subsequent reference-price movements.
- Greater potential for adverse selection.

### Scenario F: Inventory stress

- Price movement that causes the market maker to accumulate one-sided inventory.
- Opportunity to verify inventory controls.

These scenarios should be configurable, not hard-coded demonstrations that guarantee a profitable outcome.

Do not tune each scenario to manufacture favorable performance.

Record the seed, scenario parameters, and generated event count in every experiment.

Synthetic data is for controlled validation and sensitivity analysis. It must not be described as evidence of real-market profitability.

---

# 12. Strategy Experiments

Build a repeatable experiment runner.

It must support parameter sweeps over relevant market-making settings.

Example dimensions:

- Risk aversion \(\gamma\).
- Volatility window.
- Inventory limit.
- Base order quantity.
- Arrival-intensity parameter.
- Quote update threshold.
- Liquidity-skew coefficient.
- Latency assumption.
- Transaction-cost assumption.

Do not create an unnecessarily large Cartesian product that makes the project impractical to run.

Support a small default experiment suite and configurable extended sweeps.

For each experiment, save:

- Unique experiment ID.
- Configuration.
- Random seed.
- Input dataset identifier.
- Data range.
- Strategy version or configuration version.
- Execution-model selection.
- Starting capital.
- Ending equity.
- Net P&L.
- Maximum drawdown.
- Fill statistics.
- Adverse-selection statistics.
- Inventory statistics.
- Runtime.
- Error or completion status.

## 12.1 Required comparisons

Compare at least:

1. A simple symmetric fixed-spread market maker.
2. The baseline Avellaneda–Stoikov market maker.
3. The liquidity-aware Avellaneda–Stoikov extension.

Apply the same data, transaction costs, and evaluation conventions to comparable runs.

Include an inventory-disabled or unconstrained variant only as a clearly labeled diagnostic experiment.

Do not select a winning strategy based solely on gross P&L.

Evaluate risk, drawdown, turnover, adverse selection, fill rate, and stability across scenarios.

## 12.2 Sensitivity analysis

Analyze how results change with:

- Risk aversion.
- Volatility estimation quality.
- Inventory limits.
- Transaction costs.
- Market-data latency.
- Order-submission latency.
- Liquidity conditions.
- Quote update frequency.

Report cases where performance deteriorates.

A useful quantitative project demonstrates failure modes and limitations as well as favorable results.

## 12.3 Out-of-sample evaluation

Where historical data volume permits, split data chronologically into development and evaluation periods.

Fit or calibrate model parameters only using development-period information.

Freeze parameters before evaluation.

Do not randomly shuffle time-series events to create train/test splits.

Do not use future prices to determine quote prices, inventory decisions, or execution eligibility.

If the available dataset is too small for meaningful out-of-sample evaluation, document this limitation rather than presenting a misleading validation result.

---

# 13. Performance Engineering

The project must investigate low-latency matching rather than simply optimize for a headline number.

## 13.1 Google Benchmark suite

Create a separate benchmark executable.

Use Google Benchmark fixtures where appropriate.

Benchmark the following operations:

- Insertion of non-crossing limit orders.
- Cancellation of active orders.
- Full execution of a single resting order.
- Partial execution.
- Matching against multiple resting orders.
- Matching across multiple price levels.
- Best-bid and best-ask access.
- L2 snapshot generation.
- L3 snapshot generation.
- Mixed insertion/cancellation/trade workloads.
- High-order-count scenarios.
- High-price-level-count scenarios.
- Order-arena reuse.
- Complete engine event processing.

Use deterministic, pre-generated benchmark inputs when possible.

Do not include random-number generation, file I/O, logging, or dataset parsing in a benchmark intended to isolate matching-engine performance.

Prevent dead-code elimination by consuming benchmark outputs appropriately.

Ensure each benchmark has a clear workload definition.

## 13.2 Latency definitions

Distinguish:

- **Matching operation latency:** Time spent processing a defined matching operation.
- **Engine event latency:** Time from entering an engine event handler to completing the event.
- **End-to-end simulation latency:** Time including strategy and simulation components.
- **Throughput:** Number of processed operations or events per second.
- **Historical replay throughput:** Number of source events processed per second.

These are different measurements.

Do not present one as a substitute for another.

## 13.3 Target performance

Use sub-650-nanosecond internal matching as an aspirational measurement target for a clearly defined, favorable microbenchmark.

Do not guarantee this target in source code or documentation.

Whether it is achieved depends on hardware, compiler, build flags, workload, and benchmark methodology.

The final README must state:

- CPU model.
- Operating system.
- Compiler and version.
- Build type.
- Optimization flags.
- Benchmark workload.
- Number of benchmark repetitions.
- Relevant warm-up settings.
- Reported latency statistic.
- Sample count or iteration count.
- Whether instrumentation was enabled.

Use release builds for the primary performance measurements.

Do not use debug-build measurements to substantiate low-latency claims.

## 13.4 Latency distributions

Google Benchmark is useful for throughput and repeated timing, but do not assume that its ordinary aggregate output establishes a complete per-operation latency distribution.

If reporting p50, p90, p99, or p99.9 latency, implement a separate, documented measurement harness or use a suitable supported timing mechanism.

Account for timing overhead and avoid treating a heavily instrumented measurement as equivalent to production execution.

Clearly identify whether the reported percentile is a per-operation latency, batch latency, or another statistic.

## 13.5 Baseline comparisons

Provide a simple reference implementation and compare its behavior and performance against the optimized implementation.

The reference and optimized implementations must produce equivalent results on identical workloads.

Report:

- Operations per second.
- Average or aggregate timing as appropriate.
- Relevant latency percentiles when measured.
- Memory usage where measurable.
- Workload characteristics.
- Relative performance improvement.

Do not invent baseline numbers or claim a speedup without actual measurements.

---

# 14. C++17 Engineering Standards

Write production-quality C++17 code.

## 14.1 Code organization

Use a clean separation between public interfaces and implementation files.

A reasonable structure is:

```text
limit-order-book-market-maker/
├── CMakeLists.txt
├── CMakePresets.json
├── README.md
├── LICENSE
├── .gitignore
├── .clang-format
├── .clang-tidy
├── configs/
│   ├── default.yaml
│   ├── synthetic_stable.yaml
│   ├── synthetic_toxic_flow.yaml
│   └── strategy_baseline.yaml
├── data/
│   ├── README.md
│   └── sample/
├── docs/
│   ├── architecture.md
│   ├── matching-engine.md
│   ├── market-making-model.md
│   ├── backtesting-methodology.md
│   ├── benchmark-methodology.md
│   └── data-formats.md
├── include/
│   └── lob/
│       ├── core/
│       ├── book/
│       ├── matching/
│       ├── market_data/
│       ├── strategy/
│       ├── simulation/
│       ├── analytics/
│       └── utilities/
├── src/
│   ├── core/
│   ├── book/
│   ├── matching/
│   ├── market_data/
│   ├── strategy/
│   ├── simulation/
│   ├── analytics/
│   ├── app/
│   └── main.cpp
├── tests/
│   ├── unit/
│   ├── integration/
│   ├── property/
│   ├── fixtures/
│   └── CMakeLists.txt
├── benchmarks/
│   ├── CMakeLists.txt
│   ├── matching_benchmarks.cpp
│   ├── order_book_benchmarks.cpp
│   └── replay_benchmarks.cpp
├── scripts/
│   ├── run_tests.sh
│   ├── run_benchmarks.sh
│   └── run_experiments.py
├── results/
│   ├── .gitkeep
│   └── README.md
└── examples/
    ├── basic_matching.cpp
    ├── synthetic_market.cpp
    └── market_maker.cpp
```

Treat this as a proposed structure, not an inflexible requirement. Adjust it if a more coherent arrangement is justified.

Do not create empty directories, empty source files, or unnecessary layers merely to fill out the tree.

Keep implementation complexity proportionate to the project's objectives.

## 14.2 Coding conventions

Use:

- RAII.
- Explicit ownership semantics.
- `const` correctness.
- Strongly typed enums.
- `enum class`.
- `std::optional` where appropriate.
- `std::string_view` for non-owning string parameters when lifetimes are safe.
- `std::unique_ptr` only where dynamic ownership is appropriate.
- STL algorithms and containers when suitable.
- Clear namespaces.
- Small, focused functions.
- Meaningful names.
- Explicit error-handling policies.

Avoid:

- Unnecessary global mutable state.
- Unchecked raw-pointer ownership.
- Silent exception swallowing.
- Unnecessary dynamic allocation in the matching hot path.
- Excessive template metaprogramming.
- Premature lock-free programming.
- Premature multithreading.
- Hidden dependencies between modules.
- Giant classes that handle the entire application.
- Repeated business logic across modules.

Use exceptions, error-return types, or status objects according to a consistent project-wide policy.

## 14.3 Build system

Use modern CMake and declare the C++ standard explicitly.

Support separate targets for:

- Core library.
- CLI application.
- Unit and integration tests.
- Benchmark executable.
- Optional examples.

Use `find_package` or a documented dependency-management strategy for Google Benchmark and the testing framework.

Do not require an Internet connection during normal execution of the built application.

Document dependency installation and first-time configuration.

Support Debug and Release builds.

Enable useful compiler warnings. Treat warnings as errors in CI only after confirming the supported toolchain builds cleanly.

Add optional AddressSanitizer and UndefinedBehaviorSanitizer configurations where supported.

## 14.4 Logging and errors

Keep diagnostic output separate from machine-readable output.

Do not print informational messages into CSV or JSON result streams.

Include enough context in error messages to identify invalid inputs, configuration errors, and failed operations.

Avoid logging every matching operation by default, as this can distort performance.

Provide a diagnostic mode for detailed event tracing.

---

# 15. Command-Line Interface and User Experience

A user must be able to build, test, simulate, replay, and benchmark the project using clearly documented commands.

Provide help text for every command and option.

The interface should support operations equivalent to:

```bash
# Configure the project
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

# Build the application
cmake --build build --parallel

# Run the complete test suite
ctest --test-dir build --output-on-failure

# Run a deterministic synthetic simulation
./build/lob_sim run-simulation \
  --config configs/synthetic_stable.yaml \
  --seed 42 \
  --output results/synthetic_stable

# Run the baseline market-making strategy
./build/lob_sim run-market-maker \
  --config configs/strategy_baseline.yaml \
  --output results/market_maker_baseline

# Validate a historical dataset
./build/lob_sim validate-data \
  --input data/sample/market_events.csv

# Replay historical market data
./build/lob_sim replay-data \
  --input data/sample/market_events.csv \
  --output results/historical_replay

# Run matching-engine benchmarks
./build/lob_benchmarks

# Display application help
./build/lob_sim --help
```

These commands define the intended interface. Implement the CLI consistently with the final executable names and document any changes.

The example commands must actually work once the project is built.

Do not provide placeholder commands in the README that refer to nonexistent executables, configuration files, or options.

---

# 16. Outputs and Artifacts

Every completed simulation or replay must produce a structured output directory.

A typical result directory should contain the following artifacts when relevant.

## 16.1 Run metadata

`run_metadata.json`

Include:

- Run ID.
- UTC or explicitly documented timestamp convention.
- Project version or Git commit.
- Configuration.
- Random seed.
- Input dataset identifier.
- Replay mode.
- Strategy name.
- Execution-model name.
- Start and end simulation time.
- Event count.
- Run status.
- Relevant software and build information.

## 16.2 Trade ledger

`trades.csv`

Include:

- Trade ID.
- Simulation timestamp.
- Source event sequence.
- Buyer and seller order identifiers, when available.
- Execution price.
- Execution quantity.
- Aggressor side.
- Relevant strategy order identifier.
- Fee or rebate information when available.

## 16.3 Execution report

`execution_reports.csv`

Record accepted orders, rejected orders, partial fills, completed fills, cancellations, and other supported lifecycle events.

## 16.4 Market-data log

`market_events.csv`

Include normalized market events or an appropriately scoped event log. For large datasets, support streaming output or a configurable logging policy.

## 16.5 Equity curve

`equity_curve.csv`

Include:

- Timestamp.
- Cash.
- Inventory.
- Reference price.
- Mark-to-market equity.
- Realized P&L.
- Unrealized P&L.
- Cumulative fees.
- Inventory valuation convention.

## 16.6 Strategy decision log

`strategy_decisions.csv`

Include:

- Timestamp.
- Reference price.
- Estimated volatility.
- Inventory.
- Reservation price.
- Half-spread.
- Target bid.
- Target ask.
- Actual bid.
- Actual ask.
- Liquidity indicators.
- Risk constraints.
- Quote action.
- Decision reason.

## 16.7 Performance summary

`summary.json`

Include all core performance metrics, sample counts, execution-model assumptions, data-quality warnings, and a machine-readable completion status.

## 16.8 Human-readable report

`report.md`

Generate a concise report containing:

- Run configuration.
- Data description.
- Strategy parameters.
- Execution assumptions.
- Main performance metrics.
- Risk metrics.
- Important warnings.
- Comparison with baseline strategies.
- Interpretation limitations.

## 16.9 Benchmark output

Save benchmark results in a machine-readable format supported by the installed Google Benchmark version.

Include a README section explaining how to interpret those results.

Do not fabricate missing output files. If an artifact is optional, explain the condition under which it is generated.

---

# 17. Testing and Continuous Integration

Build an automated verification pipeline.

## 17.1 Test categories

Separate:

- Unit tests.
- Matching-engine integration tests.
- Strategy formula tests.
- Accounting tests.
- Historical parser tests.
- Replay determinism tests.
- Property-based or randomized invariant tests.
- End-to-end CLI tests.
- Benchmark smoke tests.

## 17.2 Mathematical tests

Test the reservation-price formula using hand-computed examples.

Verify that:

- Zero inventory makes the reservation price equal to the reference price under the baseline formula.
- Positive inventory shifts the reservation price downward.
- Negative inventory shifts it upward.
- Increasing risk aversion increases the magnitude of inventory skew under otherwise identical conditions.
- Increasing volatility increases inventory sensitivity.
- Zero remaining horizon removes the time-dependent inventory adjustment under the baseline equation.

Test the half-spread formula over valid parameter ranges.

Check small-risk-aversion numerical stability.

Test tick rounding independently from the continuous model.

## 17.3 Accounting tests

Verify:

- Buy executions decrease cash and increase inventory.
- Sell executions increase cash and decrease inventory.
- Fees reduce net equity.
- Rebates affect equity according to the configured convention.
- Realized P&L is consistent with the chosen accounting basis.
- Unrealized P&L is consistent with the reference price.
- Equity calculations remain internally consistent.
- Drawdown is calculated correctly for known equity curves.
- Inventory limits cannot be bypassed through partial fills or outstanding quotes.

## 17.4 Determinism tests

Run the same simulation twice with the same seed and configuration.

Compare:

- Trade ledgers.
- Execution reports.
- Final book state.
- Equity curves.
- Strategy decisions.
- Summary metrics.

For deterministic outputs, results should match under the documented comparison policy.

## 17.5 CI

Provide a GitHub Actions workflow or equivalent CI configuration.

It should:

- Configure CMake.
- Build the project.
- Run tests.
- Run formatting or static-analysis checks where configured.
- Build benchmark targets.
- Run a small benchmark smoke test if practical.
- Execute a small deterministic simulation.
- Verify expected output artifacts.

Do not make CI depend on downloading a large historical dataset.

Use synthetic fixtures for normal automated testing.

---

# 18. Repository Quality and Dependency Management

Keep dependencies minimal and justified.

Expected dependencies:

- C++17-compatible compiler.
- CMake.
- Google Benchmark.
- GoogleTest or another suitable test framework.
- Optional lightweight configuration parser.
- Optional lightweight JSON or CSV support where justified.

If third-party libraries are used:

- Pin or constrain supported versions.
- Document licensing.
- Explain installation.
- Avoid unnecessary dependencies.
- Avoid downloading or executing arbitrary scripts during the build.
- Prefer a reproducible dependency-management strategy.

Include a suitable `.gitignore`.

Do not commit large raw market datasets without checking their licensing and size.

If historical data cannot legally be redistributed, provide instructions for obtaining it and a small synthetic fixture that exercises the parser.

Never include credentials, API keys, proprietary datasets, or private financial information.

---

# 19. Complete README Requirements

The README is a first-class project deliverable.

Write a polished, detailed, technically accurate README.md that acts as the main entry point for a recruiter, quantitative developer, or engineer reviewing the repository.

It must explain both **what the system does and how it actually works**.

Do not write a generic README consisting of a short description, a few installation commands, and a screenshot.

Do not leave placeholders such as "TODO", "add benchmark results", or "describe the model later."

If a real result is unavailable, label it as not yet measured and explain how to reproduce it.

## 19.1 Project title and summary

Include:

- Project name.
- Concise description.
- Major technologies.
- Main features.
- Project scope.
- A clear explanation that this is a simulator and research project, not a production exchange or live trading service.

## 19.2 Architecture overview

Include a Mermaid architecture diagram showing the relationships among:

- Input market data.
- Market-data normalization.
- Event replay.
- Order book.
- Matching engine.
- Execution reports.
- Market-making strategy.
- Portfolio and risk accounting.
- Analytics.
- Benchmarking.
- Output artifacts.

The diagram must correspond to the implemented architecture.

Add an event-flow explanation covering the lifecycle of a market event and a strategy-generated order.

## 19.3 Directory structure

Document the important directories and files.

Explain where the matching engine, order-book structures, strategy formulas, market-data adapters, tests, and benchmarks live.

Do not dump an enormous file tree without explaining its organization.

## 19.4 Matching-engine explanation

Explain:

- L2 versus L3 market data.
- Limit versus market orders.
- Price-time priority.
- Partial fills.
- Cancellation.
- Replacement and modification.
- The order arena.
- Free-list management.
- Doubly linked order queues.
- Price-level indexing.
- The order-ID index.
- L2 aggregation.
- L3 snapshots.
- Execution reports.
- Book invariants.

Include at least one worked example.

For example, show how an incoming buy order consumes the oldest eligible resting sell orders across one or more price levels.

Demonstrate how the resting-order prices determine execution prices.

## 19.5 Complexity analysis

Document expected complexity for the actual implementation.

Discuss:

- Price-level insertion and deletion.
- Best-price lookup.
- Order insertion.
- Cancellation by order ID.
- Matching one or more resting orders.
- L2 depth queries.
- L3 snapshot generation.
- Full-book traversal.

State which costs are constant-time, logarithmic, or proportional to the number of visited orders or levels.

Do not claim O(1) insertion or cancellation for an entire engine if the price-level indexing operation is logarithmic.

## 19.6 Quantitative model explanation

Explain the Avellaneda–Stoikov model from first principles.

Define every symbol.

Show the reservation-price and optimal half-spread equations.

Explain:

- Inventory skew.
- Risk aversion.
- Volatility.
- Time horizon.
- Order-arrival intensity.
- Bid and ask quote construction.
- Tick-size conversion.
- Inventory constraints.
- Volatility estimation.
- Liquidity-aware extensions.

Include a numerical worked example using explicitly labeled illustrative inputs.

Show the sequence from reference price to reservation price, half-spread, and rounded bid/ask quotes.

The example must be mathematically consistent with the implemented parameterization.

## 19.7 Historical replay and data formats

Explain:

- Supported CSV schemas.
- Required columns.
- Timestamp conventions.
- Price and quantity normalization.
- Sequence ordering.
- Malformed-record handling.
- Replay modes.
- The distinction between L2 snapshots and L3 order events.
- Fill-model assumptions.
- Queue-position limitations.
- Latency assumptions.

Provide sample input rows that match the actual parser.

Explain how to validate a dataset before replay.

## 19.8 Backtesting methodology

Explain the cash and inventory ledger, mark-to-market equity, fees, realized and unrealized P&L, spread capture, adverse selection, fill rates, inventory exposure, and drawdown.

For every major performance metric, state the formula or operational definition.

Clearly distinguish:

- Synthetic simulation.
- Historical reference replay.
- Historical order-book reconstruction.
- Direct exchange matching inside the simulator.
- Estimated historical fills.

Explain how look-ahead bias and optimistic execution assumptions can distort results.

## 19.9 Installation and build

Provide exact prerequisites and installation instructions.

Cover:

- Compiler requirements.
- CMake installation.
- Google Benchmark.
- Test framework.
- Optional dependencies.
- Debug build.
- Release build.
- Sanitizer build.
- Clean rebuild.

Provide commands for Linux and macOS where practical. If Windows is supported, provide the actual tested Windows build procedure rather than generic instructions.

## 19.10 Running the application

Document every implemented CLI command with:

- Purpose.
- Required arguments.
- Optional arguments.
- Example.
- Expected outputs.
- Common errors.

Include a complete quick-start workflow from cloning the repository to running a deterministic market-making simulation and examining its output.

## 19.11 Testing instructions

Explain how to run unit tests, integration tests, randomized invariant tests, sanitizer builds, and end-to-end simulations.

Describe what each test category verifies.

Report actual test results only after running the tests.

## 19.12 Benchmark methodology and results

Explain:

- Why matching-engine microbenchmarks are useful.
- Why latency is hardware-dependent.
- How Google Benchmark is used.
- How workloads are constructed.
- Why logging and I/O should be excluded from isolated matching benchmarks.
- How throughput differs from latency.
- How percentile measurements are collected.
- How to reproduce benchmark runs.
- How to compare the reference and optimized implementations.

Provide a results table with fields such as:

| Operation | Workload | Throughput | Latency statistic | Environment |
|---|---|---|---|---|
| Limit insertion | Actual benchmark workload | Measured result | Measured result | Actual machine |
| Cancellation | Actual benchmark workload | Measured result | Measured result | Actual machine |
| Matching | Actual benchmark workload | Measured result | Measured result | Actual machine |
| Mixed events | Actual benchmark workload | Measured result | Measured result | Actual machine |

Populate the table only with measurements that were actually collected.

If no benchmark has been run, label the results as not measured and include the commands needed to generate them.

Never claim sub-650 ns matching latency without reproducible evidence supporting the exact latency definition and workload.

## 19.13 Experimental results

Document the actual results of the baseline fixed-spread strategy, the classical Avellaneda–Stoikov strategy, and the liquidity-aware extension.

Use identical evaluation conventions when comparing them.

Include tables for:

- Net P&L.
- Maximum drawdown.
- Fill rate.
- Spread capture.
- Adverse selection.
- Inventory exposure.
- Fees.
- Runtime.

Clearly label synthetic and historical results.

Explain important negative results, unstable parameter configurations, and failure cases.

Do not present illustrative numbers as experimental findings.

## 19.14 Known limitations

Include a candid limitations section covering, where applicable:

- Lack of real exchange connectivity.
- Simplified exchange semantics.
- Lack of hidden-liquidity modeling.
- Queue-position uncertainty.
- Incomplete historical order-level information.
- Simplified transaction costs.
- Volatility-estimation limitations.
- Sensitivity to execution assumptions.
- Model risk.
- Hardware-dependent benchmarks.
- The gap between simulated and live profitability.

## 19.15 Future extensions

Suggest technically justified extensions such as:

- More sophisticated queue-position models.
- Additional market-data feed adapters.
- More detailed exchange-specific order semantics.
- Alternative volatility estimators.
- Calibrated fill-probability models.
- Improved latency distributions.
- More robust out-of-sample testing.
- Cross-instrument inventory risk.
- Additional market microstructure statistics.

Do not imply that unimplemented extensions are already available.

## 19.16 Resume-ready project summary

Include a final section with example resume bullets.

The bullets must be templates until supported by actual implementation and measurement.

For example:

- Built a C++17 limit order book with price-time-priority matching, partial fills, cancellation, and contiguous order storage; evaluated matching latency and event throughput using Google Benchmark.
- Implemented the Avellaneda–Stoikov market-making model with inventory-aware reservation prices, volatility-dependent spreads, and configurable risk limits.
- Developed a deterministic event-replay and backtesting framework to measure fill rates, spread capture, adverse selection, inventory exposure, and maximum drawdown.

Add numerical claims only when the project produces evidence supporting them.

---

# 20. Documentation Beyond the README

Create focused technical documents where the material would make the main README unnecessarily unwieldy.

### `docs/architecture.md`

Explain module boundaries, data ownership, event flow, interfaces, and key architectural decisions.

### `docs/matching-engine.md`

Explain the data structures, order lifecycle, price-time priority, invariants, and complexity.

### `docs/market-making-model.md`

Explain the equations, parameter units, volatility estimator, inventory controls, liquidity extension, and model assumptions.

### `docs/backtesting-methodology.md`

Explain historical data limitations, execution simulation, accounting, transaction costs, markouts, drawdown, and out-of-sample evaluation.

### `docs/benchmark-methodology.md`

Explain benchmark workloads, machine configuration, timing methodology, reproducibility, and interpretation.

### `docs/data-formats.md`

Document the exact schemas, field types, timestamp formats, validation rules, and supported replay modes.

Avoid duplicating large sections of text verbatim across files. The README should summarize and link to the detailed documents.

---

# 21. Security, Reliability, and Operational Safety

Even though this is a local research simulator, apply sensible reliability practices.

- Validate external data before use.
- Avoid unsafe memory ownership.
- Prevent integer overflow in prices and quantities.
- Bound configurable allocations where practical.
- Handle malformed files without undefined behavior.
- Avoid silently overwriting prior experiment results.
- Use unique run directories or explicit overwrite options.
- Keep deterministic seeds in run metadata.
- Make output file failures visible.
- Prevent partially written output from being mistaken for a successful run.
- Report incomplete simulations clearly.
- Keep market-data parsing separate from matching-engine mutation.
- Use sanitizers to detect memory and undefined-behavior issues.
- Avoid network access during normal simulation.
- Never submit real exchange orders.

The program should be a local simulator unless an explicitly separate, fully specified extension is later requested.

---

# 22. Implementation Workflow

Do not attempt to produce the entire repository as one enormous, unverified code dump.

Work in defined implementation phases. Each phase must leave the project in a buildable, understandable state.

## Phase 1: Requirements and architecture

Produce:

- A concise architecture decision record.
- Module boundaries.
- Core data types.
- Order-book representation.
- Matching semantics.
- Quantitative model conventions.
- Data schemas.
- CLI design.
- Build and dependency plan.
- Test strategy.
- Benchmark methodology.

Resolve important ambiguities before implementation.

Document decisions that materially affect correctness or interpretation.

## Phase 2: Core matching engine

Implement:

- Core types.
- Order arena.
- Price-level structures.
- Order-ID indexing.
- L2 and L3 queries.
- Matching logic.
- Execution reports.
- Cancellation and replacement.
- Invariant checks.

Write the reference implementation and initial tests.

Compile and test before moving forward.

## Phase 3: Matching-engine hardening

Implement:

- Edge-case handling.
- Partial-fill correctness.
- Order modification semantics.
- Time-in-force behavior.
- Randomized invariant tests.
- Differential tests.
- Stress tests.
- Sanitizer checks.

Do not proceed on the assumption that compilation alone proves correctness.

## Phase 4: Market-data pipeline

Implement:

- CSV schemas.
- Parsing and validation.
- Price and quantity normalization.
- Timestamp handling.
- Event sequencing.
- Data-quality reports.
- Synthetic event generation.
- Replay interfaces.

Verify the parser with valid and malformed fixtures.

## Phase 5: Market-making model

Implement:

- Rolling volatility estimator.
- Reservation-price calculation.
- Half-spread calculation.
- Tick rounding.
- Quote construction.
- Inventory constraints.
- Liquidity-aware extension.
- Quote lifecycle management.
- Strategy-decision logging.

Write unit tests for the formulas and risk controls.

## Phase 6: Simulation and accounting

Implement:

- Deterministic event loop.
- Strategy-to-engine integration.
- Cash and inventory ledger.
- Fee and rebate accounting.
- Equity curve.
- Fill and execution statistics.
- Adverse-selection markouts.
- Drawdown.
- Output generation.

Validate the accounting against small, hand-computed examples.

## Phase 7: Historical replay

Implement the supported replay modes.

Ensure the execution model matches the data's actual capabilities.

Test ordering, malformed input handling, reproducibility, and leakage prevention.

## Phase 8: Experiment runner

Implement:

- Baseline strategy.
- Avellaneda–Stoikov strategy.
- Liquidity-aware extension.
- Parameter sweeps.
- Scenario comparisons.
- Sensitivity analysis.
- Chronological out-of-sample evaluation where supported.

Generate machine-readable summaries and human-readable reports.

## Phase 9: Performance benchmarks

Implement Google Benchmark targets.

Measure isolated matching operations and end-to-end event processing separately.

Record actual hardware, compiler, build settings, workload, and results.

Investigate bottlenecks only after establishing correct baseline measurements.

Apply optimizations one at a time and rerun correctness tests after each change.

## Phase 10: Production-quality completion

Finish:

- CLI validation.
- CMake cleanup.
- CI.
- Formatting.
- Static analysis.
- Sanitizer configuration.
- README.
- Technical documentation.
- Example configurations.
- Reproducible sample workflows.
- Final repository review.

The project is complete only when the documented build and run commands have been verified to the extent possible in the available environment.

---

# 23. Mandatory Acceptance Criteria

Use this checklist as the definition of done.

## Matching engine

- [ ] C++17 implementation.
- [ ] Price-time-priority matching.
- [ ] Limit orders.
- [ ] Market orders.
- [ ] Partial fills.
- [ ] Multiple price levels.
- [ ] Order cancellation.
- [ ] Defined modification and replacement semantics.
- [ ] L2 and L3 queries.
- [ ] Contiguous order storage.
- [ ] Efficient removal of known orders.
- [ ] Deterministic trade generation.
- [ ] Book invariants.
- [ ] Reference implementation.
- [ ] Differential and stress tests.

## Quantitative strategy

- [ ] Avellaneda–Stoikov reservation price.
- [ ] Correctly parameterized optimal half-spread.
- [ ] Volatility estimation.
- [ ] Inventory-aware quote skew.
- [ ] Tick-size rounding.
- [ ] Inventory constraints.
- [ ] Outstanding-order risk controls.
- [ ] Liquidity-aware extension.
- [ ] Strategy-decision logging.
- [ ] Unit tests for formulas.

## Simulation and research

- [ ] Deterministic event-driven simulation.
- [ ] Seeded synthetic market generator.
- [ ] Historical CSV parser.
- [ ] Replay validation.
- [ ] Explicit fill-model assumptions.
- [ ] Cash and inventory accounting.
- [ ] Fees and rebates.
- [ ] Mark-to-market equity.
- [ ] Fill statistics.
- [ ] Spread capture.
- [ ] Adverse-selection markouts.
- [ ] Inventory analysis.
- [ ] Maximum drawdown.
- [ ] Parameter sweeps.
- [ ] Baseline strategy comparison.
- [ ] Out-of-sample methodology where data permits.

## Performance

- [ ] Google Benchmark integration.
- [ ] Order insertion benchmarks.
- [ ] Cancellation benchmarks.
- [ ] Matching benchmarks.
- [ ] Mixed workload benchmarks.
- [ ] Throughput measurements.
- [ ] Clearly defined latency measurements.
- [ ] Reproducible benchmark commands.
- [ ] No fabricated performance numbers.
- [ ] Documented hardware and compiler configuration.

## Engineering

- [ ] CMake build.
- [ ] Clean module boundaries.
- [ ] Unit tests.
- [ ] Integration tests.
- [ ] Randomized invariant tests.
- [ ] Sanitizer support.
- [ ] CI workflow.
- [ ] Example configurations.
- [ ] Useful error messages.
- [ ] Deterministic output metadata.

## Documentation

- [ ] Complete README.
- [ ] Architecture diagram.
- [ ] Directory structure explanation.
- [ ] Matching-engine walkthrough.
- [ ] Complexity analysis.
- [ ] Mathematical derivation and definitions.
- [ ] Historical data documentation.
- [ ] Backtesting methodology.
- [ ] Benchmark methodology.
- [ ] Installation instructions.
- [ ] Verified CLI examples.
- [ ] Actual experimental results, or explicitly marked unmeasured results.
- [ ] Known limitations.
- [ ] Resume-ready bullet templates.

Do not mark an item complete just because the corresponding class or function exists. Verify its behavior.

---

# 24. Rules for the Coding Agent

Follow these rules throughout the implementation.

1. **Do not fabricate success.** Never claim code compiled, tests passed, benchmarks ran, or profitability was demonstrated unless supported by actual results.

2. **Do not substitute pseudocode for implementation.** Provide real, compilable source code for every required module.

3. **Do not create hollow deliverables.** A README, benchmark target, test suite, or CLI command must be functional and relevant.

4. **Do not silently weaken requirements.** If a requirement cannot be met, explain why and provide the closest technically defensible alternative.

5. **Do not confuse matching-engine latency with live trading latency.**

6. **Do not confuse a simulated fill with an observed real-exchange fill.**

7. **Do not claim L3 reconstruction from L2-only data.**

8. **Do not claim that a strategy is profitable merely because one synthetic run has positive P&L.**

9. **Do not introduce look-ahead bias.** Strategy decisions may only use information available at their decision time.

10. **Do not optimize before validating correctness.** Establish a working reference implementation and compare outputs after optimization.

11. **Do not use floating-point prices in authoritative order-book matching.** Use integer ticks.

12. **Do not make the matching engine depend on the market-making strategy.** The strategy must remain replaceable.

13. **Do not leave unsupported claims in the README or source comments.**

14. **Do not hide limitations.** Document assumptions about market data, latency, fills, volatility, and transaction costs.

15. **Do not stop after creating a project skeleton.** Complete the implementation, tests, integration, documentation, and reproducibility work.

16. **Do not ask the user to approve every ordinary engineering decision.** Make reasonable choices, document them, and continue. Ask for clarification only when an essential requirement cannot be resolved responsibly.

17. **Do not sacrifice correctness for artificial benchmark numbers.** Optimize the actual bottlenecks measured on the actual implementation.

18. **Do not hard-code benchmark outputs or precomputed strategy results.**

19. **Do not silently skip failing tests.** Report failures and resolve them before declaring completion.

20. **Do not make the repository dependent on private data or unavailable infrastructure for its basic demonstration.** A synthetic, deterministic quick-start path must always be available.

---

# 25. Final Deliverables

At the end of the task, provide the complete repository with:

1. A functioning C++17 limit order book.
2. A deterministic price-time-priority matching engine.
3. A contiguous order-storage implementation.
4. A reference matching implementation.
5. Comprehensive tests.
6. An Avellaneda–Stoikov market-making strategy.
7. A separately configurable liquidity-aware extension.
8. A synthetic market generator.
9. Historical-data parsing and replay.
10. An explicit execution-simulation framework.
11. Inventory, cash, and P&L accounting.
12. Fill-probability and adverse-selection analysis.
13. Maximum-drawdown and inventory-risk analysis.
14. Parameter sweeps and strategy comparisons.
15. Google Benchmark targets.
16. Reproducible experiment outputs.
17. A clean command-line application.
18. CMake build configuration.
19. CI and sanitizer support.
20. Example configuration files.
21. A complete, polished README.
22. Supporting technical documentation.
23. Exact build, test, simulation, and benchmark commands.
24. A final verification report listing what was implemented, what was tested, the actual measured results, and any outstanding limitations.

The final response must summarize the architecture, identify the important implementation decisions, list the delivered files, provide exact reproduction commands, and report verified test and benchmark outcomes.

If repository-writing tools are available, create and modify the actual files in the workspace rather than merely printing their contents in the conversation.

If execution is unavailable, clearly separate implemented code from unverified code and provide a precise list of commands the user must run.

---

# 26. Final Quality Standard

The finished project must be strong enough that, during an interview, I can explain:

- Why the order book uses its chosen data structures.
- How an incoming order finds the best executable price.
- How FIFO priority is maintained.
- How cancellation avoids scanning an entire price-level queue.
- Why the matching engine uses integer prices.
- How order storage reuse works.
- How the classical Avellaneda–Stoikov model derives reservation prices and spreads.
- How volatility and inventory affect quotes.
- How the liquidity-aware extension differs from the classical model.
- How historical replay works.
- Why historical data does not automatically reveal whether a hypothetical order would have filled.
- How look-ahead bias can occur.
- How adverse selection is measured.
- How spread capture differs from markout and total P&L.
- How transaction costs affect profitability.
- How inventory risk and drawdown are calculated.
- How benchmark measurements are collected and interpreted.
- What the simulator cannot faithfully reproduce about a live exchange.

The implementation should be sufficiently modular that each of these components can be inspected, tested, and discussed independently.

**The priority order is: correctness, transparent assumptions, reproducibility, measurement, and then optimization.**

Build a coherent quantitative trading systems project, not a collection of impressive-sounding features. Every resume claim must map to working code, a documented methodology, and evidence that can be independently reproduced.

Begin with the architecture and implementation plan, then execute the phases in order until the deliverables and acceptance criteria are satisfied.