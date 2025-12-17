#include "RankingAlgorithms.h"
#include <algorithm>
#include <set>
#include <cmath>
#include <limits>
#include "Customer.h"

using namespace std;

// Distance threshold defined in Customer.cpp
extern const float MAX_TRAVEL_DISTANCE;

// Baseline Algorithm: Just sorts by rating
vector<int> get_displayed_stores_baseline(const Customer& customer,
                                                const MarketState& market_state,
                                                int n_displayed) {
    vector<int> available = market_state.get_available_restaurant_ids();

    sort(available.begin(), available.end(),
        [&market_state](int a, int b) {
            const Restaurant* r1 = nullptr;
            const Restaurant* r2 = nullptr;

            for (const auto& r : market_state.restaurants) {
                if (r.business_id == a) r1 = &r;
                if (r.business_id == b) r2 = &r;
            }

            if (!r1 || !r2) return false;
            return r1->get_rating() > r2->get_rating();
        });

    int num_to_show = min(n_displayed, (int)available.size());
    return vector<int>(available.begin(), available.begin() + num_to_show);
}

// Sama's Algorithm: Complex multi-objective optimization
// Balances personalization, waste reduction, fairness, and revenue
vector<int> get_displayed_stores_sama(const Customer& customer,
                                            const MarketState& market_state,
                                            int n_displayed) {
    vector<int> available = market_state.get_available_restaurant_ids();
    if (available.empty()) return available;

    vector<int> result;
    set<int> selected;

    // Determine segment characteristics
    bool is_budget = (customer.segment == "budget");
    bool is_premium = (customer.segment == "premium");
    bool is_regular = (customer.segment == "regular");
    
    // Segment weights
    float segment_rating_weight = is_premium ? 1.5f : (is_budget ? 0.8f : 1.0f);
    float segment_price_weight = is_budget ? 1.3f : (is_premium ? 0.7f : 1.0f);
    float segment_inventory_weight = is_budget ? 0.8f : (is_premium ? 0.5f : 0.6f);
    
    // Calculate comprehensive scores
    vector<pair<int, float>> store_scores;
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
            
            // Inventory urgency (reduce waste)
            int unsold_bags = max(0, store->estimated_bags - store->reserved_count);
            float inventory_urgency = min(1.0f, (float)unsold_bags / 15.0f);
            float inventory_bonus = inventory_urgency * 1.2f * segment_inventory_weight;
            
            // Distance (partially covered in base_score)
            float distance_bonus = 0.0f;
            
            // Rating bonus
            float rating_bonus = (store->get_rating() - 3.5f) * 0.3f * segment_rating_weight;
            if (rating_bonus < 0) rating_bonus = 0;
            
            // Price bonus based on segment
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
            
            // Interaction history bonus
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
            
            // Category preference bonus
            float category_bonus = 0.0f;
            auto cat_it = customer.category_preference.find(store->business_type);
            if (cat_it != customer.category_preference.end()) {
                category_bonus = cat_it->second * 0.2f;
            }
            
            // High waste reduction priority
            float waste_reduction_bonus = 0.0f;
            if (unsold_bags > 5) {
                waste_reduction_bonus = min(2.0f, (float)unsold_bags / 5.0f) * 0.5f;
            }
            
            // Revenue optimization
            float revenue_potential = store->price_per_bag * inventory_urgency;
            float revenue_bonus = (revenue_potential / 200.0f) * 0.3f;
            
            float final_score = base_score + inventory_bonus + rating_bonus + price_bonus + 
                              history_bonus + category_bonus + distance_bonus + 
                              waste_reduction_bonus + revenue_bonus;
            store_scores.push_back({store_id, final_score});
        }
    }

    sort(store_scores.begin(), store_scores.end(),
        [](const pair<int, float>& a, const pair<int, float>& b) {
            return a.second > b.second;
        });

    // Adaptive personalization logic
    float base_personalization = is_budget ? 0.7f : (is_premium ? 0.5f : 0.6f);
    float loyalty_adjustment = customer.loyalty * 0.15f;
    
    // Check overall market waste status
    float total_unsold = 0.0f;
    int stores_with_inventory = 0;
    for (const auto& r : market_state.restaurants) {
        int unsold = max(0, r.estimated_bags - r.reserved_count);
        if (unsold > 0) {
            total_unsold += unsold;
            stores_with_inventory++;
        }
    }
    float avg_unsold = stores_with_inventory > 0 ? total_unsold / stores_with_inventory : 0.0f;
    
    // Adjust personalization if waste is high
    float waste_adjustment = (avg_unsold > 10.0f) ? -0.1f : 0.0f;
    
    float personalization_ratio = base_personalization + loyalty_adjustment + waste_adjustment;
    personalization_ratio = min(0.85f, max(0.4f, personalization_ratio));
    
    int personalized_count = min((int)(n_displayed * personalization_ratio), (int)store_scores.size());
    personalized_count = max(3, personalized_count);
    
    // SELECT 1: Personalized stores
    for (int i = 0; i < personalized_count && result.size() < n_displayed; i++) {
        int store_id = store_scores[i].first;
        result.push_back(store_id);
        selected.insert(store_id);
    }

    // SELECT 2: Discovery stores (segment-aware)
    if (result.size() < n_displayed) {
        vector<pair<int, float>> quality_new_stores;
        for (int store_id : available) {
            if (selected.find(store_id) != selected.end()) continue;
            
            // Is it a new store for this customer?
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
                    bool meets_threshold = false;
                    float quality_score = 0.0f;
                    
                    int unsold = max(0, store->estimated_bags - store->reserved_count);
                    float unsold_bonus = min(1.0f, (float)unsold / 10.0f);
                    
                    if (is_budget) {
                        if (store->price_per_bag <= customer.willingness_to_pay * 1.1f && 
                            store->estimated_bags >= 8 && store->get_rating() >= 3.8f) {
                            float value_ratio = store->get_rating() / store->price_per_bag;
                            float price_affordability = (customer.willingness_to_pay - store->price_per_bag) / customer.willingness_to_pay;
                            float inventory_safety = min(1.0f, (float)store->estimated_bags / 15.0f);
                            quality_score = value_ratio * 15.0f + price_affordability * 2.0f + 
                                          inventory_safety * 0.5f + store->get_rating() * 0.3f + unsold_bonus * 0.8f;
                            meets_threshold = true;
                        }
                    } else if (is_premium) {
                        if (store->get_rating() >= 4.0f && store->estimated_bags >= 8) {
                            float value_score = store->get_rating() / store->price_per_bag;
                            float inventory_bonus = min(1.0f, (float)store->estimated_bags / 15.0f);
                            quality_score = store->get_rating() * 1.5f + value_score * 10.0f + 
                                          inventory_bonus * 0.5f + unsold_bonus * 0.6f;
                            meets_threshold = true;
                        }
                    } else {
                        if (store->get_rating() >= 3.9f && store->estimated_bags >= 8) {
                            float value_score = store->get_rating() / store->price_per_bag;
                            float inventory_bonus = min(1.0f, (float)store->estimated_bags / 15.0f);
                            quality_score = store->get_rating() + value_score * 10.0f + 
                                          inventory_bonus * 0.5f + unsold_bonus * 0.7f;
                            meets_threshold = true;
                        }
                    }
                    
                    if (meets_threshold) {
                        quality_new_stores.push_back({store_id, quality_score});
                    }
                }
            }
        }
        
        if (!quality_new_stores.empty()) {
            sort(quality_new_stores.begin(), quality_new_stores.end(),
                [](const pair<int, float>& a, const pair<int, float>& b) {
                    return a.second > b.second;
                });
            
            result.push_back(quality_new_stores[0].first);
            selected.insert(quality_new_stores[0].first);
        }
    }

    // SELECT 3: Price-competitive selection
    if (result.size() < n_displayed) {
        vector<pair<int, float>> competitive_stores;
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
                if (store->estimated_bags >= 8) {
                    float value_ratio = store->get_rating() / store->price_per_bag;
                    float inventory_safety = min(1.0f, (float)store->estimated_bags / 15.0f);
                    float competitive_score = 0.0f;
                    bool is_competitive = false;
                    
                    if (is_budget) {
                        if (store->price_per_bag <= customer.willingness_to_pay * 1.1f && value_ratio > 0.025f) {
                            float price_affordability = (customer.willingness_to_pay - store->price_per_bag) / customer.willingness_to_pay;
                            competitive_score = value_ratio * 120.0f + price_affordability * 3.0f + inventory_safety * 0.5f + store->get_rating() * 0.3f;
                            is_competitive = true;
                        }
                    } else if (is_premium) {
                        if (value_ratio > 0.03f && store->get_rating() >= 3.8f) {
                            competitive_score = value_ratio * 100.0f + inventory_safety * 0.5f + store->get_rating() * 0.8f;
                            is_competitive = true;
                        }
                    } else {
                        if (value_ratio > 0.03f) {
                            competitive_score = value_ratio * 100.0f + inventory_safety * 0.5f + store->get_rating() * 0.5f;
                            is_competitive = true;
                        }
                    }
                    
                    if (is_competitive) {
                        competitive_stores.push_back({store_id, competitive_score});
                    }
                }
            }
        }
        
        if (!competitive_stores.empty()) {
            sort(competitive_stores.begin(), competitive_stores.end(),
                [](const pair<int, float>& a, const pair<int, float>& b) {
                    return a.second > b.second;
                });
            
            result.push_back(competitive_stores[0].first);
            selected.insert(competitive_stores[0].first);
        }
    }

    // SELECT 4: Fill remaining with best available
    for (size_t i = 0; i < store_scores.size() && result.size() < n_displayed; i++) {
        int store_id = store_scores[i].first;
        if (selected.find(store_id) == selected.end()) {
            result.push_back(store_id);
            selected.insert(store_id);
        }
    }

    return result;
}

