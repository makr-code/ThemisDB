#!/bin/bash
# ThemisDB Config Migration Helper Script
# This script helps reorganize configuration files according to the new structure

set -e

CONFIG_ROOT="${1:-.}/config"

if [ ! -d "$CONFIG_ROOT" ]; then
    echo "❌ Config directory not found: $CONFIG_ROOT"
    exit 1
fi

echo "🔍 ThemisDB Configuration Reorganization Helper"
echo "================================================"
echo "Root: $CONFIG_ROOT"
echo ""

# Counter
MIGRATED=0
SKIPPED=0
ERRORS=0

# Colors
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Helper functions
log_success() {
    echo -e "${GREEN}✅ $1${NC}"
    ((MIGRATED++))
}

log_skip() {
    echo -e "${YELLOW}⏭️  $1${NC}"
    ((SKIPPED++))
}

log_error() {
    echo -e "${RED}❌ $1${NC}"
    ((ERRORS++))
}

migrate_file() {
    local source="$1"
    local dest="$2"
    local dest_dir=$(dirname "$dest")

    if [ ! -f "$CONFIG_ROOT/$source" ]; then
        return 0  # File doesn't exist, skip
    fi

    # Create destination directory if needed
    mkdir -p "$CONFIG_ROOT/$dest_dir"

    # Check if destination already exists
    if [ -f "$CONFIG_ROOT/$dest" ]; then
        if cmp -s "$CONFIG_ROOT/$source" "$CONFIG_ROOT/$dest"; then
            log_skip "$source → $dest (identical, keeping backup)"
        else
            log_error "$source → $dest (destination exists with different content)"
            return 1
        fi
    else
        cp -v "$CONFIG_ROOT/$source" "$CONFIG_ROOT/$dest" >/dev/null 2>&1
        log_success "Migrated: $source → $dest"
    fi
}

