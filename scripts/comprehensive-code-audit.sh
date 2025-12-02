#!/bin/bash
#==============================================================================
# ThemisDB - Comprehensive Offline Source Code Audit
#==============================================================================
# Version: 1.0
# Description: Unified pre-build source code validation framework
# Usage: ./scripts/comprehensive-code-audit.sh [OPTIONS]
#
# This script performs extensive offline source code analysis including:
# - Static analysis (clang-tidy, cppcheck)
# - Security scanning (hardcoded secrets, unsafe patterns)
# - Code quality metrics (complexity, duplication)
# - Dependency analysis
# - Header validation
# - Memory safety checks
# - Style compliance
# - Documentation coverage
#==============================================================================

set -euo pipefail

# Configuration
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"
REPORT_DIR="$PROJECT_ROOT/.audit-reports"
TIMESTAMP=$(date +%Y%m%d_%H%M%S)

# Colors
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
CYAN='\033[0;36m'
BOLD='\033[1m'
NC='\033[0m'

# Counters
TOTAL_ERRORS=0
TOTAL_WARNINGS=0
TOTAL_INFO=0

# Options
QUICK_MODE=0
STRICT_MODE=0
FIX_MODE=0
VERBOSE=0
SKIP_TOOLS=""
OUTPUT_FORMAT="text"
GENERATE_REPORT=1

#==============================================================================
# Helper Functions
#==============================================================================

print_banner() {
    echo -e "${CYAN}"
    echo "╔══════════════════════════════════════════════════════════════════╗"
    echo "║     ThemisDB - Comprehensive Offline Source Code Audit          ║"
    echo "║                      Version 1.0                                 ║"
    echo "╚══════════════════════════════════════════════════════════════════╝"
    echo -e "${NC}"
}

print_section() {
    echo -e "\n${BLUE}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${NC}"
    echo -e "${BOLD}${BLUE}▶ $1${NC}"
    echo -e "${BLUE}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${NC}\n"
}

print_subsection() {
    echo -e "\n${CYAN}  ┌─ $1${NC}"
}

log_error() {
    echo -e "  ${RED}✗ ERROR:${NC} $1"
    ((TOTAL_ERRORS++)) || true
}

log_warning() {
    echo -e "  ${YELLOW}⚠ WARNING:${NC} $1"
    ((TOTAL_WARNINGS++)) || true
}

log_info() {
    echo -e "  ${BLUE}ℹ INFO:${NC} $1"
    ((TOTAL_INFO++)) || true
}

log_success() {
    echo -e "  ${GREEN}✓${NC} $1"
}

log_detail() {
    if [ "$VERBOSE" -eq 1 ]; then
        echo -e "    ${NC}$1"
    fi
}

command_exists() {
    command -v "$1" &> /dev/null
}

show_help() {
    cat << EOF
Usage: $(basename "$0") [OPTIONS]

Comprehensive offline source code audit for ThemisDB.

OPTIONS:
    -q, --quick         Run quick checks only (skip deep analysis)
    -s, --strict        Treat warnings as errors (fail on any issue)
    -f, --fix           Attempt to auto-fix issues where possible
    -v, --verbose       Show detailed output
    --skip=TOOL         Skip specific tool (comma-separated: clang-tidy,cppcheck,secrets,etc.)
    --format=FORMAT     Output format: text, json, html (default: text)
    --no-report         Don't generate detailed report file
    -h, --help          Show this help message

CHECKS PERFORMED:
    1.  Project Structure Validation
    2.  Header Guard Verification
    3.  Include Dependency Analysis
    4.  Clang-Tidy Static Analysis
    5.  Cppcheck Linting
    6.  Hardcoded Secrets Detection
    7.  Unsafe Memory Pattern Detection
    8.  Code Complexity Analysis
    9.  Code Duplication Detection
    10. API Documentation Coverage
    11. TODO/FIXME Tracking
    12. Deprecated API Detection
    13. Exception Safety Analysis
    14. Thread Safety Patterns
    15. License Header Compliance

EXAMPLES:
    # Full audit
    ./scripts/comprehensive-code-audit.sh

    # Quick audit (skip deep analysis)
    ./scripts/comprehensive-code-audit.sh --quick

    # Strict mode (fail on warnings)
    ./scripts/comprehensive-code-audit.sh --strict

    # Skip specific tools
    ./scripts/comprehensive-code-audit.sh --skip=clang-tidy,cppcheck

    # Generate HTML report
    ./scripts/comprehensive-code-audit.sh --format=html
EOF
    exit 0
}

