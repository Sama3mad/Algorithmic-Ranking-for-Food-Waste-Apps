# Food Waste Marketplace Simulation - Function Documentation

## Overview
This project simulates a food waste marketplace where local stores offer "surprise bags" of discounted food to students. The simulation evaluates different ranking algorithms to minimize food waste, maximize revenue, and ensure fair exposure for all stores.

## Project Structure

### Core Data Structures

#### `Timestamp` (Timestamp.h/cpp)
Represents a time of day with hour and minute.

**Functions:**
- **`Timestamp(int h = 8, int m = 0)`**: Constructor. Default is 8:00 AM (start of business day).
- **`int to_minutes() const`**: Converts timestamp to total minutes since midnight. Used for time comparisons and sorting.
- **`bool operator<(const Timestamp& other) const`**: Comparison operator for sorting reservations by time.
- **`std::string to_string() const`**: Converts to human-readable string format (e.g., "14:30").

---

#### `CustomerHistory` (Customer.h)
Tracks a customer's interaction history with the platform.

**Data Members:**
- `visits`: Total number of times customer opened the app
- `reservations`: Total reservation attempts
- `successes`: Successful reservations (confirmed)
- `cancellations`: Cancelled reservations
- `last_reservation_time`: When they last made a reservation
- `categories_reserved`: Map of categories (bakery, cafe, restaurant) they've tried
- `store_interactions`: Map of store_id -> StoreInteraction (tracks per-store history)

**Nested Struct `StoreInteraction`:**
- `reservations`: Times they tried to reserve from this store
- `successes`: Successful reservations
- `cancellations`: Cancelled reservations

---

#### `Customer` (Customer.h/cpp)
Represents a customer in the marketplace.

**Data Members:**
- `id`: Unique customer identifier
- `longitude`, `latitude`: Customer location
- `customer_name`: Customer's name
- `segment`: Customer segment ("budget", "regular", "premium")
- `willingness_to_pay`: Maximum price customer is willing to pay
- `weights`: Weights for decision-making factors (rating_w, price_w, novelty_w)
- `loyalty`: Customer loyalty (0.0-1.0, affects leaving threshold)
- `leaving_threshold`: Minimum score needed to make a purchase
- `history`: Customer's interaction history
- `churned`: Whether customer has left the platform
- `category_preference`: Preferences for different food categories
- `store_valuations`: Map of store_id -> valuation (from CSV)

**Functions:**
- **`Customer()`**: Default constructor. Creates customer with default values.
- **`Customer(int customer_id, const std::string& seg = "regular")`**: Constructor with ID and segment.
- **`Customer(int id_, float lon_, float lat_, const std::string& name_, const std::string& segment_, float wtp, float rating_weight, float price_weight, float novelty_weight, float leaving_thresh)`**: Full constructor with all parameters.
- **`float calculate_store_score(const Restaurant& store) const`**: 
  - **Logic**: Calculates how much this customer likes a specific store.
  - **Formula**: `rating_score + price_score + novelty_score`
    - `rating_score = weights.rating_w * store.get_rating()`
    - `price_score = weights.price_w * (willingness_to_pay - store.price_per_bag) / willingness_to_pay`
    - `novelty_score = weights.novelty_w * (1.0f / (1.0f + times_tried_category))`
  - **Returns**: Total score (higher = customer likes store more)

- **`void update_loyalty(bool was_cancelled)`**: 
  - **Logic**: Updates customer loyalty based on reservation outcome.
  - If cancelled: decreases loyalty by 0.1 (min 0.0)
  - If successful: increases loyalty by 0.05 (max 1.0)

- **`void update_category_preference(const std::string& category)`**: 
  - **Logic**: Increases preference for a category after successful purchase by 0.1.

- **`void record_visit()`**: Increments visit count.

- **`void record_reservation_attempt(int store_id, const std::string& category, Timestamp time)`**: 
  - **Logic**: Records that customer attempted to reserve from a store.
  - Updates reservation count, last reservation time, category history, and store interaction history.

