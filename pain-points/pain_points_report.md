# Developer Pain Point Report

## 🚨 High Risk Functions (Complex + Untested)

- **[`ottr::parse_istream()`](https://github.com/ruxven/ottr/blob/e2e1f7ff4d98441992ea127dc818616e6ffac0f4/src/parser.cpp#L62-L167)** in `src/parser.cpp`
  - Complexity: **43** (above threshold value: 15)
  - Branch Coverage: **71.4%** (below threshold value: 80)

- **[`ottr::allocate_weighted()`](https://github.com/ruxven/ottr/blob/e2e1f7ff4d98441992ea127dc818616e6ffac0f4/src/alloc.cpp#L13-L96)** in `src/alloc.cpp`
  - Complexity: **26** (above threshold value: 15)
  - Branch Coverage: **78.0%** (below threshold value: 80)

- **[`ottr::process_world()`](https://github.com/ruxven/ottr/blob/e2e1f7ff4d98441992ea127dc818616e6ffac0f4/src/engine.cpp#L20-L100)** in `src/engine.cpp`
  - Complexity: **17** (above threshold value: 15)
  - Branch Coverage: **75.0%** (below threshold value: 80)

- **[`ottr::validate_world()`](https://github.com/ruxven/ottr/blob/e2e1f7ff4d98441992ea127dc818616e6ffac0f4/src/validate.cpp#L12-L50)** in `src/validate.cpp`
  - Complexity: **15** (above threshold value: 15)
  - Branch Coverage: **79.2%** (below threshold value: 80)

## 🔒 Security Risk Functions (Tested but Vulnerable)

None detected!

