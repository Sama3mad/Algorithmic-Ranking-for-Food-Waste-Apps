#ifndef METRICS_H
#define METRICS_H

#include <map>
#include <vector>
#include <iostream>
#include <iomanip>
#include "MarketState.h"
#include "Reservation.h"
#include "Timestamp.h"

using namespace std;

// Simulation Metrics
struct SimulationMetrics {
    int total_bags_sold;
    int total_bags_cancelled;
    int total_bags_unsold;
    float total_revenue_generated;
    float total_revenue_lost;
    int customers_who_left;
    int total_customer_arrivals;

    // Per-store metrics
    map<int, int> bags_sold_per_store;
    map<int, int> bags_cancelled_per_store;
    map<int, float> revenue_per_store;
    map<int, int> times_displayed_per_store;
    map<int, int> waste_per_store;

    float gini_coefficient_exposure;

    // Constructor
    SimulationMetrics();

    // Print summary
    void print_summary() const;
    
    // Print summary to stream
    void print_summary_to_stream(ostream& os) const;
};

// Metrics Collector
class MetricsCollector {
public:
    SimulationMetrics metrics;

    // Log customer arrival
    void log_customer_arrival(int customer_id, Timestamp time);

    // Log displayed stores
    void log_stores_displayed(const vector<int>& store_ids);

    // Log reservation
    void log_reservation(const Reservation& res, float price);

    // Log customer left
    void log_customer_left(int customer_id);

    // Log cancellation
    void log_cancellation(const Reservation& res, float lost_revenue);

    // Log confirmation
    void log_confirmation(const Reservation& res, int bags_received = 1);

    // Log end of day
    void log_end_of_day(const MarketState& market_state);

    // Calculate fairness
    void calculate_fairness_metrics(const MarketState& market_state);
};

#endif // METRICS_H

