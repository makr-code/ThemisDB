# Script to add open() calls after RocksDBWrapper construction in benchmarks
$ErrorActionPreference = "Stop"

$benchmarkDir = "C:\VCC\themis\benchmarks"
$files = @(
    "bench_comprehensive.cpp",
    "bench_compression.cpp",
    "bench_core_performance.cpp",
    "bench_changefeed_throughput.cpp",
    "bench_gnn_embeddings.cpp",
    "bench_graph_traversal.cpp",
    "bench_index_rebuild.cpp",
    "bench_ingestion_kv.cpp",
    "bench_llm_real_models.cpp",
    "bench_mmdb.cpp",
    "bench_mvcc.cpp",
    "bench_pagerank.cpp",
    "bench_saga_compensation.cpp",
    "bench_timeseries_ingestion.cpp",
    "bench_tpcc.cpp",
    "bench_transaction_throughput.cpp",
    "bench_ycsb.cpp"
)

$fixCount = 0
$alreadyFixed = 0

foreach ($file in $files) {
    $filePath = Join-Path $benchmarkDir $file
    if (-not (Test-Path $filePath)) {
        Write-Host "Skipping $file - not found" -ForegroundColor Yellow
        continue
    }
    
    $content = Get-Content $filePath -Raw
    $originalContent = $content
    
    # Pattern 1: db_ = std::make_unique<RocksDBWrapper>(cfg); without immediate open()
    # Replace with: db_ = std::make_unique<RocksDBWrapper>(cfg);\n        if (!db_->open()) { throw ... }
    
    $pattern1 = '(db_ = std::make_unique<(?:themis::)?RocksDBWrapper>\((?:cfg|config)\);)\s*\n(\s+)(?!if\s*\(!db_->open\(\))(?!db_->open\(\))'
    $replacement1 = '$1' + "`n" + '$2if (!db_->open()) { throw std::runtime_error("Failed to open RocksDB in benchmark"); }' + "`n" + '$2'
    
    $newContent = [regex]::Replace($content, $pattern1, $replacement1)
    
    if ($newContent -ne $originalContent) {
        Set-Content -Path $filePath -Value $newContent -NoNewline
        Write-Host "Fixed $file" -ForegroundColor Green
        $fixCount++
    } else {
        Write-Host "Already fixed or no match: $file" -ForegroundColor Cyan
        $alreadyFixed++
    }
}

Write-Host "`nSummary:" -ForegroundColor White
Write-Host "  Fixed: $fixCount files" -ForegroundColor Green
Write-Host "  Already fixed/No match: $alreadyFixed files" -ForegroundColor Cyan
Write-Host "Done!" -ForegroundColor White
