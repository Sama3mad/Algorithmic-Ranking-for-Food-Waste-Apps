import { Restaurant, Customer, SimulationResult, AlgorithmMetrics, RankingAlgorithm } from '@/types/simulation'

// Constants (matching C++ exactly)
const MAX_TRAVEL_DISTANCE = 0.05 // Matches C++ Customer.cpp
const NUM_DAYS = 7
const CUSTOMERS_PER_DAY = 100
const N_DISPLAYED = 5
const MAX_BAGS_PER_CUSTOMER = 3

// Helper: Calculate distance (Euclidean, matches C++)
function calculateDistance(lat1: number, lon1: number, lat2: number, lon2: number): number {
  const dlat = lat2 - lat1
  const dlon = lon2 - lon1
  return Math.sqrt(dlat * dlat + dlon * dlon)
}

// Generate random number between min and max
function randomBetween(min: number, max: number): number {
  return Math.random() * (max - min) + min
}

// Customer history tracking
interface CustomerHistory {
  visits: number
  reservations: number
  successes: number
  cancellations: number
  categories_reserved: Record<string, number>
  store_interactions: Record<number, {
    reservations: number
    successes: number
    cancellations: number
  }>
}

// Extended customer with history
interface ExtendedCustomer extends Customer {
  history: CustomerHistory
  churned: boolean
}

// Generate customers from CSV data (matching C++ ArrivalGenerator logic)
function generateCustomersFromCSV(csvData: any[]): ExtendedCustomer[] {
  return csvData.map((row, idx) => {
    const valuations: Record<number, number> = {}
    Object.keys(row).forEach(key => {
      const normalizedKey = key.toLowerCase().trim()
      if ((normalizedKey.startsWith('store') || normalizedKey.includes('store')) && 
          (normalizedKey.includes('valuation') || normalizedKey.includes('_id_'))) {
        const match = key.match(/\d+/)
        if (match) {
          const storeId = parseInt(match[0])
          valuations[storeId] = parseFloat(row[key]) || 0
        }
      }
    })

    const customerId = parseInt(row.CustomerID || row.customerid || row.id || String(idx + 1))
    const longitude = parseFloat(row.longitude || row.lon || '31.2')
    const latitude = parseFloat(row.latitude || row.lat || '30.0')

    return {
      id: customerId,
      longitude: isNaN(longitude) ? 31.2 : longitude,
      latitude: isNaN(latitude) ? 30.0 : latitude,
      customer_name: `Customer ${customerId}`,
      segment: 'regular', // Default, can be overridden from CSV
      willingness_to_pay: 200.0, // Default from C++
      weights: {
        rating_w: 1.0,
        price_w: 1.0,
        novelty_w: 0.5
      },
      loyalty: 0.8,
      leaving_threshold: 3.0, // Matches C++ default
      store_valuations: valuations,
      category_preference: {
        'bakery': 1.0,
        'cafe': 1.0,
        'restaurant': 1.0
      },
      history: {
        visits: 0,
        reservations: 0,
        successes: 0,
        cancellations: 0,
        categories_reserved: {},
        store_interactions: {}
      },
      churned: false
    }
  })
}

// Load restaurants from CSV
function loadRestaurantsFromCSV(csvData: any[]): Restaurant[] {
  return csvData.map((row, idx) => {
    const estimated = parseInt(
      row.average_bags_at_9AM || 
      row.average_bags || 
      row.estimated_bags || 
      row.bags || 
      '10'
    ) || 10
    const actual = Math.floor(estimated * randomBetween(0.8, 1.2))

    const businessId = parseInt(row.store_id || row.id || String(idx + 1)) || (idx + 1)
    const businessName = row.store_name || row.name || row.business_name || `Store ${businessId}`
    const branch = row.branch || ''
    const rating = parseFloat(row.average_overall_rating || row.rating || row.overall_rating || '4.0') || 4.0
    const price = parseFloat(row.price || row.price_per_bag || '100') || 100
    const longitude = parseFloat(row.longitude || row.lon || '31.2') || 31.2
    const latitude = parseFloat(row.latitude || row.lat || '30.0') || 30.0
    const businessType = row.business_type || row.type || 'restaurant'

    return {
      business_id: businessId,
      business_name: businessName,
      branch: branch,
      estimated_bags: estimated,
      general_ranking: rating,
      price_per_bag: price,
      longitude: longitude,
      latitude: latitude,
      business_type: businessType,
      actual_bags: actual,
      reserved_count: 0,
      has_inventory: actual > 0
    }
  })
}

