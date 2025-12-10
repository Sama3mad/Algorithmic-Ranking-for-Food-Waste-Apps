#include <iostream>
#include <iomanip>
#include <fstream>
#include <vector>
#include "Restaurant.h"
#include "RestaurantLoader.h"
#include "SimulationEngine.h"
#include "RankingAlgorithms.h"

int main() {
    std::cout << "=== Food Waste Marketplace Simulation ===" << std::endl;

    // Load restaurants from CSV file, or use default if not available
    std::vector<Restaurant> restaurants;
    if (!RestaurantLoader::load_restaurants_from_csv("stores.csv", restaurants)) {
        std::cout << "Using default restaurants..." << std::endl;
        RestaurantLoader::generate_default_restaurants(restaurants);
    }

    std::cout << "\n" << std::string(70, '=') << std::endl;
    std::cout << "RUNNING 7-DAY SIMULATION WITH BASELINE ALGORITHM" << std::endl;
    std::cout << std::string(70, '=') << std::endl;

    // Initialize simulation with BASELINE algorithm
    SimulationEngine engine_baseline(5, "customer.csv", RankingAlgorithm::BASELINE);
    engine_baseline.initialize(restaurants);

    // Run 7-day simulation with 100 customers per day
    engine_baseline.run_multi_day_simulation(7, 100);

    // Print results
    std::cout << "\n" << std::string(70, '=') << std::endl;
    std::cout << "BASELINE ALGORITHM RESULTS" << std::endl;
    std::cout << std::string(70, '=') << std::endl;
    engine_baseline.get_metrics().print_summary();

    // Export to CSV
    engine_baseline.export_results("simulation_results_baseline.csv");
    
    // Write detailed log (clear file first, then write baseline)
    std::ofstream clear_log("simulation_log.txt");
    clear_log.close();
    engine_baseline.log_detailed_metrics();

    std::cout << "\n\n" << std::string(70, '=') << std::endl;
    std::cout << "RUNNING 7-DAY SIMULATION WITH SMART ALGORITHM" << std::endl;
    std::cout << std::string(70, '=') << std::endl;

    // Initialize simulation with SMART algorithm
    SimulationEngine engine_smart(5, "customer.csv", RankingAlgorithm::SMART);
    engine_smart.initialize(restaurants);

    // Run 7-day simulation with 100 customers per day
    engine_smart.run_multi_day_simulation(7, 100);

    // Print results
    std::cout << "\n" << std::string(70, '=') << std::endl;
    std::cout << "SMART ALGORITHM RESULTS" << std::endl;
    std::cout << std::string(70, '=') << std::endl;
    engine_smart.get_metrics().print_summary();

    // Export to CSV
    engine_smart.export_results("simulation_results_smart.csv");
    
    // Write detailed log with comparison to baseline
    engine_smart.log_detailed_metrics(&engine_baseline.get_metrics());

    // Comparison Summary
    std::cout << "\n\n" << std::string(70, '=') << std::endl;
    std::cout << "7-DAY ALGORITHM COMPARISON SUMMARY" << std::endl;
    std::cout << std::string(70, '=') << std::endl;
    
    const auto& metrics_baseline = engine_baseline.get_metrics();
    const auto& metrics_smart = engine_smart.get_metrics();
    
    std::cout << "\nMetric                          | Baseline | Smart    | Change" << std::endl;
    std::cout << std::string(70, '-') << std::endl;
    std::cout << "Bags Sold                       | " << std::setw(8) << metrics_baseline.total_bags_sold 
              << " | " << std::setw(8) << metrics_smart.total_bags_sold
              << " | " << std::setw(6) << (metrics_smart.total_bags_sold - metrics_baseline.total_bags_sold) << std::endl;
    std::cout << "Bags Cancelled                  | " << std::setw(8) << metrics_baseline.total_bags_cancelled
              << " | " << std::setw(8) << metrics_smart.total_bags_cancelled
              << " | " << std::setw(6) << (metrics_smart.total_bags_cancelled - metrics_baseline.total_bags_cancelled) << std::endl;
    std::cout << "Bags Unsold (Waste)             | " << std::setw(8) << metrics_baseline.total_bags_unsold
              << " | " << std::setw(8) << metrics_smart.total_bags_unsold
              << " | " << std::setw(6) << (metrics_smart.total_bags_unsold - metrics_baseline.total_bags_unsold) << std::endl;
    std::cout << "Revenue Generated               | $" << std::fixed << std::setprecision(2) << std::setw(7) << metrics_baseline.total_revenue_generated
              << " | $" << std::setw(7) << metrics_smart.total_revenue_generated
              << " | $" << std::setw(6) << (metrics_smart.total_revenue_generated - metrics_baseline.total_revenue_generated) << std::endl;
    std::cout << "Revenue Lost                    | $" << std::setw(7) << metrics_baseline.total_revenue_lost
              << " | $" << std::setw(7) << metrics_smart.total_revenue_lost
              << " | $" << std::setw(6) << (metrics_smart.total_revenue_lost - metrics_baseline.total_revenue_lost) << std::endl;
    std::cout << "Customers Who Left              | " << std::setw(8) << metrics_baseline.customers_who_left
              << " | " << std::setw(8) << metrics_smart.customers_who_left
              << " | " << std::setw(6) << (metrics_smart.customers_who_left - metrics_baseline.customers_who_left) << std::endl;
    std::cout << "Gini Coefficient (Fairness)    | " << std::setprecision(4) << std::setw(8) << metrics_baseline.gini_coefficient_exposure
              << " | " << std::setw(8) << metrics_smart.gini_coefficient_exposure
              << " | " << std::setw(6) << (metrics_smart.gini_coefficient_exposure - metrics_baseline.gini_coefficient_exposure) << std::endl;
    std::cout << std::string(70, '=') << std::endl;

    return 0;
}

