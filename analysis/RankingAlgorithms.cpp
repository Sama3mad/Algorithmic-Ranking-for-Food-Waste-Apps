#include "RankingAlgorithms.h"
#include <algorithm>
#include <set>

std::vector<int> get_displayed_stores_baseline(const Customer& customer,
                                                const MarketState& market_state,
                                                int n_displayed) {
    std::vector<int> available = market_state.get_available_restaurant_ids();

    std::sort(available.begin(), available.end(),
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

    int num_to_show = std::min(n_displayed, (int)available.size());
    return std::vector<int>(available.begin(), available.begin() + num_to_show);
}

std::vector<int> get_displayed_stores_smart(const Customer& customer,
                                            const MarketState& market_state,
                                            int n_displayed) {
    std::vector<int> available = market_state.get_available_restaurant_ids();
    if (available.empty()) return available;

    std::vector<int> result;
    std::set<int> selected;

    // ========================================================================
    // ENHANCED SMART ALGORITHM: Segment-Aware + Adaptive Personalization
    // Strategy: Use customer segment to optimize ranking for each customer type
    // ========================================================================
    
    // Determine segment-specific parameters
    bool is_budget = (customer.segment == "budget");
    bool is_premium = (customer.segment == "premium");
    bool is_regular = (customer.segment == "regular");
    
    // Segment-specific scoring weights
    float segment_rating_weight = is_premium ? 1.5f : (is_budget ? 0.8f : 1.0f);
    float segment_price_weight = is_budget ? 1.3f : (is_premium ? 0.7f : 1.0f);
    float segment_inventory_weight = is_budget ? 0.8f : (is_premium ? 0.5f : 0.6f);  // Budget cares more about availability
    
    // Calculate comprehensive scores for all stores with segment awareness
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
                // Budget customers get bonus for stores below their WTP
                if (store->price_per_bag < customer.willingness_to_pay) {
                    float price_savings = (customer.willingness_to_pay - store->price_per_bag) / customer.willingness_to_pay;
                    price_bonus = price_savings * 0.4f;  // Up to 0.4 bonus for good prices
                }
            } else if (is_premium) {
                // Premium customers slightly prefer higher-priced stores (quality signal)
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
                history_bonus = success_rate * 0.5f;  // Up to 0.5 bonus for good history
                
                // Strong penalty for stores with cancellation history
                if (hist_it->second.cancellations > 0) {
                    float cancel_rate = (float)hist_it->second.cancellations / hist_it->second.reservations;
                    history_bonus -= cancel_rate * 1.0f;  // Strong penalty for cancellations
                }
            }
            
            // Segment-specific category preference bonus
            float category_bonus = 0.0f;
            auto cat_it = customer.category_preference.find(store->business_type);
            if (cat_it != customer.category_preference.end()) {
                category_bonus = cat_it->second * 0.2f;  // Bonus for preferred categories
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
    // Budget customers: More personalized (they're price-sensitive, need what they want)
    // Premium customers: Can show more variety (they're more adventurous)
    // Regular customers: Balanced approach
    float base_personalization = is_budget ? 0.7f : (is_premium ? 0.5f : 0.6f);
    float loyalty_adjustment = customer.loyalty * 0.15f;  // Loyal customers can handle more variety
    float personalization_ratio = base_personalization + loyalty_adjustment;
    personalization_ratio = std::min(0.85f, std::max(0.5f, personalization_ratio));  // Clamp between 0.5-0.85
    
    int personalized_count = std::min((int)(n_displayed * personalization_ratio), (int)store_scores.size());
    personalized_count = std::max(3, personalized_count);  // At least 3 personalized
    
    // STEP 1: Select personalized stores (customer preferences + inventory + rating)
    for (int i = 0; i < personalized_count && result.size() < n_displayed; i++) {
        int store_id = store_scores[i].first;
        result.push_back(store_id);
        selected.insert(store_id);
    }

    // STEP 2: Segment-Aware Smart Discovery
    // Budget: Show new stores only if they're cheap AND good quality
    // Premium: Show new stores if they're high-rated (adventurous)
    // Regular: Balanced approach
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
                    bool meets_threshold = false;
                    float quality_score = 0.0f;
                    
                    if (is_budget) {
                        // Budget: Must be affordable AND have good value AND safe inventory
                        if (store->price_per_bag <= customer.willingness_to_pay * 1.1f && 
                            store->estimated_bags >= 8 && store->get_rating() >= 3.8f) {
                            float value_ratio = store->get_rating() / store->price_per_bag;
                            float price_affordability = (customer.willingness_to_pay - store->price_per_bag) / customer.willingness_to_pay;
                            float inventory_safety = std::min(1.0f, (float)store->estimated_bags / 15.0f);
                            quality_score = value_ratio * 15.0f + price_affordability * 2.0f + inventory_safety * 0.5f + store->get_rating() * 0.3f;
                            meets_threshold = true;
                        }
                    } else if (is_premium) {
                        // Premium: Must be high-rated (4.0+) AND have good inventory
                        if (store->get_rating() >= 4.0f && store->estimated_bags >= 8) {
                            float value_score = store->get_rating() / store->price_per_bag;
                            float inventory_bonus = std::min(1.0f, (float)store->estimated_bags / 15.0f);
                            quality_score = store->get_rating() * 1.5f + value_score * 10.0f + inventory_bonus * 0.5f;
                            meets_threshold = true;
                        }
                    } else {
                        // Regular: Balanced - good rating (3.9+) AND safe inventory
                        if (store->get_rating() >= 3.9f && store->estimated_bags >= 8) {
                            float value_score = store->get_rating() / store->price_per_bag;
                            float inventory_bonus = std::min(1.0f, (float)store->estimated_bags / 15.0f);
                            quality_score = store->get_rating() + value_score * 10.0f + inventory_bonus * 0.5f;
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
            std::sort(quality_new_stores.begin(), quality_new_stores.end(),
                [](const std::pair<int, float>& a, const std::pair<int, float>& b) {
                    return a.second > b.second;
                });
            
            // Select the BEST quality new store for this segment
            result.push_back(quality_new_stores[0].first);
            selected.insert(quality_new_stores[0].first);
        }
    }

    // STEP 3: Segment-Aware Price-Competitive Selection
    // Budget: Prioritize best value (rating/price) with affordability
    // Premium: Show competitive stores but prioritize quality
    // Regular: Balanced value approach
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
                // Must be safe (inventory >= 8)
                if (store->estimated_bags >= 8) {
                    float value_ratio = store->get_rating() / store->price_per_bag;
                    float inventory_safety = std::min(1.0f, (float)store->estimated_bags / 15.0f);
                    float competitive_score = 0.0f;
                    bool is_competitive = false;
                    
                    if (is_budget) {
                        // Budget: Must be affordable AND have good value
                        if (store->price_per_bag <= customer.willingness_to_pay * 1.1f && value_ratio > 0.025f) {
                            float price_affordability = (customer.willingness_to_pay - store->price_per_bag) / customer.willingness_to_pay;
                            competitive_score = value_ratio * 120.0f + price_affordability * 3.0f + inventory_safety * 0.5f + store->get_rating() * 0.3f;
                            is_competitive = true;
                        }
                    } else if (is_premium) {
                        // Premium: Good value but also high quality
                        if (value_ratio > 0.03f && store->get_rating() >= 3.8f) {
                            competitive_score = value_ratio * 100.0f + inventory_safety * 0.5f + store->get_rating() * 0.8f;
                            is_competitive = true;
                        }
                    } else {
                        // Regular: Good value threshold
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
            std::sort(competitive_stores.begin(), competitive_stores.end(),
                [](const std::pair<int, float>& a, const std::pair<int, float>& b) {
                    return a.second > b.second;
                });
            
            result.push_back(competitive_stores[0].first);
            selected.insert(competitive_stores[0].first);
        }
    }

    // STEP 4: Fill remaining slots with best available (prioritize customer satisfaction)
    // This ensures we always show n_displayed stores if available
    for (size_t i = 0; i < store_scores.size() && result.size() < n_displayed; i++) {
        int store_id = store_scores[i].first;
        if (selected.find(store_id) == selected.end()) {
            result.push_back(store_id);
            selected.insert(store_id);
        }
    }

    return result;
}

std::vector<int> get_displayed_stores(const Customer& customer,
                                      const MarketState& market_state,
                                      int n_displayed,
                                      RankingAlgorithm algorithm) {
    if (algorithm == RankingAlgorithm::SMART) {
        return get_displayed_stores_smart(customer, market_state, n_displayed);
    } else {
        return get_displayed_stores_baseline(customer, market_state, n_displayed);
    }
}

