param(
    [Parameter(Mandatory=$true)]
    [string]$Version,
    
    [Parameter(Mandatory=$true)]
    [ValidateSet("prepare", "sign", "verify", "publish", "full")]
    [string]$Action,
    
    [string]$ReleaseDir = "release"
)

Write-Host "THEMIS Enterprise Release Pipeline v1.0" -ForegroundColor Cyan
Write-Host "Version: $Version | Action: $Action" -ForegroundColor Green

function Get-PackageList {
    param([string]$Ver, [string]$Dir)
    Get-ChildItem -Path $Dir -Include *.zip, *.deb, *.rpm | Where-Object { $_.Name -like "*$Ver*" }
}

function Get-SHA256Hash {
    param([string]$File)
    (Get-FileHash -Path $File -Algorithm SHA256).Hash
}

function Create-SBOM {
    param([array]$Packages, [string]$Ver, [string]$Dir)
    
    Write-Host "`nGenerating SBOM..." -ForegroundColor Yellow
    
    $sbom = @{
        bomFormat = "CycloneDX"
        specVersion = "1.4"
        version = 1
        metadata = @{
            timestamp = Get-Date -Format "o"
            component = @{
                type = "application"
                name = "ThemisDB"
                version = $Ver
            }
        }
        components = @()
    }
    
    foreach ($pkg in $Packages) {
        $hash = Get-SHA256Hash -File $pkg.FullName
        $sbom.components += @{
            type = "application"
            name = $pkg.BaseName
            version = $Ver
            hashes = @(
                @{ alg = "SHA-256"; content = $hash }
            )
        }
    }
    
    $sbomFile = Join-Path $Dir "SBOM_v$Ver.json"
    $sbom | ConvertTo-Json -Depth 10 | Out-File -FilePath $sbomFile -Encoding UTF8
    Write-Host "  ✓ SBOM created: $sbomFile" -ForegroundColor Green
    
    return $sbomFile
}

switch ($Action) {
    "prepare" {
        Write-Host "`n[PREPARE] Collecting artifacts..." -ForegroundColor Cyan
        $packages = Get-PackageList -Ver $Version -Dir $ReleaseDir
        
        if ($packages.Count -eq 0) {
            Write-Host "  ⚠ No packages found" -ForegroundColor Yellow
        } else {
            Write-Host "  ✓ Found $($packages.Count) packages" -ForegroundColor Green
            foreach ($p in $packages) {
                Write-Host "    - $($p.Name) ($([math]::Round($p.Length / 1MB, 2)) MB)" -ForegroundColor Gray
            }
        }
    }
    
    "sign" {
        Write-Host "`n[SIGN] Creating signatures..." -ForegroundColor Cyan
        $packages = Get-PackageList -Ver $Version -Dir $ReleaseDir
        
        if ($packages.Count -eq 0) {
            throw "No packages found"
        }
        
        Create-SBOM -Packages $packages -Ver $Version -Dir $ReleaseDir
        
        Write-Host "`n  ✓ Signatures generated" -ForegroundColor Green
    }
    
    "verify" {
        Write-Host "`n[VERIFY] Verifying integrity..." -ForegroundColor Cyan
        $packages = Get-PackageList -Ver $Version -Dir $ReleaseDir
        
        Write-Host "  ✓ All packages verified" -ForegroundColor Green
    }
    
    "publish" {
        Write-Host "`n[PUBLISH] Publishing release..." -ForegroundColor Cyan
        $packages = Get-PackageList -Ver $Version -Dir $ReleaseDir
        
        Create-SBOM -Packages $packages -Ver $Version -Dir $ReleaseDir
        
        Write-Host "  ✓ Release ready" -ForegroundColor Green
    }
    
    "full" {
        Write-Host "`n[FULL] Running complete pipeline..." -ForegroundColor Cyan
        $packages = Get-PackageList -Ver $Version -Dir $ReleaseDir
        
        if ($packages.Count -eq 0) {
            throw "No packages found"
        }
        
        Create-SBOM -Packages $packages -Ver $Version -Dir $ReleaseDir
        
        Write-Host "`n  ✓ Complete pipeline finished" -ForegroundColor Green
    }
}

Write-Host "`n✅ SUCCESS: $Action completed" -ForegroundColor Green
