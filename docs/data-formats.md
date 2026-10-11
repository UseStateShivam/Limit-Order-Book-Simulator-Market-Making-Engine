# Market Data Schemas & Replay Specifications

## 1. Supported Data Schemas

### 1.1 Market Events / Top-of-Book (`market_events.csv`)
Represents continuous BBO quote changes and observed public trades:

| Column | C++ Type | Description | Unit / Format |
|---|---|---|---|
| `timestamp_ns` | `uint64_t` | Monotonically increasing epoch timestamp | Nanoseconds |
| `sequence` | `uint64_t` | Monotonically increasing feed sequence number | Integer counter |
| `event_type` | `enum` | Event category: `BBO`, `TRADE`, `SNAPSHOT` | String token |
| `symbol` | `std::string` | Instrument identifier | e.g. `BTC-USDT` |
| `bid_price` | `double` | Prevailing best bid price | Decimal currency units |
| `bid_qty` | `double` | Prevailing best bid depth | Quantity units / lots |
| `ask_price` | `double` | Prevailing best ask price | Decimal currency units |
| `ask_qty` | `double` | Prevailing best ask depth | Quantity units / lots |
| `trade_price` | `double` | Last public executed trade price | Decimal currency (0 if quote) |
| `trade_qty` | `double` | Last public executed trade size | Quantity units (0 if quote) |

### 1.2 Order Events / L3 Feed (`order_events.csv`)
Individual order event stream:

| Column | C++ Type | Description | Unit / Format |
|---|---|---|---|
| `timestamp_ns` | `uint64_t` | Monotonically increasing epoch timestamp | Nanoseconds |
| `sequence` | `uint64_t` | Monotonically increasing sequence number | Integer counter |
| `event_type` | `enum` | Lifecycle action: `ADD`, `CANCEL`, `MODIFY`, `EXECUTE` | String token |
| `order_id` | `uint64_t` | Unique order identifier | Positive integer |
| `side` | `enum` | `BUY` or `SELL` | String token |
| `price` | `double` | Limit price | Decimal currency units |
| `qty` | `double` | Order size | Quantity units / lots |
| `symbol` | `std::string` | Instrument identifier | e.g. `BTC-USDT` |

## 2. Validation & Quality Rules

The `CsvParser` automatically analyzes and reports:
1. **Timestamp Monotonicity**: Checks for out-of-order timestamps ($t_{i} < t_{i-1}$).
2. **Sequence Gaps**: Checks for gaps in the feed sequence ($s_i > s_{i-1} + 1$).
3. **Duplicate Sequence Numbers**: Detects dropped or duplicate events ($s_i == s_{i-1}$).
4. **Column Integrity & Range Constraints**: Rejects malformed rows with missing columns, negative quantities, or zero prices.
