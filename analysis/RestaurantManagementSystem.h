#ifndef RESTAURANT_MANAGEMENT_SYSTEM_H
#define RESTAURANT_MANAGEMENT_SYSTEM_H

#include "MarketState.h"
#include "Reservation.h"
#include "Customer.h"

// ============================================================================
// RESTAURANT MANAGEMENT SYSTEM
// ============================================================================
// Handles end-of-day processing for restaurants
// Confirms or cancels reservations based on actual inventory
// Updates restaurant ratings dynamically
// ============================================================================
class RestaurantManagementSystem {
public:
    // Process end of day: confirm/cancel reservations based on actual inventory
    // Handles three cases:
    // 1. No orders -> all bags become waste
    // 2. Enough/more bags -> distribute fairly (respecting max per customer)
    // 3. Shortage -> confirm first-come-first-served, cancel excess
    static void process_end_of_day(MarketState& market_state);

    // Handle reservation cancellation
    // Updates customer history and restaurant rating
    static void handle_cancellation(Reservation& reservation, 
                                    Customer& customer, 
                                    MarketState& market_state);

    // Handle reservation confirmation
    // Updates customer history and restaurant rating
    static void handle_confirmation(Reservation& reservation, 
                                    Customer& customer, 
                                    MarketState& market_state, 
                                    int bags_received = 1);
};

#endif // RESTAURANT_MANAGEMENT_SYSTEM_H

