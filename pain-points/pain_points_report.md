# Developer Pain Point Report

## 🚨 Top High Risk Functions (Complex + Untested)

- **[`ottr::allocate_weighted()`](https://github.com/ruxven/ottr/blob/5c299f13433c4d89958b44a62b8e3d164f280b65/src/alloc.cpp#L13-L165)** in `src/alloc.cpp`
  - Score: **10021**
  - Complexity: **45** (above threshold value: 15)
  - Branch Coverage: **79.8%** (below threshold value: 80)
  - Line Coverage: **95.0%**)
  - Critical findings: ``

- **[`ottr::process_world()`](https://github.com/ruxven/ottr/blob/5c299f13433c4d89958b44a62b8e3d164f280b65/src/engine.cpp#L25-L195)** in `src/engine.cpp`
  - Score: **8124**
  - Complexity: **37** (above threshold value: 15)
  - Branch Coverage: **76.4%** (below threshold value: 80)
  - Line Coverage: **89.1%**)
  - Critical findings: `clang-diagnostic-unused-parameter`

- **[`main()`](https://github.com/ruxven/ottr/blob/5c299f13433c4d89958b44a62b8e3d164f280b65/src/main.cpp#L20-L97)** in `src/main.cpp`
  - Score: **3700**
  - Complexity: **17** (above threshold value: 15)
  - Branch Coverage: **0.0%** (below threshold value: 80)
  - Line Coverage: **0.0%** (below threshold value: 80)
  - Critical findings: ``

## 🔒 Top Security Risk Functions (Prioritized by coverage)

- **[`ottr::CalculationOptions::validate()`](https://github.com/ruxven/ottr/blob/5c299f13433c4d89958b44a62b8e3d164f280b65/src/options.cpp#L5-L8)** in `src/options.cpp`
  - Score: **10100**
  - Branch Coverage: **0.0%**
  - Line Coverage: **100.0%**
  - Critical findings: `clang-diagnostic-unused-parameter`

- **[`ottr::process_world()`](https://github.com/ruxven/ottr/blob/5c299f13433c4d89958b44a62b8e3d164f280b65/src/engine.cpp#L25-L195)** in `src/engine.cpp`
  - Score: **10076**
  - Branch Coverage: **76.4%**
  - Line Coverage: **89.1%**
  - Critical findings: `clang-diagnostic-unused-parameter`

