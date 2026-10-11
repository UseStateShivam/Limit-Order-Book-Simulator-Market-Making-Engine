# Avellaneda–Stoikov Market-Making Model

## 1. Mathematical Formulation

The classical quantitative market-making model proposed by Avellaneda & Stoikov (2008) solves for optimal quoting spreads and inventory skew for a dealer subject to inventory risk and market arrival intensity.

### 1.1 Reservation Price ($r_t$)
The reservation price (or indifference price) represents the subjective price at which the market maker is indifferent between keeping their current position and executing a trade:

$$r_t = S_t - q_t \gamma \sigma^2 \tau$$

Where:
- $S_t$: Reference mid-price at time $t$
- $q_t$: Current signed inventory ($q_t > 0$ long, $q_t < 0$ short)
- $\gamma$: Risk aversion parameter ($\gamma > 0$)
- $\sigma$: Estimated volatility of the mid-price
- $\tau = T - t$: Remaining strategy horizon in seconds

**Economic Intuition**:
- When inventory is positive ($q_t > 0$), $r_t < S_t$. The market maker lowers both quotes to attract buyers and discourage sellers.
- When inventory is negative ($q_t < 0$), $r_t > S_t$. The market maker raises quotes to attract sellers and close the short position.
- Higher risk aversion $\gamma$, higher volatility $\sigma$, or longer remaining horizon $\tau$ amplifies the inventory adjustment.

### 1.2 Optimal Half-Spread ($\delta_t$)
Under exponential order arrival intensity $\lambda(\delta) = A e^{-k \delta}$, the baseline optimal half-spread is given by:

$$\delta_t = \frac{\gamma \sigma^2 \tau}{2} + \frac{1}{\gamma} \ln\left(1 + \frac{\gamma}{k}\right)$$

Where:
- $k$: Order arrival intensity decay parameter ($k > 0$)
- $\frac{\gamma \sigma^2 \tau}{2}$: Spread compensation required for holding inventory risk over horizon $\tau$.
- $\frac{1}{\gamma} \ln\left(1 + \frac{\gamma}{k}\right)$: Monopolistic compensation for order arrival elasticity.

### 1.3 Small-$\gamma$ Numerical Stability
As $\gamma \to 0$ (risk-neutral market maker), naive computation of $\frac{1}{\gamma}\ln(1 + \gamma/k)$ suffers numerical cancellation. Using Taylor series expansion:

$$\lim_{\gamma \to 0} \frac{1}{\gamma} \ln\left(1 + \frac{\gamma}{k}\right) \approx \frac{1}{k} - \frac{\gamma}{2k^2} + \mathcal{O}(\gamma^2)$$

Our implementation uses this Taylor approximation when $\gamma < 10^{-5}$ to ensure floating-point stability.

### 1.4 Target Quotes and Discrete Tick Rounding
Continuous target prices:
$$p_b^* = r_t - \delta_t$$
$$p_a^* = r_t + \delta_t$$

These continuous values are mapped to discrete exchange ticks:
$$\text{bid\_ticks} = \lfloor p_b^* / \text{tick\_size} \rfloor$$
$$\text{ask\_ticks} = \lceil p_a^* / \text{tick\_size} \rceil$$

Subject to the constraints:
1. $\text{bid\_ticks} < \text{ask\_ticks}$ (minimum spread $\ge 1$ tick).
2. Quotes cannot cross the prevailing inside book: $\text{bid\_ticks} \le \text{best\_ask} - 1$ and $\text{ask\_ticks} \ge \text{best\_bid} + 1$.

## 2. Liquidity-Aware Extension

To capture microstructure order flow dynamics beyond the continuous mid-price, the engine provides an extension incorporating normalized top-of-book imbalance:

$$I_t = \frac{Q_b - Q_a}{Q_b + Q_a}$$

Where $Q_b$ and $Q_a$ are cumulative quantities across the top 5 price levels.
The reservation price is adjusted as:

$$r_t' = r_t + \eta \cdot I_t \cdot \text{tick\_size}$$

Where $\eta$ (`liquidity_skew_coeff`) parameterizes how aggressively the strategy anticipates short-term order book pressure.
