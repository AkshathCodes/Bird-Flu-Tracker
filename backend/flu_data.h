#pragma once

#include <string>
#include <vector>

#include <nlohmann/json.hpp>

struct CaseLocation {
    std::string location;
    double lat;
    double lng;
    int cases;
};

struct AnimalOutbreak {
    std::string location;
    double lat;
    double lng;
    int cases;
    std::string type;
};

struct HighRiskArea {
    std::string name;
    double lat;
    double lng;
    std::string type;
    std::string risk;
};

struct FluData {
    std::vector<CaseLocation> humanBirdFlu;
    std::vector<CaseLocation> humanSwineFlu;
    std::vector<AnimalOutbreak> animalBirdFlu;
    std::vector<AnimalOutbreak> animalSwineFlu;
    std::vector<HighRiskArea> highRiskAreas;
};

const FluData& getFluData();
nlohmann::json fluDataToJson(const FluData& data);
