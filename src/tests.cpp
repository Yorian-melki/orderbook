#include "matching_engine.hpp"
#include <iostream>
#include <cassert>

using namespace orderbook;

// Helper to create orders quickly
Order make_order(uint64_t id, OrderType type, Side side, double price, uint32_t qty) {
    Order o;
    o.id = id;
    o.type = type;
    o.side = side;
    o.price = price;
    o.quantity = qty;
    o.timestamp = std::chrono::steady_clock::now();
    return o;
}

// TEST 1: Limit buy + limit sell at crossing prices → match
void test_crossing_limit_orders() {
    MatchingEngine engine;
    
    // Sell at 100
    auto trades1 = engine.process_order(make_order(1, OrderType::Limit, Side::Sell, 100.0, 10));
    assert(trades1.empty()); // No match yet
    assert(engine.has_asks());
    
    // Buy at 100 (crosses)
    auto trades2 = engine.process_order(make_order(2, OrderType::Limit, Side::Buy, 100.0, 10));
    assert(trades2.size() == 1);
    assert(trades2[0].price == 100.0);
    assert(trades2[0].quantity == 10);
    assert(trades2[0].buy_order_id == 2);
    assert(trades2[0].sell_order_id == 1);
    
    // Book should be empty now
    assert(!engine.has_bids());
    assert(!engine.has_asks());
    
    std::cout << "TEST 1 PASSED: Crossing limit orders match" << std::endl;
}

// TEST 2: Limit order, then cancel → order removed
void test_cancel_order() {
    MatchingEngine engine;
    
    // Add order
    engine.process_order(make_order(1, OrderType::Limit, Side::Buy, 100.0, 10));
    assert(engine.has_bids());
    
    // Cancel it
    bool cancelled = engine.cancel_order(1);
    assert(cancelled);
    assert(!engine.has_bids());
    
    // Cancel non-existent order
    bool cancelled2 = engine.cancel_order(999);
    assert(!cancelled2);
    
    std::cout << "TEST 2 PASSED: Cancel removes order" << std::endl;
}

// TEST 3: Partial fill → remaining quantity preserved
void test_partial_fill() {
    MatchingEngine engine;
    
    // Sell 10 at 100
    engine.process_order(make_order(1, OrderType::Limit, Side::Sell, 100.0, 10));
    
    // Buy only 3 at 100
    auto trades = engine.process_order(make_order(2, OrderType::Limit, Side::Buy, 100.0, 3));
    assert(trades.size() == 1);
    assert(trades[0].quantity == 3);
    
    // Sell order should still be on book with quantity 7
    assert(engine.has_asks());
    assert(engine.best_ask() == 100.0);
    
    // Buy remaining 7
    auto trades2 = engine.process_order(make_order(3, OrderType::Limit, Side::Buy, 100.0, 7));
    assert(trades2.size() == 1);
    assert(trades2[0].quantity == 7);
    
    // Now book should be empty
    assert(!engine.has_asks());
    
    std::cout << "TEST 3 PASSED: Partial fill preserves remaining quantity" << std::endl;
}

// TEST 4: Market order against multiple price levels → sweeps book
void test_market_order_sweeps() {
    MatchingEngine engine;
    
    // Add sells at different prices
    engine.process_order(make_order(1, OrderType::Limit, Side::Sell, 100.0, 5));
    engine.process_order(make_order(2, OrderType::Limit, Side::Sell, 101.0, 5));
    engine.process_order(make_order(3, OrderType::Limit, Side::Sell, 102.0, 5));
    
    // Market buy for 12 (should take all of 100, all of 101, and 2 from 102)
    auto trades = engine.process_order(make_order(4, OrderType::Market, Side::Buy, 0.0, 12));
    
    assert(trades.size() == 3);
    assert(trades[0].price == 100.0);
    assert(trades[0].quantity == 5);
    assert(trades[1].price == 101.0);
    assert(trades[1].quantity == 5);
    assert(trades[2].price == 102.0);
    assert(trades[2].quantity == 2);
    
    // Should still have 3 left at 102
    assert(engine.has_asks());
    assert(engine.best_ask() == 102.0);
    
    std::cout << "TEST 4 PASSED: Market order sweeps multiple levels" << std::endl;
}

// TEST 5: Empty book + market order → no crash, no trades
void test_market_order_empty_book() {
    MatchingEngine engine;
    
    // Market buy on empty book
    auto trades = engine.process_order(make_order(1, OrderType::Market, Side::Buy, 0.0, 10));
    assert(trades.empty());
    
    // Market sell on empty book
    auto trades2 = engine.process_order(make_order(2, OrderType::Market, Side::Sell, 0.0, 10));
    assert(trades2.empty());
    
    std::cout << "TEST 5 PASSED: Market order on empty book handles gracefully" << std::endl;
}

int main() {
    std::cout << "Running unit tests...\n" << std::endl;
    
    test_crossing_limit_orders();
    test_cancel_order();
    test_partial_fill();
    test_market_order_sweeps();
    test_market_order_empty_book();
    
    std::cout << "\n=== ALL 5 TESTS PASSED ===" << std::endl;
    return 0;
}
