#include <string>
#include <vector>
#include <map>
#include <queue>
#include <iostream>
#include <fstream>
#include <ctime>
#include <random>
#include <algorithm>
#include <memory>
#include <cmath>

// Forward declarations
class Customer;
class Restaurant;
class Reservation;
class MarketState;
#include <fstream>
#include <sstream>

// ============================================================================
// TIMESTAMP UTILITIES
// ============================================================================
struct Timestamp {
    int hour;
    int minute;

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
// CUSTOMER HISTORY
// ============================================================================
struct CustomerHistory {
    int visits;
    int reservations;
    int successes;
    int cancellations;
    Timestamp last_reservation_time;
    std::map<std::string, int> categories_reserved;

    struct StoreInteraction {
        int reservations;
        int successes;
        int cancellations;

        StoreInteraction() : reservations(0), successes(0), cancellations(0) {}
    };
    std::map<int, StoreInteraction> store_interactions;

    CustomerHistory() : visits(0), reservations(0), successes(0),
        cancellations(0), last_reservation_time(0, 0) {
    }
};

// ============================================================================
// CUSTOMER MODEL
// ============================================================================
class Customer {
public:
    int id;
    float longitude;
    float latitude;
    std::string customer_name;
    std::string segment;
    float willingness_to_pay;

    struct Weights {
        float rating_w;
        float price_w;
        float novelty_w;

        Weights(float r = 1.0f, float p = 1.0f, float n = 0.5f)
            : rating_w(r), price_w(p), novelty_w(n) {
        }
    } weights;

    float loyalty;
    float leaving_threshold;
    CustomerHistory history;
    bool churned;

    std::map<std::string, float> category_preference;

    // Default constructor
    Customer() : id(0), customer_name("garry"),segment("regular"), willingness_to_pay(200.0f),
        loyalty(0.8f), leaving_threshold(5.0f), churned(false) {
        category_preference["bakery"] = 1.0f;
        category_preference["cafe"] = 1.0f;
        category_preference["restaurant"] = 1.0f;
    }

    Customer(int customer_id, const std::string& seg = "regular")
        : id(customer_id), segment(seg), willingness_to_pay(200.0f),
        loyalty(0.8f), leaving_threshold(3.0f), churned(false) {

        category_preference["bakery"] = 1.0f;
        category_preference["cafe"] = 1.0f;
        category_preference["restaurant"] = 1.0f;
    }


    Customer(int id_,
         float lon_,
         float lat_,
         const std::string& name_,
         const std::string& segment_,
         float wtp,
         float rating_weight,
         float price_weight,
         float novelty_weight,
         float leaving_thresh)
    {
        id = id_;
        longitude = lon_;
        latitude = lat_;
        customer_name = name_;
        segment = segment_;
        willingness_to_pay = wtp;

        // Set weights from CSV
        weights.rating_w = rating_weight;
        weights.price_w = price_weight;
        weights.novelty_w = novelty_weight;

        loyalty = 0.8f;
        leaving_threshold = leaving_thresh;
        churned = false;

        // Default categories if needed
        category_preference["bakery"] = 1.0f;
        category_preference["cafe"] = 1.0f;
        category_preference["restaurant"] = 1.0f;
    }

    float calculate_store_score(const Restaurant& store) const;

    void update_loyalty(bool was_cancelled) {
        if (was_cancelled) {
            loyalty = std::max(0.0f, loyalty - 0.1f);
        }
        else {
            loyalty = std::min(1.0f, loyalty + 0.05f);
        }
    }

    void update_category_preference(const std::string& category) {
        category_preference[category] += 0.1f;
    }

    void record_visit() {
        history.visits++;
    }

    void record_reservation_attempt(int store_id, const std::string& category,
        Timestamp time) {
        history.reservations++;
        history.last_reservation_time = time;
        history.categories_reserved[category]++;
        history.store_interactions[store_id].reservations++;
    }

    void record_reservation_success(int store_id, const std::string& category) {
        history.successes++;
        history.store_interactions[store_id].successes++;
        update_category_preference(category);
    }

    void record_reservation_cancellation(int store_id) {
        history.cancellations++;
        history.store_interactions[store_id].cancellations++;
        update_loyalty(true);
    }
};

// ============================================================================
// RESTAURANT MODEL
// ============================================================================
class Restaurant {
public:
    int business_id;
    std::string business_name;
    float general_ranking;
    std::string business_type;

