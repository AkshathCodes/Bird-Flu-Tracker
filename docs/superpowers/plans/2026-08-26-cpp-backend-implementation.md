# C++ Backend Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Recode the Bird Flu Tracker's outbreak dataset and risk-assessment logic from client-side JavaScript into a C++ HTTP backend, while reusing the existing HTML/Leaflet frontend, and publish the result to the `AkshathCodes/Bird-Flu-Tracker` GitHub repo.

**Architecture:** A single C++ executable (`bird_flu_tracker`) built with cpp-httplib serves the static frontend from a sibling `web/` folder and exposes `GET /api/flu-data` and `GET /api/risk` JSON endpoints backed by a small `flu_core` static library (`flu_data.cpp`, `risk.cpp`). The frontend's JS is edited to fetch from these endpoints instead of reading a hardcoded object and running risk math locally. Launching the executable opens the user's default browser to `http://localhost:8080`.

**Tech Stack:** C++17, CMake + FetchContent, cpp-httplib (header-only HTTP server), nlohmann/json (header-only JSON).

**Spec:** `docs/superpowers/specs/2026-08-26-cpp-backend-design.md`

## Global Constraints

- C++17 standard.
- Windows-only executable target (per spec Non-goals — no cross-platform build guarantee).
- Only two third-party dependencies, both fetched via CMake `FetchContent`: cpp-httplib (`GIT_TAG master`) and nlohmann/json (`GIT_TAG v3.11.3`). No other third-party libraries.
- No test framework — tests are plain `<cassert>` checks in `backend/tests.cpp`, built as their own executable.
- Static frontend assets ship as a sibling `web/` folder next to the executable (copied there by a CMake post-build step) — not embedded into the binary.
- The "Subscribe to alerts" email feature is dropped, not ported (it was already non-functional in the original).
- City-name geocoding is untouched — it keeps calling Nominatim directly from browser JS; the C++ backend is not involved in it.

---

### Task 1: Project scaffolding + minimal static server

**Files:**
- Create: `web/index.html` (copy of `C:\Users\akshu\Downloads\index.html`, unmodified)
- Create: `.gitignore`
- Create: `CMakeLists.txt`
- Create: `backend/CMakeLists.txt`
- Create: `backend/main.cpp`

**Interfaces:**
- Produces: an executable that serves whatever is in the top-level `web/` directory at `http://localhost:8080/`, and opens the default browser to that URL on startup. Later tasks add API routes to this same `main.cpp`.

- [ ] **Step 1: Copy the original page into `web/index.html`**

Copy `C:\Users\akshu\Downloads\index.html` to `web/index.html` in the repo, byte-for-byte, unmodified. This is the starting point that Task 5 will edit.

- [ ] **Step 2: Create `.gitignore`**

```
build/
```

- [ ] **Step 3: Create top-level `CMakeLists.txt`**

```cmake
cmake_minimum_required(VERSION 3.16)
project(BirdFluTracker CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_subdirectory(backend)
```

- [ ] **Step 4: Create `backend/CMakeLists.txt`**

```cmake
include(FetchContent)

FetchContent_Declare(
    httplib
    GIT_REPOSITORY https://github.com/yhirose/cpp-httplib.git
    GIT_TAG master
)
FetchContent_MakeAvailable(httplib)

add_executable(bird_flu_tracker main.cpp)
target_link_libraries(bird_flu_tracker PRIVATE httplib::httplib)

add_custom_command(TARGET bird_flu_tracker POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_SOURCE_DIR}/web
        $<TARGET_FILE_DIR:bird_flu_tracker>/web
)
```

- [ ] **Step 5: Create `backend/main.cpp`**

```cpp
#include <chrono>
#include <cstdlib>
#include <filesystem>
#include <thread>

#include "httplib.h"

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

    std::thread(openBrowser).detach();
    server.listen("localhost", 8080);
    return 0;
}
```

- [ ] **Step 6: Configure and build**

Run:
```
cmake -S . -B build
cmake --build build --config Release
```
Expected: build succeeds, no errors.

- [ ] **Step 7: Verify the server serves the original page**