- **`void record_reservation_success(int store_id, const std::string& category)`**: 
  - **Logic**: Records successful reservation.
  - Updates success count, store interaction success, and increases category preference.

- **`void record_reservation_cancellation(int store_id)`**: 
  - **Logic**: Records cancelled reservation.
  - Updates cancellation count, store interaction cancellation, and decreases loyalty.

---

#### `Restaurant` (Restaurant.h/cpp)
Represents a restaurant/store in the marketplace.

**Data Members:**
- `business_id`: Unique store identifier
- `business_name`: Store name
- `general_ranking`: Current dynamic rating (1.0-5.0)
- `business_type`: Category ("bakery", "cafe", "restaurant")
- `estimated_bags`: Estimated bags at start of day (9 AM)
- `price_per_bag`: Price per surprise bag
- `actual_bags`: Actual bags available at end of day (10 PM)
- `reserved_count`: Number of reservations made
- `has_inventory`: Whether store still has inventory
- `max_bags_per_customer`: Maximum bags a single customer can receive (default: 3)
- `total_orders_confirmed`: Total confirmed orders (for rating updates)
- `total_orders_cancelled`: Total cancelled orders (for rating updates)
- `initial_rating`: Initial rating (for comparison)

**Functions:**
- **`Restaurant(int id, const std::string& name, const std::string& branch_name, int est_bags, float rating, float price, float lon, float lat, const std::string& type = "")`**: 
  - **Purpose**: Constructor for CSV loading (with all required CSV columns).
  - **Parameters**: All CSV format columns plus optional business_type.

- **`Restaurant(int id, const std::string& name, float rating, const std::string& type, int est_bags, float price)`**: 
  - **Purpose**: Legacy constructor (for backward compatibility).
  - **Note**: Sets branch, longitude, latitude to default values.

- **`bool can_accept_reservation() const`**: 
  - **Logic**: Checks if restaurant can accept more reservations.
  - **Returns**: `true` if has inventory AND reserved_count < estimated_bags

- **`void set_actual_inventory(int bags)`**: Sets actual inventory at end of day.

- **`void update_rating_on_confirmation()`**: 
  - **Logic**: Updates rating when order is confirmed (positive feedback).
  - Increases rating by 0.01, capped at 5.0.
  - Increments `total_orders_confirmed`.

- **`void update_rating_on_cancellation()`**: 
  - **Logic**: Updates rating when order is cancelled (negative feedback).
  - Decreases rating by 0.05, capped at 1.0.
  - Increments `total_orders_cancelled`.

- **`float get_rating() const`**: Returns current dynamic rating.

---

#### `Reservation` (Reservation.h/cpp)
Represents a customer's reservation for a surprise bag.

**Data Members:**
- `reservation_id`: Unique reservation identifier
- `customer_id`: ID of customer who made reservation
- `restaurant_id`: ID of restaurant
- `reservation_time`: When reservation was made
- `status`: Current status (PENDING, CONFIRMED, CANCELLED)

**Functions:**
- **`Reservation(int res_id, int cust_id, int rest_id, Timestamp time)`**: Constructor. Initializes status to PENDING.

---

#### `MarketState` (MarketState.h/cpp)
Central state management for the entire marketplace.

**Data Members:**
- `restaurants`: Vector of all restaurants
- `customers`: Map of customer_id -> Customer
- `reservations`: Vector of all reservations
- `current_time`: Current simulation time
- `next_reservation_id`: Next available reservation ID

**Functions:**
- **`MarketState()`**: Constructor. Initializes time to 8:00 AM, next_reservation_id to 1.

- **`std::vector<int> get_available_restaurant_ids() const`**: 
  - **Logic**: Gets list of restaurant IDs that can still accept reservations.
  - **Returns**: Vector of restaurant IDs where `can_accept_reservation()` is true.

- **`Restaurant* get_restaurant(int id)`**: 
  - **Logic**: Gets restaurant by ID.
  - **Returns**: Pointer to restaurant, or nullptr if not found.

