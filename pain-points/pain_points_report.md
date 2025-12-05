# Developer Pain Point Report

## 🚨 Top High Risk Functions (Complex + Untested)

- **[`ottr::parse_istream()`](https://github.com/ruxven/ottr/blob/cf7de39acaea322dcb10f053e5c8787076f945ab/src/parser.cpp#L62-L167)** in `src/parser.cpp`
  - Score: **10029**
  - Complexity: **43** (above threshold value: 15)
  - Branch Coverage: **71.4%** (below threshold value: 80)
  - Line Coverage: **95.9%**)
  - Critical findings: ``

- **[`ottr::allocate_weighted()`](https://github.com/ruxven/ottr/blob/cf7de39acaea322dcb10f053e5c8787076f945ab/src/alloc.cpp#L13-L102)** in `src/alloc.cpp`
  - Score: **5922**
  - Complexity: **26** (above threshold value: 15)
  - Branch Coverage: **78.0%** (below threshold value: 80)
  - Line Coverage: **100.0%**)
  - Critical findings: `cplusplus.NewDeleteLeaks, cplusplus.NewDeleteLeaks`

- **[`ottr::process_world()`](https://github.com/ruxven/ottr/blob/cf7de39acaea322dcb10f053e5c8787076f945ab/src/engine.cpp#L20-L100)** in `src/engine.cpp`
  - Score: **3825**
  - Complexity: **17** (above threshold value: 15)
  - Branch Coverage: **75.0%** (below threshold value: 80)
  - Line Coverage: **85.3%**)
  - Critical findings: `clang-diagnostic-unused-parameter`

## 🔒 Top Security Risk Functions (Prioritized by coverage)

- **[`ottr::allocate_weighted()`](https://github.com/ruxven/ottr/blob/cf7de39acaea322dcb10f053e5c8787076f945ab/src/alloc.cpp#L13-L102)** in `src/alloc.cpp`
  - Score: **20078**
  - Branch Coverage: **78.0%**
  - Line Coverage: **100.0%**
  - Critical findings: `cplusplus.NewDeleteLeaks, cplusplus.NewDeleteLeaks`

- **[`ottr::process_world()`](https://github.com/ruxven/ottr/blob/cf7de39acaea322dcb10f053e5c8787076f945ab/src/engine.cpp#L20-L100)** in `src/engine.cpp`
  - Score: **10075**
  - Branch Coverage: **75.0%**
  - Line Coverage: **85.3%**
  - Critical findings: `clang-diagnostic-unused-parameter`