Find the built executable:
```powershell
Get-ChildItem -Path build -Recurse -Filter "bird_flu_tracker.exe"
```

Run it, request the page, then stop it:
```powershell
$proc = Start-Process -FilePath "<path found above>" -PassThru
Start-Sleep -Seconds 1
$content = (Invoke-WebRequest http://localhost:8080/ -UseBasicParsing).Content
$content -match "Global Flu Tracker"
Stop-Process -Id $proc.Id
```
Expected: the match expression prints `True`.

- [ ] **Step 8: Commit**

```bash
git add web/index.html .gitignore CMakeLists.txt backend/CMakeLists.txt backend/main.cpp
git commit -m "Add CMake scaffolding and minimal static file server"
```

---

### Task 2: Port the flu dataset to C++

**Files:**
- Create: `backend/flu_data.h`
- Create: `backend/flu_data.cpp`
- Create: `backend/tests.cpp`
- Modify: `backend/CMakeLists.txt`

**Interfaces:**
- Consumes: nothing from Task 1 besides the existing build setup.
- Produces: `struct FluData` with fields `humanBirdFlu`, `humanSwineFlu`, `animalBirdFlu`, `animalSwineFlu`, `highRiskAreas` (types `std::vector<CaseLocation>`, `std::vector<CaseLocation>`, `std::vector<AnimalOutbreak>`, `std::vector<AnimalOutbreak>`, `std::vector<HighRiskArea>`); `const FluData& getFluData()`; `nlohmann::json fluDataToJson(const FluData&)`. Task 3 and Task 4 depend on these exact names and shapes.

- [ ] **Step 1: Write the failing test**

Create `backend/tests.cpp`:

```cpp
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
```

- [ ] **Step 2: Create `backend/flu_data.h`**

```cpp
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
```

- [ ] **Step 3: Wire up the build so the test target exists (it will fail to link)**

Modify `backend/CMakeLists.txt` to add the `json` dependency, the `flu_core` library, and the `backend_tests` executable. Replace the whole file with:

```cmake
include(FetchContent)

FetchContent_Declare(
    httplib
    GIT_REPOSITORY https://github.com/yhirose/cpp-httplib.git
    GIT_TAG master
)
FetchContent_MakeAvailable(httplib)

FetchContent_Declare(
    json
    GIT_REPOSITORY https://github.com/nlohmann/json.git
    GIT_TAG v3.11.3
)
FetchContent_MakeAvailable(json)

if (MSVC)
    add_compile_options(/utf-8)
endif()

add_library(flu_core STATIC
    flu_data.cpp
)
target_link_libraries(flu_core PUBLIC nlohmann_json::nlohmann_json)
target_include_directories(flu_core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

add_executable(bird_flu_tracker main.cpp)
target_link_libraries(bird_flu_tracker PRIVATE httplib::httplib)

add_executable(backend_tests tests.cpp)
target_link_libraries(backend_tests PRIVATE flu_core)

add_custom_command(TARGET bird_flu_tracker POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_SOURCE_DIR}/web
        $<TARGET_FILE_DIR:bird_flu_tracker>/web
)
```

- [ ] **Step 4: Run the build to verify it fails**

Run: `cmake -S . -B build && cmake --build build --config Release`
Expected: FAIL — linker error, `getFluData`/`fluDataToJson` are declared but not defined (no `flu_data.cpp` yet).

- [ ] **Step 5: Create `backend/flu_data.cpp` with the ported dataset**

```cpp
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
            {"S\u00e3o Paulo, Brazil", -23.5, -46.6, 6000, "pig"}
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
```

Note: `S\u00e3o` is the JSON/C++ universal-character-name escape for "São" — used instead of a literal UTF-8 byte in source to avoid any editor/compiler encoding ambiguity; it compiles to the same UTF-8 string.

- [ ] **Step 6: Run the build and tests to verify they pass**

Run:
```
cmake --build build --config Release
```
Then find and run `backend_tests` the same way Task 1 Step 7 found `bird_flu_tracker.exe` (same directory, filename `backend_tests.exe`).
Expected: prints `All flu_data tests passed`, exit code `0`.

