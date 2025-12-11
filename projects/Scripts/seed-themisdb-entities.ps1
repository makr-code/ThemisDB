# Seed test data into ThemisDB 0.1.0 REST API
# ASCII-only to prevent UTF-8 parsing issues

$baseUri = "http://localhost:8765"

# Test Users
$users = @(
    @{
        urn = "users:max.mustermann"
        name = "Max Mustermann"
        email = "max.mustermann@example.com"
        role = "Admin"
    },
    @{
        urn = "users:anna.schmidt"
        name = "Anna Schmidt"
        email = "anna.schmidt@example.com"
        role = "Editor"
    },
    @{
        urn = "users:thomas.mueller"
        name = "Thomas Mueller"
        email = "thomas.mueller@example.com"
        role = "Editor"
    },
    @{
        urn = "users:lisa.weber"
        name = "Lisa Weber"
        email = "lisa.weber@example.com"
        role = "Viewer"
    },
    @{
        urn = "users:michael.braun"
        name = "Michael Braun"
        email = "michael.braun@example.com"
        role = "Viewer"
    }
)

# Test Documents
$documents = @(
    @{ urn = "documents:0"; title = "Sample Document 1"; type = "Report"; status = "Active" },
    @{ urn = "documents:1"; title = "Sample Document 2"; type = "Specification"; status = "Active" },
    @{ urn = "documents:2"; title = "Sample Document 3"; type = "Guide"; status = "Archived" },
    @{ urn = "documents:3"; title = "Sample Document 4"; type = "Manual"; status = "Active" },
    @{ urn = "documents:4"; title = "Sample Document 5"; type = "Procedure"; status = "Active" },
    @{ urn = "documents:5"; title = "Sample Document 6"; type = "Policy"; status = "Archived" },
    @{ urn = "documents:6"; title = "Sample Document 7"; type = "Standard"; status = "Active" },
    @{ urn = "documents:7"; title = "Sample Document 8"; type = "Guideline"; status = "Active" },
    @{ urn = "documents:8"; title = "Sample Document 9"; type = "Framework"; status = "Active" },
    @{ urn = "documents:9"; title = "Sample Document 10"; type = "Template"; status = "Active" }
)

# Seed users
Write-Host "Seeding users..."
foreach ($user in $users) {
    $body = $user | ConvertTo-Json
    try {
        Invoke-RestMethod -Uri "$baseUri/entities/$($user.urn)" -Method Put -Body $body -ContentType "application/json" -ErrorAction Stop | Out-Null
        Write-Host "  Created: $($user.urn)"
    } catch {
        Write-Host "  Error creating $($user.urn): $_"
    }
}

# Seed documents
Write-Host "Seeding documents..."
foreach ($doc in $documents) {
    $body = $doc | ConvertTo-Json
    try {
        Invoke-RestMethod -Uri "$baseUri/entities/$($doc.urn)" -Method Put -Body $body -ContentType "application/json" -ErrorAction Stop | Out-Null
        Write-Host "  Created: $($doc.urn) - $($doc.title)"
    } catch {
        Write-Host "  Error creating $($doc.urn): $_"
    }
}

Write-Host "Seed complete."
