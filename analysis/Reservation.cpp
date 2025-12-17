#include "Reservation.h"

using namespace std;

Reservation::Reservation(int res_id, int cust_id, int rest_id, Timestamp time)
    : reservation_id(res_id), customer_id(cust_id),
      restaurant_id(rest_id), reservation_time(time),
      status(PENDING), bags_received(0) {
}

