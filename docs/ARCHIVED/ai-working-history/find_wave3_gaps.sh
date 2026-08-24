#!/bin/bash
# Find Wave 3 gaps - complex control flow move patterns

echo "WAVE 3 GAP DISCOVERY - Searching for complex control flow patterns"
echo "=" 80

# Search in target modules for moved-from patterns with context
# Pattern: std::move followed by variable access (not immediate cleanup)

TARGET_DIRS="sharding storage training gpu analytics"

for dir in $TARGET_DIRS; do
    echo ""
    echo "Scanning src/$dir/ for move patterns..."
    grep -r "std::move" src/$dir --include="*.cpp" | grep -v "\.clear()" | head -20
done

echo ""
echo "Now checking specific files identified in Gap Report..."

# Check specific locations from Gap Report
FILES=(
    "src/sharding/cross_shard_transaction.cpp:3472"
    "src/storage/wom_tree.cpp:408"
)

for file_loc in "${FILES[@]}"; do
    IFS=: read -r file line <<< "$file_loc"
    echo ""
    echo "=== $file:$line ==="
    if [ -f "$file" ]; then
        # Show context around the line
        sed -n "$((line-5)),$((line+5))p" "$file"
    fi
done
