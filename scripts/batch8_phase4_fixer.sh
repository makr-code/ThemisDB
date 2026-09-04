#!/bin/bash
# Batch 8 Phase 4: Automated -Wsign-compare Fixer
# Stage 1: Loop counter conversions (Category A)
# Target: Convert "int i" to "size_t i" in for loops comparing against .size()

set -e

REPO_ROOT="/home/runner/work/ThemisDB/ThemisDB"
CATEGORY="$1"
DRY_RUN="${2:-false}"

if [ -z "$CATEGORY" ]; then
    echo "Usage: $0 <category_a|category_b|category_c|category_d> [dry_run]"
    exit 1
fi

# Find all C++ source files (excluding build, cache, git)
find_cpp_files() {
    find "$REPO_ROOT" \( -name "*.cpp" -o -name "*.cc" -o -name "*.h" -o -name "*.hpp" \) \
        ! -path "*/build*" ! -path "*/.cache*" ! -path "*/.git*" ! -path "*/vcpkg*" | sort
}

# Category A: Convert loop counters from int to size_t
fix_category_a() {
    local file="$1"
    local dry_run="$2"
    
    # Pattern: for (int i = 0; i < something.size()
    # This is the most common and safest pattern
    
    if ! grep -q "for\s*(\s*int\s\+[a-zA-Z_][a-zA-Z0-9_]*\s*=\s*0\s*;\s*[a-zA-Z_][a-zA-Z0-9_]*\s*<.*\.size()" "$file" 2>/dev/null; then
        return 0
    fi
    
    local count=$(grep -o "for\s*(\s*int\s\+[a-zA-Z_][a-zA-Z0-9_]*\s*=\s*0\s*;\s*[a-zA-Z_][a-zA-Z0-9_]*\s*<.*\.size()" "$file" 2>/dev/null | wc -l || echo 0)
    
    if [ "$count" -gt 0 ]; then
        echo "File: $file (found $count occurrences)"
        
        if [ "$dry_run" = "false" ]; then
            # Use sed to replace int to size_t in for loop declarations
            sed -i 's/for\s\+(\s*int\(\s\+[a-zA-Z_][a-zA-Z0-9_]*\s*=\s*0\s*;\s*[a-zA-Z_][a-zA-Z0-9_]*\s*<.*\.size()/for (size_t\1/' "$file"
        fi
        
        return "$count"
    fi
    
    return 0
}

# Category B: Remove unsigned literal suffixes (U, UL, ULL)
fix_category_b() {
    local file="$1"
    local dry_run="$2"
    
    # Count occurrences of unsigned suffixes
    local count_u=$(grep -o '\b[0-9]\+U\b' "$file" 2>/dev/null | wc -l || echo 0)
    local count_ul=$(grep -o '\b[0-9]\+UL\b' "$file" 2>/dev/null | wc -l || echo 0)
    local count_ull=$(grep -o '\b[0-9]\+ULL\b' "$file" 2>/dev/null | wc -l || echo 0)
    
    local total=$((count_u + count_ul + count_ull))
    
    if [ "$total" -gt 0 ]; then
        echo "File: $file (U=$count_u, UL=$count_ul, ULL=$count_ull)"
        
        if [ "$dry_run" = "false" ]; then
            # Remove U suffix but keep leading zeros as single digit
            sed -i 's/\b\([0-9]\+\)U\b/\1/g' "$file"
            sed -i 's/\b\([0-9]\+\)UL\b/\1/g' "$file"
            sed -i 's/\b\([0-9]\+\)ULL\b/\1/g' "$file"
        fi
        
        return "$total"
    fi
    
    return 0
}

# Category C: Add static_cast for direct int/size_t comparisons
# This is more complex and requires careful handling
fix_category_c() {
    local file="$1"
    local dry_run="$2"
    
    # Look for patterns like: if (x < vec.size())
    if ! grep -qE 'if\s*\(\s*[a-zA-Z_][a-zA-Z0-9_]*\s*[<>]=?\s*[a-zA-Z_][a-zA-Z0-9_]*\.size\(' "$file" 2>/dev/null; then
        return 0
    fi
    
    local count=$(grep -o -E 'if\s*\(\s*[a-zA-Z_][a-zA-Z0-9_]*\s*[<>]=?\s*[a-zA-Z_][a-zA-Z0-9_]*\.size\(' "$file" 2>/dev/null | wc -l || echo 0)
    
    if [ "$count" -gt 0 ]; then
        echo "File: $file (found $count potential casts needed)"
        # This requires careful validation - skipping automated fix
        return 0
    fi
    
    return 0
}

# Category D: Mixed-type arithmetic (requires validation)
fix_category_d() {
    local file="$1"
    local dry_run="$2"
    
    # This category requires more careful analysis
    # Placeholder for now
    return 0
}

# Main execution
echo "=== Batch 8 Phase 4: Sign-Compare Hardening ==="
echo "Category: $CATEGORY"
echo "Dry Run: $DRY_RUN"
echo ""

case "$CATEGORY" in
    category_a)
        echo "Processing Category A: Loop counter conversions"
        total_fixed=0
        while IFS= read -r file; do
            fix_category_a "$file" "$DRY_RUN" && ((total_fixed += $?)) || true
        done < <(find_cpp_files)
        echo "Total fixes in Category A: $total_fixed"
        ;;
    category_b)
        echo "Processing Category B: Unsigned literal suffix removal"
        total_fixed=0
        while IFS= read -r file; do
            fix_category_b "$file" "$DRY_RUN" && ((total_fixed += $?)) || true
        done < <(find_cpp_files)
        echo "Total fixes in Category B: $total_fixed"
        ;;
    category_c)
        echo "Processing Category C: Static cast additions"
        total_fixed=0
        while IFS= read -r file; do
            fix_category_c "$file" "$DRY_RUN" && ((total_fixed += $?)) || true
        done < <(find_cpp_files)
        echo "Total potential casts in Category C: $total_fixed"
        ;;
    category_d)
        echo "Processing Category D: Mixed-type arithmetic"
        echo "Category D requires manual validation - skipping"
        ;;
    *)
        echo "Unknown category: $CATEGORY"
        exit 1
        ;;
esac

echo ""
echo "Process complete!"
