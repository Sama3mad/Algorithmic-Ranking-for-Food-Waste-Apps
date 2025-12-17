#ifndef CUSTOMER_H
#define CUSTOMER_H

#include <string>
#include <map>
#include "Timestamp.h"

using namespace std;

// Forward declaration
class Restaurant;

// Customer History
struct CustomerHistory {
    int visits;
    int reservations;
    int successes;
    int cancellations;
    Timestamp last_reservation_time;
    
    // Category history
    map<string, int> categories_reserved;

    // Store interaction history
    struct StoreInteraction {
        int reservations;
        int successes;
        int cancellations;

        StoreInteraction();
    };
    map<int, StoreInteraction> store_interactions;

    CustomerHistory();
};

// Customer Model
class Customer {
public:
    int id;
    float longitude;
    float latitude;
    string customer_name;
    string segment;
    float willingness_to_pay;

    // Decision weights
    struct Weights {
        float rating_w;
        float price_w;
        float novelty_w;

        Weights(float r = 1.0f, float p = 1.0f, float n = 0.5f);
    } weights;

    float loyalty;
    float leaving_threshold;
    CustomerHistory history;
    bool churned;

    // Preferences
    map<string, float> category_preference;
    
    // Store valuations
    map<int, float> store_valuations;

    // Constructors
    Customer();
    Customer(int customer_id, const string& seg = "regular");
    Customer(int id_, float lon_, float lat_, const string& name_,
             const string& segment_, float wtp, float rating_weight,
             float price_weight, float novelty_weight, float leaving_thresh);

    // Calculate score for a store
    float calculate_store_score(const Restaurant& store) const;

    // Update loyalty
    void update_loyalty(bool was_cancelled);

    // Update category preference
    void update_category_preference(const string& category);

    // Record visit
    void record_visit();

    // Record reservation attempt
    void record_reservation_attempt(int store_id, const string& category, Timestamp time);

    // Record successful reservation
    void record_reservation_success(int store_id, const string& category);

    // Record cancellation
    void record_reservation_cancellation(int store_id);
};

#endif // CUSTOMER_H

