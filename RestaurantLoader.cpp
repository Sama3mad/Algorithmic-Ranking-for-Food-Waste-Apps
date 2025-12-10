#include "RestaurantLoader.h"
#include <fstream>
#include <sstream>
#include <iostream>
#include <algorithm>

bool RestaurantLoader::load_restaurants_from_csv(const std::string& filename,
                                                  std::vector<Restaurant>& restaurants) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        std::cerr << "Warning: Could not open restaurant CSV file: " << filename << std::endl;
        std::cerr << "Will use default restaurants instead." << std::endl;
        return false;
    }

    std::string line;
    bool firstLine = true;
    std::vector<std::string> header_columns;
    int store_id_idx = -1, store_name_idx = -1, branch_idx = -1;
    int bags_idx = -1, rating_idx = -1, price_idx = -1;
    int longitude_idx = -1, latitude_idx = -1;
    int business_type_idx = -1;  // Optional extra column

    // Read header
    if (std::getline(file, line)) {
        std::stringstream header_ss(line);
        std::string col;
        int col_index = 0;
        while (std::getline(header_ss, col, ',')) {
            // Trim whitespace
            col.erase(0, col.find_first_not_of(" \t"));
            col.erase(col.find_last_not_of(" \t") + 1);
            
            header_columns.push_back(col);
            
            // Convert to lowercase for comparison
            std::string col_lower = col;
            std::transform(col_lower.begin(), col_lower.end(), col_lower.begin(), ::tolower);
            
            // Map required columns
            if (col_lower == "store_id") store_id_idx = col_index;
            else if (col_lower == "store_name") store_name_idx = col_index;
            else if (col_lower == "branch") branch_idx = col_index;
            else if (col_lower == "average_bags_at_9am") bags_idx = col_index;
            else if (col_lower == "average_overall_rating") rating_idx = col_index;
            else if (col_lower == "price") price_idx = col_index;
            else if (col_lower == "longitude") longitude_idx = col_index;
            else if (col_lower == "latitude") latitude_idx = col_index;
            // Optional extra columns
            else if (col_lower == "business_type" || col_lower == "type") business_type_idx = col_index;
            
            col_index++;
        }
    }

    // Check if all required columns are present
    if (store_id_idx == -1 || store_name_idx == -1 || branch_idx == -1 ||
        bags_idx == -1 || rating_idx == -1 || price_idx == -1 ||
        longitude_idx == -1 || latitude_idx == -1) {
        std::cerr << "Error: Missing required columns in CSV file." << std::endl;
        std::cerr << "Required columns: store_id, store_name, branch, average_bags_at_9AM, "
                  << "average_overall_rating, price, longitude, latitude" << std::endl;
        file.close();
        return false;
    }

    // Read data rows
    int row_num = 1;
    while (std::getline(file, line)) {
        if (line.empty()) continue;
        
        row_num++;
        std::stringstream ss(line);
        std::vector<std::string> values;
        std::string val;
        
        // Parse CSV line (handle quoted values)
        bool in_quotes = false;
        std::string current_val;
        for (char c : line) {
            if (c == '"') {
                in_quotes = !in_quotes;
            } else if (c == ',' && !in_quotes) {
                values.push_back(current_val);
                current_val.clear();
            } else {
                current_val += c;
            }
        }
        values.push_back(current_val);  // Add last value
        
        // Trim whitespace from values
        for (auto& v : values) {
            v.erase(0, v.find_first_not_of(" \t"));
            v.erase(v.find_last_not_of(" \t") + 1);
        }
        
        if (values.size() < header_columns.size()) {
            std::cerr << "Warning: Row " << row_num << " has fewer columns than header. Skipping." << std::endl;
            continue;
        }
        
        try {
            // Parse required CSV columns
            int store_id = std::stoi(values[store_id_idx]);
            std::string store_name = values[store_name_idx];
            std::string branch = values[branch_idx];
            int bags = std::stoi(values[bags_idx]);
            float rating = std::stof(values[rating_idx]);
            float price = std::stof(values[price_idx]);
            float longitude = std::stof(values[longitude_idx]);
            float latitude = std::stof(values[latitude_idx]);
            
            // Parse optional extra columns
            std::string business_type = "";
            if (business_type_idx != -1 && business_type_idx < (int)values.size()) {
                business_type = values[business_type_idx];
            }
            
            // Create restaurant
            Restaurant restaurant(store_id, store_name, branch, bags, rating, price,
                                 longitude, latitude, business_type);
            
            restaurants.push_back(restaurant);
        } catch (const std::exception& e) {
            std::cerr << "Warning: Error parsing row " << row_num << ": " << e.what() << ". Skipping." << std::endl;
            continue;
        }
    }

    file.close();
    std::cout << "Loaded " << restaurants.size() << " restaurants from " << filename << std::endl;
    return true;
}

void RestaurantLoader::generate_default_restaurants(std::vector<Restaurant>& restaurants) {
    // Generate default restaurants with all required CSV columns (increased to 15 stores)
    restaurants.push_back(Restaurant(1, "Krispy Kreme", "Zamalek", 10, 4.8f, 80.0f, 31.22f, 30.05f, "bakery"));
    restaurants.push_back(Restaurant(2, "TBS Pizza", "New Cairo", 10, 4.2f, 150.0f, 31.25f, 30.08f, "restaurant"));
    restaurants.push_back(Restaurant(3, "Starbucks", "Zamalek", 15, 4.5f, 100.0f, 31.23f, 30.06f, "cafe"));
    restaurants.push_back(Restaurant(4, "Paul Bakery", "New Cairo", 12, 4.6f, 90.0f, 31.26f, 30.09f, "bakery"));
    restaurants.push_back(Restaurant(5, "Costa Coffee", "Zamalek", 8, 4.3f, 85.0f, 31.24f, 30.07f, "cafe"));
    restaurants.push_back(Restaurant(6, "Greggs", "New Cairo", 20, 4.0f, 70.0f, 31.27f, 30.10f, "bakery"));
    restaurants.push_back(Restaurant(7, "Pizza Hut", "Zamalek", 14, 4.1f, 140.0f, 31.21f, 30.04f, "restaurant"));
    restaurants.push_back(Restaurant(8, "Pret A Manger", "New Cairo", 18, 4.4f, 95.0f, 31.28f, 30.11f, "cafe"));
    restaurants.push_back(Restaurant(9, "Subway", "Zamalek", 16, 3.9f, 110.0f, 31.20f, 30.03f, "restaurant"));
    restaurants.push_back(Restaurant(10, "Tim Hortons", "New Cairo", 10, 4.2f, 80.0f, 31.29f, 30.12f, "cafe"));
    restaurants.push_back(Restaurant(11, "Dunkin Donuts", "Zamalek", 12, 4.3f, 75.0f, 31.19f, 30.02f, "bakery"));
    restaurants.push_back(Restaurant(12, "Domino's Pizza", "New Cairo", 15, 4.0f, 130.0f, 31.30f, 30.13f, "restaurant"));
    restaurants.push_back(Restaurant(13, "Cinnabon", "Zamalek", 9, 4.4f, 85.0f, 31.18f, 30.01f, "bakery"));
    restaurants.push_back(Restaurant(14, "Caribou Coffee", "New Cairo", 11, 4.1f, 90.0f, 31.31f, 30.14f, "cafe"));
    restaurants.push_back(Restaurant(15, "Panera Bread", "Zamalek", 13, 4.2f, 95.0f, 31.17f, 30.00f, "restaurant"));
}