// Calculate customer score for a store (matches C++ Customer::calculate_store_score exactly)
function calculateStoreScore(customer: ExtendedCustomer, restaurant: Restaurant): number {
  const distance = calculateDistance(
    customer.latitude, customer.longitude,
    restaurant.latitude, restaurant.longitude
  )
  
  // Filter out stores that are too far (matches C++ code)
  if (distance > MAX_TRAVEL_DISTANCE) {
    return -100.0
  }

  // Calculate component scores (matches C++ formula exactly)
  const rating_score = customer.weights.rating_w * restaurant.general_ranking
  const price_score = customer.weights.price_w * (customer.willingness_to_pay - restaurant.price_per_bag) / customer.willingness_to_pay
  
  // Novelty score (higher for new categories)
  const categoryCount = customer.history.categories_reserved[restaurant.business_type] || 0
  const novelty_score = customer.weights.novelty_w * (categoryCount === 0 ? 1.0 : 1.0 / (1.0 + categoryCount))
  
  // Distance score: closer is better
  const normalized_distance = distance / MAX_TRAVEL_DISTANCE
  const distance_score = (1.0 - normalized_distance) * 1.5

  return rating_score + price_score + novelty_score + distance_score
}

// Ranking Algorithms (matching C++ exactly)
function getDisplayedStoresBaseline(
  customer: ExtendedCustomer,
  restaurants: Restaurant[],
  nDisplayed: number
): number[] {
  const available = restaurants.filter(r => r.has_inventory && r.estimated_bags > r.reserved_count)
  available.sort((a, b) => b.general_ranking - a.general_ranking)
  return available.slice(0, nDisplayed).map(r => r.business_id)
}

