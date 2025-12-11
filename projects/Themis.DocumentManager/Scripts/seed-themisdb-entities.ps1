# ThemisDB Entity Seed (REST /entities)
# ASCII-only to avoid UTF-8 issues

param(
    [string]$ThemisDbUrl = "http://localhost:8765"
)

$ErrorActionPreference = "Stop"
Write-Host "=== ThemisDB Entity Seed (/entities) ===" -ForegroundColor Cyan
Write-Host "Target: $ThemisDbUrl" -ForegroundColor Gray

function Set-Entity {
    param([string]$Urn, [object]$Object)
    $payload = @{ blob = ($Object | ConvertTo-Json -Depth 10) } | ConvertTo-Json -Depth 5
    Invoke-RestMethod -Uri "$ThemisDbUrl/entities/$Urn" -Method Put -Body $payload -ContentType "application/json"
}

$users = @(
    @{ username="max.mustermann"; displayName="Max Mustermann"; role="User"; dept="Legal" },
    @{ username="anna.schmidt"; displayName="Anna Schmidt"; role="Editor"; dept="HR" },
    @{ username="thomas.mueller"; displayName="Thomas Mueller"; role="Manager"; dept="IT" },
    @{ username="lisa.weber"; displayName="Lisa Weber"; role="User"; dept="Finance" },
    @{ username="michael.braun"; displayName="Michael Braun"; role="Admin"; dept="Ops" }
)

Write-Host "[1/2] Seeding users..." -ForegroundColor Yellow
foreach ($u in $users) {
    $urn = "users:" + $u.username
    Set-Entity -Urn $urn -Object $u | Out-Null
    Write-Host "  ok $urn" -ForegroundColor Green
}

$docs = 1..10 | ForEach-Object {
    $id = [guid]::NewGuid().ToString()
    $author = ($users | Get-Random).username
    @{ id=$id; title="Sample Document $_"; status="Active"; author=$author; createdAt=[DateTime]::UtcNow }
}

Write-Host "[2/2] Seeding documents..." -ForegroundColor Yellow
foreach ($d in $docs) {
    $urn = "documents:" + $d.id
    Set-Entity -Urn $urn -Object $d | Out-Null
    Write-Host "  ok $urn" -ForegroundColor Green
}

Write-Host "Seed complete." -ForegroundColor Cyan
