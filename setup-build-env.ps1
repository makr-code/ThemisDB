#Requires -Version 5.1
<#
.SYNOPSIS
    Richtet die Build-Umgebung für ThemisDB unter Windows ein.

.DESCRIPTION
    Dieses Script findet Visual Studio 2022, setzt alle benötigten Umgebungsvariablen
    und PATH-Einträge für MSVC, Ninja und vcpkg.

.PARAMETER Permanent
    Wenn gesetzt, werden die Umgebungsvariablen dauerhaft in der Benutzersitzung gespeichert.
    
.PARAMETER Architecture
    Ziel-Architektur (x64, x86, arm64). Standard: x64

.EXAMPLE
    .\setup-build-env.ps1
    Richtet die Umgebung nur für die aktuelle PowerShell-Sitzung ein.

.EXAMPLE
    .\setup-build-env.ps1 -Permanent
    Setzt die Umgebungsvariablen dauerhaft für den Benutzer.
#>

param(
    [switch]$Permanent,
    [ValidateSet('x64', 'x86', 'arm64')]
    [string]$Architecture = 'x64'
)

$ErrorActionPreference = 'Stop'

Write-Host "=== ThemisDB Build-Umgebung Setup ===" -ForegroundColor Cyan
Write-Host ""

# Visual Studio finden (priorisiert 2026, dann 2022)
function Find-VisualStudio {
    $vswhere = "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe"
    
    if (-not (Test-Path $vswhere)) {
        Write-Error "vswhere.exe nicht gefunden. Bitte Visual Studio 2022/2026 installieren."
        return $null
    }
    
    # Erst mit spezifischen Komponenten versuchen
    $vsPath = & $vswhere -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath 2>$null
    
    # Falls nicht gefunden, ohne spezifische Komponenten-Anforderung
    if (-not $vsPath) {
        $vsPath = & $vswhere -latest -products * -property installationPath 2>$null
    }
    
    # Als letzten Versuch: bekannte Pfade durchsuchen (neueste zuerst)
    if (-not $vsPath) {
        $knownPaths = @(
            # VS 2026 (Version 18.x - Insiders/Preview)
            "C:\Program Files\Microsoft Visual Studio\18\Insiders",
            "C:\Program Files\Microsoft Visual Studio\18\Professional",
            "C:\Program Files\Microsoft Visual Studio\18\Enterprise",
            "C:\Program Files\Microsoft Visual Studio\18\Community",
            "C:\Program Files\Microsoft Visual Studio\2026\Professional",
            "C:\Program Files\Microsoft Visual Studio\2026\Enterprise",
            "C:\Program Files\Microsoft Visual Studio\2026\Community",
            # VS 2022 (Version 17.x)
            "C:\Program Files\Microsoft Visual Studio\2022\Professional",
            "C:\Program Files\Microsoft Visual Studio\2022\Enterprise",
            "C:\Program Files\Microsoft Visual Studio\2022\Community",
            # x86 Pfade
            "C:\Program Files (x86)\Microsoft Visual Studio\18\Insiders",
            "C:\Program Files (x86)\Microsoft Visual Studio\18\Professional",
            "C:\Program Files (x86)\Microsoft Visual Studio\2026\Professional",
            "C:\Program Files (x86)\Microsoft Visual Studio\2026\Enterprise",
            "C:\Program Files (x86)\Microsoft Visual Studio\2026\Community",
            "C:\Program Files (x86)\Microsoft Visual Studio\2022\Professional",
            "C:\Program Files (x86)\Microsoft Visual Studio\2022\Enterprise",
            "C:\Program Files (x86)\Microsoft Visual Studio\2022\Community"
        )
        foreach ($path in $knownPaths) {
            if (Test-Path $path) {
                $vsPath = $path
                break
            }
        }
    }
    
    if (-not $vsPath) {
        Write-Error "Visual Studio 2022/2026 nicht gefunden."
        return $null
    }
    
    return $vsPath
}

# MSVC Version finden
function Find-MSVCVersion {
    param([string]$vsPath)
    
    $msvcBase = Join-Path $vsPath "VC\Tools\MSVC"
    if (-not (Test-Path $msvcBase)) {
        Write-Error "MSVC Tools nicht gefunden in: $msvcBase"
        return $null
    }
    
    $versions = Get-ChildItem $msvcBase -Directory | Sort-Object Name -Descending
    if ($versions.Count -eq 0) {
        Write-Error "Keine MSVC-Version gefunden"
        return $null
    }
    
    return $versions[0].Name
}

