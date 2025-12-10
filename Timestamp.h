#ifndef TIMESTAMP_H
#define TIMESTAMP_H

#include <string>

// ============================================================================
// TIMESTAMP UTILITIES
// ============================================================================
// Represents a time of day with hour and minute
// Used throughout the simulation to track when events occur
// ============================================================================
struct Timestamp {
    int hour;   // Hour of day (0-23)
    int minute; // Minute of hour (0-59)

    // Constructor: default is 8:00 AM (start of business day)
    Timestamp(int h = 8, int m = 0);

    // Convert timestamp to total minutes since midnight
    // Used for time comparisons and sorting
    int to_minutes() const;

    // Comparison operator for sorting reservations by time
    bool operator<(const Timestamp& other) const;

    // Convert to human-readable string format (e.g., "14:30")
    std::string to_string() const;
};

#endif // TIMESTAMP_H

