#include "Restaurant.h"
#include <algorithm>

// New constructor for CSV loading (with all required CSV columns)
Restaurant::Restaurant(int id, const std::string& name, const std::string& branch_name,
                       int est_bags, float rating, float price, float lon, float lat,
                       const std::string& type)
    : business_id(id), business_name(name), branch(branch_name),
      estimated_bags(est_bags), general_ranking(rating), price_per_bag(price),
      longitude(lon), latitude(lat), business_type(type),
      actual_bags(0), reserved_count(0), has_inventory(true),
      max_bags_per_customer(3), total_orders_confirmed(0),
      total_orders_cancelled(0), initial_rating(rating),
      rating_at_day_start(rating), daily_orders_confirmed(0),
      daily_orders_cancelled(0) {
}

// Legacy constructor (for backward compatibility)
Restaurant::Restaurant(int id, const std::string& name, float rating,
                       const std::string& type, int est_bags, float price)
    : business_id(id), business_name(name), branch(""),
      estimated_bags(est_bags), general_ranking(rating), price_per_bag(price),
      longitude(0.0f), latitude(0.0f), business_type(type),
      actual_bags(0), reserved_count(0), has_inventory(true),
      max_bags_per_customer(3), total_orders_confirmed(0),
      total_orders_cancelled(0), initial_rating(rating),
      rating_at_day_start(rating), daily_orders_confirmed(0),
      daily_orders_cancelled(0) {
}

bool Restaurant::can_accept_reservation() const {
    return has_inventory && (reserved_count < estimated_bags);
}

void Restaurant::set_actual_inventory(int bags) {
    actual_bags = bags;
}

void Restaurant::update_rating_on_confirmation() {
    total_orders_confirmed++;
    daily_orders_confirmed++;
    general_ranking = std::min(5.0f, general_ranking + 0.01f);
}

void Restaurant::update_rating_on_cancellation() {
    total_orders_cancelled++;
    daily_orders_cancelled++;
    general_ranking = std::max(1.0f, general_ranking - 0.05f);
}

float Restaurant::get_rating() const {
    return general_ranking;
}