- **`Customer* get_customer(int id)`**: 
  - **Logic**: Gets customer by ID.
  - **Returns**: Pointer to customer, or nullptr if not found.

---

### Ranking Algorithms

#### `RankingAlgorithm` Enum (RankingAlgorithms.h)
- `BASELINE`: Top-N by rating only (same stores for all customers)
- `SMART`: Personalized + fairness + waste reduction

#### `get_displayed_stores_baseline` (RankingAlgorithms.cpp)
- **Purpose**: Baseline ranking algorithm - shows same top-rated stores to ALL customers.
- **Logic**:
  1. Get all available restaurants
  2. Sort by dynamic rating (descending) using `get_rating()`
  3. Return top N stores
- **Parameters**: `customer` (unused), `market_state`, `n_displayed`
- **Returns**: Vector of store IDs (top N by rating)

#### `get_displayed_stores_smart` (RankingAlgorithms.cpp)
- **Purpose**: Smart ranking algorithm that balances personalization, discovery, and fairness.
- **Logic**:
  1. **Step 1 (Personalized)**: Calculate scores for all available stores based on customer preferences using `customer.calculate_store_score()`. Select top 3 stores.
  2. **Step 2 (New Store Discovery)**: Find stores customer has never visited. Select one with good (but not necessarily top) rating from top 50% of new stores (for fairness).
  3. **Step 3 (Price-Competitive)**: Find store with best value (rating/price ratio). Select it.
  4. **Step 4 (Fill Remaining)**: Fill any remaining slots with best available stores based on customer preference.
- **Parameters**: `customer`, `market_state`, `n_displayed`
- **Returns**: Vector of store IDs (3 personalized + 1 new + 1 price-competitive + fill remaining)

#### `get_displayed_stores` (RankingAlgorithms.cpp)
- **Purpose**: Unified ranking function that routes to appropriate algorithm.
- **Logic**: Calls `get_displayed_stores_smart` if algorithm is SMART, otherwise calls `get_displayed_stores_baseline`.
- **Parameters**: `customer`, `market_state`, `n_displayed`, `algorithm`
- **Returns**: Vector of store IDs

---

### Customer Decision System

#### `CustomerDecisionSystem` (CustomerDecisionSystem.h/cpp)
Handles customer decision-making logic.

#### `process_customer_arrival`
- **Purpose**: Main entry point for processing a customer's arrival.
- **Logic**:
  1. Record customer visit
  2. Get displayed stores using ranking algorithm
  3. If no stores available, customer churns
  4. Calculate scores for displayed stores
  5. Select store (probabilistic selection)
  6. If no store selected, customer churns
  7. Create reservation
  8. If reservation fails, customer churns
- **Parameters**: `customer`, `market_state`, `n_displayed`, `algorithm`
- **Returns**: Store ID if reservation created, -1 if customer left

#### `calculate_store_scores`
- **Purpose**: Calculate scores for displayed stores based on customer preferences.
- **Logic**: For each displayed store, calls `customer.calculate_store_score(store)`.
- **Parameters**: `customer`, `displayed_store_ids`, `market_state`
- **Returns**: Vector of scores (one per displayed store)

#### `select_store`
- **Purpose**: Select which store customer will choose (probabilistic selection).
- **Logic**:
  1. Calculate threshold based on `customer.leaving_threshold` and `customer.loyalty` (lower loyalty = higher threshold)
  2. Find maximum score
  3. If max score < threshold, customer leaves
  4. Adjust scores based on customer history:
     - Boost score for stores with good success rate (+1.5 * success_rate)
     - Penalize stores with cancellations (-2.0 * cancel_rate)
  5. Filter stores below threshold
  6. Use probabilistic selection (softmax) to choose store
- **Parameters**: `customer`, `displayed_store_ids`, `scores`, `market_state`
- **Returns**: Store ID or -1 if customer leaves

