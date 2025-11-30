# Sync Documentation to GitHub Wiki
# Dieses Script synchronisiert die docs/ Markdown-Dateien mit dem GitHub Wiki

param(
    [string]$WikiRepo = "https://github.com/makr-code/ThemisDB.wiki.git",
    [string]$DocsPath = ".\docs",
    [string]$WikiPath = ".\wiki-temp",
    [switch]$CleanupAfter = $true
)

Write-Host "=== ThemisDB Documentation -> Wiki Sync ===" -ForegroundColor Cyan

# Pruefe ob docs/ existiert
if (-not (Test-Path $DocsPath)) {
    Write-Error "Dokumentationsverzeichnis nicht gefunden: $DocsPath"
    exit 1
}

# Cleanup old wiki clone if exists
if (Test-Path $WikiPath) {
    Write-Host "Loesche altes Wiki-Verzeichnis..." -ForegroundColor Yellow
    Remove-Item -Path $WikiPath -Recurse -Force
}

# Clone Wiki Repository
Write-Host "Clone Wiki Repository..." -ForegroundColor Green
git clone $WikiRepo $WikiPath
if ($LASTEXITCODE -ne 0) {
    Write-Error "Wiki-Clone fehlgeschlagen. Stelle sicher, dass das Wiki auf GitHub aktiviert ist!"
    Write-Host "Aktiviere das Wiki auf: https://github.com/makr-code/ThemisDB/wiki" -ForegroundColor Yellow
    exit 1
}

# Synchronisiere Markdown-Dateien
Write-Host "Synchronisiere Markdown-Dateien..." -ForegroundColor Green

# Loesche alte Dateien im Wiki (ausser .git)
Get-ChildItem -Path $WikiPath -Exclude ".git" | Remove-Item -Recurse -Force

# Kopiere alle Markdown-Dateien (rekursiv, Struktur beibehalten)
$mdFiles = Get-ChildItem -Path $DocsPath -Filter "*.md" -Recurse
$totalFiles = $mdFiles.Count
$counter = 0

foreach ($file in $mdFiles) {
    $counter++
    $relativePath = $file.FullName.Substring($DocsPath.Length + 1)
    $targetPath = Join-Path $WikiPath $relativePath
    $targetDir = Split-Path $targetPath -Parent
    
    # Erstelle Zielverzeichnis falls noetig
    if (-not (Test-Path $targetDir)) {
        New-Item -Path $targetDir -ItemType Directory -Force | Out-Null
    }
    
    Copy-Item -Path $file.FullName -Destination $targetPath -Force
    Write-Progress -Activity "Kopiere Dateien" -Status "$counter von $totalFiles" -PercentComplete (($counter / $totalFiles) * 100)
}

Write-Progress -Activity "Kopiere Dateien" -Completed

# Kopiere auch wichtige YAML-Dateien fuer Kontext
if (Test-Path "mkdocs.yml") {
    Copy-Item "mkdocs.yml" -Destination $WikiPath -Force
    Write-Host "  mkdocs.yml kopiert" -ForegroundColor Gray
}

# Erstelle Wiki-Home.md falls nicht vorhanden
$homePath = Join-Path $WikiPath "Home.md"
if (-not (Test-Path $homePath)) {
    if (Test-Path (Join-Path $DocsPath "index.md")) {
        Copy-Item (Join-Path $DocsPath "index.md") -Destination $homePath -Force
        Write-Host "  Home.md erstellt aus index.md" -ForegroundColor Gray
    }
}

# Erzeuge Grundseiten: _Header.md, _Sidebar.md, _Footer.md
Write-Host "Erzeuge Basis-Wiki-Seiten (_Header, _Sidebar, _Footer)..." -ForegroundColor Green

# Helper: Fuege Link nur hinzu, wenn Datei vorhanden ist
function Add-LinkIfExists {
    param(
        [ref]$Lines,
        [string]$WikiRoot,
        [string]$RelPath,
        [string]$Title
    )
    $target = Join-Path $WikiRoot $RelPath
    if (Test-Path $target) {
        # Konvertiere Pfad zu Wiki-Link-Format (ohne .md und mit - statt /)
        $wikiLink = $RelPath -replace '\.md$', '' -replace '/', '-'
        $Lines.Value += "* [[$Title|$wikiLink]]"
    }
}

