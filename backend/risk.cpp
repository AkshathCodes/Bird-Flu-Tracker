#include "risk.h"

#include <cmath>
#include <limits>
#include <vector>

namespace {

constexpr double kEarthRadiusKm = 6371.0;
constexpr double kPi = 3.14159265358979323846;

struct Point {
    double lat;
    double lng;
    int cases;
};

RiskResult nearestRisk(double lat, double lng, const std::vector<Point>& points,
                        double safeDistance, const std::string& type) {
    double bestDistance = std::numeric_limits<double>::infinity();
    int bestCases = 0;
    for (const auto& p : points) {
        double d = haversineDistanceKm(lat, lng, p.lat, p.lng);
        if (d < bestDistance) {
            bestDistance = d;
            bestCases = p.cases;
        }
    }
    std::string level = getRiskLevel(bestDistance, safeDistance);
    return RiskResult{bestDistance, bestCases, level, getRiskAdvice(level, type)};
}

}  // namespace

double haversineDistanceKm(double lat1, double lon1, double lat2, double lon2) {
    double phi1 = lat1 * kPi / 180.0;
    double phi2 = lat2 * kPi / 180.0;
    double deltaPhi = (lat2 - lat1) * kPi / 180.0;
    double deltaLambda = (lon2 - lon1) * kPi / 180.0;

    double a = std::sin(deltaPhi / 2) * std::sin(deltaPhi / 2) +
               std::cos(phi1) * std::cos(phi2) *
               std::sin(deltaLambda / 2) * std::sin(deltaLambda / 2);
    return kEarthRadiusKm * 2 * std::atan2(std::sqrt(a), std::sqrt(1 - a));
}

std::string getRiskLevel(double distance, double safeDistance) {
    double ratio = distance / safeDistance;
    if (ratio < 0.3) return "high";
    if (ratio < 0.7) return "medium";
    if (ratio < 1) return "low";
    return "none";
}

std::string getRiskAdvice(const std::string& level, const std::string& type) {
    std::string general = (type == "bird")
        ? "Avoid contact with birds and poultry. Cook meat thoroughly."
        : "Practice good hygiene and consider vaccination.";

    std::string specific;
    if (level == "high") specific = "High risk area. Seek medical advice if symptomatic.";
    else if (level == "medium") specific = "Moderate risk. Monitor for symptoms.";
    else if (level == "low") specific = "Low risk. Maintain normal precautions.";
    else specific = "No significant risk detected.";

    return specific + " " + (level != "none" ? general : "");
}

RiskAssessment assessRisk(double lat, double lng, const FluData& data) {
    std::vector<Point> birdPoints;
    for (const auto& c : data.humanBirdFlu) birdPoints.push_back({c.lat, c.lng, c.cases});
    for (const auto& c : data.animalBirdFlu) birdPoints.push_back({c.lat, c.lng, c.cases});

    std::vector<Point> swinePoints;
    for (const auto& c : data.humanSwineFlu) swinePoints.push_back({c.lat, c.lng, c.cases});
    for (const auto& c : data.animalSwineFlu) swinePoints.push_back({c.lat, c.lng, c.cases});

    RiskAssessment result;
    result.bird = nearestRisk(lat, lng, birdPoints, SAFE_DISTANCE_BIRD_KM, "bird");
    result.swine = nearestRisk(lat, lng, swinePoints, SAFE_DISTANCE_SWINE_KM, "swine");
    return result;
}
