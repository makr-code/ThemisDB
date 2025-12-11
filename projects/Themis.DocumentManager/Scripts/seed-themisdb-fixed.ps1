# ThemisDB Test Data Seeding Script
# Erstellt umfangreiche Testdaten für DSM (Document Management System)

param(
    [string]$ThemisDbUrl = "http://localhost:8765",
    [string]$Username = "admin",
    [string]$Password = "admin"
)

$ErrorActionPreference = "Stop"

Write-Host "=== ThemisDB Test Data Seed ===" -ForegroundColor Cyan
Write-Host "Target: $ThemisDbUrl" -ForegroundColor Gray

# Helper Functions
function Invoke-ThemisQuery {
    param([string]$Query, [object]$BindVars = @{})
    
    $body = @{
        query = $Query
        bindVars = $BindVars
    } | ConvertTo-Json -Depth 10
    
    try {
        $response = Invoke-RestMethod -Uri "$ThemisDbUrl/query" -Method Post -Body $body -ContentType "application/json"
        return $response
    }
    catch {
        Write-Warning "Query failed: $($_.Exception.Message)"
        return $null
    }
}

function New-TestUser {
    param([string]$Username, [string]$DisplayName, [string]$Role)
    
    $query = @"
INSERT {
    _key: @username,
    username: @username,
    displayName: @displayName,
    email: CONCAT(@username, '@themis-test.local'),
    role: @role,
    department: @dept,
    active: true,
    createdAt: DATE_NOW()
} INTO users
RETURN NEW
"@
    
    Invoke-ThemisQuery -Query $query -BindVars @{
        username = $Username
        displayName = $DisplayName
        role = $Role
        dept = @("Recht", "Personal", "IT", "Finanzen", "Einkauf") | Get-Random
    }
}

function New-TestDocument {
    param(
        [string]$Title,
        [string]$Type,
        [string]$Author,
        [string]$Status = "Active",
        [hashtable]$Metadata = @{}
    )
    
    $query = @"
INSERT {
    title: @title,
    documentType: @type,
    author: @author,
    status: @status,
    createdAt: DATE_NOW() - RAND() * 86400000 * 365,
    modifiedAt: DATE_NOW() - RAND() * 86400000 * 30,
    metadata: @metadata,
    content: CONCAT('Inhalt des Dokuments: ', @title),
    fileSize: FLOOR(RAND() * 5000000) + 10000,
    mimeType: 'application/pdf',
    classification: @classification,
    retentionYears: @retention,
    tags: @tags
} INTO documents
RETURN NEW
"@
    
    Invoke-ThemisQuery -Query $query -BindVars @{
        title = $Title
        type = $Type
        author = $Author
        status = $Status
        metadata = $Metadata
        classification = @("Öffentlich", "Intern", "Vertraulich", "Geheim") | Get-Random
        retention = @(5, 7, 10, 15, 30) | Get-Random
        tags = @(@("Vertrag", "Rechnung", "Protokoll", "Bericht", "Antrag") | Get-Random -Count 2)
    }
}

function New-TestProcess {
    param(
        [string]$Name,
        [string]$Type,
        [string]$Owner,
        [string]$Status = "InProgress"
    )
    
    $query = @"
INSERT {
    processName: @name,
    processType: @type,
    owner: @owner,
    status: @status,
    startDate: DATE_NOW() - RAND() * 86400000 * 180,
    dueDate: DATE_NOW() + RAND() * 86400000 * 90,
    priority: @priority,
    description: CONCAT('Prozess: ', @name),
    attachedDocuments: [],
    participants: @participants,
    steps: @steps
} INTO processes
RETURN NEW
"@
    
    Invoke-ThemisQuery -Query $query -BindVars @{
        name = $Name
        type = $Type
        owner = $Owner
        status = $Status
        priority = @("Low", "Medium", "High", "Critical") | Get-Random
        participants = @($Owner, "user1", "user2") | Get-Random -Count 2
        steps = @(
            @{ name = "Antrag stellen"; completed = $true }
            @{ name = "Prüfung"; completed = $false }
            @{ name = "Genehmigung"; completed = $false }
        )
    }
}

