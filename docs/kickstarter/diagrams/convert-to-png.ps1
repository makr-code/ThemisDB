#!/usr/bin/env pwsh
# Convert Mermaid diagrams to PNG images
# Requires: npm, mermaid-cli (mermaid)
# Installation: npm install -g mermaid-cli

param(
    [string]$DiagramDir = "$(Split-Path -Parent $PSCommandPath)",
    [string]$OutputDir = "$(Split-Path -Parent $PSCommandPath)/png"
)

# Ensure output directory exists
if (!(Test-Path $OutputDir)) {
    New-Item -ItemType Directory -Path $OutputDir -Force | Out-Null
    Write-Host "[OK] Created output directory: $OutputDir"
}

# Check if mermaid CLI is installed
$mermaidCmd = Get-Command mmdc -ErrorAction SilentlyContinue
if (!$mermaidCmd) {
    Write-Host "[WARN] mermaid-cli not found. Installing globally..."
    npm install -g mermaid-cli
}

# Find all .mmd files
$mmdFiles = Get-ChildItem -Path $DiagramDir -Filter "*.mmd" -File

if ($mmdFiles.Count -eq 0) {
    Write-Host "[WARN] No .mmd files found in $DiagramDir"
    exit 1
}

Write-Host "Converting $($mmdFiles.Count) Mermaid diagrams to PNG..."

foreach ($file in $mmdFiles) {
    $inputPath = $file.FullName
    $outputPath = Join-Path -Path $OutputDir -ChildPath "$($file.BaseName).png"
    
    Write-Host "  Converting: $($file.Name)..."
    
    try {
        # Use mermaid CLI to convert to PNG
        mmdc -i $inputPath -o $outputPath -w 1920 -H 1080 -s 2
        Write-Host "    [OK] Saved: $outputPath"
    }
    catch {
        Write-Host "    [ERROR] Error converting $($file.Name): $_"
    }
}

Write-Host "[DONE] Conversion complete. PNG files saved to: $OutputDir"
