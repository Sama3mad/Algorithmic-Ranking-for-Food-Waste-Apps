#include "CustomerDecisionSystem.h"
#include "RankingAlgorithms.h"
#include <algorithm>
#include <ctime>
#include <random>
#include <cmath>

using namespace std;

// Process a customer arrival event
// Returns the selected store ID, or -1 if no store was selected
int CustomerDecisionSystem::process_customer_arrival(Customer& customer,
                                                      MarketState& market_state,
                                                      int n_displayed,
                                                      RankingAlgorithm algorithm) {
    customer.record_visit();
    vector<int> displayed = get_displayed_stores(customer, market_state, n_displayed, algorithm);
    
    // Track impressions for fairness algorithm
    for (int store_id : displayed) {
        market_state.impression_counts[store_id]++;
    }

    // Customer leaves if no stores are shown
    if (displayed.empty()) {
        customer.churned = true;
        return -1;
    }

    // Calculate customer's score for each displayed store
    vector<float> scores = calculate_store_scores(customer, displayed, market_state);
    
    // Customer selects a store based on scores and probabilities
    int selected = select_store(customer, displayed, scores, market_state);

    if (selected == -1) {
        customer.churned = true; // Customer leaves platform
        return -1;
    }

    // Try to create the reservation
    bool success = create_reservation(customer, selected, market_state);
    if (!success) {
        customer.churned = true;
        return -1;
    }

    return selected;
}

// Calculate scores for a list of stores
vector<float> CustomerDecisionSystem::calculate_store_scores(
    const Customer& customer,
    const vector<int>& displayed_store_ids,
    const MarketState& market_state) {
    
    vector<float> scores;
    for (int store_id : displayed_store_ids) {
        const Restaurant* store = market_state.get_restaurant(store_id);
        if (store) {
            float score = customer.calculate_store_score(*store);
            scores.push_back(score);
        } else {
            scores.push_back(-100.0f);  // Invalid store
        }
    }
    return scores;
}

// Select a store from the displayed options
int CustomerDecisionSystem::select_store(const Customer& customer,
                                         const vector<int>& displayed_store_ids,
                                         const vector<float>& scores,
                                         const MarketState& market_state) {
    if (scores.empty()) return -1;

    // Determine the minimum score required to make a purchase
    float base_threshold = customer.leaving_threshold;
    float loyalty_adjustment = (1.0f - customer.loyalty) * 2.0f;
    float threshold = base_threshold + loyalty_adjustment;

    // If best option is too poor, customer leaves
    float max_score = *max_element(scores.begin(), scores.end());
    if (max_score < threshold) {
        return -1;
    }

    // Adjust scores based on history and inventory safety
    vector<float> adjusted_scores = scores;
    for (size_t i = 0; i < displayed_store_ids.size(); i++) {
        int store_id = displayed_store_ids[i];
        
        // History adjustment (boost successful past stores, penalize cancellations)
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
        
        // Inventory safety bonus: customers prefer stores that likely have stock
        const Restaurant* store = market_state.get_restaurant(store_id);
        if (store) {
            float inventory_safety = min(1.0f, (float)store->estimated_bags / 12.0f);
            adjusted_scores[i] += inventory_safety * 0.3f;
        }
    }

    // Filter valid options (meeting threshold and distance check)
    vector<int> valid_indices;
    vector<float> valid_scores;
    for (size_t i = 0; i < adjusted_scores.size(); i++) {
        // Score < -50 means too far
        if (adjusted_scores[i] >= threshold && adjusted_scores[i] > -50.0f) {
            valid_indices.push_back(i);
            valid_scores.push_back(adjusted_scores[i]);
        }
    }
    
    if (valid_scores.empty()) {
        return -1;
    }
    
    // Choose probabilistically among valid options
    return probabilistic_select(displayed_store_ids, adjusted_scores, valid_indices, valid_scores);
}

// Probabilistic selection using Softmax
int CustomerDecisionSystem::probabilistic_select(
    const vector<int>& store_ids,
    const vector<float>& all_scores,
    const vector<int>& valid_indices,
    const vector<float>& valid_scores) {
    
    static mt19937 rng(time(nullptr));
    
    float min_score = *min_element(valid_scores.begin(), valid_scores.end());
    float temperature = 2.0f; // Controls randomness (higher = more random)
    
    // Calculate Softmax probabilities
    vector<float> probabilities;
    float sum_exp = 0.0f;
    
    for (float score : valid_scores) {
        float shifted_score = score - min_score + 1.0f;
        float exp_score = exp(shifted_score / temperature);
        probabilities.push_back(exp_score);
        sum_exp += exp_score;
    }
    
    for (float& prob : probabilities) {
        prob /= sum_exp;
    }
    
    // Weighted random choice
    uniform_real_distribution<float> dist(0.0f, 1.0f);
    float random_val = dist(rng);
    float cumulative = 0.0f;
    
    for (size_t i = 0; i < probabilities.size(); i++) {
        cumulative += probabilities[i];
        if (random_val <= cumulative) {
            return store_ids[valid_indices[i]];
        }
    }
    
    // Fallback
    return store_ids[valid_indices.back()];
}

// Create a reservation
bool CustomerDecisionSystem::create_reservation(Customer& customer,
                                                int restaurant_id,
                                                MarketState& market_state) {
    Restaurant* restaurant = market_state.get_restaurant(restaurant_id);
    
    // Check if store can accept
    if (!restaurant || !restaurant->can_accept_reservation()) {
        return false;
    }

    // Create reservation object
    Reservation res(market_state.next_reservation_id++,
                    customer.id,
                    restaurant_id,
                    market_state.current_time);

    // Update customer history
    customer.record_reservation_attempt(restaurant_id,
                                        restaurant->business_type,
                                        market_state.current_time);

    // Update store state
    restaurant->reserved_count++;
    market_state.reservations.push_back(res);

    return true;
}

