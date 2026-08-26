# Bird Flu Tracker — C++ Backend Design

## Overview

The existing `index.html` (a single-page site with a Leaflet map, hardcoded
outbreak data, and client-side risk-assessment JS) gets recoded so the core
logic — the outbreak dataset, distance calculation, and risk assessment —
lives in a C++ HTTP server instead of inline JavaScript. The frontend keeps
its Leaflet map, layout, and styling essentially unchanged; it fetches data
from the C++ backend instead of reading a hardcoded JS object.

The result ships as a folder containing a Windows executable and a `web/`
directory. Running the executable starts a local server and opens the
default browser to it — no install step, no separate frontend server.

## Goals

- Port the outbreak dataset (human/animal cases for bird flu and swine flu,
  high-risk areas) from the JS object into C++ data structures.
- Port the risk-assessment logic (Haversine distance, risk level, risk
  advice) from JS functions into C++.
- Serve both the static frontend and the data via one C++ executable.
- Preserve the existing UI/UX (map, markers, toggles, risk panel, dark mode,
  prevention/data/resources sections) with no visible behavior change from
  the user's perspective, aside from data now being fetched instead of
  inline.

## Non-goals

- No native GUI toolkit (Dear ImGui, Qt, etc.) — rejected in favor of
  reusing the existing HTML/CSS/Leaflet frontend, which is both less code
  and higher fidelity to the original design.
- No change to city-name geocoding — it keeps calling the Nominatim API
  directly from browser JS, exactly as the original page does. The backend
  is not involved in that request.
- No real email subscription backend — the original "Subscribe to alerts"
  feature was already fake (a `setTimeout` with no network call), so it is
  dropped rather than ported.
- No cross-platform installer/package — a single build target (Windows),
  with a small cross-platform browser-launch snippet as a low-cost nicety,
  not a portability guarantee for Mac/Linux.
- No embedding of static assets into the executable. The exe reads `web/`
  from a path relative to itself; the two ship together as a folder.

## Architecture

```
Bird-Flu-Tracker/
  backend/
    main.cpp         Server setup, static file mount, route handlers, browser launch
    flu_data.h        Struct definitions for case/outbreak records
    flu_data.cpp       The ported dataset + to-JSON serialization
    risk.h             Haversine distance + risk-level + risk-advice declarations
    risk.cpp            Their implementations (ported from the JS versions)
    tests.cpp           Standalone assertions (<cassert>) checking risk.cpp
                         output against known values from the original JS
    CMakeLists.txt     FetchContent for cpp-httplib + nlohmann/json
  web/
    index.html         Existing page, with the hardcoded fluData object
                        removed; JS fetches it from the backend instead
  CMakeLists.txt        Top-level, adds backend/
```

### Request flow

1. User runs `bird-flu-tracker.exe`.
2. The process starts an `httplib::Server`, mounts `web/` as static root
   (`/` → `web/index.html`), registers the two API routes below, starts
   listening on `localhost:<port>` (fixed port, e.g. 8080), then launches
   the OS default browser pointed at that URL.
3. Browser loads `index.html`; on `DOMContentLoaded` it calls
   `GET /api/flu-data` once, replacing the current `initApp()` read of the
   hardcoded `fluData` object.
4. When the user requests a risk assessment (current location button, or
   after a Nominatim geocode result), the frontend calls
   `GET /api/risk?lat=..&lng=..` instead of running `assessRisk()` locally.

### API contract

**`GET /api/flu-data`**

Returns the full dataset as JSON, matching the shape of the current JS
`fluData` object exactly (`humanCases.birdFlu`, `humanCases.swineFlu`,
`animalCases.birdFlu`, `animalCases.swineFlu`, `highRiskAreas`), so the
existing map-rendering JS (`addHumanCases`, `addAnimalCases`,
`addHighRiskAreas`, `updateStats`) needs only its data source changed, not
its logic.

**`GET /api/risk?lat=<float>&lng=<float>`**

Returns:
```json
{
  "bird":  { "distance": 123.4, "cases": 16, "level": "medium", "advice": "..." },
  "swine": { "distance": 456.7, "cases": 8,  "level": "low",    "advice": "..." }
}
```
`distance` in km, `level` one of `high|medium|low|none`, `advice` the same
text the JS `getRiskAdvice()` currently produces. Missing/non-numeric
`lat`/`lng` → 400 with a JSON error body; the frontend already has an error
path (`showMessage(..., 'error')`) to route this into.

## Data ownership

`flu_data.cpp` is the single source of truth for the dataset — it replaces
the JS `fluData` literal. The JS file no longer declares `fluData` at all;
`state` and the rendering functions consume whatever `GET /api/flu-data`
returns.

## Testing

`backend/tests.cpp` is a small standalone program (no test framework
dependency) with `<cassert>` checks:
- Haversine distance between two known points matches the value the
  original JS `calculateDistance()` produces for the same inputs.
- `getRiskLevel()` boundary behavior (ratios at/near 0.3, 0.7, 1.0) matches
  the JS thresholds.
- `/api/flu-data` sums (bird/swine/animal case totals, high-risk count)
  match the totals currently shown in the page's Quick Stats bar.

Run as part of the build; a failing assertion aborts the test binary with a
nonzero exit code.

## Manual verification

- `cmake --build` succeeds cleanly.
- Running the exe opens a browser tab showing the same page layout as the
  current `index.html`.
- Toggling Human/Animal cases, the prediction animation, dark mode, and the
  location risk panel all behave the same as before, now backed by fetched
  data instead of inline data.
