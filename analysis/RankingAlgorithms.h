#ifndef RANKING_ALGORITHMS_H
#define RANKING_ALGORITHMS_H

#include <vector>
#include "Customer.h"
#include "MarketState.h"

using namespace std;

// Algorithm types
enum class RankingAlgorithm {
    BASELINE,      // Top-N by rating
    SAMA,          // Multi-objective optimization
    ANDREW,        // Fairness-focused
    AMER,          // Minimum distance
    ZIAD,          // Weighted score
    HARMONY        // Combined strategy
};

// Baseline: Top-rated stores
vector<int> get_displayed_stores_baseline(const Customer& customer,
                                                const MarketState& market_state,
                                                int n_displayed);

// Sama: Personalized + Waste Reduction
vector<int> get_displayed_stores_sama(const Customer& customer,
                                           const MarketState& market_state,
                                           int n_displayed);

// Ziad: Weighted Score (Price, Rating, Unsold)
vector<int> get_displayed_stores_ziad(const Customer& customer,
                                            const MarketState& market_state,
                                            int n_displayed);

// Andrew: Fairness (Impression Counts)
vector<int> get_displayed_stores_andrew(const Customer& customer,
                                             MarketState& market_state,
                                             int n_displayed);

// Amer: Closest Store Guarantee
vector<int> get_displayed_stores_amer(const Customer& customer,
                                           const MarketState& market_state,
                                           int n_displayed);

// Harmony: Unified Strategy
vector<int> get_displayed_stores_harmony(const Customer& customer,
                                              MarketState& market_state,
                                              int n_displayed);

// Dispatcher function
vector<int> get_displayed_stores(const Customer& customer,
                                      MarketState& market_state,
                                      int n_displayed,
                                      RankingAlgorithm algorithm = RankingAlgorithm::BASELINE);

#endif // RANKING_ALGORITHMS_H

