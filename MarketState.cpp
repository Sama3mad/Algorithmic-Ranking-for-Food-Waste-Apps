#include "MarketState.h"

MarketState::MarketState() : current_time(8, 0), next_reservation_id(1) {}

std::vector<int> MarketState::get_available_restaurant_ids() const {
    std::vector<int> available;
    for (const auto& restaurant : restaurants) {
        if (restaurant.can_accept_reservation()) {
            available.push_back(restaurant.business_id);
        }
    }
    return available;
}

Restaurant* MarketState::get_restaurant(int id) {
    for (auto& r : restaurants) {
        if (r.business_id == id) return &r;
    }
    return nullptr;
}

const Restaurant* MarketState::get_restaurant(int id) const {
    for (const auto& r : restaurants) {
        if (r.business_id == id) return &r;
    }
    return nullptr;
}

Customer* MarketState::get_customer(int id) {
    auto it = customers.find(id);
    return (it != customers.end()) ? &(it->second) : nullptr;
}

