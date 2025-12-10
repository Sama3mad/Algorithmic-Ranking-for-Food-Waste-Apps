#ifndef ARRIVAL_GENERATOR_H
#define ARRIVAL_GENERATOR_H

#include <vector>
#include <string>
#include <random>
#include <ctime>
#include "Customer.h"
#include "Timestamp.h"
#include "Restaurant.h"

// ============================================================================
// ARRIVAL GENERATOR
// ============================================================================
// Generates customer arrivals and loads customers from CSV
// Handles customer data loading and generation
// ============================================================================
class ArrivalGenerator {
private:
    std::mt19937 rng;                          // Random number generator
    std::vector<Customer> customers_from_csv;  // Customers loaded from CSV

public:
    // Constructor: initialize with optional seed
    ArrivalGenerator(unsigned seed = std::time(nullptr));

    // Constructor: try to load customers from CSV file
    ArrivalGenerator(const std::string& csv_path, unsigned seed = std::time(nullptr));

    // Generate random arrival times for customers (8 AM - 9 PM)
    // Returns sorted list of timestamps
    std::vector<Timestamp> generate_arrival_times(int num_customers);

    // Load customers from CSV file
    // Supports two formats:
    // 1. User format: latitude, longitude, store1_id_valuation, store2_id_valuation, ...
    // 2. Code format: id, lon, lat, name, segment, wtp, rating_w, price_w, novelty_w, leave_thresh
    // Returns true if file loaded successfully, false otherwise
    bool load_customers_from_csv(const std::string& filename);

    // Generate a customer (from CSV if available, otherwise random)
    // If CSV loaded: cycles through CSV customers
    // If no CSV: generates random customer with store valuations
    Customer generate_customer(int index, const std::vector<Restaurant>& restaurants = std::vector<Restaurant>());
};

#endif // ARRIVAL_GENERATOR_H

