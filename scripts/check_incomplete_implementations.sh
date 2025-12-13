#!/bin/bash
# Script to identify incomplete implementations and missing CMake entries
# Usage: ./scripts/check_incomplete_implementations.sh

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(dirname "$SCRIPT_DIR")"

echo "==================================================================="
echo "ThemisDB Incomplete Implementations & CMake Coverage Check"
echo "==================================================================="
echo ""

cd "$REPO_ROOT"

# Create temporary files for comparison
ALL_CPP_FILES=$(mktemp)
CMAKE_CPP_FILES=$(mktemp)

# Cleanup on exit
trap "rm -f $ALL_CPP_FILES $CMAKE_CPP_FILES" EXIT

# Find all .cpp files in src/
find src -name "*.cpp" -type f | sort > "$ALL_CPP_FILES"

# Extract .cpp files referenced in CMakeLists.txt
grep -h "src/.*\.cpp" CMakeLists.txt | sed 's/^[[:space:]]*//' | grep "^src/" | sort | uniq > "$CMAKE_CPP_FILES"

# Count files
TOTAL_CPP=$(wc -l < "$ALL_CPP_FILES")
CMAKE_CPP=$(wc -l < "$CMAKE_CPP_FILES")
MISSING_CPP=$(comm -23 "$ALL_CPP_FILES" "$CMAKE_CPP_FILES" | wc -l)

echo "📊 Summary:"
echo "  Total .cpp files in src/: $TOTAL_CPP"
echo "  Files in CMakeLists.txt:  $CMAKE_CPP"
echo "  Missing from build:       $MISSING_CPP"
echo ""

if [ $MISSING_CPP -gt 0 ]; then
    echo "⚠️  Files NOT in CMakeLists.txt:"
    echo "-----------------------------------------------"
    comm -23 "$ALL_CPP_FILES" "$CMAKE_CPP_FILES"
    echo ""
fi

# Find stub implementations
echo "🔍 Stub Implementations:"
echo "-----------------------------------------------"
echo "Files with 'stub' return statements:"
grep -r "return false.*[Ss]tub\|return {}.*[Ss]tub\|return nullptr.*[Ss]tub" src --include="*.cpp" -l | sort | uniq
echo ""

# Find TODO/FIXME markers
TODO_COUNT=$(grep -r "TODO\|FIXME" src --include="*.cpp" --include="*.h" 2>/dev/null | wc -l || echo 0)
echo "📝 TODO/FIXME Markers: $TODO_COUNT"
if [ $TODO_COUNT -gt 0 ]; then
    echo "Files with TODO/FIXME (showing top 20):"
    grep -r "TODO\|FIXME" src --include="*.cpp" --include="*.h" -l 2>/dev/null | sort | uniq | head -20
fi
echo ""

# Find "not implemented" errors
echo "❌ 'Not Implemented' Errors:"
echo "-----------------------------------------------"
grep -ri "not implemented\|not yet implemented" src --include="*.cpp" -l 2>/dev/null | sort | uniq || echo "  None found"
echo ""

# Find minimal implementations (< 20 lines)
echo "📄 Minimal Implementations (< 20 lines):"
echo "-----------------------------------------------"
find src -name "*.cpp" -type f -exec sh -c 'lines=$(wc -l < "$1"); if [ $lines -lt 20 ]; then printf "%-50s (%3d lines)\n" "$1" "$lines"; fi' _ {} \; | sort

echo ""
echo "==================================================================="
echo "✅ Check complete!"
echo "==================================================================="
