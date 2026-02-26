#!/usr/bin/env bash
# ThemisDB OpenAPI SDK Generator
#
# Generates client SDKs for Python, JavaScript, and Go from the OpenAPI spec
# using openapi-generator-cli.
#
# Usage:
#   ./scripts/generate-sdks.sh [--python] [--javascript] [--go] [--all]
#   ./scripts/generate-sdks.sh           # generates all SDKs (default)
#
# Requirements:
#   - Docker (recommended): docker pull openapitools/openapi-generator-cli
#   - OR Java 11+: download openapi-generator-cli.jar from
#     https://repo1.maven.org/maven2/org/openapitools/openapi-generator-cli/
#
# Output:
#   openapi/generated/python/      - Python SDK
#   openapi/generated/javascript/  - JavaScript/TypeScript SDK
#   openapi/generated/go/          - Go SDK

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "${SCRIPT_DIR}/.." && pwd)"
OPENAPI_SPEC="${REPO_ROOT}/openapi/openapi.yaml"
OUTPUT_DIR="${REPO_ROOT}/openapi/generated"

# Generator version (pinned for reproducibility)
GENERATOR_VERSION="7.10.0"

# Package metadata
PACKAGE_NAME="themisdb"
PACKAGE_VERSION="0.1.0"
MODULE_NAME="themisdb_client"
GO_MODULE="github.com/makr-code/ThemisDB/openapi/generated/go"

# Color helpers
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m'

info()    { echo -e "${GREEN}[generate-sdks]${NC} $*"; }
warn()    { echo -e "${YELLOW}[generate-sdks]${NC} $*"; }
error()   { echo -e "${RED}[generate-sdks]${NC} $*" >&2; }

# ---------------------------------------------------------------------------
# Resolve generator invocation (Docker preferred, Java fallback)
# ---------------------------------------------------------------------------
GENERATOR_CMD=""
JAR_PATH="/tmp/openapi-generator-cli-${GENERATOR_VERSION}.jar"

detect_generator() {
    if command -v docker &>/dev/null && docker info &>/dev/null 2>&1; then
        info "Using Docker-based openapi-generator-cli ${GENERATOR_VERSION}"
        GENERATOR_CMD="docker run --rm \
            -v \"${REPO_ROOT}:/local\" \
            openapitools/openapi-generator-cli:v${GENERATOR_VERSION} generate"
        SPEC_PATH="/local/openapi/openapi.yaml"
        OUT_PREFIX="/local/openapi/generated"
    elif [ -f "${JAR_PATH}" ]; then
        info "Using local JAR ${JAR_PATH}"
        GENERATOR_CMD="java -jar ${JAR_PATH} generate"
        SPEC_PATH="${OPENAPI_SPEC}"
        OUT_PREFIX="${OUTPUT_DIR}"
    elif command -v java &>/dev/null; then
        warn "openapi-generator-cli JAR not found at ${JAR_PATH}."
        warn "Attempting download from Maven Central..."
        MAVEN_URL="https://repo1.maven.org/maven2/org/openapitools/openapi-generator-cli/${GENERATOR_VERSION}/openapi-generator-cli-${GENERATOR_VERSION}.jar"
        if curl -fsSL -o "${JAR_PATH}" "${MAVEN_URL}"; then
            info "Downloaded openapi-generator-cli-${GENERATOR_VERSION}.jar"
            GENERATOR_CMD="java -jar ${JAR_PATH} generate"
            SPEC_PATH="${OPENAPI_SPEC}"
            OUT_PREFIX="${OUTPUT_DIR}"
        else
            error "Download failed. Please provide Docker or download the JAR manually:"
            error "  curl -fsSL -o ${JAR_PATH} ${MAVEN_URL}"
            exit 1
        fi
    else
        error "No suitable generator found. Install Docker or Java 11+."
        exit 1
    fi
}

# ---------------------------------------------------------------------------
# Individual language generators
# ---------------------------------------------------------------------------
generate_python() {
    info "Generating Python SDK → ${OUTPUT_DIR}/python/"
    local out="${OUT_PREFIX}/python"
    eval "${GENERATOR_CMD}" \
        -i "${SPEC_PATH}" \
        -g python \
        -o "${out}" \
        --package-name "${MODULE_NAME}" \
        --additional-properties="packageName=${MODULE_NAME},projectName=${PACKAGE_NAME}-client,packageVersion=${PACKAGE_VERSION},library=urllib3" \
        --git-repo-id="ThemisDB" \
        --git-user-id="makr-code"
    info "Python SDK generated successfully."
}

generate_javascript() {
    info "Generating JavaScript/TypeScript SDK → ${OUTPUT_DIR}/javascript/"
    local out="${OUT_PREFIX}/javascript"
    eval "${GENERATOR_CMD}" \
        -i "${SPEC_PATH}" \
        -g typescript-fetch \
        -o "${out}" \
        --additional-properties="npmName=@themisdb/openapi-client,npmVersion=${PACKAGE_VERSION},supportsES6=true,withInterfaces=true" \
        --git-repo-id="ThemisDB" \
        --git-user-id="makr-code"
    info "JavaScript/TypeScript SDK generated successfully."
}

generate_go() {
    info "Generating Go SDK → ${OUTPUT_DIR}/go/"
    local out="${OUT_PREFIX}/go"
    eval "${GENERATOR_CMD}" \
        -i "${SPEC_PATH}" \
        -g go \
        -o "${out}" \
        --additional-properties="packageName=themisdbclient,moduleName=${GO_MODULE},packageVersion=${PACKAGE_VERSION},withGoMod=true" \
        --git-repo-id="ThemisDB" \
        --git-user-id="makr-code"
    info "Go SDK generated successfully."
}

# ---------------------------------------------------------------------------
# Main
# ---------------------------------------------------------------------------
GEN_PYTHON=false
GEN_JAVASCRIPT=false
GEN_GO=false

parse_args() {
    if [ $# -eq 0 ]; then
        GEN_PYTHON=true
        GEN_JAVASCRIPT=true
        GEN_GO=true
        return
    fi
    for arg in "$@"; do
        case "${arg}" in
            --python)      GEN_PYTHON=true ;;
            --javascript)  GEN_JAVASCRIPT=true ;;
            --go)          GEN_GO=true ;;
            --all)
                GEN_PYTHON=true
                GEN_JAVASCRIPT=true
                GEN_GO=true
                ;;
            *)
                error "Unknown argument: ${arg}"
                echo "Usage: $0 [--python] [--javascript] [--go] [--all]"
                exit 1
                ;;
        esac
    done
}

main() {
    parse_args "$@"

    if [ ! -f "${OPENAPI_SPEC}" ]; then
        error "OpenAPI spec not found: ${OPENAPI_SPEC}"
        exit 1
    fi

    detect_generator
    mkdir -p "${OUTPUT_DIR}"

    ${GEN_PYTHON}     && generate_python
    ${GEN_JAVASCRIPT} && generate_javascript
    ${GEN_GO}         && generate_go

    info "SDK generation complete. Output: ${OUTPUT_DIR}/"
}

main "$@"
