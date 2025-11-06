# Developer Pain Point Report

## 🚨 Top High Risk Functions (Complex + Untested)

- **[`ottr::parse_istream()`](https://github.com/ruxven/ottr/blob/111dda2588de5b7621d1c45621545d5716ad8630/src/parser.cpp#L62-L167)** in `src/parser.cpp`
  - Score: **10029**
  - Complexity: **43** (above threshold value: 15)
  - Branch Coverage: **71.4%** (below threshold value: 80)
  - Line Coverage: **95.9%**)
  - Critical findings: ``

- **[`ottr::allocate_weighted()`](https://github.com/ruxven/ottr/blob/111dda2588de5b7621d1c45621545d5716ad8630/src/alloc.cpp#L13-L96)** in `src/alloc.cpp`
  - Score: **5922**
  - Complexity: **26** (above threshold value: 15)
  - Branch Coverage: **78.0%** (below threshold value: 80)
  - Line Coverage: **100.0%**)
  - Critical findings: ``

- **[`ottr::process_world()`](https://github.com/ruxven/ottr/blob/111dda2588de5b7621d1c45621545d5716ad8630/src/engine.cpp#L20-L100)** in `src/engine.cpp`
  - Score: **3825**
  - Complexity: **17** (above threshold value: 15)
  - Branch Coverage: **75.0%** (below threshold value: 80)
  - Line Coverage: **85.3%**)
  - Critical findings: ``

## 🔒 Top Security Risk Functions (Prioritized by coverage)

- **[`main()`](https://github.com/ruxven/ottr/blob/111dda2588de5b7621d1c45621545d5716ad8630/src/main.cpp#L20-L75)** in `src/main.cpp`
  - Score: **1000**
  - Branch Coverage: **0.0%**
  - Line Coverage: **0.0%**
  - Critical findings: `cpp.lang.security.system-command.dont-call-system.dont-call-system`

