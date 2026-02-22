#Requires -Version 5.1
<#
.SYNOPSIS
    Research Backlog Manager - GitHub Issue Generation from Research Sources
.DESCRIPTION
    Generates GitHub Issues from docs/research/ structure, code audit for undocumented
    references, and missing module influence links. Adapted from interactive_github_master.ps1.
.VERSION
    1.0.0
#>

param(
    [string]$Repository = "makr-code/ThemisDB",
    [string]$SourcePath = $null,
    [string]$OllamaUrl = "http://localhost:11434",
    [string]$OllamaModel = "llama3.2",
    [switch]$DryRun = $false,
    [switch]$AutoCommit = $false,
    [switch]$UseAI = $true,
    [ValidateSet("interactive", "stats", "create-module-issues", "create-all-issues",
                 "ollama-settings", "audit-codebase", "generate-research-index",
                 "validate-research-links")]
    [string]$Action = "interactive",
    [string]$ModuleName = "",
    [switch]$Yes = $false,
    [switch]$Help = $false,

    # Research-specific parameters
    [ValidateSet("P1-Critical", "P2-High", "P3-Medium", "P4-Low", "all")]
    [string]$Priority = "all",
    [ValidateSet("papers", "best-practices", "architecture", "all")]
    [string]$ResearchType = "all",
    [int]$ResearchSourcesMinimum = 1,
    [switch]$IncludeUndocumentedCode = $true,
    [string]$CodeAuditPatterns = "",
    [switch]$GenerateIndexAfterCreation = $true,
    [ValidateRange(0, 600)]
    [int]$DelayBetweenIssuesSec = 0,
    [ValidateRange(0, 600)]
    [int]$DelayBetweenModulesSec = 0,
    [ValidateRange(0, 1800)]
    [int]$RateLimitCooldownSec = 120,
    [ValidateRange(0, 10)]
    [int]$RateLimitMaxRetries = 1,
    [ValidateRange(0, 60)]
    [int]$RateLimitJitterSec = 5
)

# ============================================================================
# SECTION 1: GLOBAL VARIABLES & EXIT CODES
# ============================================================================

$script:RepoRoot = $null
$script:SourceDir = $null
$script:ResearchDir = $null
$script:UseAI = $UseAI
$script:OllamaUrl = $OllamaUrl
$script:OllamaModel = $OllamaModel
$script:IsNonInteractive = ($Action -ne "interactive")
$script:ExitCodes = @{
    Success             = 0
    GenericError        = 1
    RepositoryNotFound  = 2
    GitHubAuthFailed    = 3
    GitHubCliMissing    = 4
    ResearchDirMissing  = 5
    SourceDirMissing    = 6
    InvalidPriority     = 7
    AuditFailed         = 8
    UnknownAction       = 9
    ValidationFailed    = 10
}
$script:MilestoneCache = @{}
$script:ResearchItemCache = @{}

# Priority to month mapping for research milestones
$script:PriorityMonthMap = @{
    "P1-Critical" = "2026-03"
    "P2-High"     = "2026-04"
    "P3-Medium"   = "2026-05"
    "P4-Low"      = "2026-06"
}

$script:ResearchLabels = @{
    Base            = "research-backlog"
    Papers          = "research::papers"
    BestPractices   = "research::best-practices"
    Architecture    = "research::architecture"
    Undocumented    = "research::undocumented-ref"
    P1              = "p1-critical"
    P2              = "p2-high"
    P3              = "p3-medium"
    P4              = "p4-low"
}

# Module keywords used for automatic module inference from research file content
$script:ModuleKeywords = @(
    'storage', 'index', 'transaction', 'query', 'graph', 'vector',
    'acceleration', 'cache', 'network', 'replication', 'auth',
    'llm', 'gpu', 'core', 'cdc', 'config', 'aql', 'geo'
)

# Regex pattern for detecting scientific/research foundation sections in README files
$script:ScientificSectionPattern = '(?i)(Wissenschaftliche\s+Grundlagen|Scientific\s+Foundation|Research\s+Basis|References|Forschungsgrundlage|Theoretical\s+Background)'

# Regex patterns for recognized research algorithm names in source code
$script:ResearchAlgorithmPattern = '(?i)(HNSW|FAISS|ScaNN|RaBitQ|OPQ|ColBERT|CLIP|BM25|RotatE|QuatE|TComplEx)'

function Exit-WithCode {
    param(
        [string]$Reason,
        [int]$Code
    )
    if (-not [string]::IsNullOrWhiteSpace($Reason)) {
        Write-Host "  Exit: $Reason (Code: $Code)" -ForegroundColor Red
    }
    exit $Code
}

function Show-Usage {
    Write-Host ""
    Write-Host "  Research Backlog Manager - Usage" -ForegroundColor Cyan
    Write-Host "  =============================================================" -ForegroundColor Cyan
    Write-Host ""
    Write-Host "  Parameter:" -ForegroundColor Green
    Write-Host "    -Action <interactive|stats|create-module-issues|create-all-issues|" -ForegroundColor White
    Write-Host "             ollama-settings|audit-codebase|generate-research-index|" -ForegroundColor White
    Write-Host "             validate-research-links>" -ForegroundColor White
    Write-Host "    -ModuleName <name>                (nur fuer create-module-issues)" -ForegroundColor White
    Write-Host "    -SourcePath <path>                (z.B. .\src fuer Code-Audit)" -ForegroundColor White
    Write-Host "    -Repository <owner/repo>" -ForegroundColor White
    Write-Host "    -DryRun                           (nur simulieren)" -ForegroundColor White
    Write-Host "    -UseAI                            (AI aktivieren)" -ForegroundColor White
    Write-Host "    -Yes                              (auto-bestaetigen)" -ForegroundColor White
    Write-Host "    -Priority <P1-Critical|P2-High|P3-Medium|P4-Low|all>" -ForegroundColor White
    Write-Host "    -ResearchType <papers|best-practices|architecture|all>" -ForegroundColor White
    Write-Host "    -ResearchSourcesMinimum <int>     (mind. X Quellen pro Modul)" -ForegroundColor White
    Write-Host "    -IncludeUndocumentedCode          (Code-Audit aktivieren)" -ForegroundColor White
    Write-Host "    -CodeAuditPatterns <regex>        (Patterns fuer Code-Audit)" -ForegroundColor White
    Write-Host "    -GenerateIndexAfterCreation       (Index nach Issue-Erstellung regenerieren)" -ForegroundColor White
    Write-Host "    -DelayBetweenIssuesSec            (Wartezeit zwischen Issue-Erstellungen)" -ForegroundColor White
    Write-Host "    -DelayBetweenModulesSec           (Wartezeit zwischen Modulen)" -ForegroundColor White
    Write-Host "    -Help                             (diese Hilfe anzeigen)" -ForegroundColor White
    Write-Host ""
    Write-Host "  Beispiele:" -ForegroundColor Green
    Write-Host "    .\gh\research_github_master.ps1 -Action stats" -ForegroundColor Gray
    Write-Host "    .\gh\research_github_master.ps1 -Action audit-codebase -SourcePath .\src -DryRun" -ForegroundColor Gray
    Write-Host "    .\gh\research_github_master.ps1 -Action create-module-issues -ModuleName storage -Priority P1-Critical -Yes" -ForegroundColor Gray
    Write-Host "    .\gh\research_github_master.ps1 -Action create-all-issues -Priority P1-Critical -Yes -DelayBetweenIssuesSec 2" -ForegroundColor Gray
    Write-Host "    .\gh\research_github_master.ps1 -Action generate-research-index -Yes" -ForegroundColor Gray
    Write-Host ""
    Write-Host "  Exit-Codes:" -ForegroundColor Green
    Write-Host "    0 = Erfolg" -ForegroundColor White
    Write-Host "    1 = Generischer Fehler" -ForegroundColor White
    Write-Host "    2 = Repository nicht gefunden" -ForegroundColor White
    Write-Host "    3 = GitHub Auth fehlgeschlagen" -ForegroundColor White
    Write-Host "    4 = GitHub CLI fehlt" -ForegroundColor White
    Write-Host "    5 = Research-Verzeichnis fehlt" -ForegroundColor White
    Write-Host "    6 = Source-Verzeichnis fehlt" -ForegroundColor White
    Write-Host "    7 = Ungueltige Prioritaet" -ForegroundColor White
    Write-Host "    8 = Audit fehlgeschlagen" -ForegroundColor White
    Write-Host "    9 = Unbekannte Action" -ForegroundColor White
    Write-Host "   10 = Validierung fehlgeschlagen" -ForegroundColor White
    Write-Host ""
}

# ============================================================================
# SECTION 2: UTILITY FUNCTIONS
# ============================================================================

function Normalize-MarkdownLine {
    param(
        [string]$Text,
        [int]$MaxLength = 0
    )
    if ([string]::IsNullOrWhiteSpace($Text)) { return "" }
    $value = $Text -replace "`r", " " -replace "`n", " "
    $value = [regex]::Replace($value, '[^\x20-\x7E]', ' ')
    $value = ($value -replace "\s+", " ").Trim()
    if ($MaxLength -gt 0 -and $value.Length -gt $MaxLength) {
        return ($value.Substring(0, $MaxLength - 3) + "...")
    }
    return $value
}

function Normalize-MilestoneTitle {
    param([string]$Title)
    if ([string]::IsNullOrWhiteSpace($Title)) { return "" }
    $normalized = (($Title.Trim()) -replace "\s+", " ")
    $quarterMatch = [regex]::Match($normalized, '(?i)\bQ\s*([1-4])\s*(20\d{2})\b')
    if ($quarterMatch.Success) {
        return ("Q{0} {1}" -f $quarterMatch.Groups[1].Value, $quarterMatch.Groups[2].Value)
    }
    $quarterMatchAlt = [regex]::Match($normalized, '(?i)\b(20\d{2})\s*Q\s*([1-4])\b')
    if ($quarterMatchAlt.Success) {
        return ("Q{0} {1}" -f $quarterMatchAlt.Groups[2].Value, $quarterMatchAlt.Groups[1].Value)
    }
    return $normalized
}