- [ ] **Step 7: Commit**

```bash
git add backend/flu_data.h backend/flu_data.cpp backend/tests.cpp backend/CMakeLists.txt
git commit -m "Port flu outbreak dataset from JS to C++"
```

---

### Task 3: Port risk-assessment logic to C++

**Files:**
- Create: `backend/risk.h`
- Create: `backend/risk.cpp`
- Modify: `backend/tests.cpp`
- Modify: `backend/CMakeLists.txt`

**Interfaces:**
- Consumes: `FluData`, `CaseLocation`, `AnimalOutbreak` from Task 2 (`flu_data.h`).
- Produces: `double haversineDistanceKm(double lat1, double lon1, double lat2, double lon2)`; `std::string getRiskLevel(double distance, double safeDistance)`; `std::string getRiskAdvice(const std::string& level, const std::string& type)`; `struct RiskResult { double distance; int cases; std::string level; std::string advice; }`; `struct RiskAssessment { RiskResult bird; RiskResult swine; }`; `RiskAssessment assessRisk(double lat, double lng, const FluData& data)`. Task 4 depends on `assessRisk` and `RiskAssessment`/`RiskResult` exactly as named here.

- [ ] **Step 1: Write the failing tests**

Replace `backend/tests.cpp` with:

```cpp
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
```

- [ ] **Step 2: Create `backend/risk.h`**

```cpp
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
```

- [ ] **Step 3: Wire risk.cpp into the build (it doesn't exist yet, so this fails)**

Modify `backend/CMakeLists.txt`: change the `flu_core` sources list from

```cmake
add_library(flu_core STATIC
    flu_data.cpp
)
```

to

```cmake
add_library(flu_core STATIC
    flu_data.cpp
    risk.cpp
)
```

- [ ] **Step 4: Run the build to verify it fails**

Run: `cmake -S . -B build && cmake --build build --config Release`
Expected: FAIL — `risk.cpp` does not exist.

- [ ] **Step 5: Create `backend/risk.cpp`**

```cpp
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
```

- [ ] **Step 6: Run the build and tests to verify they pass**

Run: `cmake --build build --config Release`, then run `backend_tests.exe` (same discovery method as before).
Expected: prints `All tests passed`, exit code `0`.

- [ ] **Step 7: Commit**

```bash
git add backend/risk.h backend/risk.cpp backend/tests.cpp backend/CMakeLists.txt
git commit -m "Port Haversine distance and risk-assessment logic to C++"
```

---

### Task 4: Expose the data and risk API endpoints

**Files:**
- Modify: `backend/main.cpp`
- Modify: `backend/CMakeLists.txt`

**Interfaces:**
- Consumes: `getFluData()`, `fluDataToJson()` from Task 2; `assessRisk()`, `RiskAssessment`, `RiskResult` from Task 3.
- Produces: `GET /api/flu-data` and `GET /api/risk?lat=&lng=` HTTP routes. Task 5's frontend JS depends on these exact paths, query parameter names, and response shapes (as documented in the spec's API contract).

- [ ] **Step 1: Modify `backend/CMakeLists.txt` to link `flu_core` into the server executable**

Change:
```cmake
add_executable(bird_flu_tracker main.cpp)
target_link_libraries(bird_flu_tracker PRIVATE httplib::httplib)
```
to:
```cmake
add_executable(bird_flu_tracker main.cpp)
target_link_libraries(bird_flu_tracker PRIVATE flu_core httplib::httplib)
```

- [ ] **Step 2: Modify `backend/main.cpp` to add the two API routes**

Replace the whole file with:

```cpp
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
```

- [ ] **Step 3: Build**

Run: `cmake --build build --config Release`
Expected: build succeeds.

- [ ] **Step 4: Verify the endpoints manually**

