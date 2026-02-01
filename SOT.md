# SOT — Quant Internship

## Objective
Maximize probability of Tier-1 quant internship offer.

## Decision (LOCKED)
- ONE GitHub project only
- Project: C++ Order Book / Matching Engine
- Status: **COMPLETE**

## GitHub Repository
https://github.com/Yorian-melki/orderbook

## MVP Delivered
| Artifact | Status |
|----------|--------|
| src/order.hpp | ✓ |
| src/order_book.hpp | ✓ |
| src/matching_engine.hpp | ✓ |
| src/tests.cpp | ✓ 5/5 pass |
| src/benchmark.cpp | ✓ 166ns median |
| README.md | ✓ |

## Proof Delivered
| Proof | Target | Actual |
|-------|--------|--------|
| Correctness | 5 tests | 5/5 pass |
| Latency (median) | ≤10 μs | 0.166 μs |
| Latency (P99) | ≤50 μs | 0.334 μs |
| Understanding | README | Architecture + trade-offs |

## Environment
| Fact | Value |
|------|-------|
| OS | macOS arm64 |
| Compiler | Apple clang 15.0.0 |
| Hardware | Apple Silicon M-series |

## Next Phase
Use this project to apply to quant internships.

## Completed Actions
| Action | Outcome |
|--------|---------|
| Verify C++ compiler | ✓ clang 15.0.0 |
| Create order.hpp | ✓ compiles |
| Create order_book.hpp | ✓ compiles |
| Create matching_engine.hpp | ✓ compiles |
| Create tests.cpp | ✓ 5/5 pass |
| Create benchmark.cpp | ✓ 166ns |
| Create README.md | ✓ complete |
| Push to GitHub | ✓ public |