// Distance helper
static float calculate_distance(float lat1, float lon1, float lat2, float lon2) {
    float dlat = lat2 - lat1;
    float dlon = lon2 - lon1;
    return sqrt(dlat * dlat + dlon * dlon);
}

// Andrew's Algorithm: Prioritizes Fairness using impression counts
vector<int> get_displayed_stores_andrew(const Customer& customer,
                                             MarketState& market_state,
                                             int n_displayed) {
    vector<int> available = market_state.get_available_restaurant_ids();
    if (available.empty()) return available;

    vector<pair<int, float>> store_scores;
    
    for (int store_id : available) {
        const Restaurant* store = market_state.get_restaurant(store_id);
        if (!store) continue;
        
        float base_score = customer.calculate_store_score(*store);
        
        // Dampen score if store has been shown many times
        int impressions = market_state.impression_counts[store_id];
        float damping_factor = log(impressions + 1.0f) + 1.0f;
        float adjusted_score = base_score / damping_factor;
        
        store_scores.push_back({store_id, adjusted_score});
    }
    
    sort(store_scores.begin(), store_scores.end(),
        [](const pair<int, float>& a, const pair<int, float>& b) {
            return a.second > b.second;
        });
    
    int num_to_show = min(n_displayed, (int)store_scores.size());
    vector<int> result;
    for (int i = 0; i < num_to_show; i++) {
        result.push_back(store_scores[i].first);
    }
    
    return result;
}

