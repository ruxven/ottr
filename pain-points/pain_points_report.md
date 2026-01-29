# Developer Pain Point Report

## 🚨 Top High Risk Functions (Complex + Untested)

- **[`ottr::allocate_weighted()`](https://github.com/ruxven/ottr/blob/32501aa61d24e76b60f0bd62b1b31a45f8424f9a/src/alloc.cpp#L13-L96)** in `src/alloc.cpp`
  - Score: **10022**
  - Complexity: **26** (above threshold value: 15)
  - Branch Coverage: **78.0%** (below threshold value: 80)
  - Line Coverage: **100.0%**)
  - Critical findings: ``

- **[`main()`](https://github.com/ruxven/ottr/blob/32501aa61d24e76b60f0bd62b1b31a45f8424f9a/src/main.cpp#L20-L95)** in `src/main.cpp`
  - Score: **6500**
  - Complexity: **17** (above threshold value: 15)
  - Branch Coverage: **0.0%** (below threshold value: 80)
  - Line Coverage: **0.0%** (below threshold value: 80)
  - Critical findings: ``

- **[`ottr::process_world()`](https://github.com/ruxven/ottr/blob/32501aa61d24e76b60f0bd62b1b31a45f8424f9a/src/engine.cpp#L20-L100)** in `src/engine.cpp`
  - Score: **6425**
  - Complexity: **17** (above threshold value: 15)
  - Branch Coverage: **75.0%** (below threshold value: 80)
  - Line Coverage: **85.3%**)
  - Critical findings: `clang-diagnostic-unused-parameter`

## 🔒 Top Security Risk Functions (Prioritized by coverage)

- **[`ottr::process_world()`](https://github.com/ruxven/ottr/blob/32501aa61d24e76b60f0bd62b1b31a45f8424f9a/src/engine.cpp#L20-L100)** in `src/engine.cpp`
  - Score: **10075**
  - Branch Coverage: **75.0%**
  - Line Coverage: **85.3%**
  - Critical findings: `clang-diagnostic-unused-parameter`