# _Header.md
$headerPath = Join-Path $WikiPath "_Header.md"
$headerLines = @()
$headerLines += "[ThemisDB](https://github.com/makr-code/ThemisDB) | [[Home]] | [Issues](https://github.com/makr-code/ThemisDB/issues)"
$headerLines += ""
Set-Content -Path $headerPath -Value $headerLines -Encoding UTF8

# _Sidebar.md
$sidebarPath = Join-Path $WikiPath "_Sidebar.md"
$sb = @()
$sb += "## ThemisDB Wiki"
$sb += "* [[Home|Home]]"
$sb += "* [[Features Overview|FEATURES]]"
$sb += "* [[Quick Reference|DOCS_QUICKREF]]"
$sb += "* [[Documentation Index|DOCUMENTATION_INDEX]]"
$sb += ""

$sb += "### Getting Started"
$sb += "* [[Build Guide|BUILD_GUIDE]]"
$sb += "* [[Architecture Overview|architecture]]"
$sb += "* [[Deployment Guide|guides-deployment]]"
$sb += "* [[Operations Runbook|guides-operations_runbook]]"
$sb += ""

$sb += "### SDKs and Clients"
$sb += "* [[JavaScript SDK|clients-javascript_sdk_quickstart]]"
$sb += "* [[Python SDK|clients-python_sdk_quickstart]]"
$sb += "* [[Rust SDK|clients-rust_sdk_quickstart]]"
$sb += "* [[SDK Implementation Status|clients-sdk_implementation_plan]]"
$sb += "* [[SDK Language Analysis|clients-sdk_language_analysis]]"
$sb += ""

$sb += "### Query Language (AQL)"
$sb += "* [[AQL Overview|aql-README]]"
$sb += "* [[AQL Syntax Reference|aql-syntax]]"
$sb += "* [[EXPLAIN and PROFILE|aql-explain_profile]]"
$sb += "* [[Hybrid Queries|aql-hybrid-queries]]"
$sb += "* [[Pattern Matching|aql-pattern_matching]]"
$sb += "* [[Subquery Implementation|aql-subquery_implementation_summary]]"
$sb += "* [[Subquery Quick Reference|aql-subquery_quick_reference]]"
$sb += "* [[Fulltext Release Notes|aql-release_notes_fulltext]]"
$sb += ""

$sb += "### Search and Retrieval"
$sb += "* [[Hybrid Search Design|search-hybrid_search_design]]"
$sb += "* [[Fulltext Search API|search-fulltext_api]]"
$sb += "* [[Content Search|content-search_api]]"
$sb += "* [[Pagination Benchmarks|search-pagination_benchmarks]]"
$sb += "* [[Stemming|search-stemming]]"
$sb += "* [[Hybrid Fusion API|search-hybrid_fusion_api]]"
$sb += "* [[Performance Tuning|search-performance_tuning]]"
$sb += "* [[Migration Guide|search-migration_guide]]"
$sb += ""

$sb += "### Storage and Indexes"
$sb += "* [[Storage Overview|src-storage-README]]"
$sb += "* [[RocksDB Layout|storage-rocksdb_layout]]"
$sb += "* [[Geo Schema|storage-geo_relational_schema]]"
$sb += "* [[Index Types|features-indexes]]"
$sb += "* [[Index Statistics|features-index_stats_maintenance]]"
$sb += "* [[Index Backup|features-index_backup]]"
$sb += "* [[HNSW Persistence|features-hnsw_persistence]]"
$sb += "* [[Vector Index|src-index-vector_index.cpp]]"
$sb += "* [[Graph Index|src-index-graph_index.cpp]]"
$sb += "* [[Secondary Index|src-index-secondary_index.cpp]]"
$sb += ""

$sb += "### Security and Compliance"
$sb += "* [[Security Overview|security-overview]]"
$sb += "* [[RBAC and Authorization|guides-rbac_authorization]]"
$sb += "* [[TLS Setup|guides-tls_setup]]"
$sb += "* [[Certificate Pinning|security-certificate_pinning]]"
$sb += "* [[Encryption Strategy|security-encryption_strategy]]"
$sb += "* [[Column Encryption|security-column_encryption]]"
$sb += "* [[Key Management|security-key_management]]"
$sb += "* [[Key Rotation|security-key_rotation_strategy]]"
$sb += "* [[HSM Integration|security-hsm_integration]]"
$sb += "* [[PKI Integration|security-pki_integration_architecture]]"
$sb += "* [[eIDAS Signatures|security-eidas_qualified_signatures]]"
$sb += "* [[PII Detection|security-pii_detection]]"
$sb += "* [[PII API|security-pii_api]]"
$sb += "* [[Threat Model|security-threat_model]]"
$sb += "* [[Hardening Guide|security-hardening_guide]]"
$sb += "* [[Incident Response|security-INCIDENT_RESPONSE_PLAN]]"
$sb += "* [[SBOM|security-SBOM]]"
$sb += ""

