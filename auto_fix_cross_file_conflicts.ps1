# auto_fix_cross_file_conflicts.ps1
# Automatically fixes cross-file TEST/TEST_F conflicts

$conflicts = @{
    'AQLWithClauseTest' = ('test_aql_with_clause.cpp', 'test_config_coverage.cpp')
    'FilesystemBlobBackendTest' = ('test_blob_storage.cpp', 'test_config_coverage.cpp')
    'BackupRestoreIntegrationTest' = ('test_backup_restore_integration.cpp', 'test_graph_backup.cpp')
    'LoRAProvenanceManagerTest' = ('test_lora_provenance.cpp', 'test_config_coverage.cpp')
    'BiasAuditReportTest' = ('test_model_governance.cpp', 'test_config_coverage.cpp')
    'SubqueryTest' = ('test_aql_subqueries.cpp', 'test_config_coverage.cpp')
    'AQLSimilarityTest' = ('test_aql_similarity.cpp', 'test_config_coverage.cpp')
    'LRUCacheTest' = ('test_config_coverage.cpp')
    'ArrowUserRegistrationPluginTest' = ('security/test_arrow_user_registration_plugin.cpp', 'test_config_coverage.cpp')
    'BufferBinaryProtocolTest' = ('test_binary_protocol_buffer.cpp', 'test_config_coverage.cpp')
    'ArrowLibIntegrationTest' = ('test_lib_arrow_integration.cpp', 'test_config_coverage.cpp')
    'SAGAOrchestratorTest' = ('test_saga_orchestrator.cpp', 'test_config_coverage.cpp')
    'AccessControlTest' = ('test_access_control.cpp', 'test_config_coverage.cpp')
    'DirectXBackendTest' = ('test_directx_backend.cpp', 'test_config_coverage.cpp')
    'BlobStorageManagerTest' = ('test_blob_storage.cpp', 'test_config_coverage.cpp')
    'VulkanBackendTest' = ('test_vulkan_backend.cpp', 'test_config_coverage.cpp')
    'ReviewSchedulerTest' = ('test_config_coverage.cpp')
    'OpenCLErasureCoderParityTest' = ('test_opencl_erasure_coder_parity.cpp', 'test_config_coverage.cpp')
    'HealthCheckTest' = ('test_config_coverage.cpp')
    'GraphQLWsHandlerTest' = ('test_graphql_ws_handler.cpp', 'test_config_coverage.cpp')
    'GPUSafeFailTest' = ('test_config_coverage.cpp')
    'ParallelReplicationWorkerTest' = ('test_config_coverage.cpp')
    'TracingMiddlewareTest' = ('test_config_coverage.cpp')
    'PrometheusMetricsTest' = ('test_config_coverage.cpp')
}

Write-Host "Strategy: All TEST/TEST_F conflicts in test_config_coverage.cpp will be renamed to *NoFixture suffix"
Write-Host "This keeps dedicated fixture classes in primary files intact."
Write-Host ""

# The main fix file
$fix_file = 'C:\VCC\themis\tests\test_config_coverage.cpp'
$content = Get-Content -Path $fix_file -Raw

$replacements = @()
foreach ($suite in $conflicts.Keys) {
    $files = $conflicts[$suite]
    # If test_config_coverage is in the list, rename those tests
    if ($files -contains 'test_config_coverage.cpp') {
        # Replace TEST(SuiteName, with TEST(SuiteNameNoFixture,
        $old_pattern = "TEST\($suite,"
        $new_pattern = "TEST(${suite}NoFixture,"
        
        $count = ($content | Select-String -Pattern [regex]::Escape($old_pattern) | Measure-Object).Count
        
        if ($count -gt 0) {
            $replacements += @{ pattern = $old_pattern; replacement = $new_pattern; count = $count; suite = $suite }
            $content = $content -replace [regex]::Escape($old_pattern), $new_pattern
        }
    }
}

# Save the fixed content
Set-Content -Path $fix_file -Value $content -Encoding UTF8

Write-Host "Applied fixes to $fix_file:"
foreach ($r in $replacements) {
    Write-Host "  - Renamed $($r.count) tests from $($r.suite) to $($r.suite)NoFixture"
}

Write-Host "`nTotal replacements: $(($replacements | Measure-Object -Property count -Sum).Sum)"
