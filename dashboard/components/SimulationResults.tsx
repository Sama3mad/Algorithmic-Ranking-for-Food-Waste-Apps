'use client'

import { useState } from 'react'
import { SimulationResult } from '@/types/simulation'

interface SimulationResultsProps {
  results: SimulationResult[]
}

export default function SimulationResults({ results }: SimulationResultsProps) {
  const [selectedAlgorithm, setSelectedAlgorithm] = useState<string>(results[0]?.Algorithm || '')

  const selectedResult = results.find(r => r.Algorithm === selectedAlgorithm) || results[0]

  if (!selectedResult) return null

  return (
    <div className="bg-white rounded-xl shadow-lg p-8">
      <div className="flex items-center justify-between mb-6">
        <h2 className="text-3xl font-bold text-gray-900">Simulation Results</h2>
        
        {/* Algorithm Selector */}
        <div className="flex items-center space-x-4">
          <label className="text-sm font-medium text-gray-700">Algorithm:</label>
          <select
            value={selectedAlgorithm}
            onChange={(e) => setSelectedAlgorithm(e.target.value)}
            className="border border-gray-300 rounded-lg px-4 py-2 text-sm font-medium text-gray-700 bg-white hover:border-gray-400 focus:outline-none focus:ring-2 focus:ring-blue-500 focus:border-transparent"
          >
            {results.map((result) => (
              <option key={result.Algorithm} value={result.Algorithm}>
                {result.Algorithm}
              </option>
            ))}
          </select>
        </div>
      </div>

      {/* Results Table */}
      <div className="overflow-x-auto">
        <table className="min-w-full divide-y divide-gray-200">
          <thead className="bg-gradient-to-r from-blue-600 to-blue-700">
            <tr>
              <th className="px-6 py-3 text-left text-xs font-semibold text-white uppercase tracking-wider">
                Restaurant
              </th>
              <th className="px-6 py-3 text-left text-xs font-semibold text-white uppercase tracking-wider">
                Estimated
              </th>
              <th className="px-6 py-3 text-left text-xs font-semibold text-white uppercase tracking-wider">
                Actual
              </th>
              <th className="px-6 py-3 text-left text-xs font-semibold text-white uppercase tracking-wider">
                Reserved
              </th>
              <th className="px-6 py-3 text-left text-xs font-semibold text-white uppercase tracking-wider">
                Sold
              </th>
              <th className="px-6 py-3 text-left text-xs font-semibold text-white uppercase tracking-wider">
                Cancelled
              </th>
              <th className="px-6 py-3 text-left text-xs font-semibold text-white uppercase tracking-wider">
                Waste
              </th>
              <th className="px-6 py-3 text-left text-xs font-semibold text-white uppercase tracking-wider">
                Revenue
              </th>
              <th className="px-6 py-3 text-left text-xs font-semibold text-white uppercase tracking-wider">
                Exposures
              </th>
            </tr>
          </thead>
          <tbody className="bg-white divide-y divide-gray-200">
            {selectedResult.results.map((row, idx) => (
              <tr key={idx} className={idx % 2 === 0 ? 'bg-white' : 'bg-gray-50'}>
                <td className="px-6 py-4 whitespace-nowrap text-sm font-medium text-gray-900">
                  {row.Restaurant}
                </td>
                <td className="px-6 py-4 whitespace-nowrap text-sm text-gray-700">
                  {row.Estimated}
                </td>
                <td className="px-6 py-4 whitespace-nowrap text-sm text-gray-700">
                  {row.Actual}
                </td>
                <td className="px-6 py-4 whitespace-nowrap text-sm text-gray-700">
                  {row.Reserved}
                </td>
                <td className="px-6 py-4 whitespace-nowrap text-sm font-semibold text-green-600">
                  {row.Sold}
                </td>
                <td className="px-6 py-4 whitespace-nowrap text-sm text-orange-600">
                  {row.Cancelled}
                </td>
                <td className="px-6 py-4 whitespace-nowrap text-sm font-semibold text-red-600">
                  {row.Waste}
                </td>
                <td className="px-6 py-4 whitespace-nowrap text-sm font-semibold text-blue-600">
                  ${row.Revenue.toFixed(2)}
                </td>
                <td className="px-6 py-4 whitespace-nowrap text-sm text-gray-700">
                  {row.Exposures}
                </td>
              </tr>
            ))}
          </tbody>
          <tfoot className="bg-gray-100">
            <tr>
              <td className="px-6 py-4 whitespace-nowrap text-sm font-bold text-gray-900">
                Totals
              </td>
              <td className="px-6 py-4 whitespace-nowrap text-sm font-semibold text-gray-700">
                {selectedResult.results.reduce((sum, r) => sum + r.Estimated, 0)}
              </td>
              <td className="px-6 py-4 whitespace-nowrap text-sm font-semibold text-gray-700">
                {selectedResult.results.reduce((sum, r) => sum + r.Actual, 0)}
              </td>
              <td className="px-6 py-4 whitespace-nowrap text-sm font-semibold text-gray-700">
                {selectedResult.results.reduce((sum, r) => sum + r.Reserved, 0)}
              </td>
              <td className="px-6 py-4 whitespace-nowrap text-sm font-bold text-green-600">
                {selectedResult.results.reduce((sum, r) => sum + r.Sold, 0)}
              </td>
              <td className="px-6 py-4 whitespace-nowrap text-sm font-semibold text-orange-600">
                {selectedResult.results.reduce((sum, r) => sum + r.Cancelled, 0)}
              </td>
              <td className="px-6 py-4 whitespace-nowrap text-sm font-bold text-red-600">
                {selectedResult.results.reduce((sum, r) => sum + r.Waste, 0)}
              </td>
              <td className="px-6 py-4 whitespace-nowrap text-sm font-bold text-blue-600">
                ${selectedResult.results.reduce((sum, r) => sum + r.Revenue, 0).toFixed(2)}
              </td>
              <td className="px-6 py-4 whitespace-nowrap text-sm font-semibold text-gray-700">
                {selectedResult.results.reduce((sum, r) => sum + r.Exposures, 0)}
              </td>
            </tr>
          </tfoot>
        </table>
      </div>

      {/* Summary Cards */}
      <div className="grid grid-cols-1 md:grid-cols-4 gap-4 mt-8">
        <div className="bg-green-50 rounded-lg p-4 border border-green-200">
          <div className="text-sm font-medium text-green-700 mb-1">Total Sold</div>
          <div className="text-2xl font-bold text-green-900">
            {selectedResult.results.reduce((sum, r) => sum + r.Sold, 0)}
          </div>
        </div>
        <div className="bg-red-50 rounded-lg p-4 border border-red-200">
          <div className="text-sm font-medium text-red-700 mb-1">Total Waste</div>
          <div className="text-2xl font-bold text-red-900">
            {selectedResult.results.reduce((sum, r) => sum + r.Waste, 0)}
          </div>
        </div>
        <div className="bg-blue-50 rounded-lg p-4 border border-blue-200">
          <div className="text-sm font-medium text-blue-700 mb-1">Total Revenue</div>
          <div className="text-2xl font-bold text-blue-900">
            ${selectedResult.results.reduce((sum, r) => sum + r.Revenue, 0).toFixed(2)}
          </div>
        </div>
        <div className="bg-purple-50 rounded-lg p-4 border border-purple-200">
          <div className="text-sm font-medium text-purple-700 mb-1">Total Exposures</div>
          <div className="text-2xl font-bold text-purple-900">
            {selectedResult.results.reduce((sum, r) => sum + r.Exposures, 0)}
          </div>
        </div>
      </div>
    </div>
  )
}