$sb += "### Enterprise Features"
$sb += "* [[Enterprise Overview|enterprise-README]]"
$sb += "* [[Scalability Features|ENTERPRISE_SCALABILITY]]"
$sb += "* [[Scalability Strategy|performance-ENTERPRISE_SCALABILITY_STRATEGY]]"
$sb += "* [[HTTP Client Pool|HTTP_CLIENT_POOL_COMPLETE]]"
$sb += "* [[Enterprise Build Guide|ENTERPRISE_BUILD_GUIDE]]"
$sb += "* [[Enterprise Ingestion|features-enterprise_ingestion]]"
$sb += ""

$sb += "### Performance and Optimization"
$sb += "* [[Benchmarks Overview|performance-benchmarks]]"
$sb += "* [[Compression Benchmarks|performance-compression_benchmarks]]"
$sb += "* [[Compression Strategy|performance-compression_strategy]]"
$sb += "* [[Memory Tuning|performance-memory_tuning]]"
$sb += "* [[Hardware Acceleration|performance-HARDWARE_ACCELERATION]]"
$sb += "* [[GPU Acceleration Plan|performance-GPU_ACCELERATION_PLAN]]"
$sb += "* [[CUDA Backend|performance-CUDA_BACKEND]]"
$sb += "* [[Vulkan Backend|performance-VULKAN_BACKEND]]"
$sb += "* [[Multi-CPU Support|performance-MULTI_CPU_SUPPORT]]"
$sb += "* [[TBB Integration|performance-TBB_INTEGRATION]]"
$sb += ""

$sb += "### Features and Capabilities"
$sb += "* [[Time Series|features-time_series]]"
$sb += "* [[Vector Operations|features-vector_ops]]"
$sb += "* [[Graph Features|features-property_graph_model]]"
$sb += "* [[Temporal Graphs|features-temporal_graphs]]"
$sb += "* [[Path Constraints|features-path_constraints]]"
$sb += "* [[Recursive Queries|features-recursive_path_queries]]"
$sb += "* [[Audit Logging|features-audit_logging]]"
$sb += "* [[Change Data Capture|features-change_data_capture]]"
$sb += "* [[Transactions|features-transactions]]"
$sb += "* [[Semantic Cache|features-semantic_cache]]"
$sb += "* [[Cursor Pagination|features-cursor_pagination]]"
$sb += "* [[Compliance Features|features-compliance]]"
$sb += "* [[GNN Embeddings|features-gnn_embeddings]]"
$sb += ""

$sb += "### Geo and Spatial"
$sb += "* [[Geo Overview|geo-README]]"
$sb += "* [[Geo Architecture|geo-architecture]]"
$sb += "* [[3D Game Acceleration|geo-geo_acceleration_3d_games]]"
$sb += "* [[Geo Feature Tiering|geo-geo_feature_tiering]]"
$sb += "* [[G3 Phase 2 Status|geo-geo_g3_phase2_status]]"
$sb += "* [[G5 Implementation|geo-geo_g5_implementation]]"
$sb += "* [[Integration Guide|geo-geo_integration_readme]]"
$sb += ""

$sb += "### Content and Ingestion"
$sb += "* [[Content Architecture|architecture-content_architecture]]"
$sb += "* [[Content Pipeline|architecture-content_pipeline]]"
$sb += "* [[Content Manager|src-content-content_manager.cpp]]"
$sb += "* [[JSON Ingestion|ingestion-json_ingestion_spec]]"
$sb += "* [[Content Ingestion|content-ingestion]]"
$sb += "* [[Filesystem API|content-filesystem_api]]"
$sb += "* [[Image Processor|content-image_processor_design]]"
$sb += "* [[Geo Processor|content-geo_processor_design]]"
$sb += "* [[Policy Implementation|content-policy_implementation]]"
$sb += ""

$sb += "### Sharding and Scaling"
$sb += "* [[Sharding Overview|sharding-README]]"
$sb += "* [[Horizontal Scaling|sharding-horizontal_scaling_strategy]]"
$sb += "* [[Phase 1 Report|sharding-phase1_report]]"
$sb += "* [[Phases 1-3 Summary|sharding-phases_1-3_summary]]"
$sb += "* [[Implementation Summary|sharding-implementation_summary]]"
$sb += ""

