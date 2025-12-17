#include "SimulationEngine.h"
#include "CustomerDecisionSystem.h"
#include "RestaurantManagementSystem.h"
#include "RankingAlgorithms.h"
#include <iostream>
#include <iomanip>
#include <fstream>
#include <ctime>
#include <random>
#include <algorithm>

using namespace std;

SimulationEngine::SimulationEngine(int n_display, const string& customer_csv, 
                                   RankingAlgorithm algorithm)
    : n_displayed(n_display),
      // Use fixed seed if CSV is empty (for reproducibility)
      arrival_generator(customer_csv, customer_csv.empty() ? 12345 : time(nullptr)),
      ranking_algorithm(algorithm),
      next_customer_id(0),
      output_stream(&cout),
      use_pre_generated_data(false) {}

void SimulationEngine::initialize(const vector<Restaurant>& restaurants) {
    market_state.restaurants = restaurants;

    mt19937 rng(time(nullptr));
    uniform_real_distribution<float> variance(0.8f, 1.2f);

    for (auto& restaurant : market_state.restaurants) {
        int actual = (int)(restaurant.estimated_bags * variance(rng));
        restaurant.set_actual_inventory(max(0, actual));
    }
}

void SimulationEngine::run_day_simulation(int num_customers, bool use_customer_pool, int day_index) {
    string algo_name = "BASELINE";
    if (ranking_algorithm == RankingAlgorithm::SAMA) algo_name = "SAMA";
    else if (ranking_algorithm == RankingAlgorithm::ANDREW) algo_name = "ANDREW";
    else if (ranking_algorithm == RankingAlgorithm::AMER) algo_name = "AMER";
    else if (ranking_algorithm == RankingAlgorithm::ZIAD) algo_name = "ZIAD";
    else if (ranking_algorithm == RankingAlgorithm::HARMONY) algo_name = "HARMONY";
    
    *output_stream << "\n=== Starting Day Simulation (" << algo_name << " Algorithm) ===" << endl;
    *output_stream << "Number of customers: " << num_customers << endl;
    *output_stream << "Number of stores: " << market_state.restaurants.size() << endl;

    *output_stream << "\nInitial Store Inventory:" << endl;
    for (const auto& r : market_state.restaurants) {
        *output_stream << r.business_name << ": Estimated=" << r.estimated_bags
            << ", Actual=" << r.actual_bags
            << ", Price=$" << r.price_per_bag
            << ", Rating=" << fixed << setprecision(2) << r.get_rating() << endl;
    }

    // Use pre-generated arrival times if available
    vector<Timestamp> arrival_times;
    if (use_pre_generated_data && day_index >= 0 && day_index < (int)pre_generated_arrival_times.size()) {
        arrival_times = pre_generated_arrival_times[day_index];
    } else {
        arrival_times = arrival_generator.generate_arrival_times(num_customers);
    }

    int successful_reservations = 0;
    int active_customer_index = 0;
    
    for (int i = 0; i < num_customers; i++) {
        Customer customer;
        
        // Always use pre-generated customers if available (for fair comparison)
        if (use_pre_generated_data && active_customer_index < (int)customer_pool.size()) {
            customer = customer_pool[active_customer_index];
            active_customer_index++;
        } else if (use_customer_pool && active_customer_index < (int)customer_pool.size()) {
            customer = customer_pool[active_customer_index];
            active_customer_index++;
        } else {
            // Fallback: generate new customer
            customer = arrival_generator.generate_customer(next_customer_id++, market_state.restaurants);
            if (use_customer_pool) {
                customer_pool.push_back(customer);
            }
        }
        
        market_state.current_time = arrival_times[i];

        metrics_collector.log_customer_arrival(customer.id, arrival_times[i]);

        vector<int> displayed = get_displayed_stores(customer, market_state, n_displayed, ranking_algorithm);
        metrics_collector.log_stores_displayed(displayed);

        market_state.customers.insert(make_pair(customer.id, customer));

        int selected = CustomerDecisionSystem::process_customer_arrival(
            market_state.customers[customer.id], market_state, n_displayed, ranking_algorithm);

        if (selected == -1) {
            metrics_collector.log_customer_left(customer.id);
        }
        else {
            successful_reservations++;
        }
    }

    *output_stream << "\nTotal Reservations Made: " << successful_reservations << endl;
    *output_stream << "Processing end of day..." << endl;
    RestaurantManagementSystem::process_end_of_day(market_state);

    metrics_collector.log_end_of_day(market_state);
    metrics_collector.calculate_fairness_metrics(market_state);
    
    *output_stream << "\n=== RATING CHANGES (Dynamic Ratings) ===" << endl;
    for (const auto& r : market_state.restaurants) {
        float rating_change = r.get_rating() - r.rating_at_day_start;
        *output_stream << r.business_name << ": " << fixed << setprecision(2) 
                  << r.rating_at_day_start << " -> " << r.get_rating() 
                  << " (" << (rating_change >= 0 ? "+" : "") << rating_change << ")"
                  << " [Confirmed: " << r.daily_orders_confirmed 
                  << ", Cancelled: " << r.daily_orders_cancelled << "]" << endl;
    }
}

