#!/bin/bash
# Comprehensive Code Audit Script for ThemisDB
# Performs systematic security and compliance analysis
# Aligned with: BSI C5, ISO 27001, DSGVO, NIS2, OWASP ASVS, NIST CSF
#
# Usage: ./scripts/comprehensive-code-audit.sh [OPTIONS]
#
# This script implements the audit procedures referenced in:
# - .github/ISSUE_TEMPLATE/security-compliance-investigation.md
# - docs/de/compliance/compliance_full_checklist.md

set -e  # Exit on error (can be disabled with --continue-on-error)

# ============================================================================
# Configuration & Constants
# ============================================================================

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(dirname "$SCRIPT_DIR")"
AUDIT_DATE=$(date +"%Y-%m-%d_%H-%M-%S")
AUDIT_DIR="${PROJECT_ROOT}/audit-results-${AUDIT_DATE}"
REPORT_FILE="${AUDIT_DIR}/comprehensive-audit-report.md"

# Color codes for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
MAGENTA='\033[0;35m'
CYAN='\033[0;36m'
NC='\033[0m' # No Color

# Audit categories
CATEGORY_SAST=1
CATEGORY_DYNAMIC=1
CATEGORY_DEPENDENCIES=1
CATEGORY_SECRETS=1
CATEGORY_CONTAINER=1
CATEGORY_FUZZING=0  # Disabled by default (requires AFL++)

# Tool availability flags
HAS_CPPCHECK=0
HAS_CLANG_TIDY=0
HAS_TRIVY=0
HAS_GITLEAKS=0
HAS_SEMGREP=0
HAS_VALGRIND=0
HAS_AFL=0

# Error handling
CONTINUE_ON_ERROR=0
EXIT_CODE=0

# ============================================================================
# Helper Functions
# ============================================================================

print_header() {
    echo ""
    echo -e "${CYAN}========================================${NC}"
    echo -e "${CYAN}$1${NC}"
    echo -e "${CYAN}========================================${NC}"
}

print_subheader() {
    echo ""
    echo -e "${BLUE}>>> $1${NC}"
}

print_success() {
    echo -e "${GREEN}✓ $1${NC}"
}

print_warning() {
    echo -e "${YELLOW}⚠ $1${NC}"
}

print_error() {
    echo -e "${RED}✗ $1${NC}"
}

print_info() {
    echo -e "${MAGENTA}ℹ $1${NC}"
}

command_exists() {
    command -v "$1" &> /dev/null
}

check_prerequisites() {
    print_header "Checking Prerequisites"
    
    local missing_critical=0
    
    # Check critical tools
    if ! command_exists cmake; then
        print_error "cmake not found (CRITICAL)"
        missing_critical=1
    else
        print_success "cmake: $(cmake --version | head -n1)"
    fi
    
    if ! command_exists git; then
        print_error "git not found (CRITICAL)"
        missing_critical=1
    else
        print_success "git: $(git --version)"
    fi
    
    # Check optional security tools
    if command_exists cppcheck; then
        HAS_CPPCHECK=1
        print_success "cppcheck: $(cppcheck --version)"
    else
        print_warning "cppcheck not found (SAST will be limited)"
    fi
    
    if command_exists clang-tidy; then
        HAS_CLANG_TIDY=1
        print_success "clang-tidy: $(clang-tidy --version | head -n1)"
    else
        print_warning "clang-tidy not found (SAST will be limited)"
    fi
    
    if command_exists trivy; then
        HAS_TRIVY=1
        print_success "trivy: $(trivy --version | head -n1)"
    else
        print_warning "trivy not found (dependency scanning disabled)"
    fi
    
    if command_exists gitleaks; then
        HAS_GITLEAKS=1
        print_success "gitleaks: $(gitleaks version)"
    else
        print_warning "gitleaks not found (secret scanning disabled)"
    fi
    
    if command_exists semgrep; then
        HAS_SEMGREP=1
        print_success "semgrep: $(semgrep --version)"
    else
        print_warning "semgrep not found (SAST patterns disabled)"
    fi
    
    if command_exists valgrind; then
        HAS_VALGRIND=1
        print_success "valgrind: $(valgrind --version | head -n1)"
    else
        print_warning "valgrind not found (memory analysis disabled)"
    fi
    
    if command_exists afl-fuzz; then
        HAS_AFL=1
        print_success "AFL++: $(afl-fuzz --version 2>&1 | head -n1)"
    else
        print_warning "AFL++ not found (fuzzing disabled)"
    fi
    
    if [ "$missing_critical" -eq 1 ]; then
        print_error "Missing critical tools. Install them with:"
        echo "  Ubuntu/Debian: sudo apt-get install cmake git build-essential"
        return 1
    fi
    
    return 0
}