$sb += "### APIs and Integration"
$sb += "* [[OpenAPI Specification|apis-openapi]]"
$sb += "* [[Hybrid Search API|apis-hybrid_search_api]]"
$sb += "* [[ContentFS API|apis-contentfs_api]]"
$sb += "* [[HTTP Server Implementation|src-server-http_server.cpp]]"
$sb += "* [[REST API|src-api-README]]"
$sb += ""

$sb += "### Admin Tools"
$sb += "* [[Admin Guide|admin_tools-admin_guide]]"
$sb += "* [[User Guide|admin_tools-user_guide]]"
$sb += "* [[Feature Matrix|admin_tools-feature_matrix]]"
$sb += "* [[Search Sort Filter|admin_tools-search_sort_filter]]"
$sb += "* [[Demo Script|admin_tools-demo_script]]"
$sb += ""

$sb += "### Observability"
$sb += "* [[Metrics Overview|observability-metrics]]"
$sb += "* [[Prometheus Metrics|observability-prometheus_metrics]]"
$sb += "* [[Tracing|observability-tracing]]"
$sb += ""

$sb += "### Development"
$sb += "* [[Developer Guide|development-README]]"
$sb += "* [[Implementation Status|development-implementation_status]]"
$sb += "* [[Development Roadmap|development-roadmap]]"
$sb += "* [[Build Strategy|BUILD_STRATEGY]]"
$sb += "* [[Build Acceleration|development-build_acceleration]]"
$sb += "* [[Code Quality Guide|guides-code_quality]]"
$sb += "* [[AQL LET Implementation|development-aql_let_implementation_guide]]"
$sb += "* [[Audit API Implementation|development-audit_api_implementation]]"
$sb += "* [[SAGA API Implementation|development-saga_api_implementation]]"
$sb += "* [[PKI eIDAS|development-pki-eidas]]"
$sb += "* [[WAL Archiving|development-wal-archiving]]"
$sb += ""

$sb += "### Architecture"
$sb += "* [[Architecture Overview|architecture-README]]"
$sb += "* [[Strategic Overview|architecture-strategic_overview]]"
$sb += "* [[Ecosystem|architecture-ecosystem_overview]]"
$sb += "* [[MVCC Design|architecture-mvcc_design]]"
$sb += "* [[Base Entity|architecture-base_entity]]"
$sb += "* [[Caching Strategy|architecture-cache_invalidation_strategy]]"
$sb += "* [[Caching Data Structures|architecture-caching_data_structures]]"
$sb += ""

$sb += "### Deployment and Operations"
$sb += "* [[Docker Build|deployment-docker_build]]"
$sb += "* [[Docker Status|deployment-docker_status]]"
$sb += "* [[Multi-Arch CI/CD|CI_CD_MULTIARCH]]"
$sb += "* [[ARM Build Guide|ARM_RASPBERRY_PI_BUILD]]"
$sb += "* [[ARM Packages|ARM_PACKAGES]]"
$sb += "* [[Raspberry Pi Tuning|RASPBERRY_PI_TUNING]]"
$sb += "* [[Packaging Guide|packaging]]"
$sb += "* [[Package Maintainers|PACKAGE-MAINTAINERS]]"
$sb += ""

$sb += "### Exporters and Integrations"
$sb += "* [[JSONL LLM Exporter|exporters-JSONL_LLM_EXPORTER]]"
$sb += "* [[LoRA Adapter Metadata|exporters-LORA_ADAPTER_METADATA]]"
$sb += "* [[vLLM Multi-LoRA|exporters-VLLM_MULTI_LORA_INTEGRATION]]"
$sb += "* [[Postgres Importer|importers-POSTGRES_IMPORTER]]"
$sb += ""

$sb += "### Reports and Status"
$sb += "* [[Roadmap|roadmap]]"
$sb += "* [[Changelog|changelog]]"
$sb += "* [[Database Capabilities|reports-database_capabilities_roadmap]]"
$sb += "* [[Implementation Summary|reports-themis_implementation_summary]]"
$sb += "* [[Sachstandsbericht 2025|reports-themis_sachstandsbericht_2025]]"
$sb += "* [[Enterprise Final Report|ENTERPRISE_FINAL_REPORT]]"
$sb += "* [[Test Report|reports-TEST_REPORT]]"
$sb += "* [[Build Success Report|reports-BUILD_SUCCESS_REPORT]]"
$sb += "* [[Integration Analysis|reports-INTEGRATION_ANALYSIS]]"
$sb += ""

