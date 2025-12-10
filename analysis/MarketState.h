#ifndef MARKETSTATE_H
#define MARKETSTATE_H

#include <vector>
#include <map>
#include "Restaurant.h"
#include "Customer.h"
#include "Reservation.h"
#include "Timestamp.h"

// ============================================================================
// MARKET STATE
// ============================================================================
// Central state management for the entire marketplace
// Contains all restaurants, customers, and reservations
// ============================================================================
class MarketState {
public:
    std::vector<Restaurant> restaurants;    // All restaurants in the marketplace
    std::map<int, Customer> customers;      // All customers (customer_id -> Customer)
    std::vector<Reservation> reservations;  // All reservations made
    Timestamp current_time;                // Current simulation time
    int next_reservation_id;                // Next available reservation ID

    // Constructor: initializes time to 8:00 AM
    MarketState();

    // Get list of restaurant IDs that can still accept reservations
    std::vector<int> get_available_restaurant_ids() const;

    // Get restaurant by ID (returns nullptr if not found)
    Restaurant* get_restaurant(int id);
    const Restaurant* get_restaurant(int id) const;

    // Get customer by ID (returns nullptr if not found)
    Customer* get_customer(int id);
};

#endif // MARKETSTATE_H

