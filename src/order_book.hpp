#ifndef ORDER_BOOK_HPP
#define ORDER_BOOK_HPP

#include "order.hpp"
#include <map>
#include <list>
#include <unordered_map>
#include <optional>
#include <functional>

namespace orderbook {

// A price level contains a queue of orders (FIFO)
using OrderQueue = std::list<Order>;

class OrderBook {
public:
    // Add a limit order to the book
    // Returns true if added, false if order ID already exists
    bool add_order(const Order& order) {
        if (order.type != OrderType::Limit) {
            return false; // Only limit orders go on the book
        }
        if (order_locations_.count(order.id)) {
            return false; // Duplicate ID
        }

        if (order.side == Side::Buy) {
            auto& queue = bids_[order.price];
            queue.push_back(order);
            auto it = std::prev(queue.end());
            order_locations_[order.id] = {order.side, order.price, it};
        } else {
            auto& queue = asks_[order.price];
            queue.push_back(order);
            auto it = std::prev(queue.end());
            order_locations_[order.id] = {order.side, order.price, it};
        }
        return true;
    }

    // Cancel an order by ID
    // Returns true if cancelled, false if not found
    bool cancel_order(uint64_t order_id) {
        auto loc_it = order_locations_.find(order_id);
        if (loc_it == order_locations_.end()) {
            return false;
        }

        const auto& loc = loc_it->second;
        if (loc.side == Side::Buy) {
            auto& queue = bids_[loc.price];
            queue.erase(loc.iterator);
            if (queue.empty()) {
                bids_.erase(loc.price);
            }
        } else {
            auto& queue = asks_[loc.price];
            queue.erase(loc.iterator);
            if (queue.empty()) {
                asks_.erase(loc.price);
            }
        }
        order_locations_.erase(loc_it);
        return true;
    }

    // Modify order quantity (for partial fills)
    // Returns true if modified, false if not found
    bool modify_quantity(uint64_t order_id, uint32_t new_quantity) {
        auto loc_it = order_locations_.find(order_id);
        if (loc_it == order_locations_.end()) {
            return false;
        }
        loc_it->second.iterator->quantity = new_quantity;
        return true;
    }

    // Get best bid price and order (highest buy price)
    std::optional<std::reference_wrapper<Order>> get_best_bid() {
        if (bids_.empty()) {
            return std::nullopt;
        }
        return bids_.rbegin()->second.front();
    }

    // Get best ask price and order (lowest sell price)
    std::optional<std::reference_wrapper<Order>> get_best_ask() {
        if (asks_.empty()) {
            return std::nullopt;
        }
        return asks_.begin()->second.front();
    }

    // Check if book has bids
    bool has_bids() const { return !bids_.empty(); }

    // Check if book has asks
    bool has_asks() const { return !asks_.empty(); }

    // Get best bid price
    std::optional<double> best_bid_price() const {
        if (bids_.empty()) return std::nullopt;
        return bids_.rbegin()->first;
    }

    // Get best ask price
    std::optional<double> best_ask_price() const {
        if (asks_.empty()) return std::nullopt;
        return asks_.begin()->first;
    }

    // Get number of orders on each side
    size_t bid_count() const {
        size_t count = 0;
        for (const auto& [price, queue] : bids_) {
            count += queue.size();
        }
        return count;
    }

    size_t ask_count() const {
        size_t count = 0;
        for (const auto& [price, queue] : asks_) {
            count += queue.size();
        }
        return count;
    }

private:
    // Bids sorted by price (use map, iterate with rbegin for highest first)
    std::map<double, OrderQueue> bids_;
    
    // Asks sorted by price (lowest first by default)
    std::map<double, OrderQueue> asks_;

    // For O(1) cancel: maps order_id -> location in the book
    struct OrderLocation {
        Side side;
        double price;
        OrderQueue::iterator iterator;
    };
    std::unordered_map<uint64_t, OrderLocation> order_locations_;
};

} // namespace orderbook

#endif // ORDER_BOOK_HPP