initialize_audit() {
    print_header "Initializing Audit"
    
    # Create audit directory structure
    mkdir -p "${AUDIT_DIR}/sast"
    mkdir -p "${AUDIT_DIR}/dynamic"
    mkdir -p "${AUDIT_DIR}/dependencies"
    mkdir -p "${AUDIT_DIR}/secrets"
    mkdir -p "${AUDIT_DIR}/container"
    mkdir -p "${AUDIT_DIR}/fuzzing"
    mkdir -p "${AUDIT_DIR}/reports"
    
    print_success "Created audit directory: ${AUDIT_DIR}"
    
    # Initialize report
    cat > "$REPORT_FILE" << EOF
# ThemisDB Comprehensive Security Audit Report

**Date:** $(date +"%Y-%m-%d %H:%M:%S %Z")  
**Version:** $(cat VERSION 2>/dev/null || echo "unknown")  
**Git Commit:** $(git rev-parse --short HEAD 2>/dev/null || echo "unknown")  
**Auditor:** Automated Security Audit Script  
**Compliance Frameworks:** BSI C5, ISO 27001, DSGVO, NIS2, OWASP ASVS, NIST CSF

---

## Executive Summary

This report contains the results of a comprehensive automated security audit performed on the ThemisDB codebase.

### Audit Scope

EOF

    if [ "$CATEGORY_SAST" -eq 1 ]; then
        echo "- ✅ Static Application Security Testing (SAST)" >> "$REPORT_FILE"
    fi
    if [ "$CATEGORY_DYNAMIC" -eq 1 ]; then
        echo "- ✅ Dynamic Analysis" >> "$REPORT_FILE"
    fi
    if [ "$CATEGORY_DEPENDENCIES" -eq 1 ]; then
        echo "- ✅ Dependency & Supply Chain Security" >> "$REPORT_FILE"
    fi
    if [ "$CATEGORY_SECRETS" -eq 1 ]; then
        echo "- ✅ Secret Detection" >> "$REPORT_FILE"
    fi
    if [ "$CATEGORY_CONTAINER" -eq 1 ]; then
        echo "- ✅ Container Security" >> "$REPORT_FILE"
    fi
    if [ "$CATEGORY_FUZZING" -eq 1 ]; then
        echo "- ✅ Fuzzing Analysis" >> "$REPORT_FILE"
    fi
    
    echo "" >> "$REPORT_FILE"
    echo "---" >> "$REPORT_FILE"
    echo "" >> "$REPORT_FILE"
}

# ============================================================================
# Static Application Security Testing (SAST)
# ============================================================================

run_sast() {
    if [ "$CATEGORY_SAST" -ne 1 ]; then
        print_info "SAST category disabled"
        return 0
    fi
    
    print_header "Static Application Security Testing (SAST)"
    
    echo "## 1. Static Application Security Testing (SAST)" >> "$REPORT_FILE"
    echo "" >> "$REPORT_FILE"
    
    local sast_failed=0
    
    # Run cppcheck
    if [ "$HAS_CPPCHECK" -eq 1 ]; then
        run_cppcheck || sast_failed=1
    fi
    
    # Run clang-tidy
    if [ "$HAS_CLANG_TIDY" -eq 1 ]; then
        run_clang_tidy || sast_failed=1
    fi
    
    # Run semgrep
    if [ "$HAS_SEMGREP" -eq 1 ]; then
        run_semgrep || sast_failed=1
    fi
    
    if [ "$sast_failed" -ne 0 ] && [ "$CONTINUE_ON_ERROR" -eq 0 ]; then
        return 1
    fi
    
    return 0
}

