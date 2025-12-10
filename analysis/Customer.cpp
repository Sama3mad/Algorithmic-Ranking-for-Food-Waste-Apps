#include "Customer.h"
#include "Restaurant.h"
#include <algorithm>

CustomerHistory::StoreInteraction::StoreInteraction() 
    : reservations(0), successes(0), cancellations(0) {}

CustomerHistory::CustomerHistory() 
    : visits(0), reservations(0), successes(0), cancellations(0), last_reservation_time(0, 0) {}

Customer::Weights::Weights(float r, float p, float n)
    : rating_w(r), price_w(p), novelty_w(n) {}

Customer::Customer() 
    : id(0), customer_name("garry"), segment("regular"), willingness_to_pay(200.0f),
      loyalty(0.8f), leaving_threshold(5.0f), churned(false) {
    category_preference["bakery"] = 1.0f;
    category_preference["cafe"] = 1.0f;
    category_preference["restaurant"] = 1.0f;
}

Customer::Customer(int customer_id, const std::string& seg)
    : id(customer_id), segment(seg), willingness_to_pay(200.0f),
      loyalty(0.8f), leaving_threshold(3.0f), churned(false) {
    category_preference["bakery"] = 1.0f;
    category_preference["cafe"] = 1.0f;
    category_preference["restaurant"] = 1.0f;
}

Customer::Customer(int id_, float lon_, float lat_, const std::string& name_,
                   const std::string& segment_, float wtp, float rating_weight,
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

float Customer::calculate_store_score(const Restaurant& store) const {
    float rating_score = weights.rating_w * store.get_rating();
    float price_score = weights.price_w * (willingness_to_pay - store.price_per_bag) / willingness_to_pay;
    
    float novelty_score = 0.0f;
    auto it = history.categories_reserved.find(store.business_type);
    if (it == history.categories_reserved.end()) {
        novelty_score = weights.novelty_w * 1.0f;
    } else {
        novelty_score = weights.novelty_w * (1.0f / (1.0f + it->second));
    }
    
    return rating_score + price_score + novelty_score;
}

void Customer::update_loyalty(bool was_cancelled) {
    if (was_cancelled) {
        loyalty = std::max(0.0f, loyalty - 0.1f);
    } else {
        loyalty = std::min(1.0f, loyalty + 0.05f);
    }
}

void Customer::update_category_preference(const std::string& category) {
    category_preference[category] += 0.1f;
}

void Customer::record_visit() {
    history.visits++;
}

void Customer::record_reservation_attempt(int store_id, const std::string& category, Timestamp time) {
    history.reservations++;
    history.last_reservation_time = time;
    history.categories_reserved[category]++;
    history.store_interactions[store_id].reservations++;
}

void Customer::record_reservation_success(int store_id, const std::string& category) {
    history.successes++;
    history.store_interactions[store_id].successes++;
    update_category_preference(category);
}

void Customer::record_reservation_cancellation(int store_id) {
    history.cancellations++;
    history.store_interactions[store_id].cancellations++;
    update_loyalty(true);
}