$sb += "### Compliance and Governance"
$sb += "* [[BCP and DRP|compliance-BCP_DRP]]"
$sb += "* [[DPIA|compliance-DPIA]]"
$sb += "* [[Risk Register|compliance-RISK_REGISTER]]"
$sb += "* [[Vendor Assessment|compliance-VENDOR_ASSESSMENT]]"
$sb += "* [[Compliance Dashboard|COMPLIANCE_DASHBOARD]]"
$sb += "* [[Compliance Strategy|features-compliance_governance_strategy]]"
$sb += ""

$sb += "### Testing and Quality"
$sb += "* [[Quality Assurance|guides-quality_assurance]]"
$sb += "* [[Known Issues|guides-known_issues]]"
$sb += "* [[Content Features Test|content-features_test_report]]"
$sb += ""

$sb += "### Source Code Documentation"
$sb += "* [[Source Overview|src-README]]"
$sb += "* [[API Implementation|src-api-README]]"
$sb += "* [[Query Engine|src-query-README]]"
$sb += "* [[Storage Layer|src-storage-README]]"
$sb += "* [[Security Implementation|src-security-README]]"
$sb += "* [[CDC Implementation|src-cdc-README]]"
$sb += "* [[Time Series|src-timeseries-README]]"
$sb += "* [[Utils and Helpers|src-utils-README]]"
$sb += ""

$sb += "### Reference"
$sb += "* [[Glossary|glossary]]"
$sb += "* [[Style Guide|guides-styleguide]]"
$sb += "* [[Publishing Guide|guides-publishing]]"
$sb += ""

$sb += "---"
$sb += "_Updated: $(Get-Date -Format 'yyyy-MM-dd')_"

Set-Content -Path $sidebarPath -Value $sb -Encoding UTF8

# _Footer.md
$footerPath = Join-Path $WikiPath "_Footer.md"
$footerLines = @()
$footerLines += "ThemisDB Documentation - auto-synced from /docs on $(Get-Date -Format 'yyyy-MM-dd')"
Set-Content -Path $footerPath -Value $footerLines -Encoding UTF8

# Git Commit & Push
Write-Host "Committe Aenderungen ins Wiki..." -ForegroundColor Green
Set-Location $WikiPath

git add .

$changesCount = (git status --porcelain | Measure-Object).Count
if ($changesCount -eq 0) {
    Write-Host "Keine Aenderungen zu committen." -ForegroundColor Yellow
    Set-Location ..
    if ($CleanupAfter) {
        Remove-Item -Path $WikiPath -Recurse -Force
    }
    exit 0
}

$commitMsg = "Auto-sync documentation from docs/ ($(Get-Date -Format 'yyyy-MM-dd HH:mm'))"
git commit -m $commitMsg

Write-Host "Pushe zum GitHub Wiki..." -ForegroundColor Green
git push origin master

if ($LASTEXITCODE -eq 0) {
    Write-Host ""
    Write-Host "Dokumentation erfolgreich ins Wiki synchronisiert!" -ForegroundColor Green
    Write-Host "   $changesCount Datei(en) aktualisiert" -ForegroundColor Gray
    Write-Host "   Wiki URL: https://github.com/makr-code/ThemisDB/wiki" -ForegroundColor Cyan
} else {
    Write-Error "Push fehlgeschlagen!"
    Set-Location ..
    exit 1
}

# Cleanup
Set-Location ..
if ($CleanupAfter) {
    Write-Host "Raeume temporaeres Verzeichnis auf..." -ForegroundColor Gray
    Remove-Item -Path $WikiPath -Recurse -Force
}

Write-Host ""
Write-Host "Fertig!" -ForegroundColor Green

# PDF-Generierung
Write-Host "" 
Write-Host "=== PDF-Generierung ===" -ForegroundColor Cyan

# Prüfe ob Python verfügbar ist
$pythonCmd = Get-Command python -ErrorAction SilentlyContinue
if (-not $pythonCmd) {
    Write-Warning "Python nicht gefunden. PDF-Generierung uebersprungen."
    Write-Host "Bitte installiere Python: https://www.python.org/downloads/" -ForegroundColor Yellow
    exit 0
}

