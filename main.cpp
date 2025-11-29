#include <string>
#include <vector>
#include <map>
#include <queue>
#include <iostream>
#include <ctime>
#include <random>
#include <algorithm>
#include <memory>

// Forward declarations
class Customer;
class Restaurant;
class Reservation;
class MarketState;

// ============================================================================
// TIMESTAMP UTILITIES
// ============================================================================
struct Timestamp {
    int hour;      // 0-23
    int minute;    // 0-59
    
    Timestamp(int h = 8, int m = 0) : hour(h), minute(m) {}
    
    int to_minutes() const { return hour * 60 + minute; }
    
    bool operator<(const Timestamp& other) const {
        return to_minutes() < other.to_minutes();
    }
    
    std::string to_string() const {
        return std::to_string(hour) + ":" + 
               (minute < 10 ? "0" : "") + std::to_string(minute);
    }
};

// ============================================================================
// CUSTOMER HISTORY - Track customer behavior
// ============================================================================
struct CustomerHistory {
    int visits;                                    // Number of times customer arrived
    int reservations;                              // Total reservation attempts
    int successes;                                 // Confirmed reservations
    int cancellations;                             // Cancellations received
    Timestamp last_reservation_time;               // Last reservation timestamp
    std::map<std::string, int> categories_reserved; // {"bakery": 4, "cafe": 2}
    
    // Store-specific interactions: {store_id: {reservations, successes, cancellations}}
    struct StoreInteraction {
        int reservations;
        int successes;
        int cancellations;
        
        StoreInteraction() : reservations(0), successes(0), cancellations(0) {}
    };
    std::map<int, StoreInteraction> store_interactions;
    
    CustomerHistory() : visits(0), reservations(0), successes(0), 
                        cancellations(0), last_reservation_time(0, 0) {}
};

// ============================================================================
// CUSTOMER MODEL
// ============================================================================
class Customer {
public:
    int id;
    std::string segment;              // "budget", "regular", "premium"
    float willingness_to_pay;         // Maximum price willing to pay
    
    // Utility function weights
    struct Weights {
        float rating_w;    // Weight for store rating
        float price_w;     // Weight for price (lower is better)
        float novelty_w;   // Weight for trying new categories
        
        Weights(float r = 1.0f, float p = 1.0f, float n = 0.5f) 
            : rating_w(r), price_w(p), novelty_w(n) {}
    } weights;
    
    float loyalty;                    // Probability of returning (0-1)
    float leaving_threshold;          // Min score needed to not leave
    CustomerHistory history;
    bool churned;                     // Has customer left permanently?
    
    // Preference for categories (higher = more preferred)
    std::map<std::string, float> category_preference;
    
    Customer(int customer_id, const std::string& seg = "regular")
        : id(customer_id), segment(seg), willingness_to_pay(200.0f),
          loyalty(0.8f), leaving_threshold(5.0f), churned(false) {
        
        // Initialize default preferences
        category_preference["bakery"] = 1.0f;
        category_preference["cafe"] = 1.0f;
        category_preference["restaurant"] = 1.0f;
    }
    
    // Calculate utility score for a store
    float calculate_store_score(const Restaurant& store) const;
    
    // Update loyalty based on experience
    void update_loyalty(bool was_cancelled);
    
    // Increase preference for a category after successful reservation
    void update_category_preference(const std::string& category);
    
    // === CustomerHistory Update Functions ===
    // These encapsulate all history updates to maintain consistency
    
    // Called when customer arrives to the app
    void record_visit();
    
    // Called when customer makes a reservation attempt
    void record_reservation_attempt(int store_id, const std::string& category, 
                                   Timestamp time);
    
    // Called when reservation is confirmed
    void record_reservation_success(int store_id, const std::string& category);
    
    // Called when reservation is cancelled
    void record_reservation_cancellation(int store_id);
};

