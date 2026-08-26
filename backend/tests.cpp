#include <cassert>
#include <cstdio>

#include "flu_data.h"

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

    std::printf("All flu_data tests passed\n");
    return 0;
}
