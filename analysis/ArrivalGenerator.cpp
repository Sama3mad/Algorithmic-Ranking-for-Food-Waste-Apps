#include "ArrivalGenerator.h"
#include <fstream>
#include <sstream>
#include <algorithm>
#include <ctime>
#include <cctype>
#include <iostream>

using namespace std;

// Initialize generator with a random seed
ArrivalGenerator::ArrivalGenerator(unsigned seed) : rng(seed) {}

// Initialize with a CSV file and seed
ArrivalGenerator::ArrivalGenerator(const string& csv_path, unsigned seed)
    : rng(seed) {
    load_customers_from_csv(csv_path);
}

// Generate random arrival times for customers
vector<Timestamp> ArrivalGenerator::generate_arrival_times(int num_customers) {
    vector<Timestamp> times;
    uniform_int_distribution<int> hour_dist(8, 21); // 8 AM to 9 PM
    uniform_int_distribution<int> min_dist(0, 59);

    for (int i = 0; i < num_customers; i++) {
        times.push_back(Timestamp(hour_dist(rng), min_dist(rng)));
    }

    // Sort times so customers arrive in order
    sort(times.begin(), times.end());
    return times;
}

// Load customer data from a CSV file
bool ArrivalGenerator::load_customers_from_csv(const string& filename) {
    if (filename.empty()) {
        return false;
    }
    ifstream file(filename);
    if (!file.is_open()) {
        cerr << "Warning: Could not open customer CSV file: " << filename << endl;
        cerr << "Will generate customers instead." << endl;
        return false;
    }

    string line;
    vector<string> header_columns;
    
    // Column indices for ALL possible columns
    int customer_id_idx = -1, longitude_idx = -1, latitude_idx = -1;
    int customer_name_idx = -1, segment_idx = -1, wtp_idx = -1;
    int rating_w_idx = -1, price_w_idx = -1, novelty_w_idx = -1;
    int loyalty_idx = -1, leaving_threshold_idx = -1;
    vector<int> store_valuation_columns;
    vector<int> store_ids;  // Corresponding store IDs

    // Read header and map columns
    if (getline(file, line)) {
        stringstream header_ss(line);
        string col;
        int col_index = 0;
        
        while (getline(header_ss, col, ',')) {
            // Trim whitespace
            col.erase(0, col.find_first_not_of(" \t\r\n"));
            col.erase(col.find_last_not_of(" \t\r\n") + 1);
            header_columns.push_back(col);
            
            // Convert to lowercase for case-insensitive comparison
            string col_lower = col;
            transform(col_lower.begin(), col_lower.end(), col_lower.begin(), ::tolower);
            
            // Map mandatory columns
            if (col_lower == "customerid" || col_lower == "customer_id") {
                customer_id_idx = col_index;
            } else if (col_lower == "longitude" || col_lower == "lon") {
                longitude_idx = col_index;
            } else if (col_lower == "latitude" || col_lower == "lat") {
                latitude_idx = col_index;
            }
            // Map optional columns
            else if (col_lower == "customer_name" || col_lower == "name") {
                customer_name_idx = col_index;
            } else if (col_lower == "segment") {
                segment_idx = col_index;
            } else if (col_lower == "willingness_to_pay" || col_lower == "wtp") {
                wtp_idx = col_index;
            } else if (col_lower == "rating_weight" || col_lower == "rating_w") {
                rating_w_idx = col_index;
            } else if (col_lower == "price_weight" || col_lower == "price_w") {
                price_w_idx = col_index;
            } else if (col_lower == "novelty_weight" || col_lower == "novelty_w") {
                novelty_w_idx = col_index;
            } else if (col_lower == "loyalty") {
                loyalty_idx = col_index;
            } else if (col_lower == "leaving_threshold") {
                leaving_threshold_idx = col_index;
            }
            // Map store valuation columns (store1_id_valuation, etc.)
            else if (col_lower.find("store") != string::npos && 
                     (col_lower.find("valuation") != string::npos || col_lower.find("_id_") != string::npos)) {
                size_t store_pos = col_lower.find("store");
                if (store_pos != string::npos) {
                    size_t num_start = store_pos + 5;
                    while (num_start < col_lower.length() && !isdigit(col_lower[num_start])) {
                        num_start++;
                    }
                    if (num_start < col_lower.length()) {
                        try {
                            size_t num_end = num_start;
                            while (num_end < col_lower.length() && isdigit(col_lower[num_end])) {
                                num_end++;
                            }
                            int store_id = stoi(col_lower.substr(num_start, num_end - num_start));
                            store_valuation_columns.push_back(col_index);
                            store_ids.push_back(store_id);
                        } catch (...) {
                            // Failed to parse store ID, skip this column
                        }
                    }
                }
            }
            
            col_index++;
        }
    }

    // Check for mandatory columns
    if (customer_id_idx == -1 || longitude_idx == -1 || latitude_idx == -1) {
        cerr << "Error: Missing mandatory columns in customer CSV file." << endl;
        cerr << "Required columns: CustomerID, longitude, latitude" << endl;
        if (customer_id_idx == -1) cerr << "  - Missing: CustomerID" << endl;
        if (longitude_idx == -1) cerr << "  - Missing: longitude" << endl;
        if (latitude_idx == -1) cerr << "  - Missing: latitude" << endl;
        file.close();
        return false;
    }

    // Read data rows
    int row_num = 1;
    while (getline(file, line)) {
        if (line.empty()) continue;
        
        row_num++;
        stringstream ss(line);
        vector<string> values;
        string val;
        
        // Parse CSV line
        while (getline(ss, val, ',')) {
            // Trim whitespace
            val.erase(0, val.find_first_not_of(" \t\r\n"));
            val.erase(val.find_last_not_of(" \t\r\n") + 1);
            values.push_back(val);
        }
        
        if (values.size() < header_columns.size()) {
            cerr << "Warning: Row " << row_num << " has fewer columns than header. Skipping." << endl;
            continue;
        }
        
        try {
            // Parse mandatory columns
            int customer_id = stoi(values[customer_id_idx]);
            float longitude = stof(values[longitude_idx]);
            float latitude = stof(values[latitude_idx]);
            
            // Parse store valuations
            map<int, float> store_valuations;
            for (size_t i = 0; i < store_valuation_columns.size(); i++) {
                int col_idx = store_valuation_columns[i];
                if (col_idx < (int)values.size() && !values[col_idx].empty()) {
                    try {
                        float valuation = stof(values[col_idx]);
                        store_valuations[store_ids[i]] = valuation;
                    } catch (...) {
                        // Skip invalid valuation
                    }
                }
            }
            
            // Parse or generate optional columns
            string customer_name;
            if (customer_name_idx != -1 && customer_name_idx < (int)values.size() && !values[customer_name_idx].empty()) {
                customer_name = values[customer_name_idx];
            } else {
                customer_name = "Customer_" + to_string(customer_id);
            }
            
            // Generate segment if missing (random distribution)
            string segment;
            if (segment_idx != -1 && segment_idx < (int)values.size() && !values[segment_idx].empty()) {
                segment = values[segment_idx];
            } else {
                // Random segment: budget (33%), regular (34%), premium (33%)
                int seg = rng() % 3;
                if (seg == 0) segment = "budget";
                else if (seg == 1) segment = "regular";
                else segment = "premium";
            }
            
            // Generate willingness_to_pay based on segment if missing
            float willingness_to_pay;
            if (wtp_idx != -1 && wtp_idx < (int)values.size() && !values[wtp_idx].empty()) {
                willingness_to_pay = stof(values[wtp_idx]);
            } else {
                // Generate based on segment
                if (segment == "budget") {
                    willingness_to_pay = 80.0f + (rng() % 40);  // 80-120 EGP
                } else if (segment == "regular") {
                    willingness_to_pay = 120.0f + (rng() % 60);  // 120-180 EGP
                } else {  // premium
                    willingness_to_pay = 180.0f + (rng() % 80);  // 180-260 EGP
                }
            }
            
            // Generate weights based on segment if missing
            float rating_w, price_w, novelty_w;
            
            if (rating_w_idx != -1 && rating_w_idx < (int)values.size() && !values[rating_w_idx].empty()) {
                rating_w = stof(values[rating_w_idx]);
            } else {
                if (segment == "budget") {
                    rating_w = 0.5f + (rng() % 100) / 200.0f;  // 0.5-1.0
                } else if (segment == "regular") {
                    rating_w = 1.0f + (rng() % 100) / 200.0f;  // 1.0-1.5
                } else {  // premium
                    rating_w = 1.5f + (rng() % 100) / 200.0f;  // 1.5-2.0
                }
            }
            
            if (price_w_idx != -1 && price_w_idx < (int)values.size() && !values[price_w_idx].empty()) {
                price_w = stof(values[price_w_idx]);
            } else {
                if (segment == "budget") {
                    price_w = 1.5f + (rng() % 100) / 200.0f;  // 1.5-2.0 (price sensitive)
                } else if (segment == "regular") {
                    price_w = 0.8f + (rng() % 80) / 200.0f;  // 0.8-1.2
                } else {  // premium
                    price_w = 0.3f + (rng() % 80) / 200.0f;  // 0.3-0.7 (less price sensitive)
                }
            }
            
            if (novelty_w_idx != -1 && novelty_w_idx < (int)values.size() && !values[novelty_w_idx].empty()) {
                novelty_w = stof(values[novelty_w_idx]);
            } else {
                if (segment == "budget") {
                    novelty_w = 0.2f + (rng() % 60) / 200.0f;  // 0.2-0.5
                } else if (segment == "regular") {
                    novelty_w = 0.4f + (rng() % 60) / 200.0f;  // 0.4-0.7
                } else {  // premium
                    novelty_w = 0.6f + (rng() % 80) / 200.0f;  // 0.6-1.0 (adventurous)
                }
            }
            
            // Generate leaving_threshold based on segment if missing
            float leaving_threshold;
            if (leaving_threshold_idx != -1 && leaving_threshold_idx < (int)values.size() && !values[leaving_threshold_idx].empty()) {
                leaving_threshold = stof(values[leaving_threshold_idx]);
            } else {
                if (segment == "budget") {
                    leaving_threshold = 1.5f + (rng() % 20) / 20.0f;  // 1.5-2.5
                } else if (segment == "regular") {
                    leaving_threshold = 2.5f + (rng() % 20) / 20.0f;  // 2.5-3.5
                } else {  // premium
                    leaving_threshold = 3.5f + (rng() % 20) / 20.0f;  // 3.5-4.5
                }
            }
            
            // Create customer with parsed/generated values
            Customer c(customer_id, longitude, latitude, customer_name, segment,
                      willingness_to_pay, rating_w, price_w, novelty_w, leaving_threshold);
            c.store_valuations = store_valuations;
            
            customers_from_csv.push_back(c);
            
        } catch (const exception& e) {
            cerr << "Warning: Error parsing row " << row_num << ": " << e.what() << ". Skipping." << endl;
            continue;
        }
    }

    file.close();
    
    cout << "Loaded " << customers_from_csv.size()
              << " customers from " << filename << endl;
    if (!store_valuation_columns.empty()) {
        cout << "  Found " << store_valuation_columns.size() << " store valuation columns" << endl;
    }
    
    // Count segments for verification
    int budget_count = 0, regular_count = 0, premium_count = 0;
    for (const auto& c : customers_from_csv) {
        if (c.segment == "budget") budget_count++;
        else if (c.segment == "regular") regular_count++;
        else if (c.segment == "premium") premium_count++;
    }
    cout << "  Segment distribution: Budget=" << budget_count 
              << ", Regular=" << regular_count 
              << ", Premium=" << premium_count << endl;
    
    return true;
}

// Generate a singe customer (either from CSV pool or random)
Customer ArrivalGenerator::generate_customer(int index, const vector<Restaurant>& restaurants) {
    if (!customers_from_csv.empty()) {
        // Recycle customers if we run out
        int csv_index = index % customers_from_csv.size();
        Customer c = customers_from_csv[csv_index];
        c.id = index;
        return c;
    }
    
    // Fallback: Generate completely random customer
    uniform_int_distribution<int> segment_dist(0, 2);
    int seg = segment_dist(rng);

    string segment;
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

    uniform_real_distribution<float> lon_dist(31.2f, 31.3f);
    uniform_real_distribution<float> lat_dist(30.0f, 30.1f);
    
    Customer customer(index, lon_dist(rng), lat_dist(rng), 
                     "Customer_" + to_string(index), segment, 
                     wtp, rating_w, price_w, novelty_w, leaving_thresh);
    
    if (!restaurants.empty()) {
        uniform_real_distribution<float> valuation_dist(0.0f, 5.0f);
        for (const auto& restaurant : restaurants) {
            float valuation = valuation_dist(rng);
            customer.store_valuations[restaurant.business_id] = valuation;
        }
    }
    
    return customer;
}