function getDisplayedStoresSama(
  customer: ExtendedCustomer,
  restaurants: Restaurant[],
  nDisplayed: number,
  impressionCounts: Record<number, number>
): number[] {
  const available = restaurants.filter(r => r.has_inventory && r.estimated_bags > r.reserved_count)
  if (available.length === 0) return []

  const is_budget = customer.segment === 'budget'
  const is_premium = customer.segment === 'premium'
  const is_regular = customer.segment === 'regular'
  
  const segment_rating_weight = is_premium ? 1.5 : (is_budget ? 0.8 : 1.0)
  const segment_price_weight = is_budget ? 1.3 : (is_premium ? 0.7 : 1.0)
  const segment_inventory_weight = is_budget ? 0.8 : (is_premium ? 0.5 : 0.6)
  
  // Calculate comprehensive scores
  const storeScores: Array<{id: number, score: number}> = []
  for (const store of available) {
    const baseScore = calculateStoreScore(customer, store)
    
    const unsoldBags = Math.max(0, store.estimated_bags - store.reserved_count)
    const inventoryUrgency = Math.min(1, unsoldBags / 15)
    const inventoryBonus = inventoryUrgency * 1.2 * segment_inventory_weight
    
    const ratingBonus = Math.max(0, (store.general_ranking - 3.5) * 0.3 * segment_rating_weight)
    
    let priceBonus = 0
    if (is_budget && store.price_per_bag < customer.willingness_to_pay) {
      priceBonus = ((customer.willingness_to_pay - store.price_per_bag) / customer.willingness_to_pay) * 0.4
    } else if (is_premium && store.price_per_bag > 100) {
      priceBonus = 0.1
    }
    
    let historyBonus = 0
    const hist = customer.history.store_interactions[store.business_id]
    if (hist && hist.reservations > 0) {
      const successRate = hist.successes / hist.reservations
      historyBonus = successRate * 0.5
      if (hist.cancellations > 0) {
        const cancelRate = hist.cancellations / hist.reservations
        historyBonus -= cancelRate * 1.0
      }
    }
    
    const categoryBonus = (customer.category_preference[store.business_type] || 0) * 0.2
    
    let wasteBonus = 0
    if (unsoldBags > 5) {
      wasteBonus = Math.min(2, unsoldBags / 5) * 0.5
    }
    
    const revenueBonus = (store.price_per_bag * inventoryUrgency / 200) * 0.3
    
    const finalScore = baseScore + inventoryBonus + ratingBonus + priceBonus + 
                      historyBonus + categoryBonus + wasteBonus + revenueBonus
    storeScores.push({ id: store.business_id, score: finalScore })
  }

  storeScores.sort((a, b) => b.score - a.score)

  // Adaptive personalization logic
  const basePersonalization = is_budget ? 0.7 : (is_premium ? 0.5 : 0.6)
  const loyaltyAdjustment = customer.loyalty * 0.15
  
  // Check overall market waste status
  let totalUnsold = 0
  let storesWithInventory = 0
  for (const r of restaurants) {
    const unsold = Math.max(0, r.estimated_bags - r.reserved_count)
    if (unsold > 0) {
      totalUnsold += unsold
      storesWithInventory++
    }
  }
  const avgUnsold = storesWithInventory > 0 ? totalUnsold / storesWithInventory : 0
  
  const wasteAdjustment = (avgUnsold > 10) ? -0.1 : 0
  let personalizationRatio = basePersonalization + loyaltyAdjustment + wasteAdjustment
  personalizationRatio = Math.min(0.85, Math.max(0.4, personalizationRatio))
  
  let personalizedCount = Math.min(Math.floor(nDisplayed * personalizationRatio), storeScores.length)
  personalizedCount = Math.max(3, personalizedCount)

  const result: number[] = []
  const selected = new Set<number>()

  // SELECT 1: Personalized stores
  for (let i = 0; i < personalizedCount && result.length < nDisplayed; i++) {
    result.push(storeScores[i].id)
    selected.add(storeScores[i].id)
  }

  // SELECT 2: Discovery stores (segment-aware) - matches C++ exactly
  if (result.length < nDisplayed) {
    const qualityNewStores: Array<{id: number, score: number}> = []
    for (const store of available) {
      if (selected.has(store.business_id)) continue
      
      const hist = customer.history.store_interactions[store.business_id]
      const isNew = !hist || hist.reservations === 0
      
      if (isNew) {
        let meetsThreshold = false
        let qualityScore = 0.0
        
        const unsold = Math.max(0, store.estimated_bags - store.reserved_count)
        const unsoldBonus = Math.min(1, unsold / 10)
        
        if (is_budget) {
          if (store.price_per_bag <= customer.willingness_to_pay * 1.1 && 
              store.estimated_bags >= 8 && store.general_ranking >= 3.8) {
            const valueRatio = store.general_ranking / store.price_per_bag
            const priceAffordability = (customer.willingness_to_pay - store.price_per_bag) / customer.willingness_to_pay
            const inventorySafety = Math.min(1, store.estimated_bags / 15)
            qualityScore = valueRatio * 15.0 + priceAffordability * 2.0 + 
                          inventorySafety * 0.5 + store.general_ranking * 0.3 + unsoldBonus * 0.8
            meetsThreshold = true
          }
        } else if (is_premium) {
          if (store.general_ranking >= 4.0 && store.estimated_bags >= 8) {
            const valueScore = store.general_ranking / store.price_per_bag
            const inventoryBonus = Math.min(1, store.estimated_bags / 15)
            qualityScore = store.general_ranking * 1.5 + valueScore * 10.0 + 
                          inventoryBonus * 0.5 + unsoldBonus * 0.6
            meetsThreshold = true
          }
        } else {
          if (store.general_ranking >= 3.9 && store.estimated_bags >= 8) {
            const valueScore = store.general_ranking / store.price_per_bag
            const inventoryBonus = Math.min(1, store.estimated_bags / 15)
            qualityScore = store.general_ranking + valueScore * 10.0 + 
                          inventoryBonus * 0.5 + unsoldBonus * 0.7
            meetsThreshold = true
          }
        }
        
        if (meetsThreshold) {
          qualityNewStores.push({ id: store.business_id, score: qualityScore })
        }
      }
    }
    
    if (qualityNewStores.length > 0) {
      qualityNewStores.sort((a, b) => b.score - a.score)
      result.push(qualityNewStores[0].id)
      selected.add(qualityNewStores[0].id)
    }
  }

  // SELECT 3: Price-competitive selection - matches C++ exactly
  if (result.length < nDisplayed) {
    const competitiveStores: Array<{id: number, score: number}> = []
    for (const store of available) {
      if (selected.has(store.business_id)) continue
      
      if (store.estimated_bags >= 8) {
        const valueRatio = store.general_ranking / store.price_per_bag
        const inventorySafety = Math.min(1, store.estimated_bags / 15)
        let competitiveScore = 0.0
        let isCompetitive = false
        
        if (is_budget) {
          if (store.price_per_bag <= customer.willingness_to_pay * 1.1 && valueRatio > 0.025) {
            const priceAffordability = (customer.willingness_to_pay - store.price_per_bag) / customer.willingness_to_pay
            competitiveScore = valueRatio * 120.0 + priceAffordability * 3.0 + 
                              inventorySafety * 0.5 + store.general_ranking * 0.3
            isCompetitive = true
          }
        } else if (is_premium) {
          if (valueRatio > 0.03 && store.general_ranking >= 3.8) {
            competitiveScore = valueRatio * 100.0 + inventorySafety * 0.5 + store.general_ranking * 0.8
            isCompetitive = true
          }
        } else {
          if (valueRatio > 0.03) {
            competitiveScore = valueRatio * 100.0 + inventorySafety * 0.5 + store.general_ranking * 0.5
            isCompetitive = true
          }
        }
        
        if (isCompetitive) {
          competitiveStores.push({ id: store.business_id, score: competitiveScore })
        }
      }
    }
    
    if (competitiveStores.length > 0) {
      competitiveStores.sort((a, b) => b.score - a.score)
      result.push(competitiveStores[0].id)
      selected.add(competitiveStores[0].id)
    }
  }

  // SELECT 3: Fill remaining with best available
  for (const score of storeScores) {
    if (result.length >= nDisplayed) break
    if (!selected.has(score.id)) {
      result.push(score.id)
      selected.add(score.id)
    }
  }

  return result
}