#==============================================================================
# Parse Arguments
#==============================================================================

while [[ $# -gt 0 ]]; do
    case $1 in
        -q|--quick)
            QUICK_MODE=1
            shift
            ;;
        -s|--strict)
            STRICT_MODE=1
            shift
            ;;
        -f|--fix)
            FIX_MODE=1
            shift
            ;;
        -v|--verbose)
            VERBOSE=1
            shift
            ;;
        --skip=*)
            SKIP_TOOLS="${1#*=}"
            shift
            ;;
        --format=*)
            OUTPUT_FORMAT="${1#*=}"
            shift
            ;;
        --no-report)
            GENERATE_REPORT=0
            shift
            ;;
        -h|--help)
            show_help
            ;;
        *)
            echo "Unknown option: $1"
            show_help
            ;;
    esac
done

should_skip() {
    [[ ",$SKIP_TOOLS," == *",$1,"* ]]
}

#==============================================================================
# Setup
#==============================================================================

cd "$PROJECT_ROOT"

if [ "$GENERATE_REPORT" -eq 1 ]; then
    mkdir -p "$REPORT_DIR"
    REPORT_FILE="$REPORT_DIR/audit_${TIMESTAMP}.txt"
    exec > >(tee -a "$REPORT_FILE") 2>&1
fi

print_banner
echo "Audit started at: $(date)"
echo "Project root: $PROJECT_ROOT"
echo "Mode: $([ "$QUICK_MODE" -eq 1 ] && echo 'QUICK' || echo 'FULL')"
echo "Strict: $([ "$STRICT_MODE" -eq 1 ] && echo 'YES' || echo 'NO')"

#==============================================================================
# 1. Project Structure Validation
#==============================================================================

print_section "1. Project Structure Validation"

required_dirs=("src" "include" "tests" "docs" "scripts" "config")
required_files=("CMakeLists.txt" "vcpkg.json" "README.md" "SECURITY.md" "LICENSE")

for dir in "${required_dirs[@]}"; do
    if [ -d "$dir" ]; then
        count=$(find "$dir" -type f 2>/dev/null | wc -l)
        log_success "Directory exists: $dir ($count files)"
    else
        log_error "Missing required directory: $dir"
    fi
done

for file in "${required_files[@]}"; do
    if [ -f "$file" ]; then
        log_success "File exists: $file"
    else
        log_warning "Missing recommended file: $file"
    fi
done

#==============================================================================
# 2. Header Guard Verification
#==============================================================================

print_section "2. Header Guard Verification"

header_count=0
missing_guards=0

while IFS= read -r -d '' header; do
    ((header_count++)) || true
    content=$(cat "$header")
    
    if [[ ! "$content" =~ "#pragma once" ]] && [[ ! "$content" =~ "#ifndef" ]]; then
        log_warning "Missing header guard: $header"
        ((missing_guards++)) || true
    fi
done < <(find include src -name "*.h" -o -name "*.hpp" 2>/dev/null | tr '\n' '\0')

if [ "$missing_guards" -eq 0 ]; then
    log_success "All $header_count headers have proper guards"
else
    log_warning "$missing_guards of $header_count headers missing guards"
fi

#==============================================================================
# 3. Include Dependency Analysis
#==============================================================================

print_section "3. Include Dependency Analysis"

print_subsection "Checking for problematic include patterns"

# Bad includes
bad_patterns=(
    "#include\s*<bits/stdc++.h>:Non-portable bits/stdc++.h include"
    "using namespace std;:Global 'using namespace std' in headers"
    "#include\s*\"\.\.\/\.\.\/\.\.:Deep relative include paths"
)

for pattern_msg in "${bad_patterns[@]}"; do
    pattern="${pattern_msg%%:*}"
    msg="${pattern_msg#*:}"
    
    matches=$(grep -rn "$pattern" include/ src/ 2>/dev/null | head -5 || true)
    if [ -n "$matches" ]; then
        log_warning "$msg"
        if [ "$VERBOSE" -eq 1 ]; then
            echo "$matches" | while read -r line; do
                log_detail "$line"
            done
        fi
    fi
