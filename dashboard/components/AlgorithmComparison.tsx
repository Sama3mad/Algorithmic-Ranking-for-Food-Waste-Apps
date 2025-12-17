'use client'

import { AlgorithmMetrics } from '@/types/simulation'
import { BarChart, Bar, XAxis, YAxis, CartesianGrid, Tooltip, Legend, ResponsiveContainer, LineChart, Line } from 'recharts'

interface AlgorithmComparisonProps {
  metrics: AlgorithmMetrics[]
}

export default function AlgorithmComparison({ metrics }: AlgorithmComparisonProps) {
  // Prepare data for charts
  const chartData = metrics.map(m => ({
    algorithm: m.algorithm,
    bagsSold: m.total_bags_sold,
    waste: m.total_bags_unsold,
    revenue: Math.round(m.total_revenue_generated),
    conversionRate: m.conversion_rate,
    revenueEfficiency: m.revenue_efficiency,
    giniCoefficient: m.gini_coefficient
  }))

  // Find best performers
  const bestBagsSold = metrics.reduce((best, current) => 
    current.total_bags_sold > best.total_bags_sold ? current : best
  )
  const bestWasteReduction = metrics.reduce((best, current) => 
    current.total_bags_unsold < best.total_bags_unsold ? current : best
  )
  const bestRevenue = metrics.reduce((best, current) => 
    current.total_revenue_generated > best.total_revenue_generated ? current : best
  )
  const bestConversion = metrics.reduce((best, current) => 
    current.conversion_rate > best.conversion_rate ? current : best
  )
  const bestFairness = metrics.reduce((best, current) => 
    current.gini_coefficient < best.gini_coefficient ? current : best
  )

  return (
    <div className="bg-white rounded-xl shadow-lg p-8">
      <h2 className="text-3xl font-bold text-gray-900 mb-8">Algorithm Comparison</h2>

      {/* Key Metrics Table */}
      <div className="mb-8">
        <h3 className="text-xl font-semibold text-gray-800 mb-4">Overall Metrics Comparison</h3>
        <div className="overflow-x-auto">
          <table className="min-w-full divide-y divide-gray-200">
            <thead className="bg-gradient-to-r from-purple-600 to-purple-700">
              <tr>
                <th className="px-6 py-3 text-left text-xs font-semibold text-white uppercase tracking-wider">
                  Algorithm
                </th>
                <th className="px-6 py-3 text-left text-xs font-semibold text-white uppercase tracking-wider">
                  Bags Sold
                </th>
                <th className="px-6 py-3 text-left text-xs font-semibold text-white uppercase tracking-wider">
                  Bags Cancelled
                </th>
                <th className="px-6 py-3 text-left text-xs font-semibold text-white uppercase tracking-wider">
                  Waste
                </th>
                <th className="px-6 py-3 text-left text-xs font-semibold text-white uppercase tracking-wider">
                  Revenue ($)
                </th>
                <th className="px-6 py-3 text-left text-xs font-semibold text-white uppercase tracking-wider">
                  Revenue Lost ($)
                </th>
                <th className="px-6 py-3 text-left text-xs font-semibold text-white uppercase tracking-wider">
                  Revenue Efficiency (%)
                </th>
                <th className="px-6 py-3 text-left text-xs font-semibold text-white uppercase tracking-wider">
                  Customers Left
                </th>
                <th className="px-6 py-3 text-left text-xs font-semibold text-white uppercase tracking-wider">
                  Conversion Rate (%)
                </th>
                <th className="px-6 py-3 text-left text-xs font-semibold text-white uppercase tracking-wider">
                  Gini Coefficient
                </th>
              </tr>
            </thead>
            <tbody className="bg-white divide-y divide-gray-200">
              {metrics.map((metric, idx) => (
                <tr key={metric.algorithm} className={idx % 2 === 0 ? 'bg-white' : 'bg-gray-50'}>
                  <td className="px-6 py-4 whitespace-nowrap text-sm font-semibold text-gray-900">
                    {metric.algorithm}
                  </td>
                  <td className={`px-6 py-4 whitespace-nowrap text-sm font-semibold ${
                    metric.algorithm === bestBagsSold.algorithm ? 'text-green-600' : 'text-gray-700'
                  }`}>
                    {metric.total_bags_sold}
                    {metric.algorithm === bestBagsSold.algorithm && ' 🏆'}
                  </td>
                  <td className="px-6 py-4 whitespace-nowrap text-sm text-gray-700">
                    {metric.total_bags_cancelled}
                  </td>
                  <td className={`px-6 py-4 whitespace-nowrap text-sm font-semibold ${
                    metric.algorithm === bestWasteReduction.algorithm ? 'text-green-600' : 'text-red-600'
                  }`}>
                    {metric.total_bags_unsold}
                    {metric.algorithm === bestWasteReduction.algorithm && ' 🏆'}
                  </td>
                  <td className={`px-6 py-4 whitespace-nowrap text-sm font-semibold ${
                    metric.algorithm === bestRevenue.algorithm ? 'text-green-600' : 'text-blue-600'
                  }`}>
                    ${metric.total_revenue_generated.toFixed(2)}
                    {metric.algorithm === bestRevenue.algorithm && ' 🏆'}
                  </td>
                  <td className="px-6 py-4 whitespace-nowrap text-sm text-orange-600">
                    ${metric.total_revenue_lost.toFixed(2)}
                  </td>
                  <td className="px-6 py-4 whitespace-nowrap text-sm text-gray-700">
                    {metric.revenue_efficiency.toFixed(2)}%
                  </td>
                  <td className="px-6 py-4 whitespace-nowrap text-sm text-gray-700">
                    {metric.customers_who_left}
                  </td>
                  <td className={`px-6 py-4 whitespace-nowrap text-sm font-semibold ${
                    metric.algorithm === bestConversion.algorithm ? 'text-green-600' : 'text-gray-700'
                  }`}>
                    {metric.conversion_rate.toFixed(2)}%
                    {metric.algorithm === bestConversion.algorithm && ' 🏆'}
                  </td>
                  <td className={`px-6 py-4 whitespace-nowrap text-sm font-semibold ${
                    metric.algorithm === bestFairness.algorithm ? 'text-green-600' : 'text-gray-700'
                  }`}>
                    {metric.gini_coefficient.toFixed(4)}
                    {metric.algorithm === bestFairness.algorithm && ' 🏆'}
                  </td>
                </tr>
              ))}
            </tbody>
          </table>
        </div>
      </div>

      {/* Charts */}
      <div className="grid grid-cols-1 lg:grid-cols-2 gap-8 mb-8">
        {/* Bags Sold vs Waste */}
        <div className="bg-gray-50 rounded-lg p-6">
          <h3 className="text-lg font-semibold text-gray-800 mb-4">Bags Sold vs Waste</h3>
          <ResponsiveContainer width="100%" height={300}>
            <BarChart data={chartData}>
              <CartesianGrid strokeDasharray="3 3" />
              <XAxis dataKey="algorithm" />
              <YAxis />
              <Tooltip />
              <Legend />
              <Bar dataKey="bagsSold" fill="#10b981" name="Bags Sold" />
              <Bar dataKey="waste" fill="#ef4444" name="Waste" />
            </BarChart>
          </ResponsiveContainer>
        </div>

        {/* Revenue Comparison */}
        <div className="bg-gray-50 rounded-lg p-6">
          <h3 className="text-lg font-semibold text-gray-800 mb-4">Revenue Generated</h3>
          <ResponsiveContainer width="100%" height={300}>
            <BarChart data={chartData}>
              <CartesianGrid strokeDasharray="3 3" />
              <XAxis dataKey="algorithm" />
              <YAxis />
              <Tooltip formatter={(value: number) => `$${value.toFixed(2)}`} />
              <Legend />
              <Bar dataKey="revenue" fill="#3b82f6" name="Revenue ($)" />
            </BarChart>
          </ResponsiveContainer>
        </div>

        {/* Conversion Rate */}
        <div className="bg-gray-50 rounded-lg p-6">
          <h3 className="text-lg font-semibold text-gray-800 mb-4">Conversion Rate (%)</h3>
          <ResponsiveContainer width="100%" height={300}>
            <LineChart data={chartData}>
              <CartesianGrid strokeDasharray="3 3" />
              <XAxis dataKey="algorithm" />
              <YAxis domain={[0, 100]} />
              <Tooltip formatter={(value: number) => `${value.toFixed(2)}%`} />
              <Legend />
              <Line type="monotone" dataKey="conversionRate" stroke="#8b5cf6" strokeWidth={3} name="Conversion Rate (%)" />
            </LineChart>
          </ResponsiveContainer>
        </div>

        {/* Fairness (Gini Coefficient) */}
        <div className="bg-gray-50 rounded-lg p-6">
          <h3 className="text-lg font-semibold text-gray-800 mb-4">Fairness (Gini Coefficient)</h3>
          <p className="text-xs text-gray-600 mb-2">Lower is better (0 = perfect equality)</p>
          <ResponsiveContainer width="100%" height={300}>
            <BarChart data={chartData}>
              <CartesianGrid strokeDasharray="3 3" />
              <XAxis dataKey="algorithm" />
              <YAxis domain={[0, 1]} />
              <Tooltip formatter={(value: number) => value.toFixed(4)} />
              <Legend />
              <Bar dataKey="giniCoefficient" fill="#f59e0b" name="Gini Coefficient" />
            </BarChart>
          </ResponsiveContainer>
        </div>
      </div>

      {/* Performance Summary */}
      <div className="bg-gradient-to-r from-blue-50 to-purple-50 rounded-lg p-6">
        <h3 className="text-xl font-semibold text-gray-800 mb-4">Performance Summary</h3>
        <div className="grid grid-cols-1 md:grid-cols-2 lg:grid-cols-3 gap-4">
          <div className="bg-white rounded-lg p-4 shadow-sm">
            <div className="text-sm text-gray-600 mb-1">Best Sales Performance</div>
            <div className="text-lg font-bold text-green-600">{bestBagsSold.algorithm}</div>
            <div className="text-sm text-gray-700">{bestBagsSold.total_bags_sold} bags sold</div>
          </div>
          <div className="bg-white rounded-lg p-4 shadow-sm">
            <div className="text-sm text-gray-600 mb-1">Best Waste Reduction</div>
            <div className="text-lg font-bold text-green-600">{bestWasteReduction.algorithm}</div>
            <div className="text-sm text-gray-700">{bestWasteReduction.total_bags_unsold} bags wasted</div>
          </div>
          <div className="bg-white rounded-lg p-4 shadow-sm">
            <div className="text-sm text-gray-600 mb-1">Highest Revenue</div>
            <div className="text-lg font-bold text-blue-600">{bestRevenue.algorithm}</div>
            <div className="text-sm text-gray-700">${bestRevenue.total_revenue_generated.toFixed(2)}</div>
          </div>
          <div className="bg-white rounded-lg p-4 shadow-sm">
            <div className="text-sm text-gray-600 mb-1">Best Conversion Rate</div>
            <div className="text-lg font-bold text-purple-600">{bestConversion.algorithm}</div>
            <div className="text-sm text-gray-700">{bestConversion.conversion_rate.toFixed(2)}%</div>
          </div>
          <div className="bg-white rounded-lg p-4 shadow-sm">
            <div className="text-sm text-gray-600 mb-1">Most Fair (Lowest Gini)</div>
            <div className="text-lg font-bold text-orange-600">{bestFairness.algorithm}</div>
            <div className="text-sm text-gray-700">{bestFairness.gini_coefficient.toFixed(4)}</div>
          </div>
        </div>
      </div>
    </div>
  )
}

