param(
    [Parameter(Mandatory=$true)]
    [string]$Version,
    
    [string]$ReleaseDir = "release"
)

Write-Host "THEMIS SBOM & Release Manifest Generator" -ForegroundColor Cyan

# Get all packages for this version
$packages = Get-ChildItem -Path $ReleaseDir -Filter "*$Version*" -Include *.zip, *.deb, *.rpm -ErrorAction SilentlyContinue

if ($packages.Count -eq 0) {
    Write-Host "No packages found for version $Version" -ForegroundColor Yellow
    exit
}

Write-Host "Found $($packages.Count) packages" -ForegroundColor Green

# Create SBOM data
$sbomData = @()
foreach ($pkg in $packages) {
    $hash = (Get-FileHash -Path $pkg.FullName -Algorithm SHA256).Hash
    $sbomData += @{
        filename = $pkg.Name
        size_mb = [math]::Round($pkg.Length / 1MB, 2)
        sha256 = $hash
    }
    Write-Host "  ✓ $($pkg.Name) - $hash" -ForegroundColor Green
}

# Write SBOM as JSON
$sbomFile = Join-Path $ReleaseDir "SBOM_v$Version.json"
$sbomData | ConvertTo-Json | Out-File -FilePath $sbomFile -Encoding UTF8

# Write manifest as signatures
$sigFile = Join-Path $ReleaseDir "MANIFEST_v$Version.txt"
foreach ($item in $sbomData) {
    "$($item.filename)	$($item.sha256)" | Add-Content $sigFile
}

Write-Host "`n✅ Generated:" -ForegroundColor Green
Write-Host "  - $sbomFile" -ForegroundColor Green
Write-Host "  - $sigFile" -ForegroundColor Green
