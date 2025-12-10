#ifndef RESTAURANT_H
#define RESTAURANT_H

#include <string>

// ============================================================================
// RESTAURANT MODEL
// ============================================================================
// Represents a restaurant/store in the marketplace
// Tracks inventory, pricing, ratings, and order statistics
// ============================================================================
class Restaurant {
public:
    // CSV Format Columns (Required)
    int business_id;                    // store_id: Unique store identifier
    std::string business_name;          // store_name: Store name (e.g., "Krispy Kreme")
    std::string branch;                 // branch: Branch name or area (e.g., "Zamalek", "New Cairo")
    int estimated_bags;                 // average_bags_at_9AM: Estimated bags at start of day (9 AM)
    float general_ranking;              // average_overall_rating: Current dynamic rating (1.0-5.0)
    float price_per_bag;                // price: Price per surprise bag (in minor units, e.g., EGP)
    float longitude;                    // longitude: Store longitude (float)
    float latitude;                     // latitude: Store latitude (float)

    // Extra Variables (Not in CSV)
    std::string business_type;          // Category: "bakery", "cafe", "restaurant"
    int actual_bags;                    // Actual bags available at end of day (10 PM)
    int reserved_count;                 // Number of reservations made
    bool has_inventory;                 // Whether store still has inventory
    int max_bags_per_customer;          // Maximum bags a single customer can receive
    
    // Dynamic rating tracking
    int total_orders_confirmed;         // Total confirmed orders (for rating updates)
    int total_orders_cancelled;         // Total cancelled orders (for rating updates)
    float initial_rating;                // Initial rating (for comparison)

    // Constructor (for CSV loading)
    Restaurant(int id, const std::string& name, const std::string& branch_name,
               int est_bags, float rating, float price, float lon, float lat,
               const std::string& type = "");

    // Legacy constructor (for backward compatibility)
    Restaurant(int id, const std::string& name, float rating,
               const std::string& type, int est_bags, float price);

    // Check if restaurant can accept more reservations
    bool can_accept_reservation() const;

    // Set actual inventory at end of day
    void set_actual_inventory(int bags);
    
    // Update rating when order is confirmed (positive feedback)
    // Increases rating by 0.01, capped at 5.0
    void update_rating_on_confirmation();
    
    // Update rating when order is cancelled (negative feedback)
    // Decreases rating by 0.05, capped at 1.0
    void update_rating_on_cancellation();
    
    // Get current dynamic rating
    float get_rating() const;
};

#endif // RESTAURANT_H

