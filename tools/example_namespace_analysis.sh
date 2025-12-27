#!/bin/bash
# Example usage of the ThemisDB Namespace Analyzer

# Script directory
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"

echo "==================================="
echo "ThemisDB Namespace Analyzer Examples"
echo "==================================="
echo ""

# Example 1: Basic analysis (Markdown only)
echo "Example 1: Basic analysis (Markdown only)"
echo "Command: python3 tools/namespace_analyzer.py --format markdown"
echo ""
python3 "$SCRIPT_DIR/namespace_analyzer.py" --format markdown --output-dir "$REPO_ROOT/namespace_analysis"
echo ""

# Example 2: Full analysis with all formats
echo "Example 2: Full analysis with all formats"
echo "Command: python3 tools/namespace_analyzer.py --format all"
echo ""
# python3 "$SCRIPT_DIR/namespace_analyzer.py" --format all --output-dir "$REPO_ROOT/namespace_analysis"
echo "[Skipped - uncomment to run]"
echo ""

# Example 3: Analysis with Git metadata (slower but more detailed)
echo "Example 3: Analysis with Git metadata"
echo "Command: python3 tools/namespace_analyzer.py --include-git --format csv"
echo ""
# python3 "$SCRIPT_DIR/namespace_analyzer.py" --include-git --format csv --output-dir "$REPO_ROOT/namespace_analysis_with_git"
echo "[Skipped - uncomment to run, takes longer due to git blame]"
echo ""

# Example 4: JSON output for automated processing
echo "Example 4: JSON output for automated processing"
echo "Command: python3 tools/namespace_analyzer.py --format json"
echo ""
# python3 "$SCRIPT_DIR/namespace_analyzer.py" --format json --output-dir "$REPO_ROOT/namespace_analysis"
echo "[Skipped - uncomment to run]"
echo ""

echo "==================================="
echo "Analysis complete!"
echo "Results are in: $REPO_ROOT/namespace_analysis/"
echo "==================================="
echo ""
echo "Generated files:"
ls -lh "$REPO_ROOT/namespace_analysis/" 2>/dev/null || echo "No files generated yet."
