#include <iostream>
#include <iomanip>
#include <fstream>
#include <vector>
#include <sstream>
#include "Restaurant.h"
#include "RestaurantLoader.h"
#include "SimulationEngine.h"
#include "RankingAlgorithms.h"

using namespace std;

// Generate a detailed comparison report
void write_comparison_report(const vector<pair<string, SimulationMetrics>>& all_metrics, 
                            const string& filename) {
    ofstream out(filename);
    
    out << "======================================================================\n";
    out << "FOOD WASTE MARKETPLACE SIMULATION - ALGORITHM COMPARISON REPORT\n";
    out << "======================================================================\n\n";
    
    out << "Simulation Period: 7 Days\n";
    out << "Customers per Day: 100\n";
    out << "Total Customers: 700\n\n";
    
    out << string(100, '=') << "\n";
    out << "OVERALL METRICS COMPARISON\n";
    out << string(100, '=') << "\n\n";
    
    // Header
    out << left << setw(35) << "Metric";
    for (const auto& pair : all_metrics) {
        out << setw(20) << pair.first;
    }
    out << "\n" << string(100, '-') << "\n";
    
    // Bags Sold
    out << left << setw(35) << "Bags Sold";
    for (const auto& pair : all_metrics) {
        out << setw(20) << pair.second.total_bags_sold;
    }
    out << "\n";
    
    // Bags Cancelled
    out << left << setw(35) << "Bags Cancelled";
    for (const auto& pair : all_metrics) {
        out << setw(20) << pair.second.total_bags_cancelled;
    }
    out << "\n";
    
    // Bags Unsold (Waste)
    out << left << setw(35) << "Bags Unsold (Waste)";
    for (const auto& pair : all_metrics) {
        out << setw(20) << pair.second.total_bags_unsold;
    }
    out << "\n";
    
    // Revenue Generated
    out << left << setw(35) << "Revenue Generated ($)";
    for (const auto& pair : all_metrics) {
        out << fixed << setprecision(2) << setw(20) << pair.second.total_revenue_generated;
    }
    out << "\n";
    
    // Revenue Lost
    out << left << setw(35) << "Revenue Lost ($)";
    for (const auto& pair : all_metrics) {
        out << fixed << setprecision(2) << setw(20) << pair.second.total_revenue_lost;
    }
    out << "\n";
    
    // Revenue Efficiency
    out << left << setw(35) << "Revenue Efficiency (%)";
    for (const auto& pair : all_metrics) {
        float efficiency = (pair.second.total_revenue_generated + pair.second.total_revenue_lost) > 0 ?
            (pair.second.total_revenue_generated / 
             (pair.second.total_revenue_generated + pair.second.total_revenue_lost)) * 100.0f : 0.0f;
        out << fixed << setprecision(2) << setw(20) << efficiency;
    }
    out << "\n";
    
    // Customers Who Left
    out << left << setw(35) << "Customers Who Left";
    for (const auto& pair : all_metrics) {
        out << setw(20) << pair.second.customers_who_left;
    }
    out << "\n";
    
    // Conversion Rate
    out << left << setw(35) << "Conversion Rate (%)";
    for (const auto& pair : all_metrics) {
        float conversion = pair.second.total_customer_arrivals > 0 ?
            ((float)(pair.second.total_customer_arrivals - pair.second.customers_who_left) / 
             pair.second.total_customer_arrivals) * 100.0f : 0.0f;
        out << fixed << setprecision(2) << setw(20) << conversion;
    }
    out << "\n";
    
    // Gini Coefficient
    out << left << setw(35) << "Gini Coefficient (Fairness)";
    for (const auto& pair : all_metrics) {
        out << fixed << setprecision(4) << setw(20) << pair.second.gini_coefficient_exposure;
    }
    out << "\n";
    out << string(100, '=') << "\n\n";
    
    // Detailed per-algorithm breakdown
    for (const auto& pair : all_metrics) {
        out << string(100, '=') << "\n";
        out << pair.first << " ALGORITHM - DETAILED METRICS\n";
        out << string(100, '=') << "\n\n";
        
        const auto& m = pair.second;
        
        out << "--- SALES METRICS ---\n";
        out << "Total Bags Sold: " << m.total_bags_sold << "\n";
        out << "Total Bags Cancelled: " << m.total_bags_cancelled << "\n";
        out << "Total Bags Unsold (Waste): " << m.total_bags_unsold << "\n\n";
        
        out << "--- REVENUE METRICS ---\n";
        out << "Total Revenue Generated: $" << fixed << setprecision(2) << m.total_revenue_generated << "\n";
        out << "Revenue Lost (from cancellations): $" << m.total_revenue_lost << "\n";
        float revenue_efficiency = (m.total_revenue_generated + m.total_revenue_lost) > 0 ? 
            (m.total_revenue_generated / (m.total_revenue_generated + m.total_revenue_lost)) * 100.0f : 0.0f;
        out << "Revenue Efficiency: " << fixed << setprecision(2) << revenue_efficiency << "%\n\n";
        
        out << "--- CUSTOMER METRICS ---\n";
        out << "Total Customer Arrivals: " << m.total_customer_arrivals << "\n";
        out << "Customers Who Left (No Purchase): " << m.customers_who_left << "\n";
        float conversion_rate = m.total_customer_arrivals > 0 ? 
            ((float)(m.total_customer_arrivals - m.customers_who_left) / m.total_customer_arrivals) * 100.0f : 0.0f;
        out << "Conversion Rate: " << fixed << setprecision(2) << conversion_rate << "%\n\n";
        
        out << "--- FAIRNESS METRICS ---\n";
        out << "Gini Coefficient (Exposure): " << fixed << setprecision(4) << m.gini_coefficient_exposure << "\n";
        out << "  (0 = perfect equality, 1 = maximum inequality)\n\n";
        
        out << "\n";
    }
    
    // Comparison table
    out << string(100, '=') << "\n";
    out << "ALGORITHM COMPARISON TABLE (vs BASELINE)\n";
    out << string(100, '=') << "\n\n";
    
    if (!all_metrics.empty()) {
        const auto& baseline = all_metrics[0].second;
        
        out << left << setw(35) << "Metric";
        out << setw(20) << "Baseline";
        for (size_t i = 1; i < all_metrics.size(); i++) {
            out << setw(20) << all_metrics[i].first;
        }
        out << "\n" << string(100, '-') << "\n";
        
        // Bags Sold
        out << left << setw(35) << "Bags Sold";
        out << setw(20) << baseline.total_bags_sold;
        for (size_t i = 1; i < all_metrics.size(); i++) {
            int diff = all_metrics[i].second.total_bags_sold - baseline.total_bags_sold;
            out << setw(20) << (diff >= 0 ? "+" : "") << diff;
        }
        out << "\n";
        
        // Waste
        out << left << setw(35) << "Waste Reduction";
        out << setw(20) << baseline.total_bags_unsold;
        for (size_t i = 1; i < all_metrics.size(); i++) {
            // Positive diff means LESS waste (improvement)
            int diff = baseline.total_bags_unsold - all_metrics[i].second.total_bags_unsold;
            out << setw(20) << (diff >= 0 ? "+" : "") << diff;
        }
        out << "\n";
        
        // Revenue
        out << left << setw(35) << "Revenue Increase ($)";
        out << fixed << setprecision(2) << setw(20) << baseline.total_revenue_generated;
        for (size_t i = 1; i < all_metrics.size(); i++) {
            float diff = all_metrics[i].second.total_revenue_generated - baseline.total_revenue_generated;
            out << setw(20) << (diff >= 0 ? "+" : "") << diff;
        }
        out << "\n";
        
        // Customers Who Left
        out << left << setw(35) << "Customers Retained";
        out << setw(20) << (baseline.total_customer_arrivals - baseline.customers_who_left);
        for (size_t i = 1; i < all_metrics.size(); i++) {
            // Diff in retained customers (higher is better)
            int retained_baseline = baseline.total_customer_arrivals - baseline.customers_who_left;
            int retained_algo = all_metrics[i].second.total_customer_arrivals - all_metrics[i].second.customers_who_left;
            int diff = retained_algo - retained_baseline;
            out << setw(20) << (diff >= 0 ? "+" : "") << diff;
        }
        out << "\n";
        
        // Fairness
        out << left << setw(35) << "Fairness Improvement";
        out << fixed << setprecision(4) << setw(20) << baseline.gini_coefficient_exposure;
        for (size_t i = 1; i < all_metrics.size(); i++) {
            // Lower Gini is better so +diff means better
            float diff = baseline.gini_coefficient_exposure - all_metrics[i].second.gini_coefficient_exposure;
            out << setw(20) << (diff >= 0 ? "+" : "") << fixed << setprecision(4) << diff;
        }
        out << "\n";
    }
    
    out << string(100, '=') << "\n";
    out.close();
}

