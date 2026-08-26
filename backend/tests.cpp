#include <cmath>
#include <cstdio>
#include <cstdlib>

#include "flu_data.h"
#include "risk.h"

static void check(bool ok, const char* what) {
    if (!ok) {
        std::fprintf(stderr, "FAILED: %s\n", what);
        std::exit(1);
    }
}
#define CHECK(x) check((x), #x)

int main() {
    const FluData& data = getFluData();

    int birdHumanTotal = 0;
    for (const auto& c : data.humanBirdFlu) birdHumanTotal += c.cases;
    CHECK(birdHumanTotal == 80);

    int swineHumanTotal = 0;
    for (const auto& c : data.humanSwineFlu) swineHumanTotal += c.cases;
    CHECK(swineHumanTotal == 12);

    int animalTotal = 0;
    for (const auto& c : data.animalBirdFlu) animalTotal += c.cases;
    for (const auto& c : data.animalSwineFlu) animalTotal += c.cases;
    CHECK(animalTotal == 51000);

    CHECK(data.highRiskAreas.size() == 3);

    nlohmann::json j = fluDataToJson(data);
    CHECK(j["humanCases"]["birdFlu"].size() == 4);
    CHECK(j["humanCases"]["birdFlu"][0]["location"] == "Cambodia");
    CHECK(j["highRiskAreas"].size() == 3);

    CHECK(haversineDistanceKm(10.0, 20.0, 10.0, 20.0) == 0.0);
    CHECK(std::fabs(haversineDistanceKm(0.0, 0.0, 0.0, 90.0) - 10007.5) < 1.0);
    CHECK(std::fabs(haversineDistanceKm(90.0, 0.0, -90.0, 0.0) - 20015.1) < 1.0);

    CHECK(getRiskLevel(10.0, 100.0) == "high");
    CHECK(getRiskLevel(50.0, 100.0) == "medium");
    CHECK(getRiskLevel(80.0, 100.0) == "low");
    CHECK(getRiskLevel(150.0, 100.0) == "none");
    CHECK(getRiskLevel(30.0, 100.0) == "medium");
    CHECK(getRiskLevel(70.0, 100.0) == "low");
    CHECK(getRiskLevel(100.0, 100.0) == "none");

    CHECK(getRiskAdvice("high", "bird") ==
        "High risk area. Seek medical advice if symptomatic. "
        "Avoid contact with birds and poultry. Cook meat thoroughly.");
    CHECK(getRiskAdvice("none", "swine") == "No significant risk detected. ");

    RiskAssessment nearCambodia = assessRisk(12.6, 105.0, data);
    CHECK(nearCambodia.bird.distance < 1.0);
    CHECK(nearCambodia.bird.cases == 16);

    std::printf("All tests passed\n");
    return 0;
}