function New-TestFile {
    param(
        [string]$Name,
        [string]$FolderPath,
        [string]$Owner
    )
    
    $query = @"
INSERT {
    fileName: @name,
    folderPath: @path,
    owner: @owner,
    createdAt: DATE_NOW() - RAND() * 86400000 * 730,
    modifiedAt: DATE_NOW() - RAND() * 86400000 * 60,
    fileType: @fileType,
    fileSize: FLOOR(RAND() * 10000000) + 1000,
    mimeType: @mimeType,
    checksum: MD5(CONCAT(@name, TO_STRING(DATE_NOW()))),
    locked: false
} INTO files
RETURN NEW
"@
    
    $ext = @(".pdf", ".docx", ".xlsx", ".pptx", ".txt", ".jpg") | Get-Random
    
    Invoke-ThemisQuery -Query $query -BindVars @{
        name = $Name + $ext
        path = $FolderPath
        owner = $Owner
        fileType = $ext.TrimStart('.')
        mimeType = switch ($ext) {
            ".pdf" { "application/pdf" }
            ".docx" { "application/vnd.openxmlformats-officedocument.wordprocessingml.document" }
            ".xlsx" { "application/vnd.openxmlformats-officedocument.spreadsheetml.sheet" }
            ".pptx" { "application/vnd.openxmlformats-officedocument.presentationml.presentation" }
            ".txt" { "text/plain" }
            ".jpg" { "image/jpeg" }
            default { "application/octet-stream" }
        }
    }
}

# Main Seeding Logic
Write-Host "`n[1/7] Creating Users..." -ForegroundColor Yellow

$users = @(
    @{ Username = "max.mustermann"; DisplayName = "Max Mustermann"; Role = "User" }
    @{ Username = "anna.schmidt"; DisplayName = "Anna Schmidt"; Role = "Editor" }
    @{ Username = "thomas.mueller"; DisplayName = "Thomas Müller"; Role = "Admin" }
    @{ Username = "lisa.weber"; DisplayName = "Lisa Weber"; Role = "User" }
    @{ Username = "michael.braun"; DisplayName = "Michael Braun"; Role = "Editor" }
    @{ Username = "sarah.klein"; DisplayName = "Sarah Klein"; Role = "User" }
    @{ Username = "peter.wolf"; DisplayName = "Peter Wolf"; Role = "Manager" }
    @{ Username = "julia.neumann"; DisplayName = "Julia Neumann"; Role = "User" }
    @{ Username = "david.schwarz"; DisplayName = "David Schwarz"; Role = "Editor" }
    @{ Username = "laura.hofmann"; DisplayName = "Laura Hofmann"; Role = "User" }
)

foreach ($user in $users) {
    New-TestUser @user
    Write-Host "  ✓ Created user: $($user.DisplayName)" -ForegroundColor Green
}

Write-Host "`n[2/7] Creating Documents..." -ForegroundColor Yellow

$documentTypes = @(
    @{ Type = "Vertrag"; Count = 50 }
    @{ Type = "Rechnung"; Count = 100 }
    @{ Type = "Protokoll"; Count = 75 }
    @{ Type = "Bericht"; Count = 60 }
    @{ Type = "Antrag"; Count = 40 }
    @{ Type = "Beschluss"; Count = 30 }
    @{ Type = "Akte"; Count = 80 }
    @{ Type = "Schriftverkehr"; Count = 120 }
)

$counter = 0
foreach ($docType in $documentTypes) {
    for ($i = 1; $i -le $docType.Count; $i++) {
        $author = $users | Get-Random | Select-Object -ExpandProperty Username
        $title = "$($docType.Type) Nr. $i - $(Get-Random -Minimum 1000 -Maximum 9999)"
        
        New-TestDocument -Title $title -Type $docType.Type -Author $author -Metadata @{
            department = @("Recht", "Personal", "IT", "Finanzen", "Einkauf") | Get-Random
            project = "PRJ-$(Get-Random -Minimum 100 -Maximum 999)"
            confidential = (Get-Random -Maximum 10) -lt 3
        }
        
        $counter++
        if ($counter % 25 -eq 0) {
            Write-Host "  ✓ Created $counter documents..." -ForegroundColor Green
        }
    }
}

Write-Host "  ✓ Total: $counter documents created" -ForegroundColor Green

Write-Host "`n[3/7] Creating Processes..." -ForegroundColor Yellow

$processTypes = @(
    "Genehmigungsverfahren", "Beschaffung", "Personalantrag", "Budgetplanung",
    "Vertragsabschluss", "Projektfreigabe", "Datenschutzprüfung", "Risikoanalyse"
)

for ($i = 1; $i -le 50; $i++) {
    $owner = $users | Get-Random | Select-Object -ExpandProperty Username
    $processType = $processTypes | Get-Random
    $name = "$processType $i/2024"
    
    New-TestProcess -Name $name -Type $processType -Owner $owner
    
    if ($i % 10 -eq 0) {
        Write-Host "  ✓ Created $i processes..." -ForegroundColor Green
    }
}

Write-Host "`n[4/7] Creating File Structure..." -ForegroundColor Yellow

