# Run individual subtree splits for a curated list of admin tool folders.
$paths = @(
    'projects/Themis.AdminTools.Shared',
    'tools/admin_tools_dotnet/Themis.AdminTools.Shared',
    'tools/admin_tools_dotnet/Themis.USBAdminTool',
    'tools/admin_tools_dotnet/Themis.SAGAVerifier',
    'tools/Themis.USBAdminTool',
    'tools/admin_tools_dotnet/Themis.RetentionManager',
    'tools/Themis.SAGAVerifier',
    'tools/admin_tools_dotnet/Themis.PIIManager',
    'tools/Themis.RetentionManager',
    'tools/admin_tools_dotnet/Themis.KeyRotationDashboard',
    'tools/Themis.PIIManager',
    'tools/admin_tools_dotnet/Themis.IngestionTool',
    'tools/Themis.KeyRotationDashboard',
    'tools/Themis.IngestionTool',
    'tools/admin_tools_dotnet/Themis.ImpactAnalysisViewer',
    'tools/admin_tools_dotnet/Themis.GISViewer.ControlPanel',
    'tools/Themis.ImpactAnalysisViewer',
    'tools/admin_tools_dotnet/Themis.ComplianceReports',
    'tools/Themis.GISViewer.ControlPanel',
    'tools/Themis.ComplianceReports',
    'tools/admin_tools_dotnet/Themis.ClassificationDashboard',
    'tools/Themis.ClassificationDashboard',
    'tools/admin_tools_dotnet/Themis.AuditLogViewer',
    'tools/Themis.AuditLogViewer',
    'tools/admin_tools_dotnet/Themis.AqlQueryBuilder',
    'tools/Themis.AqlQueryBuilder',
    'tools/Themis.AdminTools.Shared'
)

foreach($p in $paths){
    if(-not (Test-Path $p)){
        Write-Host "SKIP (not exists): $p"
        continue
    }
    $san = $p -replace '[^a-zA-Z0-9]','-'
    $branch = "split-$san"
    if((git branch --list $branch) -ne ''){
        Write-Host "SKIP (branch exists): $branch"
        continue
    }
    Write-Host "SPLIT: $p -> $branch"
    git subtree split --prefix=$p -b $branch
    if($LASTEXITCODE -ne 0){
        Write-Host "FAILED: $p (exit $LASTEXITCODE)"
    } else {
        Write-Host "OK: $branch"
    }
}

Write-Host "Done individual splits."
