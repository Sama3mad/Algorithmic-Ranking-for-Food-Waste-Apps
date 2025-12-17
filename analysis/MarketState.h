#ifndef MARKETSTATE_H
#define MARKETSTATE_H

#include <vector>
#include <map>
#include "Restaurant.h"
#include "Customer.h"
#include "Reservation.h"
#include "Timestamp.h"

using namespace std;

// Market State
class MarketState {
public:
    vector<Restaurant> restaurants;
    map<int, Customer> customers;
    vector<Reservation> reservations;
    Timestamp current_time;
    int next_reservation_id;
    map<int, int> impression_counts;

    // Constructor
    MarketState();

    // Get restaurants with inventory
    vector<int> get_available_restaurant_ids() const;

    // Helpers
    Restaurant* get_restaurant(int id);
    const Restaurant* get_restaurant(int id) const;
    Customer* get_customer(int id);
};

#endif // MARKETSTATE_H