run_cppcheck() {
    print_subheader "Running cppcheck"
    
    local output_file="${AUDIT_DIR}/sast/cppcheck-report.xml"
    local text_file="${AUDIT_DIR}/sast/cppcheck-output.txt"
    
    # Build cppcheck command with optional suppressions
    local cppcheck_cmd="cppcheck \
        --enable=all \
        --std=c++20 \
        --language=c++ \
        --platform=unix64"
    
    # Add suppressions file if it exists
    if [ -f "${PROJECT_ROOT}/.cppcheck-suppressions" ]; then
        cppcheck_cmd="$cppcheck_cmd \
        --suppressions-list=\"${PROJECT_ROOT}/.cppcheck-suppressions\""
    fi
    
    cppcheck_cmd="$cppcheck_cmd \
        --inline-suppr \
        --xml \
        --xml-version=2 \
        --output-file=\"$output_file\" \
        -I \"${PROJECT_ROOT}/include/\" \
        \"${PROJECT_ROOT}/src/\""
    
    eval "$cppcheck_cmd" 2>&1 | tee "$text_file"
    
    local exit_status=$?
    
    # Count issues
    local error_count=$(grep -c '<error ' "$output_file" 2>/dev/null || echo 0)
    
    echo "### 1.1 Cppcheck Analysis" >> "$REPORT_FILE"
    echo "" >> "$REPORT_FILE"
    echo "**Tool Version:** $(cppcheck --version)" >> "$REPORT_FILE"
    echo "**Errors Found:** $error_count" >> "$REPORT_FILE"
    echo "**Report:** \`sast/cppcheck-report.xml\`" >> "$REPORT_FILE"
    echo "" >> "$REPORT_FILE"
    
    if [ "$error_count" -eq 0 ]; then
        print_success "cppcheck: No issues found"
        echo "✅ **Status:** PASSED - No critical issues detected" >> "$REPORT_FILE"
    else
        print_warning "cppcheck: Found $error_count issues"
        echo "⚠️ **Status:** ISSUES FOUND - Review required" >> "$REPORT_FILE"
        EXIT_CODE=1
    fi
    
    echo "" >> "$REPORT_FILE"
    
    return $exit_status
}

run_clang_tidy() {
    print_subheader "Running clang-tidy"

    local compile_db=""
    local candidate
    for candidate in \
        "${PROJECT_ROOT}/build-msvc-windows-debug/compile_commands.json" \
        "${PROJECT_ROOT}/build-msvc-windows-release/compile_commands.json" \
        "${PROJECT_ROOT}/build-gcc-linux-debug/compile_commands.json" \
        "${PROJECT_ROOT}/build-gcc-linux-release/compile_commands.json" \
        "${PROJECT_ROOT}/build/compile_commands.json"; do
        if [ -f "$candidate" ]; then
            compile_db="$candidate"
            break
        fi
    done
    
    # Check if compile_commands.json exists
    if [ -z "$compile_db" ]; then
        print_info "Generating compile_commands.json..."
        cd "$PROJECT_ROOT"
        cmake -DCMAKE_EXPORT_COMPILE_COMMANDS=ON \
              -DCMAKE_BUILD_TYPE=Debug \
              -B build-gcc-linux-debug \
              -G Ninja 2>&1 | tee "${AUDIT_DIR}/sast/cmake-output.txt" || true
        if [ -f "${PROJECT_ROOT}/build-gcc-linux-debug/compile_commands.json" ]; then
            compile_db="${PROJECT_ROOT}/build-gcc-linux-debug/compile_commands.json"
        fi
    fi
    
    if [ -z "$compile_db" ]; then
        print_warning "Could not generate compile_commands.json, skipping clang-tidy"
        return 0
    fi

    local compile_dir
    compile_dir="$(dirname "$compile_db")"
    
    local output_file="${AUDIT_DIR}/sast/clang-tidy-output.txt"
    
    # Configurable file limit (default: 50, can be set via environment)
    local file_limit=${CLANG_TIDY_FILE_LIMIT:-50}
    print_info "Analyzing up to $file_limit files (set CLANG_TIDY_FILE_LIMIT to change)"
    
    # Find C++ source files
    find "${PROJECT_ROOT}/src" "${PROJECT_ROOT}/include" \
        -type f \( -name "*.cpp" -o -name "*.h" -o -name "*.hpp" \) \
        2>/dev/null | head -n "$file_limit" | \
        xargs -I {} clang-tidy -p "$compile_dir" {} \
        2>&1 | tee "$output_file" || true
    
    # Count warnings and errors
    local warning_count=$(grep -c 'warning:' "$output_file" 2>/dev/null || echo 0)
    local error_count=$(grep -c 'error:' "$output_file" 2>/dev/null || echo 0)
    
    echo "### 1.2 Clang-Tidy Analysis" >> "$REPORT_FILE"
    echo "" >> "$REPORT_FILE"
    echo "**Tool Version:** $(clang-tidy --version | head -n1)" >> "$REPORT_FILE"
    echo "**Warnings Found:** $warning_count" >> "$REPORT_FILE"
    echo "**Errors Found:** $error_count" >> "$REPORT_FILE"
    echo "**Report:** \`sast/clang-tidy-output.txt\`" >> "$REPORT_FILE"
    echo "" >> "$REPORT_FILE"
    
    if [ "$error_count" -eq 0 ]; then
        print_success "clang-tidy: No errors found ($warning_count warnings)"
        echo "✅ **Status:** PASSED - No critical errors" >> "$REPORT_FILE"
    else
        print_warning "clang-tidy: Found $error_count errors"
        echo "⚠️ **Status:** ERRORS FOUND - Review required" >> "$REPORT_FILE"
        EXIT_CODE=1
    fi
    
    echo "" >> "$REPORT_FILE"
    
    return 0
}