    int estimated_bags;
    float price_per_bag;

    int actual_bags;

    int reserved_count;
    bool has_inventory;

    Restaurant(int id, const std::string& name, float rating,
        const std::string& type, int est_bags, float price)
        : business_id(id), business_name(name), general_ranking(rating),
        business_type(type), estimated_bags(est_bags), price_per_bag(price),
        actual_bags(0), reserved_count(0), has_inventory(true) {
    }

    bool can_accept_reservation() const {
        return has_inventory && (reserved_count < estimated_bags);
    }

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
        status(PENDING) {
    }
};

// ============================================================================
// MARKET STATE
// ============================================================================
class MarketState {
public:
    std::vector<Restaurant> restaurants;
    std::map<int, Customer> customers;
    std::vector<Reservation> reservations;
    Timestamp current_time;

    int next_reservation_id;

    MarketState() : current_time(8, 0), next_reservation_id(1) {}

    std::vector<int> get_available_restaurant_ids() const {
        std::vector<int> available;
        for (const auto& restaurant : restaurants) {
            if (restaurant.can_accept_reservation()) {
                available.push_back(restaurant.business_id);
            }
        }
        return available;
    }

    Restaurant* get_restaurant(int id) {
        for (auto& r : restaurants) {
            if (r.business_id == id) return &r;
        }
        return nullptr;
    }

    Customer* get_customer(int id) {
        auto it = customers.find(id);
        return (it != customers.end()) ? &(it->second) : nullptr;
    }
};

// ============================================================================
// Customer score calculation
// ============================================================================
float Customer::calculate_store_score(const Restaurant& store) const {
    float rating_score = weights.rating_w * store.general_ranking;
    float price_score = weights.price_w * (willingness_to_pay - store.price_per_bag) / willingness_to_pay;

    float novelty_score = 0.0f;
    auto it = history.categories_reserved.find(store.business_type);
    if (it == history.categories_reserved.end()) {
        novelty_score = weights.novelty_w * 1.0f;
    }
    else {
        novelty_score = weights.novelty_w * (1.0f / (1.0f + it->second));
    }

    // 4) Availability / unsold-bags component
    //    More unsold bags → higher score
    int unsold_bags = std::max(0, store.actual_bags - store.reserved_count);
    int capacity   = std::max(1, store.actual_bags);  // avoid divide by zero

    // ratio in [0,1]: 0 = no stock, 1 = fully unsold
    float availability_ratio = static_cast<float>(unsold_bags) / capacity;

    // weight for how much availability matters vs rating/price/novelty
    const float availability_weight = 1.0f;

    float availability_score = availability_weight * availability_ratio;

    // 5) Final total score
    float total_score = rating_score + price_score + novelty_score + availability_score;
    return total_score;
}

// ============================================================================
// RANKING SYSTEM - Baseline: Top-N by popularity (rating)
// ============================================================================
std::vector<int> get_displayed_stores(const Customer& customer,
    const MarketState& market_state,
    int n_displayed) {

    std::vector<int> available = market_state.get_available_restaurant_ids();

    // Sort by *customer's* store score (descending)
    std::sort(available.begin(), available.end(),
        [&market_state, &customer](int a, int b) {
            const Restaurant* r1 = nullptr;
            const Restaurant* r2 = nullptr;

            for (const auto& r : market_state.restaurants) {
                if (r.business_id == a) r1 = &r;
                if (r.business_id == b) r2 = &r;
            }

            // Just in case (shouldn't happen, but keeps sort safe)
            if (!r1 || !r2) return false;

            float s1 = customer.calculate_store_score(*r1);
            float s2 = customer.calculate_store_score(*r2);

            return s1 > s2; // descending
        });

    int num_to_show = std::min(n_displayed, (int)available.size());
    return std::vector<int>(available.begin(), available.begin() + num_to_show);
}

// ============================================================================
// CUSTOMER DECISION SYSTEM
// ============================================================================
class CustomerDecisionSystem {
public:
    static int process_customer_arrival(Customer& customer,
        MarketState& market_state,
        int n_displayed) {
        customer.record_visit();

        std::vector<int> displayed = get_displayed_stores(customer, market_state, n_displayed);

        if (displayed.empty()) {
            customer.churned = true;
            return -1;
        }

        std::vector<float> scores = calculate_store_scores(customer, displayed, market_state);

        int selected = select_store(customer, displayed, scores);

        if (selected == -1) {
            customer.churned = true;
            return -1;
        }

        bool success = create_reservation(customer, selected, market_state);

        if (!success) {
            customer.churned = true;
            return -1;
        }

        return selected;
    }

