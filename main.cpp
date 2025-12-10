#include <iostream>
#include <iomanip>
#include <fstream>
#include <vector>
#include <sstream>
#include "Restaurant.h"
#include "RestaurantLoader.h"
#include "SimulationEngine.h"
#include "RankingAlgorithms.h"

void write_comparison_report(const std::vector<std::pair<std::string, SimulationMetrics>>& all_metrics, 
                            const std::string& filename) {
    std::ofstream out(filename);
    
    out << "======================================================================\n";
    out << "FOOD WASTE MARKETPLACE SIMULATION - ALGORITHM COMPARISON REPORT\n";
    out << "======================================================================\n\n";
    
    out << "Simulation Period: 7 Days\n";
    out << "Customers per Day: 100\n";
    out << "Total Customers: 700\n\n";
    
    out << std::string(100, '=') << "\n";
    out << "OVERALL METRICS COMPARISON\n";
    out << std::string(100, '=') << "\n\n";
    
    // Header
    out << std::left << std::setw(35) << "Metric";
    for (const auto& pair : all_metrics) {
        out << std::setw(20) << pair.first;
    }
    out << "\n" << std::string(100, '-') << "\n";
    
    // Bags Sold
    out << std::left << std::setw(35) << "Bags Sold";
    for (const auto& pair : all_metrics) {
        out << std::setw(20) << pair.second.total_bags_sold;
    }
    out << "\n";
    
    // Bags Cancelled
    out << std::left << std::setw(35) << "Bags Cancelled";
    for (const auto& pair : all_metrics) {
        out << std::setw(20) << pair.second.total_bags_cancelled;
    }
    out << "\n";
    
    // Bags Unsold (Waste)
    out << std::left << std::setw(35) << "Bags Unsold (Waste)";
    for (const auto& pair : all_metrics) {
        out << std::setw(20) << pair.second.total_bags_unsold;
    }
    out << "\n";
    
    // Revenue Generated
    out << std::left << std::setw(35) << "Revenue Generated ($)";
    for (const auto& pair : all_metrics) {
        out << std::fixed << std::setprecision(2) << std::setw(20) << pair.second.total_revenue_generated;
    }
    out << "\n";
    
    // Revenue Lost
    out << std::left << std::setw(35) << "Revenue Lost ($)";
    for (const auto& pair : all_metrics) {
        out << std::fixed << std::setprecision(2) << std::setw(20) << pair.second.total_revenue_lost;
    }
    out << "\n";
    
    // Revenue Efficiency
    out << std::left << std::setw(35) << "Revenue Efficiency (%)";
    for (const auto& pair : all_metrics) {
        float efficiency = (pair.second.total_revenue_generated + pair.second.total_revenue_lost) > 0 ?
            (pair.second.total_revenue_generated / 
             (pair.second.total_revenue_generated + pair.second.total_revenue_lost)) * 100.0f : 0.0f;
        out << std::fixed << std::setprecision(2) << std::setw(20) << efficiency;
    }
    out << "\n";
    
    // Customers Who Left
    out << std::left << std::setw(35) << "Customers Who Left";
    for (const auto& pair : all_metrics) {
        out << std::setw(20) << pair.second.customers_who_left;
    }
    out << "\n";
    
    // Conversion Rate
    out << std::left << std::setw(35) << "Conversion Rate (%)";
    for (const auto& pair : all_metrics) {
        float conversion = pair.second.total_customer_arrivals > 0 ?
            ((float)(pair.second.total_customer_arrivals - pair.second.customers_who_left) / 
             pair.second.total_customer_arrivals) * 100.0f : 0.0f;
        out << std::fixed << std::setprecision(2) << std::setw(20) << conversion;
    }
    out << "\n";
    
    // Gini Coefficient
    out << std::left << std::setw(35) << "Gini Coefficient (Fairness)";
    for (const auto& pair : all_metrics) {
        out << std::fixed << std::setprecision(4) << std::setw(20) << pair.second.gini_coefficient_exposure;
    }
    out << "\n";
    out << std::string(100, '=') << "\n\n";
    
    // Detailed per-algorithm breakdown
    for (const auto& pair : all_metrics) {
        out << std::string(100, '=') << "\n";
        out << pair.first << " ALGORITHM - DETAILED METRICS\n";
        out << std::string(100, '=') << "\n\n";
        
        const auto& m = pair.second;
        
        out << "--- SALES METRICS ---\n";
        out << "Total Bags Sold: " << m.total_bags_sold << "\n";
        out << "Total Bags Cancelled: " << m.total_bags_cancelled << "\n";
        out << "Total Bags Unsold (Waste): " << m.total_bags_unsold << "\n\n";
        
        out << "--- REVENUE METRICS ---\n";
        out << "Total Revenue Generated: $" << std::fixed << std::setprecision(2) << m.total_revenue_generated << "\n";
        out << "Revenue Lost (from cancellations): $" << m.total_revenue_lost << "\n";
        float revenue_efficiency = (m.total_revenue_generated + m.total_revenue_lost) > 0 ? 
            (m.total_revenue_generated / (m.total_revenue_generated + m.total_revenue_lost)) * 100.0f : 0.0f;
        out << "Revenue Efficiency: " << std::fixed << std::setprecision(2) << revenue_efficiency << "%\n\n";
        
        out << "--- CUSTOMER METRICS ---\n";
        out << "Total Customer Arrivals: " << m.total_customer_arrivals << "\n";
        out << "Customers Who Left (No Purchase): " << m.customers_who_left << "\n";
        float conversion_rate = m.total_customer_arrivals > 0 ? 
            ((float)(m.total_customer_arrivals - m.customers_who_left) / m.total_customer_arrivals) * 100.0f : 0.0f;
        out << "Conversion Rate: " << std::fixed << std::setprecision(2) << conversion_rate << "%\n\n";
        
        out << "--- FAIRNESS METRICS ---\n";
        out << "Gini Coefficient (Exposure): " << std::fixed << std::setprecision(4) << m.gini_coefficient_exposure << "\n";
        out << "  (0 = perfect equality, 1 = maximum inequality)\n\n";
        
        out << "\n";
    }
    
    // Comparison table
    out << std::string(100, '=') << "\n";
    out << "ALGORITHM COMPARISON TABLE (vs BASELINE)\n";
    out << std::string(100, '=') << "\n\n";
    
    if (!all_metrics.empty()) {
        const auto& baseline = all_metrics[0].second;
        
        out << std::left << std::setw(35) << "Metric";
        out << std::setw(20) << "Baseline";
        for (size_t i = 1; i < all_metrics.size(); i++) {
            out << std::setw(20) << all_metrics[i].first;
        }
        out << "\n" << std::string(100, '-') << "\n";
        
        // Bags Sold
        out << std::left << std::setw(35) << "Bags Sold";
        out << std::setw(20) << baseline.total_bags_sold;
        for (size_t i = 1; i < all_metrics.size(); i++) {
            int diff = all_metrics[i].second.total_bags_sold - baseline.total_bags_sold;
            out << std::setw(20) << (diff >= 0 ? "+" : "") << diff;
        }
        out << "\n";
        
        // Waste
        out << std::left << std::setw(35) << "Waste Reduction";
        out << std::setw(20) << baseline.total_bags_unsold;
        for (size_t i = 1; i < all_metrics.size(); i++) {
            int diff = baseline.total_bags_unsold - all_metrics[i].second.total_bags_unsold;
            out << std::setw(20) << (diff >= 0 ? "+" : "") << diff;
        }
        out << "\n";
        
        // Revenue
        out << std::left << std::setw(35) << "Revenue Increase ($)";
        out << std::fixed << std::setprecision(2) << std::setw(20) << baseline.total_revenue_generated;
        for (size_t i = 1; i < all_metrics.size(); i++) {
            float diff = all_metrics[i].second.total_revenue_generated - baseline.total_revenue_generated;
            out << std::setw(20) << (diff >= 0 ? "+" : "") << diff;
        }
        out << "\n";
        
        // Customers Who Left
        out << std::left << std::setw(35) << "Customers Retained";
        out << std::setw(20) << (baseline.total_customer_arrivals - baseline.customers_who_left);
        for (size_t i = 1; i < all_metrics.size(); i++) {
            int diff = (baseline.customers_who_left - all_metrics[i].second.customers_who_left);
            out << std::setw(20) << (diff >= 0 ? "+" : "") << diff;
        }
        out << "\n";
        
        // Fairness
        out << std::left << std::setw(35) << "Fairness Improvement";
        out << std::fixed << std::setprecision(4) << std::setw(20) << baseline.gini_coefficient_exposure;
        for (size_t i = 1; i < all_metrics.size(); i++) {
            float diff = baseline.gini_coefficient_exposure - all_metrics[i].second.gini_coefficient_exposure;
            out << std::setw(20) << (diff >= 0 ? "+" : "") << std::fixed << std::setprecision(4) << diff;
        }
        out << "\n";
    }
    
    out << std::string(100, '=') << "\n";
    out.close();
}