function getDisplayedStoresAndrew(
  customer: ExtendedCustomer,
  restaurants: Restaurant[],
  nDisplayed: number,
  impressionCounts: Record<number, number>
): number[] {
  const available = restaurants.filter(r => r.has_inventory && r.estimated_bags > r.reserved_count)
  if (available.length === 0) return []

  const storeScores: Array<{id: number, score: number}> = []
  
  for (const store of available) {
    const baseScore = calculateStoreScore(customer, store)
    const impressions = impressionCounts[store.business_id] || 0
    const dampingFactor = Math.log(impressions + 1) + 1
    const adjustedScore = baseScore / dampingFactor
    storeScores.push({ id: store.business_id, score: adjustedScore })
  }
  
  storeScores.sort((a, b) => b.score - a.score)
  return storeScores.slice(0, Math.min(nDisplayed, storeScores.length)).map(s => s.id)
}

function getDisplayedStoresAmer(
  customer: ExtendedCustomer,
  restaurants: Restaurant[],
  nDisplayed: number
): number[] {
  const available = restaurants.filter(r => {
    if (!r.has_inventory || r.estimated_bags <= r.reserved_count) return false
    const distance = calculateDistance(customer.latitude, customer.longitude, r.latitude, r.longitude)
    return distance <= MAX_TRAVEL_DISTANCE
  })

  const result: number[] = []
  const selected = new Set<number>()
  
  // Step 1: Find absolute closest store
  let closestStoreId = -1
  let minDistance = Infinity
  
  for (const store of available) {
    const distance = calculateDistance(customer.latitude, customer.longitude, store.latitude, store.longitude)
    if (distance < minDistance) {
      minDistance = distance
      closestStoreId = store.business_id
    }
  }
  
  if (closestStoreId !== -1) {
    result.push(closestStoreId)
    selected.add(closestStoreId)
  }
  
  // Step 2: Score remaining
  const storeScores: Array<{id: number, score: number}> = []
  
  for (const store of available) {
    if (selected.has(store.business_id)) continue
    const baseScore = calculateStoreScore(customer, store)
    const pricePenalty = store.price_per_bag * 0.01
    const distance = calculateDistance(customer.latitude, customer.longitude, store.latitude, store.longitude)
    const distancePenalty = distance * 20.0
    const finalScore = baseScore - pricePenalty - distancePenalty
    storeScores.push({ id: store.business_id, score: finalScore })
  }
  
  storeScores.sort((a, b) => b.score - a.score)
  const remaining = nDisplayed - result.length
  for (let i = 0; i < remaining && i < storeScores.length; i++) {
    result.push(storeScores[i].id)
  }
  
  return result
}

function getDisplayedStoresZiad(
  customer: ExtendedCustomer,
  restaurants: Restaurant[],
  nDisplayed: number
): number[] {
  const available = restaurants.filter(r => {
    if (!r.has_inventory || r.estimated_bags <= r.reserved_count) return false
    const distance = calculateDistance(customer.latitude, customer.longitude, r.latitude, r.longitude)
    return distance <= MAX_TRAVEL_DISTANCE
  })
  if (available.length === 0) return []

  const priceWeight = -0.01
  const ratingWeight = 1.5
  const unsoldWeight = 0.1

  const storeScores: Array<{id: number, score: number}> = []
  
  for (const store of available) {
    const unsoldBags = Math.max(0, store.estimated_bags - store.reserved_count)
    const score = (priceWeight * store.price_per_bag) + 
                  (ratingWeight * store.general_ranking) + 
                  (unsoldWeight * unsoldBags)
    storeScores.push({ id: store.business_id, score })
  }
  
  storeScores.sort((a, b) => b.score - a.score)
  const numToShow = Math.min(nDisplayed, Math.min(5, storeScores.length))
  return storeScores.slice(0, numToShow).map(s => s.id)
}

