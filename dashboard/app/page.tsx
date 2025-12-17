'use client'

import { useState } from 'react'
import InfoSection from '@/components/InfoSection'
import FileUpload from '@/components/FileUpload'
import SimulationResults from '@/components/SimulationResults'
import AlgorithmComparison from '@/components/AlgorithmComparison'
import { SimulationResult, AlgorithmMetrics } from '@/types/simulation'

export default function Home() {
  const [results, setResults] = useState<SimulationResult[] | null>(null)
  const [comparison, setComparison] = useState<AlgorithmMetrics[] | null>(null)
  const [isRunning, setIsRunning] = useState(false)

  const handleSimulationComplete = (simResults: SimulationResult[], compResults: AlgorithmMetrics[]) => {
    setResults(simResults)
    setComparison(compResults)
    setIsRunning(false)
  }

  const handleSimulationStart = () => {
    setIsRunning(true)
    setResults(null)
    setComparison(null)
  }

  return (
    <main className="min-h-screen bg-gradient-to-br from-blue-50 via-white to-green-50">
      <div className="container mx-auto px-4 py-8 max-w-7xl">
        {/* Header */}
        <header className="text-center mb-12">
          <h1 className="text-5xl font-bold text-gray-900 mb-4">
            Food Waste Marketplace
          </h1>
          <p className="text-xl text-gray-600">
            Simulation Dashboard & Algorithm Comparison
          </p>
          <div className="mt-4 w-24 h-1 bg-gradient-to-r from-blue-500 to-green-500 mx-auto rounded"></div>
        </header>

        {/* Info Section */}
        <InfoSection />

        {/* File Upload Section */}
        <div className="mb-12">
          <FileUpload 
            onSimulationStart={handleSimulationStart}
            onSimulationComplete={handleSimulationComplete}
            isRunning={isRunning}
          />
        </div>

        {/* Results Section */}
        {results && results.length > 0 && (
          <div className="mb-12">
            <SimulationResults results={results} />
          </div>
        )}

        {/* Comparison Section */}
        {comparison && comparison.length > 0 && (
          <div className="mb-12">
            <AlgorithmComparison metrics={comparison} />
          </div>
        )}

        {/* Footer */}
        <footer className="mt-16 text-center text-gray-500 text-sm">
          <p>Food Waste Marketplace Simulation Dashboard © 2025</p>
        </footer>
      </div>
    </main>
  )
}

