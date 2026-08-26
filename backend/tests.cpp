#include <cassert>
#include <cmath>
#include <cstdio>

#include "flu_data.h"
#include "risk.h"

int main() {
    const FluData& data = getFluData();

    int birdHumanTotal = 0;
    for (const auto& c : data.humanBirdFlu) birdHumanTotal += c.cases;
    assert(birdHumanTotal == 80);

    int swineHumanTotal = 0;
    for (const auto& c : data.humanSwineFlu) swineHumanTotal += c.cases;
    assert(swineHumanTotal == 12);

    int animalTotal = 0;
    for (const auto& c : data.animalBirdFlu) animalTotal += c.cases;
    for (const auto& c : data.animalSwineFlu) animalTotal += c.cases;
    assert(animalTotal == 51000);

    assert(data.highRiskAreas.size() == 3);

    nlohmann::json j = fluDataToJson(data);
    assert(j["humanCases"]["birdFlu"].size() == 4);
    assert(j["humanCases"]["birdFlu"][0]["location"] == "Cambodia");
    assert(j["highRiskAreas"].size() == 3);

    assert(haversineDistanceKm(10.0, 20.0, 10.0, 20.0) == 0.0);
    assert(std::fabs(haversineDistanceKm(0.0, 0.0, 0.0, 90.0) - 10007.5) < 1.0);
    assert(std::fabs(haversineDistanceKm(90.0, 0.0, -90.0, 0.0) - 20015.1) < 1.0);

    assert(getRiskLevel(10.0, 100.0) == "high");
    assert(getRiskLevel(50.0, 100.0) == "medium");
    assert(getRiskLevel(80.0, 100.0) == "low");
    assert(getRiskLevel(150.0, 100.0) == "none");

    assert(getRiskAdvice("high", "bird") ==
        "High risk area. Seek medical advice if symptomatic. "
        "Avoid contact with birds and poultry. Cook meat thoroughly.");
    assert(getRiskAdvice("none", "swine") == "No significant risk detected. ");

    RiskAssessment nearCambodia = assessRisk(12.6, 105.0, data);
    assert(nearCambodia.bird.distance < 1.0);
    assert(nearCambodia.bird.cases == 16);

    std::printf("All tests passed\n");
    return 0;
}