#### `probabilistic_select`
- **Purpose**: Probabilistic store selection using softmax (mimics real behavior).
- **Logic**:
  1. Normalize scores to positive range
  2. Apply exponential with temperature parameter (default 2.0): `exp(score / temperature)`
  3. Normalize to probabilities (softmax)
  4. Use weighted random selection based on probabilities
- **Parameters**: `store_ids`, `all_scores`, `valid_indices`, `valid_scores`
- **Returns**: Selected store ID

#### `create_reservation`
- **Purpose**: Create a reservation for the customer.
- **Logic**:
  1. Check if restaurant can accept reservation
  2. Create new Reservation with next_reservation_id
  3. Record reservation attempt in customer history
  4. Increment restaurant's reserved_count
  5. Add reservation to market_state
- **Parameters**: `customer`, `restaurant_id`, `market_state`
- **Returns**: `true` if successful, `false` otherwise

---

### Restaurant Management System

#### `RestaurantManagementSystem` (RestaurantManagementSystem.h/cpp)
Handles end-of-day processing for restaurants.

#### `process_end_of_day`
- **Purpose**: Process end of day: confirm/cancel reservations based on actual inventory.
- **Logic**: For each restaurant:
  1. Get all pending reservations for this restaurant
  2. Sort by reservation time (first come, first served)
  3. **Case 1**: No orders received → all bags become waste (handled in metrics)
  4. **Case 2**: Actual bags >= reserved count (enough/more bags):
     - Distribute bags fairly among customers
     - Calculate `bags_per_customer = min(max_bags_per_customer, actual_bags / num_reservations)`
     - Distribute extra bags (one per customer, up to max)
     - Confirm all reservations
  5. **Case 3**: Actual bags < reserved count (shortage):
     - Confirm first `actual_bags` reservations (first come, first served)
     - Cancel the rest
- **Parameters**: `market_state`

#### `handle_cancellation`
- **Purpose**: Handle reservation cancellation.
- **Logic**:
  1. Set reservation status to CANCELLED
  2. Record cancellation in customer history
  3. Update restaurant rating (decrease by 0.05)
- **Parameters**: `reservation`, `customer`, `market_state`

#### `handle_confirmation`
- **Purpose**: Handle reservation confirmation.
- **Logic**:
  1. Set reservation status to CONFIRMED
  2. Record success in customer history
  3. Update restaurant rating (increase by 0.01)
- **Parameters**: `reservation`, `customer`, `market_state`, `bags_received` (default: 1)

---

### Restaurant Loader

#### `RestaurantLoader` (RestaurantLoader.h/cpp)
Handles loading restaurants from CSV file.

**Functions:**
- **`static bool load_restaurants_from_csv(const std::string& filename, std::vector<Restaurant>& restaurants)`**: 
  - **Purpose**: Load restaurants from CSV file.
  - **Logic**:
    1. Opens CSV file
    2. Reads header and maps column names to indices (case-insensitive)
    3. Validates all required columns are present (store_id, store_name, branch, average_bags_at_9AM, average_overall_rating, price, longitude, latitude)
    4. Reads each row and parses values
    5. Creates Restaurant objects with CSV data
    6. Handles optional extra columns (like business_type)
    7. Handles quoted values in CSV
  - **Returns**: `true` if file loaded successfully, `false` otherwise
  - **CSV Format**: `store_id, store_name, branch, average_bags_at_9AM, average_overall_rating, price, longitude, latitude, [optional: business_type]`

- **`static void generate_default_restaurants(std::vector<Restaurant>& restaurants)`**: 
  - **Purpose**: Generate default restaurants (fallback if CSV not available).
  - **Logic**: Creates 10 default restaurants with all required CSV columns populated (including branch, longitude, latitude).

---

### Arrival Generator

#### `ArrivalGenerator` (ArrivalGenerator.h/cpp)
Generates customer arrivals and loads customers from CSV.

#### `ArrivalGenerator(unsigned seed = std::time(nullptr))`
- **Purpose**: Constructor with optional seed for random number generator.

#### `ArrivalGenerator(const std::string& csv_path, unsigned seed = std::time(nullptr))`
- **Purpose**: Constructor that tries to load customers from CSV file.

