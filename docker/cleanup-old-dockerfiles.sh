#!/bin/bash
# ============================================================================
# Docker Files Cleanup Script
# ============================================================================
# Löscht überflüssige Dockerfiles, die durch Dockerfile.unified ersetzt wurden
#
# WARNUNG: Erstellt Backup vor dem Löschen!
#
# Usage:
#   ./cleanup-old-dockerfiles.sh [--dry-run]
#
# Options:
#   --dry-run    Zeigt was gelöscht würde, ohne zu löschen

set -e

DRY_RUN=false
if [ "$1" = "--dry-run" ]; then
    DRY_RUN=true
fi

DOCKER_DIR="$(cd "$(dirname "$0")" && pwd)"
BACKUP_DIR="${DOCKER_DIR}/backup-$(date +%Y%m%d-%H%M%S)"

echo "============================================"
echo "ThemisDB Docker Files Cleanup"
echo "============================================"
echo "Docker Dir:  ${DOCKER_DIR}"
if [ "$DRY_RUN" = true ]; then
    echo "Mode:        DRY RUN (no files deleted)"
else
    echo "Backup Dir:  ${BACKUP_DIR}"
fi
echo "============================================"
echo ""

# Liste der zu löschenden Dockerfiles
OBSOLETE_DOCKERFILES=(
    # Edition-spezifisch (ersetzt durch Dockerfile.unified)
    "Dockerfile.hyperscaler"
    "Dockerfile.hyperscaler-simple"
    "Dockerfile.hyperscaler-runtime"
    "Dockerfile.minimal"
    "Dockerfile.minimal-fast"
    "Dockerfile.themis-server"
    
    # Fast/Quick Builds (ersetzt durch Dockerfile.dev)
    "Dockerfile.fast"
    "Dockerfile.prebuilt"
    "Dockerfile.prebuild"
    "Dockerfile.quick"
    "Dockerfile.quick-linux"
    "Dockerfile.optimized-local"
    
    # Build/Runtime
    "Dockerfile.build-in-docker"
    "Dockerfile.runtime"
    "Dockerfile.simple"
    "Dockerfile.docker-deploy"
    
    # Release
    "Dockerfile.release"
    "Dockerfile.release-packages"
)

# Optional: Dockerfile.benchmark (nach manueller Prüfung)
OPTIONAL_DOCKERFILES=(
    "Dockerfile.benchmark"
)

# Count files
TOTAL_FILES=$((${#OBSOLETE_DOCKERFILES[@]} + ${#OPTIONAL_DOCKERFILES[@]}))
EXISTING_COUNT=0

# Check which files exist
echo "Analyzing files..."
for file in "${OBSOLETE_DOCKERFILES[@]}" "${OPTIONAL_DOCKERFILES[@]}"; do
    if [ -f "${DOCKER_DIR}/${file}" ]; then
        EXISTING_COUNT=$((EXISTING_COUNT + 1))
    fi
done

echo "Found ${EXISTING_COUNT} files to remove (out of ${TOTAL_FILES})"
echo ""

if [ "$EXISTING_COUNT" -eq 0 ]; then
    echo "✅ No files to clean up. Already clean!"
    exit 0
fi

if [ "$DRY_RUN" = false ]; then
    # Create backup
    echo "Creating backup..."
    mkdir -p "${BACKUP_DIR}"
    
    for file in "${OBSOLETE_DOCKERFILES[@]}" "${OPTIONAL_DOCKERFILES[@]}"; do
        if [ -f "${DOCKER_DIR}/${file}" ]; then
            cp "${DOCKER_DIR}/${file}" "${BACKUP_DIR}/"
        fi
    done
    
    echo "✅ Backup created: ${BACKUP_DIR}"
    echo ""
fi

# Delete obsolete files
echo "Removing obsolete Dockerfiles..."
echo ""

DELETED_COUNT=0

for file in "${OBSOLETE_DOCKERFILES[@]}"; do
    if [ -f "${DOCKER_DIR}/${file}" ]; then
        echo "  🗑️  ${file}"
        if [ "$DRY_RUN" = false ]; then
            rm "${DOCKER_DIR}/${file}"
            DELETED_COUNT=$((DELETED_COUNT + 1))
        fi
    fi
done

echo ""
echo "Optional files (require manual review):"
for file in "${OPTIONAL_DOCKERFILES[@]}"; do
    if [ -f "${DOCKER_DIR}/${file}" ]; then
        echo "  ⚠️  ${file} (review before deleting)"
    fi
done

echo ""
echo "============================================"

if [ "$DRY_RUN" = true ]; then
    echo "DRY RUN completed"
    echo "Would remove: ${EXISTING_COUNT} files"
    echo ""
    echo "To actually delete files, run:"
    echo "  ./cleanup-old-dockerfiles.sh"
else
    echo "✅ Cleanup completed"
    echo "Removed:  ${DELETED_COUNT} files"
    echo "Backup:   ${BACKUP_DIR}"
    echo ""
    echo "Remaining Dockerfiles:"
    ls -1 "${DOCKER_DIR}"/Dockerfile* | wc -l | xargs echo "  Total:"
    echo ""
    echo "Core files (should remain):"
    echo "  ✅ Dockerfile.unified"
    echo "  ✅ Dockerfile.dev"
    echo "  ✅ Dockerfile.vcpkg-base"
    echo "  ✅ Dockerfile.vcpkg-deps"
    echo "  ✅ Dockerfile.llama-base"
    echo "  ✅ Dockerfile (legacy reference)"
    echo ""
    echo "Special purpose (should remain):"
    echo "  ✅ Dockerfile.qnap"
    echo "  ✅ Dockerfile.qnap.build"
    echo "  ✅ Dockerfile.qnap.runtime"
    echo "  ✅ Dockerfile.wire-protocol"
    echo "  ✅ Dockerfile.llm-raid-tests"
fi

echo ""
echo "Next steps:"
echo "  1. Test builds with Dockerfile.unified"
echo "  2. Review Dockerfile.benchmark manually"
echo "  3. Update CI/CD to use new Dockerfiles"
echo "  4. Update README.md with new build commands"
echo ""
echo "Rollback (if needed):"
if [ "$DRY_RUN" = false ]; then
    echo "  cp ${BACKUP_DIR}/* ${DOCKER_DIR}/"
fi

exit 0
