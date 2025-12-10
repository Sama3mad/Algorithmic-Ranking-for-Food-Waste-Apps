#ifndef SIMULATION_ENGINE_H
#define SIMULATION_ENGINE_H

#include <string>
#include <vector>
#include "MarketState.h"
#include "Metrics.h"
#include "ArrivalGenerator.h"
#include "RankingAlgorithms.h"
#include "Restaurant.h"

// ============================================================================
// SIMULATION ENGINE
// ============================================================================
// Main simulation controller
// Orchestrates the entire day simulation
// ============================================================================
class SimulationEngine {
private:
    MarketState market_state;              // Current market state
    MetricsCollector metrics_collector;    // Metrics collection
    ArrivalGenerator arrival_generator;     // Customer generation
    int n_displayed;                       // Number of stores to display
    RankingAlgorithm ranking_algorithm;    // Which ranking algorithm to use

public:
    // Constructor
    // n_display: number of stores to show each customer
    // customer_csv: path to customer CSV file (optional)
    // algorithm: ranking algorithm to use (BASELINE or SMART)
    SimulationEngine(int n_display, const std::string& customer_csv, 
                     RankingAlgorithm algorithm = RankingAlgorithm::BASELINE);

    // Initialize simulation with restaurants
    // Sets actual inventory (80-120% of estimate)
    void initialize(const std::vector<Restaurant>& restaurants);

    // Run a full day simulation
    // Processes customer arrivals, reservations, end-of-day confirmation/cancellation
    void run_day_simulation(int num_customers);

    // Run multi-day simulation
    // Runs simulation for specified number of days
    // Aggregates metrics across all days
    void run_multi_day_simulation(int num_days, int num_customers_per_day);

    // Get current metrics
    const SimulationMetrics& get_metrics() const;

    // Export results to CSV file
    void export_results(const std::string& filename);

    // Write detailed metrics log to file
    // If comparison_metrics provided, includes comparison section
    void log_detailed_metrics(const SimulationMetrics* comparison_metrics = nullptr);
};

#endif // SIMULATION_ENGINE_H