function getDisplayedStoresHarmony(
  customer: ExtendedCustomer,
  restaurants: Restaurant[],
  nDisplayed: number,
  impressionCounts: Record<number, number>
): number[] {
  const available = restaurants.filter(r => {
    if (!r.has_inventory || r.estimated_bags <= r.reserved_count) return false
    const distance = calculateDistance(customer.latitude, customer.longitude, r.latitude, r.longitude)
    return distance <= MAX_TRAVEL_DISTANCE
  })
  if (available.length === 0) return []

  // Average impressions for fairness
  let totalImpressions = 0
  let storeCount = 0
  for (const count of Object.values(impressionCounts)) {
    totalImpressions += count
    storeCount++
  }
  const avgImpressions = storeCount > 0 ? totalImpressions / storeCount : 1.0

  const storeScores: Array<{id: number, score: number}> = []
  
  for (const store of available) {
    const baseScore = calculateStoreScore(customer, store)
    
    // Satisfaction bonus
    let satisfactionBonus = 0
    if (customer.segment === 'premium' && store.general_ranking >= 4.0) {
      satisfactionBonus = 0.5
    } else if (customer.segment === 'budget' && store.price_per_bag <= customer.willingness_to_pay) {
      satisfactionBonus = 0.4
    } else if (customer.segment === 'regular' && store.general_ranking >= 3.8) {
      satisfactionBonus = 0.3
    }
    
    const hist = customer.history.store_interactions[store.business_id]
    if (hist && hist.successes > 0) {
      const successRate = hist.successes / hist.reservations
      satisfactionBonus += successRate * 0.3
    }
    
    // Waste reduction
    const unsoldBags = Math.max(0, store.estimated_bags - store.reserved_count)
    let wasteBonus = unsoldBags * 0.08
    if (unsoldBags > 12) {
      wasteBonus += 0.6
    }
    
    // Fairness
    const impressions = impressionCounts[store.business_id] || 0
    let fairnessBoost = 0
    if (impressions < avgImpressions * 0.5) {
      fairnessBoost = 0.8
    } else if (impressions > avgImpressions * 1.5) {
      fairnessBoost = -0.4
    }
    
    // Revenue potential
    const inventorySafety = Math.min(1, store.estimated_bags / 10)
    const revenueBonus = (store.price_per_bag / 100) * inventorySafety * 0.3
    
    // Quality penalty
    let qualityPenalty = 0
    if (store.estimated_bags < 5) {
      qualityPenalty = -1.5
    }
    
    const finalScore = baseScore + satisfactionBonus + wasteBonus + fairnessBoost + revenueBonus + qualityPenalty
    storeScores.push({ id: store.business_id, score: finalScore })
  }
  
  storeScores.sort((a, b) => b.score - a.score)

  const result: number[] = []
  const selected = new Set<number>()

  // STEP 2: Fill top 70% slots
  const directSlots = Math.floor(nDisplayed * 0.7)
  for (let i = 0; i < directSlots && i < storeScores.length; i++) {
    result.push(storeScores[i].id)
    selected.add(storeScores[i].id)
  }

  // STEP 3: Add one high-waste store
  if (result.length < nDisplayed) {
    for (const score of storeScores) {
      if (selected.has(score.id)) continue
      const store = restaurants.find(r => r.business_id === score.id)
      if (store) {
        const unsold = Math.max(0, store.estimated_bags - store.reserved_count)
        if (unsold >= 10) {
          result.push(score.id)
          selected.add(score.id)
          break
        }
      }
    }
  }

  // STEP 4: Add one discovery store
  if (result.length < nDisplayed) {
    for (const score of storeScores) {
      if (selected.has(score.id)) continue
      const hist = customer.history.store_interactions[score.id]
      const isNew = !hist || hist.reservations === 0
      if (isNew) {
        const store = restaurants.find(r => r.business_id === score.id)
        if (store && store.general_ranking >= 3.8 && store.estimated_bags >= 6) {
          result.push(score.id)
          selected.add(score.id)
          break
        }
      }
    }
  }

  // STEP 5: Fill remaining
  for (const score of storeScores) {
    if (result.length >= nDisplayed) break
    if (!selected.has(score.id)) {
      result.push(score.id)
      selected.add(score.id)
    }
  }

  // Track impressions (matches C++ - Harmony tracks its own impressions at the end)
  for (const storeId of result) {
    impressionCounts[storeId] = (impressionCounts[storeId] || 0) + 1
  }

  return result
}

// Customer decision making (matches C++ CustomerDecisionSystem exactly)
function processCustomerDecision(
  customer: ExtendedCustomer,
  displayedStores: number[],
  restaurants: Restaurant[]
): number | null {
  if (displayedStores.length === 0) return null

  // Calculate base scores
  const scores = displayedStores.map(storeId => {
    const store = restaurants.find(r => r.business_id === storeId)
    if (!store) return { id: storeId, score: -100.0 }
    return { id: storeId, score: calculateStoreScore(customer, store) }
  })

  // Determine threshold
  const baseThreshold = customer.leaving_threshold
  const loyaltyAdjustment = (1.0 - customer.loyalty) * 2.0
  const threshold = baseThreshold + loyaltyAdjustment

  // If best option is too poor, customer leaves
  const maxScore = Math.max(...scores.map(s => s.score))
  if (maxScore < threshold) return null

  // Adjust scores based on history and inventory safety
  const adjustedScores = scores.map((s, idx) => {
    const storeId = s.id
    let adjusted = s.score
    
    // History adjustment
    const hist = customer.history.store_interactions[storeId]
    if (hist && hist.reservations > 0) {
      const successRate = hist.successes / hist.reservations
      adjusted += successRate * 1.5
      if (hist.cancellations > 0) {
        const cancelRate = hist.cancellations / hist.reservations
        adjusted -= cancelRate * 2.0
      }
    }
    
    // Inventory safety bonus
    const store = restaurants.find(r => r.business_id === storeId)
    if (store) {
      const inventorySafety = Math.min(1, store.estimated_bags / 12)
      adjusted += inventorySafety * 0.3
    }
    
    return { id: storeId, score: adjusted }
  })

  // Filter valid options
  const validOptions = adjustedScores.filter(s => s.score >= threshold && s.score > -50.0)
  
  if (validOptions.length === 0) return null

  // Probabilistic selection using Softmax
  const minScore = Math.min(...validOptions.map(o => o.score))
  const temperature = 2.0
  
  const probabilities = validOptions.map(opt => {
    const shiftedScore = opt.score - minScore + 1.0
    return Math.exp(shiftedScore / temperature)
  })
  
  const sumExp = probabilities.reduce((a, b) => a + b, 0)
  const normalizedProbs = probabilities.map(p => p / sumExp)
  
  // Weighted random choice
  const randomVal = Math.random()
  let cumulative = 0
  for (let i = 0; i < normalizedProbs.length; i++) {
    cumulative += normalizedProbs[i]
    if (randomVal <= cumulative) {
      return validOptions[i].id
    }
  }
  
  return validOptions[validOptions.length - 1].id
}