# Prüfe ob mkdocs verfügbar ist
$mkdocsCheck = python -m mkdocs --version 2>&1
if ($LASTEXITCODE -ne 0) {
    Write-Warning "mkdocs nicht gefunden. PDF-Generierung uebersprungen."
    Write-Host "Installation: pip install mkdocs mkdocs-material" -ForegroundColor Yellow
    exit 0
}

# Prüfe ob mkdocs.yml existiert
if (-not (Test-Path "mkdocs.yml")) {
    Write-Warning "mkdocs.yml nicht gefunden. PDF-Generierung uebersprungen."
    exit 0
}

Write-Host "Generiere PDF-Dokumentation..." -ForegroundColor Green

# Backup existing site/ if present
if (Test-Path "site") {
    Write-Host "  Sichere bestehendes site/ Verzeichnis..." -ForegroundColor Gray
    if (Test-Path "site-backup") {
        Remove-Item -Path "site-backup" -Recurse -Force
    }
    Move-Item -Path "site" -Destination "site-backup" -Force
}

# Build documentation
try {
    Write-Host "  mkdocs build ausgefuehrt..." -ForegroundColor Gray
    python -m mkdocs build --clean 2>&1 | Out-Null
    
    if ($LASTEXITCODE -eq 0 -and (Test-Path "site/print_page/index.html")) {
        Write-Host "  HTML-Version generiert" -ForegroundColor Gray
        
        # Prüfe auf wkhtmltopdf (bessere Windows-Unterstützung als weasyprint)
        $wkCmd = Get-Command wkhtmltopdf -ErrorAction SilentlyContinue
        if ($wkCmd) {
            Write-Host "  Konvertiere HTML zu PDF mit wkhtmltopdf..." -ForegroundColor Gray
            $htmlFile = Resolve-Path "site/print_page/index.html"
            $targetPdf = Join-Path $DocsPath "ThemisDB-Documentation.pdf"
            
            wkhtmltopdf --enable-local-file-access --print-media-type --no-background `
                --minimum-font-size 12 --page-size A4 --margin-top 20mm --margin-bottom 20mm `
                --margin-left 15mm --margin-right 15mm --footer-center "[page] / [topage]" `
                "$htmlFile" "$targetPdf" 2>&1 | Out-Null
            
            if ($LASTEXITCODE -eq 0 -and (Test-Path $targetPdf)) {
                $pdfSize = [math]::Round((Get-Item $targetPdf).Length / 1MB, 2)
                Write-Host "" 
                Write-Host "PDF erfolgreich generiert!" -ForegroundColor Green
                Write-Host "  Datei: $targetPdf" -ForegroundColor Cyan
                Write-Host "  Groesse: $pdfSize MB" -ForegroundColor Gray
                
                # Kopiere auch ins Wiki
                if (Test-Path $WikiPath) {
                    $wikiPdf = Join-Path $WikiPath "ThemisDB-Documentation.pdf"
                    Copy-Item -Path $targetPdf -Destination $wikiPdf -Force
                    
                    Push-Location $WikiPath
                    git add "ThemisDB-Documentation.pdf"
                    $pdfChanges = (git status --porcelain | Measure-Object).Count
                    if ($pdfChanges -gt 0) {
                        git commit -m "Update PDF documentation ($(Get-Date -Format 'yyyy-MM-dd HH:mm'))"
                        git push origin master
                        Write-Host "  PDF ins Wiki gepusht" -ForegroundColor Green
                    }
                    Pop-Location
                }
            } else {
                Write-Warning "wkhtmltopdf Konvertierung fehlgeschlagen"
            }
        } else {
            Write-Host "" 
            Write-Host "wkhtmltopdf nicht gefunden - PDF wird nicht generiert" -ForegroundColor Yellow
            Write-Host "Download: https://wkhtmltopdf.org/downloads.html" -ForegroundColor Gray
            Write-Host "Oder verwende die print_page in MkDocs: http://localhost:8000/print_page/" -ForegroundColor Gray
        }
    } else {
        Write-Warning "mkdocs build fehlgeschlagen oder print_page nicht verfuegbar"
    }
    
} catch {
    Write-Warning "Fehler bei PDF-Generierung: $_"
} finally {
    # Restore backup if build failed
    if ((Test-Path "site-backup") -and -not (Test-Path "site")) {
        Move-Item -Path "site-backup" -Destination "site" -Force
    }
}

Write-Host "" 
Write-Host "Alle Aufgaben abgeschlossen!" -ForegroundColor Green
