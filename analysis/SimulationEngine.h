#ifndef SIMULATION_ENGINE_H
#define SIMULATION_ENGINE_H

#include <string>
#include <vector>
#include "MarketState.h"
#include "Metrics.h"
#include "ArrivalGenerator.h"
#include "RankingAlgorithms.h"
#include "Restaurant.h"

using namespace std;

// Simulation Engine
class SimulationEngine {
private:
    MarketState market_state;
    MetricsCollector metrics_collector;
    ArrivalGenerator arrival_generator;
    int n_displayed;
    RankingAlgorithm ranking_algorithm;
    vector<Customer> customer_pool;
    int next_customer_id;
    ostream* output_stream;
    vector<Customer> pre_generated_customers;
    vector<vector<Timestamp>> pre_generated_arrival_times;
    bool use_pre_generated_data;

public:
    // Constructor
    SimulationEngine(int n_display, const string& customer_csv, 
                     RankingAlgorithm algorithm = RankingAlgorithm::BASELINE);

    // Initialize
    void initialize(const vector<Restaurant>& restaurants);

    // Run day simulation
    void run_day_simulation(int num_customers, bool use_customer_pool = false, int day_index = -1);

    // Run multi-day simulation
    void run_multi_day_simulation(int num_days, int num_customers_per_day);

    // Get metrics
    const SimulationMetrics& get_metrics() const;

    // Export results
    void export_results(const string& filename);

    // Log detailed metrics
    void log_detailed_metrics(const SimulationMetrics* comparison_metrics = nullptr);
    
    // Set output stream
    void set_output_stream(ostream* os);
    
    // Set customer pool
    void set_customer_pool(const vector<Customer>& pool);
    
    // Set arrival times
    void set_arrival_times(const vector<vector<Timestamp>>& times);
};

#endif // SIMULATION_ENGINE_H

