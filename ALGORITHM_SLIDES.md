# Ranking Algorithm Slides

---

## BASELINE Algorithm

**How it shows restaurants:**

• **Simple Rating-Based Ranking**
  - Sorts all restaurants by rating (highest to lowest)
  - Displays top N restaurants to every customer
  - **Same list for all customers** (no personalization)

• **Selection Criteria:**
  - Only factor: Restaurant rating
  - No consideration of: customer preferences, distance, price, inventory, or fairness

• **Result:**
  - Popular high-rated restaurants dominate
  - Low-rated or new restaurants rarely appear
  - No waste reduction strategy

---

## SAMA Algorithm (Enhanced Smart)

**How it shows restaurants:**

• **Multi-Step Intelligent Selection (4 Steps)**

**STEP 1: Personalized Stores (60-85% of slots)**
  - Comprehensive scoring: customer preferences + inventory urgency + waste reduction + revenue potential
  - Segment-aware weights (budget/regular/premium)
  - Considers: rating, price, distance, history, category preferences, unsold inventory

**STEP 2: Smart Discovery (1 store)**
  - Finds new stores customer has never tried
  - Quality thresholds: must meet segment-specific criteria (affordability, rating, inventory)
  - Includes waste reduction bonus for stores with high unsold inventory

**STEP 3: Price-Competitive (1 store)**
  - Best value stores (rating/price ratio)
  - Segment-specific value thresholds
  - Ensures inventory safety (≥8 bags)

**STEP 4: Fill Remaining Slots**
  - Best available stores from comprehensive scoring

• **Key Features:**
  - Adaptive personalization (adjusts based on market waste situation)
  - Balances personalization, waste reduction, fairness, and revenue
  - Dynamic inventory awareness

---

## ANDREW Algorithm

**How it shows restaurants:**

• **Fairness Through Impression Damping**

**Scoring Formula:**
  - Base Score = Customer preference (rating, price, distance, novelty)
  - Adjusted Score = Base Score ÷ log(impressions + 1)
  - Displays top N by adjusted score

• **How it works:**
  - Tracks how many times each store has been displayed (impression count)
  - Frequently shown stores get mathematically dampened
  - Rarely shown stores get boosted
  - Breaks "rich get richer" feedback loop

• **Result:**
  - Equalizes visibility across all stores
  - New or unpopular stores get fair chance
  - Reduces concentration on top performers
  - Aims to minimize total system waste by distributing demand

---

## AMER Algorithm

**How it shows restaurants:**

• **Distance-First with Price Optimization**

**STEP 1: Closest Restaurant Guarantee**
  - Always includes the closest restaurant to customer (within travel threshold)
  - Ensures at least one nearby option

**STEP 2: Score Remaining Stores**
  - Score = Customer Preference + Rating - (Price Penalty) - (Distance Penalty)
  - Negative weights: lower price = better, closer = better
  - Positive factors: customer preferences, restaurant ratings

**STEP 3: Fill Remaining Slots**
  - Selects top-scoring stores from remaining options

• **Key Features:**
  - Prioritizes proximity (convenience)
  - Price-sensitive (favors lower prices)
  - Balances distance, price, and quality
  - Guarantees at least one nearby option

---

## ZIAD Algorithm

**How it shows restaurants:**

• **Waste-Reduction Focused Scoring**

**Scoring Formula:**
  - Score = (Price Weight × Price) + (Rating Weight × Rating) + (Unsold Weight × Unsold Bags)
  - Weights:
    - Price: -0.01 (negative - lower price is better)
    - Rating: +1.5 (positive - higher rating is better)
    - Unsold Bags: +0.1 (positive - more unsold = higher priority for waste reduction)

**Selection:**
  - Calculates score for each restaurant
  - Displays top 5 restaurants (or n_displayed, whichever is smaller)
  - Respects distance threshold (excludes restaurants too far)

• **Key Features:**
  - Explicitly prioritizes stores with high unsold inventory
  - Balances price, quality (rating), and waste reduction
  - Simple, transparent scoring system
  - Focused on minimizing food waste

---


