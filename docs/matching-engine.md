# Limit Order Book & Matching Engine Specification

## 1. Data Structures

The order book is optimized for cache locality and predictable constant-time mutations:

### 1.1 Contiguous Order Arena (`OrderArena`)
- Orders are allocated from a contiguous array `std::vector<OrderRecord>`.
- Reclaimed slots are recycled via a free list (`std::vector<uint32_t> free_indices_`).
- Generational handles (`OrderHandle { uint32_t index, uint32_t generation }`) ensure that stale pointers or handles to canceled orders fail safely and detect stale access immediately.

### 1.2 Price Level Queue (`PriceLevel`)
- Each price level represents an aggregate price tick.
- Orders at the same price are maintained in an intrusive doubly-linked list using 32-bit arena array indices (`prev_idx`, `next_idx`).
- Head pointer points to the earliest arriving resting order (highest time priority). Tail pointer points to the newest.
- Order insertion at the tail is $O(1)$.
- Order removal by arena index is $O(1)$ without scanning the queue.

### 1.3 Price Indexing and Order Lookup
- **Bids**: `std::map<Price, PriceLevel, std::greater<Price>>` (best bid at `.begin()`).
- **Asks**: `std::map<Price, PriceLevel, std::less<Price>>` (best ask at `.begin()`).
- **Order ID Index**: `std::unordered_map<OrderId, OrderHandle>` maps order IDs directly to arena slots for $O(1)$ average cancellation and modification.

## 2. Algorithmic Complexity

| Operation | Complexity | Description |
|---|---|---|
| Limit Order Insert (Resting) | $O(\log L)$ | Inserts/locates price level in map ($L$ active levels), $O(1)$ appends to queue |
| Order Cancellation | $O(\log L)$ | $O(1)$ hash lookup, $O(1)$ list unlink, $O(\log L)$ only if empty level pruned |
| Order Modification (Downward) | $O(1)$ | $O(1)$ hash lookup, updates remaining quantity without losing FIFO position |
| Order Modification (Upward) | $O(1)$ | $O(1)$ unlinks and appends to tail of level (loses time priority) |
| Best Bid / Ask Lookup | $O(1)$ | Dereferences `.begin()` on ordered container |
| Taker Matching (Single Level) | $O(K)$ | Matches and removes $K$ filled resting orders |
| L2 Snapshot Generation | $O(D)$ | Iterates top $D$ price levels |
| L3 Snapshot Generation | $O(N_D)$ | Traverses all individual resting orders across top $D$ levels |

## 3. Order Matching Semantics

- **Price-Time Priority**: An incoming order executes first against the best available opposite price. Within that price, resting orders are filled strictly in arrival sequence order (FIFO).
- **Execution Price**: Baseline continuous double auction matches at the resting maker order's limit price.
- **Partial Fills**: If an incoming order only partially fills a resting order, the remaining quantity of the resting order remains at the head of the price queue, retaining its priority.
- **Market Orders**: Aggressively execute against existing liquidity. Any unfilled remainder is immediately canceled (never rests).
- **Time-in-Force (TIF)**:
  - `GTC` (Good-Til-Cancelled): Unfilled remainder rests on the book.
  - `IOC` (Immediate-Or-Cancel): Unfilled remainder is immediately canceled.
  - `FOK` (Fill-Or-Kill): Pre-checks eligible liquidity. If the book cannot satisfy the full quantity immediately, the entire order is canceled with zero executions.

## 4. Invariant Verification

The engine verifies these invariants during debug runs and testing:
1. **Uncrossed Book**: $\text{Best Bid} < \text{Best Ask}$ whenever both sides exist.
2. **Quantity Consistency**: For every price level, `aggregate_qty` equals the sum of `remaining_qty` across its queue.
3. **Queue Integrity**: Head and tail pointers are valid, forward and backward list links are symmetric and free of cycles.
4. **Index Consistency**: The order-ID hash table size strictly matches the total active orders across all price levels.
