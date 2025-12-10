#include "ArrivalGenerator.h"
#include <fstream>
#include <sstream>
#include <algorithm>
#include <ctime>
#include <cctype>
#include <iostream>

ArrivalGenerator::ArrivalGenerator(unsigned seed) : rng(seed) {}

ArrivalGenerator::ArrivalGenerator(const std::string& csv_path, unsigned seed)
    : rng(seed) {
    load_customers_from_csv(csv_path);
}

std::vector<Timestamp> ArrivalGenerator::generate_arrival_times(int num_customers) {
    std::vector<Timestamp> times;
    std::uniform_int_distribution<int> hour_dist(8, 21);
    std::uniform_int_distribution<int> min_dist(0, 59);

    for (int i = 0; i < num_customers; i++) {
        times.push_back(Timestamp(hour_dist(rng), min_dist(rng)));
    }

    std::sort(times.begin(), times.end());
    return times;
}

bool ArrivalGenerator::load_customers_from_csv(const std::string& filename) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        std::cerr << "Warning: Could not open customer CSV file: " << filename << std::endl;
        std::cerr << "Will generate customers instead." << std::endl;
        return false;
    }

    std::string line;
    std::vector<std::string> header_columns;
    std::vector<int> store_id_columns;

    if (std::getline(file, line)) {
        std::stringstream header_ss(line);
        std::string col;
        int col_index = 0;
        while (std::getline(header_ss, col, ',')) {
            header_columns.push_back(col);
            
            std::string col_lower = col;
            std::transform(col_lower.begin(), col_lower.end(), col_lower.begin(), ::tolower);
            
            if (col_lower.find("store") != std::string::npos && 
                (col_lower.find("valuation") != std::string::npos || col_lower.find("_id_") != std::string::npos)) {
                size_t store_pos = col_lower.find("store");
                if (store_pos != std::string::npos) {
                    size_t num_start = store_pos + 5;
                    while (num_start < col.length() && !std::isdigit(col_lower[num_start])) {
                        num_start++;
                    }
                    if (num_start < col_lower.length()) {
                        try {
                            size_t num_end = num_start;
                            while (num_end < col_lower.length() && std::isdigit(col_lower[num_end])) {
                                num_end++;
                            }
                            int store_id = std::stoi(col_lower.substr(num_start, num_end - num_start));
                            store_id_columns.push_back(col_index);
                        } catch (...) {
                        }
                    }
                }
            }
            col_index++;
        }
    }

    while (std::getline(file, line)) {
        if (line.empty()) continue;
        std::stringstream ss(line);
        std::vector<std::string> values;
        std::string val;
        
        while (std::getline(ss, val, ',')) {
            values.push_back(val);
        }
        
        if (values.size() < 2) continue;
        
        float lat = 0.0f, lon = 0.0f;
        int col_idx = 0;
        
        if (col_idx < values.size() && !values[col_idx].empty()) {
            try {
                lat = std::stof(values[col_idx]);
            } catch (...) {
                lat = 0.0f;
            }
        }
        col_idx++;
        
        if (col_idx < values.size() && !values[col_idx].empty()) {
            try {
                lon = std::stof(values[col_idx]);
            } catch (...) {
                lon = 0.0f;
            }
        }
        col_idx++;
        
        std::map<int, float> store_vals;
        for (size_t idx = 0; idx < store_id_columns.size(); idx++) {
            int store_col_idx = store_id_columns[idx];
            if (store_col_idx < (int)values.size() && !values[store_col_idx].empty()) {
                try {
                    float valuation = std::stof(values[store_col_idx]);
                    std::string col_name = header_columns[store_col_idx];
                    std::string col_lower = col_name;
                    std::transform(col_lower.begin(), col_lower.end(), col_lower.begin(), ::tolower);
                    
                    size_t store_pos = col_lower.find("store");
                    int store_id = -1;
                    if (store_pos != std::string::npos) {
                        size_t num_start = store_pos + 5;
                        while (num_start < col_lower.length() && !std::isdigit(col_lower[num_start])) {
                            num_start++;
                        }
                        if (num_start < col_lower.length()) {
                            size_t num_end = num_start;
                            while (num_end < col_lower.length() && std::isdigit(col_lower[num_end])) {
                                num_end++;
                            }
                            store_id = std::stoi(col_lower.substr(num_start, num_end - num_start));
                        }
                    }
                    
                    if (store_id == -1) {
                        store_id = store_col_idx - 2 + 1;
                    }
                    
                    store_vals[store_id] = valuation;
                } catch (...) {
                }
            }
        }
        
        int id = customers_from_csv.size();
        std::string name = "Customer_" + std::to_string(id);
        std::string segment = "regular";
        float wtp = 150.0f;
        float rating_w = 1.0f;
        float price_w = 1.0f;
        float novelty_w = 0.5f;
        float leaving_thresh = 3.0f;
        
        int code_format_start = 2 + store_id_columns.size();
        
        if (code_format_start < (int)values.size()) {
            if (code_format_start < (int)values.size() && !values[code_format_start].empty()) {
                try {
                    id = std::stoi(values[code_format_start]);
                } catch (...) {
                    id = customers_from_csv.size();
                }
            }
            code_format_start++;
            
            if (code_format_start < (int)values.size()) code_format_start++;
            if (code_format_start < (int)values.size()) code_format_start++;
            
            if (code_format_start < (int)values.size() && !values[code_format_start].empty()) {
                name = values[code_format_start];
            }
            code_format_start++;
            
            if (code_format_start < (int)values.size() && !values[code_format_start].empty()) {
                segment = values[code_format_start];
            }
            code_format_start++;
            
            if (code_format_start < (int)values.size() && !values[code_format_start].empty()) {
                try {
                    wtp = std::stof(values[code_format_start]);
                } catch (...) {
                    wtp = 150.0f;
                }
            }
            code_format_start++;
            
            if (code_format_start < (int)values.size() && !values[code_format_start].empty()) {
                try {
                    rating_w = std::stof(values[code_format_start]);
                } catch (...) {
                    rating_w = 1.0f;
                }
            }
            code_format_start++;
            
            if (code_format_start < (int)values.size() && !values[code_format_start].empty()) {
                try {
                    price_w = std::stof(values[code_format_start]);
                } catch (...) {
                    price_w = 1.0f;
                }
            }
            code_format_start++;
            
            if (code_format_start < (int)values.size() && !values[code_format_start].empty()) {
                try {
                    novelty_w = std::stof(values[code_format_start]);
                } catch (...) {
                    novelty_w = 0.5f;
                }
            }
            code_format_start++;
            
            if (code_format_start < (int)values.size() && !values[code_format_start].empty()) {
                try {
                    leaving_thresh = std::stof(values[code_format_start]);
                } catch (...) {
                    leaving_thresh = 3.0f;
                }
            }
        }
        
        Customer c(id, lon, lat, name, segment, wtp, rating_w, price_w, novelty_w, leaving_thresh);
        c.store_valuations = store_vals;
        customers_from_csv.push_back(c);
    }

    std::cout << "Loaded " << customers_from_csv.size()
              << " customers from " << filename << std::endl;
    if (!store_id_columns.empty()) {
        std::cout << "  Found " << store_id_columns.size() << " store valuation columns" << std::endl;
    }

    file.close();
    return true;
}