// ============================================================================
// RESTAURANT MODEL
// ============================================================================
class Restaurant {
public:
    int business_id;
    std::string business_name;
    float general_ranking;            // 1.0 to 5.0 rating
    std::string business_type;        // "bakery", "cafe", "restaurant", etc.
    
    // Morning estimate (8 AM)
    int estimated_bags;
    float price_per_bag;              // Price is FIXED throughout the day
    
    // End-of-day reality (10 PM)
    int actual_bags;                  // Only the bag count changes
    
    // Real-time tracking
    int reserved_count;               // Current number of reservations
    bool has_inventory;               // Does store still have bags?
    
    Restaurant(int id, const std::string& name, float rating, 
               const std::string& type, int est_bags, float price)
        : business_id(id), business_name(name), general_ranking(rating),
          business_type(type), estimated_bags(est_bags), price_per_bag(price),
          actual_bags(0), reserved_count(0), has_inventory(true) {}
    
    // Check if store can accept more reservations
    bool can_accept_reservation() const {
        return has_inventory && (reserved_count < estimated_bags);
    }
    
    // Update at end of day with actual inventory (price stays the same!)
    void set_actual_inventory(int bags) {
        actual_bags = bags;
    }
};

// ============================================================================
// RESERVATION MODEL
// ============================================================================
class Reservation {
public:
    enum Status { PENDING, CONFIRMED, CANCELLED };
    
    int reservation_id;
    int customer_id;
    int restaurant_id;
    Timestamp reservation_time;
    Status status;
    
    Reservation(int res_id, int cust_id, int rest_id, Timestamp time)
        : reservation_id(res_id), customer_id(cust_id), 
          restaurant_id(rest_id), reservation_time(time), 
          status(PENDING) {}
};

// ============================================================================
// MARKET STATE - Current state of the entire marketplace
// ============================================================================
class MarketState {
public:
    std::vector<Restaurant> restaurants;
    std::map<int, Customer> customers;           // customer_id -> Customer
    std::vector<Reservation> reservations;
    Timestamp current_time;
    
    int next_reservation_id;
    
    MarketState() : current_time(8, 0), next_reservation_id(1) {}
    
    // Get available restaurants (have inventory)
    std::vector<int> get_available_restaurant_ids() const {
        std::vector<int> available;
        for (const auto& restaurant : restaurants) {
            if (restaurant.can_accept_reservation()) {
                available.push_back(restaurant.business_id);
            }
        }
        return available;
    }
    
    // Find restaurant by ID
    Restaurant* get_restaurant(int id) {
        for (auto& r : restaurants) {
            if (r.business_id == id) return &r;
        }
        return nullptr;
    }
    
    // Find customer by ID
    Customer* get_customer(int id) {
        auto it = customers.find(id);
        return (it != customers.end()) ? &(it->second) : nullptr;
    }
};

// ============================================================================
// RANKING SYSTEM INTERFACE
// ============================================================================

std::vector<int> get_displayed_stores(const Customer& customer, 
                                       const MarketState& market_state,
                                       int n_displayed);

// ============================================================================
// CUSTOMER DECISION SYSTEM 
// ============================================================================
class CustomerDecisionSystem {
public:
    // Main customer arrival and decision process
    static int process_customer_arrival(Customer& customer, 
                                        MarketState& market_state,
                                        int n_displayed);
    
    // Calculate scores for displayed stores
    static std::vector<float> calculate_store_scores(
        const Customer& customer,
        const std::vector<int>& displayed_store_ids,
        const MarketState& market_state);
    
    // Customer makes a choice based on scores
    static int select_store(const Customer& customer,
                           const std::vector<int>& displayed_store_ids,
                           const std::vector<float>& scores);
    
    // Create a reservation
    static bool create_reservation(Customer& customer,
                                  int restaurant_id,
                                  MarketState& market_state);
};

// ============================================================================
// RESTAURANT MANAGEMENT SYSTEM 
// ============================================================================
class RestaurantManagementSystem {
public:
    // Process end-of-day inventory updates
    static void process_end_of_day(MarketState& market_state);
    
