#ifndef RANKING_ALGORITHMS_H
#define RANKING_ALGORITHMS_H

#include <vector>
#include "Customer.h"
#include "MarketState.h"

// ============================================================================
// RANKING ALGORITHM TYPES
// ============================================================================
enum class RankingAlgorithm {
    BASELINE,  // Top-N by rating only (same stores for all customers)
    SMART      // Personalized + fairness + waste reduction
};

// ============================================================================
// RANKING SYSTEM - BASELINE
// ============================================================================
// Shows the same top-rated stores to ALL customers
// Sorts by dynamic rating (changes as orders are confirmed/cancelled)
// ============================================================================
std::vector<int> get_displayed_stores_baseline(const Customer& customer,
                                                const MarketState& market_state,
                                                int n_displayed);

// ============================================================================
// RANKING SYSTEM - SMART
// ============================================================================
// Personalized algorithm that balances:
// - 3 stores based on customer preferences/history
// - 1 new store (never tried by customer) for discovery and fairness
// - 1 price-competitive store (best value: rating/price)
// Goals: Minimize waste, maximize revenue, give stores fair chances
// ============================================================================
std::vector<int> get_displayed_stores_smart(const Customer& customer,
                                            const MarketState& market_state,
                                            int n_displayed);

// ============================================================================
// UNIFIED RANKING FUNCTION
// ============================================================================
// Routes to the appropriate ranking algorithm based on the algorithm parameter
// ============================================================================
std::vector<int> get_displayed_stores(const Customer& customer,
                                      const MarketState& market_state,
                                      int n_displayed,
                                      RankingAlgorithm algorithm = RankingAlgorithm::BASELINE);

#endif // RANKING_ALGORITHMS_H

