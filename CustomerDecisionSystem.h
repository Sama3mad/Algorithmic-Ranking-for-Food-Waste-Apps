#ifndef CUSTOMER_DECISION_SYSTEM_H
#define CUSTOMER_DECISION_SYSTEM_H

#include <vector>
#include "Customer.h"
#include "MarketState.h"
#include "RankingAlgorithms.h"

// ============================================================================
// CUSTOMER DECISION SYSTEM
// ============================================================================
// Handles customer decision-making logic when they arrive at the app
// Processes customer arrival, calculates scores, selects store, creates reservation
// ============================================================================
class CustomerDecisionSystem {
public:
    // Main entry point: process a customer's arrival
    // Returns store_id if reservation created, -1 if customer left
    static int process_customer_arrival(Customer& customer,
                                        MarketState& market_state,
                                        int n_displayed,
                                        RankingAlgorithm algorithm = RankingAlgorithm::BASELINE);

    // Calculate scores for displayed stores based on customer preferences
    static std::vector<float> calculate_store_scores(
        const Customer& customer,
        const std::vector<int>& displayed_store_ids,
        const MarketState& market_state);

    // Select which store customer will choose (probabilistic selection)
    // Returns store_id or -1 if customer leaves
    static int select_store(const Customer& customer,
                            const std::vector<int>& displayed_store_ids,
                            const std::vector<float>& scores,
                            const MarketState& market_state);

    // Probabilistic store selection using softmax
    // Mimics real behavior: customers don't always pick the "best" option
    static int probabilistic_select(
        const std::vector<int>& store_ids,
        const std::vector<float>& all_scores,
        const std::vector<int>& valid_indices,
        const std::vector<float>& valid_scores);

    // Create a reservation for the customer
    // Returns true if successful, false otherwise
    static bool create_reservation(Customer& customer,
                                   int restaurant_id,
                                   MarketState& market_state);
};

#endif // CUSTOMER_DECISION_SYSTEM_H

