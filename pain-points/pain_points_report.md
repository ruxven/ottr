# Developer Pain Point Report

## 🚨 High Risk Functions (Complex + Untested)

- **[`ottr::parse_istream()`](https://github.com/ruxven/ottr/blob/9b563631e6a7ea893f2cba9288460b66f6318d98/src/parser.cpp#L62-L167)** in `src/parser.cpp`
  - Complexity: **43** (high)
  - Coverage: **0.0%** (low)

- **[`ottr::allocate_weighted()`](https://github.com/ruxven/ottr/blob/9b563631e6a7ea893f2cba9288460b66f6318d98/src/alloc.cpp#L13-L96)** in `src/alloc.cpp`
  - Complexity: **26** (high)
  - Coverage: **0.0%** (low)

- **[`ottr::process_world()`](https://github.com/ruxven/ottr/blob/9b563631e6a7ea893f2cba9288460b66f6318d98/src/engine.cpp#L20-L100)** in `src/engine.cpp`
  - Complexity: **17** (high)
  - Coverage: **0.0%** (low)

- **[`ottr::tokenize_line()`](https://github.com/ruxven/ottr/blob/9b563631e6a7ea893f2cba9288460b66f6318d98/src/parser.cpp#L11-L53)** in `src/parser.cpp`
  - Complexity: **16** (high)
  - Coverage: **0.0%** (low)

- **[`ottr::validate_world()`](https://github.com/ruxven/ottr/blob/9b563631e6a7ea893f2cba9288460b66f6318d98/src/validate.cpp#L12-L50)** in `src/validate.cpp`
  - Complexity: **15** (high)
  - Coverage: **0.0%** (low)

## 🔒 Security Risk Functions (Tested but Vulnerable)

None detected!

