# PowerShell script to fix constructor overloading pattern across all header files
# Pattern: explicit ClassName(const Config& config = {});
# Fix: Split into no-arg version + explicit parameterized version

$HeaderFiles = @(
    "include\exporters\huggingface_exporter.h",
    "include\exporters\data_augmentation.h",
    "include\exporters\arrow_ipc_exporter.h",
    "include\exporters\incremental_exporter.h",
    "include\exporters\join_exporter.h",
    "include\index\ann_index.h",
    "include\exporters\streaming_exporter.h",
    "include\voice\wake_word_detector.h",
    "include\exporters\pii_detector.h",
    "include\voice\voice_tts_customizer.h",
    "include\exporters\parquet_exporter.h",
    "include\voice\voice_telephony.h",
    "include\exporters\jsonl_llm_exporter.h",
    "include\voice\voice_security.h",
    "include\voice\voice_model_cache.h",
    "include\voice\voice_meeting_support.h",
    "include\voice\voice_intent_detector.h",
    "include\voice\voice_error_handler.h",
    "include\voice\voice_batch_processor.h",
    "include\voice\voice_auth.h",
    "include\index\distributed_vector_index.h",
    "include\index\cuda_hnsw_graph_traversal.h",
    "include\voice\emotion_analyzer.h",
    "include\analytics\model_serving.h",
    "include\analytics\ml_serving.h",
    "include\api\otlp_exporter.h",
    "include\importers\gui_import_wizard.h",
    "include\cache\distributed_cache_coordinator.h",
    "include\cache\cache_replication.h",
    "include\graph\distributed_graph.h",
    "include\updates\schema_migration_tester.h",
    "include\replication\raft_v2.h",
    "include\replication\multi_tier_replication.h",
    "include\observability\ml_anomaly_detector.h",
    "include\transaction\saga_orchestrator.h",
    "include\governance\policy_file_watcher.h",
    "include\transaction\distributed_saga.h",
    "include\query\adaptive_join.h",
    "include\storage\nvme_manager.h",
    "include\rag\onnx_model_loader.h",
    "include\timeseries\ts_auto_buffer_adaptive.h",
    "include\search\search_highlighter.h",
    "include\rag\streaming_retriever.h",
    "include\llm\shared_worker_pool.h",
    "include\llm\speculative_decoder.h",
    "include\llm\lora_framework\lora_checkpoint_manager.h"
)

function Fix-ConstructorPattern {
    param(
        [string]$FilePath
    )
    
    $fullPath = "$PSScriptRoot\$FilePath"
    
    if (-not (Test-Path $fullPath)) {
        Write-Host "File not found: $FilePath" -ForegroundColor Red
        return $false
    }
    
    $content = Get-Content -Path $fullPath -Raw
    
    # Pattern: explicit ClassName(ClassName = broken if split like this; need to find class name first)
    # Extract class name from file path: include\voice\wake_word_detector.h -> WakeWordDetector
    $fileName = Split-Path -Leaf $FilePath
    $className = @()
    
    # Convert snake_case to PascalCase
    $words = $fileName -replace '\.h$' -split '_'
    $className = ($words | ForEach-Object { $_ -creplace '^.', { $_.ToString().ToUpper() } }) -join ''
    
    # Handle special cases or get from actual content
    if ($content -match 'explicit\s+(\w+)\s*\(\s*') {
        $className = $matches[1]
    }
    
    Write-Host "Processing $FilePath (ClassName: $className)" -ForegroundColor Cyan
    
    # Pattern to find: explicit ClassName(const ConfigType& config = {});
    $pattern = "(explicit\s+$className\s*\(\s*const\s+(\w+)\s*&\s+\w+\s*=\s*\{\}\s*\);)"
    
    if ($content -match $pattern) {
        Write-Host "  Found pattern to fix: $($matches[1])" -ForegroundColor Yellow
        
        # Extract config type
        if ($content -match "explicit\s+$className\s*\(\s*const\s+(\w+)\s*&") {
            $configType = $matches[1]
            $paramName = if ($content -match "explicit\s+$className\s*\(\s*const\s+$configType\s*&\s+(\w+)\s*=") {
                $matches[1]
            } else {
                "config"
            }
            
            # Replace the old pattern with new one
            $newText = "    $className();`n    explicit $className(const $configType& $paramName);"
            $content = $content -replace $pattern, $newText
            
            Set-Content -Path $fullPath -Value $content -NoNewline
            Write-Host "  ✅ Fixed" -ForegroundColor Green
            return $true
        }
    } else {
        Write-Host "  ⚠ Pattern not found or already fixed" -ForegroundColor Gray
        return $false
    }
}

# Main loop
$fixedCount = 0
foreach ($file in $HeaderFiles) {
    if (Fix-ConstructorPattern -FilePath $file) {
        $fixedCount++
    }
}

Write-Host "`nFixed $fixedCount / $($HeaderFiles.Count) files" -ForegroundColor Cyan