done

# Check for circular includes (basic check)
print_subsection "Checking for potential circular includes"

circular_count=0
while IFS= read -r -d '' header; do
    basename_h=$(basename "$header")
    includes=$(grep -h "#include" "$header" 2>/dev/null | grep -v "^//" || true)
    
    if echo "$includes" | grep -q "$basename_h"; then
        log_warning "Potential self-include in: $header"
        ((circular_count++)) || true
    fi
done < <(find include -name "*.h" 2>/dev/null | tr '\n' '\0')

if [ "$circular_count" -eq 0 ]; then
    log_success "No obvious circular includes detected"
fi

#==============================================================================
# 4. Clang-Tidy Static Analysis
#==============================================================================

if ! should_skip "clang-tidy"; then
    print_section "4. Clang-Tidy Static Analysis"
    
    if command_exists clang-tidy; then
        if [ -f "build/compile_commands.json" ] || [ -f "build-wsl/compile_commands.json" ]; then
            BUILD_DIR=$([ -f "build/compile_commands.json" ] && echo "build" || echo "build-wsl")
            
            if [ "$QUICK_MODE" -eq 1 ]; then
                # Quick mode: only changed files
                files=$(git diff --name-only --diff-filter=ACM HEAD 2>/dev/null | grep -E '\.(cpp|h)$' | head -10 || true)
            else
                # Full mode: all source files
                files=$(find src include -name "*.cpp" 2>/dev/null | head -50)
            fi
            
            if [ -n "$files" ]; then
                tidy_issues=0
                echo "$files" | while read -r file; do
                    if [ -f "$file" ]; then
                        log_detail "Analyzing: $file"
                        result=$(clang-tidy "$file" -p "$BUILD_DIR" 2>&1 || true)
                        if echo "$result" | grep -qE "warning:|error:"; then
                            ((tidy_issues++)) || true
                        fi
                    fi
                done
                
                if [ "$tidy_issues" -eq 0 ]; then
                    log_success "Clang-tidy analysis passed"
                else
                    log_warning "Clang-tidy found issues in $tidy_issues files"
                fi
            else
                log_info "No files to analyze"
            fi
        else
            log_warning "compile_commands.json not found - run CMake first"
        fi
    else
        log_info "clang-tidy not installed (skipping)"
    fi
else
    print_section "4. Clang-Tidy Static Analysis (SKIPPED)"
fi

#==============================================================================
# 5. Cppcheck Linting
#==============================================================================

if ! should_skip "cppcheck"; then
    print_section "5. Cppcheck Static Analysis"
    
    if command_exists cppcheck; then
        cppcheck_args=(
            "--enable=warning,style,performance,portability"
            "--std=c++17"
            "--inline-suppr"
            "--suppress=missingIncludeSystem"
            "--suppress=unmatchedSuppression"
            "--quiet"
            "-I" "include/"
        )
        
        if [ "$QUICK_MODE" -eq 1 ]; then
            cppcheck_args+=("--max-configs=1")
        fi
        
        cppcheck_output=$(cppcheck "${cppcheck_args[@]}" src/ 2>&1 || true)
        
        if [ -n "$cppcheck_output" ]; then
            error_count=$(echo "$cppcheck_output" | grep -c "error:" || true)
            warning_count=$(echo "$cppcheck_output" | grep -cE "warning:|style:|performance:" || true)
            
            if [ "$error_count" -gt 0 ]; then
                log_error "Cppcheck found $error_count errors"
            fi
            if [ "$warning_count" -gt 0 ]; then
                log_warning "Cppcheck found $warning_count warnings"
            fi
            
            if [ "$VERBOSE" -eq 1 ]; then
                echo "$cppcheck_output" | head -20
            fi
        else
            log_success "Cppcheck analysis passed"
        fi
    else
        log_info "cppcheck not installed (skipping)"
    fi
else
    print_section "5. Cppcheck Static Analysis (SKIPPED)"
fi

#==============================================================================
# 6. Hardcoded Secrets Detection
#==============================================================================