void SimulationEngine::run_multi_day_simulation(int num_days, int num_customers_per_day) {
    string algo_name = "BASELINE";
    if (ranking_algorithm == RankingAlgorithm::SAMA) algo_name = "SAMA";
    else if (ranking_algorithm == RankingAlgorithm::ANDREW) algo_name = "ANDREW";
    else if (ranking_algorithm == RankingAlgorithm::AMER) algo_name = "AMER";
    else if (ranking_algorithm == RankingAlgorithm::ZIAD) algo_name = "ZIAD";
    
    *output_stream << "\n" << string(70, '=') << endl;
    *output_stream << "=== Starting " << num_days << "-Day Simulation (" << algo_name << " Algorithm) ===" << endl;
    *output_stream << "Number of customers per day: " << num_customers_per_day << endl;
    *output_stream << "Number of stores: " << market_state.restaurants.size() << endl;
    *output_stream << string(70, '=') << endl;

    // Store initial ratings
    for (auto& r : market_state.restaurants) {
        r.initial_rating = r.general_ranking;
    }
    
    // Reset impression counts
    market_state.impression_counts.clear();

    SimulationMetrics aggregated_metrics;
    
    // Initialize customer pool
    if (use_pre_generated_data && !pre_generated_customers.empty()) {
        // Reset to fresh copy of pre-generated customers
        customer_pool.clear();
        for (const auto& customer : pre_generated_customers) {
            Customer fresh_customer = customer;
            fresh_customer.churned = false;
            fresh_customer.history = CustomerHistory(); 
            fresh_customer.loyalty = 0.8f; 
            customer_pool.push_back(fresh_customer);
        }
        next_customer_id = pre_generated_customers.size();
    } else if (customer_pool.empty()) {
        // Pre-generate a pool
        for (int i = 0; i < num_customers_per_day * 2; i++) {
            Customer customer = arrival_generator.generate_customer(next_customer_id++, market_state.restaurants);
            customer_pool.push_back(customer);
        }
    }
    
    for (int day = 1; day <= num_days; day++) {
        *output_stream << "\n" << string(70, '-') << endl;
        *output_stream << "DAY " << day << " of " << num_days << endl;
        *output_stream << string(70, '-') << endl;

        // Reset daily state
        market_state.reservations.clear();
        market_state.current_time = Timestamp(8, 0);
        market_state.next_reservation_id = 1;

        // Filter out churned customers
        vector<Customer> active_customers;
        for (const auto& customer : customer_pool) {
            if (!customer.churned) {
                active_customers.push_back(customer);
            }
        }
        
        // Replenish customer pool if needed
        while ((int)active_customers.size() < num_customers_per_day) {
            Customer new_customer;
            if (use_pre_generated_data && next_customer_id < (int)pre_generated_customers.size()) {
                new_customer = pre_generated_customers[next_customer_id];
                new_customer.id = next_customer_id;
                new_customer.churned = false;
                new_customer.history = CustomerHistory();
                new_customer.loyalty = 0.8f;
            } else {
                new_customer = arrival_generator.generate_customer(next_customer_id, market_state.restaurants);
            }
            active_customers.push_back(new_customer);
            next_customer_id++;
        }
        
        customer_pool = active_customers;
        
        // Reset restaurant daily state
        for (auto& restaurant : market_state.restaurants) {
            restaurant.rating_at_day_start = restaurant.general_ranking;
            restaurant.daily_orders_confirmed = 0;
            restaurant.daily_orders_cancelled = 0;
            restaurant.reserved_count = 0;
            restaurant.has_inventory = true;
            
            // Randomize daily inventory
            mt19937 rng(time(nullptr) + day);
            uniform_real_distribution<float> variance(0.8f, 1.2f);
            int actual = (int)(restaurant.estimated_bags * variance(rng));
            restaurant.set_actual_inventory(max(0, actual));
        }
        
        metrics_collector.metrics = SimulationMetrics();

        // Run daily simulation
        run_day_simulation(num_customers_per_day, true, day - 1);
        
        // Update customer pool with outcomes
        for (auto& pool_customer : customer_pool) {
            auto it = market_state.customers.find(pool_customer.id);
            if (it != market_state.customers.end()) {
                pool_customer.history = it->second.history;
                pool_customer.loyalty = it->second.loyalty;
                pool_customer.churned = it->second.churned;
                pool_customer.category_preference = it->second.category_preference;
            }
        }
        
        market_state.customers.clear();

        // Aggregate metrics
        const auto& day_metrics = metrics_collector.metrics;
        aggregated_metrics.total_bags_sold += day_metrics.total_bags_sold;
        aggregated_metrics.total_bags_cancelled += day_metrics.total_bags_cancelled;
        aggregated_metrics.total_bags_unsold += day_metrics.total_bags_unsold;
        aggregated_metrics.total_revenue_generated += day_metrics.total_revenue_generated;
        aggregated_metrics.total_revenue_lost += day_metrics.total_revenue_lost;
        aggregated_metrics.customers_who_left += day_metrics.customers_who_left;
        aggregated_metrics.total_customer_arrivals += day_metrics.total_customer_arrivals;

        // Aggregate per-store metrics
        for (const auto& pair : day_metrics.bags_sold_per_store) {
            aggregated_metrics.bags_sold_per_store[pair.first] += pair.second;
        }
        for (const auto& pair : day_metrics.bags_cancelled_per_store) {
            aggregated_metrics.bags_cancelled_per_store[pair.first] += pair.second;
        }
        for (const auto& pair : day_metrics.waste_per_store) {
            aggregated_metrics.waste_per_store[pair.first] += pair.second;
        }
        for (const auto& pair : day_metrics.revenue_per_store) {
            aggregated_metrics.revenue_per_store[pair.first] += pair.second;
        }
        for (const auto& pair : day_metrics.times_displayed_per_store) {
            aggregated_metrics.times_displayed_per_store[pair.first] += pair.second;
        }

        *output_stream << "\nDay " << day << " Summary:" << endl;
        *output_stream << "  Bags Sold: " << day_metrics.total_bags_sold << endl;
        *output_stream << "  Waste: " << day_metrics.total_bags_unsold << endl;
        *output_stream << "  Revenue: $" << fixed << setprecision(2) << day_metrics.total_revenue_generated << endl;
    }

    // Calculate final fairness metric (Gini)
    vector<int> exposures;
    for (const auto& restaurant : market_state.restaurants) {
        exposures.push_back(aggregated_metrics.times_displayed_per_store[restaurant.business_id]);
    }
    if (!exposures.empty()) {
        sort(exposures.begin(), exposures.end());
        float sum = 0.0f;
        float weighted_sum = 0.0f;
        for (size_t i = 0; i < exposures.size(); i++) {
            sum += exposures[i];
            weighted_sum += exposures[i] * (i + 1);
        }
        if (sum > 0) {
            int n = exposures.size();
            aggregated_metrics.gini_coefficient_exposure = (2.0f * weighted_sum) / (n * sum) - (n + 1.0f) / n;
        }
    }

    metrics_collector.metrics = aggregated_metrics;

    *output_stream << "\n" << string(70, '=') << endl;
    *output_stream << "=== " << num_days << "-DAY SIMULATION COMPLETE ===" << endl;
    *output_stream << string(70, '=') << endl;
}

