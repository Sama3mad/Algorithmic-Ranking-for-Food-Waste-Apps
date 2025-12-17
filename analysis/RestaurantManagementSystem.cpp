#include "RestaurantManagementSystem.h"
#include <algorithm>

using namespace std;

void RestaurantManagementSystem::process_end_of_day(MarketState& market_state) {
    for (auto& restaurant : market_state.restaurants) {
        vector<Reservation*> restaurant_reservations;
        for (auto& res : market_state.reservations) {
            if (res.restaurant_id == restaurant.business_id &&
                res.status == Reservation::PENDING) {
                restaurant_reservations.push_back(&res);
            }
        }

        // Process reservations in order of time
        sort(restaurant_reservations.begin(), restaurant_reservations.end(),
            [](Reservation* a, Reservation* b) {
                return a->reservation_time < b->reservation_time;
            });

        int num_reservations = restaurant_reservations.size();
        int actual_bags = restaurant.actual_bags;

        if (num_reservations == 0) {
            continue;
        }

        if (actual_bags >= num_reservations) {
            // Enough bags for everyone
            int remaining_bags = actual_bags;
            int bags_per_customer = min(
                restaurant.max_bags_per_customer,
                remaining_bags / num_reservations
            );
            
            int extra_bags = remaining_bags - (bags_per_customer * num_reservations);
            
            for (size_t i = 0; i < restaurant_reservations.size(); i++) {
                Reservation* res = restaurant_reservations[i];
                Customer* customer = market_state.get_customer(res->customer_id);
                
                if (customer) {
                    int bags_for_this_customer = bags_per_customer;
                    // Distribute extra bags to first customers if possible
                    if (extra_bags > 0 && bags_for_this_customer < restaurant.max_bags_per_customer) {
                        bags_for_this_customer++;
                        extra_bags--;
                    }
                    
                    handle_confirmation(*res, *customer, market_state, bags_for_this_customer);
                }
            }
        } else {
            // Not enough bags for everyone (some get bags, others get cancelled)
            // First customers get 1 bag each until stock runs out
            for (int i = 0; i < actual_bags; i++) {
                Reservation* res = restaurant_reservations[i];
                Customer* customer = market_state.get_customer(res->customer_id);
                if (customer) {
                    handle_confirmation(*res, *customer, market_state, 1);
                }
            }
            
            // Remaining reservations are cancelled
            for (int i = actual_bags; i < num_reservations; i++) {
                Reservation* res = restaurant_reservations[i];
                Customer* customer = market_state.get_customer(res->customer_id);
                if (customer) {
                    handle_cancellation(*res, *customer, market_state);
                }
            }
        }
    }
}

void RestaurantManagementSystem::handle_cancellation(Reservation& reservation, 
                                                      Customer& customer, 
                                                      MarketState& market_state) {
    reservation.status = Reservation::CANCELLED;
    customer.record_reservation_cancellation(reservation.restaurant_id);
    
    Restaurant* restaurant = market_state.get_restaurant(reservation.restaurant_id);
    if (restaurant) {
        restaurant->update_rating_on_cancellation();
    }
}

void RestaurantManagementSystem::handle_confirmation(Reservation& reservation, 
                                                      Customer& customer, 
                                                      MarketState& market_state, 
                                                      int bags_received) {
    reservation.status = Reservation::CONFIRMED;
    reservation.bags_received = bags_received;  // Track bags given to customer
    customer.record_reservation_success(reservation.restaurant_id, "");
    
    Restaurant* restaurant = market_state.get_restaurant(reservation.restaurant_id);
    if (restaurant) {
        restaurant->update_rating_on_confirmation();
    }
}

