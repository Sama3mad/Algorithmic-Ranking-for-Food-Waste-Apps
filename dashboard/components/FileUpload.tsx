'use client'

import { useState, useRef } from 'react'
import Papa from 'papaparse'
import { runFullSimulation } from '@/lib/simulationEngine'
import { SimulationResult, AlgorithmMetrics } from '@/types/simulation'

interface FileUploadProps {
  onSimulationStart: () => void
  onSimulationComplete: (results: SimulationResult[], comparison: AlgorithmMetrics[]) => void
  isRunning: boolean
}

export default function FileUpload({ onSimulationStart, onSimulationComplete, isRunning }: FileUploadProps) {
  const [customerFile, setCustomerFile] = useState<File | null>(null)
  const [storesFile, setStoresFile] = useState<File | null>(null)
  const [error, setError] = useState<string | null>(null)
  const customerInputRef = useRef<HTMLInputElement>(null)
  const storesInputRef = useRef<HTMLInputElement>(null)

  const handleCustomerFileChange = (e: React.ChangeEvent<HTMLInputElement>) => {
    const file = e.target.files?.[0]
    if (file) {
      if (file.type !== 'text/csv' && !file.name.endsWith('.csv')) {
        setError('Please upload a CSV file for customers')
        return
      }
      setCustomerFile(file)
      setError(null)
    }
  }

  const handleStoresFileChange = (e: React.ChangeEvent<HTMLInputElement>) => {
    const file = e.target.files?.[0]
    if (file) {
      if (file.type !== 'text/csv' && !file.name.endsWith('.csv')) {
        setError('Please upload a CSV file for stores')
        return
      }
      setStoresFile(file)
      setError(null)
    }
  }

  const parseCSV = (file: File): Promise<any[]> => {
    return new Promise((resolve, reject) => {
      Papa.parse(file, {
        header: true,
        skipEmptyLines: true,
        complete: (results) => {
          resolve(results.data as any[])
        },
        error: (error) => {
          reject(error)
        }
      })
    })
  }

  const handleRunSimulation = async () => {
    if (!customerFile || !storesFile) {
      setError('Please upload both customer.csv and stores.csv files')
      return
    }

    setError(null)
    onSimulationStart()

    try {
      const [customerData, storesData] = await Promise.all([
        parseCSV(customerFile),
        parseCSV(storesFile)
      ])

      if (customerData.length === 0 || storesData.length === 0) {
        throw new Error('CSV files appear to be empty')
      }

      // Run simulation (this might take a moment)
      const { results, comparison } = await runFullSimulation(customerData, storesData)
      
      onSimulationComplete(results, comparison)
    } catch (err) {
      setError(err instanceof Error ? err.message : 'An error occurred during simulation')
      console.error('Simulation error:', err)
    }
  }

  const handleClearFiles = () => {
    setCustomerFile(null)
    setStoresFile(null)
    setError(null)
    if (customerInputRef.current) customerInputRef.current.value = ''
    if (storesInputRef.current) storesInputRef.current.value = ''
  }

  return (
    <div className="bg-white rounded-xl shadow-lg p-8">
      <h2 className="text-3xl font-bold text-gray-900 mb-6">Run Simulation</h2>
      
      <div className="space-y-6">
        {/* Customer File Upload */}
        <div>
          <label className="block text-sm font-medium text-gray-700 mb-2">
            Customer Data (customer.csv)
          </label>
          <div className="flex items-center space-x-4">
            <input
              ref={customerInputRef}
              type="file"
              accept=".csv"
              onChange={handleCustomerFileChange}
              disabled={isRunning}
              className="block w-full text-sm text-gray-500 file:mr-4 file:py-2 file:px-4 file:rounded-lg file:border-0 file:text-sm file:font-semibold file:bg-blue-50 file:text-blue-700 hover:file:bg-blue-100 disabled:opacity-50 disabled:cursor-not-allowed"
            />
            {customerFile && (
              <span className="text-sm text-green-600 font-medium">
                ✓ {customerFile.name}
              </span>
            )}
          </div>
        </div>

        {/* Stores File Upload */}
        <div>
          <label className="block text-sm font-medium text-gray-700 mb-2">
            Restaurant Data (stores.csv)
          </label>
          <div className="flex items-center space-x-4">
            <input
              ref={storesInputRef}
              type="file"
              accept=".csv"
              onChange={handleStoresFileChange}
              disabled={isRunning}
              className="block w-full text-sm text-gray-500 file:mr-4 file:py-2 file:px-4 file:rounded-lg file:border-0 file:text-sm file:font-semibold file:bg-blue-50 file:text-blue-700 hover:file:bg-blue-100 disabled:opacity-50 disabled:cursor-not-allowed"
            />
            {storesFile && (
              <span className="text-sm text-green-600 font-medium">
                ✓ {storesFile.name}
              </span>
            )}
          </div>
        </div>

        {/* Error Message */}
        {error && (
          <div className="bg-red-50 border border-red-200 rounded-lg p-4">
            <p className="text-sm text-red-800">{error}</p>
          </div>
        )}

        {/* Action Buttons */}
        <div className="flex space-x-4 pt-4">
          <button
            onClick={handleRunSimulation}
            disabled={isRunning || !customerFile || !storesFile}
            className="flex-1 bg-gradient-to-r from-blue-600 to-blue-700 text-white font-semibold py-3 px-6 rounded-lg hover:from-blue-700 hover:to-blue-800 disabled:opacity-50 disabled:cursor-not-allowed transition-all duration-200 shadow-lg hover:shadow-xl transform hover:-translate-y-0.5"
          >
            {isRunning ? (
              <span className="flex items-center justify-center">
                <svg className="animate-spin -ml-1 mr-3 h-5 w-5 text-white" xmlns="http://www.w3.org/2000/svg" fill="none" viewBox="0 0 24 24">
                  <circle className="opacity-25" cx="12" cy="12" r="10" stroke="currentColor" strokeWidth="4"></circle>
                  <path className="opacity-75" fill="currentColor" d="M4 12a8 8 0 018-8V0C5.373 0 0 5.373 0 12h4zm2 5.291A7.962 7.962 0 014 12H0c0 3.042 1.135 5.824 3 7.938l3-2.647z"></path>
                </svg>
                Running Simulation...
              </span>
            ) : (
              'Run Simulation'
            )}
          </button>
          
          <button
            onClick={handleClearFiles}
            disabled={isRunning}
            className="px-6 py-3 border border-gray-300 text-gray-700 font-semibold rounded-lg hover:bg-gray-50 disabled:opacity-50 disabled:cursor-not-allowed transition-colors"
          >
            Clear Files
          </button>
        </div>

        {/* Info Text */}
        <div className="bg-blue-50 rounded-lg p-4 mt-4">
          <p className="text-sm text-blue-800">
            <strong>Note:</strong> The simulation will run all 6 ranking algorithms (BASELINE, SAMA, ANDREW, AMER, ZIAD, HARMONY) 
            and compare their performance. This may take a few moments to complete.
          </p>
        </div>
      </div>
    </div>
  )
}

