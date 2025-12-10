#include "CustomerDecisionSystem.h"
#include "RankingAlgorithms.h"
#include <algorithm>
#include <ctime>
#include <random>
#include <cmath>

int CustomerDecisionSystem::process_customer_arrival(Customer& customer,
                                                      MarketState& market_state,
                                                      int n_displayed,
                                                      RankingAlgorithm algorithm) {
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

std::vector<float> CustomerDecisionSystem::calculate_store_scores(
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

int CustomerDecisionSystem::select_store(const Customer& customer,
                                         const std::vector<int>& displayed_store_ids,
                                         const std::vector<float>& scores,
                                         const MarketState& market_state) {
    if (scores.empty()) return -1;

    float base_threshold = customer.leaving_threshold;
    float loyalty_adjustment = (1.0f - customer.loyalty) * 2.0f;
    float threshold = base_threshold + loyalty_adjustment;

    float max_score = *std::max_element(scores.begin(), scores.end());
    if (max_score < threshold) {
        return -1;
    }

    std::vector<float> adjusted_scores = scores;
    for (size_t i = 0; i < displayed_store_ids.size(); i++) {
        int store_id = displayed_store_ids[i];
        
        // Adjust based on customer history
        auto it = customer.history.store_interactions.find(store_id);
        if (it != customer.history.store_interactions.end()) {
            const auto& interaction = it->second;
            if (interaction.reservations > 0) {
                float success_rate = (float)interaction.successes / interaction.reservations;
                adjusted_scores[i] += success_rate * 1.5f;
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
    
    return probabilistic_select(displayed_store_ids, adjusted_scores, valid_indices, valid_scores);
}

int CustomerDecisionSystem::probabilistic_select(
    const std::vector<int>& store_ids,
    const std::vector<float>& all_scores,
    const std::vector<int>& valid_indices,
    const std::vector<float>& valid_scores) {
    
    static std::mt19937 rng(std::time(nullptr));
    
    float min_score = *std::min_element(valid_scores.begin(), valid_scores.end());
    float temperature = 2.0f;
    
    std::vector<float> probabilities;
    float sum_exp = 0.0f;
    
    for (float score : valid_scores) {
        float shifted_score = score - min_score + 1.0f;
        float exp_score = std::exp(shifted_score / temperature);
        probabilities.push_back(exp_score);
        sum_exp += exp_score;
    }
    
    for (float& prob : probabilities) {
        prob /= sum_exp;
    }
    
    std::uniform_real_distribution<float> dist(0.0f, 1.0f);
    float random_val = dist(rng);
    float cumulative = 0.0f;
    
    for (size_t i = 0; i < probabilities.size(); i++) {
        cumulative += probabilities[i];
        if (random_val <= cumulative) {
            return store_ids[valid_indices[i]];
        }
    }
    
    return store_ids[valid_indices.back()];
}

bool CustomerDecisionSystem::create_reservation(Customer& customer,
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

