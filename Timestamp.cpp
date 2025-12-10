#include "Timestamp.h"

Timestamp::Timestamp(int h, int m) : hour(h), minute(m) {}

int Timestamp::to_minutes() const {
    return hour * 60 + minute;
}

bool Timestamp::operator<(const Timestamp& other) const {
    return to_minutes() < other.to_minutes();
}

std::string Timestamp::to_string() const {
    return std::to_string(hour) + ":" +
        (minute < 10 ? "0" : "") + std::to_string(minute);
}

