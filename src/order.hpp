#ifndef ORDER_HPP
#define ORDER_HPP

#include <cstdint>
#include <chrono>

namespace orderbook {

enum class OrderType {
    Market,
    Limit
};

enum class Side {
    Buy,
    Sell
};

struct Order {
    uint64_t id;
    OrderType type;
    Side side;
    double price;
    uint32_t quantity;
    std::chrono::steady_clock::time_point timestamp;
};

} // namespace orderbook

#endif // ORDER_HPP
