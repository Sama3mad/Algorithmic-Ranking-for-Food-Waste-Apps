'use client'

import { useState } from 'react'

export default function InfoSection() {
  const [activeTab, setActiveTab] = useState<'overview' | 'algorithms' | 'simulation'>('overview')

  return (
    <div className="bg-white rounded-xl shadow-lg p-8 mb-8">
      <div className="border-b border-gray-200 mb-6">
        <nav className="flex space-x-8">
          <button
            onClick={() => setActiveTab('overview')}
            className={`py-4 px-2 border-b-2 font-medium text-sm ${
              activeTab === 'overview'
                ? 'border-blue-500 text-blue-600'
                : 'border-transparent text-gray-500 hover:text-gray-700 hover:border-gray-300'
            }`}
          >
            Project Overview
          </button>
          <button
            onClick={() => setActiveTab('algorithms')}
            className={`py-4 px-2 border-b-2 font-medium text-sm ${
              activeTab === 'algorithms'
                ? 'border-blue-500 text-blue-600'
                : 'border-transparent text-gray-500 hover:text-gray-700 hover:border-gray-300'
            }`}
          >
            Ranking Algorithms
          </button>
          <button
            onClick={() => setActiveTab('simulation')}
            className={`py-4 px-2 border-b-2 font-medium text-sm ${
              activeTab === 'simulation'
                ? 'border-blue-500 text-blue-600'
                : 'border-transparent text-gray-500 hover:text-gray-700 hover:border-gray-300'
            }`}
          >
            How Simulation Works
          </button>
        </nav>
      </div>

      <div className="prose max-w-none">
        {activeTab === 'overview' && (
          <div className="space-y-4">
            <h3 className="text-2xl font-bold text-gray-900">Project Overview</h3>
            <p className="text-gray-700 leading-relaxed">
              This dashboard presents a comprehensive simulation of a food waste marketplace platform where restaurants 
              can list and sell surplus food bags to customers. The simulation evaluates six distinct ranking algorithms 
              to determine their effectiveness in minimizing food waste, optimizing revenue generation, and ensuring 
              equitable exposure across all participating restaurants.
            </p>
            
            <div className="bg-blue-50 rounded-lg p-4 mt-6">
              <h4 className="font-semibold text-blue-900 mb-2">Simulation Parameters and Assumptions</h4>
              <ul className="list-disc list-inside text-gray-700 space-y-2">
                <li>The simulation period spans <strong>7 consecutive days</strong>, with <strong>100 customer arrivals per day</strong>, resulting in a total of 700 customer interactions</li>
                <li>Each customer is presented with exactly <strong>5 restaurant options</strong> per visit, selected by the active ranking algorithm</li>
                <li>Restaurants maintain both estimated and actual daily inventory levels, where actual inventory varies randomly between 80% and 120% of the estimated value to model real-world inventory uncertainty</li>
                <li>Customer decision-making incorporates multiple factors: restaurant ratings, price points, geographical distance (maximum travel distance of 0.05 coordinate units), and individual customer preferences including segment classification (budget, premium, regular)</li>
                <li>Reservations are subject to a <strong>10% cancellation rate</strong>, representing real-world scenarios where customers may change plans or encounter unforeseen circumstances</li>
                <li>Food bags that remain unsold at the end of each business day are classified as waste, contributing to the total waste metric</li>
                <li>Restaurant ratings are dynamically adjusted based on transaction outcomes: successful order confirmations increase ratings, while cancellations result in rating decreases</li>
                <li>When restaurant inventory exceeds reservation demand, customers may receive up to <strong>3 bags per reservation</strong> to accelerate waste reduction, with excess inventory distributed proportionally among early reservations</li>
                <li>Reservations are processed at the end of each day, with bag allocation prioritized by reservation timestamp when inventory is limited</li>
              </ul>
            </div>

            <div className="bg-green-50 rounded-lg p-4 mt-4">
              <h4 className="font-semibold text-green-900 mb-2">Performance Metrics</h4>
              <ul className="list-disc list-inside text-gray-700 space-y-2">
                <li><strong>Bags Sold:</strong> Total quantity of food bags successfully delivered to customers through confirmed reservations</li>
                <li><strong>Bags Cancelled:</strong> Number of reservations that were cancelled, either due to inventory constraints or customer-initiated cancellations</li>
                <li><strong>Bags Unsold (Waste):</strong> Total quantity of food bags that remained unsold at the end of each day, representing food waste</li>
                <li><strong>Revenue Generated:</strong> Total monetary value from confirmed sales, calculated as the sum of (bags received × price per bag) for all successful transactions</li>
                <li><strong>Revenue Lost:</strong> Potential revenue that was not realized due to reservation cancellations</li>
                <li><strong>Revenue Efficiency:</strong> Percentage of total potential revenue (generated + lost) that was successfully captured, calculated as (Revenue Generated / (Revenue Generated + Revenue Lost)) × 100</li>
                <li><strong>Conversion Rate:</strong> Percentage of arriving customers who completed a purchase, calculated as ((Total Arrivals - Customers Who Left) / Total Arrivals) × 100</li>
                <li><strong>Gini Coefficient:</strong> Statistical measure of fairness in restaurant exposure distribution, ranging from 0 (perfect equality) to 1 (maximum inequality), calculated based on the distribution of impression counts across all restaurants</li>
              </ul>
            </div>
          </div>
        )}

        {activeTab === 'algorithms' && (
          <div className="space-y-6">
            <h3 className="text-2xl font-bold text-gray-900">Ranking Algorithms</h3>
            
            <div className="space-y-4">
              <div className="border-l-4 border-gray-400 pl-4">
                <h4 className="font-semibold text-lg text-gray-900">BASELINE</h4>
                <p className="text-gray-700 mt-1">
                  The simplest approach - ranks restaurants purely by their average rating. This serves as our 
                  baseline for comparison. Higher-rated restaurants always appear first, regardless of other factors.
                </p>
              </div>

              <div className="border-l-4 border-blue-400 pl-4">
                <h4 className="font-semibold text-lg text-gray-900">SAMA</h4>
                <p className="text-gray-700 mt-1">
                  A sophisticated multi-objective optimization algorithm that balances personalization, waste reduction, 
                  fairness, and revenue. It considers customer segments (budget, premium, regular), inventory urgency, 
                  historical interactions, and category preferences to create highly personalized rankings.
                </p>
              </div>

              <div className="border-l-4 border-purple-400 pl-4">
                <h4 className="font-semibold text-lg text-gray-900">ANDREW</h4>
                <p className="text-gray-700 mt-1">
                  Focuses on fairness by tracking how often each restaurant is shown to customers. Restaurants with 
                  fewer impressions get a boost, ensuring more equitable exposure across all restaurants in the marketplace.
                </p>
              </div>

              <div className="border-l-4 border-green-400 pl-4">
                <h4 className="font-semibold text-lg text-gray-900">AMER</h4>
                <p className="text-gray-700 mt-1">
                  Prioritizes proximity by guaranteeing that the closest available restaurant is always shown. This 
                  algorithm ensures customers see nearby options first, which can improve conversion rates and reduce 
                  travel time.
                </p>
              </div>

              <div className="border-l-4 border-orange-400 pl-4">
                <h4 className="font-semibold text-lg text-gray-900">ZIAD</h4>
                <p className="text-gray-700 mt-1">
                  Uses a weighted scoring system that balances price, rating, and unsold inventory. It gives equal 
                  weight to affordability, quality, and waste reduction, creating a balanced ranking that appeals to 
                  value-conscious customers.
                </p>
              </div>

              <div className="border-l-4 border-pink-400 pl-4">
                <h4 className="font-semibold text-lg text-gray-900">HARMONY</h4>
                <p className="text-gray-700 mt-1">
                  A unified strategy that combines the best aspects of other algorithms. It balances inventory urgency, 
                  fairness (exposure tracking), and rating quality to create a well-rounded ranking system that 
                  performs well across multiple metrics.
                </p>
              </div>
            </div>
          </div>
        )}

        {activeTab === 'simulation' && (
          <div className="space-y-4">
            <h3 className="text-2xl font-bold text-gray-900">Simulation Execution Process</h3>
            
            <div className="space-y-4 text-gray-700">
              <div>
                <h4 className="font-semibold text-lg text-gray-900 mb-2">1. Initialization Phase</h4>
                <p>
                  The simulation initializes by loading restaurant data from stores.csv and customer data from customer.csv. 
                  Each restaurant is assigned an estimated inventory value from the input data, and actual daily inventory 
                  is calculated by applying a random variance factor between 0.8 and 1.2 to the estimated value. This variance 
                  models the inherent uncertainty in restaurant inventory management. Customer data includes geographical 
                  coordinates, segment classification, willingness to pay, and store valuation preferences.
                </p>
              </div>

              <div>
                <h4 className="font-semibold text-lg text-gray-900 mb-2">2. Customer Arrival Generation</h4>
                <p>
                  Each simulation day processes 100 customer arrivals, with arrival times distributed throughout the 
                  operating hours (8 AM to 9 PM). Each customer is characterized by unique attributes: geographical 
                  location (latitude/longitude), customer segment (budget, premium, or regular), willingness to pay, 
                  decision weights for rating, price, and novelty factors, and individual store valuations that influence 
                  their preference calculations.
                </p>
              </div>

              <div>
                <h4 className="font-semibold text-lg text-gray-900 mb-2">3. Restaurant Ranking and Display</h4>
                <p>
                  Upon customer arrival, the selected ranking algorithm evaluates all available restaurants (those with 
                  inventory exceeding current reservations) and selects the top 5 to display. The algorithm considers 
                  multiple factors including customer preferences, restaurant inventory levels, current ratings, pricing, 
                  geographical proximity, historical customer-restaurant interactions, and fairness metrics (impression 
                  counts for fairness-focused algorithms). The specific selection methodology varies by algorithm, with 
                  each implementing distinct optimization strategies.
                </p>
              </div>

              <div>
                <h4 className="font-semibold text-lg text-gray-900 mb-2">4. Customer Decision-Making Process</h4>
                <p>
                  Customers evaluate displayed restaurants using a scoring function that combines base store scores 
                  (rating, price, distance, novelty) with adjustment factors including historical interaction success 
                  rates, cancellation penalties, and inventory safety bonuses. The decision threshold is calculated as 
                  the base leaving threshold plus a loyalty adjustment factor. Customers employ probabilistic selection 
                  using a Softmax function with temperature parameter 2.0 to choose among valid options that meet their 
                  threshold. If no restaurant meets the minimum threshold, the customer leaves without making a reservation.
                </p>
              </div>

              <div>
                <h4 className="font-semibold text-lg text-gray-900 mb-2">5. Reservation Creation and Processing</h4>
                <p>
                  When a customer selects a restaurant, a reservation is created with PENDING status and added to the 
                  daily reservation queue. Reservations are processed at the end of each day in chronological order. 
                  If actual inventory exceeds the number of reservations, each customer receives a base allocation of 
                  min(MAX_BAGS_PER_CUSTOMER, floor(actual_bags / num_reservations)) bags, with remaining inventory 
                  distributed to early reservations up to the 3-bag maximum. When reservations exceed inventory, the first 
                  N customers (where N = actual_bags) receive one bag each, and remaining reservations are cancelled. 
                  An additional 10% of reservations are randomly cancelled to model real-world cancellation scenarios.
                </p>
              </div>

              <div>
                <h4 className="font-semibold text-lg text-gray-900 mb-2">6. End-of-Day Processing</h4>
                <p>
                  At the conclusion of each day, the system calculates waste as the difference between actual inventory 
                  and total bags distributed to customers. Restaurant ratings are updated dynamically: each confirmed 
                  order increases the rating, while cancellations decrease it. Customer history is updated to reflect 
                  successful transactions, cancellations, and store interactions. Restaurant inventory is replenished 
                  for the next day with a new variance factor applied. All daily reservations are cleared, and the 
                  simulation proceeds to the next day with updated state information.
                </p>
              </div>

              <div>
                <h4 className="font-semibold text-lg text-gray-900 mb-2">7. Results Aggregation and Comparison</h4>
                <p>
                  Following the completion of all 7 simulation days, comprehensive metrics are aggregated for each 
                  algorithm. The system calculates total bags sold, bags cancelled, bags wasted, revenue generated, 
                  revenue lost, conversion rates, and Gini coefficients for exposure fairness. These metrics enable 
                  comparative analysis of algorithm performance across multiple dimensions: waste reduction effectiveness, 
                  revenue optimization, customer retention, and equitable restaurant exposure distribution.
                </p>
              </div>
            </div>
          </div>
        )}
      </div>
    </div>
  )
}