const SimulationMetrics& SimulationEngine::get_metrics() const {
    return metrics_collector.metrics;
}

void SimulationEngine::export_results(const string& filename) {
    string algo_name = "BASELINE";
    if (ranking_algorithm == RankingAlgorithm::SAMA) algo_name = "SAMA";
    else if (ranking_algorithm == RankingAlgorithm::ANDREW) algo_name = "ANDREW";
    else if (ranking_algorithm == RankingAlgorithm::AMER) algo_name = "AMER";
    else if (ranking_algorithm == RankingAlgorithm::ZIAD) algo_name = "ZIAD";
    
    ofstream out(filename);
    out << "Algorithm," << algo_name << "\n";
    out << "Restaurant,Estimated,Actual,Reserved,Sold,Cancelled,Waste,Revenue,Exposures\n";

    for (const auto& restaurant : market_state.restaurants) {
        int reserved = 0;
        for (const auto& res : market_state.reservations) {
            if (res.restaurant_id == restaurant.business_id) {
                reserved++;
            }
        }

        out << restaurant.business_name << ","
            << restaurant.estimated_bags << ","
            << restaurant.actual_bags << ","
            << reserved << ","
            << metrics_collector.metrics.bags_sold_per_store[restaurant.business_id] << ","
            << metrics_collector.metrics.bags_cancelled_per_store[restaurant.business_id] << ","
            << metrics_collector.metrics.waste_per_store[restaurant.business_id] << ","
            << fixed << setprecision(2) 
            << metrics_collector.metrics.revenue_per_store[restaurant.business_id] << ","
            << metrics_collector.metrics.times_displayed_per_store[restaurant.business_id] << "\n";
    }

    out.close();
}

