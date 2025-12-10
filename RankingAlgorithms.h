#ifndef RANKING_ALGORITHMS_H
#define RANKING_ALGORITHMS_H

#include <vector>
#include "Customer.h"
#include "MarketState.h"

// ============================================================================
// RANKING ALGORITHM TYPES
// ============================================================================
enum class RankingAlgorithm {
    BASELINE,      // Top-N by rating only (same stores for all customers)
    SAMA,          // Enhanced personalized + fairness + waste reduction (very smart)
    ANDREW,        // Equalize visibility by inversely weighting impression count
    AMER,          // Negative price/distance weights + closest restaurant guarantee
    ZIAD           // Score based on price weight, rating weight, and unsold bags weight
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
// RANKING SYSTEM - SAMA
// ============================================================================
// Enhanced personalized algorithm that balances:
// - Customer preferences/history with adaptive weights
// - New store discovery for fairness
// - Price-competitive stores
// - Inventory-aware ranking to reduce waste
// - Distance optimization
// Goals: Minimize waste, maximize revenue, ensure fairness, optimize customer satisfaction
// ============================================================================
std::vector<int> get_displayed_stores_sama(const Customer& customer,
                                           const MarketState& market_state,
                                           int n_displayed);

// ============================================================================
// RANKING SYSTEM - ZIAD
// ============================================================================
// Score-based algorithm:
// Score = (price_weight * price) + (rating_weight * rating) + (unsold_weight * unsold_bags)
// Displays top 5 restaurants based on this score
// ============================================================================
std::vector<int> get_displayed_stores_ziad(const Customer& customer,
                                            const MarketState& market_state,
                                            int n_displayed);

// ============================================================================
// RANKING SYSTEM - ANDREW
// ============================================================================
// Equalizes visibility by inversely weighting stores based on impression count
// Divides score by log(impressions + 1) to dampen frequently shown stores
// ============================================================================
std::vector<int> get_displayed_stores_andrew(const Customer& customer,
                                             MarketState& market_state,
                                             int n_displayed);

// ============================================================================
// RANKING SYSTEM - AMER
// ============================================================================
// Uses negative weights for price and distance
// Always includes the closest restaurant to the customer
// ============================================================================
std::vector<int> get_displayed_stores_amer(const Customer& customer,
                                           const MarketState& market_state,
                                           int n_displayed);

// ============================================================================
// UNIFIED RANKING FUNCTION
// ============================================================================
// Routes to the appropriate ranking algorithm based on the algorithm parameter
// ============================================================================
std::vector<int> get_displayed_stores(const Customer& customer,
                                      MarketState& market_state,
                                      int n_displayed,
                                      RankingAlgorithm algorithm = RankingAlgorithm::BASELINE);

#endif // RANKING_ALGORITHMS_H