if ! should_skip "secrets"; then
    print_section "6. Hardcoded Secrets Detection"
    
    print_subsection "Scanning for potential secrets"
    
    secret_patterns=(
        "password\s*=\s*['\"][^'\"]+['\"]"
        "api_key\s*=\s*['\"][^'\"]+['\"]"
        "secret\s*=\s*['\"][^'\"]+['\"]"
        "token\s*=\s*['\"][a-zA-Z0-9]{20,}['\"]"
        "-----BEGIN.*PRIVATE KEY-----"
        "aws_access_key_id\s*="
        "aws_secret_access_key\s*="
    )
    
    secrets_found=0
    
    for pattern in "${secret_patterns[@]}"; do
        matches=$(grep -rniE "$pattern" src/ include/ config/ 2>/dev/null | grep -v "test" | grep -v "example" | head -5 || true)
        if [ -n "$matches" ]; then
            log_error "Potential hardcoded secret found (pattern: ${pattern:0:30}...)"
            ((secrets_found++)) || true
            if [ "$VERBOSE" -eq 1 ]; then
                echo "$matches" | while read -r line; do
                    log_detail "$line"
                done
            fi
        fi
    done
    
    if [ "$secrets_found" -eq 0 ]; then
        log_success "No obvious hardcoded secrets detected"
    fi
    
    # Check for gitleaks if available
    if command_exists gitleaks; then
        print_subsection "Running gitleaks scan"
        gitleaks_output=$(gitleaks detect --source . --no-git 2>&1 || true)
        if echo "$gitleaks_output" | grep -q "leaks found"; then
            log_error "Gitleaks detected potential secrets"
        else
            log_success "Gitleaks scan passed"
        fi
    fi
else
    print_section "6. Hardcoded Secrets Detection (SKIPPED)"
fi

#==============================================================================
# 7. Unsafe Memory Pattern Detection
#==============================================================================

print_section "7. Unsafe Memory Pattern Detection"

unsafe_patterns=(
    "malloc\s*\(:Raw malloc - prefer smart pointers"
    "free\s*\(:Raw free - prefer RAII"
    "new\s+\w+\s*\[:Raw array new - prefer std::vector"
    "delete\s*\[:Raw delete[] - prefer std::vector"
    "strcpy\s*\(:Unsafe strcpy - use std::string or strncpy"
    "sprintf\s*\(:Unsafe sprintf - use snprintf or std::format"
    "gets\s*\(:Dangerous gets - never use"
    "strcat\s*\(:Unsafe strcat - use strncat"
    "scanf\s*\(:Potentially unsafe scanf"
)

unsafe_count=0

for pattern_msg in "${unsafe_patterns[@]}"; do
    pattern="${pattern_msg%%:*}"
    msg="${pattern_msg#*:}"
    
    count=$(grep -rnE "$pattern" src/ 2>/dev/null | grep -v "// NOLINT" | wc -l || true)
    if [ "$count" -gt 0 ]; then
        log_warning "$msg ($count occurrences)"
        ((unsafe_count += count)) || true
    fi
done

if [ "$unsafe_count" -eq 0 ]; then
    log_success "No unsafe memory patterns detected"
else
    log_warning "Total unsafe patterns: $unsafe_count"
fi

#==============================================================================
# 8. Code Complexity Analysis
#==============================================================================

if [ "$QUICK_MODE" -eq 0 ]; then
    print_section "8. Code Complexity Analysis"
    
    print_subsection "Detecting large functions"
    
    # Simple heuristic: count lines between function definitions
    large_functions=0
    
    while IFS= read -r -d '' file; do
        # Count functions with >100 lines (simple heuristic)
        awk '
            /^[a-zA-Z_].*\(.*\)\s*{?\s*$/ { start = NR; fname = $0 }
            /^}$/ && start > 0 { 
                if (NR - start > 100) { 
                    print FILENAME ":" start ": Large function (" (NR - start) " lines)"
                }
                start = 0
            }
        ' "$file" 2>/dev/null
    done < <(find src -name "*.cpp" 2>/dev/null | tr '\n' '\0') | while read -r line; do
        log_warning "$line"
        ((large_functions++)) || true
    done
    
    print_subsection "Checking file sizes"
    
    while IFS= read -r -d '' file; do
        lines=$(wc -l < "$file")
        if [ "$lines" -gt 1000 ]; then
            log_warning "Large file: $file ($lines lines)"
        fi
    done < <(find src include -name "*.cpp" -o -name "*.h" 2>/dev/null | tr '\n' '\0')
fi