    // Cancel excess reservations for a restaurant (FCFS)
    static std::vector<int> cancel_excess_reservations(
        Restaurant& restaurant,
        std::vector<Reservation>& reservations);
    
    // Handle a single cancellation
    static void handle_cancellation(Reservation& reservation,
                                   Customer& customer);
    
    // Handle a single confirmation
    static void handle_confirmation(Reservation& reservation,
                                   Customer& customer);
    
    // Default ranking: top-n by popularity
    static std::vector<int> rank_by_popularity(
        const std::vector<int>& available_ids,
        const MarketState& market_state,
        int n);
};

// ============================================================================
// CUSTOMER ARRIVAL GENERATOR 
// ============================================================================
class ArrivalGenerator {
private:
    std::mt19937 rng;
    
public:
    ArrivalGenerator(unsigned seed = std::time(nullptr)) : rng(seed) {}
    
    // Generate customer arrival times throughout the day
    std::vector<Timestamp> generate_arrival_times(int num_customers);
    
    // Generate a new customer
    Customer generate_customer(int customer_id);
};

// ============================================================================
// METRICS AND LOGGING SYSTEM 
// ============================================================================
struct SimulationMetrics {
    int total_bags_sold;
    int total_bags_cancelled;
    int total_bags_unsold;
    float total_revenue_generated;
    float total_revenue_lost;
    int customers_who_left;
    int total_customer_arrivals;
    
    // Per-store metrics
    std::map<int, int> bags_sold_per_store;
    std::map<int, int> bags_cancelled_per_store;
    std::map<int, float> revenue_per_store;
    std::map<int, int> times_displayed_per_store;
    
    // Fairness metrics
    float gini_coefficient_exposure;  // Measure of exposure inequality
    
    SimulationMetrics() : total_bags_sold(0), total_bags_cancelled(0),
                         total_bags_unsold(0), total_revenue_generated(0.0f),
                         total_revenue_lost(0.0f), customers_who_left(0),
                         total_customer_arrivals(0),
                         gini_coefficient_exposure(0.0f) {}
    
    void print_summary() const {
        std::cout << "\n=== SIMULATION METRICS ===" << std::endl;
        std::cout << "Total Bags Sold: " << total_bags_sold << std::endl;
        std::cout << "Total Bags Cancelled: " << total_bags_cancelled << std::endl;
        std::cout << "Total Bags Unsold: " << total_bags_unsold << std::endl;
        std::cout << "Total Revenue: " << total_revenue_generated << std::endl;
        std::cout << "Revenue Lost: " << total_revenue_lost << std::endl;
        std::cout << "Customers Arrived: " << total_customer_arrivals << std::endl;
        std::cout << "Customers Left: " << customers_who_left << std::endl;
    }
};

class MetricsCollector {
public:
    SimulationMetrics metrics;
    
    void log_customer_arrival(int customer_id, Timestamp time);
    void log_stores_displayed(const std::vector<int>& store_ids);
    void log_reservation(const Reservation& res, float price);
    void log_customer_left(int customer_id);
    void log_cancellation(const Reservation& res, float lost_revenue);
    void log_confirmation(const Reservation& res);
    void log_end_of_day(const MarketState& market_state);
    
    void calculate_fairness_metrics(const MarketState& market_state);
};

// ============================================================================
// MAIN SIMULATION ENGINE 
// ============================================================================
class SimulationEngine {
private:
    MarketState market_state;
    MetricsCollector metrics_collector;
    ArrivalGenerator arrival_generator;
    int n_displayed;
    
public:
    SimulationEngine(int n_display = 5) : n_displayed(n_display) {}
    
    // Initialize the simulation with restaurants and data
    void initialize(const std::vector<Restaurant>& restaurants);
    
    // Run a full day simulation
    void run_day_simulation(int num_customers);
    
    // Get final metrics
    const SimulationMetrics& get_metrics() const {
        return metrics_collector.metrics;
    }
    
    // Export results for analysis
    void export_results(const std::string& filename);
};