function Get-SectionLines {
    param(
        [array]$Lines,
        [string]$HeadingRegex
    )
    $start = -1
    for ($i = 0; $i -lt $Lines.Count; $i++) {
        if ($Lines[$i] -match $HeadingRegex) {
            $start = $i
            break
        }
    }
    if ($start -lt 0) { return @() }
    $result = @()
    for ($j = $start + 1; $j -lt $Lines.Count; $j++) {
        if ($Lines[$j] -match '^\s*#{2,6}\s+') { break }
        $result += $Lines[$j]
    }
    return $result
}

# ============================================================================
# SECTION 3: RESEARCH FUNCTIONS
# ============================================================================

function Parse-ResearchFrontmatter {
    <#
    .SYNOPSIS
        Parse YAML frontmatter from a research markdown file.
    .PARAMETER FilePath
        Path to the research file (e.g. /docs/research/GPU_VECTOR_INDEXING_RESEARCH.md)
    .OUTPUTS
        PSObject with: title, authors, year, link, influenced_modules, status, tags, priority, type
    #>
    param([string]$FilePath)

    $result = [PSCustomObject]@{
        title             = ""
        authors           = @()
        year              = ""
        link              = ""
        influenced_modules = @()
        status            = "Unknown"
        tags              = @()
        priority          = "P3-Medium"
        type              = "papers"
        file_path         = $FilePath
        file_name         = [System.IO.Path]::GetFileNameWithoutExtension($FilePath)
    }

    if (-not (Test-Path $FilePath)) { return $result }

    try {
        $content = Get-Content $FilePath -Raw -Encoding UTF8
        if ([string]::IsNullOrWhiteSpace($content)) { return $result }

        # Try to parse YAML frontmatter between --- delimiters
        $frontmatterMatch = [regex]::Match($content, '^---\s*\r?\n(.*?)\r?\n---', [System.Text.RegularExpressions.RegexOptions]::Singleline)
        if ($frontmatterMatch.Success) {
            $yaml = $frontmatterMatch.Groups[1].Value
            $lines = @($yaml -split "`r?`n")
            foreach ($line in $lines) {
                $kv = [regex]::Match($line, '^(\w[\w_-]*):\s*(.*)$')
                if (-not $kv.Success) { continue }
                $key = $kv.Groups[1].Value.Trim().ToLower()
                $val = $kv.Groups[2].Value.Trim().Trim('"').Trim("'")
                switch ($key) {
                    "title"              { $result.title = $val }
                    "authors"            { $result.authors = @($val -split ',\s*' | Where-Object { $_ }) }
                    "year"               { $result.year = $val }
                    "link"               { $result.link = $val }
                    "status"             { $result.status = $val }
                    "priority"           { $result.priority = $val }
                    "type"               { $result.type = $val }
                    "influenced_modules" { $result.influenced_modules = @($val -split ',\s*' | Where-Object { $_ }) }
                    "tags"               { $result.tags = @($val -split ',\s*' | Where-Object { $_ }) }
                }
            }
        }

        # Fall back: extract title from first H1 heading if not set
        if ([string]::IsNullOrWhiteSpace($result.title)) {
            $h1 = [regex]::Match($content, '^#\s+(.+)$', [System.Text.RegularExpressions.RegexOptions]::Multiline)
            if ($h1.Success) {
                $result.title = Normalize-MarkdownLine -Text $h1.Groups[1].Value -MaxLength 120
            } else {
                $result.title = [System.IO.Path]::GetFileNameWithoutExtension($FilePath) -replace '[_-]', ' '
            }
        }

        # Infer priority from content keywords
        if ($result.priority -eq "P3-Medium") {
            if ($content -match '(?i)\bP0\b|\bCritical\b|\bp0-critical\b') {
                $result.priority = "P1-Critical"
            } elseif ($content -match '(?i)\bP1\b|\bHigh Priority\b|\bp1-high\b') {
                $result.priority = "P2-High"
            } elseif ($content -match '(?i)\bP2\b|\bMedium Priority\b') {
                $result.priority = "P3-Medium"
            } elseif ($content -match '(?i)\bP3\b|\bLow Priority\b|\bp4-low\b') {
                $result.priority = "P4-Low"
            }
        }

        # Infer type from filename and content
        if ($result.type -eq "papers") {
            $fn = $FilePath.ToLower()
            if ($fn -match 'best.?pract|guideline|standard') {
                $result.type = "best-practices"
            } elseif ($fn -match 'architect|design|adr|decision') {
                $result.type = "architecture"
            }
        }

        # Infer influenced modules from content (module section headers, mentions)
        if ($result.influenced_modules.Count -eq 0) {
            $foundModules = @()
            foreach ($kw in $script:ModuleKeywords) {
                $pattern = '\b' + $kw + '\b'
                if ($content -imatch $pattern) {
                    $foundModules += $kw
                }
            }
            $result.influenced_modules = @($foundModules | Select-Object -Unique | Select-Object -First 5)
        }

        # Infer status from content
        if ($result.status -eq "Unknown") {
            if ($content -match '(?i)Research Complete|Implemented|Production') {
                $result.status = "Implemented"
            } elseif ($content -match '(?i)Planning|In Progress|Planned') {
                $result.status = "Planned"
            } else {
                $result.status = "Researching"
            }
        }

    } catch {
        Write-Host "    Warn: Frontmatter-Parsing fehlgeschlagen fuer $FilePath`: $_" -ForegroundColor Yellow
    }

    return $result
}

function Expand-ResearchSourceLines {
    <#
    .SYNOPSIS
        Extract structured issue parts from a research file and affected modules.
    .PARAMETER ResearchItem
        PSObject from Parse-ResearchFrontmatter
    .PARAMETER AffectedModules
        Array of module names that are influenced
    .OUTPUTS
        Array of strings representing issue body sections
    #>
    param(
        [PSObject]$ResearchItem,
        [string[]]$AffectedModules
    )

    $lines = @()

    if (-not [string]::IsNullOrWhiteSpace($ResearchItem.title)) {
        $lines += "**Quelle:** $($ResearchItem.title)"
    }

    if ($ResearchItem.authors.Count -gt 0) {
        $lines += "**Autoren:** $($ResearchItem.authors -join ', ')"
    }

    if (-not [string]::IsNullOrWhiteSpace($ResearchItem.year)) {
        $lines += "**Jahr:** $($ResearchItem.year)"
    }

    if (-not [string]::IsNullOrWhiteSpace($ResearchItem.link)) {
        $lines += "**Link:** $($ResearchItem.link)"
    }

    if ($AffectedModules.Count -gt 0) {
        $lines += "**Betroffene Module:** $($AffectedModules -join ', ')"
    }

    if ($ResearchItem.tags.Count -gt 0) {
        $lines += "**Tags:** $($ResearchItem.tags -join ', ')"
    }

    $lines += "**Status:** $($ResearchItem.status)"
    $lines += "**Typ:** $($ResearchItem.type)"
    $lines += "**Datei:** $($ResearchItem.file_path)"

    return $lines
}

