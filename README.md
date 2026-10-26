# Global Flu Tracker

**A C++17 geospatial prototype with a local HTTP API and an interactive Leaflet map.**

Explore a fixed bird- and swine-flu dataset, select locations, and inspect distance-based scores. The project began as a HackAppleton team prototype; this repository includes a C++ backend, a CMake build, and backend tests.

## What the implementation demonstrates

- **C++ core logic:** great-circle distance using the Haversine formula, nearest-location searches, and threshold-based scoring.
- **HTTP/JSON integration:** a local server exposes the dataset and location-based calculations to the browser.
- **Separation of concerns:** data and scoring logic live in a reusable `flu_core` library, separate from HTTP handling and the frontend.
- **Verification:** backend tests cover data totals, JSON serialization, known distances, threshold boundaries, advice strings, and a sample location assessment.

## Scope and limitations

The dataset is compiled into the program in [`backend/flu_data.cpp`](backend/flu_data.cpp). It is not a live surveillance feed, and the repository does not establish provenance or freshness for its case counts.

Scores use distance to the nearest stored location. The frontend's "AI Pandemic Simulation" is a scripted visualization, not a trained model or validated epidemiological forecast. Scores and generated advice are demonstration outputs rather than medical guidance.

## Build and run

Requires **CMake 3.16+**, a **C++17 compiler**, and **Git**. Initial configuration needs internet access because CMake fetches [cpp-httplib](https://github.com/yhirose/cpp-httplib) and [nlohmann/json](https://github.com/nlohmann/json).

```sh
git clone https://github.com/AkshathCodes/Bird-Flu-Tracker.git
cd Bird-Flu-Tracker
cmake -S . -B build
cmake --build build --config Release
```

Run the executable that matches your generator:

| Environment | Command |
| --- | --- |
| Windows, Visual Studio Release | `.\build\backend\Release\bird_flu_tracker.exe` |
| Linux/macOS, single-configuration generator | `./build/backend/bird_flu_tracker` |

The server listens on `http://localhost:8080` and attempts to open the page in your default browser. Open that address manually if needed. CMake copies `web/` beside the executable; keep that directory with the binary. Stop the server with Ctrl+C in its terminal.

The map, place search, styles, and images use external services, so the full browser interface also needs internet access.

## Architecture

```text
Browser: web/index.html + Leaflet
             |
             | HTTP / JSON on localhost:8080
             v
Server: backend/main.cpp (cpp-httplib)
             |
             v
flu_core: flu_data.cpp + risk.cpp
             |
             +-- Fixed dataset and JSON serialization
             +-- Haversine distance and scoring rules
```

| Component | Responsibility |
| --- | --- |
| [`backend/flu_data.cpp`](backend/flu_data.cpp) | Fixed human/animal records and JSON serialization |
| [`backend/risk.cpp`](backend/risk.cpp) | Great-circle distances, nearest locations, scores, and advice strings |
| [`backend/main.cpp`](backend/main.cpp) | Static frontend hosting and HTTP routes |
| [`backend/tests.cpp`](backend/tests.cpp) | Backend checks with explicit failure handling |
| [`web/index.html`](web/index.html) | Map layers, place search, location interaction, and scripted simulation |

## API

With the server running:

| Route | Result |
| --- | --- |
| `GET /api/flu-data` | Fixed human cases, animal cases, and high-risk-area records |
| `GET /api/risk?lat=12.6&lng=105.0` | Bird/swine results containing `distance`, `cases`, `level`, and `advice` |

Distance is measured in kilometers. Bird and swine scoring use base distances of 100 km and 50 km respectively. For distance divided by base distance, levels are `high` below 0.3, `medium` below 0.7, `low` below 1.0, and `none` otherwise. These are demonstration rules.

## Backend tests

The normal build includes `backend_tests` and runs it after that target is built. To run it again explicitly:

```powershell
# Windows, Visual Studio Release
.\build\backend\Release\backend_tests.exe
```

```sh
# Linux/macOS, single-configuration generator
./build/backend/backend_tests
```

On success, the executable prints `All tests passed`. A failed check reports its expression and exits with a nonzero status. This is a backend test suite; it does not establish browser coverage or validate the disease model.

## License

[MIT](LICENSE).