Customer ArrivalGenerator::generate_customer(int index, const std::vector<Restaurant>& restaurants) {
    if (!customers_from_csv.empty()) {
        int csv_index = index % customers_from_csv.size();
        Customer c = customers_from_csv[csv_index];
        c.id = index;
        return c;
    }
    
    std::uniform_int_distribution<int> segment_dist(0, 2);
    int seg = segment_dist(rng);

    std::string segment;
    float wtp;
    float rating_w, price_w, novelty_w;
    float leaving_thresh;

    if (seg == 0) {
        segment = "budget";
        wtp = 80.0f + (rng() % 40);
        rating_w = 0.5f + (rng() % 100) / 200.0f;
        price_w = 1.5f + (rng() % 100) / 200.0f;
        novelty_w = 0.3f + (rng() % 100) / 200.0f;
        leaving_thresh = 2.0f + (rng() % 30) / 10.0f;
    }
    else if (seg == 1) {
        segment = "regular";
        wtp = 120.0f + (rng() % 60);
        rating_w = 1.0f + (rng() % 100) / 200.0f;
        price_w = 1.0f + (rng() % 100) / 200.0f;
        novelty_w = 0.5f + (rng() % 100) / 200.0f;
        leaving_thresh = 3.0f + (rng() % 40) / 10.0f;
    }
    else {
        segment = "premium";
        wtp = 180.0f + (rng() % 80);
        rating_w = 1.5f + (rng() % 100) / 200.0f;
        price_w = 0.5f + (rng() % 100) / 200.0f;
        novelty_w = 0.8f + (rng() % 100) / 200.0f;
        leaving_thresh = 4.0f + (rng() % 40) / 10.0f;
    }

    std::uniform_real_distribution<float> lon_dist(31.2f, 31.3f);
    std::uniform_real_distribution<float> lat_dist(30.0f, 30.1f);
    
    Customer customer(index, lon_dist(rng), lat_dist(rng), 
                     "Customer_" + std::to_string(index), segment, 
                     wtp, rating_w, price_w, novelty_w, leaving_thresh);
    
    if (!restaurants.empty()) {
        std::uniform_real_distribution<float> valuation_dist(0.0f, 5.0f);
        for (const auto& restaurant : restaurants) {
            float valuation = valuation_dist(rng);
            customer.store_valuations[restaurant.business_id] = valuation;
        }
    }
    
    return customer;
}

