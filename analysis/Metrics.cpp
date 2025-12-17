#include "Metrics.h"
#include <algorithm>
#include <iostream>
#include <iomanip>

using namespace std;

// Initialize metrics
SimulationMetrics::SimulationMetrics() 
    : total_bags_sold(0), total_bags_cancelled(0),
      total_bags_unsold(0), total_revenue_generated(0.0f),
      total_revenue_lost(0.0f), customers_who_left(0),
      total_customer_arrivals(0),
      gini_coefficient_exposure(0.0f) {
}

// Print metrics to console
void SimulationMetrics::print_summary() const {
    cout << "\n========================================" << endl;
    cout << "=== SIMULATION METRICS SUMMARY ===" << endl;
    cout << "========================================" << endl;
    cout << "\n--- SALES METRICS ---" << endl;
    cout << "Total Bags Sold: " << total_bags_sold << endl;
    cout << "Total Bags Cancelled: " << total_bags_cancelled << endl;
    cout << "Total Bags Unsold (Waste): " << total_bags_unsold << endl;
    
    cout << "\n--- REVENUE METRICS ---" << endl;
    cout << "Total Revenue Generated: $" << fixed << setprecision(2) 
              << total_revenue_generated << endl;
    cout << "Revenue Lost (from cancellations): $" << fixed << setprecision(2) 
              << total_revenue_lost << endl;
    float revenue_efficiency = total_customer_arrivals > 0 ? 
        (total_revenue_generated / (total_revenue_generated + total_revenue_lost)) * 100.0f : 0.0f;
    cout << "Revenue Efficiency: " << fixed << setprecision(2) 
              << revenue_efficiency << "%" << endl;
    
    cout << "\n--- CUSTOMER METRICS ---" << endl;
    cout << "Total Customer Arrivals: " << total_customer_arrivals << endl;
    cout << "Customers Who Left (No Purchase): " << customers_who_left << endl;
    float conversion_rate = total_customer_arrivals > 0 ? 
        ((float)(total_customer_arrivals - customers_who_left) / total_customer_arrivals) * 100.0f : 0.0f;
    cout << "Conversion Rate: " << fixed << setprecision(2) 
              << conversion_rate << "%" << endl;
    
    cout << "\n--- FAIRNESS METRICS ---" << endl;
    cout << "Gini Coefficient (Exposure): " << fixed << setprecision(4) 
              << gini_coefficient_exposure << endl;
    cout << "  (0 = perfect equality, 1 = maximum inequality)" << endl;
    
    cout << "\n========================================" << endl;
}

// Print metrics to a stream (e.g. file)
void SimulationMetrics::print_summary_to_stream(ostream& os) const {
    os << "\n========================================\n";
    os << "=== SIMULATION METRICS SUMMARY ===\n";
    os << "========================================\n\n";
    os << "--- SALES METRICS ---\n";
    os << "Total Bags Sold: " << total_bags_sold << "\n";
    os << "Total Bags Cancelled: " << total_bags_cancelled << "\n";
    os << "Total Bags Unsold (Waste): " << total_bags_unsold << "\n\n";
    
    os << "--- REVENUE METRICS ---\n";
    os << "Total Revenue Generated: $" << fixed << setprecision(2) 
              << total_revenue_generated << "\n";
    os << "Revenue Lost (from cancellations): $" << fixed << setprecision(2) 
              << total_revenue_lost << "\n";
    float revenue_efficiency = total_customer_arrivals > 0 ? 
        (total_revenue_generated / (total_revenue_generated + total_revenue_lost)) * 100.0f : 0.0f;
    os << "Revenue Efficiency: " << fixed << setprecision(2) 
              << revenue_efficiency << "%\n\n";
    
    os << "--- CUSTOMER METRICS ---\n";
    os << "Total Customer Arrivals: " << total_customer_arrivals << "\n";
    os << "Customers Who Left (No Purchase): " << customers_who_left << "\n";
    float conversion_rate = total_customer_arrivals > 0 ? 
        ((float)(total_customer_arrivals - customers_who_left) / total_customer_arrivals) * 100.0f : 0.0f;
    os << "Conversion Rate: " << fixed << setprecision(2) 
              << conversion_rate << "%\n\n";
    
    os << "--- FAIRNESS METRICS ---\n";
    os << "Gini Coefficient (Exposure): " << fixed << setprecision(4) 
              << gini_coefficient_exposure << "\n";
    os << "  (0 = perfect equality, 1 = maximum inequality)\n";
    
    os << "\n========================================\n";
}