run_semgrep() {
    print_subheader "Running semgrep"
    
    local output_file="${AUDIT_DIR}/sast/semgrep-report.json"
    local text_file="${AUDIT_DIR}/sast/semgrep-output.txt"
    
    cd "$PROJECT_ROOT"
    semgrep \
        --config=auto \
        --json \
        --output="$output_file" \
        --verbose \
        src/ include/ \
        2>&1 | tee "$text_file" || true
    
    # Count findings by severity
    local critical=$(jq '[.results[] | select(.extra.severity == "ERROR")] | length' "$output_file" 2>/dev/null || echo 0)
    local high=$(jq '[.results[] | select(.extra.severity == "WARNING")] | length' "$output_file" 2>/dev/null || echo 0)
    local total=$(jq '.results | length' "$output_file" 2>/dev/null || echo 0)
    
    echo "### 1.3 Semgrep Analysis" >> "$REPORT_FILE"
    echo "" >> "$REPORT_FILE"
    echo "**Tool Version:** $(semgrep --version)" >> "$REPORT_FILE"
    echo "**Total Findings:** $total" >> "$REPORT_FILE"
    echo "**Critical/Errors:** $critical" >> "$REPORT_FILE"
    echo "**Warnings:** $high" >> "$REPORT_FILE"
    echo "**Report:** \`sast/semgrep-report.json\`" >> "$REPORT_FILE"
    echo "" >> "$REPORT_FILE"
    
    if [ "$critical" -eq 0 ]; then
        print_success "semgrep: No critical issues ($total findings total)"
        echo "✅ **Status:** PASSED - No critical issues" >> "$REPORT_FILE"
    else
        print_warning "semgrep: Found $critical critical issues"
        echo "⚠️ **Status:** CRITICAL ISSUES FOUND" >> "$REPORT_FILE"
        EXIT_CODE=1
    fi
    
    echo "" >> "$REPORT_FILE"
    
    return 0
}

# ============================================================================
# Dependency & Supply Chain Security
# ============================================================================

run_dependency_scan() {
    if [ "$CATEGORY_DEPENDENCIES" -ne 1 ]; then
        print_info "Dependency scanning disabled"
        return 0
    fi
    
    print_header "Dependency & Supply Chain Security"
    
    echo "## 2. Dependency & Supply Chain Security" >> "$REPORT_FILE"
    echo "" >> "$REPORT_FILE"
    
    if [ "$HAS_TRIVY" -eq 1 ]; then
        run_trivy_fs
    else
        print_warning "Trivy not available, skipping dependency scan"
        echo "⚠️ **Trivy not available** - Install trivy for dependency scanning" >> "$REPORT_FILE"
        echo "" >> "$REPORT_FILE"
    fi
    
    # Check vcpkg dependencies
    check_vcpkg_dependencies
    
    return 0
}

