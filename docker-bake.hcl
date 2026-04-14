# docker-bake.hcl - Multiarch build configuration with persistent caching
# Usage:
#   docker buildx bake -f docker-bake.hcl themisdb-community
#   docker buildx bake -f docker-bake.hcl themisdb-enterprise
#   docker buildx bake -f docker-bake.hcl themisdb-all

variable "REGISTRY" {
  default = "docker.io"
}

variable "DOCKER_NAMESPACE" {
  default = "themisdb"
}

variable "IMAGE_NAME" {
  default = "themisdb"
}

variable "TAG" {
  default = "latest"
}

variable "BUILDKIT_CONTEXT_KEEP_GIT_DIR" {
  default = "false"
}

variable "CACHE_FROM_TYPE" {
  default = "type=local,src=.buildx-cache-new"
}

variable "CACHE_TO_TYPE" {
  default = "type=local,dest=.buildx-cache-new,mode=max"
}

# ============================================================================
# COMMUNITY Edition - Full LLM + GPU support, smaller footprint
# ============================================================================
target "themisdb-community" {
  dockerfile = "Dockerfile"
  args = {
    THEMIS_EDITION = "COMMUNITY"
    ENABLE_LLM     = "ON"
    ENABLE_GPU     = "ON"
  }
  platforms = ["linux/amd64", "linux/arm64"]
  tags = [
    "${REGISTRY}/${DOCKER_NAMESPACE}/${IMAGE_NAME}:${TAG}-community"
  ]
  cache-from = ["${CACHE_FROM_TYPE}"]
  cache-to   = ["${CACHE_TO_TYPE}"]
  output     = ["type=oci,dest=./build-output-community"]
}

# ============================================================================
# ENTERPRISE Edition - Full features + advanced optimizations
# ============================================================================
target "themisdb-enterprise" {
  dockerfile = "Dockerfile"
  args = {
    THEMIS_EDITION = "ENTERPRISE"
    ENABLE_LLM     = "ON"
    ENABLE_GPU     = "ON"
  }
  platforms = ["linux/amd64", "linux/arm64"]
  tags = [
    "${REGISTRY}/${DOCKER_NAMESPACE}/${IMAGE_NAME}:${TAG}-enterprise"
  ]
  cache-from = ["${CACHE_FROM_TYPE}"]
  cache-to   = ["${CACHE_TO_TYPE}"]
  output     = ["type=oci,dest=./build-output-enterprise"]
}

# ============================================================================
# MINIMAL Edition - CPU-only, stripped dependencies
# ============================================================================
target "themisdb-minimal" {
  dockerfile = "Dockerfile"
  args = {
    THEMIS_EDITION = "MINIMAL"
    ENABLE_LLM     = "OFF"
    ENABLE_GPU     = "OFF"
    FORCE_CPU_ONLY = "ON"
  }
  platforms = ["linux/amd64", "linux/arm64"]
  tags = [
    "${REGISTRY}/${DOCKER_NAMESPACE}/${IMAGE_NAME}:${TAG}-minimal"
  ]
  cache-from = ["${CACHE_FROM_TYPE}"]
  cache-to   = ["${CACHE_TO_TYPE}"]
  output     = ["type=oci,dest=./build-output-minimal"]
}

# ============================================================================
# HYPERSCALER Edition - GPU + distributed features
# ============================================================================
target "themisdb-hyperscaler" {
  dockerfile = "Dockerfile"
  args = {
    THEMIS_EDITION = "HYPERSCALER"
    ENABLE_LLM     = "ON"
    ENABLE_GPU     = "ON"
  }
  platforms = ["linux/amd64", "linux/arm64"]
  tags = [
    "${REGISTRY}/${DOCKER_NAMESPACE}/${IMAGE_NAME}:${TAG}-hyperscaler"
  ]
  cache-from = ["${CACHE_FROM_TYPE}"]
  cache-to   = ["${CACHE_TO_TYPE}"]
  output     = ["type=oci,dest=./build-output-hyperscaler"]
}

# ============================================================================
# Build all editions
# ============================================================================
target "themisdb-all" {
  inherits = ["themisdb-community", "themisdb-enterprise", "themisdb-minimal", "themisdb-hyperscaler"]
}

# ============================================================================
# Debug target - single platform, faster iteration, output to docker daemon
# ============================================================================
target "themisdb-debug" {
  dockerfile = "Dockerfile"
  args = {
    THEMIS_EDITION = "COMMUNITY"
    ENABLE_LLM     = "ON"
    ENABLE_GPU     = "ON"
  }
  platforms = ["linux/amd64"]
  tags = [
    "${IMAGE_NAME}:debug"
  ]
  cache-from = ["${CACHE_FROM_TYPE}"]
  cache-to   = ["${CACHE_TO_TYPE}"]
  output     = ["type=docker"]
}