    static std::vector<float> calculate_store_scores(
        const Customer& customer,
        const std::vector<int>& displayed_store_ids,
        const MarketState& market_state) {

        std::vector<float> scores;
        for (int store_id : displayed_store_ids) {
            const Restaurant* store = nullptr;
            for (const auto& r : market_state.restaurants) {
                if (r.business_id == store_id) {
                    store = &r;
                    break;
                }
            }
            if (store) {
                scores.push_back(customer.calculate_store_score(*store));
            }
        }
        return scores;
    }

    static int select_store(const Customer& customer,
        const std::vector<int>& displayed_store_ids,
        const std::vector<float>& scores) {

        if (scores.empty()) return -1;

        float max_score = *std::max_element(scores.begin(), scores.end());

        // Lower threshold - customers are more willing to buy
        if (max_score < 3.0f) {
            return -1;
        }

        int best_idx = std::distance(scores.begin(),
            std::max_element(scores.begin(), scores.end()));
        return displayed_store_ids[best_idx];
    }

    static bool create_reservation(Customer& customer,
        int restaurant_id,
        MarketState& market_state) {

        Restaurant* restaurant = market_state.get_restaurant(restaurant_id);
        if (!restaurant || !restaurant->can_accept_reservation()) {
            return false;
        }

        Reservation res(market_state.next_reservation_id++,
            customer.id,
            restaurant_id,
            market_state.current_time);

        customer.record_reservation_attempt(restaurant_id,
            restaurant->business_type,
            market_state.current_time);

        restaurant->reserved_count++;
        market_state.reservations.push_back(res);

        return true;
    }
};

// ============================================================================
// RESTAURANT MANAGEMENT SYSTEM
// ============================================================================
class RestaurantManagementSystem {
public:
    static void process_end_of_day(MarketState& market_state) {
        for (auto& restaurant : market_state.restaurants) {
            if (restaurant.reserved_count > restaurant.actual_bags) {
                std::vector<int> cancelled_ids = cancel_excess_reservations(
                    restaurant, market_state.reservations);

                for (int res_id : cancelled_ids) {
                    for (auto& res : market_state.reservations) {
                        if (res.reservation_id == res_id) {
                            Customer* customer = market_state.get_customer(res.customer_id);
                            if (customer) {
                                handle_cancellation(res, *customer);
                            }
                        }
                    }
                }
            }
            else {
                for (auto& res : market_state.reservations) {
                    if (res.restaurant_id == restaurant.business_id &&
                        res.status == Reservation::PENDING) {
                        Customer* customer = market_state.get_customer(res.customer_id);
                        if (customer) {
                            handle_confirmation(res, *customer);
                        }
                    }
                }
            }
        }
    }

    static std::vector<int> cancel_excess_reservations(
        Restaurant& restaurant,
        std::vector<Reservation>& reservations) {

        std::vector<int> cancelled_ids;
        std::vector<Reservation*> restaurant_reservations;

        for (auto& res : reservations) {
            if (res.restaurant_id == restaurant.business_id &&
                res.status == Reservation::PENDING) {
                restaurant_reservations.push_back(&res);
            }
        }

        std::sort(restaurant_reservations.begin(), restaurant_reservations.end(),
            [](Reservation* a, Reservation* b) {
                return a->reservation_time < b->reservation_time;
            });

        int to_cancel = restaurant.reserved_count - restaurant.actual_bags;
        for (int i = restaurant.actual_bags; i < (int)restaurant_reservations.size(); i++) {
            restaurant_reservations[i]->status = Reservation::CANCELLED;
            cancelled_ids.push_back(restaurant_reservations[i]->reservation_id);
        }

        return cancelled_ids;
    }

    static void handle_cancellation(Reservation& reservation, Customer& customer) {
        reservation.status = Reservation::CANCELLED;
        customer.record_reservation_cancellation(reservation.restaurant_id);
    }

    static void handle_confirmation(Reservation& reservation, Customer& customer) {
        reservation.status = Reservation::CONFIRMED;
        customer.record_reservation_success(reservation.restaurant_id, "");
    }
};

// ============================================================================
// ARRIVAL GENERATOR
// ============================================================================
class ArrivalGenerator {
private:
    std::mt19937 rng;
    std::vector<Customer> customers_from_csv;

public:
    ArrivalGenerator(unsigned seed = std::time(nullptr)) : rng(seed) {}