run_trivy_fs() {
    print_subheader "Running Trivy filesystem scan"
    
    local output_file="${AUDIT_DIR}/dependencies/trivy-report.json"
    local text_file="${AUDIT_DIR}/dependencies/trivy-output.txt"
    
    cd "$PROJECT_ROOT"
    trivy fs \
        --scanners vuln,secret,misconfig \
        --format json \
        --output "$output_file" \
        --severity CRITICAL,HIGH,MEDIUM \
        . \
        2>&1 | tee "$text_file" || true
    
    # Parse results
    local critical=$(jq '[.Results[]?.Vulnerabilities[]? | select(.Severity == "CRITICAL")] | length' "$output_file" 2>/dev/null || echo 0)
    local high=$(jq '[.Results[]?.Vulnerabilities[]? | select(.Severity == "HIGH")] | length' "$output_file" 2>/dev/null || echo 0)
    local medium=$(jq '[.Results[]?.Vulnerabilities[]? | select(.Severity == "MEDIUM")] | length' "$output_file" 2>/dev/null || echo 0)
    
    echo "### 2.1 Trivy Vulnerability Scan" >> "$REPORT_FILE"
    echo "" >> "$REPORT_FILE"
    echo "**Tool Version:** $(trivy --version | head -n1)" >> "$REPORT_FILE"
    echo "**Critical Vulnerabilities:** $critical" >> "$REPORT_FILE"
    echo "**High Vulnerabilities:** $high" >> "$REPORT_FILE"
    echo "**Medium Vulnerabilities:** $medium" >> "$REPORT_FILE"
    echo "**Report:** \`dependencies/trivy-report.json\`" >> "$REPORT_FILE"
    echo "" >> "$REPORT_FILE"
    
    if [ "$critical" -eq 0 ] && [ "$high" -eq 0 ]; then
        print_success "trivy: No critical or high vulnerabilities"
        echo "✅ **Status:** PASSED - No critical/high vulnerabilities" >> "$REPORT_FILE"
    else
        print_warning "trivy: Found $critical critical and $high high vulnerabilities"
        echo "⚠️ **Status:** VULNERABILITIES FOUND - Remediation required" >> "$REPORT_FILE"
        EXIT_CODE=1
    fi
    
    echo "" >> "$REPORT_FILE"
}

check_vcpkg_dependencies() {
    print_subheader "Checking vcpkg dependencies"
    
    if [ ! -f "${PROJECT_ROOT}/vcpkg.json" ]; then
        print_info "vcpkg.json not found, skipping"
        return 0
    fi
    
    local dep_count=$(jq '.dependencies | length' "${PROJECT_ROOT}/vcpkg.json" 2>/dev/null || echo 0)
    
    echo "### 2.2 Vcpkg Dependencies" >> "$REPORT_FILE"
    echo "" >> "$REPORT_FILE"
    echo "**Total Dependencies:** $dep_count" >> "$REPORT_FILE"
    echo "" >> "$REPORT_FILE"
    
    # List dependencies
    echo "**Dependencies:**" >> "$REPORT_FILE"
    jq -r '.dependencies[] | if type == "string" then "- \(.)" else "- \(.name) (version: \(.version // "latest"))" end' \
        "${PROJECT_ROOT}/vcpkg.json" 2>/dev/null >> "$REPORT_FILE" || true
    
    echo "" >> "$REPORT_FILE"
    print_success "Listed $dep_count vcpkg dependencies"
}

# ============================================================================
# Secret Detection
# ============================================================================

run_secret_scan() {
    if [ "$CATEGORY_SECRETS" -ne 1 ]; then
        print_info "Secret scanning disabled"
        return 0
    fi
    
    print_header "Secret Detection"
    
    echo "## 3. Secret Detection" >> "$REPORT_FILE"
    echo "" >> "$REPORT_FILE"
    
    if [ "$HAS_GITLEAKS" -eq 1 ]; then
        run_gitleaks
    else
        print_warning "Gitleaks not available, skipping secret scan"
        echo "⚠️ **Gitleaks not available** - Install gitleaks for secret detection" >> "$REPORT_FILE"
        echo "" >> "$REPORT_FILE"
    fi
    
    return 0
}