create_index() {
    local category="$1"
    local dir="$2"
    
    if [ ! -d "$CONFIG_ROOT/$dir" ]; then
        return 0
    fi

    local index_file="$CONFIG_ROOT/$dir/README.md"
    
    if [ ! -f "$index_file" ]; then
        cat > "$index_file" << EOF
# $category Configuration

## Overview

This directory contains configuration files for $category.

## Files

$(ls -1 "$CONFIG_ROOT/$dir" | grep -v README.md | sed 's/^/- `/; s/$/.`/' || echo "No files")

## Usage

Import these configs in your main \`config.yaml\`:

\`\`\`yaml
# config/config.yaml
$(head -5 "$CONFIG_ROOT/$dir"/*.yaml 2>/dev/null | grep -v "^==" | grep -v "^--$" | head -3 || echo "See individual files")
\`\`\`

---
Generated: $(date)
EOF
        echo -e "${BLUE}📝 Created index: $index_file${NC}"
    fi
}

# ============================================================================
# Security Configs Migration
# ============================================================================
echo -e "${BLUE}📋 Security Configuration${NC}"
echo "---"
migrate_file "pii_patterns.yaml" "security/pii_patterns.yaml"
migrate_file "rbac_roles.json" "security/rbac_roles.json"
migrate_file "rbac_roles.yaml" "security/rbac_roles.yaml"
migrate_file "user_roles.json" "security/user_roles.json"
migrate_file "graph_protection.yaml" "security/graph_protection.yaml"
migrate_file "auth_kerberos.example.yaml" "security/auth_kerberos.example.yaml"
migrate_file "security.yaml" "security/security.yaml"
create_index "Security" "security"
echo ""

# ============================================================================
# AI/ML Configs Migration
# ============================================================================
echo -e "${BLUE}📋 AI/ML Configuration${NC}"
echo "---"
mkdir -p "$CONFIG_ROOT/ai_ml/llm"
mkdir -p "$CONFIG_ROOT/ai_ml/vision"
migrate_file "lora_training_config.yaml" "ai_ml/lora_training_config.yaml"
migrate_file "vision_config.yaml" "ai_ml/vision/config.yaml"
migrate_file "vision_licenses.yaml" "ai_ml/vision/licenses.yaml"
migrate_file "llm_system_prompts.yaml" "ai_ml/llm/system_prompts.yaml"
migrate_file "llm-models.yaml" "ai_ml/llm/models.yaml"
migrate_file "llm_config.example.yaml" "ai_ml/llm/config.example.yaml"
migrate_file "llm_config.production.yaml" "ai_ml/llm/config.production.yaml"
migrate_file "llm_extended_context.yaml" "ai_ml/llm/extended_context.yaml"
migrate_file "rag_judge.yaml" "ai_ml/rag_judge.yaml"
migrate_file "voice_assistant.yaml" "ai_ml/voice_assistant.yaml"
create_index "AI/ML" "ai_ml"
echo ""

# ============================================================================
# Compliance & Governance
# ============================================================================
echo -e "${BLUE}📋 Compliance & Governance${NC}"
echo "---"
mkdir -p "$CONFIG_ROOT/compliance/audit"
migrate_file "ethical_guidelines.yaml" "compliance/ethical_guidelines.yaml"
migrate_file "governance.yaml" "compliance/governance.yaml"
migrate_file "audit.yaml" "compliance/audit/audit.yaml"
migrate_file "ai_audit_config.yaml" "compliance/audit/ai_audit_config.yaml"
create_index "Compliance" "compliance"
echo ""

# ============================================================================
# Data Management
# ============================================================================
echo -e "${BLUE}📋 Data Management${NC}"
echo "---"
migrate_file "mime_types.yaml" "data_management/mime_types.yaml"
migrate_file "storage_redundancy.yaml" "data_management/storage_redundancy.yaml"
migrate_file "retention_policies.yaml" "data_management/retention_policies.yaml"
create_index "Data Management" "data_management"
echo ""

# ============================================================================
# Performance Tuning
# ============================================================================
echo -e "${BLUE}📋 Performance Tuning${NC}"
echo "---"
mkdir -p "$CONFIG_ROOT/performance/query_cache"
migrate_file "scaling_optimizations.yaml" "performance/scaling_optimizations.yaml"
migrate_file "acceleration.yaml" "performance/acceleration.yaml"
migrate_file "config_2ssd_performance.yaml" "performance/config_2ssd_performance.yaml"
migrate_file "config_multi_ssd.yaml" "performance/config_multi_ssd.yaml"
migrate_file "query_cache_mixed.yaml" "performance/query_cache/mixed.yaml"
migrate_file "query_cache_olap.yaml" "performance/query_cache/olap.yaml"
migrate_file "query_cache_oltp.yaml" "performance/query_cache/oltp.yaml"
create_index "Performance" "performance"
echo ""

# ============================================================================
# Networking
# ============================================================================
echo -e "${BLUE}📋 Networking${NC}"
echo "---"
migrate_file "connection_pool_config.yaml" "networking/connection_pool_config.yaml"
create_index "Networking" "networking"
echo ""

# ============================================================================
# Content & Processing
# ============================================================================
echo -e "${BLUE}📋 Content & Processing${NC}"
echo "---"
mkdir -p "$CONFIG_ROOT/content/processors"
migrate_file "content_processors.yaml" "content/processors.yaml"
migrate_file "fem_edge_type_defaults.yaml" "content/fem_edge_type_defaults.yaml"
create_index "Content" "content"
echo ""

# ============================================================================
# Monitoring & Observability
# ============================================================================
echo -e "${BLUE}📋 Monitoring${NC}"
echo "---"
mkdir -p "$CONFIG_ROOT/monitoring/prometheus"
migrate_file "prometheus-arm.yml" "monitoring/prometheus/arm.yml"
migrate_file "prometheus_ethics.yml" "monitoring/prometheus/ethics.yml"
create_index "Monitoring" "monitoring"
echo ""

# ============================================================================
# Distributed Systems
# ============================================================================
echo -e "${BLUE}📋 Distributed Systems${NC}"
echo "---"
mkdir -p "$CONFIG_ROOT/distributed/replication"
mkdir -p "$CONFIG_ROOT/distributed/sharding"
migrate_file "replication.example.yaml" "distributed/replication/basic.example.yaml"
migrate_file "replication-ha.example.yaml" "distributed/replication/ha.example.yaml"
migrate_file "sharding-with-metrics.yaml" "distributed/sharding/with-metrics.yaml"
create_index "Distributed" "distributed"
echo ""

# ============================================================================
# Summary Report
# ============================================================================
echo ""
echo "📊 Migration Summary"
echo "================================================"
echo -e "  ${GREEN}Migrated: $MIGRATED${NC}"
echo -e "  ${YELLOW}Skipped:  $SKIPPED${NC}"
echo -e "  ${RED}Errors:   $ERRORS${NC}"
echo ""

if [ $ERRORS -eq 0 ]; then
    echo -e "${GREEN}✅ Migration completed successfully!${NC}"
    echo ""
    echo "📝 Next Steps:"
    echo "1. Review migrated files in each subdirectory"
    echo "2. Update your load paths to use new structure"
    echo "3. Test your application startup"
    echo "4. Check logs for any deprecation warnings"
    echo ""
    echo "📚 For more info: config/MIGRATION_GUIDE.md"
else
    echo -e "${RED}⚠️  Migration completed with errors!${NC}"
    echo ""
    echo "⚠️  Please review errors above and fix manually if needed."
fi

echo ""
