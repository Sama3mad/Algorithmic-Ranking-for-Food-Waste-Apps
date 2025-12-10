#ifndef METRICS_H
#define METRICS_H

#include <map>
#include <vector>
#include <iostream>
#include <iomanip>
#include "MarketState.h"
#include "Reservation.h"
#include "Timestamp.h"

// ============================================================================
// SIMULATION METRICS
// ============================================================================
// Stores all metrics collected during simulation
// Used for evaluation and comparison of algorithms
// ============================================================================
struct SimulationMetrics {
    int total_bags_sold;              // Total bags successfully sold
    int total_bags_cancelled;          // Total bags cancelled (shortage)
    int total_bags_unsold;             // Total bags unsold (waste - only if no orders)
    float total_revenue_generated;     // Total revenue from confirmed orders
    float total_revenue_lost;          // Revenue lost from cancellations
    int customers_who_left;             // Customers who left without purchasing
    int total_customer_arrivals;       // Total customers who visited app

    // Per-store metrics
    std::map<int, int> bags_sold_per_store;        // store_id -> bags sold
    std::map<int, int> bags_cancelled_per_store;    // store_id -> bags cancelled
    std::map<int, float> revenue_per_store;         // store_id -> revenue
    std::map<int, int> times_displayed_per_store;   // store_id -> exposure count
    std::map<int, int> waste_per_store;             // store_id -> waste (bags)

    float gini_coefficient_exposure;   // Fairness metric (0=equal, 1=unequal)

    // Constructor: initialize all metrics to zero
    SimulationMetrics();

    // Print formatted summary of all metrics
    void print_summary() const;
};

// ============================================================================
// METRICS COLLECTOR
// ============================================================================
// Collects and calculates metrics throughout the simulation
// ============================================================================
class MetricsCollector {
public:
    SimulationMetrics metrics;  // The metrics being collected

    // Log a customer arrival
    void log_customer_arrival(int customer_id, Timestamp time);

    // Log which stores were displayed to a customer
    void log_stores_displayed(const std::vector<int>& store_ids);

    // Log a reservation (placeholder - finalized at end of day)
    void log_reservation(const Reservation& res, float price);

    // Log when a customer leaves without purchasing
    void log_customer_left(int customer_id);

    // Log a cancellation
    void log_cancellation(const Reservation& res, float lost_revenue);

    // Log a confirmation
    void log_confirmation(const Reservation& res);

    // Calculate end-of-day metrics from final state
    // Counts sold, cancelled, waste, revenue, etc.
    void log_end_of_day(const MarketState& market_state);

    // Calculate fairness metrics (Gini coefficient)
    // Measures inequality in store exposure
    void calculate_fairness_metrics(const MarketState& market_state);
};

#endif // METRICS_H