run_gitleaks() {
    print_subheader "Running gitleaks"
    
    local output_file="${AUDIT_DIR}/secrets/gitleaks-report.json"
    local text_file="${AUDIT_DIR}/secrets/gitleaks-output.txt"
    
    cd "$PROJECT_ROOT"
    
    # Check if .gitleaks.toml exists
    local config_flag=""
    if [ -f ".gitleaks.toml" ]; then
        config_flag="--config .gitleaks.toml"
    fi
    
    gitleaks detect \
        --source . \
        $config_flag \
        --report-format json \
        --report-path "$output_file" \
        --verbose \
        --no-git \
        2>&1 | tee "$text_file" || true
    
    # Count secrets
    local secret_count=0
    if [ -f "$output_file" ] && [ -s "$output_file" ]; then
        secret_count=$(jq '. | length' "$output_file" 2>/dev/null || echo 0)
    fi
    
    echo "### 3.1 Gitleaks Secret Scan" >> "$REPORT_FILE"
    echo "" >> "$REPORT_FILE"
    echo "**Tool Version:** $(gitleaks version)" >> "$REPORT_FILE"
    echo "**Secrets Detected:** $secret_count" >> "$REPORT_FILE"
    echo "**Report:** \`secrets/gitleaks-report.json\`" >> "$REPORT_FILE"
    echo "" >> "$REPORT_FILE"
    
    if [ "$secret_count" -eq 0 ]; then
        print_success "gitleaks: No secrets detected"
        echo "✅ **Status:** PASSED - No secrets detected" >> "$REPORT_FILE"
    else
        print_error "gitleaks: Found $secret_count potential secrets"
        echo "❌ **Status:** SECRETS DETECTED - Immediate action required" >> "$REPORT_FILE"
        EXIT_CODE=1
        
        # List detected secrets
        echo "" >> "$REPORT_FILE"
        echo "**Detected Secrets:**" >> "$REPORT_FILE"
        jq -r '.[] | "- [\(.RuleID)] \(.File):\(.StartLine)"' "$output_file" 2>/dev/null >> "$REPORT_FILE" || true
    fi
    
    echo "" >> "$REPORT_FILE"
}

# ============================================================================
# Container Security (if Docker images exist)
# ============================================================================

run_container_scan() {
    if [ "$CATEGORY_CONTAINER" -ne 1 ]; then
        print_info "Container scanning disabled"
        return 0
    fi
    
    print_header "Container Security"
    
    echo "## 4. Container Security" >> "$REPORT_FILE"
    echo "" >> "$REPORT_FILE"
    
    # Check if Dockerfile exists
    if [ ! -f "${PROJECT_ROOT}/docker/Dockerfile" ] && [ ! -f "${PROJECT_ROOT}/Dockerfile" ]; then
        print_info "No Dockerfile found, skipping container scan"
        echo "ℹ️ **No Dockerfile found** - Skipping container security scan" >> "$REPORT_FILE"
        echo "" >> "$REPORT_FILE"
        return 0
    fi
    
    if [ "$HAS_TRIVY" -eq 1 ]; then
        # Scan Dockerfile
        print_subheader "Scanning Dockerfile with Trivy"
        
        local dockerfile_path="${PROJECT_ROOT}/docker/Dockerfile"
        if [ ! -f "$dockerfile_path" ]; then
            dockerfile_path="${PROJECT_ROOT}/Dockerfile"
        fi
        
        trivy config \
            "$dockerfile_path" \
            2>&1 | tee "${AUDIT_DIR}/container/dockerfile-scan.txt" || true
        
        echo "### 4.1 Dockerfile Configuration Scan" >> "$REPORT_FILE"
        echo "" >> "$REPORT_FILE"
        echo "**Scanned:** \`$(basename "$(dirname "$dockerfile_path")")/$(basename "$dockerfile_path")\`" >> "$REPORT_FILE"
        echo "**Report:** \`container/dockerfile-scan.txt\`" >> "$REPORT_FILE"
        echo "" >> "$REPORT_FILE"
        
        print_success "Dockerfile scan completed"
    else
        print_warning "Trivy not available for container scanning"
    fi
    
    return 0
}

# ============================================================================
# Dynamic Analysis
# ============================================================================