#### `generate_arrival_times`
- **Purpose**: Generate random arrival times for customers (8 AM - 9 PM).
- **Logic**:
  1. Generate random hour (8-21) and minute (0-59) for each customer
  2. Sort times chronologically
- **Parameters**: `num_customers`
- **Returns**: Sorted vector of timestamps

#### `load_customers_from_csv`
- **Purpose**: Load customers from CSV file with flexible format support.
- **Logic**:
  1. Read header to identify column types
  2. Parse user format columns first:
     - `latitude`, `longitude` (required)
     - `storeX_id_valuation` columns (extract store ID from column name)
  3. Parse code format columns (optional):
     - `id`, `lon`, `lat`, `name`, `segment`, `wtp`, `rating_w`, `price_w`, `novelty_w`, `leave_thresh`
     - Skip `lon` and `lat` if already read from user format
  4. Create Customer objects with all data
  5. Store in `customers_from_csv` vector
- **Parameters**: `filename`
- **Returns**: `true` if file loaded successfully, `false` otherwise

#### `generate_customer`
- **Purpose**: Generate a customer (from CSV if available, otherwise random).
- **Logic**:
  1. If CSV loaded: cycle through CSV customers (use modulo)
  2. If no CSV: generate random customer:
     - Random segment (budget, regular, premium)
     - Segment-specific attributes:
       - Budget: lower WTP (80-120), higher price sensitivity, lower leaving threshold
       - Regular: medium WTP (120-180), balanced weights
       - Premium: higher WTP (180-260), quality-focused, lower price sensitivity
     - Random location (Cairo area: lon 31.2-31.3, lat 30.0-30.1)
     - Generate random store valuations (0.0-5.0) for all restaurants
- **Parameters**: `index`, `restaurants` (optional, for generating store valuations)
- **Returns**: Customer object

---

### Metrics

#### `SimulationMetrics` (Metrics.h/cpp)
Stores all metrics collected during simulation.

**Data Members:**
- `total_bags_sold`: Total bags successfully sold
- `total_bags_cancelled`: Total bags cancelled (shortage)
- `total_bags_unsold`: Total bags unsold (waste - only if no orders)
- `total_revenue_generated`: Total revenue from confirmed orders
- `total_revenue_lost`: Revenue lost from cancellations
- `customers_who_left`: Customers who left without purchasing
- `total_customer_arrivals`: Total customers who visited app
- `bags_sold_per_store`: Map of store_id -> bags sold
- `bags_cancelled_per_store`: Map of store_id -> bags cancelled
- `revenue_per_store`: Map of store_id -> revenue
- `times_displayed_per_store`: Map of store_id -> exposure count
- `waste_per_store`: Map of store_id -> waste (bags)
- `gini_coefficient_exposure`: Fairness metric (0=equal, 1=unequal)

**Functions:**
- **`SimulationMetrics()`**: Constructor. Initializes all metrics to zero.

- **`void print_summary() const`**: 
  - **Purpose**: Print formatted summary of all metrics.
  - **Output**: Sales metrics, revenue metrics, customer metrics, fairness metrics.

#### `MetricsCollector` (Metrics.h/cpp)
Collects and calculates metrics throughout the simulation.

**Functions:**
- **`void log_customer_arrival(int customer_id, Timestamp time)`**: Increments total customer arrivals.

- **`void log_stores_displayed(const std::vector<int>& store_ids)`**: 
  - **Purpose**: Log which stores were displayed to a customer.
  - **Logic**: Increments `times_displayed_per_store` for each displayed store.

- **`void log_reservation(const Reservation& res, float price)`**: Placeholder (finalized at end of day).

- **`void log_customer_left(int customer_id)`**: Increments customers who left.

- **`void log_cancellation(const Reservation& res, float lost_revenue)`**: 
  - **Purpose**: Log a cancellation.
  - **Logic**: Increments cancelled bags, adds to revenue lost, updates per-store metrics.

