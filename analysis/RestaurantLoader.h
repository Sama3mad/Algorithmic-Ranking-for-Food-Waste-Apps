#ifndef RESTAURANT_LOADER_H
#define RESTAURANT_LOADER_H

#include <vector>
#include <string>
#include "Restaurant.h"

using namespace std;

// Restaurant Loader
class RestaurantLoader {
public:
    // Load from CSV
    static bool load_restaurants_from_csv(const string& filename,
                                          vector<Restaurant>& restaurants);

    // Generate defaults
    static void generate_default_restaurants(vector<Restaurant>& restaurants);
};

#endif // RESTAURANT_LOADER_H

