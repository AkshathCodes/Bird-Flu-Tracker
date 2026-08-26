#include <chrono>
#include <cstdlib>
#include <filesystem>
#include <thread>

#include "flu_data.h"
#include "httplib.h"
#include "risk.h"

namespace {

void openBrowser() {
    std::this_thread::sleep_for(std::chrono::milliseconds(300));
#ifdef _WIN32
    std::system("start http://localhost:8080");
#elif defined(__APPLE__)
    std::system("open http://localhost:8080");
#else
    std::system("xdg-open http://localhost:8080");
#endif
}

}  // namespace

int main(int argc, char** argv) {
    std::filesystem::path exeDir = std::filesystem::absolute(argv[0]).parent_path();
    std::filesystem::path webDir = exeDir / "web";

    httplib::Server server;
    server.set_mount_point("/", webDir.string());

    server.Get("/api/flu-data", [](const httplib::Request&, httplib::Response& res) {
        res.set_content(fluDataToJson(getFluData()).dump(), "application/json");
    });

    server.Get("/api/risk", [](const httplib::Request& req, httplib::Response& res) {
        if (!req.has_param("lat") || !req.has_param("lng")) {
            res.status = 400;
            res.set_content(nlohmann::json{{"error", "lat and lng are required"}}.dump(),
                             "application/json");
            return;
        }
        try {
            double lat = std::stod(req.get_param_value("lat"));
            double lng = std::stod(req.get_param_value("lng"));
            RiskAssessment assessment = assessRisk(lat, lng, getFluData());

            nlohmann::json j;
            j["bird"] = {{"distance", assessment.bird.distance}, {"cases", assessment.bird.cases},
                         {"level", assessment.bird.level}, {"advice", assessment.bird.advice}};
            j["swine"] = {{"distance", assessment.swine.distance}, {"cases", assessment.swine.cases},
                          {"level", assessment.swine.level}, {"advice", assessment.swine.advice}};
            res.set_content(j.dump(), "application/json");
        } catch (const std::exception&) {
            res.status = 400;
            res.set_content(nlohmann::json{{"error", "lat and lng must be numbers"}}.dump(),
                             "application/json");
        }
    });

    std::thread(openBrowser).detach();
    server.listen("localhost", 8080);
    return 0;
}
