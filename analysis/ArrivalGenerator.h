#ifndef ARRIVAL_GENERATOR_H
#define ARRIVAL_GENERATOR_H

#include <vector>
#include <string>
#include <random>
#include <ctime>
#include "Customer.h"
#include "Timestamp.h"
#include "Restaurant.h"

using namespace std;

// Arrival Generator
class ArrivalGenerator {
private:
    mt19937 rng; // Random number generator
    vector<Customer> customers_from_csv;

public:
    // Constructors
    ArrivalGenerator(unsigned seed = time(nullptr));
    ArrivalGenerator(const string& csv_path, unsigned seed = time(nullptr));

    // Generate arrival times (8 AM - 9 PM)
    vector<Timestamp> generate_arrival_times(int num_customers);

    // Load customers from CSV
    bool load_customers_from_csv(const string& filename);

    // Generate a customer
    Customer generate_customer(int index, const vector<Restaurant>& restaurants = vector<Restaurant>());
};

#endif // ARRIVAL_GENERATOR_H

