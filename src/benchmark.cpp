#include "matching_engine.hpp"
#include <iostream>
#include <vector>
#include <algorithm>
#include <iomanip>

using namespace orderbook;
using Clock = std::chrono::steady_clock;

// Measure latency of a single operation in nanoseconds
template<typename Func>
long long measure_ns(Func&& func) {
    auto start = Clock::now();
    func();
    auto end = Clock::now();
    return std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count();
}

// Calculate percentile from sorted vector
double percentile(std::vector<long long>& sorted_data, double p) {
    size_t idx = static_cast<size_t>(p * sorted_data.size());
    if (idx >= sorted_data.size()) idx = sorted_data.size() - 1;
    return static_cast<double>(sorted_data[idx]);
}

int main() {
    const int NUM_OPERATIONS = 100000;
    
    std::cout << "=== ORDER BOOK LATENCY BENCHMARK ===" << std::endl;
    std::cout << "Operations per test: " << NUM_OPERATIONS << std::endl;
    std::cout << std::endl;

    // Benchmark 1: Add limit orders
    {
        std::vector<long long> latencies;
        latencies.reserve(NUM_OPERATIONS);
        MatchingEngine engine;
        
        for (int i = 0; i < NUM_OPERATIONS; i++) {
            Order order;
            order.id = i;
            order.type = OrderType::Limit;
            order.side = (i % 2 == 0) ? Side::Buy : Side::Sell;
            order.price = 100.0 + (i % 100); // Spread across 100 price levels
            order.quantity = 10;
            
            long long ns = measure_ns([&]() {
                engine.process_order(order);
            });
            latencies.push_back(ns);
        }
        
        std::sort(latencies.begin(), latencies.end());
        
        double median = percentile(latencies, 0.50);
        double p99 = percentile(latencies, 0.99);
        double mean = 0;
        for (auto l : latencies) mean += l;
        mean /= latencies.size();
        
        std::cout << "ADD LIMIT ORDER:" << std::endl;
        std::cout << "  Median:  " << std::fixed << std::setprecision(0) << median << " ns (" << median/1000 << " μs)" << std::endl;
        std::cout << "  Mean:    " << mean << " ns (" << mean/1000 << " μs)" << std::endl;
        std::cout << "  P99:     " << p99 << " ns (" << p99/1000 << " μs)" << std::endl;
        std::cout << std::endl;
    }

    // Benchmark 2: Matching orders (crossing)
    {
        std::vector<long long> latencies;
        latencies.reserve(NUM_OPERATIONS);
        
        for (int i = 0; i < NUM_OPERATIONS; i++) {
            MatchingEngine engine;
            
            // Pre-populate with a sell order
            Order sell;
            sell.id = 1;
            sell.type = OrderType::Limit;
            sell.side = Side::Sell;
            sell.price = 100.0;
            sell.quantity = 10;
            engine.process_order(sell);
            
            // Measure matching buy
            Order buy;
            buy.id = 2;
            buy.type = OrderType::Limit;
            buy.side = Side::Buy;
            buy.price = 100.0;
            buy.quantity = 10;
            
            long long ns = measure_ns([&]() {
                engine.process_order(buy);
            });
            latencies.push_back(ns);
        }
        
        std::sort(latencies.begin(), latencies.end());
        
        double median = percentile(latencies, 0.50);
        double p99 = percentile(latencies, 0.99);
        double mean = 0;
        for (auto l : latencies) mean += l;
        mean /= latencies.size();
        
        std::cout << "MATCH ORDER (crossing):" << std::endl;
        std::cout << "  Median:  " << std::fixed << std::setprecision(0) << median << " ns (" << median/1000 << " μs)" << std::endl;
        std::cout << "  Mean:    " << mean << " ns (" << mean/1000 << " μs)" << std::endl;
        std::cout << "  P99:     " << p99 << " ns (" << p99/1000 << " μs)" << std::endl;
        std::cout << std::endl;
    }

    // Benchmark 3: Cancel orders
    {
        std::vector<long long> latencies;
        latencies.reserve(NUM_OPERATIONS);
        MatchingEngine engine;
        
        // Pre-populate
        for (int i = 0; i < NUM_OPERATIONS; i++) {
            Order order;
            order.id = i;
            order.type = OrderType::Limit;
            order.side = Side::Buy;
            order.price = 100.0 + (i % 100);
            order.quantity = 10;
            engine.process_order(order);
        }
        
        // Measure cancels
        for (int i = 0; i < NUM_OPERATIONS; i++) {
            long long ns = measure_ns([&]() {
                engine.cancel_order(i);
            });
            latencies.push_back(ns);
        }
        
        std::sort(latencies.begin(), latencies.end());
        
        double median = percentile(latencies, 0.50);
        double p99 = percentile(latencies, 0.99);
        double mean = 0;
        for (auto l : latencies) mean += l;
        mean /= latencies.size();
        
        std::cout << "CANCEL ORDER:" << std::endl;
        std::cout << "  Median:  " << std::fixed << std::setprecision(0) << median << " ns (" << median/1000 << " μs)" << std::endl;
        std::cout << "  Mean:    " << mean << " ns (" << mean/1000 << " μs)" << std::endl;
        std::cout << "  P99:     " << p99 << " ns (" << p99/1000 << " μs)" << std::endl;
        std::cout << std::endl;
    }

    // Summary
    std::cout << "=== BENCHMARK COMPLETE ===" << std::endl;
    std::cout << "Hardware: Apple Silicon (arm64)" << std::endl;
    std::cout << "Compiler: Apple clang 15.0.0" << std::endl;
    std::cout << "Standard: C++17" << std::endl;
    
    return 0;
}