// Amer's Algorithm: Prioritizes closest store first
vector<int> get_displayed_stores_amer(const Customer& customer,
                                           const MarketState& market_state,
                                           int n_displayed) {
    vector<int> available = market_state.get_available_restaurant_ids();
    if (available.empty()) return available;

    vector<int> result;
    set<int> selected;
    
    // Step 1: Find absolute closest store
    int closest_store_id = -1;
    float min_distance = numeric_limits<float>::max();
    
    for (int store_id : available) {
        const Restaurant* store = market_state.get_restaurant(store_id);
        if (!store) continue;
        
        float distance = calculate_distance(customer.latitude, customer.longitude,
                                           store->latitude, store->longitude);
        
        if (distance < min_distance && distance <= MAX_TRAVEL_DISTANCE) {
            min_distance = distance;
            closest_store_id = store_id;
        }
    }
    
    if (closest_store_id != -1) {
        result.push_back(closest_store_id);
        selected.insert(closest_store_id);
    }
    
    // Step 2: Score remaining, penalizing price and distance heavily
    vector<pair<int, float>> store_scores;
    
    for (int store_id : available) {
        if (selected.find(store_id) != selected.end()) continue;
        
        const Restaurant* store = market_state.get_restaurant(store_id);
        if (!store) continue;
        
        float distance = calculate_distance(customer.latitude, customer.longitude,
                                           store->latitude, store->longitude);
        if (distance > MAX_TRAVEL_DISTANCE) continue;
        
        float base_score = customer.calculate_store_score(*store);
        float price_penalty = store->price_per_bag * 0.01f;
        float distance_penalty = distance * 20.0f;
        
        float final_score = base_score - price_penalty - distance_penalty;
        
        store_scores.push_back({store_id, final_score});
    }
    
    sort(store_scores.begin(), store_scores.end(),
        [](const pair<int, float>& a, const pair<int, float>& b) {
            return a.second > b.second;
        });
    
    int remaining = n_displayed - (int)result.size();
    for (int i = 0; i < remaining && i < (int)store_scores.size(); i++) {
        result.push_back(store_scores[i].first);
        selected.insert(store_scores[i].first);
    }
    
    return result;
}