int main() {
    std::cout << "=== Food Waste Marketplace Simulation ===" << std::endl;
    std::cout << "Running all ranking algorithms..." << std::endl;
    std::cout << "Detailed logs: detailed_simulation_log.txt" << std::endl;
    std::cout << "Comparison report: algorithm_comparison_report.txt" << std::endl;

    // Load restaurants from CSV file, or use default if not available
    std::vector<Restaurant> restaurants;
    if (!RestaurantLoader::load_restaurants_from_csv("stores.csv", restaurants)) {
        std::cout << "Using default restaurants..." << std::endl;
        RestaurantLoader::generate_default_restaurants(restaurants);
    }

    std::vector<std::pair<std::string, RankingAlgorithm>> algorithms = {
        {"BASELINE", RankingAlgorithm::BASELINE},
        {"SAMA", RankingAlgorithm::SAMA},
        {"ANDREW", RankingAlgorithm::ANDREW},
        {"AMER", RankingAlgorithm::AMER},
        {"ZIAD", RankingAlgorithm::ZIAD}
    };

    std::vector<std::pair<std::string, SimulationMetrics>> all_metrics;

    // Generate customers and arrival times ONCE for fair comparison
    // All algorithms will use the exact same customers and arrival times
    std::cout << "Generating customers and arrival times (shared across all algorithms)..." << std::endl;
    
    ArrivalGenerator shared_generator(12345);  // Fixed seed for reproducibility
    if (!shared_generator.load_customers_from_csv("customer.csv")) {
        std::cout << "No customer CSV found, generating random customers..." << std::endl;
    }
    
    // Pre-generate customer pool (2x daily amount to account for churn)
    std::vector<Customer> shared_customer_pool;
    for (int i = 0; i < 100 * 2; i++) {
        Customer customer = shared_generator.generate_customer(i, restaurants);
        shared_customer_pool.push_back(customer);
    }
    
    // Pre-generate arrival times for each day
    std::vector<std::vector<Timestamp>> shared_arrival_times;
    for (int day = 0; day < 7; day++) {
        std::vector<Timestamp> day_times = shared_generator.generate_arrival_times(100);
        shared_arrival_times.push_back(day_times);
    }
    
    std::cout << "Generated " << shared_customer_pool.size() << " customers and " 
              << shared_arrival_times.size() << " days of arrival times." << std::endl;

    // Open output file for detailed simulation logs
    std::ofstream detailed_log("detailed_simulation_log.txt");
    
    // Run simulation for each algorithm
    for (const auto& algo_pair : algorithms) {
        std::cout << "Running " << algo_pair.first << " algorithm..." << std::endl;
        
        detailed_log << "\n" << std::string(100, '=') << "\n";
        detailed_log << "SIMULATION: " << algo_pair.first << " ALGORITHM\n";
        detailed_log << std::string(100, '=') << "\n";
        
        // Create engine without loading CSV (we'll use pre-generated customers)
        SimulationEngine engine(5, "", algo_pair.second);  // Empty CSV path to skip loading
        engine.initialize(restaurants);
        engine.set_output_stream(&detailed_log);  // Redirect output to file
        
        // Set shared customers and arrival times for fair comparison
        // This ensures ALL algorithms use the exact same customers and arrival times
        engine.set_customer_pool(shared_customer_pool);
        engine.set_arrival_times(shared_arrival_times);
        
        engine.run_multi_day_simulation(7, 100);
        
        // Print summary to file
        detailed_log << "\n" << std::string(100, '=') << "\n";
        detailed_log << algo_pair.first << " ALGORITHM RESULTS\n";
        detailed_log << std::string(100, '=') << "\n";
        engine.get_metrics().print_summary_to_stream(detailed_log);
        
        // Export individual CSV
        std::string csv_filename = "simulation_results_" + algo_pair.first + ".csv";
        engine.export_results(csv_filename);
        
        // Store metrics for comparison
        all_metrics.push_back({algo_pair.first, engine.get_metrics()});
        
        std::cout << "Completed " << algo_pair.first << " algorithm." << std::endl;
    }
    
    detailed_log.close();

    // Write comparison report
    write_comparison_report(all_metrics, "algorithm_comparison_report.txt");
    
    std::cout << "\n========================================" << std::endl;
    std::cout << "All simulations completed!" << std::endl;
    std::cout << "Results saved to: algorithm_comparison_report.txt" << std::endl;
    std::cout << "Individual CSV files saved for each algorithm." << std::endl;
    std::cout << "========================================" << std::endl;

    return 0;
}
