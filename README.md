# Food Waste Marketplace Simulation

A comprehensive simulation system for evaluating ranking algorithms in a food waste marketplace platform, similar to Too Good To Go. This project includes both a C++ simulation engine and an interactive web dashboard for visualizing results.

## 📋 Table of Contents

- [Overview](#overview)
- [Project Structure](#project-structure)
- [Features](#features)
- [Getting Started](#getting-started)
- [Analysis Folder (C++ Simulation)](#analysis-folder-c-simulation)
- [Dashboard Folder (Web Interface)](#dashboard-folder-web-interface)
- [Ranking Algorithms](#ranking-algorithms)
- [Data Format](#data-format)
- [Links](#links)
- [Contributors](#contributors)

## 🎯 Overview

This project simulates a food waste marketplace where restaurants sell surplus food bags to customers at discounted prices. The simulation evaluates different ranking algorithms to optimize for multiple objectives:

- **Waste Reduction**: Minimize unsold food bags
- **Revenue Maximization**: Increase total platform revenue
- **Customer Satisfaction**: Improve conversion rates and reduce churn
- **Fair Exposure**: Ensure equitable distribution of impressions across restaurants

The system runs 7-day simulations with 100 customer arrivals per day (700 total customers) and compares six different ranking algorithms against a baseline.

## 📁 Project Structure

```
Food_App_project/
│
├── analysis/                 # C++ simulation engine
│   ├── *.cpp, *.h           # Core simulation source files
│   ├── main.cpp             # Main entry point
│   ├── stores.csv           # Restaurant data
│   ├── customer.csv         # Customer data
│   ├── *.exe               # Compiled executables
│   └── simulation_results_*.csv  # Algorithm-specific results
│
└── dashboard/               # Next.js web dashboard
    ├── app/                 # Next.js app directory
    ├── components/          # React components
    ├── lib/                 # Simulation logic (TypeScript)
    ├── types/               # TypeScript type definitions
    └── package.json         # Node.js dependencies
```

## ✨ Features

### Simulation Engine
- Multi-day discrete-event simulation
- Dynamic inventory management with actual vs. estimated bags
- Customer decision modeling with Softmax probabilistic selection
- Reservation system with end-of-day processing
- Comprehensive metrics collection (sales, revenue, waste, fairness)

### Web Dashboard
- Interactive CSV file upload
- Real-time simulation execution
- Visual comparison of ranking algorithms
- Detailed metrics tables and charts
- Responsive design with modern UI

## 🚀 Getting Started

### Prerequisites

**For C++ Simulation:**
- C++ compiler with C++11 support (g++, clang++, or MSVC)
- Standard C++ libraries (no external dependencies)

**For Web Dashboard:**
- Node.js 18+ and npm
- Modern web browser

## 📊 Analysis Folder (C++ Simulation)

The `analysis/` folder contains the core C++ simulation engine that runs all ranking algorithms and generates comparison reports.

### Quick Start

1. **Navigate to the analysis directory:**
   ```bash
   cd analysis
   ```

2. **Compile the simulation:**
   ```bash
   g++ -std=c++11 -O2 main.cpp SimulationEngine.cpp Restaurant.cpp Customer.cpp \
       CustomerDecisionSystem.cpp RestaurantManagementSystem.cpp RankingAlgorithms.cpp \
       Metrics.cpp MarketState.cpp Reservation.cpp Timestamp.cpp RestaurantLoader.cpp \
       ArrivalGenerator.cpp -o simulation.exe
   ```

3. **Run the simulation:**
   ```bash
   ./simulation.exe    # Windows
   ./simulation        # Linux/Mac
   ```

### Output Files

The simulation generates several output files:

- **`algorithm_comparison_report.txt`**: Comprehensive comparison of all algorithms
- **`detailed_simulation_log.txt`**: Day-by-day simulation logs
- **`simulation_results_[ALGORITHM].csv`**: Per-restaurant metrics for each algorithm

### Key Files

- **`main.cpp`**: Entry point, orchestrates simulation runs
- **`RankingAlgorithms.cpp`**: Implements all 6 ranking algorithms
- **`CustomerDecisionSystem.cpp`**: Customer behavior and selection logic
- **`RestaurantManagementSystem.cpp`**: End-of-day reservation processing
- **`Metrics.cpp`**: Metrics collection and Gini coefficient calculation

## 🌐 Dashboard Folder (Web Interface)

The `dashboard/` folder contains a Next.js/TypeScript web application that provides an interactive interface for running simulations and visualizing results.

### Quick Start

1. **Navigate to the dashboard directory:**
   ```bash
   cd dashboard
   ```

2. **Install dependencies:**
   ```bash
   npm install
   ```

3. **Run the development server:**
   ```bash
   npm run dev
   ```

4. **Open in browser:**
   ```
   http://localhost:3000
   ```

### Deployment

The dashboard is configured for deployment on Vercel:

1. **Build for production:**
   ```bash
   npm run build
   ```

2. **Deploy to Vercel:**
   - Connect your GitHub repository to Vercel
   - Select the `dashboard` folder as the root directory
   - Deploy automatically on every push

### Features

- **File Upload**: Upload `customer.csv` and `stores.csv` files
- **Interactive Simulation**: Run simulations with real-time progress
- **Algorithm Comparison**: Compare all 6 algorithms side-by-side
- **Visualizations**: Charts and tables for metrics analysis
- **Responsive Design**: Works on desktop and mobile devices

## 🧮 Ranking Algorithms

The project implements and compares six ranking algorithms:

1. **BASELINE**: Simple rating-based ranking (highest rated first)
2. **SAMA**: Multi-objective optimization with personalization and waste reduction
3. **ANDREW**: Fairness-focused with impression count damping
4. **AMER**: Proximity-based with closest store guarantee
5. **ZIAD**: Weighted linear combination (price, rating, unsold bags)
6. **HARMONY**: Unified strategy combining all strengths (final/best algorithm)

Each algorithm displays 5 restaurants (`N_DISPLAYED=5`) to each customer based on their specific ranking logic.

## 📄 Data Format

### stores.csv

Restaurant data with the following columns:

```csv
store_id,store_name,branch,average_bags_at_9AM,average_overall_rating,price,longitude,latitude,business_type
1,Krispy Kreme,Zamalek,4,4.8,80.0,31.22,30.05,bakery
...
```

- `store_id`: Unique restaurant identifier
- `average_bags_at_9AM`: Estimated daily inventory
- `average_overall_rating`: Initial rating (1.0-5.0)
- `price`: Price per bag
- `longitude`, `latitude`: Geographic coordinates
- `business_type`: Category (bakery, cafe, restaurant)

### customer.csv

Customer data with the following columns:

```csv
CustomerID,longitude,latitude,store1_id_valuation,store2_id_valuation,...,store15_id_valuation
1,31.20801,30.03583,0.85862,0.10032,...,0.41516
...
```

- `CustomerID`: Unique customer identifier
- `longitude`, `latitude`: Customer location
- `storeX_id_valuation`: Preference value [0,1] for each restaurant

## 🔗 Links

### GitHub Repository
📦 **Source Code**: [https://github.com/Sama3mad/Algorithmic-Ranking-for-Food-Waste-Apps.git](https://github.com/Sama3mad/Algorithmic-Ranking-for-Food-Waste-Apps.git)

### Live Dashboard
🌐 **Interactive Dashboard**: [food-waste-dpp-dashboard2.vercel.app]

### Documentation
📖 **Project Report**: See `PROJECT_REPORT.txt` in the repository for detailed documentation including:
- System description and assumptions
- Algorithm design and justification
- Evaluation metrics
- Results analysis
- Code walkthrough

## 📈 Simulation Parameters

Default simulation configuration:

- **Simulation Period**: 7 days
- **Customers per Day**: 100
- **Total Customers**: 700
- **Stores Displayed**: 5 (`N_DISPLAYED`)
- **Max Travel Distance**: 0.05 (Euclidean distance)
- **Max Bags per Customer**: 3
- **Inventory Variance**: 80-120% of estimated bags
- **Temperature (Softmax)**: 2.0

## 📊 Evaluation Metrics

The simulation tracks comprehensive metrics:

- **Sales Metrics**: Bags sold, bags cancelled, bags unsold (waste)
- **Revenue Metrics**: Revenue generated, revenue lost, revenue efficiency
- **Customer Metrics**: Conversion rate, customers who left
- **Fairness Metrics**: Gini coefficient for exposure distribution

## 🔧 Customization

### Modify Simulation Parameters

Edit the following in the source code:

- **`main.cpp`**: Change `run_multi_day_simulation(7, 100)` for different periods/customers
- **`Customer.cpp`**: Modify `MAX_TRAVEL_DISTANCE` (default: 0.05)
- **`Restaurant.h`**: Change `max_bags_per_customer` (default: 3)
- **`SimulationEngine.cpp`**: Adjust `N_DISPLAYED` (default: 5)

### Add New Ranking Algorithms

1. Add algorithm enum to `RankingAlgorithms.h`
2. Implement algorithm in `RankingAlgorithms.cpp`
3. Add case to dispatcher function `get_displayed_stores()`
4. Update `main.cpp` to include in algorithm list

## 🤝 Contributors

- **Sama**: Algorithm design and implementation
- **Team Members**: Algorithm contributions (SAMA, ANDREW, AMER, ZIAD algorithms)

## 📝 License

This project is part of an academic research project on algorithmic ranking for food waste applications.

## 🙏 Acknowledgments

- Inspired by food waste reduction platforms like Too Good To Go
- Uses standard algorithms: Softmax for probabilistic selection, Gini coefficient for fairness measurement

---

**Note**: For detailed technical documentation, algorithm descriptions, and implementation details, please refer to `PROJECT_REPORT.txt` in the repository root.