// Ziad's Algorithm: Weighted linear combination (Price, Rating, Unsold)
vector<int> get_displayed_stores_ziad(const Customer& customer,
                                            const MarketState& market_state,
                                            int n_displayed) {
    vector<int> available = market_state.get_available_restaurant_ids();
    if (available.empty()) return available;

    const float price_weight = -0.01f;
    const float rating_weight = 1.5f;
    const float unsold_weight = 0.1f;

    vector<pair<int, float>> store_scores;
    
    for (int store_id : available) {
        const Restaurant* store = market_state.get_restaurant(store_id);
        if (!store) continue;
        
        float distance = calculate_distance(customer.latitude, customer.longitude,
                                           store->latitude, store->longitude);
        if (distance > MAX_TRAVEL_DISTANCE) continue;
        
        int unsold_bags = max(0, store->estimated_bags - store->reserved_count);
        
        float score = (price_weight * store->price_per_bag) + 
                      (rating_weight * store->get_rating()) + 
                      (unsold_weight * unsold_bags);
        
        store_scores.push_back({store_id, score});
    }
    
    sort(store_scores.begin(), store_scores.end(),
        [](const pair<int, float>& a, const pair<int, float>& b) {
            return a.second > b.second;
        });
    
    int num_to_show = min(n_displayed, min(5, (int)store_scores.size()));
    vector<int> result;
    for (int i = 0; i < num_to_show; i++) {
        result.push_back(store_scores[i].first);
    }
    
    return result;
}