```powershell
$exe = (Get-ChildItem -Path build -Recurse -Filter "bird_flu_tracker.exe").FullName
$proc = Start-Process -FilePath $exe -PassThru
Start-Sleep -Seconds 1

$data = (Invoke-WebRequest "http://localhost:8080/api/flu-data" -UseBasicParsing).Content | ConvertFrom-Json
$data.humanCases.birdFlu.Count            # expect 4
$data.humanCases.birdFlu[0].location      # expect Cambodia

$risk = (Invoke-WebRequest "http://localhost:8080/api/risk?lat=12.6&lng=105.0" -UseBasicParsing).Content | ConvertFrom-Json
$risk.bird.cases                          # expect 16
$risk.bird.level                          # expect high

try {
    Invoke-WebRequest "http://localhost:8080/api/risk" -UseBasicParsing
} catch {
    $_.Exception.Response.StatusCode.value__   # expect 400
}

Stop-Process -Id $proc.Id
```
Expected: the marked values match.

- [ ] **Step 5: Commit**

```bash
git add backend/main.cpp backend/CMakeLists.txt
git commit -m "Expose /api/flu-data and /api/risk endpoints"
```

---

### Task 5: Rewire the frontend to use the backend

**Files:**
- Modify: `web/index.html`

**Interfaces:**
- Consumes: `GET /api/flu-data` and `GET /api/risk?lat=&lng=` from Task 4, with the response shapes documented in the spec.
- Produces: no new interfaces — this is the last consumer in the chain.

- [ ] **Step 1: Replace the first `<script>` block's logic**

In `web/index.html`, the first `<script>` block (the one starting with `// Constants` and ending right before the second `<script>` block that defines `runPrediction`) currently declares `SAFE_DISTANCES`, a hardcoded `fluData` object, and the client-side `calculateDistance`/`getRiskLevel`/`getRiskAdvice`/`assessRisk`/`displayRiskResults`/`initApp` functions. Replace that entire block with:

```html
<script>
        // App State
        let fluData = null;

        const state = {
            map: null,
            userMarker: null,
            currentLayer: "human",
            markers: {
                human: [],
                animal: []
            }
        };

        // Initialize Application
        document.addEventListener('DOMContentLoaded', function() {
            initApp();
        });

        async function initApp() {
            const response = await fetch('/api/flu-data');
            fluData = await response.json();
            initMap();
            updateStats();
            document.getElementById('updateTime').textContent = formatDate(new Date());
            document.getElementById('currentDate').textContent = formatDate(new Date());
        }

        // Initialize Map - Fixed version
        function initMap() {
            state.map = L.map('map').setView([20, 0], 2);
            L.tileLayer('https://{s}.tile.openstreetmap.org/{z}/{x}/{y}.png', {
                attribution: '&copy; <a href="https://www.openstreetmap.org/copyright">OpenStreetMap</a>'
            }).addTo(state.map);

            // Add human cases by default
            showData('human');
        }

        // Show human or animal data
        function showData(layer) {
            // Update active button
            document.querySelectorAll('.toggle-btn').forEach(btn => {
                btn.classList.toggle('active', btn.textContent.includes(layer === 'human' ? 'Human' : 'Animal'));
            });

            // Clear existing markers
            state.markers[state.currentLayer].forEach(marker => state.map.removeLayer(marker));
            state.markers[state.currentLayer] = [];
            
            state.currentLayer = layer;
            
            if (layer === 'human') {
                addHumanCases();
            } else {
                addAnimalCases();
            }
            
            // Always show high risk areas
            addHighRiskAreas();
        }

        // Add human case markers
        function addHumanCases() {
            // Bird flu human cases
            fluData.humanCases.birdFlu.forEach(location => {
                const marker = L.circleMarker([location.lat, location.lng], {
                    radius: 5 + Math.min(15, location.cases / 2),
                    fillColor: '#d32f2f',
                    color: '#fff',
                    weight: 1,
                    fillOpacity: 0.8
                }).bindPopup(`
                    <b>${location.location}</b><br>
                    <i class="fa fa-dove"></i> Bird Flu Human Cases<br>
                    Cases: ${location.cases}
                `).addTo(state.map);
                
                state.markers.human.push(marker);
            });
            
            // Swine flu human cases
            fluData.humanCases.swineFlu.forEach(location => {
                const marker = L.circleMarker([location.lat, location.lng], {
                    radius: 5 + Math.min(15, location.cases),
                    fillColor: '#fb8c00',
                    color: '#fff',
                    weight: 1,
                    fillOpacity: 0.8
                }).bindPopup(`
                    <b>${location.location}</b><br>
                    <i class="fa fa-piggy-bank"></i> Swine Flu Human Cases<br>
                    Cases: ${location.cases}
                `).addTo(state.map);
                
                state.markers.human.push(marker);
            });
        }

        // Add animal case markers
        function addAnimalCases() {
            // Bird flu animal cases
            fluData.animalCases.birdFlu.forEach(outbreak => {
                const marker = L.circleMarker([outbreak.lat, outbreak.lng], {
                    radius: 5 + Math.min(20, outbreak.cases / 1000),
                    fillColor: '#9c27b0',
                    color: '#fff',
                    weight: 1,
                    fillOpacity: 0.8
                }).bindPopup(`
                    <b>${outbreak.location}</b><br>
                    <i class="fa fa-dove"></i> Bird Flu in ${outbreak.type === "poultry" ? "Poultry" : "Wild Birds"}<br>
                    Cases: ${outbreak.cases.toLocaleString()}
                `).addTo(state.map);
                
                state.markers.animal.push(marker);
            });
            
            // Swine flu animal cases
            fluData.animalCases.swineFlu.forEach(outbreak => {
                const marker = L.circleMarker([outbreak.lat, outbreak.lng], {
                    radius: 5 + Math.min(20, outbreak.cases / 500),
                    fillColor: '#795548',
                    color: '#fff',
                    weight: 1,
                    fillOpacity: 0.8
                }).bindPopup(`
                    <b>${outbreak.location}</b><br>
                    <i class="fa fa-piggy-bank"></i> Swine Flu in Pigs<br>
                    Cases: ${outbreak.cases.toLocaleString()}
                `).addTo(state.map);
                
                state.markers.animal.push(marker);
            });
        }

        // Add high risk areas
        function addHighRiskAreas() {
            fluData.highRiskAreas.forEach(area => {
                const color = area.risk === "high" ? '#ff0000' : '#ff9800';
                const rect = L.rectangle(
                    [
                        [area.lat - 2, area.lng - 2],
                        [area.lat + 2, area.lng + 2]
                    ],
                    {
                        color: color,
                        weight: 1,
                        fillColor: color,
                        fillOpacity: 0.2
                    }
                ).bindPopup(`
                    <b>${area.name}</b><br>
                    <i class="fa fa-exclamation-triangle"></i> ${area.type === "bird" ? "Bird Flu" : "Swine Flu"} High Risk Area
                `).addTo(state.map);
                
                // Add to both layers
                state.markers.human.push(rect);
                state.markers.animal.push(rect);
            });
        }

        // Update statistics
        function updateStats() {
            document.getElementById('birdCases').textContent = 
                fluData.humanCases.birdFlu.reduce((sum, loc) => sum + loc.cases, 0);
            document.getElementById('swineCases').textContent = 
                fluData.humanCases.swineFlu.reduce((sum, loc) => sum + loc.cases, 0);
            document.getElementById('animalCases').textContent = 
                fluData.animalCases.birdFlu.reduce((sum, loc) => sum + loc.cases, 0) +
                fluData.animalCases.swineFlu.reduce((sum, loc) => sum + loc.cases, 0);
            document.getElementById('highRisk').textContent = fluData.highRiskAreas.length;
        }

        // Format date
        function formatDate(date) {
            return date.toLocaleDateString('en-US', {
                year: 'numeric',
                month: 'long',
                day: 'numeric',
                hour: '2-digit',
                minute: '2-digit'
            });
        }

        // Toggle mobile menu
        function toggleMobileMenu() {
            const mobileNav = document.getElementById('mobileNav');
            mobileNav.classList.toggle('w3-show');
        }

        // Get user location
        function getUserLocation() {
            if (!navigator.geolocation) {
                showMessage("Geolocation is not supported by your browser", 'error');
                return;
            }
            
            showMessage("<i class='fa fa-spinner fa-spin'></i> Detecting your location...", 'loading');
            
            navigator.geolocation.getCurrentPosition(
                position => {
                    const { latitude: lat, longitude: lng } = position.coords;
                    addUserMarker(lat, lng, "Your Location");
                    assessRisk(lat, lng);
                },
                error => {
                    showMessage(`Error: ${error.message}`, 'error');
                },
                { enableHighAccuracy: true }
            );
        }

        // Add user marker
        function addUserMarker(lat, lng, title) {
            if (state.userMarker) {
                state.map.removeLayer(state.userMarker);
            }
            
            state.userMarker = L.marker([lat, lng], {
                icon: L.divIcon({
                    className: 'user-marker',
                    html: '<i class="fa fa-user"></i>',
                    iconSize: [30, 30]
                }),
                zIndexOffset: 1000
            }).addTo(state.map)
              .bindPopup(`<b>${title}</b><br>Coordinates: ${lat.toFixed(4)}, ${lng.toFixed(4)}`)
              .openPopup();
            
            state.map.setView([lat, lng], 8);
        }

        // Search location
        async function searchLocation() {
            const location = document.getElementById('locationInput').value.trim();
            if (!location) return;
            
            showMessage("<i class='fa fa-spinner fa-spin'></i> Searching location...", 'loading');
            
            try {
                const coords = await geocodeLocation(location);
                if (coords) {
                    addUserMarker(coords.lat, coords.lng, location);
                    assessRisk(coords.lat, coords.lng);
                } else {
                    showMessage("Location not found", 'error');
                }
            } catch (error) {
                console.error("Geocoding error:", error);
                showMessage("Error searching location", 'error');
            }
        }

        // Geocode location
        async function geocodeLocation(query) {
            const response = await fetch(
                `https://nominatim.openstreetmap.org/search?format=json&q=${encodeURIComponent(query)}`
            );
            const data = await response.json();
            return data.length > 0 ? { 
                lat: parseFloat(data[0].lat), 
                lng: parseFloat(data[0].lon) 
            } : null;
        }

        // Assess risk (now backed by the C++ server)
        async function assessRisk(lat, lng) {
            try {
                const response = await fetch(`/api/risk?lat=${lat}&lng=${lng}`);
                const risks = await response.json();
                displayRiskResults(risks.bird, risks.swine);
            } catch (error) {
                console.error("Risk assessment error:", error);
                showMessage("Error assessing risk", 'error');
            }
        }

        // Display risk results
        function displayRiskResults(birdRisk, swineRisk) {
            document.getElementById('riskAssessment').innerHTML = `
                <div class="risk-panel risk-${birdRisk.level}">
                    <h4><i class="fa fa-dove"></i> Bird Flu Risk: ${birdRisk.level.toUpperCase()}</h4>
                    <p>Nearest outbreak: ${birdRisk.distance.toFixed(1)} km (${birdRisk.cases} cases)</p>
                    <p><i class="fa fa-lightbulb"></i> ${birdRisk.advice}</p>
                </div>
                <div class="risk-panel risk-${swineRisk.level}">
                    <h4><i class="fa fa-piggy-bank"></i> Swine Flu Risk: ${swineRisk.level.toUpperCase()}</h4>
                    <p>Nearest outbreak: ${swineRisk.distance.toFixed(1)} km (${swineRisk.cases} cases)</p>
                    <p><i class="fa fa-lightbulb"></i> ${swineRisk.advice}</p>
                </div>
            `;
        }

        // Show message
        function showMessage(message, type) {
            const element = document.getElementById('riskAssessment');
            element.innerHTML = `<p class="w3-center ${type === 'error' ? 'w3-text-red' : ''}">${message}</p>`;
        }
    </script>
