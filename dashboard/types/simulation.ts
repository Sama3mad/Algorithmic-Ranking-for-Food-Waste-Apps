export interface Restaurant {
  business_id: number
  business_name: string
  branch: string
  estimated_bags: number
  general_ranking: number
  price_per_bag: number
  longitude: number
  latitude: number
  business_type: string
  actual_bags: number
  reserved_count: number
  has_inventory: boolean
}

export interface Customer {
  id: number
  longitude: number
  latitude: number
  customer_name: string
  segment: string
  willingness_to_pay: number
  weights: {
    rating_w: number
    price_w: number
    novelty_w: number
  }
  loyalty: number
  leaving_threshold: number
  store_valuations: Record<number, number>
  category_preference: Record<string, number>
}

export interface RestaurantResult {
  Restaurant: string
  Estimated: number
  Actual: number
  Reserved: number
  Sold: number
  Cancelled: number
  Waste: number
  Revenue: number
  Exposures: number
}

export interface SimulationResult {
  Algorithm: string
  results: RestaurantResult[]
}

export interface AlgorithmMetrics {
  algorithm: string
  total_bags_sold: number
  total_bags_cancelled: number
  total_bags_unsold: number
  total_revenue_generated: number
  total_revenue_lost: number
  revenue_efficiency: number
  customers_who_left: number
  conversion_rate: number
  gini_coefficient: number
  total_customer_arrivals: number
}

export type RankingAlgorithm = 
  | 'BASELINE'
  | 'SAMA'
  | 'ANDREW'
  | 'AMER'
  | 'ZIAD'
  | 'HARMONY'