$folders = @(
    "/Verträge/2024",
    "/Verträge/2023",
    "/Rechnungen/Eingang/2024",
    "/Rechnungen/Ausgang/2024",
    "/Personal/Bewerbungen",
    "/Personal/Verträge",
    "/Projekte/PRJ-001",
    "/Projekte/PRJ-002",
    "/Akten/Laufend",
    "/Akten/Archiv"
)

$fileCounter = 0
foreach ($folder in $folders) {
    for ($i = 1; $i -le 20; $i++) {
        $owner = $users | Get-Random | Select-Object -ExpandProperty Username
        $fileName = "Dokument_$(Get-Random -Minimum 1000 -Maximum 9999)"
        
        New-TestFile -Name $fileName -FolderPath $folder -Owner $owner
        $fileCounter++
    }
}

Write-Host "  ✓ Created $fileCounter files in $($folders.Count) folders" -ForegroundColor Green

Write-Host "`n[5/7] Creating Relations (Document → Process)..." -ForegroundColor Yellow

$relQuery = @"
FOR doc IN documents
    LIMIT 100
    LET proc = FIRST(FOR p IN processes SORT RAND() LIMIT 1 RETURN p)
    FILTER proc != null
    INSERT {
        _from: doc._id,
        _to: proc._id,
        relationType: 'attached_to',
        createdAt: DATE_NOW()
    } INTO document_process_edges
    OPTIONS { ignoreErrors: true }
RETURN NEW
"@

Invoke-ThemisQuery -Query $relQuery
Write-Host "  ✓ Created document-process relations" -ForegroundColor Green

Write-Host "`n[6/7] Creating Audit Log Entries..." -ForegroundColor Yellow

$auditQuery = @"
FOR i IN 1..500
    INSERT {
        timestamp: DATE_NOW() - RAND() * 86400000 * 90,
        action: RAND() > 0.5 ? 'VIEW' : (RAND() > 0.5 ? 'EDIT' : 'DELETE'),
        user: CONCAT('user', FLOOR(RAND() * 10) + 1),
        documentId: CONCAT('doc_', FLOOR(RAND() * 500)),
        ipAddress: CONCAT('192.168.', FLOOR(RAND() * 255), '.', FLOOR(RAND() * 255)),
        userAgent: 'ThemisDB DSM/1.0'
    } INTO audit_logs
RETURN NEW
"@

Invoke-ThemisQuery -Query $auditQuery
Write-Host "  ✓ Created 500 audit log entries" -ForegroundColor Green

Write-Host "`n[7/7] Creating Metadata & Tags..." -ForegroundColor Yellow

$tagQuery = @"
FOR doc IN documents
    LIMIT 300
    UPDATE doc WITH {
        tags: (FOR i IN 1..FLOOR(RAND() * 5) + 1
               RETURN CONCAT('tag-', FLOOR(RAND() * 50)))
    } IN documents
RETURN NEW
"@

Invoke-ThemisQuery -Query $tagQuery
Write-Host "  ✓ Added tags to documents" -ForegroundColor Green

Write-Host "`n[8/11] Creating Wiedervorlagen..." -ForegroundColor Yellow

$wiedervorlageQuery = @"
FOR i IN 1..80
    LET user = CONCAT('user', FLOOR(RAND() * 10) + 1)
    LET doc = FIRST(FOR d IN documents SORT RAND() LIMIT 1 RETURN d)
    INSERT {
        documentId: doc._id,
        documentTitle: doc.title,
        assignedTo: user,
        dueDate: DATE_NOW() + RAND() * 86400000 * 120,
        createdAt: DATE_NOW() - RAND() * 86400000 * 30,
        priority: RAND() > 0.7 ? 'High' : (RAND() > 0.5 ? 'Medium' : 'Low'),
        status: RAND() > 0.3 ? 'Pending' : 'Completed',
        notes: CONCAT('Wiedervorlage für ', doc.title),
        reminderSent: RAND() > 0.5
    } INTO wiedervorlagen
RETURN NEW
"@

Invoke-ThemisQuery -Query $wiedervorlageQuery
Write-Host "  ✓ Created 80 Wiedervorlagen (reminders)" -ForegroundColor Green

Write-Host "`n[9/11] Creating Mitzeichnungen..." -ForegroundColor Yellow

