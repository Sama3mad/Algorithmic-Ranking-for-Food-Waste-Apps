#include "Metrics.h"
#include <algorithm>

SimulationMetrics::SimulationMetrics() 
    : total_bags_sold(0), total_bags_cancelled(0),
      total_bags_unsold(0), total_revenue_generated(0.0f),
      total_revenue_lost(0.0f), customers_who_left(0),
      total_customer_arrivals(0),
      gini_coefficient_exposure(0.0f) {
}

void SimulationMetrics::print_summary() const {
    std::cout << "\n========================================" << std::endl;
    std::cout << "=== SIMULATION METRICS SUMMARY ===" << std::endl;
    std::cout << "========================================" << std::endl;
    std::cout << "\n--- SALES METRICS ---" << std::endl;
    std::cout << "Total Bags Sold: " << total_bags_sold << std::endl;
    std::cout << "Total Bags Cancelled: " << total_bags_cancelled << std::endl;
    std::cout << "Total Bags Unsold (Waste): " << total_bags_unsold << std::endl;
    
    std::cout << "\n--- REVENUE METRICS ---" << std::endl;
    std::cout << "Total Revenue Generated: $" << std::fixed << std::setprecision(2) 
              << total_revenue_generated << std::endl;
    std::cout << "Revenue Lost (from cancellations): $" << std::fixed << std::setprecision(2) 
              << total_revenue_lost << std::endl;
    float revenue_efficiency = total_customer_arrivals > 0 ? 
        (total_revenue_generated / (total_revenue_generated + total_revenue_lost)) * 100.0f : 0.0f;
    std::cout << "Revenue Efficiency: " << std::fixed << std::setprecision(2) 
              << revenue_efficiency << "%" << std::endl;
    
    std::cout << "\n--- CUSTOMER METRICS ---" << std::endl;
    std::cout << "Total Customer Arrivals: " << total_customer_arrivals << std::endl;
    std::cout << "Customers Who Left (No Purchase): " << customers_who_left << std::endl;
    float conversion_rate = total_customer_arrivals > 0 ? 
        ((float)(total_customer_arrivals - customers_who_left) / total_customer_arrivals) * 100.0f : 0.0f;
    std::cout << "Conversion Rate: " << std::fixed << std::setprecision(2) 
              << conversion_rate << "%" << std::endl;
    
    std::cout << "\n--- FAIRNESS METRICS ---" << std::endl;
    std::cout << "Gini Coefficient (Exposure): " << std::fixed << std::setprecision(4) 
              << gini_coefficient_exposure << std::endl;
    std::cout << "  (0 = perfect equality, 1 = maximum inequality)" << std::endl;
    
    std::cout << "\n========================================" << std::endl;
}

void MetricsCollector::log_customer_arrival(int customer_id, Timestamp time) {
    metrics.total_customer_arrivals++;
}

void MetricsCollector::log_stores_displayed(const std::vector<int>& store_ids) {
    for (int id : store_ids) {
        metrics.times_displayed_per_store[id]++;
    }
}

void MetricsCollector::log_reservation(const Reservation& res, float price) {
    // Will be finalized at end of day
}

void MetricsCollector::log_customer_left(int customer_id) {
    metrics.customers_who_left++;
}

void MetricsCollector::log_cancellation(const Reservation& res, float lost_revenue) {
    metrics.total_bags_cancelled++;
    metrics.total_revenue_lost += lost_revenue;
    metrics.bags_cancelled_per_store[res.restaurant_id]++;
}

void MetricsCollector::log_confirmation(const Reservation& res) {
    metrics.total_bags_sold++;
}

void MetricsCollector::log_end_of_day(const MarketState& market_state) {
    metrics.total_bags_sold = 0;
    metrics.total_bags_cancelled = 0;
    metrics.total_bags_unsold = 0;
    metrics.total_revenue_generated = 0.0f;
    metrics.total_revenue_lost = 0.0f;

    for (const auto& restaurant : market_state.restaurants) {
        int sold = 0;
        int cancelled = 0;

        for (const auto& res : market_state.reservations) {
            if (res.restaurant_id == restaurant.business_id) {
                if (res.status == Reservation::CONFIRMED) {
                    sold++;
                    metrics.total_bags_sold++;
                    metrics.revenue_per_store[restaurant.business_id] +=
                        restaurant.price_per_bag;
                    metrics.total_revenue_generated += restaurant.price_per_bag;
                }
                else if (res.status == Reservation::CANCELLED) {
                    cancelled++;
                    metrics.total_bags_cancelled++;
                    metrics.total_revenue_lost += restaurant.price_per_bag;
                }
            }
        }

        metrics.bags_sold_per_store[restaurant.business_id] = sold;
        metrics.bags_cancelled_per_store[restaurant.business_id] = cancelled;

        int unsold = 0;
        if (sold == 0 && cancelled == 0) {
            unsold = restaurant.actual_bags;
        } else {
            unsold = 0;
        }

        if (unsold > 0) {
            metrics.total_bags_unsold += unsold;
            metrics.waste_per_store[restaurant.business_id] = unsold;
        } else {
            metrics.waste_per_store[restaurant.business_id] = 0;
        }
    }
}

void MetricsCollector::calculate_fairness_metrics(const MarketState& market_state) {
    std::vector<int> exposures;
    for (const auto& restaurant : market_state.restaurants) {
        exposures.push_back(metrics.times_displayed_per_store[restaurant.business_id]);
    }

    if (exposures.empty()) {
        metrics.gini_coefficient_exposure = 0.0f;
        return;
    }

    std::sort(exposures.begin(), exposures.end());

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

