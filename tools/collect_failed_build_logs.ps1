$targets = @(
'module_provenance_test_provenance_aql_integration_focused',
'module_provenance_test_provenance_tracker_focused',
'module_rag_test_rag_adaptive_retrieval_focused',
'module_rag_test_rag_context_assembler_focused',
'module_rag_test_rag_context_engine_focused',
'module_rag_test_rag_multi_hop_reasoner_focused',
'module_rag_test_rag_replug_retriever_focused',
'module_rag_test_rag_rlaif_trainer_focused',
'module_redis_test_redis_hiredis_bridge_focused',
'module_scraper_test_scraper_plugin_focused',
'module_search_test_search_future_interfaces_focused',
'module_security_test_kdf_argon2_bridge_focused',
'module_security_test_oauth_token_manager_focused',
'module_security_test_safe_format_focused',
'module_security_test_stub_279_302_remediation_focused',
'module_service_test_service_mesh_api_handler_focused',
'module_storage_test_tensor_storage_observer_focused',
'module_stub_test_stub_252_tnsr_task_focused',
'module_temporal_test_temporal_aggregation_focused',
'module_temporal_test_temporal_graph_focused',
'module_tensor_test_tensor_hiss_structural_search_focused',
'module_tensor_test_tensor_ht_focused',
'module_tensor_test_tensor_mid_layer_abstractions_focused',
'module_tensor_test_tensor_recompress_focused',
'module_tensor_test_tensor_utr_focused',
'module_thesis_test_thesis_budget_management_focused',
'module_tiered_test_tiered_storage_focused',
'module_tool_test_tool_registry_focused',
'module_toolbox_test_toolbox_primitives_focused',
'module_training_test_training_convergence_focused',
'module_training_test_training_database_optimizer_focused',
'module_training_test_training_diagnostics_consistency_focused',
'module_training_test_training_lora_adapter_focused',
'module_training_test_training_phase2_focused',
'module_training_test_training_pipeline_e2e_focused',
'module_two_test_two_unified_2pc_recovery_focused',
'module_user_test_user_storage_v03_focused',
'module_wisckey_test_wisckey_gc_focused'
)
$logDir = Join-Path $PSScriptRoot '..\build-msvc-windows-release\build-logs' ; if(-not (Test-Path $logDir)) { New-Item -ItemType Directory -Path $logDir | Out-Null }
Set-Location -Path (Resolve-Path "$PSScriptRoot\..\")
foreach($t in $targets){
  Write-Host "=== Building: $t ==="
  $out = Join-Path $logDir ($t + '.log')
  & cmake --build --preset windows-release --target $t --parallel 1 *> $out
  $exit = $LASTEXITCODE
  if($exit -ne 0){ Write-Host "$t failed with exitcode $exit (log: $out)" } else { Write-Host "$t succeeded (log: $out)" }
}
