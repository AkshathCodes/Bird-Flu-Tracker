#pragma once

#include <string>

#include "flu_data.h"

constexpr double SAFE_DISTANCE_BIRD_KM = 100.0;
constexpr double SAFE_DISTANCE_SWINE_KM = 50.0;

double haversineDistanceKm(double lat1, double lon1, double lat2, double lon2);

std::string getRiskLevel(double distance, double safeDistance);
std::string getRiskAdvice(const std::string& level, const std::string& type);

struct RiskResult {
    double distance;
    int cases;
    std::string level;
    std::string advice;
};

struct RiskAssessment {
    RiskResult bird;
    RiskResult swine;
};

RiskAssessment assessRisk(double lat, double lng, const FluData& data);
