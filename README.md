# Low-Latency Order Book & Matching Engine (C++)

A high-performance limit order book and matching engine implemented in C++17, designed for sub-microsecond latency.

## Performance

Benchmarked on Apple Silicon M-series (arm64), Apple clang 15.0.0, -O3 optimization:

| Operation | Median | P99 |
|-----------|--------|-----|
| Add Limit Order | 166 ns | 334 ns |
| Match (crossing) | 166 ns | 209 ns |
| Cancel Order | 83 ns | 125 ns |

**100,000 operations per test.**

## Features

- **Order Types**: Market, Limit, Cancel
- **Matching**: Price-time priority (FIFO at each price level)
- **Partial Fills**: Remaining quantity preserved at same queue position
- **O(log n) add/match**: Red-black tree (`std::map`) for price levels
- **O(1) cancel**: Hash map lookup for order location

## Architecture┌─────────────────────────────────────────────────────────┐
│                   MatchingEngine                        │
│  ┌───────────────────────────────────────────────────┐  │
│  │                   OrderBook                       │  │
│  │  ┌─────────────┐         ┌─────────────┐         │  │
│  │  │    Bids     │         │    Asks     │         │  │
│  │  │ std::map    │         │ std::map    │         │  │
│  │  │ (price→     │         │ (price→     │         │  │
│  │  │  OrderQueue)│         │  OrderQueue)│         │  │
│  │  └─────────────┘         └─────────────┘         │  │
│  │                                                   │  │
│  │  ┌─────────────────────────────────────────────┐  │  │
│  │  │ order_locations_: unordered_map<id, loc>    │  │  │
│  │  │ (O(1) cancel lookup)                        │  │  │
│  │  └─────────────────────────────────────────────┘  │  │
│  └───────────────────────────────────────────────────┘  │
└─────────────────────────────────────────────────────────┘## Design Decisions

### Why `std::map` (red-black tree) for price levels?

**Chosen**: `std::map<double, OrderQueue>`

**Trade-off considered**: Skip-list vs. red-black tree

| Factor | std::map (RB-tree) | Skip-list |
|--------|-------------------|-----------|
| Worst-case complexity | O(log n) guaranteed | O(log n) expected |
| Cache locality | Poor (pointer chasing) | Poor (pointer chasing) |
| Implementation | Standard library | Custom required |
| Iterator stability | Stable | Stable |

**Decision**: `std::map` chosen for correctness and simplicity. Production systems (e.g., Mercury) use custom skip-lists for ~10-20% improvement, but this requires 500+ lines of careful implementation. For this project, proving correct matching logic matters more than squeezing nanoseconds.

### Why `std::list` for order queues?

**Requirement**: FIFO matching + O(1) removal after partial fill

| Structure | Insert back | Remove front | Remove middle |
|-----------|-------------|--------------|---------------|
| `std::list` | O(1) | O(1) | O(1) with iterator |
| `std::deque` | O(1) | O(1) | O(n) |
| `std::vector` | O(1) amortized | O(n) | O(n) |

**Decision**: `std::list` is the only structure that allows O(1) removal from middle (needed when an order is cancelled or partially filled and later fully filled out of FIFO order).

### Why `std::unordered_map` for order locations?

Cancel operations must be O(1). Without this map, cancelling order #12345 would require scanning the entire book. The hash map stores `{order_id → (side, price, iterator)}`, enabling direct access.

**Memory cost**: ~48 bytes per order (id + side + price + iterator + hash overhead)

**Speed benefit**: Cancel reduced from O(n) to O(1)

## Building
```bash
# Compile with optimizations
clang++ -std=c++17 -O3 -Wall -Werror src/benchmark.cpp -o benchmark

# Run benchmark
./benchmark

# Run tests
clang++ -std=c++17 -Wall -Werror src/tests.cpp -o run_tests
./run_tests
```

## Limitations

This is a **single-threaded** implementation. Production systems require:

- Lock-free data structures for multi-threaded access
- Memory pooling to avoid allocator latency
- SIMD for batch operations
- Kernel bypass (DPDK/io_uring) for network I/O
- Custom allocators (jemalloc, tcmalloc)

## Future Work

1. **Multi-threading**: Lock-free order book using atomics
2. **Memory pooling**: Pre-allocated order objects to avoid heap allocation
3. **FIX protocol**: Parse standard trading messages
4. **Iceberg orders**: Hidden quantity support
5. **Market data output**: L2/L3 book snapshots

## File Structurequant-orderbook/
├── src/
│   ├── order.hpp           # Order struct definition
│   ├── order_book.hpp      # Order book data structure
│   ├── matching_engine.hpp # Matching logic
│   ├── tests.cpp           # Unit tests
│   └── benchmark.cpp       # Latency benchmark
├── README.md
└── SOT.md                  # Project tracking## References

- [Mercury Order Book](https://github.com/jonathanmcintyre/Mercury) - High-performance implementation (~320ns/order)
- Almgren-Chriss framework - Optimal execution theory
- Price-time priority - Standard exchange matching semantics