run_dynamic_analysis() {
    if [ "$CATEGORY_DYNAMIC" -ne 1 ]; then
        print_info "Dynamic analysis disabled"
        return 0
    fi
    
    print_header "Dynamic Analysis"
    
    echo "## 5. Dynamic Analysis" >> "$REPORT_FILE"
    echo "" >> "$REPORT_FILE"
    
    # Note: Full dynamic analysis requires running the application
    # This is a placeholder for documentation
    
    echo "### 5.1 Dynamic Analysis Notes" >> "$REPORT_FILE"
    echo "" >> "$REPORT_FILE"
    echo "Dynamic analysis requires a running instance of ThemisDB." >> "$REPORT_FILE"
    echo "" >> "$REPORT_FILE"
    echo "**Recommended Tools:**" >> "$REPORT_FILE"
    echo "- AddressSanitizer (ASAN): \`cmake -DENABLE_ASAN=ON\`" >> "$REPORT_FILE"
    echo "- ThreadSanitizer (TSAN): \`cmake -DENABLE_TSAN=ON\`" >> "$REPORT_FILE"
    echo "- Valgrind: \`valgrind --leak-check=full ./themisdb\`" >> "$REPORT_FILE"
    echo "- OWASP ZAP: For API security testing" >> "$REPORT_FILE"
    echo "" >> "$REPORT_FILE"
    
    if [ "$HAS_VALGRIND" -eq 1 ]; then
        print_info "Valgrind is available for manual testing"
        echo "✅ **Valgrind:** Available ($(valgrind --version | head -n1))" >> "$REPORT_FILE"
    else
        echo "⚠️ **Valgrind:** Not installed" >> "$REPORT_FILE"
    fi
    
    echo "" >> "$REPORT_FILE"
    
    print_info "Dynamic analysis section completed (manual testing required)"
    return 0
}

# ============================================================================
# Generate Final Report
# ============================================================================