function Get-UndocumentedResearchReferences {
    <#
    .SYNOPSIS
        Scan source code directory for research references that lack documentation.
    .PARAMETER SourceDirectory
        Path to the source code directory to scan
    .PARAMETER ResearchDir
        Path to docs/research directory
    .PARAMETER AuditPatterns
        Optional comma-separated regex patterns to search for
    .OUTPUTS
        Array of PSCustomObject with: ReferencedPaper, Module, FilePath, LineNumber, Status
    #>
    param(
        [string]$SourceDirectory,
        [string]$ResearchDir,
        [string]$AuditPatterns = ""
    )

    $results = @()

    if (-not (Test-Path $SourceDirectory)) {
        Write-Host "  Warn: Source-Verzeichnis nicht gefunden: $SourceDirectory" -ForegroundColor Yellow
        return $results
    }

    # Build list of known research documents for cross-referencing
    $knownResearchFiles = @()
    if (Test-Path $ResearchDir) {
        $knownResearchFiles = @(Get-ChildItem $ResearchDir -Recurse -Filter "*.md" -ErrorAction SilentlyContinue |
            ForEach-Object { [System.IO.Path]::GetFileNameWithoutExtension($_.Name).ToLower() })
    }

    # Default patterns: look for common research reference patterns in code comments
    $defaultPatterns = @(
        '(?i)//\s*(Paper|Research|Reference|See also|Based on|According to):\s*(.+)',
        '(?i)#\s*(Paper|Research|Reference|See also|Based on|According to):\s*(.+)',
        '(?i)/\*\s*(Paper|Research|Reference|See also|Based on|According to):\s*(.+)',
        '(?i)@(paper|research|reference|see)\s+(.+)',
        "$script:ResearchAlgorithmPattern\s",
        '(?i)(arxiv|doi\.org|proceedings\.mlr|papers\.nips|dl\.acm)'
    )

    if (-not [string]::IsNullOrWhiteSpace($AuditPatterns)) {
        $customPatterns = @($AuditPatterns -split ',\s*' | Where-Object { $_ })
        $defaultPatterns += $customPatterns
    }

    $sourceFiles = @(Get-ChildItem $SourceDirectory -Recurse -ErrorAction SilentlyContinue |
        Where-Object { $_.Extension -in @('.cpp', '.h', '.hpp', '.cc', '.c', '.py', '.cs', '.go', '.rs') })

    Write-Host "  Scanne $($sourceFiles.Count) Quelldateien auf undokumentierte Research-Referenzen..." -ForegroundColor Cyan

    foreach ($file in $sourceFiles) {
        $moduleDir = $file.DirectoryName
        $moduleName = Split-Path -Leaf (Split-Path -Parent $file.FullName)
        if ($moduleName -eq (Split-Path -Leaf $SourceDirectory)) {
            $moduleName = Split-Path -Leaf $file.DirectoryName
        }

        try {
            $lines = Get-Content $file.FullName -Encoding UTF8 -ErrorAction SilentlyContinue
            if (-not $lines) { continue }

            for ($i = 0; $i -lt $lines.Count; $i++) {
                $line = $lines[$i]

                foreach ($pattern in $defaultPatterns) {
                    $m = [regex]::Match($line, $pattern)
                    if (-not $m.Success) { continue }

                    $reference = if ($m.Groups.Count -gt 2) { $m.Groups[2].Value.Trim() }
                                 elseif ($m.Groups.Count -gt 1) { $m.Groups[1].Value.Trim() }
                                 else { $m.Value.Trim() }

                    $reference = Normalize-MarkdownLine -Text $reference -MaxLength 100

                    # Check if there's a corresponding research doc
                    $refNorm = $reference.ToLower() -replace '[^a-z0-9]', ''
                    $isDocumented = $knownResearchFiles | Where-Object {
                        $docNorm = $_ -replace '[^a-z0-9]', ''
                        $docNorm -like "*$refNorm*" -or $refNorm -like "*$docNorm*"
                    }

                    $relPath = $file.FullName
                    if ($relPath.StartsWith($SourceDirectory)) {
                        $relPath = $relPath.Substring($SourceDirectory.Length).TrimStart('/\')
                    }

                    $results += [PSCustomObject]@{
                        ReferencedPaper = $reference
                        Module          = $moduleName
                        FilePath        = $relPath
                        LineNumber      = $i + 1
                        Status          = if ($isDocumented) { "documented" } else { "undocumented" }
                        FullPath        = $file.FullName
                    }

                    break  # one match per line per file pass
                }
            }
        } catch {
            # Skip unreadable files silently
        }
    }

    return $results
}

function Get-MissingModuleInfluenceLinks {
    <#
    .SYNOPSIS
        Find modules that have implementations but lack a scientific foundations section in README.
    .PARAMETER SourceDirectory
        Path to the source code directory
    .OUTPUTS
        Array of PSCustomObject with: ModuleName, ModulePath, HasReadme, HasScientificSection
    #>
    param([string]$SourceDirectory)

    $results = @()

    if (-not (Test-Path $SourceDirectory)) {
        return $results
    }

    $moduleDirs = @(Get-ChildItem $SourceDirectory -Directory -ErrorAction SilentlyContinue)

    foreach ($dir in $moduleDirs) {
        $readmePath = Join-Path $dir.FullName "README.md"
        $hasReadme = Test-Path $readmePath
        $hasScientificSection = $false

        if ($hasReadme) {
            try {
                $content = Get-Content $readmePath -Raw -Encoding UTF8 -ErrorAction SilentlyContinue
                if ($content) {
                    $hasScientificSection = ($content -match $script:ScientificSectionPattern)
                }
            } catch { }
        }

        # Check if there are source files (has implementation)
        $hasImpl = (Get-ChildItem $dir.FullName -Recurse -ErrorAction SilentlyContinue |
            Where-Object { $_.Extension -in @('.cpp', '.h', '.hpp', '.cc', '.c', '.py') } |
            Measure-Object).Count -gt 0

        if ($hasImpl -and -not $hasScientificSection) {
            $results += [PSCustomObject]@{
                ModuleName         = $dir.Name
                ModulePath         = $dir.FullName
                HasReadme          = $hasReadme
                HasScientificSection = $hasScientificSection
            }
        }
    }

    return $results
}

function Resolve-ResearchMilestoneTitle {
    <#
    .SYNOPSIS
        Resolve the milestone title for a research backlog item based on priority.
    .PARAMETER Priority
        Research priority (P1-Critical, P2-High, P3-Medium, P4-Low)
    .PARAMETER ResearchType
        Type of research (papers, best-practices, architecture)
    .OUTPUTS
        String: milestone title like "research-backlog-2026-03"
    #>
    param(
        [string]$Priority = "P3-Medium",
        [string]$ResearchType = "papers"
    )

    $monthKey = if ($script:PriorityMonthMap.ContainsKey($Priority)) {
        $script:PriorityMonthMap[$Priority]
    } else {
        "2026-06"
    }

    return "research-backlog-$monthKey"
}

function Get-ResearchPriorityLabel {
    param([string]$Priority)
    switch ($Priority) {
        "P1-Critical" { return $script:ResearchLabels.P1 }
        "P2-High"     { return $script:ResearchLabels.P2 }
        "P3-Medium"   { return $script:ResearchLabels.P3 }
        "P4-Low"      { return $script:ResearchLabels.P4 }
        default       { return $script:ResearchLabels.P3 }
    }
}

function Get-ResearchTypeLabel {
    param([string]$ResearchType)
    switch ($ResearchType) {
        "papers"         { return $script:ResearchLabels.Papers }
        "best-practices" { return $script:ResearchLabels.BestPractices }
        "architecture"   { return $script:ResearchLabels.Architecture }
        default          { return $script:ResearchLabels.Papers }
    }
}

function Get-AllResearchItems {
    <#
    .SYNOPSIS
        Discover and parse all research files in the docs/research directory.
    .PARAMETER ResearchDir
        Path to the docs/research directory
    .PARAMETER FilterType
        Filter by type: papers, best-practices, architecture, or all
    .PARAMETER FilterPriority
        Filter by priority: P1-Critical, P2-High, P3-Medium, P4-Low, or all
    .OUTPUTS
        Array of PSCustomObject from Parse-ResearchFrontmatter
    #>
    param(
        [string]$ResearchDir,
        [string]$FilterType = "all",
        [string]$FilterPriority = "all"
    )

    if (-not (Test-Path $ResearchDir)) {
        Write-Host "  Research-Verzeichnis nicht gefunden: $ResearchDir" -ForegroundColor Red
        return @()
    }

    $mdFiles = @(Get-ChildItem $ResearchDir -Recurse -Filter "*.md" -ErrorAction SilentlyContinue |
        Where-Object { $_.Name -ne "README.md" })

    $items = @()
    foreach ($file in $mdFiles) {
        $item = Parse-ResearchFrontmatter -FilePath $file.FullName

        # Apply type filter
        if ($FilterType -ne "all" -and $item.type -ne $FilterType) { continue }

        # Apply priority filter
        if ($FilterPriority -ne "all" -and $item.priority -ne $FilterPriority) { continue }

        $items += $item
    }

    return $items
}

function Get-ResearchItemsForModule {
    <#
    .SYNOPSIS
        Get research items that influence a specific module.
    .PARAMETER ModuleName
        Name of the module
    .PARAMETER AllResearchItems
        Array of all research items
    .OUTPUTS
        Filtered array of research items
    #>
    param(
        [string]$ModuleName,
        [array]$AllResearchItems
    )

    $moduleNorm = $ModuleName.ToLower()
    return @($AllResearchItems | Where-Object {
        ($_.influenced_modules | ForEach-Object { $_.ToLower() }) -contains $moduleNorm -or
        ($_.file_name.ToLower() -like "*$moduleNorm*")
    })
}

# ============================================================================
# SECTION 4: AUDIT FUNCTIONS
# ============================================================================

function Audit-CodebaseForResearchReferences {
    <#
    .SYNOPSIS
        Full audit of the codebase for research references. Displays results.
    #>
    param(
        [string]$SourceDirectory,
        [string]$ResearchDir,
        [string]$AuditPatterns = ""
    )

    Write-Host ""
    Write-Host "  Code-Audit: Undokumentierte Research-Referenzen" -ForegroundColor Cyan
    Write-Host "  =================================================" -ForegroundColor Cyan
    Write-Host ""

    $refs = Get-UndocumentedResearchReferences `
        -SourceDirectory $SourceDirectory `
        -ResearchDir $ResearchDir `
        -AuditPatterns $AuditPatterns

    if ($refs.Count -eq 0) {
        Write-Host "  Keine Research-Referenzen gefunden." -ForegroundColor Green
        return $refs
    }

    $undocumented = @($refs | Where-Object { $_.Status -eq "undocumented" })
    $documented = @($refs | Where-Object { $_.Status -eq "documented" })

    Write-Host "  Gesamt gefunden: $($refs.Count)" -ForegroundColor White
    Write-Host "    Dokumentiert:   $($documented.Count)" -ForegroundColor Green
    Write-Host "    Undokumentiert: $($undocumented.Count)" -ForegroundColor Yellow
    Write-Host ""

    if ($undocumented.Count -gt 0) {
        Write-Host "  Undokumentierte Referenzen:" -ForegroundColor Yellow
        $byModule = $undocumented | Group-Object Module
        foreach ($group in $byModule) {
            Write-Host ""
            Write-Host "    Modul: $($group.Name)" -ForegroundColor Cyan
            foreach ($ref in $group.Group | Select-Object -First 10) {
                Write-Host "      - $($ref.ReferencedPaper)" -ForegroundColor White
                Write-Host "        Datei: $($ref.FilePath):$($ref.LineNumber)" -ForegroundColor DarkGray
            }
            if ($group.Group.Count -gt 10) {
                Write-Host "        ... und $($group.Group.Count - 10) weitere" -ForegroundColor DarkGray
            }
        }
    }

    return $refs
}

function Validate-ResearchMetadata {
    <#
    .SYNOPSIS
        Validate metadata (frontmatter) in research files.
    #>
    param([string]$ResearchDir)

    Write-Host ""
    Write-Host "  Research-Metadaten-Validierung" -ForegroundColor Cyan
    Write-Host "  ================================" -ForegroundColor Cyan
    Write-Host ""

    $items = Get-AllResearchItems -ResearchDir $ResearchDir
    $valid = 0
    $invalid = 0
    $warnings = @()

    foreach ($item in $items) {
        $itemWarnings = @()

        if ([string]::IsNullOrWhiteSpace($item.title)) {
            $itemWarnings += "Kein Titel"
        }
        if ($item.influenced_modules.Count -eq 0) {
            $itemWarnings += "Keine beeinflussten Module angegeben"
        }
        if ($item.status -eq "Unknown") {
            $itemWarnings += "Status unbekannt"
        }

        if ($itemWarnings.Count -gt 0) {
            $invalid++
            $warnings += [PSCustomObject]@{
                File     = [System.IO.Path]::GetFileName($item.file_path)
                Warnings = $itemWarnings
            }
        } else {
            $valid++
        }
    }

    Write-Host "  Valide:   $valid" -ForegroundColor Green
    Write-Host "  Warnungen: $invalid" -ForegroundColor $(if ($invalid -gt 0) { "Yellow" } else { "Green" })
    Write-Host ""

    foreach ($w in $warnings) {
        Write-Host "  $($w.File):" -ForegroundColor Yellow
        foreach ($msg in $w.Warnings) {
            Write-Host "    - $msg" -ForegroundColor DarkYellow
        }
    }

    return @{
        Valid    = $valid
        Invalid  = $invalid
        Warnings = $warnings
    }
}

function Generate-ResearchIndex {
    <#
    .SYNOPSIS
        Generate / regenerate the Implementation-Influence-Index from research files.
    #>
    param(
        [string]$ResearchDir,
        [string]$OutputPath = "",
        [switch]$DryRun
    )

    if ([string]::IsNullOrWhiteSpace($OutputPath)) {
        $OutputPath = Join-Path $ResearchDir "IMPLEMENTATION_INFLUENCE_INDEX.md"
    }

    Write-Host ""
    Write-Host "  Research-Index generieren..." -ForegroundColor Cyan

    $items = Get-AllResearchItems -ResearchDir $ResearchDir
    if ($items.Count -eq 0) {
        Write-Host "  Keine Research-Dateien gefunden." -ForegroundColor Yellow
        return
    }

    # Group by influenced_modules
    $moduleIndex = @{}
    foreach ($item in $items) {
        foreach ($mod in $item.influenced_modules) {
            if (-not $moduleIndex.ContainsKey($mod)) {
                $moduleIndex[$mod] = @()
            }
            $moduleIndex[$mod] += $item
        }
    }

    $now = Get-Date -Format "yyyy-MM-dd'T'HH:mm:ssZ"
    $indexLines = @()
    $indexLines += "# Implementation Influence Index"
    $indexLines += ""
    $indexLines += "> Auto-generated by research_github_master.ps1 on $now"
    $indexLines += ""
    $indexLines += "## Summary"
    $indexLines += ""
    $indexLines += "| Module | Papers | Best Practices | Architecture |"
    $indexLines += "|--------|--------|---------------|--------------|"

    $sortedModules = @($moduleIndex.Keys | Sort-Object)
    foreach ($mod in $sortedModules) {
        $papers = ($moduleIndex[$mod] | Where-Object { $_.type -eq "papers" }).Count
        $bp     = ($moduleIndex[$mod] | Where-Object { $_.type -eq "best-practices" }).Count
        $arch   = ($moduleIndex[$mod] | Where-Object { $_.type -eq "architecture" }).Count
        $indexLines += "| $mod | $papers | $bp | $arch |"
    }

    $indexLines += ""
    $indexLines += "## Module Details"
    $indexLines += ""

    foreach ($mod in $sortedModules) {
        $indexLines += "### $mod"
        $indexLines += ""
        foreach ($item in ($moduleIndex[$mod] | Sort-Object priority)) {
            $prio = $item.priority
            $status = $item.status
            $title = $item.title
            $relPath = $item.file_path
            if ($relPath.StartsWith($ResearchDir)) {
                $relPath = $relPath.Substring($ResearchDir.Length).TrimStart('/\')
            }
            $indexLines += "- **[$prio]** $title"
            $indexLines += "  - Status: $status | Type: $($item.type)"
            $indexLines += "  - File: [$relPath]($relPath)"
            $indexLines += ""
        }
    }

    $indexContent = $indexLines -join "`n"

    if ($DryRun) {
        Write-Host "  [DRY RUN] Wuerde Index schreiben nach: $OutputPath" -ForegroundColor Yellow
        Write-Host "  [DRY RUN] $($items.Count) Research-Items, $($moduleIndex.Count) Module" -ForegroundColor Yellow
        return
    }

    [System.IO.File]::WriteAllText($OutputPath, $indexContent, [System.Text.Encoding]::UTF8)
    Write-Host "  Index gespeichert: $OutputPath" -ForegroundColor Green
    Write-Host "  Research-Items: $($items.Count) | Module: $($moduleIndex.Count)" -ForegroundColor Gray
}

# ============================================================================
# SECTION 5: GITHUB FUNCTIONS (adapted)
# ============================================================================

function Test-GitHubCLI {
    try {
        $null = gh --version 2>&1
        return $LASTEXITCODE -eq 0
    } catch {
        return $false
    }
}

function Test-GitHubAuth {
    try {
        $null = gh auth status 2>&1
        return $LASTEXITCODE -eq 0
    } catch {
        return $false
    }
}

function Get-ExistingLabels {
    param([string]$Repo)
    try {
        $labelsJson = gh label list --repo $Repo --limit 1000 --json name 2>&1
        if ($LASTEXITCODE -ne 0) { return @() }
        $labels = $labelsJson | ConvertFrom-Json
        return @($labels | ForEach-Object { $_.name })
    } catch {
        return @()
    }
}

function Get-LabelColor {
    param([string]$LabelName)
    if ($LabelName -like "p1-*" -or $LabelName -like "p2-*") { return "d93f0b" }
    if ($LabelName -like "p3-*") { return "e4e669" }
    if ($LabelName -like "p4-*") { return "c2e0c6" }
    if ($LabelName -like "research*") { return "1d76db" }
    $hash = [Math]::Abs($LabelName.GetHashCode())
    $palette = @("1d76db", "0052cc", "5319e7", "0e8a16", "006b75", "fbca04")
    return $palette[$hash % $palette.Count]
}

function Ensure-LabelsExist {
    param(
        [string]$Repo,
        [array]$Labels,
        [switch]$DryRun
    )
    $cleanLabels = @($Labels | Where-Object { -not [string]::IsNullOrWhiteSpace($_) } | Select-Object -Unique)
    if ($cleanLabels.Count -eq 0) { return $true }

    Write-Host "  Pruefe Labels..." -NoNewline -ForegroundColor Cyan
    $existing = Get-ExistingLabels -Repo $Repo
    $existingSet = @{}
    foreach ($name in $existing) { $existingSet[$name.ToLower()] = $true }

    $missing = @($cleanLabels | Where-Object { -not $existingSet.ContainsKey($_.ToLower()) })

    if ($missing.Count -eq 0) {
        Write-Host " OK" -ForegroundColor Green
        return $true
    }

    Write-Host " $($missing.Count) fehlen" -ForegroundColor Yellow

    foreach ($label in $missing) {
        if ($DryRun) {
            Write-Host "    [DRY RUN] Wuerde Label erstellen: $label" -ForegroundColor Yellow
            continue
        }
        $color = Get-LabelColor -LabelName $label
        $result = gh label create "$label" --repo $Repo --color $color --description "Auto-created by research backlog manager" 2>&1
        if ($LASTEXITCODE -eq 0) {
            Write-Host "    Label erstellt: $label" -ForegroundColor Green
        } else {
            if ("$result" -match "already exists") {
                Write-Host "    Label existiert bereits: $label" -ForegroundColor Gray
            } else {
                Write-Host "    Label konnte nicht erstellt werden: $label - $result" -ForegroundColor Red
                return $false
            }
        }
    }
    return $true
}

function Get-OrCreateMilestoneNumber {
    param(
        [string]$Repo,
        [string]$MilestoneTitle,
        [switch]$DryRun
    )
    $normalizedTitle = Normalize-MilestoneTitle -Title $MilestoneTitle
    if ([string]::IsNullOrWhiteSpace($normalizedTitle)) { return 0 }

    $cacheKey = "$Repo|$($normalizedTitle.ToLowerInvariant())"
    if ($script:MilestoneCache.ContainsKey($cacheKey)) {
        return [int]$script:MilestoneCache[$cacheKey]
    }

    try {
        $existingJson = gh api "repos/$Repo/milestones?state=all&per_page=100" --paginate --jq '.[] | "\(.number)|\(.title)"' 2>&1
        if ($LASTEXITCODE -eq 0) {
            $lines = @($existingJson -split "`r?`n" | Where-Object { -not [string]::IsNullOrWhiteSpace($_) })
            foreach ($line in $lines) {
                $parts = $line -split "\|", 2
                if ($parts.Count -lt 2) { continue }
                $numberText = $parts[0].Trim()
                $existingTitle = $parts[1].Trim()
                if ($existingTitle -ieq $normalizedTitle -or $existingTitle -ieq $MilestoneTitle) {
                    $number = 0
                    if ([int]::TryParse($numberText, [ref]$number) -and $number -gt 0) {
                        $script:MilestoneCache[$cacheKey] = $number
                        return $number
                    }
                }
            }
        }

        if ($DryRun) {
            Write-Host "    [DRY RUN] Wuerde Milestone erstellen: $MilestoneTitle" -ForegroundColor Yellow
            return 0
        }

        $createJson = gh api "repos/$Repo/milestones" -X POST -f "title=$MilestoneTitle" -f "state=open" 2>&1
        if ($LASTEXITCODE -eq 0) {
            $created = $createJson | ConvertFrom-Json
            $number = [int]$created.number
            $script:MilestoneCache[$cacheKey] = $number
            Write-Host "    Milestone erstellt: $MilestoneTitle (#$number)" -ForegroundColor Gray
            return $number
        }
        Write-Host "    Milestone konnte nicht erstellt werden: $MilestoneTitle" -ForegroundColor Yellow
        return 0
    } catch {
        Write-Host "    Milestone-Fehler: $_" -ForegroundColor Yellow
        return 0
    }
}

function Get-ExistingResearchIssues {
    param([string]$Repo)

    Write-Host "  Pruefe vorhandene Research-Issues..." -NoNewline

    try {
        $issuesJson = gh issue list `
            --repo $Repo `
            --state all `
            --label "research-backlog" `
            --limit 500 `
            --json number,title,state,labels,url 2>&1

        if ($LASTEXITCODE -ne 0) {
            Write-Host " FEHLER" -ForegroundColor Red
            return @()
        }

        $issues = @($issuesJson | ConvertFrom-Json)
        Write-Host " $($issues.Count) gefunden" -ForegroundColor Green
        return $issues
    } catch {
        Write-Host " EXCEPTION: $_" -ForegroundColor Red
        return @()
    }
}

function Test-ResearchIssueExists {
    param(
        [string]$IssueTitle,
        [array]$ExistingIssues
    )

    $normalizeTitle = {
        param([string]$t)
        ($t.ToLower() -replace '[^a-z0-9\s]', ' ' -replace '\s+', ' ').Trim()
    }

    $searchNorm = & $normalizeTitle $IssueTitle

    foreach ($issue in $ExistingIssues) {
        $issueNorm = & $normalizeTitle $issue.title
        if ($issueNorm -eq $searchNorm) {
            return @{ Exists = $true; Issue = $issue }
        }
        if ($issueNorm.Contains($searchNorm) -or $searchNorm.Contains($issueNorm)) {
            $shortLen = [Math]::Min($searchNorm.Length, $issueNorm.Length)
            $longLen = [Math]::Max($searchNorm.Length, $issueNorm.Length)
            if ($longLen -gt 0 -and ([double]$shortLen / [double]$longLen) -ge 0.85) {
                return @{ Exists = $true; Issue = $issue }
            }
        }
    }
    return @{ Exists = $false; Issue = $null }
}

function New-GitHubIssue {
    param(
        [string]$Repo,
        [string]$Title,
        [string]$Body,
        [array]$Labels = @(),
        [int]$MilestoneNumber = 0
    )
    try {
        $cleanLabels = @($Labels | Where-Object { -not [string]::IsNullOrWhiteSpace($_) } | Select-Object -Unique)
        $payload = @{ title = $Title; body = $Body }
        if ($cleanLabels.Count -gt 0) { $payload.labels = $cleanLabels }
        if ($MilestoneNumber -gt 0) { $payload.milestone = $MilestoneNumber }
        $json = $payload | ConvertTo-Json -Depth 5

        Write-Host "    Erstelle Issue..." -NoNewline -ForegroundColor Cyan
        $result = $json | gh api "repos/$Repo/issues" -X POST --input - 2>&1

        if ($LASTEXITCODE -eq 0) {
            Write-Host " ERFOLG!" -ForegroundColor Green
            $issueNumber = 0
            $issueUrl = ""
            try {
                $issueData = $result | ConvertFrom-Json
                $issueNumber = [int]$issueData.number
                $issueUrl = "$($issueData.html_url)"
                Write-Host "    Issue #$issueNumber erstellt: $issueUrl" -ForegroundColor Gray
            } catch { }
            return @{ Success = $true; Number = $issueNumber; Url = $issueUrl; Error = ""; IsRateLimited = $false }
        } else {
            $errorText = "$result"
            Write-Host " FEHLER!" -ForegroundColor Red
            Write-Host "    GitHub API Error: $errorText" -ForegroundColor Red
            return @{
                Success = $false; Number = 0; Url = ""; Error = $errorText
                IsRateLimited = ($errorText -match 'secondary rate limit' -or $errorText -match 'HTTP 403')
            }
        }
    } catch {
        Write-Host " EXCEPTION!" -ForegroundColor Red
        return @{
            Success = $false; Number = 0; Url = ""; Error = "$_"
            IsRateLimited = ("$_" -match 'secondary rate limit' -or "$_" -match 'HTTP 403')
        }
    }
}

# ============================================================================
# SECTION 6: OLLAMA INTEGRATION
# ============================================================================

function Test-OllamaAvailable {
    try {
        $null = Invoke-RestMethod -Uri "$script:OllamaUrl/api/tags" -Method Get -TimeoutSec 3 -ErrorAction Stop
        return $true
    } catch {
        return $false
    }
}

function Invoke-OllamaChat {
    param(
        [string]$SystemPrompt,
        [string]$UserPrompt,
        [string]$Model = $script:OllamaModel,
        [double]$Temperature = 0.7,
        [int]$NumPredict = 500
    )
    try {
        $body = @{
            model    = $Model
            messages = @(
                @{ role = "system"; content = $SystemPrompt },
                @{ role = "user"; content = $UserPrompt }
            )
            stream  = $false
            options = @{ temperature = $Temperature; num_predict = $NumPredict }
        } | ConvertTo-Json -Depth 10

        $response = Invoke-RestMethod `
            -Uri "$script:OllamaUrl/api/chat" `
            -Method Post `
            -Body $body `
            -ContentType "application/json" `
            -TimeoutSec 60 `
            -ErrorAction Stop

        return $response.message.content
    } catch {
        Write-Host "    Ollama Fehler: $_" -ForegroundColor Red
        return $null
    }
}

function Optimize-ResearchIssueTitle {
    param(
        [string]$OriginalTitle,
        [string]$ModuleName,
        [string]$ResearchType
    )

    if (-not $script:UseAI) { return $OriginalTitle }

    $systemPrompt = @"
You are a GitHub issue title optimizer for research backlog items.
Rules:
1) Return EXACTLY ONE LINE with ONLY the title text.
2) Never include prefixes like "Title:", quotes, or markdown.
3) Imperative mood, specific, actionable, <= 80 characters.
4) Preserve technical meaning.
5) Do NOT include module prefix like [module] in your output.

Example output: Document HNSW research influence on vector index implementation
"@

    $userPrompt = "Module: $ModuleName`nResearch type: $ResearchType`nOriginal: $OriginalTitle`n`nOptimize this research backlog issue title."

    Write-Host "    AI optimiert Titel..." -NoNewline -ForegroundColor Cyan
    $optimized = Invoke-OllamaChat -SystemPrompt $systemPrompt -UserPrompt $userPrompt -Temperature 0.2 -NumPredict 80

    if ($optimized) {
        $optimized = ($optimized.Trim() -replace '\s+', ' ')
        Write-Host " OK" -ForegroundColor Green
        return $optimized
    }

    Write-Host " FEHLER (Original verwendet)" -ForegroundColor Yellow
    return $OriginalTitle
}

# ============================================================================
# SECTION 7: UI FUNCTIONS
# ============================================================================

function Show-Banner {
    param([string]$Title = "Research Backlog Manager")
    Clear-Host
    Write-Host ""
    Write-Host "  =========================================================" -ForegroundColor Cyan
    Write-Host "  $Title" -ForegroundColor Cyan
    Write-Host "  =========================================================" -ForegroundColor Cyan
    Write-Host ""
}

function Wait-Continue {
    param([string]$Message = "Druecken Sie Enter um fortzufahren...")
    if ($script:IsNonInteractive) { return }
    Write-Host ""
    Write-Host "  $Message" -ForegroundColor Gray
    $null = Read-Host
}

function Get-UserChoice {
    param(
        [string]$Prompt = "Ihre Wahl",
        [array]$Options,
        [int]$Default = 1
    )
    Write-Host ""
    Write-Host "  $Prompt" -ForegroundColor Yellow
    Write-Host ""
    for ($i = 0; $i -lt $Options.Count; $i++) {
        $marker = if (($i + 1) -eq $Default) { ">" } else { " " }
        Write-Host "    $marker [$($i + 1)] $($Options[$i])" -ForegroundColor White
    }
    Write-Host ""
    Write-Host "  Ihre Wahl [1-$($Options.Count)]: " -NoNewline -ForegroundColor Yellow
    $input = Read-Host
    if ([string]::IsNullOrWhiteSpace($input)) { return $Default }
    $choice = 0
    if ([int]::TryParse($input, [ref]$choice) -and $choice -ge 1 -and $choice -le $Options.Count) {
        return $choice
    }
    return $Default
}

# ============================================================================
# SECTION 8: ISSUE CREATION - RESEARCH BACKLOG
# ============================================================================

function New-ResearchBacklogIssue {
    <#
    .SYNOPSIS
        Create a GitHub issue for a research backlog item.
    #>
    param(
        [string]$Repo,
        [PSObject]$ResearchItem,
        [string[]]$AffectedModules,
        [string]$IssuePriority,
        [string]$Milestone,
        [switch]$DryRun
    )

    $typeLabel  = Get-ResearchTypeLabel -ResearchType $ResearchItem.type
    $prioLabel  = Get-ResearchPriorityLabel -Priority $IssuePriority
    $labels     = @($script:ResearchLabels.Base, $typeLabel, $prioLabel) +
                  @($AffectedModules | Where-Object { $_ } | ForEach-Object { $_.ToLower() })

    $milestoneNumber = 0
    if (-not [string]::IsNullOrWhiteSpace($Milestone)) {
        $milestoneNumber = Get-OrCreateMilestoneNumber -Repo $Repo -MilestoneTitle $Milestone -DryRun:$DryRun
    }

    $sourceLines = Expand-ResearchSourceLines -ResearchItem $ResearchItem -AffectedModules $AffectedModules
    $sourceBlock = $sourceLines -join "`n"

    $modulesBlock = if ($AffectedModules.Count -gt 0) {
        "## Betroffene Module`n" + ($AffectedModules | ForEach-Object { "- $_" } | Join-String -Separator "`n")
    } else { "" }

    $checklistBlock = @"
## Dokumentations-Checkliste
- [ ] Abstract / Zusammenfassung der Research-Quelle
- [ ] Relevanz fuer ThemisDB erklaert
- [ ] Spezifische Anpassungen fuer ThemisDB dokumentiert
- [ ] Links zu anderen relevanten Forschungen hinzugefuegt
- [ ] Betroffene Module in Research-Frontmatter aktualisiert
- [ ] Implementation-Influence-Index regeneriert
"@

    $title = if ($script:UseAI -and (Test-OllamaAvailable)) {
        Optimize-ResearchIssueTitle `
            -OriginalTitle "Document research influence: $($ResearchItem.title)" `
            -ModuleName ($AffectedModules | Select-Object -First 1) `
            -ResearchType $ResearchItem.type
    } else {
        "Document research influence: $($ResearchItem.title)"
    }

    $issueTitle = "[research-backlog] $title"

    $body = @"
# Research Backlog: $($ResearchItem.title)

## Quelle
$sourceBlock

$modulesBlock

## Was muss dokumentiert werden?
- Wie beeinflusst diese Forschung die Implementierung in ThemisDB?
- Welche Algorithmen / Konzepte wurden uebernommen oder angepasst?
- Gibt es Performance-Benchmarks oder spezifische Metriken?
- Wie unterscheidet sich die ThemisDB-Implementierung von der Forschung?

$checklistBlock

## Prioritaet & Zeitplan
- **Prioritaet:** $IssuePriority
- **Milestone:** $Milestone
- **Research-Typ:** $($ResearchItem.type)

## Metadata
- Generated by: research_github_master.ps1
- Research file: $($ResearchItem.file_path)
- Status: $($ResearchItem.status)
$(if ($script:UseAI) { "- AI Model: $script:OllamaModel" } else { "" })
"@

    $body = [regex]::Replace($body, '(?m)\n{3,}', "`n`n")

    $labelsReady = Ensure-LabelsExist -Repo $Repo -Labels $labels -DryRun:$DryRun
    if (-not $labelsReady) {
        Write-Host "  Labels konnten nicht vorbereitet werden." -ForegroundColor Red
        return $null
    }

    if ($DryRun) {
        Write-Host "    [DRY RUN] Issue: $issueTitle" -ForegroundColor Yellow
        Write-Host "    [DRY RUN] Labels: $($labels -join ', ')" -ForegroundColor DarkYellow
        if ($milestoneNumber -gt 0) {
            Write-Host "    [DRY RUN] Milestone #$($milestoneNumber): $Milestone" -ForegroundColor DarkYellow
        }
        return @{ Success = $true; Number = 0; Url = ""; DryRun = $true }
    }

    return New-GitHubIssue `
        -Repo $Repo `
        -Title $issueTitle `
        -Body $body `
        -Labels $labels `
        -MilestoneNumber $milestoneNumber
}

function New-UndocumentedReferenceIssue {
    <#
    .SYNOPSIS
        Create a GitHub issue for undocumented code references found in audit.
    #>
    param(
        [string]$Repo,
        [array]$References,
        [string]$ModuleName,
        [switch]$DryRun
    )

    if ($References.Count -eq 0) { return $null }

    $labels = @($script:ResearchLabels.Base, $script:ResearchLabels.Undocumented, $ModuleName.ToLower(), $script:ResearchLabels.P3)
    $milestoneTitle = Resolve-ResearchMilestoneTitle -Priority "P3-Medium"
    $milestoneNumber = Get-OrCreateMilestoneNumber -Repo $Repo -MilestoneTitle $milestoneTitle -DryRun:$DryRun

    $refLines = @($References | ForEach-Object {
        "- ``$($_.ReferencedPaper)`` in ``$($_.FilePath):$($_.LineNumber)``"
    })

    $issueTitle = "[research-backlog] Document undocumented research references in module: $ModuleName"

    $body = @"
# Undokumentierte Research-Referenzen: $ModuleName

## Zusammenfassung
Im Modul **$ModuleName** wurden $($References.Count) Research-Referenzen im Code gefunden,
die keine entsprechende Dokumentation in ``docs/research/`` haben.

## Gefundene Referenzen
$($refLines -join "`n")

## Was muss getan werden?
- [ ] Jede Referenz in ``docs/research/`` dokumentieren
- [ ] Frontmatter mit ``influenced_modules`` aktualisieren
- [ ] Implementation-Influence-Index regenerieren
- [ ] Research-Zusammenfassung im Modul-README ergaenzen

## Prioritaet & Zeitplan
- **Prioritaet:** P3-Medium
- **Milestone:** $milestoneTitle

## Metadata
- Generated by: research_github_master.ps1 (Code-Audit)
- Module: $ModuleName
- Undocumented references found: $($References.Count)
"@

    $labelsReady = Ensure-LabelsExist -Repo $Repo -Labels $labels -DryRun:$DryRun
    if (-not $labelsReady) { return $null }

    if ($DryRun) {
        Write-Host "    [DRY RUN] Issue: $issueTitle" -ForegroundColor Yellow
        return @{ Success = $true; Number = 0; Url = ""; DryRun = $true }
    }

    return New-GitHubIssue `
        -Repo $Repo `
        -Title $issueTitle `
        -Body $body `
        -Labels $labels `
        -MilestoneNumber $milestoneNumber
}

# ============================================================================
# SECTION 9: ACTION HANDLERS
# ============================================================================

function Action-Stats {
    Show-Banner "Research Coverage Dashboard"

    $allItems = Get-AllResearchItems -ResearchDir $script:ResearchDir

    Write-Host "  Gesamt:" -ForegroundColor Cyan
    Write-Host "    Research-Dokumente gesamt:  $($allItems.Count)" -ForegroundColor White
    $papers = @($allItems | Where-Object { $_.type -eq "papers" })
    $bp     = @($allItems | Where-Object { $_.type -eq "best-practices" })
    $arch   = @($allItems | Where-Object { $_.type -eq "architecture" })
    Write-Host "    Papers:                     $($papers.Count)" -ForegroundColor White
    Write-Host "    Best Practices:             $($bp.Count)" -ForegroundColor White
    Write-Host "    Architecture:               $($arch.Count)" -ForegroundColor White
    Write-Host ""

    # Per priority
    Write-Host "  Nach Prioritaet:" -ForegroundColor Cyan
    foreach ($prio in @("P1-Critical", "P2-High", "P3-Medium", "P4-Low")) {
        $prioItems = @($allItems | Where-Object { $_.priority -eq $prio })
        if ($prioItems.Count -gt 0) {
            $month = if ($script:PriorityMonthMap.ContainsKey($prio)) { $script:PriorityMonthMap[$prio] } else { "-" }
            Write-Host "    $prio ($month):" -ForegroundColor Yellow
            $moduleGroups = $prioItems | Group-Object { $_.influenced_modules -join ',' }
            foreach ($item in ($prioItems | Select-Object -First 8)) {
                Write-Host "      - $($item.title) [$($item.type)]" -ForegroundColor Gray
            }
            if ($prioItems.Count -gt 8) {
                Write-Host "      ... und $($prioItems.Count - 8) weitere" -ForegroundColor DarkGray
            }
        }
    }
    Write-Host ""

    # Per module
    $moduleIndex = @{}
    foreach ($item in $allItems) {
        foreach ($mod in $item.influenced_modules) {
            if (-not $moduleIndex.ContainsKey($mod)) { $moduleIndex[$mod] = 0 }
            $moduleIndex[$mod]++
        }
    }

    if ($moduleIndex.Count -gt 0) {
        Write-Host "  Module mit Research-Links:" -ForegroundColor Cyan
        foreach ($mod in ($moduleIndex.Keys | Sort-Object)) {
            Write-Host "    $($mod.PadRight(20)) $($moduleIndex[$mod]) Quellen" -ForegroundColor White
        }
    }

    Write-Host ""
    Write-Host "  ========================================" -ForegroundColor Cyan

    # Missing links if SourceDir is available
    if ($script:SourceDir -and (Test-Path $script:SourceDir)) {
        $missing = Get-MissingModuleInfluenceLinks -SourceDirectory $script:SourceDir
        if ($missing.Count -gt 0) {
            Write-Host "  Module OHNE Wissenschaftliche-Grundlagen-Sektion: $($missing.Count)" -ForegroundColor Yellow
            foreach ($m in $missing | Select-Object -First 10) {
                Write-Host "    - $($m.ModuleName)" -ForegroundColor DarkYellow
            }
        } else {
            Write-Host "  Alle Module haben Wissenschaftliche-Grundlagen-Sektion" -ForegroundColor Green
        }
        Write-Host ""
    }

    Wait-Continue
}

function Action-AuditCodebase {
    Show-Banner "Code-Audit: Undokumentierte Research-Referenzen"

    if (-not $script:SourceDir -or -not (Test-Path $script:SourceDir)) {
        Write-Host "  Source-Verzeichnis nicht gefunden. Bitte -SourcePath angeben." -ForegroundColor Red
        Wait-Continue
        return
    }

    $refs = Audit-CodebaseForResearchReferences `
        -SourceDirectory $script:SourceDir `
        -ResearchDir $script:ResearchDir `
        -AuditPatterns $CodeAuditPatterns

    $undocumented = @($refs | Where-Object { $_.Status -eq "undocumented" })
    if ($undocumented.Count -eq 0) {
        Write-Host ""
        Write-Host "  Keine undokumentierten Referenzen gefunden." -ForegroundColor Green
        Wait-Continue
        return
    }

    Write-Host ""
    Write-Host "  $($undocumented.Count) undokumentierte Referenzen gefunden." -ForegroundColor Yellow
    Write-Host ""

    if ($DryRun) {
        Write-Host "  [DRY RUN] Wuerde Issues erstellen fuer:" -ForegroundColor Yellow
        $byModule = $undocumented | Group-Object Module
        foreach ($g in $byModule) {
            Write-Host "    - Modul '$($g.Name)': $($g.Group.Count) Referenzen" -ForegroundColor DarkYellow
        }
        Wait-Continue
        return
    }

    if (-not $Yes) {
        $choice = Get-UserChoice -Prompt "Issues erstellen fuer undokumentierte Referenzen?" -Options @("Ja", "Nein") -Default 2
        if ($choice -ne 1) {
            Write-Host "  Abgebrochen." -ForegroundColor Yellow
            Wait-Continue
            return
        }
    }

    $byModule = $undocumented | Group-Object Module
    $created = 0
    foreach ($group in $byModule) {
        $result = New-UndocumentedReferenceIssue `
            -Repo $Repository `
            -References @($group.Group) `
            -ModuleName $group.Name `
            -DryRun:$DryRun

        if ($result -and $result.Success) { $created++ }

        if ($DelayBetweenIssuesSec -gt 0) {
            Start-Sleep -Seconds $DelayBetweenIssuesSec
        }
    }

    Write-Host ""
    Write-Host "  Issues erstellt: $created / $($byModule.Count)" -ForegroundColor $(if ($created -eq $byModule.Count) { "Green" } else { "Yellow" })
    Wait-Continue
}

function Action-CreateModuleIssues {
    param([string]$ModuleNameParam)

    Show-Banner "Research-Issues fuer Modul: $ModuleNameParam"

    $allItems = Get-AllResearchItems -ResearchDir $script:ResearchDir -FilterPriority $Priority -FilterType $ResearchType
    $moduleItems = Get-ResearchItemsForModule -ModuleName $ModuleNameParam -AllResearchItems $allItems

    if ($moduleItems.Count -lt $ResearchSourcesMinimum) {
        Write-Host "  Modul '$ModuleNameParam' hat $($moduleItems.Count) Research-Quellen (Minimum: $ResearchSourcesMinimum)" -ForegroundColor Yellow
        Wait-Continue
        return
    }

    Write-Host "  Gefunden: $($moduleItems.Count) Research-Items fuer Modul '$ModuleNameParam'" -ForegroundColor Cyan
    Write-Host ""

    $existingIssues = Get-ExistingResearchIssues -Repo $Repository
    Write-Host ""

    $itemsToCreate = @()
    foreach ($item in $moduleItems) {
        $checkTitle = "Document research influence: $($item.title)"
        $check = Test-ResearchIssueExists -IssueTitle $checkTitle -ExistingIssues $existingIssues
        if ($check.Exists) {
            Write-Host "  Ueberspringe (existiert bereits): $($item.title)" -ForegroundColor Gray
            continue
        }
        $itemsToCreate += $item
    }

    if ($itemsToCreate.Count -eq 0) {
        Write-Host "  Alle Issues existieren bereits." -ForegroundColor Green
        Wait-Continue
        return
    }

    Write-Host "  Zu erstellen: $($itemsToCreate.Count) Issues" -ForegroundColor Cyan
    if ($DryRun) {
        Write-Host "  DRY RUN MODE - Keine echten Issues werden erstellt" -ForegroundColor Yellow
    }

    if (-not $Yes) {
        $choice = Get-UserChoice -Prompt "Fortfahren?" -Options @("Ja", "Nein") -Default 1
        if ($choice -ne 1) {
            Write-Host "  Abgebrochen." -ForegroundColor Yellow
            Wait-Continue
            return
        }
    }

    $created = 0
    $failed = 0
    $current = 0

    foreach ($item in $itemsToCreate) {
        if ($current -gt 0 -and $DelayBetweenIssuesSec -gt 0) {
            Start-Sleep -Seconds $DelayBetweenIssuesSec
        }
        $current++

        Write-Host "  [$current/$($itemsToCreate.Count)] $($item.title)" -ForegroundColor White

        $issuePriority = if ($item.priority) { $item.priority } else { $Priority }
        if ($issuePriority -eq "all") { $issuePriority = "P3-Medium" }

        $milestoneTitle = Resolve-ResearchMilestoneTitle -Priority $issuePriority -ResearchType $item.type
        $affectedModules = @($item.influenced_modules) + @($ModuleNameParam) | Select-Object -Unique

        $maxAttempts = [Math]::Max(1, $RateLimitMaxRetries + 1)
        $attempt = 0
        $result = $null

        do {
            $attempt++
            $result = New-ResearchBacklogIssue `
                -Repo $Repository `
                -ResearchItem $item `
                -AffectedModules $affectedModules `
                -IssuePriority $issuePriority `
                -Milestone $milestoneTitle `
                -DryRun:$DryRun

            if ($null -eq $result -or $result.Success) { break }

            $isRateLimited = ($result.ContainsKey("IsRateLimited") -and $result.IsRateLimited)
            if (-not $isRateLimited) { break }

            if ($attempt -lt $maxAttempts) {
                $waitSec = ([Math]::Max(1, $RateLimitCooldownSec) * $attempt)
                if ($RateLimitJitterSec -gt 0) { $waitSec += (Get-Random -Minimum 0 -Maximum ($RateLimitJitterSec + 1)) }
                Write-Host "    Rate-Limit. Retry $attempt/$maxAttempts nach $waitSec Sekunden..." -ForegroundColor Yellow
                Start-Sleep -Seconds $waitSec
            }
        } while ($attempt -lt $maxAttempts)

        if ($result -and $result.Success) {
            $created++
        } else {
            $failed++
            if ($result -and $result.ContainsKey("IsRateLimited") -and $result.IsRateLimited) {
                Write-Host "    Abbruch: GitHub Secondary Rate Limit." -ForegroundColor Yellow
                break
            }
        }
        Write-Host ""
    }

    Write-Host "  =============================================================" -ForegroundColor Green
    Write-Host "  Erstellt: $created" -ForegroundColor Green
    if ($failed -gt 0) {
        Write-Host "  Fehlgeschlagen: $failed" -ForegroundColor Red
    }
    Write-Host "  =============================================================" -ForegroundColor Green

    if ($GenerateIndexAfterCreation -and $created -gt 0 -and -not $DryRun) {
        Write-Host ""
        Write-Host "  Regeneriere Research-Index..." -ForegroundColor Cyan
        Generate-ResearchIndex -ResearchDir $script:ResearchDir -DryRun:$DryRun
    }

    Wait-Continue
}

function Action-CreateAllIssues {
    Show-Banner "Alle Research-Issues erstellen"

    $allItems = Get-AllResearchItems -ResearchDir $script:ResearchDir -FilterPriority $Priority -FilterType $ResearchType

    if ($allItems.Count -eq 0) {
        Write-Host "  Keine Research-Items gefunden (Priority=$Priority, Type=$ResearchType)" -ForegroundColor Yellow
        Wait-Continue
        return
    }

    Write-Host "  Gefunden: $($allItems.Count) Research-Items" -ForegroundColor Cyan
    Write-Host ""

    $existingIssues = Get-ExistingResearchIssues -Repo $Repository
    Write-Host ""

    $itemsToCreate = @()
    foreach ($item in $allItems) {
        $checkTitle = "Document research influence: $($item.title)"
        $check = Test-ResearchIssueExists -IssueTitle $checkTitle -ExistingIssues $existingIssues
        if (-not $check.Exists) {
            $itemsToCreate += $item
        }
    }

    Write-Host "  Zu erstellen: $($itemsToCreate.Count) Issues (schon vorhanden: $($allItems.Count - $itemsToCreate.Count))" -ForegroundColor Cyan
    if ($DryRun) {
        Write-Host "  DRY RUN MODE" -ForegroundColor Yellow
    }
    Write-Host ""

    if (-not $Yes) {
        $choice = Get-UserChoice -Prompt "Alle Issues erstellen?" -Options @("Ja", "Nein") -Default 1
        if ($choice -ne 1) {
            Write-Host "  Abgebrochen." -ForegroundColor Yellow
            Wait-Continue
            return
        }
    }

    $created = 0
    $failed = 0
    $current = 0
    $prevModule = ""

    foreach ($item in $itemsToCreate) {
        if ($current -gt 0 -and $DelayBetweenIssuesSec -gt 0) {
            Start-Sleep -Seconds $DelayBetweenIssuesSec
        }

        $currentModule = ($item.influenced_modules | Select-Object -First 1)
        if ($currentModule -ne $prevModule -and $prevModule -ne "" -and $DelayBetweenModulesSec -gt 0) {
            Write-Host "  Warte $DelayBetweenModulesSec Sekunden vor naechstem Modul..." -ForegroundColor DarkGray
            Start-Sleep -Seconds $DelayBetweenModulesSec
        }
        $prevModule = $currentModule

        $current++
        Write-Host "  [$current/$($itemsToCreate.Count)] $($item.title)" -ForegroundColor White

        $issuePriority = if ($item.priority -and $item.priority -ne "all") { $item.priority } else { "P3-Medium" }
        $milestoneTitle = Resolve-ResearchMilestoneTitle -Priority $issuePriority -ResearchType $item.type

        $maxAttempts = [Math]::Max(1, $RateLimitMaxRetries + 1)
        $attempt = 0
        $result = $null

        do {
            $attempt++
            $result = New-ResearchBacklogIssue `
                -Repo $Repository `
                -ResearchItem $item `
                -AffectedModules @($item.influenced_modules) `
                -IssuePriority $issuePriority `
                -Milestone $milestoneTitle `
                -DryRun:$DryRun

            if ($null -eq $result -or $result.Success) { break }

            $isRateLimited = ($result.ContainsKey("IsRateLimited") -and $result.IsRateLimited)
            if (-not $isRateLimited) { break }

            if ($attempt -lt $maxAttempts) {
                $waitSec = ([Math]::Max(1, $RateLimitCooldownSec) * $attempt)
                if ($RateLimitJitterSec -gt 0) { $waitSec += (Get-Random -Minimum 0 -Maximum ($RateLimitJitterSec + 1)) }
                Write-Host "    Rate-Limit. Retry $attempt/$maxAttempts nach $waitSec Sekunden..." -ForegroundColor Yellow
                Start-Sleep -Seconds $waitSec
            }
        } while ($attempt -lt $maxAttempts)

        if ($result -and $result.Success) {
            $created++
        } else {
            $failed++
            if ($result -and $result.ContainsKey("IsRateLimited") -and $result.IsRateLimited) {
                Write-Host "    Abbruch: GitHub Secondary Rate Limit." -ForegroundColor Yellow
                break
            }
        }
        Write-Host ""
    }

    Write-Host "  =============================================================" -ForegroundColor Green
    Write-Host "  Erstellt: $created" -ForegroundColor Green
    if ($failed -gt 0) {
        Write-Host "  Fehlgeschlagen: $failed" -ForegroundColor Red
    }
    Write-Host "  =============================================================" -ForegroundColor Green

    if ($GenerateIndexAfterCreation -and $created -gt 0 -and -not $DryRun) {
        Write-Host ""
        Write-Host "  Regeneriere Research-Index..." -ForegroundColor Cyan
        Generate-ResearchIndex -ResearchDir $script:ResearchDir -DryRun:$DryRun
    }

    Wait-Continue
}

function Action-GenerateResearchIndex {
    Show-Banner "Research-Index generieren"

    if (-not $Yes -and -not $script:IsNonInteractive) {
        $choice = Get-UserChoice -Prompt "Implementation-Influence-Index regenerieren?" -Options @("Ja", "Nein") -Default 1
        if ($choice -ne 1) {
            Write-Host "  Abgebrochen." -ForegroundColor Yellow
            Wait-Continue
            return
        }
    }

    Generate-ResearchIndex -ResearchDir $script:ResearchDir -DryRun:$DryRun
    Wait-Continue
}

function Action-ValidateResearchLinks {
    Show-Banner "Research-Metadaten validieren"
    $result = Validate-ResearchMetadata -ResearchDir $script:ResearchDir

    Write-Host ""
    if ($result.Invalid -gt 0) {
        Write-Host "  Validierung: $($result.Invalid) Dateien mit Warnungen" -ForegroundColor Yellow
    } else {
        Write-Host "  Validierung erfolgreich: Alle $($result.Valid) Dateien valide" -ForegroundColor Green
    }

    Wait-Continue
}

function Action-OllamaSettings {
    Show-Banner "Ollama Einstellungen"

    Write-Host "  Status:" -ForegroundColor Cyan
    Write-Host "    URL:          $script:OllamaUrl" -ForegroundColor White
    Write-Host "    Modell:       $script:OllamaModel" -ForegroundColor White
    Write-Host "    AI aktiviert: $script:UseAI" -ForegroundColor White
    Write-Host ""

    Write-Host "  Pruefe Verbindung..." -NoNewline
    $available = Test-OllamaAvailable
    if ($available) {
        Write-Host " OK" -ForegroundColor Green
    } else {
        Write-Host " NICHT ERREICHBAR" -ForegroundColor Red
        Write-Host "    Stellen Sie sicher, dass Ollama laeuft: ollama serve" -ForegroundColor Yellow
    }

    Write-Host ""
    $choice = Get-UserChoice -Prompt "Optionen:" -Options @("AI aktivieren/deaktivieren", "Zurueck") -Default 2

    if ($choice -eq 1) {
        $script:UseAI = -not $script:UseAI
        Write-Host "  AI $(if ($script:UseAI) { 'aktiviert' } else { 'deaktiviert' })" -ForegroundColor Green
        Start-Sleep -Seconds 2
    }
}

# ============================================================================
# SECTION 10: INITIALIZATION
# ============================================================================

function Find-RepositoryRoot {
    $current = Get-Location
    while ($current) {
        if (Test-Path (Join-Path -Path $current -ChildPath ".git")) {
            return $current.Path
        }
        $current = $current.Parent
    }
    return (Get-Location).Path
}

function Initialize-System {
    Show-Banner "Initialisierung"

    Write-Host "  [1/5] Pruefe Repository..." -NoNewline
    $script:RepoRoot = Find-RepositoryRoot
    Write-Host " OK" -ForegroundColor Green
    Write-Host "        $script:RepoRoot" -ForegroundColor Gray

    Write-Host "  [2/5] Pruefe Research-Verzeichnis..." -NoNewline
    $script:ResearchDir = Join-Path $script:RepoRoot "docs/research"
    if (-not (Test-Path $script:ResearchDir)) {
        Write-Host " FEHLER" -ForegroundColor Red
        Write-Host "  docs/research nicht gefunden: $script:ResearchDir" -ForegroundColor Red
        Exit-WithCode -Reason "Research directory not found" -Code $script:ExitCodes.ResearchDirMissing
    }
    Write-Host " OK" -ForegroundColor Green
    Write-Host "        $script:ResearchDir" -ForegroundColor Gray

    Write-Host "  [3/5] Pruefe Source-Verzeichnis..." -NoNewline
    if (-not [string]::IsNullOrWhiteSpace($SourcePath)) {
        $script:SourceDir = $SourcePath
        if (-not (Test-Path $script:SourceDir)) {
            Write-Host " FEHLER" -ForegroundColor Red
            Write-Host "  Source-Verzeichnis nicht gefunden: $script:SourceDir" -ForegroundColor Red
            Exit-WithCode -Reason "Source directory not found" -Code $script:ExitCodes.SourceDirMissing
        }
        Write-Host " OK" -ForegroundColor Green
        Write-Host "        $script:SourceDir" -ForegroundColor Gray
    } else {
        $candidate = Join-Path $script:RepoRoot "src"
        if (Test-Path $candidate) {
            $script:SourceDir = $candidate
            Write-Host " OK (auto: src)" -ForegroundColor Green
            Write-Host "        $script:SourceDir" -ForegroundColor Gray
        } else {
            Write-Host " NICHT GESETZT (Code-Audit deaktiviert)" -ForegroundColor Yellow
        }
    }

    Write-Host "  [4/5] Pruefe GitHub CLI..." -NoNewline
    if (-not (Test-GitHubCLI)) {
        Write-Host " FEHLER" -ForegroundColor Red
        Write-Host "  GitHub CLI nicht installiert! Download: https://cli.github.com/" -ForegroundColor Red
        Exit-WithCode -Reason "GitHub CLI missing" -Code $script:ExitCodes.GitHubCliMissing
    }
    Write-Host " OK" -ForegroundColor Green

    Write-Host "  [5/5] Pruefe GitHub Authentifizierung..." -NoNewline
    if (-not (Test-GitHubAuth)) {
        Write-Host " FEHLER" -ForegroundColor Red
        Write-Host "  Nicht authentifiziert! Fuehren Sie aus: gh auth login" -ForegroundColor Red
        Exit-WithCode -Reason "GitHub authentication failed" -Code $script:ExitCodes.GitHubAuthFailed
    }
    Write-Host " OK" -ForegroundColor Green

    # Optional Ollama check
    Write-Host ""
    if ($script:UseAI) {
        Write-Host "  Pruefe Ollama..." -NoNewline
        if (Test-OllamaAvailable) {
            Write-Host " OK (Modell: $script:OllamaModel)" -ForegroundColor Green
        } else {
            Write-Host " NICHT VERFUEGBAR (AI deaktiviert)" -ForegroundColor Yellow
            $script:UseAI = $false
        }
    }

    Write-Host ""
    Write-Host "  System bereit!" -ForegroundColor Green
    Wait-Continue -Message "Druecken Sie Enter um das Hauptmenue zu oeffnen..."
}

# ============================================================================
# SECTION 11: INTERACTIVE MENU
# ============================================================================

function Show-MainMenu {
    Show-Banner "Research Backlog Manager"

    Write-Host "  Repository:   $Repository" -ForegroundColor Cyan
    Write-Host "  Research-Dir: $script:ResearchDir" -ForegroundColor Cyan
    if ($script:SourceDir) {
        Write-Host "  Source-Dir:   $script:SourceDir" -ForegroundColor Cyan
    }
    Write-Host "  AI Status:    " -NoNewline -ForegroundColor Cyan
    if ($script:UseAI) {
        $ollamaStatus = if (Test-OllamaAvailable) { "AKTIV ($script:OllamaModel)" } else { "NICHT ERREICHBAR" }
        $color = if (Test-OllamaAvailable) { "Green" } else { "Red" }
        Write-Host $ollamaStatus -ForegroundColor $color
    } else {
        Write-Host "DEAKTIVIERT" -ForegroundColor Gray
    }
    if ($DryRun) {
        Write-Host "  Mode:         DRY RUN" -ForegroundColor Yellow
    }
    Write-Host ""

    Write-Host "  Optionen:" -ForegroundColor Green
    Write-Host ""
    Write-Host "    [1] Statistiken anzeigen (Research Coverage)" -ForegroundColor Cyan
    Write-Host "    [2] Code-Audit durchfuehren (undokumentierte Referenzen)" -ForegroundColor Yellow
    Write-Host "    [3] Research-Issues fuer Modul erstellen" -ForegroundColor Yellow
    Write-Host "    [4] Alle Research-Issues erstellen" -ForegroundColor Yellow
    Write-Host "    [5] Research-Index regenerieren" -ForegroundColor Cyan
    Write-Host "    [6] Research-Metadaten validieren" -ForegroundColor Cyan
    Write-Host "    [7] Ollama-Einstellungen" -ForegroundColor Cyan
    Write-Host "    [8] Beenden" -ForegroundColor Red
    Write-Host ""
    Write-Host "  Ihre Wahl: " -NoNewline -ForegroundColor Yellow
    return (Read-Host)
}

function Start-Main {
    Initialize-System

    while ($true) {
        $choice = Show-MainMenu

        switch ($choice) {
            "1" { Action-Stats }
            "2" { Action-AuditCodebase }
            "3" {
                Write-Host ""
                Write-Host "  Modulname eingeben: " -NoNewline -ForegroundColor Yellow
                $mod = Read-Host
                if (-not [string]::IsNullOrWhiteSpace($mod)) {
                    Action-CreateModuleIssues -ModuleNameParam $mod
                }
            }
            "4" { Action-CreateAllIssues }
            "5" { Action-GenerateResearchIndex }
            "6" { Action-ValidateResearchLinks }
            "7" { Action-OllamaSettings }
            "8" {
                Write-Host ""
                Write-Host "  Auf Wiedersehen!" -ForegroundColor Green
                exit $script:ExitCodes.Success
            }
            default {
                Write-Host "  Ungueltige Eingabe!" -ForegroundColor Red
                Start-Sleep -Seconds 1
            }
        }
    }
}

function Start-Automation {
    Initialize-System

    switch ($Action) {
        "stats" {
            Action-Stats
        }
        "audit-codebase" {
            Action-AuditCodebase
        }
        "create-module-issues" {
            if ([string]::IsNullOrWhiteSpace($ModuleName)) {
                Write-Host "  -ModuleName ist erforderlich fuer 'create-module-issues'" -ForegroundColor Red
                Exit-WithCode -Reason "ModuleName required" -Code $script:ExitCodes.GenericError
            }
            Action-CreateModuleIssues -ModuleNameParam $ModuleName
        }
        "create-all-issues" {
            Action-CreateAllIssues
        }
        "generate-research-index" {
            Action-GenerateResearchIndex
        }
        "validate-research-links" {
            Action-ValidateResearchLinks
        }
        "ollama-settings" {
            Action-OllamaSettings
        }
        default {
            Write-Host "  Unbekannte Action: $Action" -ForegroundColor Red
            Exit-WithCode -Reason "Unknown action" -Code $script:ExitCodes.UnknownAction
        }
    }
}

# ============================================================================
# SECTION 12: ENTRY POINT
# ============================================================================

if ($Help) {
    Show-Usage
    exit $script:ExitCodes.Success
}

if ($Action -eq "interactive") {
    Start-Main
} else {
    Start-Automation
}

exit $script:ExitCodes.Success