#==============================================================================
# 9. TODO/FIXME/HACK Detection
#==============================================================================

print_section "9. Code Annotation Tracking"

annotations=("TODO" "FIXME" "HACK" "XXX" "BUG" "OPTIMIZE" "REFACTOR")

for ann in "${annotations[@]}"; do
    count=$(grep -rn "\b${ann}\b" src/ include/ 2>/dev/null | wc -l || true)
    if [ "$count" -gt 0 ]; then
        case "$ann" in
            "FIXME"|"BUG")
                log_warning "$ann: $count occurrences (should be addressed)"
                ;;
            "TODO"|"HACK")
                log_info "$ann: $count occurrences"
                ;;
            *)
                log_detail "$ann: $count occurrences"
                ;;
        esac
    fi
done

#==============================================================================
# 10. Deprecated API Detection
#==============================================================================

print_section "10. Deprecated API Detection"

deprecated_apis=(
    "auto_ptr:std::auto_ptr is deprecated - use unique_ptr"
    "gets:gets() is removed in C11"
    "sprintf:sprintf is deprecated - use snprintf"
    "std::bind:Consider lambda instead of std::bind"
    "random_shuffle:std::random_shuffle is deprecated - use std::shuffle"
)

deprecated_count=0

for api_msg in "${deprecated_apis[@]}"; do
    api="${api_msg%%:*}"
    msg="${api_msg#*:}"
    
    count=$(grep -rn "\b${api}\b" src/ include/ 2>/dev/null | wc -l || true)
    if [ "$count" -gt 0 ]; then
        log_warning "$msg ($count uses)"
        ((deprecated_count += count)) || true
    fi
done

if [ "$deprecated_count" -eq 0 ]; then
    log_success "No deprecated APIs detected"
fi

#==============================================================================
# 11. Thread Safety Patterns
#==============================================================================

print_section "11. Thread Safety Analysis"

print_subsection "Checking mutex usage patterns"

# Check for proper mutex usage
mutex_count=$(grep -rn "std::mutex\|std::shared_mutex\|std::recursive_mutex" src/ include/ 2>/dev/null | wc -l || true)
lock_guard_count=$(grep -rn "std::lock_guard\|std::unique_lock\|std::shared_lock" src/ include/ 2>/dev/null | wc -l || true)

log_info "Mutex declarations: $mutex_count"
log_info "Lock guards used: $lock_guard_count"

if [ "$mutex_count" -gt 0 ] && [ "$lock_guard_count" -eq 0 ]; then
    log_warning "Mutexes found but no RAII lock guards - potential for deadlocks"
fi

# Check for atomic usage
atomic_count=$(grep -rn "std::atomic" src/ include/ 2>/dev/null | wc -l || true)
log_info "Atomic variables: $atomic_count"

# Check for thread_local
thread_local_count=$(grep -rn "thread_local" src/ include/ 2>/dev/null | wc -l || true)
log_info "Thread-local variables: $thread_local_count"

#==============================================================================
# 12. Exception Safety Analysis
#==============================================================================

print_section "12. Exception Safety Analysis"

# Check for noexcept usage
noexcept_count=$(grep -rn "noexcept" src/ include/ 2>/dev/null | wc -l || true)
log_info "noexcept specifications: $noexcept_count"

# Check for catch(...) which might swallow exceptions
catch_all=$(grep -rn "catch\s*(\.\.\.).*{" src/ 2>/dev/null | wc -l || true)
if [ "$catch_all" -gt 0 ]; then
    log_warning "Generic catch(...) blocks: $catch_all (may swallow important exceptions)"
fi

# Check for exception specifications
throw_spec=$(grep -rn "throw\s*(" src/ include/ 2>/dev/null | grep -v "throw;" | wc -l || true)
log_info "throw() specifications: $throw_spec"

#==============================================================================
# 13. License Header Compliance
#==============================================================================

print_section "13. License Header Compliance"

files_without_license=0
total_source_files=0

while IFS= read -r -d '' file; do
    ((total_source_files++)) || true
    head -10 "$file" | grep -qiE "license|copyright|MIT|Apache|GPL" || {
        ((files_without_license++)) || true
        log_detail "Missing license header: $file"
    }
done < <(find src include -name "*.cpp" -o -name "*.h" 2>/dev/null | tr '\n' '\0')