- **`void log_confirmation(const Reservation& res)`**: Increments total bags sold.

- **`void log_end_of_day(const MarketState& market_state)`**: 
  - **Purpose**: Calculate end-of-day metrics from final state.
  - **Logic**: For each restaurant:
    1. Count confirmed and cancelled reservations
    2. Calculate revenue (price_per_bag * confirmed)
    3. Calculate waste:
       - If no orders received: waste = actual_bags
       - If orders received: waste = 0 (bags distributed to customers)
    4. Update all per-store metrics

- **`void calculate_fairness_metrics(const MarketState& market_state)`**: 
  - **Purpose**: Calculate fairness metrics (Gini coefficient).
  - **Logic**:
    1. Collect exposure counts for all stores
    2. Sort exposures
    3. Calculate Gini coefficient using formula:
       `G = (2 * weighted_sum) / (n * sum) - (n + 1) / n`
    4. Store in `gini_coefficient_exposure`
  - **Gini Coefficient**: 0 = perfect equality, 1 = maximum inequality

---

### Simulation Engine

#### `SimulationEngine` (SimulationEngine.h/cpp)
Main simulation controller that orchestrates the entire day simulation.

**Data Members:**
- `market_state`: Current market state
- `metrics_collector`: Metrics collection
- `arrival_generator`: Customer generation
- `n_displayed`: Number of stores to display
- `ranking_algorithm`: Which ranking algorithm to use

**Functions:**
- **`SimulationEngine(int n_display, const std::string& customer_csv, RankingAlgorithm algorithm = RankingAlgorithm::BASELINE)`**: 
  - **Purpose**: Constructor.
  - **Parameters**: 
    - `n_display`: Number of stores to show each customer
    - `customer_csv`: Path to customer CSV file (optional)
    - `algorithm`: Ranking algorithm to use

- **`void initialize(const std::vector<Restaurant>& restaurants)`**: 
  - **Purpose**: Initialize simulation with restaurants.
  - **Logic**:
    1. Set restaurants in market_state
    2. Simulate actual inventory (80-120% of estimate) using random variance
    3. Set actual_bags for each restaurant

- **`void run_day_simulation(int num_customers)`**: 
  - **Purpose**: Run a full day simulation.
  - **Logic**:
    1. Print initial store inventory (estimated, actual, price, rating)
    2. Generate customer arrival times
    3. For each customer:
       - Generate customer (from CSV or random)
       - Set current time to arrival time
       - Log customer arrival
       - Get displayed stores using ranking algorithm
       - Log displayed stores
       - Process customer arrival (select store, create reservation)
       - Log if customer left
    4. Process end of day (confirm/cancel reservations)
    5. Log end of day metrics
    6. Calculate fairness metrics
    7. Print rating changes summary

- **`const SimulationMetrics& get_metrics() const`**: Returns current metrics.

- **`void export_results(const std::string& filename)`**: 
  - **Purpose**: Export results to CSV file.
  - **Logic**: Writes CSV with columns: Restaurant, Estimated, Actual, Reserved, Sold, Cancelled, Waste, Revenue, Exposures.

- **`void log_detailed_metrics(const SimulationMetrics* comparison_metrics = nullptr)`**: 
  - **Purpose**: Write detailed metrics log to file.
  - **Logic**:
    1. Append to `simulation_log.txt`
    2. Write overall metrics (bags sold, cancelled, waste, revenue, conversion rate, Gini coefficient)
    3. Write per-store metrics (rating changes, orders, bags, revenue, exposures)
    4. If comparison_metrics provided, add comparison section

---

### Main Function

#### `main` (main.cpp)
Entry point of the program.

**Logic:**
1. Create initial restaurant database (10 restaurants)
2. Run simulation with BASELINE algorithm:
   - Initialize SimulationEngine with BASELINE algorithm
   - Run day simulation with 100 customers
   - Print results summary
   - Export to `simulation_results_baseline.csv`
   - Write detailed log
