#ifndef CUSTOMER_DECISION_SYSTEM_H
#define CUSTOMER_DECISION_SYSTEM_H

#include <vector>
#include "Customer.h"
#include "MarketState.h"
#include "RankingAlgorithms.h"

using namespace std;

// Customer Decision System
class CustomerDecisionSystem {
public:
    // Main entry point
    static int process_customer_arrival(Customer& customer,
                                        MarketState& market_state,
                                        int n_displayed,
                                        RankingAlgorithm algorithm = RankingAlgorithm::BASELINE);

    // Calculate scores
    static vector<float> calculate_store_scores(
        const Customer& customer,
        const vector<int>& displayed_store_ids,
        const MarketState& market_state);

    // Select store
    static int select_store(const Customer& customer,
                            const vector<int>& displayed_store_ids,
                            const vector<float>& scores,
                            const MarketState& market_state);

    // Probabilistic selection (softmax-like)
    static int probabilistic_select(
        const vector<int>& store_ids,
        const vector<float>& all_scores,
        const vector<int>& valid_indices,
        const vector<float>& valid_scores);

    // Create reservation
    static bool create_reservation(Customer& customer,
                                   int restaurant_id,
                                   MarketState& market_state);
};

#endif // CUSTOMER_DECISION_SYSTEM_H

