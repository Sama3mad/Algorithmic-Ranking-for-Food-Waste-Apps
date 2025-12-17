#ifndef TIMESTAMP_H
#define TIMESTAMP_H

#include <string>

using namespace std;

// Timestamp Utilities
struct Timestamp {
    int hour;
    int minute;

    // Constructor
    Timestamp(int h = 8, int m = 0);

    // Convert to minutes
    int to_minutes() const;

    // Comparison
    bool operator<(const Timestamp& other) const;

    // Convert to string
    string to_string() const;
};

#endif // TIMESTAMP_H

