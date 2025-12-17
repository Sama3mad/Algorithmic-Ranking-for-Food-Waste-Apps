#include "Customer.h"
#include "Restaurant.h"
#include <algorithm>
#include <cmath>

using namespace std;

// Distance threshold for pickup (approx 5.5km)
extern const float MAX_TRAVEL_DISTANCE = 0.05f;

// Calculate distance between two coordinate points
static float calculate_distance(float lat1, float lon1, float lat2, float lon2) {
    float dlat = lat2 - lat1;
    float dlon = lon2 - lon1;
    return sqrt(dlat * dlat + dlon * dlon);
}

// Constructor for interaction history
CustomerHistory::StoreInteraction::StoreInteraction() 
    : reservations(0), successes(0), cancellations(0) {}

// Constructor for overall history
CustomerHistory::CustomerHistory() 
    : visits(0), reservations(0), successes(0), cancellations(0), last_reservation_time(0, 0) {}

// Constructor for weights
Customer::Weights::Weights(float r, float p, float n)
    : rating_w(r), price_w(p), novelty_w(n) {}

// Default constructor
Customer::Customer() 
    : id(0), customer_name("garry"), segment("regular"), willingness_to_pay(200.0f),
      loyalty(0.8f), leaving_threshold(5.0f), churned(false) {
    category_preference["bakery"] = 1.0f;
    category_preference["cafe"] = 1.0f;
    category_preference["restaurant"] = 1.0f;
}

// Constructor with ID and segment
Customer::Customer(int customer_id, const string& seg)
    : id(customer_id), segment(seg), willingness_to_pay(200.0f),
      loyalty(0.8f), leaving_threshold(3.0f), churned(false) {
    category_preference["bakery"] = 1.0f;
    category_preference["cafe"] = 1.0f;
    category_preference["restaurant"] = 1.0f;
}

// Full constructor
Customer::Customer(int id_, float lon_, float lat_, const string& name_,
                   const string& segment_, float wtp, float rating_weight,
                   float price_weight, float novelty_weight, float leaving_thresh) {
    id = id_;
    longitude = lon_;
    latitude = lat_;
    customer_name = name_;
    segment = segment_;
    willingness_to_pay = wtp;
    weights.rating_w = rating_weight;
    weights.price_w = price_weight;
    weights.novelty_w = novelty_weight;
    loyalty = 0.8f;
    leaving_threshold = leaving_thresh;
    churned = false;
    category_preference["bakery"] = 1.0f;
    category_preference["cafe"] = 1.0f;
    category_preference["restaurant"] = 1.0f;
}

// Calculate score for a store based on preferences
float Customer::calculate_store_score(const Restaurant& store) const {
    // Determine distance
    float distance = calculate_distance(latitude, longitude, store.latitude, store.longitude);
    
    // Filter out stores that are too far
    if (distance > MAX_TRAVEL_DISTANCE) {
        return -100.0f;
    }
    
    // Calculate component scores
    float rating_score = weights.rating_w * store.get_rating();
    float price_score = weights.price_w * (willingness_to_pay - store.price_per_bag) / willingness_to_pay;
    
    // Novelty score (higher for new categories)
    float novelty_score = 0.0f;
    auto it = history.categories_reserved.find(store.business_type);
    if (it == history.categories_reserved.end()) {
        novelty_score = weights.novelty_w * 1.0f;
    } else {
        novelty_score = weights.novelty_w * (1.0f / (1.0f + it->second));
    }
    
    // Distance score: closer is better
    float normalized_distance = distance / MAX_TRAVEL_DISTANCE;
    float distance_score = (1.0f - normalized_distance) * 1.5f;
    
    return rating_score + price_score + novelty_score + distance_score;
}

// Update loyalty after an interaction
void Customer::update_loyalty(bool was_cancelled) {
    if (was_cancelled) {
        loyalty = max(0.0f, loyalty - 0.1f);
    } else {
        loyalty = min(1.0f, loyalty + 0.05f);
    }
}

// Increase preference for a category
void Customer::update_category_preference(const string& category) {
    category_preference[category] += 0.1f;
}

// Record a visit to the app
void Customer::record_visit() {
    history.visits++;
}

// Record a reservation attempt
void Customer::record_reservation_attempt(int store_id, const string& category, Timestamp time) {
    history.reservations++;
    history.last_reservation_time = time;
    history.categories_reserved[category]++;
    history.store_interactions[store_id].reservations++;
}

// Record a successful purchase
void Customer::record_reservation_success(int store_id, const string& category) {
    history.successes++;
    history.store_interactions[store_id].successes++;
    update_category_preference(category);
}

// Record a cancellation
void Customer::record_reservation_cancellation(int store_id) {
    history.cancellations++;
    history.store_interactions[store_id].cancellations++;
    update_loyalty(true);
}

