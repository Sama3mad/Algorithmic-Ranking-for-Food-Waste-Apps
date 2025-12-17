#ifndef RESERVATION_H
#define RESERVATION_H

#include "Timestamp.h"

// ============================================================================
// RESERVATION MODEL
// ============================================================================
// Represents a customer's reservation for a surprise bag
// Tracks reservation status and timing
// ============================================================================
class Reservation {
public:
    enum Status {
        PENDING,    // Reservation made, waiting for end-of-day confirmation
        CONFIRMED,  // Reservation confirmed (store has enough bags)
        CANCELLED   // Reservation cancelled (store doesn't have enough bags)
    };

    int reservation_id;      // Unique reservation identifier
    int customer_id;          // ID of customer who made reservation
    int restaurant_id;       // ID of restaurant
    Timestamp reservation_time;  // When reservation was made
    Status status;           // Current status of reservation
    int bags_received;       // Number of bags actually given to customer (0 if cancelled)

    // Constructor
    Reservation(int res_id, int cust_id, int rest_id, Timestamp time);
};

#endif // RESERVATION_H

