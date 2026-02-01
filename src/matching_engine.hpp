#ifndef MATCHING_ENGINE_HPP
#define MATCHING_ENGINE_HPP

#include "order_book.hpp"
#include <vector>

namespace orderbook {

// Represents a completed trade
struct Trade {
    uint64_t buy_order_id;
    uint64_t sell_order_id;
    double price;
    uint32_t quantity;
};

class MatchingEngine {
public:
    // Process an incoming order
    // Returns a vector of trades that occurred
    std::vector<Trade> process_order(Order order) {
        std::vector<Trade> trades;
        order.timestamp = std::chrono::steady_clock::now();

        if (order.type == OrderType::Market) {
            trades = match_market_order(order);
        } else if (order.type == OrderType::Limit) {
            trades = match_limit_order(order);
        }

        return trades;
    }

    // Cancel an existing order
    bool cancel_order(uint64_t order_id) {
        return book_.cancel_order(order_id);
    }

    // Access to book state (for testing/display)
    const OrderBook& book() const { return book_; }
    
    bool has_bids() const { return book_.has_bids(); }
    bool has_asks() const { return book_.has_asks(); }
    std::optional<double> best_bid() const { return book_.best_bid_price(); }
    std::optional<double> best_ask() const { return book_.best_ask_price(); }

private:
    OrderBook book_;

    // Match a market order against the book
    std::vector<Trade> match_market_order(Order& order) {
        std::vector<Trade> trades;

        if (order.side == Side::Buy) {
            // Buy market order: match against asks (sellers)
            while (order.quantity > 0 && book_.has_asks()) {
                auto best_ask = book_.get_best_ask();
                if (!best_ask) break;

                Order& resting = best_ask->get();
                Trade trade = execute_trade(order, resting);
                trades.push_back(trade);
            }
        } else {
            // Sell market order: match against bids (buyers)
            while (order.quantity > 0 && book_.has_bids()) {
                auto best_bid = book_.get_best_bid();
                if (!best_bid) break;

                Order& resting = best_bid->get();
                Trade trade = execute_trade(order, resting);
                trades.push_back(trade);
            }
        }

        // Any unfilled market order quantity is lost (no book placement)
        return trades;
    }

    // Match a limit order against the book, then place remainder
    std::vector<Trade> match_limit_order(Order& order) {
        std::vector<Trade> trades;

        if (order.side == Side::Buy) {
            // Buy limit: match against asks if price >= best ask
            while (order.quantity > 0 && book_.has_asks()) {
                auto best_ask_price = book_.best_ask_price();
                if (!best_ask_price || order.price < *best_ask_price) {
                    break; // Can't match: our buy price is below best ask
                }

                auto best_ask = book_.get_best_ask();
                if (!best_ask) break;

                Order& resting = best_ask->get();
                Trade trade = execute_trade(order, resting);
                trades.push_back(trade);
            }
        } else {
            // Sell limit: match against bids if price <= best bid
            while (order.quantity > 0 && book_.has_bids()) {
                auto best_bid_price = book_.best_bid_price();
                if (!best_bid_price || order.price > *best_bid_price) {
                    break; // Can't match: our sell price is above best bid
                }

                auto best_bid = book_.get_best_bid();
                if (!best_bid) break;

                Order& resting = best_bid->get();
                Trade trade = execute_trade(order, resting);
                trades.push_back(trade);
            }
        }

        // Place any remaining quantity on the book
        if (order.quantity > 0) {
            book_.add_order(order);
        }

        return trades;
    }

    // Execute a trade between incoming order and resting order
    Trade execute_trade(Order& incoming, Order& resting) {
        uint32_t fill_qty = std::min(incoming.quantity, resting.quantity);
        double fill_price = resting.price; // Price-time priority: resting order's price

        Trade trade;
        if (incoming.side == Side::Buy) {
            trade.buy_order_id = incoming.id;
            trade.sell_order_id = resting.id;
        } else {
            trade.buy_order_id = resting.id;
            trade.sell_order_id = incoming.id;
        }
        trade.price = fill_price;
        trade.quantity = fill_qty;

        // Update quantities
        incoming.quantity -= fill_qty;
        resting.quantity -= fill_qty;

        // Remove or update resting order
        if (resting.quantity == 0) {
            book_.cancel_order(resting.id);
        } else {
            book_.modify_quantity(resting.id, resting.quantity);
        }

        return trade;
    }
};

} // namespace orderbook

#endif // MATCHING_ENGINE_HPP