```

This drops `SAFE_DISTANCES`, the hardcoded `fluData` object, `calculateDistance`, `getRiskLevel`, `getRiskAdvice`, and the `subscribeAlerts` function (and its form's `onsubmit` no longer has a handler to call — see Step 2) — all now either served by the backend or removed as dead/non-functional code.

- [ ] **Step 2: Remove the now-unused alert subscription form**

The "Subscribe to alerts" panel's form calls `subscribeAlerts(event)`, which no longer exists after Step 1. Find this block in `web/index.html` (inside the Resources section) and delete it entirely:

```html
<div class="w3-panel w3-light-grey w3-padding-16 w3-margin-top">
<h3><i class="fa fa-bell"></i> Alerts</h3>
<p>Subscribe to receive outbreak notifications</p>
<form id="alertForm" onsubmit="return subscribeAlerts(event)">
<div class="w3-row">
<div class="w3-col s8">
<input class="w3-input w3-border" id="alertEmail" placeholder="Enter your email" required="" type="email"/>
</div>
<div class="w3-col s4">
<button class="w3-button w3-teal w3-block" type="submit">
<i class="fa fa-envelope"></i> Subscribe
                        </button>
</div>
</div>
</form>
<div class="w3-panel w3-round" id="subscriptionMessage" style="display:none; margin-top:10px;"></div>
</div>
```

- [ ] **Step 3: Rebuild**

Run: `cmake --build build --config Release`
This re-copies the updated `web/` folder next to the executable (no C++ source changed).

- [ ] **Step 4: Verify the served page no longer has the hardcoded dataset, and the app still works end-to-end**

```powershell
$exe = (Get-ChildItem -Path build -Recurse -Filter "bird_flu_tracker.exe").FullName
$proc = Start-Process -FilePath $exe -PassThru
Start-Sleep -Seconds 1

