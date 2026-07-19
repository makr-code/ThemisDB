param()
$bins = @(
    'module_audit_test_audit_logger_focused',
    'module_chunk_test_chunk_level_encryption_focused',
    'module_encryption_test_encryption_e2e_focused',
    'module_encryption_test_encryption_focused',
    'module_entity_test_entity_api_handler_batch_focused',
    'module_entity_test_entity_api_raid_integration_focused',
    'module_fuzz_test_fuzz_security_focused',
    'module_graph_test_graph_edge_encryption_focused',
    'module_lazy_test_lazy_reencryption_focused',
    'module_pii_test_pii_soft_delete_focused',
    'module_saga_test_saga_logger_focused',
    'module_security_test_security_di_focused',
    'module_siem_test_siem_integration_comprehensive_focused',
    'module_storage_test_storage_query_index_explicit_di_focused',
    'module_vault_test_vault_key_provider_focused',
    'module_vector_test_vector_encryption_integration_focused',
    'module_vector_test_vector_encryption_phase1_focused'
)

$binDir = Join-Path $PSScriptRoot '..\build-msvc-windows-release\bin_out' -Resolve
Write-Host "binDir: $binDir"
foreach ($b in $bins) {
    $exe = Join-Path $binDir ($b + '.exe')
    if (Test-Path $exe) {
        Write-Host "--- Running: $b ---"
        & $exe --gtest_output=xml:$b.results.xml 2>&1 | Tee-Object -FilePath "$b.console.log"
        Write-Host "--- Completed: $b ---"
    } else {
        Write-Host "Missing binary:" $exe
    }
}
Write-Host "Finished runs"