void SimulationEngine::log_detailed_metrics(const SimulationMetrics* comparison_metrics) {
    ofstream log("simulation_log.txt", ios::app);
    string algo_name = "BASELINE";
    if (ranking_algorithm == RankingAlgorithm::SAMA) algo_name = "SAMA";
    else if (ranking_algorithm == RankingAlgorithm::ANDREW) algo_name = "ANDREW";
    else if (ranking_algorithm == RankingAlgorithm::AMER) algo_name = "AMER";
    else if (ranking_algorithm == RankingAlgorithm::ZIAD) algo_name = "ZIAD";
    
    log << "\n" << string(70, '=') << "\n";
    log << "=== DETAILED SIMULATION LOG - " << algo_name << " ALGORITHM ===\n";
    log << string(70, '=') << "\n\n";
    log << "Timestamp: " << time(nullptr) << "\n\n";
    
    log << "--- Overall Metrics ---\n";
    log << "Total Bags Sold: " << metrics_collector.metrics.total_bags_sold << "\n";
    log << "Total Bags Cancelled: " << metrics_collector.metrics.total_bags_cancelled << "\n";
    log << "Total Bags Unsold (Waste): " << metrics_collector.metrics.total_bags_unsold << "\n";
    log << "Total Revenue Generated: $" << fixed << setprecision(2) 
        << metrics_collector.metrics.total_revenue_generated << "\n";
    log << "Revenue Lost: $" << metrics_collector.metrics.total_revenue_lost << "\n";
    float revenue_efficiency = metrics_collector.metrics.total_customer_arrivals > 0 ? 
        (metrics_collector.metrics.total_revenue_generated / 
         (metrics_collector.metrics.total_revenue_generated + metrics_collector.metrics.total_revenue_lost)) * 100.0f : 0.0f;
    log << "Revenue Efficiency: " << fixed << setprecision(2) << revenue_efficiency << "%\n";
    log << "Customers Arrived: " << metrics_collector.metrics.total_customer_arrivals << "\n";
    log << "Customers Who Left: " << metrics_collector.metrics.customers_who_left << "\n";
    float conversion_rate = metrics_collector.metrics.total_customer_arrivals > 0 ? 
        ((float)(metrics_collector.metrics.total_customer_arrivals - metrics_collector.metrics.customers_who_left) / 
         metrics_collector.metrics.total_customer_arrivals) * 100.0f : 0.0f;
    log << "Conversion Rate: " << fixed << setprecision(2) << conversion_rate << "%\n";
    log << "Gini Coefficient (Fairness): " << fixed << setprecision(4) 
        << metrics_collector.metrics.gini_coefficient_exposure << "\n";
    log << "  (0 = perfect equality, 1 = maximum inequality)\n\n";
    
    log << "--- Per-Store Metrics ---\n";
    for (const auto& restaurant : market_state.restaurants) {
        log << "\n" << restaurant.business_name << " (ID: " << restaurant.business_id << "):\n";
        log << "  Initial Rating: " << fixed << setprecision(2) << restaurant.initial_rating << "\n";
        log << "  Final Rating: " << fixed << setprecision(2) << restaurant.get_rating() << "\n";
        log << "  Rating Change: " << fixed << setprecision(2) 
            << (restaurant.get_rating() - restaurant.initial_rating) << "\n";
        log << "  Orders Confirmed: " << restaurant.total_orders_confirmed << "\n";
        log << "  Orders Cancelled: " << restaurant.total_orders_cancelled << "\n";
        log << "  Estimated Bags: " << restaurant.estimated_bags << "\n";
        log << "  Actual Bags: " << restaurant.actual_bags << "\n";
        log << "  Bags Sold: " << metrics_collector.metrics.bags_sold_per_store[restaurant.business_id] << "\n";
        log << "  Bags Cancelled: " << metrics_collector.metrics.bags_cancelled_per_store[restaurant.business_id] << "\n";
        log << "  Waste: " << metrics_collector.metrics.waste_per_store[restaurant.business_id] << "\n";
        log << "  Revenue: $" << fixed << setprecision(2) 
            << metrics_collector.metrics.revenue_per_store[restaurant.business_id] << "\n";
        log << "  Times Displayed: " << metrics_collector.metrics.times_displayed_per_store[restaurant.business_id] << "\n";
    }
    
    if (comparison_metrics) {
        log << "\n--- Algorithm Comparison ---\n";
        log << "Bags Sold: " << metrics_collector.metrics.total_bags_sold 
            << " vs " << comparison_metrics->total_bags_sold << "\n";
        log << "Waste: " << metrics_collector.metrics.total_bags_unsold 
            << " vs " << comparison_metrics->total_bags_unsold << "\n";
        log << "Revenue: $" << fixed << setprecision(2) 
            << metrics_collector.metrics.total_revenue_generated 
            << " vs $" << comparison_metrics->total_revenue_generated << "\n";
        log << "Fairness (Gini): " << fixed << setprecision(4) 
            << metrics_collector.metrics.gini_coefficient_exposure 
            << " vs " << comparison_metrics->gini_coefficient_exposure << "\n";
    }
    
    log.close();
}

void SimulationEngine::set_output_stream(ostream* os) {
    output_stream = os ? os : &cout;
}

void SimulationEngine::set_customer_pool(const vector<Customer>& pool) {
    pre_generated_customers = pool;
    use_pre_generated_data = true;
}

void SimulationEngine::set_arrival_times(const vector<vector<Timestamp>>& times) {
    pre_generated_arrival_times = times;
    use_pre_generated_data = true;
}

