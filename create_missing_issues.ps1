#!/usr/bin/env pwsh
<#
Create missing GitHub Issues from templates
#>

Push-Location "C:\VCC\themis"

Write-Host "=== Erstelle 3 fehlende GitHub Issues ===" -ForegroundColor Cyan

$issues = @(
    @{
        title = "[FEATURE] Phase 1: Named Snapshots (Semantic Tagging)"
        labels = "type:enhancement,area:storage,priority:P0,milestone:current"
        body = "Implement Named Snapshots feature for ThemisDB's MVCC system, enabling semantic tagging of important database states for disaster recovery, compliance, and schema migration rollback scenarios.

## 🎯 Objectives
- Semantic tagging of database states with human-readable names
- Persistent tag storage in RocksDB
- REST API for tag management (CRUD operations)
- Foundation for Point-in-Time Recovery

## ✅ Success Criteria
- SnapshotManager can create, read, update, delete tags
- Tags are persistent (survive DB restart)
- REST API functions correctly
- Test Coverage >= 95%
- Performance benchmarks met
- Documentation complete (EN + DE)

## 📋 Implementation Tasks
See template at .github/ISSUE_TEMPLATE/git_features_phase1_named_snapshots.md for full details.

**Estimated Duration:** 3-4 weeks
**Priority:** ⭐⭐⭐ Highest
**Risk:** 🟢 Low"
    },
    @{
        title = "[FEATURE] Phase 2: Diff API (Structured Diff)"
        labels = "type:enhancement,area:storage,area:api,priority:P1,milestone:next"
        body = "Implement a structured Diff API for ThemisDB's MVCC system, enabling detailed comparison of database states between two time points for audit reports, debugging, and compliance tracking.

## 🎯 Objectives
- Structured diffs between any two database states
- Filtering capabilities by table, entity type, key prefix
- Pagination support for large diff results
- Performance optimization (<100ms for 10K changes)

## ✅ Success Criteria
- Diff between arbitrary sequence numbers
- Diff between named tags
- Diff between timestamps
- Filtering works (table, key prefix)
- Pagination works correctly
- Performance: <100ms for 10K changes, <1s for 100K changes
- Test Coverage >= 95%
- Documentation complete (EN + DE)

## 📋 Implementation Tasks
See template at .github/ISSUE_TEMPLATE/git_features_phase2_diff_api.md for full details.

**Estimated Duration:** 3-4 weeks
**Priority:** ⭐⭐ Medium-High
**Risk:** 🟢 Low
**Dependencies:** Phase 1 (Named Snapshots) must be completed first"
    },
    @{
        title = "[FEATURE] Phase 3: Point-in-Time Recovery (PITR)"
        labels = "type:enhancement,area:storage,priority:P0,milestone:future"
        body = "Implement Point-in-Time Recovery for ThemisDB's MVCC system with comprehensive safety features, enabling reliable database restoration to any previous state for disaster recovery, corruption recovery, and accidental deletion scenarios.

## 🎯 Objectives
- Safe restoration to any point in time
- Automatic backup before restore operations
- Dry-run mode for preview before execution
- Selective restore (specific tables only)
- Robust error handling with automatic rollback

## ✅ Success Criteria
- Restore to sequence number works
- Restore to named tag works
- Restore to timestamp works
- Automatic backup created before restore
- Dry-run provides accurate preview
- Rollback works on failure
- Selective restore works (table filtering)
- Progress tracking functional
- Test Coverage >= 95%
- Disaster Recovery guide complete

## 📋 Implementation Tasks
See template at .github/ISSUE_TEMPLATE/git_features_phase3_pitr.md for full details.

**Estimated Duration:** 3-4 weeks
**Priority:** ⭐⭐⭐ Highest
**Risk:** 🟡 Medium (critical feature, requires extensive testing)
**Dependencies:** Phase 1 & Phase 2 must be completed first"
    }
)

$created = 0
$failed = 0

foreach ($issue in $issues) {
    Write-Host ""
    Write-Host "Creating: $($issue.title)" -ForegroundColor Yellow
    
    try {
        $result = gh issue create --title $issue.title --label $issue.labels --body $issue.body 2>&1
        
        if ($result -match "#\d+") {
            Write-Host "✅ Created: $result" -ForegroundColor Green
            $created++
        } else {
            Write-Host "❌ Failed to create issue" -ForegroundColor Red
            Write-Host $result
            $failed++
        }
    } catch {
        Write-Host "❌ Error: $($_.Exception.Message)" -ForegroundColor Red
        $failed++
    }
    
    Start-Sleep -Milliseconds 500
}

Write-Host ""
Write-Host "=== Zusammenfassung ===" -ForegroundColor Green
Write-Host "✅ Erstellt: $created | ❌ Fehler: $failed" -ForegroundColor Yellow
Write-Host ""

Pop-Location