// Log a customer arrival
void MetricsCollector::log_customer_arrival(int customer_id, Timestamp time) {
    metrics.total_customer_arrivals++;
}

// Log which stores were shown
void MetricsCollector::log_stores_displayed(const vector<int>& store_ids) {
    for (int id : store_ids) {
        metrics.times_displayed_per_store[id]++;
    }
}

void MetricsCollector::log_reservation(const Reservation& res, float price) {
    // Will be finalized at end of day
}

// Log customer leaving without purchase
void MetricsCollector::log_customer_left(int customer_id) {
    metrics.customers_who_left++;
}

// Log a cancelled reservation
void MetricsCollector::log_cancellation(const Reservation& res, float lost_revenue) {
    metrics.total_bags_cancelled++;
    metrics.total_revenue_lost += lost_revenue;
    metrics.bags_cancelled_per_store[res.restaurant_id]++;
}

// Log a confirmed order
void MetricsCollector::log_confirmation(const Reservation& res, int bags_received) {
    metrics.total_bags_sold += bags_received;
}

// Calculate daily totals with accurate waste logic
void MetricsCollector::log_end_of_day(const MarketState& market_state) {
    metrics.total_bags_sold = 0;
    metrics.total_bags_cancelled = 0;
    metrics.total_bags_unsold = 0;
    metrics.total_revenue_generated = 0.0f;
    metrics.total_revenue_lost = 0.0f;

    for (const auto& restaurant : market_state.restaurants) {
        int bags_sold = 0;
        int bags_cancelled = 0;
        int total_bags_given = 0;

        for (const auto& res : market_state.reservations) {
            if (res.restaurant_id == restaurant.business_id) {
                if (res.status == Reservation::CONFIRMED) {
                    int bags_for_this_reservation = res.bags_received;
                    bags_sold += bags_for_this_reservation;
                    total_bags_given += bags_for_this_reservation;
                    metrics.total_bags_sold += bags_for_this_reservation;
                    
                    // Revenue = price * bags
                    float revenue_for_reservation = restaurant.price_per_bag * bags_for_this_reservation;
                    metrics.revenue_per_store[restaurant.business_id] += revenue_for_reservation;
                    metrics.total_revenue_generated += revenue_for_reservation;
                }
                else if (res.status == Reservation::CANCELLED) {
                    bags_cancelled++;
                    metrics.total_bags_cancelled++;
                    metrics.total_revenue_lost += restaurant.price_per_bag;
                }
            }
        }

        metrics.bags_sold_per_store[restaurant.business_id] = bags_sold;
        metrics.bags_cancelled_per_store[restaurant.business_id] = bags_cancelled;

        // WASTE: Actual inventory minus what was given to customers
        int unsold = max(0, restaurant.actual_bags - total_bags_given);

        if (unsold > 0) {
            metrics.total_bags_unsold += unsold;
            metrics.waste_per_store[restaurant.business_id] = unsold;
        } else {
            metrics.waste_per_store[restaurant.business_id] = 0;
        }
    }
}

// Calculate Gini coefficient for fairness
void MetricsCollector::calculate_fairness_metrics(const MarketState& market_state) {
    vector<int> exposures;
    for (const auto& restaurant : market_state.restaurants) {
        exposures.push_back(metrics.times_displayed_per_store[restaurant.business_id]);
    }

    if (exposures.empty()) {
        metrics.gini_coefficient_exposure = 0.0f;
        return;
    }

    sort(exposures.begin(), exposures.end());

    float sum = 0.0f;
    float weighted_sum = 0.0f;

    for (size_t i = 0; i < exposures.size(); i++) {
        sum += exposures[i];
        weighted_sum += exposures[i] * (i + 1);
    }

    if (sum == 0) {
        metrics.gini_coefficient_exposure = 0.0f;
    }
    else {
        int n = exposures.size();
        metrics.gini_coefficient_exposure =
            (2.0f * weighted_sum) / (n * sum) - (n + 1.0f) / n;
    }
}