// Helper function to check if restaurant can accept reservation
// IMPORTANT: Uses estimated_bags (not actual_bags) to match C++ behavior
// This allows overbooking during the day; cancellations happen at end-of-day when actual_bags < reservations
function canAcceptReservation(restaurant: Restaurant): boolean {
  return restaurant.has_inventory && 
         restaurant.estimated_bags > restaurant.reserved_count
}

// Calculate Gini coefficient (matches C++ formula exactly)
function calculateGiniCoefficient(exposures: number[]): number {
  if (exposures.length === 0) return 0
  
  const sorted = [...exposures].sort((a, b) => a - b)
  const n = sorted.length
  let sum = 0
  let weightedSum = 0

  for (let i = 0; i < n; i++) {
    sum += sorted[i]
    weightedSum += sorted[i] * (i + 1)
  }

  if (sum === 0) return 0
  return (2.0 * weightedSum) / (n * sum) - (n + 1.0) / n
}

// Run simulation for one algorithm
export function runSimulation(
  restaurants: Restaurant[],
  customers: ExtendedCustomer[],
  algorithm: RankingAlgorithm,
  preGeneratedActualBags?: Map<number, Map<number, number>> // restaurant_id -> day -> actual_bags
): { result: SimulationResult; metrics: AlgorithmMetrics } {
  // Initialize state
  const restaurantMap = new Map(restaurants.map(r => [r.business_id, { ...r }]))
  const impressionCounts: Record<number, number> = {}
  const actualBagsTotal: Record<number, number> = {}
  
  let totalBagsSold = 0
  let totalBagsCancelled = 0
  let totalBagsUnsold = 0
  let totalRevenueGenerated = 0
  let totalRevenueLost = 0
  let customersWhoLeft = 0
  
  interface Reservation {
    id: number
    customer_id: number
    restaurant_id: number
    time: number
    status: 'PENDING' | 'CONFIRMED' | 'CANCELLED'
    bags_received: number
  }

  const storeStats: Record<number, {
    sold: number
    cancelled: number
    waste: number
    revenue: number
    exposures: number
    reserved: number
  }> = {}
  
  let nextReservationId = 1

  // Initialize store stats
  restaurants.forEach(r => {
    storeStats[r.business_id] = {
      sold: 0,
      cancelled: 0,
      waste: 0,
      revenue: 0,
      exposures: 0,
      reserved: 0
    }
    actualBagsTotal[r.business_id] = 0
  })

  // Ensure we have enough customers (reuse if needed)
  const customerPool: ExtendedCustomer[] = []
  while (customerPool.length < NUM_DAYS * CUSTOMERS_PER_DAY) {
    customerPool.push(...customers.map(c => ({
      ...c,
      history: {
        ...c.history,
        store_interactions: { ...c.history.store_interactions }
      }
    })))
  }
  const finalCustomers = customerPool.slice(0, NUM_DAYS * CUSTOMERS_PER_DAY)

  // Simulate 7 days
  for (let day = 0; day < NUM_DAYS; day++) {
    // Reset daily reservations and replenish inventory
    const dailyReservations: Reservation[] = []
    restaurants.forEach(r => {
      const store = restaurantMap.get(r.business_id)!
      store.reserved_count = 0
      // Use pre-generated actual bags if provided (for fair comparison across algorithms)
      // Otherwise generate randomly (for backward compatibility)
      if (preGeneratedActualBags && preGeneratedActualBags.has(r.business_id) && preGeneratedActualBags.get(r.business_id)!.has(day)) {
        store.actual_bags = preGeneratedActualBags.get(r.business_id)!.get(day)!
      } else {
        const variance = randomBetween(0.8, 1.2)
        store.actual_bags = Math.floor(r.estimated_bags * variance)
      }
      store.has_inventory = store.actual_bags > 0
      actualBagsTotal[r.business_id] += store.actual_bags
    })

    // Process customers for the day
    const dayCustomers = finalCustomers.slice(
      day * CUSTOMERS_PER_DAY,
      (day + 1) * CUSTOMERS_PER_DAY
    )

    // Create reservations during the day
    for (const customer of dayCustomers) {
      const available = Array.from(restaurantMap.values()).filter(canAcceptReservation)

      let displayed: number[]
      switch (algorithm) {
        case 'BASELINE':
          displayed = getDisplayedStoresBaseline(customer, available, N_DISPLAYED)
          break
        case 'SAMA':
          displayed = getDisplayedStoresSama(customer, available, N_DISPLAYED, impressionCounts)
          break
        case 'ANDREW':
          displayed = getDisplayedStoresAndrew(customer, available, N_DISPLAYED, impressionCounts)
          break
        case 'AMER':
          displayed = getDisplayedStoresAmer(customer, available, N_DISPLAYED)
          break
        case 'ZIAD':
          displayed = getDisplayedStoresZiad(customer, available, N_DISPLAYED)
          break
        case 'HARMONY':
          displayed = getDisplayedStoresHarmony(customer, available, N_DISPLAYED, impressionCounts)
          break
        default:
          displayed = getDisplayedStoresBaseline(customer, available, N_DISPLAYED)
      }

      // Track impressions
      displayed.forEach(storeId => {
        impressionCounts[storeId] = (impressionCounts[storeId] || 0) + 1
        storeStats[storeId].exposures++
      })

      // Customer decision
      const selectedStoreId = processCustomerDecision(customer, displayed, available)
      
      if (selectedStoreId === null) {
        customersWhoLeft++
        customer.churned = true
        continue
      }

      const selectedStore = restaurantMap.get(selectedStoreId)!
      if (canAcceptReservation(selectedStore)) {
        // Create reservation
        const reservation: Reservation = {
          id: nextReservationId++,
          customer_id: customer.id,
          restaurant_id: selectedStoreId,
          time: day * 1000 + Math.random() * 1000,
          status: 'PENDING',
          bags_received: 0
        }
        dailyReservations.push(reservation)
        selectedStore.reserved_count++
        storeStats[selectedStoreId].reserved++
        
        // Update customer history
        customer.history.visits++
        customer.history.reservations++
        if (!customer.history.store_interactions[selectedStoreId]) {
          customer.history.store_interactions[selectedStoreId] = {
            reservations: 0,
            successes: 0,
            cancellations: 0
          }
        }
        customer.history.store_interactions[selectedStoreId].reservations++
        const categoryCount = customer.history.categories_reserved[selectedStore.business_type] || 0
        customer.history.categories_reserved[selectedStore.business_type] = categoryCount + 1
      }
    }

    // End of day: Process all reservations (matches C++ RestaurantManagementSystem exactly)
    restaurants.forEach(r => {
      const store = restaurantMap.get(r.business_id)!
      const restaurantReservations = dailyReservations
        .filter(res => res.restaurant_id === r.business_id && res.status === 'PENDING')
        .sort((a, b) => a.time - b.time)

      const numReservations = restaurantReservations.length
      const actualBags = store.actual_bags

      if (numReservations === 0) {
        // No reservations, all bags become waste
        storeStats[r.business_id].waste += actualBags
        totalBagsUnsold += actualBags
        return
      }

      if (actualBags >= numReservations) {
        // Enough bags for everyone
        const bagsPerCustomer = Math.min(MAX_BAGS_PER_CUSTOMER, Math.floor(actualBags / numReservations))
        const extraBags = actualBags - (bagsPerCustomer * numReservations)
        
        let bagsDistributed = 0
        restaurantReservations.forEach((res, idx) => {
          let bagsForThisCustomer = bagsPerCustomer
          // Distribute extra bags to first customers if possible
          if (extraBags > 0 && idx < extraBags && bagsForThisCustomer < MAX_BAGS_PER_CUSTOMER) {
            bagsForThisCustomer++
          }
          
          res.status = 'CONFIRMED'
          res.bags_received = bagsForThisCustomer
          bagsDistributed += bagsForThisCustomer
          
          totalBagsSold += bagsForThisCustomer
          storeStats[r.business_id].sold += bagsForThisCustomer
          const revenue = bagsForThisCustomer * r.price_per_bag
          storeStats[r.business_id].revenue += revenue
          totalRevenueGenerated += revenue
          
          // Update customer history
          const customer = finalCustomers.find(c => c.id === res.customer_id)
          if (customer) {
            customer.history.successes++
            if (customer.history.store_interactions[r.business_id]) {
              customer.history.store_interactions[r.business_id].successes++
            }
          }
        })
        
        // Remaining bags become waste
        const remainingBags = actualBags - bagsDistributed
        if (remainingBags > 0) {
          storeStats[r.business_id].waste += remainingBags
          totalBagsUnsold += remainingBags
        }
      } else {
        // Not enough bags - first customers get 1 bag each, rest get cancelled
        // This is where cancellations happen when actualBags < numReservations
        for (let i = 0; i < actualBags; i++) {
          const res = restaurantReservations[i]
          res.status = 'CONFIRMED'
          res.bags_received = 1
          
          totalBagsSold += 1
          storeStats[r.business_id].sold += 1
          const revenue = r.price_per_bag
          storeStats[r.business_id].revenue += revenue
          totalRevenueGenerated += revenue
          
          // Update customer history
          const customer = finalCustomers.find(c => c.id === res.customer_id)
          if (customer) {
            customer.history.successes++
            if (customer.history.store_interactions[r.business_id]) {
              customer.history.store_interactions[r.business_id].successes++
            }
          }
        }
        
        // Remaining reservations are cancelled
        for (let i = actualBags; i < numReservations; i++) {
          const res = restaurantReservations[i]
          res.status = 'CANCELLED'
          
          totalBagsCancelled += 1
          storeStats[r.business_id].cancelled += 1
          totalRevenueLost += r.price_per_bag
          
          // Update customer history
          const customer = finalCustomers.find(c => c.id === res.customer_id)
          if (customer) {
            customer.history.cancellations++
            if (customer.history.store_interactions[r.business_id]) {
              customer.history.store_interactions[r.business_id].cancellations++
            }
          }
        }
      }
    })
  }

  // Build results
  const restaurantResults = restaurants.map(r => {
    const stats = storeStats[r.business_id]
    const avgActual = Math.round(actualBagsTotal[r.business_id] / NUM_DAYS)
    return {
      Restaurant: r.business_name,
      Estimated: r.estimated_bags,
      Actual: avgActual,
      Reserved: stats.reserved,
      Sold: stats.sold,
      Cancelled: stats.cancelled,
      Waste: stats.waste,
      Revenue: stats.revenue,
      Exposures: stats.exposures
    }
  })

  const result: SimulationResult = {
    Algorithm: algorithm,
    results: restaurantResults
  }

  // Calculate metrics
  const totalArrivals = NUM_DAYS * CUSTOMERS_PER_DAY
  const revenueEfficiency = (totalRevenueGenerated + totalRevenueLost) > 0
    ? (totalRevenueGenerated / (totalRevenueGenerated + totalRevenueLost)) * 100
    : 0
  const conversionRate = totalArrivals > 0
    ? ((totalArrivals - customersWhoLeft) / totalArrivals) * 100
    : 0

  const exposures = restaurants.map(r => storeStats[r.business_id].exposures)
  const giniCoefficient = calculateGiniCoefficient(exposures)

  const metrics: AlgorithmMetrics = {
    algorithm,
    total_bags_sold: totalBagsSold,
    total_bags_cancelled: totalBagsCancelled,
    total_bags_unsold: totalBagsUnsold,
    total_revenue_generated: totalRevenueGenerated,
    total_revenue_lost: totalRevenueLost,
    revenue_efficiency: revenueEfficiency,
    customers_who_left: customersWhoLeft,
    conversion_rate: conversionRate,
    gini_coefficient: giniCoefficient,
    total_customer_arrivals: totalArrivals
  }

  return { result, metrics }
}

