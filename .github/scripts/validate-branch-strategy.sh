#!/bin/bash
# Branch Strategy Validation Script
# Validates that PRs follow the correct Git Flow branch strategy

set -e

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# Function to print colored messages
error() {
    echo -e "${RED}❌ ERROR: $1${NC}" >&2
}

warning() {
    echo -e "${YELLOW}⚠️  WARNING: $1${NC}"
}

success() {
    echo -e "${GREEN}✅ $1${NC}"
}

info() {
    echo "ℹ️  $1"
}

# Get branch names from arguments or environment
BASE_BRANCH="${1:-${BASE_BRANCH}}"
HEAD_BRANCH="${2:-${HEAD_BRANCH}}"

if [ -z "$BASE_BRANCH" ] || [ -z "$HEAD_BRANCH" ]; then
    error "Usage: $0 <base-branch> <head-branch>"
    error "Or set BASE_BRANCH and HEAD_BRANCH environment variables"
    exit 1
fi

info "Validating PR: ${HEAD_BRANCH} → ${BASE_BRANCH}"

# Validation rules
VALID=true

# Rule 1: feature/* and bugfix/* must target develop
if [[ "$HEAD_BRANCH" =~ ^(feature|bugfix)/ ]]; then
    if [ "$BASE_BRANCH" != "develop" ]; then
        error "Feature and bugfix branches must target 'develop'"
        error "Current target: '${BASE_BRANCH}'"
        VALID=false
    else
        success "Feature/bugfix branch correctly targets develop"
    fi
fi

# Rule 2: release/* must target main (for initial merge)
if [[ "$HEAD_BRANCH" =~ ^release/ ]]; then
    if [ "$BASE_BRANCH" = "main" ]; then
        success "Release branch correctly targets main for production release"
    elif [ "$BASE_BRANCH" = "develop" ]; then
        warning "Release branch targeting develop - this should be for back-merge after release"
    else
        error "Release branches should target 'main' (for release) or 'develop' (for back-merge)"
        error "Current target: '${BASE_BRANCH}'"
        VALID=false
    fi
fi

# Rule 3: hotfix/* must target main
if [[ "$HEAD_BRANCH" =~ ^hotfix/ ]]; then
    if [ "$BASE_BRANCH" != "main" ]; then
        error "Hotfix branches must target 'main'"
        error "Current target: '${BASE_BRANCH}'"
        error "After merging to main, hotfix should be synced back to develop"
        VALID=false
    else
        success "Hotfix branch correctly targets main"
    fi
fi

# Rule 4: Direct pushes to main should only come from release/* or hotfix/*
if [ "$BASE_BRANCH" = "main" ]; then
    if [[ ! "$HEAD_BRANCH" =~ ^(release|hotfix)/ ]]; then
        warning "PRs to main should typically come from release/* or hotfix/* branches"
        warning "Current branch: ${HEAD_BRANCH}"
    fi
fi

# Rule 5: Validate branch naming conventions
if [[ ! "$HEAD_BRANCH" =~ ^(feature|bugfix|hotfix|release|main|develop)(/|$) ]]; then
    warning "Branch name '${HEAD_BRANCH}' doesn't follow Git Flow conventions"
    warning "Expected patterns: feature/*, bugfix/*, release/*, hotfix/*"
fi

# Exit with appropriate status
if [ "$VALID" = true ]; then
    success "Branch strategy validation passed"
    exit 0
else
    error "Branch strategy validation failed"
    exit 1
fi
