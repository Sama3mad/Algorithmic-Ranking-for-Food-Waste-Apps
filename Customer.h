#ifndef CUSTOMER_H
#define CUSTOMER_H

#include <string>
#include <map>
#include "Timestamp.h"

// Forward declaration
class Restaurant;

// ============================================================================
// CUSTOMER HISTORY
// ============================================================================
// Tracks a customer's interaction history with the platform
// ============================================================================
struct CustomerHistory {
    int visits;              // Total number of times customer opened the app
    int reservations;        // Total reservation attempts
    int successes;           // Successful reservations (confirmed)
    int cancellations;       // Cancelled reservations
    Timestamp last_reservation_time;  // When they last made a reservation
    
    // Track which categories (bakery, cafe, restaurant) they've tried
    std::map<std::string, int> categories_reserved;

    // Track interactions with each specific store
    struct StoreInteraction {
        int reservations;    // Times they tried to reserve from this store
        int successes;       // Successful reservations
        int cancellations;   // Cancelled reservations

        StoreInteraction();
    };
    std::map<int, StoreInteraction> store_interactions;  // store_id -> interaction data

    CustomerHistory();
};

// ============================================================================
// CUSTOMER MODEL
// ============================================================================
// Represents a customer in the marketplace
// Contains preferences, history, and decision-making logic
// ============================================================================
class Customer {
public:
    int id;                          // Unique customer identifier
    float longitude;                 // Customer location (longitude)
    float latitude;                  // Customer location (latitude)
    std::string customer_name;       // Customer's name
    std::string segment;             // Customer segment: "budget", "regular", "premium"
    float willingness_to_pay;        // Maximum price customer is willing to pay

    // Weights for different factors in decision-making
    struct Weights {
        float rating_w;    // Weight for store rating (higher = cares more about quality)
        float price_w;     // Weight for price (higher = more price sensitive)
        float novelty_w;   // Weight for trying new things (higher = likes variety)

        Weights(float r = 1.0f, float p = 1.0f, float n = 0.5f);
    } weights;

    float loyalty;                   // Customer loyalty (0.0-1.0, affects leaving threshold)
    float leaving_threshold;          // Minimum score needed to make a purchase
    CustomerHistory history;          // Customer's interaction history
    bool churned;                     // Whether customer has left the platform

    // Preferences for different food categories
    std::map<std::string, float> category_preference;
    
    // Store valuations: how much customer values each store (from CSV)
    std::map<int, float> store_valuations;  // store_id -> valuation

    // Constructors
    Customer();
    Customer(int customer_id, const std::string& seg = "regular");
    Customer(int id_, float lon_, float lat_, const std::string& name_,
             const std::string& segment_, float wtp, float rating_weight,
             float price_weight, float novelty_weight, float leaving_thresh);

    // Calculate how much this customer likes a specific store
    // Based on rating, price, novelty, distance, and customer preferences
    // Returns -100.0f if restaurant is beyond travel threshold
    float calculate_store_score(const Restaurant& store) const;

    // Update loyalty based on whether reservation was cancelled
    void update_loyalty(bool was_cancelled);

    // Increase preference for a category after successful purchase
    void update_category_preference(const std::string& category);

    // Record that customer visited the app
    void record_visit();

    // Record a reservation attempt
    void record_reservation_attempt(int store_id, const std::string& category, Timestamp time);

    // Record a successful reservation
    void record_reservation_success(int store_id, const std::string& category);

    // Record a cancelled reservation
    void record_reservation_cancellation(int store_id);
};

#endif // CUSTOMER_H

