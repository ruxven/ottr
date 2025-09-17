# Code Complexity Analysis

This project includes automated code complexity analysis using [Lizard](https://github.com/terryyin/lizard), a tool that analyzes cyclomatic complexity, lines of code, and other maintainability metrics.

## Configuration

The complexity analysis is configured via the `.lizard` file in the project root. Key settings include:

- **Complexity Threshold**: Functions with cyclomatic complexity > 15 are flagged
- **Length Threshold**: Functions with > 100 lines are flagged  
- **Arguments Threshold**: Functions with > 5 parameters are flagged
- **File Extensions**: Analyzes `.cpp`, `.hpp`, `.c`, `.h` files
- **Exclusions**: Skips `build/`, `tests/`, and third-party directories

## Reports Generated

The CI/CD pipeline generates several complexity reports:

1. **HTML Report** (`complexity.html`) - Interactive web-based report
2. **Text Report** (`complexity.txt`) - Detailed text output
3. **CSV Report** (`complexity.csv`) - Data for spreadsheet analysis
4. **XML Report** (`complexity.xml`) - Machine-readable format
5. **Summary Report** (`summary.txt`) - Key statistics and trends

## Accessing Reports

- **GitHub Pages**: Visit the project's GitHub Pages site for the latest reports
- **CI Artifacts**: Download reports from the GitHub Actions artifacts
- **Local Analysis**: Run `lizard src/` locally for immediate feedback

## Complexity Guidelines

- **Low Complexity (1-5)**: Simple, easy to test and maintain
- **Moderate Complexity (6-10)**: Acceptable, monitor for growth
- **High Complexity (11-15)**: Consider refactoring for better maintainability
- **Very High Complexity (>15)**: Should be refactored to reduce complexity

## Local Usage

To run complexity analysis locally:

```bash
# Install Lizard
pip install lizard

# Run basic analysis
lizard src/

# Use project configuration
lizard src/ --config .lizard

# Generate HTML report
lizard src/ -H -o complexity_report.html
```

## Integration with Development Workflow

The complexity analysis is integrated into the CI/CD pipeline and:

- Runs on every push and pull request
- Generates reports accessible via GitHub Pages
- Provides warnings for functions exceeding thresholds
- Creates complexity badges for the README
- Fails builds only on critical issues (configurable)

This helps maintain code quality and identifies areas that may benefit from refactoring.