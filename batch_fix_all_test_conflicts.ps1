#!/usr/bin/env powershell
# batch_fix_all_test_conflicts.ps1
# Systematically fixes all remaining TEST/TEST_F cross-file fixture conflicts

# Map of suite names and their conflict status
$conflict_map = @(
    @{ suite = 'AQLWithClauseTest';                 search_pattern = '^TEST\(AQLWithClauseTest,'; replace_with = 'TEST(AQLWithClauseStatelessTest,'; file = 'tests/test_config_coverage.cpp' }
    @{ suite = 'SafeFileBackendTest';               search_pattern = '^TEST\(SafeFileBackendTest,'; replace_with = 'TEST(SafeFileBackendStatelessTest,'; file = 'tests/test_config_coverage.cpp' }
    @{ suite = 'SubqueryTest';                      search_pattern = '^TEST\(SubqueryTest,'; replace_with = 'TEST(SubqueryStatelessTest,'; file = 'tests/test_config_coverage.cpp' }
    @{ suite = 'AQLSimilarityTest';                 search_pattern = '^TEST\(AQLSimilarityTest,'; replace_with = 'TEST(AQLSimilarityStatelessTest,'; file = 'tests/test_config_coverage.cpp' }
    @{ suite = 'LRUCacheTest';                      search_pattern = '^TEST\(LRUCacheTest,'; replace_with = 'TEST(LRUCacheStatelessTest,'; file = 'tests/test_config_coverage.cpp' }
    @{ suite = 'BackupManagerEnhancedTest';         search_pattern = '^TEST\(BackupManagerEnhancedTest,'; replace_with = 'TEST(BackupManagerEnhancedStatelessTest,'; file = 'tests/test_config_coverage.cpp' }
    @{ suite = 'BackupRestoreIntegrationTest';      search_pattern = '^TEST\(BackupRestoreIntegrationTest,'; replace_with = 'TEST(BackupRestoreIntegrationStatelessTest,'; file = 'tests/test_config_coverage.cpp' }
    @{ suite = 'BufferBinaryProtocolTest';          search_pattern = '^TEST\(BufferBinaryProtocolTest,'; replace_with = 'TEST(BufferBinaryProtocolStatelessTest,'; file = 'tests/test_config_coverage.cpp' }
    @{ suite = 'FilesystemBlobBackendTest';         search_pattern = '^TEST\(FilesystemBlobBackendTest,'; replace_with = 'TEST(FilesystemBlobBackendStatelessTest,'; file = 'tests/test_config_coverage.cpp' }
    @{ suite = 'DirectXBackendTest';                search_pattern = '^TEST\(DirectXBackendTest,'; replace_with = 'TEST(DirectXBackendStatelessTest,'; file = 'tests/test_config_coverage.cpp' }
    @{ suite = 'GraphQLWsHandlerTest';              search_pattern = '^TEST\(GraphQLWsHandlerTest,'; replace_with = 'TEST(GraphQLWsHandlerStatelessTest,'; file = 'tests/test_config_coverage.cpp' }
    @{ suite = 'ArrowLibIntegrationTest';           search_pattern = '^TEST\(ArrowLibIntegrationTest,'; replace_with = 'TEST(ArrowLibIntegrationStatelessTest,'; file = 'tests/test_config_coverage.cpp' }
    @{ suite = 'LoRAProvenanceManagerTest';         search_pattern = '^TEST\(LoRAProvenanceManagerTest,'; replace_with = 'TEST(LoRAProvenanceManagerStatelessTest,'; file = 'tests/test_config_coverage.cpp' }
    @{ suite = 'BiasAuditReportTest';               search_pattern = '^TEST\(BiasAuditReportTest,'; replace_with = 'TEST(BiasAuditReportStatelessTest,'; file = 'tests/test_config_coverage.cpp' }
)

$workspace_root = 'C:\VCC\themis'
$replacements = 0

foreach ($item in $conflict_map) {
    $file_path = Join-Path $workspace_root $item.file
    
    if (-not (Test-Path $file_path)) {
        Write-Host "WARNING: File not found: $file_path" -ForegroundColor Yellow
        continue
    }
    
    $content = Get-Content -Path $file_path -Raw
    $count_before = ($content | Select-String -Pattern $item.search_pattern | Measure-Object).Count
    
    if ($count_before -gt 0) {
        $content = $content -replace $item.search_pattern, $item.replace_with
        Set-Content -Path $file_path -Value $content -Encoding UTF8
        
        Write-Host "✓ $($item.suite): Renamed $count_before tests in $(Split-Path -Leaf $file_path)" -ForegroundColor Green
        $replacements += $count_before
    }
}

Write-Host "`nTotal: Fixed $replacements tests" -ForegroundColor Cyan
