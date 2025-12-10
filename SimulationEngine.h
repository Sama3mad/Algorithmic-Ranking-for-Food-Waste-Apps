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
    std::vector<Customer> customer_pool;   // Pool of customers that persist across days
    int next_customer_id;                 // Next available customer ID
    std::ostream* output_stream;          // Output stream (defaults to std::cout, can be redirected to file)
    std::vector<Customer> pre_generated_customers;  // Pre-generated customers (for fair comparison)
    std::vector<std::vector<Timestamp>> pre_generated_arrival_times;  // Pre-generated arrival times per day
    bool use_pre_generated_data;          // Flag to use pre-generated data instead of generating new

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
    // use_customer_pool: if true, reuses customers from pool across days
    // day_index: which day (0-based) for pre-generated arrival times (-1 to generate new)
    void run_day_simulation(int num_customers, bool use_customer_pool = false, int day_index = -1);

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
    
    // Set output stream (for redirecting output to file)
    void set_output_stream(std::ostream* os);
    
    // Set pre-generated customer pool (for fair algorithm comparison)
    // This ensures all algorithms use the exact same customers
    void set_customer_pool(const std::vector<Customer>& pool);
    
    // Set pre-generated arrival times for each day (for fair algorithm comparison)
    void set_arrival_times(const std::vector<std::vector<Timestamp>>& times);
};

#endif // SIMULATION_ENGINE_H

