#ifndef RESTAURANT_LOADER_H
#define RESTAURANT_LOADER_H

#include <vector>
#include <string>
#include "Restaurant.h"

// ============================================================================
// RESTAURANT LOADER
// ============================================================================
// Loads restaurants from CSV file
// CSV format: store_id, store_name, branch, average_bags_at_9AM,
//             average_overall_rating, price, longitude, latitude
// Optional extra columns: business_type, max_bags_per_customer, etc.
// ============================================================================
class RestaurantLoader {
public:
    // Load restaurants from CSV file
    // Returns true if file loaded successfully, false otherwise
    // If file doesn't exist or fails to load, returns false
    static bool load_restaurants_from_csv(const std::string& filename,
                                          std::vector<Restaurant>& restaurants);

    // Generate default restaurants (fallback if CSV not available)
    static void generate_default_restaurants(std::vector<Restaurant>& restaurants);
};

#endif // RESTAURANT_LOADER_H