    ArrivalGenerator(const std::string& csv_path, unsigned seed = std::time(nullptr))
        : rng(seed)
    {
        load_customers_from_csv(csv_path);
    }

    std::vector<Timestamp> generate_arrival_times(int num_customers) {
        std::vector<Timestamp> times;
        std::uniform_int_distribution<int> hour_dist(8, 21);
        std::uniform_int_distribution<int> min_dist(0, 59);

        for (int i = 0; i < num_customers; i++) {
            times.push_back(Timestamp(hour_dist(rng), min_dist(rng)));
        }

        std::sort(times.begin(), times.end());
        return times;
    }

    void load_customers_from_csv(const std::string& filename) {
        std::ifstream file(filename);
        if (!file.is_open()) {
            std::cerr << "Failed to open CSV: " << filename << std::endl;
            return;
        }

        std::string line;
        bool firstLine = true;

        while (std::getline(file, line)) {
            if (firstLine) {
                firstLine = false; // skip header
                continue;
            }
            if (line.empty()) continue;

            std::stringstream ss(line);

            std::string id_str, lon_str, lat_str, name_str, segment_str;
            std::string wtp_str, ratingw_str, pricew_str, noveltyw_str, leave_str;

            std::getline(ss, id_str, ',');
            std::getline(ss, lon_str, ',');
            std::getline(ss, lat_str, ',');
            std::getline(ss, name_str, ',');
            std::getline(ss, segment_str, ',');
            std::getline(ss, wtp_str, ',');
            std::getline(ss, ratingw_str, ',');
            std::getline(ss, pricew_str, ',');
            std::getline(ss, noveltyw_str, ',');
            std::getline(ss, leave_str, ',');

            int id = std::stoi(id_str);
            float lon = std::stof(lon_str);
            float lat = std::stof(lat_str);
            float wtp = std::stof(wtp_str);
            float rating_w = std::stof(ratingw_str);
            float price_w = std::stof(pricew_str);
            float novelty_w = std::stof(noveltyw_str);
            float leaving_threshold = std::stof(leave_str);

            // Create using the new constructor
            Customer c(id, lon, lat, name_str, segment_str, wtp,
                       rating_w, price_w, novelty_w, leaving_threshold);

            customers_from_csv.push_back(c);
        }

        std::cout << "Loaded " << customers_from_csv.size()
                  << " customers from " << filename << std::endl;

        file.close();
    }

    Customer generate_customer(int index) {
        if (!customers_from_csv.empty()) {
            if (index < (int)customers_from_csv.size())
                return customers_from_csv[index];
            else
                return customers_from_csv.back(); // fallback last
        }
       /*
        std::uniform_int_distribution<int> segment_dist(0, 2);
        int seg = segment_dist(rng);

        std::string segment;
        float wtp;

        if (seg == 0) {
            segment = "budget";
            wtp = 100.0f;
        }
        else if (seg == 1) {
            segment = "regular";
            wtp = 150.0f;
        }
        else {
            segment = "premium";
            wtp = 250.0f;
        }

        Customer customer(index, segment);
        customer.willingness_to_pay = wtp;
        return customer;
        */
    }






};

// ============================================================================
// METRICS
// ============================================================================
struct SimulationMetrics {
    int total_bags_sold;
    int total_bags_cancelled;
    int total_bags_unsold;
    float total_revenue_generated;
    float total_revenue_lost;
    int customers_who_left;
    int total_customer_arrivals;

    std::map<int, int> bags_sold_per_store;
    std::map<int, int> bags_cancelled_per_store;
    std::map<int, float> revenue_per_store;
    std::map<int, int> times_displayed_per_store;

    float gini_coefficient_exposure;

    SimulationMetrics() : total_bags_sold(0), total_bags_cancelled(0),
        total_bags_unsold(0), total_revenue_generated(0.0f),
        total_revenue_lost(0.0f), customers_who_left(0),
        total_customer_arrivals(0),
        gini_coefficient_exposure(0.0f) {
    }

