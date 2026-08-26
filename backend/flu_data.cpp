#include "flu_data.h"

const FluData& getFluData() {
    static const FluData data{
        {
            {"Cambodia", 12.6, 105.0, 16},
            {"USA", 37.1, -95.7, 58},
            {"UK", 55.4, -3.4, 4},
            {"China", 35.9, 104.2, 2}
        },
        {
            {"USA", 37.1, -95.7, 8},
            {"UK", 55.4, -3.4, 1},
            {"Taiwan", 23.7, 121.0, 1},
            {"Brazil", -14.2, -51.9, 2}
        },
        {
            {"Iowa, USA", 41.5, -93.6, 15000, "poultry"},
            {"Hong Kong", 22.3, 114.2, 5000, "poultry"},
            {"Netherlands", 52.1, 5.3, 12000, "poultry"}
        },
        {
            {"North Carolina, USA", 35.5, -79.0, 5000, "pig"},
            {"Guangdong, China", 23.1, 113.3, 8000, "pig"},
            {"São Paulo, Brazil", -23.5, -46.6, 6000, "pig"}
        },
        {
            {"Live Poultry Markets", 22.3, 114.2, "bird", "high"},
            {"Pig Farms Midwest USA", 41.5, -93.6, "swine", "high"},
            {"Poultry Belt Europe", 52.1, 5.3, "bird", "high"}
        }
    };
    return data;
}

nlohmann::json fluDataToJson(const FluData& data) {
    auto caseArray = [](const std::vector<CaseLocation>& v) {
        nlohmann::json arr = nlohmann::json::array();
        for (const auto& c : v) {
            arr.push_back({{"location", c.location}, {"lat", c.lat}, {"lng", c.lng}, {"cases", c.cases}});
        }
        return arr;
    };
    auto animalArray = [](const std::vector<AnimalOutbreak>& v) {
        nlohmann::json arr = nlohmann::json::array();
        for (const auto& c : v) {
            arr.push_back({{"location", c.location}, {"lat", c.lat}, {"lng", c.lng},
                            {"cases", c.cases}, {"type", c.type}});
        }
        return arr;
    };

    nlohmann::json highRisk = nlohmann::json::array();
    for (const auto& a : data.highRiskAreas) {
        highRisk.push_back({{"name", a.name}, {"lat", a.lat}, {"lng", a.lng},
                             {"type", a.type}, {"risk", a.risk}});
    }

    nlohmann::json j;
    j["humanCases"]["birdFlu"] = caseArray(data.humanBirdFlu);
    j["humanCases"]["swineFlu"] = caseArray(data.humanSwineFlu);
    j["animalCases"]["birdFlu"] = animalArray(data.animalBirdFlu);
    j["animalCases"]["swineFlu"] = animalArray(data.animalSwineFlu);
    j["highRiskAreas"] = highRisk;
    return j;
}
