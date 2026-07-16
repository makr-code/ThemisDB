#!/bin/bash
# Validate ThemisDB Documentation
# Runs all documentation validation checks locally

set -e

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Get script directory
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BASE_DIR="$(dirname "$SCRIPT_DIR")"

echo -e "${BLUE}========================================${NC}"
echo -e "${BLUE}ThemisDB Documentation Validation${NC}"
echo -e "${BLUE}========================================${NC}"
echo ""

# Check if Python 3 is available
if ! command -v python3 &> /dev/null; then
    echo -e "${RED}Error: Python 3 is required but not found${NC}"
    exit 1
fi

# Install Python dependencies if needed
echo -e "${YELLOW}Checking dependencies...${NC}"
python3 -c "import yaml" 2>/dev/null || {
    echo -e "${YELLOW}Installing PyYAML...${NC}"
    pip install pyyaml
}

# Variables for tracking
LINT_FAILED=0
LINK_FAILED=0
TOC_FAILED=0
HEADER_FAILED=0
DRIFT_FAILED=0

# Run documentation linter
echo ""
echo -e "${BLUE}Running documentation linter...${NC}"
echo -e "${BLUE}--------------------------------${NC}"
if python3 "$SCRIPT_DIR/docs-lint.py" "$@"; then
    echo -e "${GREEN}✓ Documentation linting passed${NC}"
else
    echo -e "${RED}✗ Documentation linting failed${NC}"
    LINT_FAILED=1
fi

# Run link checker
echo ""
echo -e "${BLUE}Running link checker...${NC}"
echo -e "${BLUE}--------------------------------${NC}"
if python3 "$SCRIPT_DIR/link-check.py" "$@"; then
    echo -e "${GREEN}✓ Link validation passed${NC}"
else
    echo -e "${RED}✗ Link validation failed${NC}"
    LINK_FAILED=1
fi

# Run TOC validator
echo ""
echo -e "${BLUE}Running TOC validator...${NC}"
echo -e "${BLUE}--------------------------------${NC}"
if python3 "$SCRIPT_DIR/toc-check.py"; then
    echo -e "${GREEN}✓ TOC validation passed${NC}"
else
    echo -e "${RED}✗ TOC validation failed${NC}"
    TOC_FAILED=1
fi

# Run doc header checker (all mode for local validation)
echo ""
echo -e "${BLUE}Running doc header checker...${NC}"
echo -e "${BLUE}--------------------------------${NC}"
if python3 "$SCRIPT_DIR/doc-header-check.py" --mode all --repo-root "$BASE_DIR"; then
    echo -e "${GREEN}✓ Doc header check passed${NC}"
else
    echo -e "${YELLOW}⚠ Doc header check found issues (see output above)${NC}"
    HEADER_FAILED=1
fi

# Run drift detector (informational – never blocks the local check)
echo ""
echo -e "${BLUE}Running drift detector...${NC}"
echo -e "${BLUE}--------------------------------${NC}"
if python3 "$SCRIPT_DIR/drift-detector.py" --repo-root "$BASE_DIR"; then
    echo -e "${GREEN}✓ No drift detected${NC}"
else
    echo -e "${YELLOW}⚠ Drift detected in secondary docs (see output above)${NC}"
    DRIFT_FAILED=1
fi

# Summary
echo ""
echo -e "${BLUE}========================================${NC}"
echo -e "${BLUE}Validation Summary${NC}"
echo -e "${BLUE}========================================${NC}"

if [ $LINT_FAILED -eq 0 ]; then
    echo -e "${GREEN}✓ Documentation Linting: PASSED${NC}"
else
    echo -e "${RED}✗ Documentation Linting: FAILED${NC}"
fi

if [ $LINK_FAILED -eq 0 ]; then
    echo -e "${GREEN}✓ Link Validation: PASSED${NC}"
else
    echo -e "${RED}✗ Link Validation: FAILED${NC}"
fi

if [ $TOC_FAILED -eq 0 ]; then
    echo -e "${GREEN}✓ TOC Validation: PASSED${NC}"
else
    echo -e "${RED}✗ TOC Validation: FAILED${NC}"
fi

if [ $HEADER_FAILED -eq 0 ]; then
    echo -e "${GREEN}✓ Doc Header Check: PASSED${NC}"
else
    echo -e "${YELLOW}⚠ Doc Header Check: ISSUES FOUND (non-blocking for existing docs)${NC}"
fi

if [ $DRIFT_FAILED -eq 0 ]; then
    echo -e "${GREEN}✓ Drift Detection: NO DRIFT${NC}"
else
    echo -e "${YELLOW}⚠ Drift Detection: DRIFT DETECTED (informational)${NC}"
fi

echo ""

# Exit with error only for blocking checks (lint, link, TOC).
# Header check and drift detection are intentionally non-blocking:
#   - Header check: existing docs pre-date the schema; only NEW/MODIFIED
#     docs must comply (enforced via CI changed-only mode).
#   - Drift detection: informational only — maintainers decide whether to
#     update or archive a drifting doc; the tool never fails CI.
if [ $LINT_FAILED -ne 0 ] || [ $LINK_FAILED -ne 0 ] || [ $TOC_FAILED -ne 0 ]; then
    echo -e "${RED}One or more required validation checks failed.${NC}"
    echo -e "${YELLOW}Please fix the issues above before committing.${NC}"
    exit 1
else
    echo -e "${GREEN}All required validation checks passed!${NC}"
    if [ $HEADER_FAILED -ne 0 ] || [ $DRIFT_FAILED -ne 0 ]; then
        echo -e "${YELLOW}Note: Header and drift warnings found. New/modified docs must comply with the schema.${NC}"
        echo -e "${YELLOW}See docs/CONTENT_MODEL.md and docs/_standards/DOC_TEMPLATE.md for guidance.${NC}"
    fi
    exit 0
fi