// Harmony Algorithm: The final/best strategy combining all strengths
vector<int> get_displayed_stores_harmony(const Customer& customer,
                                         MarketState& market_state,
                                         int n_displayed) {
    vector<int> available = market_state.get_available_restaurant_ids();
    if (available.empty()) return available;

    vector<int> result;
    set<int> selected;

    // STEP 1: Calculate scores for all stores
    vector<pair<int, float>> store_scores;
    
    // Average impressions for fairness calculation
    float total_impressions = 0.0f;
    int store_count = 0;
    for (const auto& pair : market_state.impression_counts) {
        total_impressions += pair.second;
        store_count++;
    }
    float avg_impressions = store_count > 0 ? total_impressions / store_count : 1.0f;
    
    for (int store_id : available) {
        const Restaurant* store = market_state.get_restaurant(store_id);
        if (!store) continue;
        
        float distance = calculate_distance(
            customer.latitude, customer.longitude,
            store->latitude, store->longitude
        );
        if (distance > MAX_TRAVEL_DISTANCE) continue;
        
        float base_score = customer.calculate_store_score(*store);
        
        // COMPONENT 1: Satisfaction bonus
        float satisfaction_bonus = 0.0f;
        if (customer.segment == "premium" && store->get_rating() >= 4.0f) {
            satisfaction_bonus = 0.5f;
        } else if (customer.segment == "budget" && 
                   store->price_per_bag <= customer.willingness_to_pay) {
            satisfaction_bonus = 0.4f;
        } else if (customer.segment == "regular" && store->get_rating() >= 3.8f) {
            satisfaction_bonus = 0.3f;
        }
        
        // History bonus
        auto hist_it = customer.history.store_interactions.find(store_id);
        if (hist_it != customer.history.store_interactions.end() && 
            hist_it->second.successes > 0) {
            float success_rate = (float)hist_it->second.successes / hist_it->second.reservations;
            satisfaction_bonus += success_rate * 0.3f;
        }
        
        // COMPONENT 2: Waste reduction
        int unsold_bags = max(0, store->estimated_bags - store->reserved_count);
        float waste_bonus = unsold_bags * 0.08f;
        if (unsold_bags > 12) {
            waste_bonus += 0.6f;
        }
        
        // COMPONENT 3: Fairness
        int impressions = market_state.impression_counts[store_id];
        float fairness_boost = 0.0f;
        
        if (impressions < avg_impressions * 0.5f) {
            fairness_boost = 0.8f; // Boost underexposed
        } else if (impressions > avg_impressions * 1.5f) {
            fairness_boost = -0.4f; // Dampen overexposed
        }
        
        // COMPONENT 4: Revenue potential
        float inventory_safety = min(1.0f, (float)store->estimated_bags / 10.0f);
        float revenue_bonus = (store->price_per_bag / 100.0f) * inventory_safety * 0.3f;
        
        // COMPONENT 5: Quality assurance
        float quality_penalty = 0.0f;
        if (store->estimated_bags < 5) {
            quality_penalty = -1.5f;
        }
        
        float final_score = base_score + satisfaction_bonus + waste_bonus + 
                           fairness_boost + revenue_bonus + quality_penalty;
        
        store_scores.push_back({store_id, final_score});
    }
    
    sort(store_scores.begin(), store_scores.end(),
        [](const pair<int, float>& a, const pair<int, float>& b) {
            return a.second > b.second;
        });
    
    // STEP 2: Fill top 70% slots strictly by score
    int direct_slots = (int)(n_displayed * 0.7f);
    for (int i = 0; i < direct_slots && i < (int)store_scores.size(); i++) {
        int store_id = store_scores[i].first;
        result.push_back(store_id);
        selected.insert(store_id);
    }
    
    // STEP 3: Add one high-waste store
    if (result.size() < n_displayed) {
        for (const auto& pair : store_scores) {
            if (selected.find(pair.first) != selected.end()) continue;
            
            const Restaurant* store = market_state.get_restaurant(pair.first);
            if (store) {
                int unsold = max(0, store->estimated_bags - store->reserved_count);
                if (unsold >= 10) {
                    result.push_back(pair.first);
                    selected.insert(pair.first);
                    break;
                }
            }
        }
    }
    
    // STEP 4: Add one discovery store
    if (result.size() < n_displayed) {
        for (const auto& pair : store_scores) {
            if (selected.find(pair.first) != selected.end()) continue;
            
            auto hist_it = customer.history.store_interactions.find(pair.first);
            bool is_new = (hist_it == customer.history.store_interactions.end() || 
                          hist_it->second.reservations == 0);
            
            if (is_new) {
                const Restaurant* store = market_state.get_restaurant(pair.first);
                if (store && store->get_rating() >= 3.8f && store->estimated_bags >= 6) {
                    result.push_back(pair.first);
                    selected.insert(pair.first);
                    break;
                }
            }
        }
    }
    
    // STEP 5: Fill remaining with best available
    for (const auto& pair : store_scores) {
        if (result.size() >= n_displayed) break;
        if (selected.find(pair.first) == selected.end()) {
            result.push_back(pair.first);
            selected.insert(pair.first);
        }
    }
    
    // Track impressions
    for (int store_id : result) {
        market_state.impression_counts[store_id]++;
    }
    
    return result;
}

// Dispatch function
vector<int> get_displayed_stores(const Customer& customer,
                                      MarketState& market_state,
                                      int n_displayed,
                                      RankingAlgorithm algorithm) {
    if (algorithm == RankingAlgorithm::SAMA) {
        return get_displayed_stores_sama(customer, market_state, n_displayed);
    } else if (algorithm == RankingAlgorithm::ANDREW) {
        return get_displayed_stores_andrew(customer, market_state, n_displayed);
    } else if (algorithm == RankingAlgorithm::AMER) {
        return get_displayed_stores_amer(customer, market_state, n_displayed);
    } else if (algorithm == RankingAlgorithm::ZIAD) {
        return get_displayed_stores_ziad(customer, market_state, n_displayed);
    } else if (algorithm == RankingAlgorithm::HARMONY) {
        return get_displayed_stores_harmony(customer, market_state, n_displayed);
    } else {
        return get_displayed_stores_baseline(customer, market_state, n_displayed);
    }
}