int main() {
    cout << "=== Food Waste Marketplace Simulation ===" << endl;
    cout << "Running all ranking algorithms..." << endl;
    cout << "Detailed logs: detailed_simulation_log.txt" << endl;
    cout << "Comparison report: algorithm_comparison_report.txt" << endl;

    // Load or generate restaurants
    vector<Restaurant> restaurants;
    if (!RestaurantLoader::load_restaurants_from_csv("stores.csv", restaurants)) {
        cout << "Using default restaurants..." << endl;
        RestaurantLoader::generate_default_restaurants(restaurants);
    }

    vector<pair<string, RankingAlgorithm>> algorithms = {
        {"BASELINE", RankingAlgorithm::BASELINE},
        {"SAMA", RankingAlgorithm::SAMA},
        {"ANDREW", RankingAlgorithm::ANDREW},
        {"AMER", RankingAlgorithm::AMER},
        {"ZIAD", RankingAlgorithm::ZIAD},
        {"HARMONY", RankingAlgorithm::HARMONY}
    };

    vector<pair<string, SimulationMetrics>> all_metrics;

    // Generate shared simulation data (Customers & Arrivals)
    // Ensures fair comparison across all algorithms
    cout << "Generating customers and arrival times (shared across all algorithms)..." << endl;
    
    ArrivalGenerator shared_generator(12345);
    if (!shared_generator.load_customers_from_csv("customer.csv")) {
        cout << "No customer CSV found, generating random customers..." << endl;
    }
    
    // Customer Pool (Account for 7 days + churn)
    vector<Customer> shared_customer_pool;
    for (int i = 0; i < 100 * 2; i++) {
        Customer customer = shared_generator.generate_customer(i, restaurants);
        shared_customer_pool.push_back(customer);
    }
    
    // Arrival Times (7 Days)
    vector<vector<Timestamp>> shared_arrival_times;
    for (int day = 0; day < 7; day++) {
        vector<Timestamp> day_times = shared_generator.generate_arrival_times(100);
        shared_arrival_times.push_back(day_times);
    }
    
    cout << "Generated " << shared_customer_pool.size() << " customers and " 
              << shared_arrival_times.size() << " days of arrival times." << endl;

    // Simulation Loop
    ofstream detailed_log("detailed_simulation_log.txt");
    
    for (const auto& algo_pair : algorithms) {
        cout << "Running " << algo_pair.first << " algorithm..." << endl;
        
        detailed_log << "\n" << string(100, '=') << "\n";
        detailed_log << "SIMULATION: " << algo_pair.first << " ALGORITHM\n";
        detailed_log << string(100, '=') << "\n";
        
        SimulationEngine engine(5, "", algo_pair.second);
        engine.initialize(restaurants);
        engine.set_output_stream(&detailed_log);
        
        // Inject shared data
        engine.set_customer_pool(shared_customer_pool);
        engine.set_arrival_times(shared_arrival_times);
        
        engine.run_multi_day_simulation(7, 100);
        
        // Log results
        detailed_log << "\n" << string(100, '=') << "\n";
        detailed_log << algo_pair.first << " ALGORITHM RESULTS\n";
        detailed_log << string(100, '=') << "\n";
        engine.get_metrics().print_summary_to_stream(detailed_log);
        
        // Export CSV
        string csv_filename = "simulation_results_" + algo_pair.first + ".csv";
        engine.export_results(csv_filename);
        
        all_metrics.push_back({algo_pair.first, engine.get_metrics()});
        
        cout << "Completed " << algo_pair.first << " algorithm." << endl;
    }
    
    detailed_log.close();

    // Final Report
    write_comparison_report(all_metrics, "algorithm_comparison_report.txt");
    
    cout << "\n========================================" << endl;
    cout << "All simulations completed!" << endl;
    cout << "Results saved to: algorithm_comparison_report.txt" << endl;
    cout << "Individual CSV files saved for each algorithm." << endl;
    cout << "========================================" << endl;

    return 0;
}