    void print_summary() const {
        std::cout << "\n=== SIMULATION METRICS ===" << std::endl;
        std::cout << "Total Bags Sold: " << total_bags_sold << std::endl;
        std::cout << "Total Bags Cancelled: " << total_bags_cancelled << std::endl;
        std::cout << "Total Bags Unsold: " << total_bags_unsold << std::endl;
        std::cout << "Total Revenue: $" << total_revenue_generated << std::endl;
        std::cout << "Revenue Lost: $" << total_revenue_lost << std::endl;
        std::cout << "Customers Arrived: " << total_customer_arrivals << std::endl;
        std::cout << "Customers Left: " << customers_who_left << std::endl;
        std::cout << "Gini Coefficient (Exposure): " << gini_coefficient_exposure << std::endl;
    }
};

class MetricsCollector {
public:
    SimulationMetrics metrics;

    void log_customer_arrival(int customer_id, Timestamp time) {
        metrics.total_customer_arrivals++;
    }

    void log_stores_displayed(const std::vector<int>& store_ids) {
        for (int id : store_ids) {
            metrics.times_displayed_per_store[id]++;
        }
    }

    void log_reservation(const Reservation& res, float price) {
        // Will be finalized at end of day
    }

    void log_customer_left(int customer_id) {
        metrics.customers_who_left++;
    }

    void log_cancellation(const Reservation& res, float lost_revenue) {
        metrics.total_bags_cancelled++;
        metrics.total_revenue_lost += lost_revenue;
        metrics.bags_cancelled_per_store[res.restaurant_id]++;
    }

    void log_confirmation(const Reservation& res) {
        metrics.total_bags_sold++;
    }

    void log_end_of_day(const MarketState& market_state) {
        // Reset totals before calculating
        metrics.total_bags_sold = 0;
        metrics.total_bags_cancelled = 0;
        metrics.total_bags_unsold = 0;
        metrics.total_revenue_generated = 0.0f;
        metrics.total_revenue_lost = 0.0f;

        for (const auto& restaurant : market_state.restaurants) {
            int sold = 0;
            int cancelled = 0;

            for (const auto& res : market_state.reservations) {
                if (res.restaurant_id == restaurant.business_id) {
                    if (res.status == Reservation::CONFIRMED) {
                        sold++;
                        metrics.total_bags_sold++;
                        metrics.revenue_per_store[restaurant.business_id] +=
                            restaurant.price_per_bag;
                        metrics.total_revenue_generated += restaurant.price_per_bag;
                    }
                    else if (res.status == Reservation::CANCELLED) {
                        cancelled++;
                        metrics.total_bags_cancelled++;
                        metrics.total_revenue_lost += restaurant.price_per_bag;
                    }
                }
            }

            metrics.bags_sold_per_store[restaurant.business_id] = sold;
            metrics.bags_cancelled_per_store[restaurant.business_id] = cancelled;

            int unsold = restaurant.actual_bags - sold;
            if (unsold > 0) {
                metrics.total_bags_unsold += unsold;
            }
        }
    }

    void calculate_fairness_metrics(const MarketState& market_state) {
        std::vector<int> exposures;
        for (const auto& restaurant : market_state.restaurants) {
            exposures.push_back(metrics.times_displayed_per_store[restaurant.business_id]);
        }

        if (exposures.empty()) {
            metrics.gini_coefficient_exposure = 0.0f;
            return;
        }

        std::sort(exposures.begin(), exposures.end());

        float sum = 0.0f;
        float weighted_sum = 0.0f;

        for (size_t i = 0; i < exposures.size(); i++) {
            sum += exposures[i];
            weighted_sum += exposures[i] * (i + 1);
        }

        if (sum == 0) {
            metrics.gini_coefficient_exposure = 0.0f;
        }
        else {
            int n = exposures.size();
            metrics.gini_coefficient_exposure =
                (2.0f * weighted_sum) / (n * sum) - (n + 1.0f) / n;
        }
    }
};

// ============================================================================
// SIMULATION ENGINE
// ============================================================================
class SimulationEngine {
private:
    MarketState market_state;
    MetricsCollector metrics_collector;
    ArrivalGenerator arrival_generator;
    int n_displayed;

public:
    SimulationEngine(int n_display, const std::string& customer_csv)
    : n_displayed(n_display),
      arrival_generator(customer_csv) {}

    void initialize(const std::vector<Restaurant>& restaurants) {
        market_state.restaurants = restaurants;

        // Simulate actual inventory (80-120% of estimate)
        std::mt19937 rng(std::time(nullptr));
        std::uniform_real_distribution<float> variance(0.8f, 1.2f);

        for (auto& restaurant : market_state.restaurants) {
            int actual = (int)(restaurant.estimated_bags * variance(rng));
            restaurant.set_actual_inventory(std::max(0, actual));
        }
    }