# Windows SDK Version finden
function Find-WindowsSDK {
    $sdkBase = "${env:ProgramFiles(x86)}\Windows Kits\10\bin"
    if (-not (Test-Path $sdkBase)) {
        Write-Warning "Windows SDK 10 nicht gefunden"
        return $null
    }
    
    $versions = Get-ChildItem $sdkBase -Directory | Where-Object { $_.Name -match '^\d+\.\d+' } | Sort-Object Name -Descending
    if ($versions.Count -eq 0) {
        return $null
    }
    
    return $versions[0].Name
}

# Hauptlogik
try {
    # Visual Studio finden
    Write-Host "Suche Visual Studio 2022..." -NoNewline
    $vsPath = Find-VisualStudio
    if (-not $vsPath) { exit 1 }
    Write-Host " Gefunden: $vsPath" -ForegroundColor Green
    
    # MSVC Version ermitteln
    Write-Host "Ermittle MSVC-Version..." -NoNewline
    $msvcVersion = Find-MSVCVersion -vsPath $vsPath
    if (-not $msvcVersion) { exit 1 }
    Write-Host " $msvcVersion" -ForegroundColor Green
    
    # Windows SDK finden
    Write-Host "Suche Windows SDK..." -NoNewline
    $sdkVersion = Find-WindowsSDK
    if ($sdkVersion) {
        Write-Host " $sdkVersion" -ForegroundColor Green
    } else {
        Write-Host " Nicht gefunden (optional)" -ForegroundColor Yellow
    }
    
    # Pfade zusammenstellen
    $newPaths = @()
    
    # MSVC Compiler und Tools
    $msvcBinPath = Join-Path $vsPath "VC\Tools\MSVC\$msvcVersion\bin\Hostx64\$Architecture"
    if (Test-Path $msvcBinPath) {
        $newPaths += $msvcBinPath
        Write-Host "✓ MSVC Compiler: $msvcBinPath" -ForegroundColor Green
    }
    
    # Ninja (über VS)
    $ninjaPath = Join-Path $vsPath "Common7\IDE\CommonExtensions\Microsoft\CMake\Ninja"
    if (Test-Path $ninjaPath) {
        $newPaths += $ninjaPath
        Write-Host "✓ Ninja: $ninjaPath" -ForegroundColor Green
    } else {
        Write-Warning "Ninja nicht in Visual Studio gefunden"
    }
    
    # CMake (über VS)
    $cmakePath = Join-Path $vsPath "Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin"
    if (Test-Path $cmakePath) {
        $newPaths += $cmakePath
        Write-Host "✓ CMake: $cmakePath" -ForegroundColor Green
    }
    
    # Windows SDK Tools
    if ($sdkVersion) {
        $sdkBinPath = "${env:ProgramFiles(x86)}\Windows Kits\10\bin\$sdkVersion\$Architecture"
        if (Test-Path $sdkBinPath) {
            $newPaths += $sdkBinPath
            Write-Host "✓ Windows SDK: $sdkBinPath" -ForegroundColor Green
        }
    }
    
    # Visual Studio Common Tools
    $vsCommonTools = Join-Path $vsPath "Common7\Tools"
    if (Test-Path $vsCommonTools) {
        $newPaths += $vsCommonTools
    }
    
    # Visual Studio IDE
    $vsIDE = Join-Path $vsPath "Common7\IDE"
    if (Test-Path $vsIDE) {
        $newPaths += $vsIDE
    }
    
    Write-Host ""
    
    # Umgebungsvariablen setzen
    $vcpkgRoot = Join-Path $PSScriptRoot "vcpkg"
    
    if ($Permanent) {
        Write-Host "Setze dauerhafte Umgebungsvariablen..." -ForegroundColor Yellow
        
        # Aktuellen User-PATH holen
        $currentPath = [Environment]::GetEnvironmentVariable('PATH', 'User')
        $pathList = $currentPath -split ';' | Where-Object { $_ }
        
        # Neue Pfade hinzufügen (wenn nicht bereits vorhanden)
        $updated = $false
        foreach ($newPath in $newPaths) {
            if ($pathList -notcontains $newPath) {
                $pathList += $newPath
                $updated = $true
                Write-Host "  + $newPath" -ForegroundColor Green
            }
        }
        
        if ($updated) {
            $newPathString = ($pathList -join ';')
            [Environment]::SetEnvironmentVariable('PATH', $newPathString, 'User')
            Write-Host "✓ PATH aktualisiert (dauerhaft)" -ForegroundColor Green
        } else {
            Write-Host "✓ PATH bereits korrekt konfiguriert" -ForegroundColor Green
        }
        
        # VCPKG_ROOT setzen
        [Environment]::SetEnvironmentVariable('VCPKG_ROOT', $vcpkgRoot, 'User')
        Write-Host "✓ VCPKG_ROOT gesetzt auf: $vcpkgRoot" -ForegroundColor Green
        
        # VS-spezifische Umgebungsvariablen
        [Environment]::SetEnvironmentVariable('VSINSTALLDIR', $vsPath, 'User')
        [Environment]::SetEnvironmentVariable('VCToolsVersion', $msvcVersion, 'User')
        
        Write-Host ""
        Write-Host "WICHTIG: Bitte öffnen Sie eine neue PowerShell-Sitzung, damit die Änderungen wirksam werden!" -ForegroundColor Yellow
        
    } else {
        Write-Host "Setze Umgebungsvariablen für diese Sitzung..." -ForegroundColor Cyan
        
        # Pfade zur aktuellen Sitzung hinzufügen
        foreach ($newPath in $newPaths) {
            if ($env:PATH -notlike "*$newPath*") {
                $env:PATH = "$newPath;$env:PATH"
            }
        }
        
        # Umgebungsvariablen für die Sitzung
        $env:VCPKG_ROOT = $vcpkgRoot
        $env:VSINSTALLDIR = $vsPath
        $env:VCToolsVersion = $msvcVersion
        $env:VCToolsInstallDir = Join-Path $vsPath "VC\Tools\MSVC\$msvcVersion\"
        
        if ($sdkVersion) {
            $env:WindowsSDKVersion = "$sdkVersion\"
            $env:WindowsSdkDir = "${env:ProgramFiles(x86)}\Windows Kits\10\"
        }
        
        # MSVC-spezifische Variablen
        $env:INCLUDE = Join-Path $vsPath "VC\Tools\MSVC\$msvcVersion\include"
        if ($sdkVersion) {
            $env:INCLUDE += ";${env:ProgramFiles(x86)}\Windows Kits\10\Include\$sdkVersion\ucrt"
            $env:INCLUDE += ";${env:ProgramFiles(x86)}\Windows Kits\10\Include\$sdkVersion\um"
            $env:INCLUDE += ";${env:ProgramFiles(x86)}\Windows Kits\10\Include\$sdkVersion\shared"
        }
        
        $env:LIB = Join-Path $vsPath "VC\Tools\MSVC\$msvcVersion\lib\$Architecture"
        if ($sdkVersion) {
            $env:LIB += ";${env:ProgramFiles(x86)}\Windows Kits\10\Lib\$sdkVersion\ucrt\$Architecture"
            $env:LIB += ";${env:ProgramFiles(x86)}\Windows Kits\10\Lib\$sdkVersion\um\$Architecture"
        }
        
        Write-Host "✓ Umgebung für diese Sitzung konfiguriert" -ForegroundColor Green
    }
    
    Write-Host ""
    Write-Host "=== Verifikation ===" -ForegroundColor Cyan
    
    # Compiler prüfen
    Write-Host "Prüfe Compiler..." -NoNewline
    $cl = Get-Command cl.exe -ErrorAction SilentlyContinue
    if ($cl) {
        Write-Host " cl.exe gefunden: $($cl.Source)" -ForegroundColor Green
    } else {
        Write-Host " cl.exe NICHT gefunden!" -ForegroundColor Red
    }
    
    # Ninja prüfen
    Write-Host "Prüfe Ninja..." -NoNewline
    $ninja = Get-Command ninja.exe -ErrorAction SilentlyContinue
    if ($ninja) {
        Write-Host " ninja.exe gefunden: $($ninja.Source)" -ForegroundColor Green
    } else {
        Write-Host " ninja.exe NICHT gefunden!" -ForegroundColor Red
    }
    
    # CMake prüfen
    Write-Host "Prüfe CMake..." -NoNewline
    $cmake = Get-Command cmake.exe -ErrorAction SilentlyContinue
    if ($cmake) {
        Write-Host " cmake.exe gefunden: $($cmake.Source)" -ForegroundColor Green
    } else {
        Write-Host " cmake.exe NICHT gefunden!" -ForegroundColor Red
    }
    
    Write-Host ""
    Write-Host "Build-Umgebung erfolgreich eingerichtet!" -ForegroundColor Green
    Write-Host ""
    Write-Host "Verfügbare CMake-Presets:" -ForegroundColor Cyan
    Write-Host "  - windows-ninja-msvc-release   (Ninja + MSVC Release)" -ForegroundColor White
    Write-Host "  - windows-ninja-msvc-debug     (Ninja + MSVC Debug)" -ForegroundColor White
    Write-Host "  - msvc-vs-release              (Visual Studio Generator)" -ForegroundColor White
    Write-Host ""
    Write-Host "Beispiel:" -ForegroundColor Cyan
    Write-Host "  cmake --preset windows-ninja-msvc-release" -ForegroundColor White
    Write-Host "  cmake --build build-msvc-ninja-release" -ForegroundColor White
    
} catch {
    Write-Error "Fehler beim Setup: $_"
    exit 1
}