$page = (Invoke-WebRequest "http://localhost:8080/" -UseBasicParsing).Content
$page -match "const fluData = \{"     # expect False (old hardcoded object is gone)
$page -match "Global Flu Tracker"     # expect True (page still renders)

$risk = (Invoke-WebRequest "http://localhost:8080/api/risk?lat=37.1&lng=-95.7" -UseBasicParsing).Content | ConvertFrom-Json
$risk.bird.cases    # expect 58 (nearest bird-flu case to that point is the USA human-case entry)

Stop-Process -Id $proc.Id
```
Expected: the marked values match. Then open `http://localhost:8080/` in an actual browser once and confirm the map loads with markers, the Human/Animal toggle works, and entering coordinates via "Current Location" or a searched city produces a risk panel.

- [ ] **Step 5: Commit**

```bash
git add web/index.html
git commit -m "Rewire frontend to fetch data and risk assessment from the C++ backend"
```

---

### Task 6: Update README with build/run instructions

**Files:**
- Modify: `README.md`

**Interfaces:** none — documentation only.

- [ ] **Step 1: Append build/run instructions to `README.md`**

Add to the end of the existing `README.md`:

```markdown

## Building and running

Requires CMake 3.16+ and a C++17 compiler.

```
cmake -S . -B build
cmake --build build --config Release
```

Run the built `bird_flu_tracker` executable (under `build/backend/` or
`build/backend/Release/`, depending on your generator). It starts a local
server on `http://localhost:8080` and opens it in your default browser
automatically — no other setup required.

## Architecture

The outbreak dataset and risk-assessment logic (distance calculation, risk
level, and advice) are implemented in C++ (`backend/flu_data.cpp`,
`backend/risk.cpp`) and served over a small local HTTP API
(`backend/main.cpp`, via [cpp-httplib](https://github.com/yhirose/cpp-httplib)).
The frontend (`web/index.html`) is the original Leaflet-based map UI,
fetching data from that API instead of using hardcoded values.
```

- [ ] **Step 2: Commit**

```bash
git add README.md
git commit -m "Document build/run instructions and architecture"
```

---

### Task 7: Push to GitHub

**Files:** none — this task only pushes existing commits.

**Interfaces:** none.

- [ ] **Step 1: Push to `origin/main`**

```bash
git push origin main
```
Expected: push succeeds; `https://github.com/AkshathCodes/Bird-Flu-Tracker` shows the new commits on its `main` branch.

- [ ] **Step 2: Verify on GitHub**

```bash
gh repo view AkshathCodes/Bird-Flu-Tracker --web
```
(or just note the URL) — confirm `backend/`, `web/`, `CMakeLists.txt`, and the updated `README.md` are visible on GitHub.