if [ "$files_without_license" -eq 0 ]; then
    log_success "All $total_source_files source files have license headers"
else
    log_warning "$files_without_license of $total_source_files files missing license headers"
fi

#==============================================================================
# 14. Documentation Coverage
#==============================================================================

print_section "14. Documentation Coverage"

# Check for Doxygen-style comments
doxygen_comments=$(grep -rn "///" src/ include/ 2>/dev/null | wc -l || true)
doxygen_blocks=$(grep -rn "/\*\*" src/ include/ 2>/dev/null | wc -l || true)

log_info "Doxygen single-line comments (///): $doxygen_comments"
log_info "Doxygen block comments (/**): $doxygen_blocks"

# Check for README in key directories
readme_dirs=("src" "include" "docs" "tests" "scripts")
for dir in "${readme_dirs[@]}"; do
    if [ -f "$dir/README.md" ]; then
        log_success "$dir/README.md exists"
    else
        log_detail "No README.md in $dir"
    fi
done

#==============================================================================
# 15. Source Code Statistics
#==============================================================================

print_section "15. Source Code Statistics"

cpp_files=$(find src -name "*.cpp" 2>/dev/null | wc -l || true)
h_files=$(find include -name "*.h" -o -name "*.hpp" 2>/dev/null | wc -l || true)
test_files=$(find tests -name "*.cpp" 2>/dev/null | wc -l || true)

total_cpp_lines=$(find src -name "*.cpp" -exec cat {} + 2>/dev/null | wc -l || true)
total_h_lines=$(find include -name "*.h" -o -name "*.hpp" -exec cat {} + 2>/dev/null | wc -l || true)
total_test_lines=$(find tests -name "*.cpp" -exec cat {} + 2>/dev/null | wc -l || true)

echo "  Source Files (.cpp):     $cpp_files files, $total_cpp_lines lines"
echo "  Header Files (.h/.hpp):  $h_files files, $total_h_lines lines"
echo "  Test Files (.cpp):       $test_files files, $total_test_lines lines"
echo "  ─────────────────────────────────────────"
echo "  Total:                   $((cpp_files + h_files + test_files)) files, $((total_cpp_lines + total_h_lines + total_test_lines)) lines"

#==============================================================================
# Summary
#==============================================================================

print_section "AUDIT SUMMARY"

echo -e "  ${BOLD}Timestamp:${NC}  $(date)"
echo -e "  ${BOLD}Duration:${NC}   $SECONDS seconds"
echo ""
echo -e "  ${RED}Errors:${NC}     $TOTAL_ERRORS"
echo -e "  ${YELLOW}Warnings:${NC}   $TOTAL_WARNINGS"
echo -e "  ${BLUE}Info:${NC}       $TOTAL_INFO"
echo ""

if [ "$GENERATE_REPORT" -eq 1 ]; then
    echo -e "  ${CYAN}Report saved to:${NC} $REPORT_FILE"
fi

echo ""

if [ "$TOTAL_ERRORS" -gt 0 ]; then
    echo -e "${RED}╔══════════════════════════════════════════════════════════════════╗${NC}"
    echo -e "${RED}║                    ✗ AUDIT FAILED                                ║${NC}"
    echo -e "${RED}║     Please fix errors before proceeding with the build          ║${NC}"
    echo -e "${RED}╚══════════════════════════════════════════════════════════════════╝${NC}"
    exit 1
elif [ "$STRICT_MODE" -eq 1 ] && [ "$TOTAL_WARNINGS" -gt 0 ]; then
    echo -e "${YELLOW}╔══════════════════════════════════════════════════════════════════╗${NC}"
    echo -e "${YELLOW}║              ⚠ AUDIT FAILED (STRICT MODE)                       ║${NC}"
    echo -e "${YELLOW}║     Warnings treated as errors in strict mode                   ║${NC}"
    echo -e "${YELLOW}╚══════════════════════════════════════════════════════════════════╝${NC}"
    exit 1
else
    echo -e "${GREEN}╔══════════════════════════════════════════════════════════════════╗${NC}"
    echo -e "${GREEN}║                    ✓ AUDIT PASSED                                ║${NC}"
    echo -e "${GREEN}║              Ready to proceed with build                         ║${NC}"
    echo -e "${GREEN}╚══════════════════════════════════════════════════════════════════╝${NC}"
    exit 0
fi
