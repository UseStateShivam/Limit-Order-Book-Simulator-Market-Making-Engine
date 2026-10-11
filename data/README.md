# Market Data Fixtures and Datasets

This directory contains reference and sample market data for replay and validation.

## Supported Schemas

### 1. Market Events / Top-of-Book (`market_events.csv`)
Columns:
- `timestamp_ns`: Unix epoch timestamp in nanoseconds
- `sequence`: Monotonically increasing sequence number
- `event_type`: Event type code (`SNAPSHOT`, `TRADE`, `BBO`, `DEPTH`)
- `symbol`: Instrument identifier (e.g. `BTC-USDT`, `ETH-USDT`)
- `bid_price`: Best bid price in decimal currency
- `bid_qty`: Best bid aggregate quantity
- `ask_price`: Best ask price in decimal currency
- `ask_qty`: Best ask aggregate quantity
- `trade_price`: Last trade price (or 0 if quote update)
- `trade_qty`: Last trade quantity (or 0 if quote update)

### 2. Order Events / L3 Feed (`order_events.csv`)
Columns:
- `timestamp_ns`: Unix epoch timestamp in nanoseconds
- `sequence`: Monotonically increasing sequence number
- `event_type`: Event type (`ADD`, `CANCEL`, `MODIFY`, `EXECUTE`)
- `order_id`: Unique order identifier
- `side`: `BUY` or `SELL`
- `price`: Limit price in decimal currency
- `qty`: Order quantity
- `symbol`: Instrument identifier
