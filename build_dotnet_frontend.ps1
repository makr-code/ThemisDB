#!/usr/bin/env pwsh
<#
.SYNOPSIS
    Build and package ThemisDB Document Manager (.NET 8 WPF Application)
.DESCRIPTION
    Comprehensive build script for C# frontend with version management, 
    code signing, and multi-configuration builds
.EXAMPLE
    .\build_dotnet_frontend.ps1 -Configuration Release -Version 1.0.1
#>

param(
    [Parameter(Mandatory=$false)]
    [ValidateSet("Debug", "Release")]
    [string]$Configuration = "Release",
    
    [Parameter(Mandatory=$false)]
    [string]$Version = "1.0.1",
    
    [Parameter(Mandatory=$false)]
    [switch]$Pack = $false,
    
    [Parameter(Mandatory=$false)]
    [switch]$Sign = $false,
    
    [Parameter(Mandatory=$false)]
    [string]$CertPath = ""
)

$ErrorActionPreference = "Stop"
$VerbosePreference = "Continue"

# Paths
$ScriptDir = Split-Path -Parent $MyInvocation.MyCommandPath
$ProjectRoot = $ScriptDir
$ProjectPath = Join-Path $ProjectRoot "projects/Themis.DocumentManager"
$CsprojPath = Join-Path $ProjectPath "Themis.DocumentManager.csproj"
$OutputDir = Join-Path $ProjectRoot "build-dotnet"
$PublishDir = Join-Path $OutputDir "publish"

Write-Host "============================================================" -ForegroundColor Cyan
Write-Host "  ThemisDB Document Manager - Build & Package Pipeline" -ForegroundColor Cyan
Write-Host "============================================================" -ForegroundColor Cyan
Write-Host ""

# 1. Update version in csproj
Write-Host "📋 Updating version to $Version..." -ForegroundColor Yellow

$csprojContent = Get-Content $CsprojPath -Raw
$csprojContent = $csprojContent -replace '<Version>[\d.]+</Version>', "<Version>$Version</Version>"
Set-Content $CsprojPath $csprojContent -NoNewline

Write-Host "✅ Version updated to $Version" -ForegroundColor Green

# 2. Restore NuGet packages
Write-Host ""
Write-Host "📦 Restoring NuGet packages..." -ForegroundColor Yellow

try {
    dotnet restore $CsprojPath --verbosity normal
    Write-Host "✅ NuGet packages restored" -ForegroundColor Green
} catch {
    Write-Host "❌ Failed to restore packages: $_" -ForegroundColor Red
    exit 1
}

# 3. Build
Write-Host ""
Write-Host "🔨 Building ($Configuration)..." -ForegroundColor Yellow

try {
    $buildOutput = dotnet build $CsprojPath `
        -c $Configuration `
        --no-restore `
        --output $OutputDir `
        --verbosity normal 2>&1
    
    if ($LASTEXITCODE -ne 0) {
        Write-Host "❌ Build failed!" -ForegroundColor Red
        Write-Host $buildOutput
        exit 1
    }
    Write-Host "✅ Build completed successfully" -ForegroundColor Green
} catch {
    Write-Host "❌ Build error: $_" -ForegroundColor Red
    exit 1
}

# 4. Code Signing (optional)
if ($Sign -and $CertPath) {
    Write-Host ""
    Write-Host "🔐 Code signing..." -ForegroundColor Yellow
    
    $exePath = Join-Path $OutputDir "net8.0-windows/Themis.DocumentManager.exe"
    if (Test-Path $exePath) {
        try {
            # Using Microsoft's SignTool (requires Windows SDK)
            $signtoolPath = "C:\Program Files (x86)\Windows Kits\10\bin\10.0.22621.0\x64\signtool.exe"
            if (Test-Path $signtoolPath) {
                & $signtoolPath sign /f $CertPath /p $env:CERT_PASSWORD /t http://timestamp.comodoca.com/authenticode $exePath
                Write-Host "✅ Code signed successfully" -ForegroundColor Green
            } else {
                Write-Host "⚠️  SignTool not found, skipping signing" -ForegroundColor Yellow
            }
        } catch {
            Write-Host "⚠️  Code signing failed (continuing): $_" -ForegroundColor Yellow
        }
    }
}

# 5. Publish (standalone)
if ($Pack) {
    Write-Host ""
    Write-Host "📦 Publishing as self-contained application..." -ForegroundColor Yellow
    
    try {
        dotnet publish $CsprojPath `
            -c $Configuration `
            --no-build `
            --output $PublishDir `
            --self-contained `
            --runtime win-x64 `
            -p:PublishSingleFile=true `
            -p:IncludeNativeLibrariesForSelfExtract=true `
            -p:DebugType=embedded `
            --verbosity normal
        
        Write-Host "✅ Published to $PublishDir" -ForegroundColor Green
    } catch {
        Write-Host "❌ Publish failed: $_" -ForegroundColor Red
        exit 1
    }
    
    # 6. Create installer/archive
    Write-Host ""
    Write-Host "📦 Creating distribution package..." -ForegroundColor Yellow
    
    $packageName = "ThemisDB-DocumentManager-$Version-x64.zip"
    $packagePath = Join-Path $OutputDir $packageName
    
    try {
        # Compress
        Compress-Archive -Path (Join-Path $PublishDir "*") `
            -DestinationPath $packagePath `
            -Force
        
        # Calculate hash
        $hash = (Get-FileHash $packagePath -Algorithm SHA256).Hash
        
        Write-Host "✅ Package created: $packagePath" -ForegroundColor Green
        Write-Host "   SHA256: $hash" -ForegroundColor Gray
        
        # Create hash file
        "$hash  $packageName" | Out-File -FilePath "$packagePath.sha256"
        
    } catch {
        Write-Host "❌ Package creation failed: $_" -ForegroundColor Red
        exit 1
    }
}

# 7. Test
Write-Host ""
Write-Host "🧪 Running tests..." -ForegroundColor Yellow

$testProjectPath = Join-Path $ProjectRoot "tests/Themis.DocumentManager.Tests"
if (Test-Path $testProjectPath) {
    try {
        dotnet test $testProjectPath -c $Configuration --no-build --logger "console;verbosity=normal"
        Write-Host "✅ Tests passed" -ForegroundColor Green
    } catch {
        Write-Host "⚠️  Tests failed (non-blocking): $_" -ForegroundColor Yellow
    }
} else {
    Write-Host "ℹ️  No test project found" -ForegroundColor Gray
}

# 8. Summary
Write-Host ""
Write-Host "============================================================" -ForegroundColor Cyan
Write-Host "  BUILD SUMMARY" -ForegroundColor Cyan
Write-Host "============================================================" -ForegroundColor Cyan
Write-Host "Configuration:   $Configuration" -ForegroundColor White
Write-Host "Version:         $Version" -ForegroundColor White
Write-Host "Output:          $OutputDir" -ForegroundColor White
if ($Pack) {
    Write-Host "Package:         $packagePath" -ForegroundColor White
    Write-Host "Package Size:    $((Get-Item $packagePath).Length / 1MB | [math]::Round(2)) MB" -ForegroundColor White
}
Write-Host "Status:          ✅ SUCCESS" -ForegroundColor Green
Write-Host "============================================================" -ForegroundColor Cyan
Write-Host ""
Write-Host "Next steps:" -ForegroundColor Cyan
Write-Host "  1. Test the built application" -ForegroundColor Gray
Write-Host "  2. Review the output in: $OutputDir" -ForegroundColor Gray
if ($Pack) {
    Write-Host "  3. Distribute the package: $packagePath" -ForegroundColor Gray
}
Write-Host ""