    void run_day_simulation(int num_customers) {
        std::cout << "\n=== Starting Day Simulation ===" << std::endl;
        std::cout << "Number of customers: " << num_customers << std::endl;
        std::cout << "Number of stores: " << market_state.restaurants.size() << std::endl;

        std::cout << "\nInitial Store Inventory:" << std::endl;
        for (const auto& r : market_state.restaurants) {
            std::cout << r.business_name << ": Estimated=" << r.estimated_bags
                << ", Actual=" << r.actual_bags
                << ", Price=$" << r.price_per_bag << std::endl;
        }

        auto arrival_times = arrival_generator.generate_arrival_times(num_customers);

        int successful_reservations = 0;
        for (int i = 0; i < num_customers; i++) {
            Customer customer = arrival_generator.generate_customer(i);
            market_state.current_time = arrival_times[i];

            metrics_collector.log_customer_arrival(customer.id, arrival_times[i]);

            std::vector<int> displayed = get_displayed_stores(customer, market_state, n_displayed);
            metrics_collector.log_stores_displayed(displayed);

            // Insert customer into map BEFORE passing reference
            market_state.customers.insert(std::make_pair(i, customer));

            int selected = CustomerDecisionSystem::process_customer_arrival(
                market_state.customers[i], market_state, n_displayed);

            if (selected == -1) {
                metrics_collector.log_customer_left(customer.id);
            }
            else {
                successful_reservations++;
            }
        }

        std::cout << "\nTotal Reservations Made: " << successful_reservations << std::endl;
        std::cout << "Processing end of day..." << std::endl;
        RestaurantManagementSystem::process_end_of_day(market_state);

        metrics_collector.log_end_of_day(market_state);
        metrics_collector.calculate_fairness_metrics(market_state);
    }

    const SimulationMetrics& get_metrics() const {
        return metrics_collector.metrics;
    }

    void export_results(const std::string& filename) {
        std::ofstream out(filename);
        out << "Restaurant,Estimated,Actual,Reserved,Sold,Cancelled,Revenue,Exposures\n";

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
                << metrics_collector.metrics.revenue_per_store[restaurant.business_id] << ","
                << metrics_collector.metrics.times_displayed_per_store[restaurant.business_id] << "\n";
        }

        out.close();
        std::cout << "\nResults exported to " << filename << std::endl;
    }
};

// ============================================================================
// MAIN FUNCTION
// ============================================================================
int main() {
    std::cout << "=== Food Waste Marketplace Simulation ===" << std::endl;

    // Create initial restaurant database
    std::vector<Restaurant> restaurants;
    restaurants.push_back(Restaurant(1, "Krispy Kreme", 4.8f, "bakery", 10, 80.0f));
    restaurants.push_back(Restaurant(2, "TBS Pizza", 4.2f, "restaurant", 10, 150.0f));
    restaurants.push_back(Restaurant(3, "Starbucks", 4.5f, "cafe", 15, 100.0f));
    restaurants.push_back(Restaurant(4, "Paul Bakery", 4.6f, "bakery", 12, 90.0f));
    restaurants.push_back(Restaurant(5, "Costa Coffee", 4.3f, "cafe", 8, 85.0f));
    restaurants.push_back(Restaurant(6, "Greggs", 4.0f, "bakery", 20, 70.0f));
    restaurants.push_back(Restaurant(7, "Pizza Hut", 4.1f, "restaurant", 14, 140.0f));
    restaurants.push_back(Restaurant(8, "Pret A Manger", 4.4f, "cafe", 18, 95.0f));
    restaurants.push_back(Restaurant(9, "Subway", 3.9f, "restaurant", 16, 110.0f));
    restaurants.push_back(Restaurant(10, "Tim Hortons", 4.2f, "cafe", 10, 80.0f));

    // Initialize simulation
    SimulationEngine engine(5, "C:\\Users\\ziadg\\Desktop\\analysis project\\customer.csv");  // Display top 5 stores
    engine.initialize(restaurants);

    // Run simulation with 100 customers
    engine.run_day_simulation(100);

    // Print results
    engine.get_metrics().print_summary();

    // Export to CSV
    engine.export_results("simulation_results.csv");

    return 0;
}