3. Run simulation with SMART algorithm:
   - Initialize SimulationEngine with SMART algorithm
   - Run day simulation with 100 customers
   - Print results summary
   - Export to `simulation_results_smart.csv`
   - Write detailed log with comparison
4. Print algorithm comparison summary table

---

## Compilation

To compile the project:

```bash
g++ -std=c++11 -o simulation *.cpp
```

Or compile each file separately:

```bash
g++ -std=c++11 -c Timestamp.cpp Customer.cpp Restaurant.cpp Reservation.cpp MarketState.cpp RankingAlgorithms.cpp CustomerDecisionSystem.cpp RestaurantManagementSystem.cpp ArrivalGenerator.cpp Metrics.cpp SimulationEngine.cpp RestaurantLoader.cpp main.cpp
g++ -std=c++11 -o simulation *.o
```

## Running

```bash
./simulation
```

The program will:
1. Run simulation with BASELINE algorithm
2. Run simulation with SMART algorithm
3. Output comparison summary
4. Generate CSV files: `simulation_results_baseline.csv`, `simulation_results_smart.csv`
5. Generate log file: `simulation_log.txt`

## Input Files

### `stores.csv` (Optional)
If present, restaurants are loaded from this file. Format:
- **Required columns** (in order):
  - `store_id`: Unique integer ID for the store (e.g., 100, 200, ...)
  - `store_name`: Store's name (e.g., "TBS", "Dunkin", ...)
  - `branch`: Branch name or area (e.g., "Zamalek", "New Cairo")
  - `average_bags_at_9AM`: Estimated number of surprise bags at 9AM (int)
  - `average_overall_rating`: Average store rating (numeric, 1-5)
  - `price`: Bag price in minor units (e.g., 10.00, 8.99, 50.00 EGP)
  - `longitude`: Store longitude (float)
  - `latitude`: Store latitude (float)
- **Optional extra columns**:
  - `business_type`: Category (e.g., "bakery", "cafe", "restaurant")

**Example CSV:**
```csv
store_id,store_name,branch,average_bags_at_9AM,average_overall_rating,price,longitude,latitude,business_type
1,Krispy Kreme,Zamalek,10,4.8,80.0,31.22,30.05,bakery
2,TBS Pizza,New Cairo,10,4.2,150.0,31.25,30.08,restaurant
```

If file is not present, default restaurants are generated automatically.

### `customer.csv` (Optional)
If present, customers are loaded from this file. Format:
- **User format** (required): `latitude, longitude, store1_id_valuation, store2_id_valuation, ...`
- **Code format** (optional): `id, lon, lat, name, segment, wtp, rating_w, price_w, novelty_w, leave_thresh`

If file is not present, customers are generated randomly.

## Output Files

- `simulation_results_baseline.csv`: Results for baseline algorithm
- `simulation_results_smart.csv`: Results for smart algorithm
- `simulation_log.txt`: Detailed log with metrics and comparisons

---

## Key Algorithms and Logic

### Dynamic Rating System
- Ratings change based on order outcomes:
  - Confirmation: +0.01 (capped at 5.0)
  - Cancellation: -0.05 (capped at 1.0)
- Ratings are used in ranking algorithms

### Waste Calculation
- Waste only occurs if restaurant received NO orders
- If orders received, all bags are distributed to customers (respecting max_bags_per_customer)
- This prevents waste when restaurants have extra inventory

### Probabilistic Customer Choice
- Customers don't always pick the "best" option
- Uses softmax with temperature parameter (2.0)
- Higher scores have higher probability, but not guaranteed
- Mimics real customer behavior

### Fairness Metric (Gini Coefficient)
- Measures inequality in store exposure
- 0 = perfect equality (all stores shown equally)
- 1 = maximum inequality (one store dominates)
- Lower is better for fairness

---

## Notes

- The simulation runs from 8:00 AM to 10:00 PM
- Actual inventory is simulated as 80-120% of estimated inventory
- Customer segments (budget, regular, premium) have different preferences and behaviors
- The SMART algorithm balances personalization, discovery, and fairness to reduce waste and increase revenue