$mitzeichnungQuery = @"
FOR i IN 1..60
    LET doc = FIRST(FOR d IN documents FILTER d.documentType IN ['Vertrag', 'Beschluss', 'Antrag'] SORT RAND() LIMIT 1 RETURN d)
    LET initiator = CONCAT('user', FLOOR(RAND() * 10) + 1)
    INSERT {
        documentId: doc._id,
        documentTitle: doc.title,
        initiator: initiator,
        createdAt: DATE_NOW() - RAND() * 86400000 * 60,
        status: RAND() > 0.4 ? 'InProgress' : (RAND() > 0.5 ? 'Approved' : 'Rejected'),
        signers: (FOR j IN 1..FLOOR(RAND() * 4) + 2
                  RETURN {
                      user: CONCAT('user', FLOOR(RAND() * 10) + 1),
                      signed: RAND() > 0.5,
                      signedAt: RAND() > 0.5 ? DATE_NOW() - RAND() * 86400000 * 30 : null,
                      comment: RAND() > 0.7 ? 'Genehmigt' : null
                  }),
        deadline: DATE_NOW() + RAND() * 86400000 * 30,
        description: CONCAT('Mitzeichnung für ', doc.title)
    } INTO mitzeichnungen
RETURN NEW
"@

Invoke-ThemisQuery -Query $mitzeichnungQuery
Write-Host "  ✓ Created 60 Mitzeichnungen (co-signatures)" -ForegroundColor Green

Write-Host "`n[10/11] Creating Email Threads..." -ForegroundColor Yellow

$emailQuery = @"
FOR i IN 1..100
    LET sender = CONCAT('user', FLOOR(RAND() * 10) + 1)
    LET threadId = CONCAT('thread-', MD5(TO_STRING(i)))
    INSERT {
        threadId: threadId,
        subject: CONCAT('RE: Anfrage ', FLOOR(RAND() * 1000)),
        sender: sender,
        recipients: (FOR j IN 1..FLOOR(RAND() * 3) + 1
                     RETURN CONCAT('user', FLOOR(RAND() * 10) + 1)),
        sentAt: DATE_NOW() - RAND() * 86400000 * 90,
        body: CONCAT('Email-Text für Thread ', threadId),
        attachments: (FOR k IN 1..FLOOR(RAND() * 3)
                      RETURN {
                          fileName: CONCAT('Anhang_', k, '.pdf'),
                          size: FLOOR(RAND() * 2000000) + 10000
                      }),
        inReplyTo: RAND() > 0.6 ? CONCAT('thread-', MD5(TO_STRING(i - 1))) : null,
        read: RAND() > 0.3,
        flagged: RAND() > 0.8,
        tags: ['Inbox']
    } INTO emails
RETURN NEW
"@

Invoke-ThemisQuery -Query $emailQuery
Write-Host "  ✓ Created 100 Email messages" -ForegroundColor Green

Write-Host "`n[11/11] Creating Classification & Retention Rules..." -ForegroundColor Yellow

$rulesQuery = @"
FOR rule IN [
    {
        name: 'Verträge Langzeitarchivierung',
        documentType: 'Vertrag',
        retentionYears: 30,
        classification: 'Vertraulich',
        autoArchive: true
    },
    {
        name: 'Rechnungen Steuerrelevant',
        documentType: 'Rechnung',
        retentionYears: 10,
        classification: 'Intern',
        autoArchive: true
    },
    {
        name: 'Protokolle Standard',
        documentType: 'Protokoll',
        retentionYears: 7,
        classification: 'Intern',
        autoArchive: false
    },
    {
        name: 'Personalakten DSGVO',
        documentType: 'Akte',
        retentionYears: 15,
        classification: 'Geheim',
        autoArchive: true
    },
    {
        name: 'Schriftverkehr Kurzfristig',
        documentType: 'Schriftverkehr',
        retentionYears: 5,
        classification: 'Intern',
        autoArchive: false
    }
]
    INSERT MERGE(rule, {
        createdAt: DATE_NOW(),
        active: true,
        createdBy: 'admin'
    }) INTO retention_rules
RETURN NEW
"@

Invoke-ThemisQuery -Query $rulesQuery
Write-Host "  ✓ Created 5 retention/classification rules" -ForegroundColor Green

Write-Host "`n=== Seeding Complete ===" -ForegroundColor Cyan
Write-Host "Summary:" -ForegroundColor Yellow
Write-Host "  • Users: $($users.Count)" -ForegroundColor White
Write-Host "  • Documents: ~555" -ForegroundColor White
Write-Host "  • Processes: 50" -ForegroundColor White
Write-Host "  • Files: $fileCounter" -ForegroundColor White
Write-Host "  • Relations: ~100" -ForegroundColor White
Write-Host "  • Audit Logs: 500" -ForegroundColor White
Write-Host "  • Wiedervorlagen: 80" -ForegroundColor White
Write-Host "  • Mitzeichnungen: 60" -ForegroundColor White
Write-Host "  • Emails: 100" -ForegroundColor White
Write-Host "  • Retention Rules: 5" -ForegroundColor White
Write-Host "`nThemisDB is now populated with comprehensive test data!" -ForegroundColor Green