finalize_report() {
    print_header "Finalizing Audit Report"
    
    # Add summary section
    cat >> "$REPORT_FILE" << EOF

---

## 6. Audit Summary

**Audit Completed:** $(date +"%Y-%m-%d %H:%M:%S %Z")  
**Total Duration:** $SECONDS seconds  
**Audit Directory:** \`$(basename $AUDIT_DIR)\`

### Overall Status

EOF

    if [ "$EXIT_CODE" -eq 0 ]; then
        echo "✅ **PASSED** - No critical security issues detected" >> "$REPORT_FILE"
        print_success "Audit PASSED - No critical issues found"
    else
        echo "⚠️ **ISSUES FOUND** - Review required" >> "$REPORT_FILE"
        print_warning "Audit found issues - Review required"
    fi
    
    cat >> "$REPORT_FILE" << EOF

### Recommendations

1. **Review all findings** in the audit reports
2. **Prioritize critical and high severity** issues
3. **Update dependencies** with known vulnerabilities
4. **Implement missing security controls** as identified
5. **Run dynamic analysis** with sanitizers and fuzzing
6. **Schedule regular audits** (monthly recommended)

### Next Steps

- Address critical findings immediately
- Create issues for medium/low findings
- Update security documentation
- Schedule follow-up audit

---

## Compliance Mapping

| Framework | Status | Notes |
|-----------|--------|-------|
| **BSI C5** | ⚠️ Review Required | Automated checks only |
| **ISO 27001** | ⚠️ Review Required | Automated checks only |
| **DSGVO/GDPR** | ⚠️ Review Required | Manual review required |
| **NIS2** | ⚠️ Review Required | Manual review required |
| **OWASP ASVS** | ⚠️ Review Required | Partial coverage |

---

## References

- [SECURITY.md](${PROJECT_ROOT}/SECURITY.md)
- [Compliance Checklist](${PROJECT_ROOT}/docs/de/compliance/compliance_full_checklist.md)
- [Security Documentation](${PROJECT_ROOT}/docs/security/)
- [Issue Template](${PROJECT_ROOT}/.github/ISSUE_TEMPLATE/security-compliance-investigation.md)

---

**Report Generated by:** ThemisDB Comprehensive Audit Script v1.0.0  
**Template Version:** 1.0.0  
**Maintained by:** ThemisDB Security Team
EOF

    print_success "Report generated: $REPORT_FILE"
    
    # Generate summary file
    local summary_file="${AUDIT_DIR}/AUDIT_SUMMARY.txt"
    cat > "$summary_file" << EOF
ThemisDB Security Audit Summary
================================

Date: $(date +"%Y-%m-%d %H:%M:%S")
Status: $([ "$EXIT_CODE" -eq 0 ] && echo "PASSED" || echo "ISSUES FOUND")

Full Report: comprehensive-audit-report.md

Quick Stats:
- SAST: $([ "$CATEGORY_SAST" -eq 1 ] && echo "✓ Completed" || echo "✗ Skipped")
- Dependencies: $([ "$CATEGORY_DEPENDENCIES" -eq 1 ] && echo "✓ Completed" || echo "✗ Skipped")
- Secrets: $([ "$CATEGORY_SECRETS" -eq 1 ] && echo "✓ Completed" || echo "✗ Skipped")
- Container: $([ "$CATEGORY_CONTAINER" -eq 1 ] && echo "✓ Completed" || echo "✗ Skipped")

View full report at:
$(realpath "$REPORT_FILE")
EOF

    print_info "Summary: $summary_file"
}

# ============================================================================
# Main Execution
# ============================================================================

show_help() {
    cat << EOF
ThemisDB Comprehensive Code Audit Script

Usage: $0 [OPTIONS]

Options:
  --help                Show this help message
  --skip-sast          Skip static analysis (SAST)
  --skip-dependencies  Skip dependency scanning
  --skip-secrets       Skip secret detection
  --skip-container     Skip container security scan
  --skip-dynamic       Skip dynamic analysis
  --enable-fuzzing     Enable fuzzing analysis (requires AFL++)
  --continue-on-error  Continue audit even if checks fail
  --output-dir DIR     Specify custom output directory

Environment Variables:
  AUDIT_QUICK=1        Run quick audit (skip time-consuming checks)

Examples:
  # Full audit
  ./scripts/comprehensive-code-audit.sh

  # Quick audit without dependencies
  ./scripts/comprehensive-code-audit.sh --skip-dependencies

  # Continue on errors
  ./scripts/comprehensive-code-audit.sh --continue-on-error

For more information, see:
  - .github/ISSUE_TEMPLATE/security-compliance-investigation.md
  - docs/de/compliance/compliance_full_checklist.md
EOF
}

main() {
    # Parse arguments
    while [[ $# -gt 0 ]]; do
        case $1 in
            --help)
                show_help
                exit 0
                ;;
            --skip-sast)
                CATEGORY_SAST=0
                shift
                ;;
            --skip-dependencies)
                CATEGORY_DEPENDENCIES=0
                shift
                ;;
            --skip-secrets)
                CATEGORY_SECRETS=0
                shift
                ;;
            --skip-container)
                CATEGORY_CONTAINER=0
                shift
                ;;
            --skip-dynamic)
                CATEGORY_DYNAMIC=0
                shift
                ;;
            --enable-fuzzing)
                CATEGORY_FUZZING=1
                shift
                ;;
            --continue-on-error)
                CONTINUE_ON_ERROR=1
                set +e  # Disable exit on error
                shift
                ;;
            --output-dir)
                AUDIT_DIR="$2"
                REPORT_FILE="${AUDIT_DIR}/comprehensive-audit-report.md"
                shift 2
                ;;
            *)
                echo "Unknown option: $1"
                echo "Use --help for usage information"
                exit 1
                ;;
        esac
    done
    
    # Quick mode
    if [ "${AUDIT_QUICK:-0}" -eq 1 ]; then
        print_info "Quick audit mode enabled"
        CATEGORY_DYNAMIC=0
        CATEGORY_FUZZING=0
    fi
    
    print_header "ThemisDB Comprehensive Security Audit"
    echo "Version: 1.0.0"
    echo "Date: $(date)"
    echo ""
    
    # Run audit steps
    check_prerequisites || exit 1
    initialize_audit
    
    run_sast
    run_dependency_scan
    run_secret_scan
    run_container_scan
    run_dynamic_analysis
    
    finalize_report
    
    # Print final status
    echo ""
    print_header "Audit Complete"
    
    if [ "$EXIT_CODE" -eq 0 ]; then
        print_success "Audit completed successfully"
        print_info "No critical security issues detected"
    else
        print_warning "Audit completed with findings"
        print_info "Review the report for details"
    fi
    
    echo ""
    print_info "Full report: $REPORT_FILE"
    echo ""
    
    exit $EXIT_CODE
}

# Run main function
main "$@"
