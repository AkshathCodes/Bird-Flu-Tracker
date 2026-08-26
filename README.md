# Bird-Flu-Tracker
An ai bird flu tracker that visualizes bird flu migration across the globe and provide feedback and advice using flight path statistics and high risk zones

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
