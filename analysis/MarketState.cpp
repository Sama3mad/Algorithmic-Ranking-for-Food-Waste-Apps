#include "MarketState.h"

using namespace std;

// Constructor initializes time to 8:00 AM
MarketState::MarketState() : current_time(8, 0), next_reservation_id(1) {}

// Get IDs of all stores that can accept reservations
vector<int> MarketState::get_available_restaurant_ids() const {
    vector<int> available;
    for (const auto& restaurant : restaurants) {
        if (restaurant.can_accept_reservation()) {
            available.push_back(restaurant.business_id);
        }
    }
    return available;
}

// Get non-const pointer to a store by ID
Restaurant* MarketState::get_restaurant(int id) {
    for (auto& r : restaurants) {
        if (r.business_id == id) return &r;
    }
    return nullptr;
}

// Get const pointer to a store by ID
const Restaurant* MarketState::get_restaurant(int id) const {
    for (const auto& r : restaurants) {
        if (r.business_id == id) return &r;
    }
    return nullptr;
}

// Get pointer to a customer by ID
Customer* MarketState::get_customer(int id) {
    auto it = customers.find(id);
    return (it != customers.end()) ? &(it->second) : nullptr;
}

