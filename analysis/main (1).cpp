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
#include <iomanip>
#include <cctype>
#include <set>

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
    std::map<int, float> store_valuations;  // Store valuations: store_id -> valuation

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
    int max_bags_per_customer;  // Maximum bags a single customer can receive
    
    // Dynamic rating tracking
    int total_orders_confirmed;
    int total_orders_cancelled;
    float initial_rating;  // Store initial rating for reference

    Restaurant(int id, const std::string& name, float rating,
        const std::string& type, int est_bags, float price)
        : business_id(id), business_name(name), general_ranking(rating),
        business_type(type), estimated_bags(est_bags), price_per_bag(price),
        actual_bags(0), reserved_count(0), has_inventory(true),
        max_bags_per_customer(3), total_orders_confirmed(0),
        total_orders_cancelled(0), initial_rating(rating) {
    }

    bool can_accept_reservation() const {
        return has_inventory && (reserved_count < estimated_bags);
    }

    void set_actual_inventory(int bags) {
        actual_bags = bags;
    }
    
    // Update rating when order is confirmed (positive feedback)
    void update_rating_on_confirmation() {
        total_orders_confirmed++;
        // Small positive adjustment: +0.01 per confirmation, capped at 5.0
        general_ranking = std::min(5.0f, general_ranking + 0.01f);
    }
    
    // Update rating when order is cancelled (negative feedback)
    void update_rating_on_cancellation() {
        total_orders_cancelled++;
        // Larger negative adjustment: -0.05 per cancellation, capped at 1.0
        general_ranking = std::max(1.0f, general_ranking - 0.05f);
    }
    
    // Get current rating
    float get_rating() const {
        return general_ranking;
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
// Customer score calculation (for customer decision-making ONLY)
// This is NOT used for ranking/display - only for customer choice
// ============================================================================
float Customer::calculate_store_score(const Restaurant& store) const {
    // Use dynamic rating (changes based on order confirmations/cancellations)
    float rating_score = weights.rating_w * store.get_rating();
    float price_score = weights.price_w * (willingness_to_pay - store.price_per_bag) / willingness_to_pay;

    float novelty_score = 0.0f;
    auto it = history.categories_reserved.find(store.business_type);
    if (it == history.categories_reserved.end()) {
        novelty_score = weights.novelty_w * 1.0f;
    }
    else {
        novelty_score = weights.novelty_w * (1.0f / (1.0f + it->second));
    }

    float total_score = rating_score + price_score + novelty_score;
    return total_score;
}

// ============================================================================
// RANKING ALGORITHM TYPES
// ============================================================================
enum class RankingAlgorithm {
    BASELINE,  // Top-N by rating only
    SMART      // Personalized + fairness + waste reduction
};

// ============================================================================
// RANKING SYSTEM - BASELINE: Pure Top-N by Popularity (Rating)
// This is the TRUE BASELINE - same stores shown to ALL customers
// Uses DYNAMIC ratings that change based on order confirmations/cancellations
// ============================================================================
std::vector<int> get_displayed_stores_baseline(const Customer& customer,
    const MarketState& market_state,
    int n_displayed) {

    std::vector<int> available = market_state.get_available_restaurant_ids();

    // BASELINE: Sort by DYNAMIC rating (changes as orders are confirmed/cancelled)
    // This shows the SAME top-rated stores to ALL customers
    std::sort(available.begin(), available.end(),
        [&market_state](int a, int b) {
            const Restaurant* r1 = nullptr;
            const Restaurant* r2 = nullptr;

            for (const auto& r : market_state.restaurants) {
                if (r.business_id == a) r1 = &r;
                if (r.business_id == b) r2 = &r;
            }

            // Safety check
            if (!r1 || !r2) return false;

            // Sort by current dynamic rating (descending)
            // Rating updates automatically when orders are confirmed/cancelled
            return r1->get_rating() > r2->get_rating();
        });

    int num_to_show = std::min(n_displayed, (int)available.size());
    return std::vector<int>(available.begin(), available.begin() + num_to_show);
}

// ============================================================================
// RANKING SYSTEM - SMART: Personalized + Fairness + Waste Reduction
// Goals: Minimize waste, maximize revenue, give stores fair chances
// Strategy: 3 personalized + 1 new store + 1 price-competitive
// ============================================================================
std::vector<int> get_displayed_stores_smart(const Customer& customer,
    const MarketState& market_state,
    int n_displayed) {

    std::vector<int> available = market_state.get_available_restaurant_ids();
    if (available.empty()) return available;

    std::vector<int> result;
    std::set<int> selected;

    // ========================================================================
    // IMPROVED SMART ALGORITHM: Adaptive Personalization + Smart Discovery
    // Strategy: Prioritize customer satisfaction while maintaining fairness
    // ========================================================================
    
    // Calculate comprehensive scores for all stores
    // Score = customer preference + inventory safety + rating quality
    std::vector<std::pair<int, float>> store_scores;
    for (int store_id : available) {
        const Restaurant* store = nullptr;
        for (const auto& r : market_state.restaurants) {
            if (r.business_id == store_id) {
                store = &r;
                break;
            }
        }
        if (store) {
            // Base customer preference score
            float base_score = customer.calculate_store_score(*store);
            
            // Segment-aware inventory safety bonus
            float inventory_safety = std::min(1.0f, (float)store->estimated_bags / 10.0f);
            float inventory_bonus = inventory_safety * 0.6f * segment_inventory_weight;
            
            // Segment-aware rating quality bonus
            float rating_bonus = (store->get_rating() - 3.5f) * 0.3f * segment_rating_weight;
            if (rating_bonus < 0) rating_bonus = 0;
            
            // Segment-aware price bonus (budget customers prefer cheaper stores)
            float price_bonus = 0.0f;
            if (is_budget) {
                if (store->price_per_bag < customer.willingness_to_pay) {
                    float price_savings = (customer.willingness_to_pay - store->price_per_bag) / customer.willingness_to_pay;
                    price_bonus = price_savings * 0.4f;
                }
            } else if (is_premium) {
                if (store->price_per_bag > 100.0f) {
                    price_bonus = 0.1f;
                }
            }
            
            // History bonus: prefer stores customer has had success with
            float history_bonus = 0.0f;
            auto hist_it = customer.history.store_interactions.find(store_id);
            if (hist_it != customer.history.store_interactions.end() && 
                hist_it->second.reservations > 0) {
                float success_rate = (float)hist_it->second.successes / hist_it->second.reservations;
                history_bonus = success_rate * 0.5f;
                
                if (hist_it->second.cancellations > 0) {
                    float cancel_rate = (float)hist_it->second.cancellations / hist_it->second.reservations;
                    history_bonus -= cancel_rate * 1.0f;
                }
            }
            
            // Segment-specific category preference bonus
            float category_bonus = 0.0f;
            auto cat_it = customer.category_preference.find(store->business_type);
            if (cat_it != customer.category_preference.end()) {
                category_bonus = cat_it->second * 0.2f;
            }
            
            float final_score = base_score + inventory_bonus + rating_bonus + price_bonus + history_bonus + category_bonus;
            store_scores.push_back({store_id, final_score});
        }
    }

    std::sort(store_scores.begin(), store_scores.end(),
        [](const std::pair<int, float>& a, const std::pair<int, float>& b) {
            return a.second > b.second;
        });

    // SEGMENT-AWARE ADAPTIVE STRATEGY: Adjust personalization based on segment AND loyalty
    float base_personalization = is_budget ? 0.7f : (is_premium ? 0.5f : 0.6f);
    float loyalty_adjustment = customer.loyalty * 0.15f;
    float personalization_ratio = base_personalization + loyalty_adjustment;
    personalization_ratio = std::min(0.85f, std::max(0.5f, personalization_ratio));
    
    int personalized_count = std::min((int)(n_displayed * personalization_ratio), (int)store_scores.size());
    personalized_count = std::max(3, personalized_count);
    
    // STEP 1: Select personalized stores (customer preferences + inventory + rating)
    for (int i = 0; i < personalized_count && result.size() < n_displayed; i++) {
        int store_id = store_scores[i].first;
        result.push_back(store_id);
        selected.insert(store_id);
    }

    // STEP 2: Smart Discovery - Only show NEW stores if they're HIGH QUALITY
    // This maintains fairness while ensuring customers actually want them
    if (result.size() < n_displayed) {
        std::vector<std::pair<int, float>> quality_new_stores;
        for (int store_id : available) {
            if (selected.find(store_id) != selected.end()) continue;
            
            // Check if customer has never visited this store
            auto hist_it = customer.history.store_interactions.find(store_id);
            bool is_new = (hist_it == customer.history.store_interactions.end() || 
                          hist_it->second.reservations == 0);
            
            if (is_new) {
                const Restaurant* store = nullptr;
                for (const auto& r : market_state.restaurants) {
                    if (r.business_id == store_id) {
                        store = &r;
                        break;
                    }
                }
                if (store) {
                    // QUALITY THRESHOLD: Only consider new stores that are actually good
                    // Must have: rating >= 4.0 AND inventory >= 8 AND good value
                    if (store->get_rating() >= 4.0f && store->estimated_bags >= 8) {
                        float value_score = store->get_rating() / store->price_per_bag;
                        float inventory_bonus = std::min(1.0f, (float)store->estimated_bags / 15.0f);
                        float quality_score = store->get_rating() + value_score * 10.0f + inventory_bonus * 0.5f;
                        quality_new_stores.push_back({store_id, quality_score});
                    }
                }
            }
        }
        
        if (!quality_new_stores.empty()) {
            std::sort(quality_new_stores.begin(), quality_new_stores.end(),
                [](const std::pair<int, float>& a, const std::pair<int, float>& b) {
                    return a.second > b.second;
                });
            
            // Select the BEST quality new store (not random, not middle)
            result.push_back(quality_new_stores[0].first);
            selected.insert(quality_new_stores[0].first);
        }
    }

    // STEP 3: Price-Competitive - Only if it's TRULY competitive AND safe
    if (result.size() < n_displayed) {
        std::vector<std::pair<int, float>> competitive_stores;
        for (int store_id : available) {
            if (selected.find(store_id) != selected.end()) continue;
            
            const Restaurant* store = nullptr;
            for (const auto& r : market_state.restaurants) {
                if (r.business_id == store_id) {
                    store = &r;
                    break;
                }
            }
            if (store) {
                // QUALITY THRESHOLD: Must be safe (inventory >= 8) AND competitive
                if (store->estimated_bags >= 8) {
                    float value_ratio = store->get_rating() / store->price_per_bag;
                    float inventory_safety = std::min(1.0f, (float)store->estimated_bags / 15.0f);
                    // Only consider if value is actually good (rating/price > 0.03)
                    if (value_ratio > 0.03f) {
                        float competitive_score = value_ratio * 100.0f + inventory_safety * 0.5f + store->get_rating() * 0.5f;
                        competitive_stores.push_back({store_id, competitive_score});
                    }
                }
            }
        }
        
        if (!competitive_stores.empty()) {
            std::sort(competitive_stores.begin(), competitive_stores.end(),
                [](const std::pair<int, float>& a, const std::pair<int, float>& b) {
                    return a.second > b.second;
                });
            
            result.push_back(competitive_stores[0].first);
            selected.insert(competitive_stores[0].first);
        }
    }

    // STEP 4: Fill remaining slots with best available (prioritize customer satisfaction)
    for (size_t i = 0; i < store_scores.size() && result.size() < n_displayed; i++) {
        int store_id = store_scores[i].first;
        if (selected.find(store_id) == selected.end()) {
            result.push_back(store_id);
            selected.insert(store_id);
        }
    }

    return result;
}

// ============================================================================
// UNIFIED RANKING FUNCTION
// ============================================================================
std::vector<int> get_displayed_stores(const Customer& customer,
    const MarketState& market_state,
    int n_displayed,
    RankingAlgorithm algorithm = RankingAlgorithm::BASELINE) {
    
    if (algorithm == RankingAlgorithm::SMART) {
        return get_displayed_stores_smart(customer, market_state, n_displayed);
    } else {
        return get_displayed_stores_baseline(customer, market_state, n_displayed);
    }
}

// ============================================================================
// CUSTOMER DECISION SYSTEM
// ============================================================================
class CustomerDecisionSystem {
public:
    static int process_customer_arrival(Customer& customer,
        MarketState& market_state,
        int n_displayed,
        RankingAlgorithm algorithm = RankingAlgorithm::BASELINE) {
        customer.record_visit();

        std::vector<int> displayed = get_displayed_stores(customer, market_state, n_displayed, algorithm);

        if (displayed.empty()) {
            customer.churned = true;
            return -1;
        }

        std::vector<float> scores = calculate_store_scores(customer, displayed, market_state);

        int selected = select_store(customer, displayed, scores, market_state);

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
        const std::vector<float>& scores,
        const MarketState& market_state) {

        if (scores.empty()) return -1;

        // Calculate threshold based on customer's leaving threshold and loyalty
        float base_threshold = customer.leaving_threshold;
        float loyalty_adjustment = (1.0f - customer.loyalty) * 2.0f;  // Lower loyalty = higher threshold
        float threshold = base_threshold + loyalty_adjustment;

        // Find best score
        float max_score = *std::max_element(scores.begin(), scores.end());

        // Customer leaves if no store meets their threshold
        if (max_score < threshold) {
            return -1;
        }

        // Consider customer history: prefer stores they've had success with
        std::vector<float> adjusted_scores = scores;
        for (size_t i = 0; i < displayed_store_ids.size(); i++) {
            int store_id = displayed_store_ids[i];
            
            // Adjust based on customer history
            auto it = customer.history.store_interactions.find(store_id);
            if (it != customer.history.store_interactions.end()) {
                const auto& interaction = it->second;
                if (interaction.reservations > 0) {
                    float success_rate = (float)interaction.successes / interaction.reservations;
                    // Boost score for stores with good history
                    adjusted_scores[i] += success_rate * 1.5f;
                    // Penalize stores with many cancellations
                    if (interaction.cancellations > 0) {
                        float cancel_rate = (float)interaction.cancellations / interaction.reservations;
                        adjusted_scores[i] -= cancel_rate * 2.0f;
                    }
                }
            }
            
            // IMPORTANT: Add inventory safety bonus to customer selection
            // This makes customers more likely to choose stores with sufficient inventory
            const Restaurant* store = market_state.get_restaurant(store_id);
            if (store) {
                float inventory_safety = std::min(1.0f, (float)store->estimated_bags / 12.0f);
                adjusted_scores[i] += inventory_safety * 0.3f;  // Bonus for inventory safety
            }
        }

        // REALISTIC CHOICE MODEL: Probabilistic selection instead of always picking max
        // This mimics real behavior where customers don't always choose the "best" option
        // Sometimes they pick cheaper options, try new places, or make suboptimal choices
        
        // Filter out stores below threshold
        std::vector<int> valid_indices;
        std::vector<float> valid_scores;
        for (size_t i = 0; i < adjusted_scores.size(); i++) {
            if (adjusted_scores[i] >= threshold) {
                valid_indices.push_back(i);
                valid_scores.push_back(adjusted_scores[i]);
            }
        }
        
        if (valid_scores.empty()) {
            return -1;
        }
        
        // Use probabilistic selection based on scores (softmax-like approach)
        // Higher scores have higher probability, but not guaranteed
        return probabilistic_select(displayed_store_ids, adjusted_scores, valid_indices, valid_scores);
    }
    
    // Probabilistic store selection - mimics real customer behavior
    static int probabilistic_select(
        const std::vector<int>& store_ids,
        const std::vector<float>& all_scores,
        const std::vector<int>& valid_indices,
        const std::vector<float>& valid_scores) {
        
        static std::mt19937 rng(std::time(nullptr));
        
        // Method 1: Weighted random selection (proportional to score)
        // This allows cheaper/lower-scored options to sometimes be chosen
        float min_score = *std::min_element(valid_scores.begin(), valid_scores.end());
        float max_score = *std::max_element(valid_scores.begin(), valid_scores.end());
        
        // Normalize scores to positive range and apply exponential to create probabilities
        // Using temperature parameter to control randomness (higher = more random)
        float temperature = 2.0f;  // Adjust this: lower = more deterministic, higher = more random
        
        std::vector<float> probabilities;
        float sum_exp = 0.0f;
        
        for (float score : valid_scores) {
            // Shift scores to be positive and apply softmax
            float shifted_score = score - min_score + 1.0f;  // Ensure positive
            float exp_score = std::exp(shifted_score / temperature);
            probabilities.push_back(exp_score);
            sum_exp += exp_score;
        }
        
        // Normalize to probabilities
        for (float& prob : probabilities) {
            prob /= sum_exp;
        }
        
        // Weighted random selection
        std::uniform_real_distribution<float> dist(0.0f, 1.0f);
        float random_val = dist(rng);
        float cumulative = 0.0f;
        
        for (size_t i = 0; i < probabilities.size(); i++) {
            cumulative += probabilities[i];
            if (random_val <= cumulative) {
                return store_ids[valid_indices[i]];
            }
        }
        
        // Fallback: return the last valid option
        return store_ids[valid_indices.back()];
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
            // Get all pending reservations for this restaurant
            std::vector<Reservation*> restaurant_reservations;
                    for (auto& res : market_state.reservations) {
                if (res.restaurant_id == restaurant.business_id &&
                    res.status == Reservation::PENDING) {
                    restaurant_reservations.push_back(&res);
                }
            }

            // Sort by reservation time (first come, first served)
            std::sort(restaurant_reservations.begin(), restaurant_reservations.end(),
                [](Reservation* a, Reservation* b) {
                    return a->reservation_time < b->reservation_time;
                });

            int num_reservations = restaurant_reservations.size();
            int actual_bags = restaurant.actual_bags;

            // Case 1: No orders received - all bags become waste
            if (num_reservations == 0) {
                // Waste is calculated in metrics
                continue;
            }

            // Case 2: Actual bags >= reserved count (enough or more than enough)
            if (actual_bags >= num_reservations) {
                // Distribute bags among customers, respecting max_bags_per_customer
                int remaining_bags = actual_bags;
                int bags_per_customer = std::min(
                    restaurant.max_bags_per_customer,
                    remaining_bags / num_reservations
                );
                
                // If we have extra bags, distribute them fairly
                int extra_bags = remaining_bags - (bags_per_customer * num_reservations);
                
                for (size_t i = 0; i < restaurant_reservations.size(); i++) {
                    Reservation* res = restaurant_reservations[i];
                    Customer* customer = market_state.get_customer(res->customer_id);
                    
                            if (customer) {
                        int bags_for_this_customer = bags_per_customer;
                        // Distribute extra bags (one per customer, up to max)
                        if (extra_bags > 0 && bags_for_this_customer < restaurant.max_bags_per_customer) {
                            bags_for_this_customer++;
                            extra_bags--;
                        }
                        
                        handle_confirmation(*res, *customer, market_state, bags_for_this_customer);
                            }
                        }
                    }
            // Case 3: Actual bags < reserved count (shortage - need to cancel some)
            else {
                // Confirm first 'actual_bags' reservations
                for (int i = 0; i < actual_bags; i++) {
                    Reservation* res = restaurant_reservations[i];
                    Customer* customer = market_state.get_customer(res->customer_id);
                        if (customer) {
                        handle_confirmation(*res, *customer, market_state, 1);
                        }
                }
                
                // Cancel the rest
                for (int i = actual_bags; i < num_reservations; i++) {
                    Reservation* res = restaurant_reservations[i];
                    Customer* customer = market_state.get_customer(res->customer_id);
                    if (customer) {
                        handle_cancellation(*res, *customer, market_state);
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

    static void handle_cancellation(Reservation& reservation, Customer& customer, MarketState& market_state) {
        reservation.status = Reservation::CANCELLED;
        customer.record_reservation_cancellation(reservation.restaurant_id);
        
        // Update restaurant rating dynamically (negative feedback)
        Restaurant* restaurant = market_state.get_restaurant(reservation.restaurant_id);
        if (restaurant) {
            restaurant->update_rating_on_cancellation();
        }
    }

    static void handle_confirmation(Reservation& reservation, Customer& customer, MarketState& market_state, int bags_received = 1) {
        reservation.status = Reservation::CONFIRMED;
        customer.record_reservation_success(reservation.restaurant_id, "");
        
        // Update restaurant rating dynamically (positive feedback)
        Restaurant* restaurant = market_state.get_restaurant(reservation.restaurant_id);
        if (restaurant) {
            restaurant->update_rating_on_confirmation();
        }
        // Note: bags_received can be used for metrics if needed
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
        // Try to load CSV, but continue even if it fails
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

    bool load_customers_from_csv(const std::string& filename) {
        std::ifstream file(filename);
        if (!file.is_open()) {
            std::cerr << "Warning: Could not open customer CSV file: " << filename << std::endl;
            std::cerr << "Will generate customers instead." << std::endl;
            return false;
        }

        std::string line;
        bool firstLine = true;
        std::vector<std::string> header_columns;
        std::vector<int> store_id_columns;  // Track which columns are store valuations

        // Read header to determine format
        if (std::getline(file, line)) {
            std::stringstream header_ss(line);
            std::string col;
            int col_index = 0;
            while (std::getline(header_ss, col, ',')) {
                header_columns.push_back(col);
                
                // Check if this is a store valuation column (format: store<id>_id_valuation)
                // Also handle variations like "store1_id_valuation", "store_1_id_valuation", etc.
                std::string col_lower = col;
                std::transform(col_lower.begin(), col_lower.end(), col_lower.begin(), ::tolower);
                
                if (col_lower.find("store") != std::string::npos && 
                    (col_lower.find("valuation") != std::string::npos || col_lower.find("_id_") != std::string::npos)) {
                    // Try to extract store ID from column name
                    // Format could be: store1_id_valuation, store_1_id_valuation, store1_valuation, etc.
                    size_t store_pos = col_lower.find("store");
                    if (store_pos != std::string::npos) {
                        size_t num_start = store_pos + 5; // "store" is 5 chars
                        // Skip any non-digit characters
                        while (num_start < col.length() && !std::isdigit(col_lower[num_start])) {
                            num_start++;
                        }
                        if (num_start < col_lower.length()) {
                            try {
                                // Extract the number
                                size_t num_end = num_start;
                                while (num_end < col_lower.length() && std::isdigit(col_lower[num_end])) {
                                    num_end++;
            }
                                int store_id = std::stoi(col_lower.substr(num_start, num_end - num_start));
                                // Store both column index and store ID mapping
                                store_id_columns.push_back(col_index);
                            } catch (...) {
                                // Couldn't parse, skip
                            }
                        }
                    }
                }
                col_index++;
            }
            firstLine = false;
        }

        // Determine format: if first column is "latitude" or "longitude", use new format
        bool new_format = (header_columns.size() > 0 && 
                          (header_columns[0] == "latitude" || header_columns[0] == "longitude"));

        // Get known restaurant IDs from market state (if available) or use store_id_columns
        // For now, we'll use the column indices to map to store IDs
        
        while (std::getline(file, line)) {
            if (line.empty()) continue;
            std::stringstream ss(line);
            std::vector<std::string> values;
            std::string val;
            
            // Read all values
            while (std::getline(ss, val, ',')) {
                values.push_back(val);
            }
            
            if (values.size() < 2) continue;  // Need at least latitude and longitude
            
            // Read user's format columns first (latitude, longitude, store valuations)
            float lat = 0.0f, lon = 0.0f;
            int col_idx = 0;
            
            // Read latitude (first column in user's format)
            if (col_idx < values.size() && !values[col_idx].empty()) {
                try {
                    lat = std::stof(values[col_idx]);
                } catch (...) {
                    lat = 0.0f;
                }
            }
            col_idx++;
            
            // Read longitude (second column in user's format)
            if (col_idx < values.size() && !values[col_idx].empty()) {
                try {
                    lon = std::stof(values[col_idx]);
                } catch (...) {
                    lon = 0.0f;
                }
            }
            col_idx++;
            
            // Read store valuations (optional - columns that match store*_id_valuation pattern)
            std::map<int, float> store_vals;
            for (size_t idx = 0; idx < store_id_columns.size(); idx++) {
                int store_col_idx = store_id_columns[idx];
                if (store_col_idx < (int)values.size() && !values[store_col_idx].empty()) {
                    try {
                        float valuation = std::stof(values[store_col_idx]);
                        // Extract store ID from the column name we parsed earlier
                        std::string col_name = header_columns[store_col_idx];
                        std::string col_lower = col_name;
                        std::transform(col_lower.begin(), col_lower.end(), col_lower.begin(), ::tolower);
                        
                        // Extract store ID from column name
                        size_t store_pos = col_lower.find("store");
                        int store_id = -1;
                        if (store_pos != std::string::npos) {
                            size_t num_start = store_pos + 5;
                            while (num_start < col_lower.length() && !std::isdigit(col_lower[num_start])) {
                                num_start++;
                            }
                            if (num_start < col_lower.length()) {
                                size_t num_end = num_start;
                                while (num_end < col_lower.length() && std::isdigit(col_lower[num_end])) {
                                    num_end++;
                                }
                                store_id = std::stoi(col_lower.substr(num_start, num_end - num_start));
                            }
                        }
                        
                        // If we couldn't extract from name, use sequential mapping as fallback
                        if (store_id == -1) {
                            store_id = store_col_idx - 2 + 1;  // Assuming first 2 cols are lat/lon
                        }
                        
                        store_vals[store_id] = valuation;
                    } catch (...) {
                        // Skip invalid valuation
                    }
                }
            }
            
            // Now read code's format columns (optional - if they exist)
            // Expected order: id, lon, lat, name, segment, wtp, rating_w, price_w, novelty_w, leave_thresh
            // But we already have lat/lon, so we skip those
            
            int id = customers_from_csv.size();  // Default: use index as ID
            std::string name = "Customer_" + std::to_string(id);
            std::string segment = "regular";
            float wtp = 150.0f;
            float rating_w = 1.0f;
            float price_w = 1.0f;
            float novelty_w = 0.5f;
            float leaving_thresh = 3.0f;
            
            // Try to read additional columns if they exist
            // Skip the store valuation columns we already processed
            int code_format_start = 2 + store_id_columns.size();  // After lat, lon, and store valuations
            
            if (code_format_start < (int)values.size()) {
                // id (optional - use index if not provided)
                if (code_format_start < (int)values.size() && !values[code_format_start].empty()) {
                    try {
                        id = std::stoi(values[code_format_start]);
                    } catch (...) {
                        id = customers_from_csv.size();
                    }
                }
                code_format_start++;
                
                // Skip lon (we already have it)
                if (code_format_start < (int)values.size()) code_format_start++;
                
                // Skip lat (we already have it)
                if (code_format_start < (int)values.size()) code_format_start++;
                
                // name
                if (code_format_start < (int)values.size() && !values[code_format_start].empty()) {
                    name = values[code_format_start];
                }
                code_format_start++;
                
                // segment
                if (code_format_start < (int)values.size() && !values[code_format_start].empty()) {
                    segment = values[code_format_start];
                }
                code_format_start++;
                
                // wtp
                if (code_format_start < (int)values.size() && !values[code_format_start].empty()) {
                    try {
                        wtp = std::stof(values[code_format_start]);
                    } catch (...) {
                        wtp = 150.0f;
                    }
                }
                code_format_start++;
                
                // rating_w
                if (code_format_start < (int)values.size() && !values[code_format_start].empty()) {
                    try {
                        rating_w = std::stof(values[code_format_start]);
                    } catch (...) {
                        rating_w = 1.0f;
                    }
                }
                code_format_start++;
                
                // price_w
                if (code_format_start < (int)values.size() && !values[code_format_start].empty()) {
                    try {
                        price_w = std::stof(values[code_format_start]);
                    } catch (...) {
                        price_w = 1.0f;
                    }
                }
                code_format_start++;
                
                // novelty_w
                if (code_format_start < (int)values.size() && !values[code_format_start].empty()) {
                    try {
                        novelty_w = std::stof(values[code_format_start]);
                    } catch (...) {
                        novelty_w = 0.5f;
                    }
                }
                code_format_start++;
                
                // leaving_thresh
                if (code_format_start < (int)values.size() && !values[code_format_start].empty()) {
                    try {
                        leaving_thresh = std::stof(values[code_format_start]);
                    } catch (...) {
                        leaving_thresh = 3.0f;
                    }
                }
            }
            
            // Create customer with all the data
            Customer c(id, lon, lat, name, segment, wtp, rating_w, price_w, novelty_w, leaving_thresh);
            c.store_valuations = store_vals;  // Set store valuations
            customers_from_csv.push_back(c);
        }

        std::cout << "Loaded " << customers_from_csv.size()
                  << " customers from " << filename << std::endl;
        if (!store_id_columns.empty()) {
            std::cout << "  Found " << store_id_columns.size() << " store valuation columns" << std::endl;
        }

        file.close();
        return true;
    }

    Customer generate_customer(int index, const std::vector<Restaurant>& restaurants = std::vector<Restaurant>()) {
        if (!customers_from_csv.empty()) {
            // Use modulo to cycle through CSV customers if needed
            int csv_index = index % customers_from_csv.size();
            Customer c = customers_from_csv[csv_index];
            c.id = index;  // Ensure unique ID
            return c;
        }
        
        // Generate random customer if no CSV loaded
        std::uniform_int_distribution<int> segment_dist(0, 2);
        int seg = segment_dist(rng);

        std::string segment;
        float wtp;
        float rating_w, price_w, novelty_w;
        float leaving_thresh;

        if (seg == 0) {
            segment = "budget";
            wtp = 80.0f + (rng() % 40);  // 80-120
            rating_w = 0.5f + (rng() % 100) / 200.0f;  // 0.5-1.0
            price_w = 1.5f + (rng() % 100) / 200.0f;   // 1.5-2.0 (price sensitive)
            novelty_w = 0.3f + (rng() % 100) / 200.0f; // 0.3-0.8
            leaving_thresh = 2.0f + (rng() % 30) / 10.0f; // 2.0-5.0
        }
        else if (seg == 1) {
            segment = "regular";
            wtp = 120.0f + (rng() % 60);  // 120-180
            rating_w = 1.0f + (rng() % 100) / 200.0f;  // 1.0-1.5
            price_w = 1.0f + (rng() % 100) / 200.0f;   // 1.0-1.5
            novelty_w = 0.5f + (rng() % 100) / 200.0f;  // 0.5-1.0
            leaving_thresh = 3.0f + (rng() % 40) / 10.0f; // 3.0-7.0
        }
        else {
            segment = "premium";
            wtp = 180.0f + (rng() % 80);  // 180-260
            rating_w = 1.5f + (rng() % 100) / 200.0f;  // 1.5-2.0 (quality focused)
            price_w = 0.5f + (rng() % 100) / 200.0f;   // 0.5-1.0 (less price sensitive)
            novelty_w = 0.8f + (rng() % 100) / 200.0f; // 0.8-1.3
            leaving_thresh = 4.0f + (rng() % 40) / 10.0f; // 4.0-8.0
        }

        // Generate random location (Cairo area approximately)
        std::uniform_real_distribution<float> lon_dist(31.2f, 31.3f);
        std::uniform_real_distribution<float> lat_dist(30.0f, 30.1f);
        
        Customer customer(index, lon_dist(rng), lat_dist(rng), 
                         "Customer_" + std::to_string(index), segment, 
                         wtp, rating_w, price_w, novelty_w, leaving_thresh);
        
        // Generate store valuations for this customer (same format as CSV would have)
        // Random valuations between 0.0 and 5.0 for each store
        if (!restaurants.empty()) {
            std::uniform_real_distribution<float> valuation_dist(0.0f, 5.0f);
            for (const auto& restaurant : restaurants) {
                float valuation = valuation_dist(rng);
                customer.store_valuations[restaurant.business_id] = valuation;
            }
        }
        
        return customer;
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
    std::map<int, int> waste_per_store;  // New: track waste per store

    float gini_coefficient_exposure;

    SimulationMetrics() : total_bags_sold(0), total_bags_cancelled(0),
        total_bags_unsold(0), total_revenue_generated(0.0f),
        total_revenue_lost(0.0f), customers_who_left(0),
        total_customer_arrivals(0),
        gini_coefficient_exposure(0.0f) {
    }

    void print_summary() const {
        std::cout << "\n========================================" << std::endl;
        std::cout << "=== SIMULATION METRICS SUMMARY ===" << std::endl;
        std::cout << "========================================" << std::endl;
        std::cout << "\n--- SALES METRICS ---" << std::endl;
        std::cout << "Total Bags Sold: " << total_bags_sold << std::endl;
        std::cout << "Total Bags Cancelled: " << total_bags_cancelled << std::endl;
        std::cout << "Total Bags Unsold (Waste): " << total_bags_unsold << std::endl;
        
        std::cout << "\n--- REVENUE METRICS ---" << std::endl;
        std::cout << "Total Revenue Generated: $" << std::fixed << std::setprecision(2) 
                  << total_revenue_generated << std::endl;
        std::cout << "Revenue Lost (from cancellations): $" << std::fixed << std::setprecision(2) 
                  << total_revenue_lost << std::endl;
        float revenue_efficiency = total_customer_arrivals > 0 ? 
            (total_revenue_generated / (total_revenue_generated + total_revenue_lost)) * 100.0f : 0.0f;
        std::cout << "Revenue Efficiency: " << std::fixed << std::setprecision(2) 
                  << revenue_efficiency << "%" << std::endl;
        
        std::cout << "\n--- CUSTOMER METRICS ---" << std::endl;
        std::cout << "Total Customer Arrivals: " << total_customer_arrivals << std::endl;
        std::cout << "Customers Who Left (No Purchase): " << customers_who_left << std::endl;
        float conversion_rate = total_customer_arrivals > 0 ? 
            ((float)(total_customer_arrivals - customers_who_left) / total_customer_arrivals) * 100.0f : 0.0f;
        std::cout << "Conversion Rate: " << std::fixed << std::setprecision(2) 
                  << conversion_rate << "%" << std::endl;
        
        std::cout << "\n--- FAIRNESS METRICS ---" << std::endl;
        std::cout << "Gini Coefficient (Exposure): " << std::fixed << std::setprecision(4) 
                  << gini_coefficient_exposure << std::endl;
        std::cout << "  (0 = perfect equality, 1 = maximum inequality)" << std::endl;
        
        std::cout << "\n========================================" << std::endl;
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

            // NEW WASTE LOGIC: Only count waste if restaurant received NO orders
            // If they received orders, we distribute all available bags (respecting max per customer)
            // So waste only occurs when no orders were received
            int unsold = 0;
            if (sold == 0 && cancelled == 0) {
                // No orders received - all actual_bags are waste
                unsold = restaurant.actual_bags;
            } else {
                // Orders received - we try to distribute all bags
                // Waste = 0 because we distribute extra bags to customers
                // (up to max_bags_per_customer limit)
                unsold = 0;
            }

            if (unsold > 0) {
                metrics.total_bags_unsold += unsold;
                metrics.waste_per_store[restaurant.business_id] = unsold;
            } else {
                metrics.waste_per_store[restaurant.business_id] = 0;
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
// RESTAURANT LOADER
// ============================================================================
class RestaurantLoader {
public:
    static bool load_restaurants_from_csv(const std::string& filename,
                                          std::vector<Restaurant>& restaurants) {
        std::ifstream file(filename);
        if (!file.is_open()) {
            std::cerr << "Warning: Could not open restaurant CSV file: " << filename << std::endl;
            std::cerr << "Will use default restaurants instead." << std::endl;
            return false;
        }

        std::string line;
        std::vector<std::string> header_columns;
        int store_id_idx = -1, store_name_idx = -1, branch_idx = -1;
        int bags_idx = -1, rating_idx = -1, price_idx = -1;
        int longitude_idx = -1, latitude_idx = -1;
        int business_type_idx = -1;

        if (std::getline(file, line)) {
            std::stringstream header_ss(line);
            std::string col;
            int col_index = 0;
            while (std::getline(header_ss, col, ',')) {
                col.erase(0, col.find_first_not_of(" \t"));
                col.erase(col.find_last_not_of(" \t") + 1);
                header_columns.push_back(col);
                
                std::string col_lower = col;
                std::transform(col_lower.begin(), col_lower.end(), col_lower.begin(), ::tolower);
                
                if (col_lower == "store_id") store_id_idx = col_index;
                else if (col_lower == "store_name") store_name_idx = col_index;
                else if (col_lower == "branch") branch_idx = col_index;
                else if (col_lower == "average_bags_at_9am") bags_idx = col_index;
                else if (col_lower == "average_overall_rating") rating_idx = col_index;
                else if (col_lower == "price") price_idx = col_index;
                else if (col_lower == "longitude") longitude_idx = col_index;
                else if (col_lower == "latitude") latitude_idx = col_index;
                else if (col_lower == "business_type" || col_lower == "type") business_type_idx = col_index;
                
                col_index++;
            }
        }

        if (store_id_idx == -1 || store_name_idx == -1 || branch_idx == -1 ||
            bags_idx == -1 || rating_idx == -1 || price_idx == -1 ||
            longitude_idx == -1 || latitude_idx == -1) {
            std::cerr << "Error: Missing required columns in CSV file." << std::endl;
            file.close();
            return false;
        }

        int row_num = 1;
        while (std::getline(file, line)) {
            if (line.empty()) continue;
            row_num++;
            std::stringstream ss(line);
            std::vector<std::string> values;
            std::string val;
            
            bool in_quotes = false;
            std::string current_val;
            for (char c : line) {
                if (c == '"') {
                    in_quotes = !in_quotes;
                } else if (c == ',' && !in_quotes) {
                    values.push_back(current_val);
                    current_val.clear();
                } else {
                    current_val += c;
                }
            }
            values.push_back(current_val);
            
            for (auto& v : values) {
                v.erase(0, v.find_first_not_of(" \t"));
                v.erase(v.find_last_not_of(" \t") + 1);
            }
            
            if (values.size() < header_columns.size()) {
                std::cerr << "Warning: Row " << row_num << " has fewer columns than header. Skipping." << std::endl;
                continue;
            }
            
            try {
                int store_id = std::stoi(values[store_id_idx]);
                std::string store_name = values[store_name_idx];
                std::string branch = values[branch_idx];
                int bags = std::stoi(values[bags_idx]);
                float rating = std::stof(values[rating_idx]);
                float price = std::stof(values[price_idx]);
                float longitude = std::stof(values[longitude_idx]);
                float latitude = std::stof(values[latitude_idx]);
                
                std::string business_type = "";
                if (business_type_idx != -1 && business_type_idx < (int)values.size()) {
                    business_type = values[business_type_idx];
                }
                
                Restaurant restaurant(store_id, store_name, branch, bags, rating, price,
                                     longitude, latitude, business_type);
                
                restaurants.push_back(restaurant);
            } catch (const std::exception& e) {
                std::cerr << "Warning: Error parsing row " << row_num << ": " << e.what() << ". Skipping." << std::endl;
                continue;
            }
        }

        file.close();
        std::cout << "Loaded " << restaurants.size() << " restaurants from " << filename << std::endl;
        return true;
    }

    static void generate_default_restaurants(std::vector<Restaurant>& restaurants) {
        restaurants.push_back(Restaurant(1, "Krispy Kreme", "Zamalek", 10, 4.8f, 80.0f, 31.22f, 30.05f, "bakery"));
        restaurants.push_back(Restaurant(2, "TBS Pizza", "New Cairo", 10, 4.2f, 150.0f, 31.25f, 30.08f, "restaurant"));
        restaurants.push_back(Restaurant(3, "Starbucks", "Zamalek", 15, 4.5f, 100.0f, 31.23f, 30.06f, "cafe"));
        restaurants.push_back(Restaurant(4, "Paul Bakery", "New Cairo", 12, 4.6f, 90.0f, 31.26f, 30.09f, "bakery"));
        restaurants.push_back(Restaurant(5, "Costa Coffee", "Zamalek", 8, 4.3f, 85.0f, 31.24f, 30.07f, "cafe"));
        restaurants.push_back(Restaurant(6, "Greggs", "New Cairo", 20, 4.0f, 70.0f, 31.27f, 30.10f, "bakery"));
        restaurants.push_back(Restaurant(7, "Pizza Hut", "Zamalek", 14, 4.1f, 140.0f, 31.21f, 30.04f, "restaurant"));
        restaurants.push_back(Restaurant(8, "Pret A Manger", "New Cairo", 18, 4.4f, 95.0f, 31.28f, 30.11f, "cafe"));
        restaurants.push_back(Restaurant(9, "Subway", "Zamalek", 16, 3.9f, 110.0f, 31.20f, 30.03f, "restaurant"));
        restaurants.push_back(Restaurant(10, "Tim Hortons", "New Cairo", 10, 4.2f, 80.0f, 31.29f, 30.12f, "cafe"));
        restaurants.push_back(Restaurant(11, "Dunkin Donuts", "Zamalek", 12, 4.3f, 75.0f, 31.19f, 30.02f, "bakery"));
        restaurants.push_back(Restaurant(12, "Domino's Pizza", "New Cairo", 15, 4.0f, 130.0f, 31.30f, 30.13f, "restaurant"));
        restaurants.push_back(Restaurant(13, "Cinnabon", "Zamalek", 9, 4.4f, 85.0f, 31.18f, 30.01f, "bakery"));
        restaurants.push_back(Restaurant(14, "Caribou Coffee", "New Cairo", 11, 4.1f, 90.0f, 31.31f, 30.14f, "cafe"));
        restaurants.push_back(Restaurant(15, "Panera Bread", "Zamalek", 13, 4.2f, 95.0f, 31.17f, 30.00f, "restaurant"));
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
    RankingAlgorithm ranking_algorithm;

public:
    SimulationEngine(int n_display, const std::string& customer_csv, 
                     RankingAlgorithm algorithm = RankingAlgorithm::BASELINE)
    : n_displayed(n_display),
      arrival_generator(customer_csv),
      ranking_algorithm(algorithm) {}

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
        std::string algo_name = (ranking_algorithm == RankingAlgorithm::SMART) ? "SMART" : "BASELINE";
        std::cout << "\n=== Starting Day Simulation (" << algo_name << " Algorithm) ===" << std::endl;
        std::cout << "Number of customers: " << num_customers << std::endl;
        std::cout << "Number of stores: " << market_state.restaurants.size() << std::endl;

        std::cout << "\nInitial Store Inventory:" << std::endl;
        for (const auto& r : market_state.restaurants) {
            std::cout << r.business_name << ": Estimated=" << r.estimated_bags
                << ", Actual=" << r.actual_bags
                << ", Price=$" << r.price_per_bag
                << ", Rating=" << std::fixed << std::setprecision(2) << r.get_rating() << std::endl;
        }

        auto arrival_times = arrival_generator.generate_arrival_times(num_customers);

        int successful_reservations = 0;
        for (int i = 0; i < num_customers; i++) {
            Customer customer = arrival_generator.generate_customer(i, market_state.restaurants);
            market_state.current_time = arrival_times[i];

            metrics_collector.log_customer_arrival(customer.id, arrival_times[i]);

            std::vector<int> displayed = get_displayed_stores(customer, market_state, n_displayed, ranking_algorithm);
            metrics_collector.log_stores_displayed(displayed);

            // Insert customer into map BEFORE passing reference
            market_state.customers.insert(std::make_pair(i, customer));

            int selected = CustomerDecisionSystem::process_customer_arrival(
                market_state.customers[i], market_state, n_displayed, ranking_algorithm);

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
        
        // Print rating changes summary
        std::cout << "\n=== RATING CHANGES (Dynamic Ratings) ===" << std::endl;
        for (const auto& r : market_state.restaurants) {
            float rating_change = r.get_rating() - r.initial_rating;
            std::cout << r.business_name << ": " << std::fixed << std::setprecision(2) 
                      << r.initial_rating << " -> " << r.get_rating() 
                      << " (" << (rating_change >= 0 ? "+" : "") << rating_change << ")"
                      << " [Confirmed: " << r.total_orders_confirmed 
                      << ", Cancelled: " << r.total_orders_cancelled << "]" << std::endl;
        }
    }

    void run_multi_day_simulation(int num_days, int num_customers_per_day) {
        std::string algo_name = (ranking_algorithm == RankingAlgorithm::SMART) ? "SMART" : "BASELINE";
        std::cout << "\n" << std::string(70, '=') << std::endl;
        std::cout << "=== Starting " << num_days << "-Day Simulation (" << algo_name << " Algorithm) ===" << std::endl;
        std::cout << "Number of customers per day: " << num_customers_per_day << std::endl;
        std::cout << "Number of stores: " << market_state.restaurants.size() << std::endl;
        std::cout << std::string(70, '=') << std::endl;

        // Store initial ratings for reference
        for (auto& r : market_state.restaurants) {
            r.initial_rating = r.general_ranking;
        }

        // Aggregate metrics across all days
        SimulationMetrics aggregated_metrics;
        
        for (int day = 1; day <= num_days; day++) {
            std::cout << "\n" << std::string(70, '-') << std::endl;
            std::cout << "DAY " << day << " of " << num_days << std::endl;
            std::cout << std::string(70, '-') << std::endl;

            // Reset daily state (but keep customer history and ratings)
            market_state.reservations.clear();
            market_state.current_time = Timestamp(8, 0);
            market_state.next_reservation_id = 1;
            
            // Reset restaurant daily state
            for (auto& restaurant : market_state.restaurants) {
                restaurant.reserved_count = 0;
                restaurant.has_inventory = true;
                // Generate new actual inventory for this day (80-120% of estimate)
                std::mt19937 rng(std::time(nullptr) + day);
                std::uniform_real_distribution<float> variance(0.8f, 1.2f);
                int actual = (int)(restaurant.estimated_bags * variance(rng));
                restaurant.set_actual_inventory(std::max(0, actual));
            }
            
            // Reset daily metrics (but we'll aggregate them)
            metrics_collector.metrics = SimulationMetrics();

            // Run day simulation
            run_day_simulation(num_customers_per_day);
            
            // Clear customers map after each day to avoid ID conflicts
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

            // Print day summary
            std::cout << "\nDay " << day << " Summary:" << std::endl;
            std::cout << "  Bags Sold: " << day_metrics.total_bags_sold << std::endl;
            std::cout << "  Waste: " << day_metrics.total_bags_unsold << std::endl;
            std::cout << "  Revenue: $" << std::fixed << std::setprecision(2) << day_metrics.total_revenue_generated << std::endl;
        }

        // Calculate final fairness metric from aggregated exposures
        std::vector<int> exposures;
        for (const auto& restaurant : market_state.restaurants) {
            exposures.push_back(aggregated_metrics.times_displayed_per_store[restaurant.business_id]);
        }
        if (!exposures.empty()) {
            std::sort(exposures.begin(), exposures.end());
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

        // Update metrics collector with aggregated metrics
        metrics_collector.metrics = aggregated_metrics;
        
        std::cout << "\n" << std::string(70, '=') << std::endl;
        std::cout << "=== " << num_days << "-DAY SIMULATION COMPLETE ===" << std::endl;
        std::cout << "Total Bags Sold: " << aggregated_metrics.total_bags_sold << std::endl;
        std::cout << "Total Waste: " << aggregated_metrics.total_bags_unsold << std::endl;
        std::cout << "Total Revenue: $" << std::fixed << std::setprecision(2) << aggregated_metrics.total_revenue_generated << std::endl;
        std::cout << "Fairness (Gini): " << std::fixed << std::setprecision(4) << aggregated_metrics.gini_coefficient_exposure << std::endl;
        std::cout << std::string(70, '=') << std::endl;
    }

    const SimulationMetrics& get_metrics() const {
        return metrics_collector.metrics;
    }

    void export_results(const std::string& filename) {
        std::string algo_name = (ranking_algorithm == RankingAlgorithm::SMART) ? "SMART" : "BASELINE";
        std::ofstream out(filename);
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
                << std::fixed << std::setprecision(2) 
                << metrics_collector.metrics.revenue_per_store[restaurant.business_id] << ","
                << metrics_collector.metrics.times_displayed_per_store[restaurant.business_id] << "\n";
        }

        out.close();
        std::cout << "\nResults exported to " << filename << std::endl;
    }
    
    void log_detailed_metrics(const SimulationMetrics* comparison_metrics = nullptr) {
        std::ofstream log("simulation_log.txt", std::ios::app);  // Append mode
        std::string algo_name = (ranking_algorithm == RankingAlgorithm::SMART) ? "SMART" : "BASELINE";
        
        log << "\n" << std::string(70, '=') << "\n";
        log << "=== DETAILED SIMULATION LOG - " << algo_name << " ALGORITHM ===\n";
        log << std::string(70, '=') << "\n\n";
        log << "Timestamp: " << std::time(nullptr) << "\n\n";
        
        log << "--- Overall Metrics ---\n";
        log << "Total Bags Sold: " << metrics_collector.metrics.total_bags_sold << "\n";
        log << "Total Bags Cancelled: " << metrics_collector.metrics.total_bags_cancelled << "\n";
        log << "Total Bags Unsold (Waste): " << metrics_collector.metrics.total_bags_unsold << "\n";
        log << "Total Revenue Generated: $" << std::fixed << std::setprecision(2) 
            << metrics_collector.metrics.total_revenue_generated << "\n";
        log << "Revenue Lost: $" << metrics_collector.metrics.total_revenue_lost << "\n";
        float revenue_efficiency = metrics_collector.metrics.total_customer_arrivals > 0 ? 
            (metrics_collector.metrics.total_revenue_generated / 
             (metrics_collector.metrics.total_revenue_generated + metrics_collector.metrics.total_revenue_lost)) * 100.0f : 0.0f;
        log << "Revenue Efficiency: " << std::fixed << std::setprecision(2) << revenue_efficiency << "%\n";
        log << "Customers Arrived: " << metrics_collector.metrics.total_customer_arrivals << "\n";
        log << "Customers Who Left: " << metrics_collector.metrics.customers_who_left << "\n";
        float conversion_rate = metrics_collector.metrics.total_customer_arrivals > 0 ? 
            ((float)(metrics_collector.metrics.total_customer_arrivals - metrics_collector.metrics.customers_who_left) / 
             metrics_collector.metrics.total_customer_arrivals) * 100.0f : 0.0f;
        log << "Conversion Rate: " << std::fixed << std::setprecision(2) << conversion_rate << "%\n";
        log << "Gini Coefficient (Fairness): " << std::fixed << std::setprecision(4) 
            << metrics_collector.metrics.gini_coefficient_exposure << "\n";
        log << "  (0 = perfect equality, 1 = maximum inequality)\n\n";
        
        log << "--- Per-Store Metrics ---\n";
        for (const auto& restaurant : market_state.restaurants) {
            log << "\n" << restaurant.business_name << " (ID: " << restaurant.business_id << "):\n";
            log << "  Initial Rating: " << std::fixed << std::setprecision(2) << restaurant.initial_rating << "\n";
            log << "  Final Rating: " << std::fixed << std::setprecision(2) << restaurant.get_rating() << "\n";
            log << "  Rating Change: " << std::fixed << std::setprecision(2) 
                << (restaurant.get_rating() - restaurant.initial_rating) << "\n";
            log << "  Orders Confirmed: " << restaurant.total_orders_confirmed << "\n";
            log << "  Orders Cancelled: " << restaurant.total_orders_cancelled << "\n";
            log << "  Estimated Bags: " << restaurant.estimated_bags << "\n";
            log << "  Actual Bags: " << restaurant.actual_bags << "\n";
            log << "  Bags Sold: " << metrics_collector.metrics.bags_sold_per_store[restaurant.business_id] << "\n";
            log << "  Bags Cancelled: " << metrics_collector.metrics.bags_cancelled_per_store[restaurant.business_id] << "\n";
            log << "  Waste: " << metrics_collector.metrics.waste_per_store[restaurant.business_id] << "\n";
            log << "  Revenue: $" << std::fixed << std::setprecision(2) 
                << metrics_collector.metrics.revenue_per_store[restaurant.business_id] << "\n";
            log << "  Times Displayed: " << metrics_collector.metrics.times_displayed_per_store[restaurant.business_id] << "\n";
        }
        
        // If comparison metrics provided, add comparison section
        if (comparison_metrics) {
            log << "\n" << std::string(70, '-') << "\n";
            log << "--- ALGORITHM COMPARISON ---\n";
            log << std::string(70, '-') << "\n\n";
            log << "Metric                          | " << algo_name << std::setw(12) << " | Other Algorithm | Difference\n";
            log << std::string(70, '-') << "\n";
            log << "Bags Sold                       | " << std::setw(12) << metrics_collector.metrics.total_bags_sold
                << " | " << std::setw(15) << comparison_metrics->total_bags_sold
                << " | " << (metrics_collector.metrics.total_bags_sold - comparison_metrics->total_bags_sold) << "\n";
            log << "Bags Cancelled                  | " << std::setw(12) << metrics_collector.metrics.total_bags_cancelled
                << " | " << std::setw(15) << comparison_metrics->total_bags_cancelled
                << " | " << (metrics_collector.metrics.total_bags_cancelled - comparison_metrics->total_bags_cancelled) << "\n";
            log << "Bags Unsold (Waste)             | " << std::setw(12) << metrics_collector.metrics.total_bags_unsold
                << " | " << std::setw(15) << comparison_metrics->total_bags_unsold
                << " | " << (metrics_collector.metrics.total_bags_unsold - comparison_metrics->total_bags_unsold) << "\n";
            log << "Revenue Generated               | $" << std::fixed << std::setprecision(2) << std::setw(10) 
                << metrics_collector.metrics.total_revenue_generated
                << " | $" << std::setw(13) << comparison_metrics->total_revenue_generated
                << " | $" << (metrics_collector.metrics.total_revenue_generated - comparison_metrics->total_revenue_generated) << "\n";
            log << "Revenue Lost                    | $" << std::setw(10) << metrics_collector.metrics.total_revenue_lost
                << " | $" << std::setw(13) << comparison_metrics->total_revenue_lost
                << " | $" << (metrics_collector.metrics.total_revenue_lost - comparison_metrics->total_revenue_lost) << "\n";
            log << "Customers Who Left              | " << std::setw(12) << metrics_collector.metrics.customers_who_left
                << " | " << std::setw(15) << comparison_metrics->customers_who_left
                << " | " << (metrics_collector.metrics.customers_who_left - comparison_metrics->customers_who_left) << "\n";
            log << "Gini Coefficient (Fairness)    | " << std::setprecision(4) << std::setw(12) 
                << metrics_collector.metrics.gini_coefficient_exposure
                << " | " << std::setw(15) << comparison_metrics->gini_coefficient_exposure
                << " | " << (metrics_collector.metrics.gini_coefficient_exposure - comparison_metrics->gini_coefficient_exposure) << "\n";
        }
        
        log.close();
        std::cout << "Detailed log written to simulation_log.txt" << std::endl;
    }
};

// ============================================================================
// MAIN FUNCTION
// ============================================================================
// ============================================================================
// MAIN FUNCTION
// ============================================================================
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
}