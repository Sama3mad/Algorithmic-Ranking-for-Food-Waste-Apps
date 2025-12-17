#ifndef RESTAURANT_H
#define RESTAURANT_H

#include <string>

using namespace std;

// Restaurant Model
class Restaurant {
public:
    // CSV Columns
    int business_id;
    string business_name;
    string branch;
    int estimated_bags;
    float general_ranking;
    float price_per_bag;
    float longitude;
    float latitude;

    // Additional Properties
    string business_type;
    int actual_bags;
    int reserved_count;
    bool has_inventory;
    int max_bags_per_customer;
    
    // Dynamic rating tracking
    int total_orders_confirmed;
    int total_orders_cancelled;
    float initial_rating;
    float rating_at_day_start;
    int daily_orders_confirmed;
    int daily_orders_cancelled;

    // Constructors
    Restaurant(int id, const string& name, const string& branch_name,
               int est_bags, float rating, float price, float lon, float lat,
               const string& type = "");

    // Legacy constructor
    Restaurant(int id, const string& name, float rating,
               const string& type, int est_bags, float price);

    // Check availability
    bool can_accept_reservation() const;

    // Set actual inventory
    void set_actual_inventory(int bags);
    
    // Update ratings
    void update_rating_on_confirmation();
    void update_rating_on_cancellation();
    
    // Get rating
    float get_rating() const;
};

#endif // RESTAURANT_H

