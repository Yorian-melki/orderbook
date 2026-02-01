# SOT — Quant Internship

## Objective
Maximize probability of Tier-1 quant internship offer.

## Decision (LOCKED)
- ONE GitHub project only
- Project: C++ Order Book / Matching Engine
- Status: **IN PROGRESS**

## MVP Scope (LOCKED)
Build:
- Order types: Market, Limit, Cancel
- Matching: FIFO price-time priority
- Partial fills with queue preservation
- Data structure: std::map (red-black tree)

Prove:
- Correctness: unit tests (5 cases minimum)
- Latency: ≤10μs median, ≤50μs p99 (student hardware)
- Understanding: README with architecture + trade-offs

Skip:
- FIX protocol, Python bindings, multi-threading, real data, GUI

## Verified Environment
| Fact | Evidence |
|------|----------|
| OS | macOS (arm64-apple-darwin23) |
| Compiler | Apple clang 15.0.0 |
| C++ Standard | C++17 works |
| Strict flags | -Wall -Werror pass |

## Progress
| File | Status | Compiles |
|------|--------|----------|
| src/order.hpp | DONE | YES |
| src/order_book.hpp | NOT STARTED | — |
| src/matching_engine.cpp | NOT STARTED | — |
| tests/ | NOT STARTED | — |
| benchmark/ | NOT STARTED | — |
| README.md | NOT STARTED | — |

## Next Action
Create src/order_book.hpp

## Blocked Until
- Nothing. Proceed.
