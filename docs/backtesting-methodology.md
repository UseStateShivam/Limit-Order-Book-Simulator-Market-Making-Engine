# Backtesting, Execution Simulation & Accounting Methodology

## 1. Accounting Conventions

### 1.1 Cash and Inventory Tracking
The portfolio tracks:
- `cash`: Liquid currency balance.
- `inventory`: Signed position ($q_t > 0$ long, $q_t < 0$ short).
- `average_acquisition_cost`: Moving average cost basis for open position.

For every execution of quantity $v$ at price $p$:
- A **Buy** decreases cash by $pv$ and increases inventory by $v$.
- A **Sell** increases cash by $pv$ and decreases inventory by $v$.
- Position reductions trigger **Realized P&L**:
  - Closing a long: $\text{Realized} = (p - \text{avg\_cost}) \times v$
  - Closing a short: $\text{Realized} = (\text{avg\_cost} - p) \times v$

### 1.2 Mark-to-Market Valuation
Mark-to-market equity $E_t$ at reference mid-price $S_t$:
$$E_t = C_t + q_t \cdot S_t$$
$$\text{Unrealized P&L} = (S_t - \text{avg\_cost}) \times q_t$$
$$\text{Net P&L} = E_t - E_0$$
$$\text{Gross P&L} = \text{Net P&L} + \text{Cumulative Fees}$$

### 1.3 Fee and Rebate Structure
- **Maker executions**: Receive a liquidity rebate (e.g., $-1$ bps, adding cash).
- **Taker executions**: Incur an aggressive execution fee (e.g., $+2$ bps, reducing cash).

## 2. Microstructure Metrics

### 2.1 Adverse Selection Markouts ($M_h$)
To measure whether filled quotes suffer from adverse selection (trading with informed counterparty flow), post-fill price markouts are evaluated at multiple forward horizons $h \in \{1\text{ms}, 10\text{ms}, 100\text{ms}, 1\text{s}\}$:

$$M_h = \epsilon (S_{t+h} - P_{\text{fill}})$$

Where $\epsilon = +1$ for buys and $\epsilon = -1$ for sells.
- $M_h > 0$: Favorable post-fill price drift (quote captured spread and price continued favorably).
- $M_h < 0$: Adverse selection (price immediately drifted against the filled position).

### 2.2 Spread Capture
For each fill, the realized spread capture relative to the prevailing mid-price is:
- Buy fill: $S_t - P_{\text{fill}}$
- Sell fill: $P_{\text{fill}} - S_t$

### 2.3 Maximum Drawdown
Defined across the equity path $\{E_u\}_{u \le t}$:
$$DD_t = \frac{E_t - \max_{u \le t} E_u}{\max_{u \le t} E_u}$$
$$\text{Max Drawdown} = \min_t DD_t$$
Both percentage drawdown and absolute peak-to-trough currency loss are reported.

## 3. Historical Replay vs. Synthetic Simulation

### Synthetic Exchange Simulation
The strategy interacts directly with the exchange order book. Background participants submit resting limit orders and taker market orders. Every fill is exact and deterministic under exchange price-time matching rules.

### Historical Reference Replay
When historical quotes and trades are replayed, public trades drive the reference price and background liquidity. Simulated fills are evaluated under conservative touch-and-trade execution models without look-ahead bias.