// Main simulation function
export async function runFullSimulation(
  customerCSV: any[],
  storesCSV: any[]
): Promise<{ results: SimulationResult[]; comparison: AlgorithmMetrics[] }> {
  const restaurants = loadRestaurantsFromCSV(storesCSV)
  const customers = generateCustomersFromCSV(customerCSV)

  // Generate actual bags once for all algorithms (for fair comparison)
  // Structure: Map<restaurant_id, Map<day, actual_bags>>
  const preGeneratedActualBags = new Map<number, Map<number, number>>()
  restaurants.forEach(r => {
    const dayMap = new Map<number, number>()
    for (let day = 0; day < NUM_DAYS; day++) {
      const variance = randomBetween(0.8, 1.2)
      const actualBags = Math.floor(r.estimated_bags * variance)
      dayMap.set(day, actualBags)
    }
    preGeneratedActualBags.set(r.business_id, dayMap)
  })

  const algorithms: RankingAlgorithm[] = ['BASELINE', 'SAMA', 'ANDREW', 'AMER', 'ZIAD', 'HARMONY']
  
  const results: SimulationResult[] = []
  const comparison: AlgorithmMetrics[] = []

  for (const algorithm of algorithms) {
    // Create fresh customer copies for each algorithm (for fair comparison)
    const freshCustomers = customers.map(c => ({
      ...c,
      history: {
        visits: 0,
        reservations: 0,
        successes: 0,
        cancellations: 0,
        categories_reserved: {},
        store_interactions: {}
      },
      churned: false
    }))
    
    // Pass the same pre-generated actual bags to all algorithms
    const { result, metrics } = runSimulation(restaurants, freshCustomers, algorithm, preGeneratedActualBags)
    results.push(result)
    comparison.push(metrics)
  }

  return { results, comparison }
}
