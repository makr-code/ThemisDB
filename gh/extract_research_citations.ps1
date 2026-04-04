#Requires -Version 5.1
<#
.SYNOPSIS
    Automated Research Citation Extraction & Insertion for ThemisDB modules
.DESCRIPTION
    Scans source code and markdown files for research references, generates
    IEEE-standard citations, and inserts them into README.md and
    FUTURE_ENHANCEMENTS.md for every module under .\src\.
.VERSION
    1.0.0
.EXAMPLE
    .\gh\extract_research_citations.ps1 -Action full -SourcePath .\src -DryRun
    .\gh\extract_research_citations.ps1 -Action scan -ModuleName index -Yes
    .\gh\extract_research_citations.ps1 -Action stats
#>

param(
    [string]$SourcePath = ".\src",
    [string]$ModuleName = "",
    [ValidateSet("scan", "generate", "insert", "full", "stats", "validate")]
    [string]$Action = "full",

    [string]$IEEECitationStyle = "ieee",
    [int]$MaxCitationsPerModule = 50,
    [switch]$IncludeCommitMessages = $false,
    [int]$CommitMessagesLast = 100,

    [switch]$DryRun = $false,
    [switch]$CreateBackup = $true,
    [switch]$Yes = $false,
    [switch]$Help = $false
)

# ============================================================================
# EXIT CODES
# ============================================================================

$script:ExitCodes = @{
    Success           = 0
    GenericError      = 1
    SourceNotFound    = 2
    ModuleNotFound    = 3
    CitationParseError = 4
    IEEEGenFailed     = 5
    FileWriteFailed   = 6
}

# ============================================================================
# HELP
# ============================================================================

function Show-Help {
    Write-Host ""
    Write-Host "  extract_research_citations.ps1 - Automated Research Citation Extraction" -ForegroundColor Cyan
    Write-Host "  ========================================================================" -ForegroundColor Cyan
    Write-Host ""
    Write-Host "  Parameters:" -ForegroundColor Green
    Write-Host "    -Action <scan|generate|insert|full|stats|validate>  (default: full)" -ForegroundColor White
    Write-Host "    -SourcePath <path>          (default: .\src)" -ForegroundColor White
    Write-Host "    -ModuleName <name>          (optional: only one module)" -ForegroundColor White
    Write-Host "    -IEEECitationStyle <ieee>   (default: ieee)" -ForegroundColor White
    Write-Host "    -MaxCitationsPerModule <n>  (default: 50)" -ForegroundColor White
    Write-Host "    -IncludeCommitMessages      (scan git log for references)" -ForegroundColor White
    Write-Host "    -CommitMessagesLast <n>     (last N commits, default: 100)" -ForegroundColor White
    Write-Host "    -DryRun                     (show changes without writing)" -ForegroundColor White
    Write-Host "    -CreateBackup               (backup .md files before writing)" -ForegroundColor White
    Write-Host "    -Yes                        (auto-confirm all prompts)" -ForegroundColor White
    Write-Host "    -Help                       (show this help)" -ForegroundColor White
    Write-Host ""
    Write-Host "  Actions:" -ForegroundColor Green
    Write-Host "    scan      - Scan source files, show citations found (no writes)" -ForegroundColor White
    Write-Host "    generate  - Generate IEEE citations and save as JSON report" -ForegroundColor White
    Write-Host "    insert    - Insert IEEE citations into README / FUTURE_ENHANCEMENTS" -ForegroundColor White
    Write-Host "    full      - scan -> generate -> insert (complete workflow)" -ForegroundColor White
    Write-Host "    stats     - Show citation statistics per module" -ForegroundColor White
    Write-Host "    validate  - Validate all generated citations for errors" -ForegroundColor White
    Write-Host ""
}

if ($Help) {
    Show-Help
    exit $script:ExitCodes.Success
}

# ============================================================================
# UTILITIES
# ============================================================================

function Write-Step {
    param([string]$Message)
    Write-Host ""
    Write-Host "  $Message" -ForegroundColor Cyan
    Write-Host "  $('=' * ($Message.Length))" -ForegroundColor Cyan
}

function Write-Info {
    param([string]$Message)
    Write-Host "  $Message" -ForegroundColor White
}

function Write-Ok {
    param([string]$Message)
    Write-Host "  [OK] $Message" -ForegroundColor Green
}

function Write-Warn {
    param([string]$Message)
    Write-Host "  [WARN] $Message" -ForegroundColor Yellow
}

function Write-Err {
    param([string]$Message)
    Write-Host "  [ERR] $Message" -ForegroundColor Red
}

# ============================================================================
# PHASE 1: PARSING
# ============================================================================

<#
.SYNOPSIS
    Parse research-related comments from a single source file.
    Recognised patterns (case-insensitive):
      // Based on:  // Reference:  // See:  // Citation:  // Paper:
      // Source:    // See also:   // Standard:  // RFC:  // ISO:
      // Research:  // DOI:        // Authors:   // Conference:
#>
function Parse-SourceCodeComments {
    param(
        [string]$FilePath
    )

    $citations = [System.Collections.Generic.List[PSObject]]::new()
    if (-not (Test-Path $FilePath)) { return $citations }

    $lines = Get-Content $FilePath -ErrorAction SilentlyContinue
    if (-not $lines) { return $citations }

    # We collect multi-line citation blocks that start with a trigger keyword
    # and continue with additional keyword-tagged continuation lines.
    $triggerPattern = '(?i)//\s*(based on|reference|citation|paper|source|see also|see|standard|rfc|iso|research|doi)\s*:\s*(.+)'
    $continuationPattern = '(?i)//\s*(authors?|year|conference|journal|publisher|edition|volume|pages?|doi|url|link|arxiv)\s*:\s*(.+)'

    $current = $null
    $rawLines = [System.Collections.Generic.List[string]]::new()

    foreach ($line in $lines) {
        $trimmed = $line.TrimStart()

        if ($trimmed -match $triggerPattern) {
            # Save any previous block
            if ($current) {
                $current | Add-Member -NotePropertyName RawLines -NotePropertyValue ($rawLines.ToArray()) -Force
                $citations.Add($current)
            }

            $keyword = $Matches[1].ToLower()
            $value   = $Matches[2].Trim()

            $current = [PSCustomObject]@{
                SourceFile  = $FilePath
                TriggerKey  = $keyword
                Title       = $value
                Authors     = ""
                Year        = ""
                Conference  = ""
                Journal     = ""
                Publisher   = ""
                DOI         = ""
                URL         = ""
                Type        = "paper"
                Raw         = $line.Trim()
            }
            $rawLines = [System.Collections.Generic.List[string]]::new()
            $rawLines.Add($line.Trim())

            # Classify type from trigger keyword
            switch -Wildcard ($keyword) {
                "standard" { $current.Type = "standard" }
                "rfc"      { $current.Type = "standard" }
                "iso"      { $current.Type = "standard" }
                "reference"{ $current.Type = "paper" }
                default    { $current.Type = "paper" }
            }

            # Pre-fill DOI/URL if value looks like one
            if ($value -match 'doi\.org|10\.\d{4}') {
                $current.DOI = $value
                $current.Title = ""
            } elseif ($value -match '^https?://') {
                $current.URL = $value
                $current.Title = ""
            }

        } elseif ($current -and $trimmed -match $continuationPattern) {
            $contKey = $Matches[1].ToLower()
            $contVal = $Matches[2].Trim()
            $rawLines.Add($line.Trim())

            switch -Wildcard ($contKey) {
                "author*"    { $current.Authors   = $contVal }
                "year"       { $current.Year       = $contVal }
                "conference" { $current.Conference = $contVal }
                "journal"    { $current.Journal    = $contVal }
                "publisher"  { $current.Publisher  = $contVal }
                "doi"        { $current.DOI        = $contVal }
                "url"        { $current.URL        = $contVal }
                "link"       { $current.URL        = $contVal }
                "arxiv"      { $current.URL        = "https://arxiv.org/abs/$contVal" }
            }
        } else {
            # Non-continuation line ends the block
            if ($current) {
                $current | Add-Member -NotePropertyName RawLines -NotePropertyValue ($rawLines.ToArray()) -Force
                $citations.Add($current)
                $current = $null
                $rawLines = [System.Collections.Generic.List[string]]::new()
            }
        }
    }

    # Flush last block
    if ($current) {
        $current | Add-Member -NotePropertyName RawLines -NotePropertyValue ($rawLines.ToArray()) -Force
        $citations.Add($current)
    }

    return $citations
}

<#
.SYNOPSIS
    Parse research references from a Markdown file.
    Looks for:
      - [Paper], [Research], [Source], [Citation] link syntax
      - "See also:" / "See:" sections listing references
      - Inline text patterns like "Based on ... (Author, Year)"
#>
function Parse-MarkdownReferences {
    param(
        [string]$FilePath
    )

    $citations = [System.Collections.Generic.List[PSObject]]::new()
    if (-not (Test-Path $FilePath)) { return $citations }

    $lines = Get-Content $FilePath -ErrorAction SilentlyContinue
    if (-not $lines) { return $citations }

    $inSeeAlso = $false

    foreach ($line in $lines) {
        $trimmed = $line.Trim()

        # Detect "See also:" / "See:" section header
        if ($trimmed -match '(?i)^#+\s*(see\s+also|references?|further\s+reading|bibliography)') {
            $inSeeAlso = $true
            continue
        }
        if ($trimmed -match '^#+') {
            $inSeeAlso = $false
        }

        # Markdown link with [Paper], [Research], [Source], [Citation] as link text
        if ($trimmed -match '\[(Paper|Research|Source|Citation)\]\(([^)]+)\)') {
            $url   = $Matches[2]
            $title = "Reference from $([System.IO.Path]::GetFileName($FilePath))"
            $citations.Add([PSCustomObject]@{
                SourceFile  = $FilePath
                TriggerKey  = "markdown-link"
                Title       = $title
                Authors     = ""
                Year        = ""
                Conference  = ""
                Journal     = ""
                Publisher   = ""
                DOI         = ""
                URL         = $url
                Type        = "paper"
                Raw         = $trimmed
            })
        }

        # "See also:" inline (not a section header)
        if ($inSeeAlso -and $trimmed -match '^\s*[-*]\s+(.+)') {
            $refText = $Matches[1].Trim()
            # Try to extract author and year
            $yr = ""
            $auth = ""
            if ($refText -match '\b(\d{4})\b') { $yr = $Matches[1] }
            if ($refText -match '^([A-Z][a-z]+(?:\s+et\s+al\.?)?(?:\s*,\s*[A-Z][a-z]+)?)') {
                $auth = $Matches[1]
            }
            $citations.Add([PSCustomObject]@{
                SourceFile  = $FilePath
                TriggerKey  = "markdown-seealso"
                Title       = $refText
                Authors     = $auth
                Year        = $yr
                Conference  = ""
                Journal     = ""
                Publisher   = ""
                DOI         = ""
                URL         = ""
                Type        = "paper"
                Raw         = $trimmed
            })
        }

        # Inline "Based on X (Author, Year)" or "See X (Author Year)"
        if ($trimmed -match '(?i)(based on|see)\s+["""]?([^"""\n(]+)["""]?\s*\(([^)]+)\)') {
            $title   = $Matches[2].Trim()
            $context = $Matches[3].Trim()
            $yr = ""
            $auth = ""
            if ($context -match '\b(\d{4})\b') { $yr = $Matches[1] }
            if ($context -match '^([^,]+)') { $auth = $Matches[1].Trim() }
            $citations.Add([PSCustomObject]@{
                SourceFile  = $FilePath
                TriggerKey  = "markdown-inline"
                Title       = $title
                Authors     = $auth
                Year        = $yr
                Conference  = ""
                Journal     = ""
                Publisher   = ""
                DOI         = ""
                URL         = ""
                Type        = "paper"
                Raw         = $trimmed
            })
        }
    }

    return $citations
}

<#
.SYNOPSIS
    Optionally parse git commit messages for research references.
#>
function Parse-CommitMessages {
    param(
        [string]$RepoRoot,
        [int]$Last = 100
    )

    $citations = [System.Collections.Generic.List[PSObject]]::new()

    try {
        $logOutput = git -C $RepoRoot log --oneline -n $Last 2>$null
        if (-not $logOutput) { return $citations }

        foreach ($logLine in $logOutput) {
            # Match patterns like "based on 2018 paper by X" or "implement X from Y et al."
            if ($logLine -match '(?i)(based on|implement.*from|paper by|research by)\s+(.+)') {
                $desc = $Matches[2].Trim()
                $yr = ""
                if ($desc -match '\b(19|20)\d{2}\b') { $yr = $Matches[0] }
                $citations.Add([PSCustomObject]@{
                    SourceFile  = "git:commit"
                    TriggerKey  = "commit"
                    Title       = $desc
                    Authors     = ""
                    Year        = $yr
                    Conference  = ""
                    Journal     = ""
                    Publisher   = ""
                    DOI         = ""
                    URL         = ""
                    Type        = "paper"
                    Raw         = $logLine.Trim()
                })
            }
        }
    } catch {
        # git not available or not a repo – silently skip
    }

    return $citations
}

<#
.SYNOPSIS
    Normalize a raw citation object, inferring missing fields where possible.
#>
function Normalize-Citation {
    param(
        [PSObject]$Citation
    )

    # Infer type from DOI/URL patterns
    if ($Citation.Type -eq "paper") {
        if ($Citation.DOI -match 'RFC|rfc' -or $Citation.Title -match '^RFC\s+\d+') {
            $Citation.Type = "standard"
        } elseif ($Citation.Title -match '^ISO' -or $Citation.Title -match '^IEC' -or $Citation.Title -match '^IEEE\s+Std') {
            $Citation.Type = "standard"
        }
    }

    # Extract year from title if not set
    if ([string]::IsNullOrWhiteSpace($Citation.Year)) {
        if ($Citation.Title -match '\b(19|20)\d{2}\b') {
            $Citation.Year = $Matches[0]
        }
    }

    # Trim whitespace
    foreach ($prop in @("Title","Authors","Year","Conference","Journal","Publisher","DOI","URL")) {
        if ($Citation.PSObject.Properties[$prop]) {
            $Citation.$prop = ($Citation.$prop ?? "").Trim()
        }
    }

    return $Citation
}

# ============================================================================
# PHASE 2: IEEE CITATION GENERATION
# ============================================================================

<#
.SYNOPSIS
    Format an author string as IEEE initials-last format.
    Input:  "Yu. A. Malkov, D. A. Yashunin"
    Output: "Y. A. Malkov and D. A. Yashunin"
#>
function Format-IEEEAuthors {
    param([string]$Authors)

    if ([string]::IsNullOrWhiteSpace($Authors)) { return $Authors }

    # Split on " and " or "," (not inside parentheses)
    $parts = $Authors -split '\s*,\s*|\s+and\s+' | ForEach-Object { $_.Trim() } | Where-Object { $_ }

    if ($parts.Count -eq 0) { return $Authors }

    if ($parts.Count -eq 1) { return $parts[0] }

    $last = $parts[-1]
    $rest = $parts[0..($parts.Count - 2)]
    return ($rest -join ", ") + ", and $last"
}

<#
.SYNOPSIS
    Generate a single IEEE-formatted citation string.
    Returns the formatted string or $null on failure.
#>
function Generate-IEEECitation {
    param(
        [int]$Number,
        [PSObject]$Citation
    )

    $num     = "[$Number]"
    $title   = $Citation.Title
    $authors = Format-IEEEAuthors $Citation.Authors
    $year    = $Citation.Year
    $doi     = $Citation.DOI
    $url     = $Citation.URL

    # ---- Standards (ISO / IEC / RFC / IEEE Std) ----
    if ($Citation.Type -eq "standard") {
        $body = "$num $title"
        if (-not [string]::IsNullOrWhiteSpace($year)) { $body += ", $year" }
        if (-not [string]::IsNullOrWhiteSpace($doi))  { $body += ". doi: $doi" }
        elseif (-not [string]::IsNullOrWhiteSpace($url)) {
            $body += ". [Online]. Available: $url"
        }
        $body += "."
        return $body
    }

    # ---- Conference / Journal paper ----
    if (-not [string]::IsNullOrWhiteSpace($authors)) {
        $body = "$num $authors, `"$title`""
    } else {
        $body = "$num `"$title`""
    }

    # Conference
    if (-not [string]::IsNullOrWhiteSpace($Citation.Conference)) {
        $body += " in *$($Citation.Conference)*"
        if (-not [string]::IsNullOrWhiteSpace($year)) { $body += ", $year" }
    }
    # Journal
    elseif (-not [string]::IsNullOrWhiteSpace($Citation.Journal)) {
        $body += " *$($Citation.Journal)*"
        if (-not [string]::IsNullOrWhiteSpace($year)) { $body += ", $year" }
    }
    # No venue info
    else {
        if (-not [string]::IsNullOrWhiteSpace($year)) { $body += " $year" }
    }

    if (-not [string]::IsNullOrWhiteSpace($doi))  { $body += ". doi: $doi" }
    elseif (-not [string]::IsNullOrWhiteSpace($url)) {
        $body += ". [Online]. Available: $url"
    }

    $body += "."
    return $body
}

<#
.SYNOPSIS
    Remove duplicate citations (exact title match, or same author+year).
#>
function Deduplicate-Citations {
    param(
        [System.Collections.Generic.List[PSObject]]$Citations
    )

    $seen    = [System.Collections.Generic.HashSet[string]]::new([System.StringComparer]::OrdinalIgnoreCase)
    $unique  = [System.Collections.Generic.List[PSObject]]::new()

    foreach ($c in $Citations) {
        $key = "$($c.Title)|$($c.Authors)|$($c.Year)"
        if ($seen.Add($key)) {
            $unique.Add($c)
        }
    }
    return $unique
}

<#
.SYNOPSIS
    Group citations by type and assign sequential numbers.
    Returns ordered list with NumberedText property set.
#>
function Group-AndNumberCitations {
    param(
        [System.Collections.Generic.List[PSObject]]$Citations
    )

    $ordered = [System.Collections.Generic.List[PSObject]]::new()

    # Order: papers first, then standards, then others
    foreach ($c in ($Citations | Where-Object { $_.Type -eq "paper" }))    { $ordered.Add($c) }
    foreach ($c in ($Citations | Where-Object { $_.Type -eq "standard" })) { $ordered.Add($c) }
    foreach ($c in ($Citations | Where-Object { $_.Type -notin @("paper","standard") })) { $ordered.Add($c) }

    $counter = 1
    foreach ($c in $ordered) {
        $text = Generate-IEEECitation -Number $counter -Citation $c
        $c | Add-Member -NotePropertyName Number       -NotePropertyValue $counter -Force
        $c | Add-Member -NotePropertyName NumberedText -NotePropertyValue $text    -Force
        $counter++
    }

    return $ordered
}

# ============================================================================
# PHASE 3: MARKDOWN INSERTION
# ============================================================================

<#
.SYNOPSIS
    Build the "## 📚 Scientific Foundations" markdown section.
#>
function Build-ScientificFoundationsSection {
    param(
        [System.Collections.Generic.List[PSObject]]$Citations,
        [string]$ModuleName
    )

    $sb = [System.Text.StringBuilder]::new()
    [void]$sb.AppendLine("## 📚 Scientific Foundations")
    [void]$sb.AppendLine("")
    [void]$sb.AppendLine("This module is based on the following research and standards:")
    [void]$sb.AppendLine("")

    $papers    = $Citations | Where-Object { $_.Type -eq "paper" }
    $standards = $Citations | Where-Object { $_.Type -eq "standard" }

    if ($papers) {
        [void]$sb.AppendLine("### Primary References")
        [void]$sb.AppendLine("")
        foreach ($c in $papers) {
            [void]$sb.AppendLine($c.NumberedText)
            [void]$sb.AppendLine("")
        }
    }

    if ($standards) {
        [void]$sb.AppendLine("### Standards & Specifications")
        [void]$sb.AppendLine("")
        foreach ($c in $standards) {
            [void]$sb.AppendLine($c.NumberedText)
            [void]$sb.AppendLine("")
        }
    }

    return $sb.ToString().TrimEnd()
}

<#
.SYNOPSIS
    Build the "## 🔬 Research References" markdown section for FUTURE_ENHANCEMENTS.md.
#>
function Build-ResearchReferencesSection {
    param(
        [System.Collections.Generic.List[PSObject]]$Citations,
        [string]$ModuleName
    )

    $sb = [System.Text.StringBuilder]::new()
    [void]$sb.AppendLine("## 🔬 Research References")
    [void]$sb.AppendLine("")
    [void]$sb.AppendLine("The following publications inform the future development of this module:")
    [void]$sb.AppendLine("")

    foreach ($c in $Citations) {
        [void]$sb.AppendLine($c.NumberedText)
        [void]$sb.AppendLine("")
    }

    return $sb.ToString().TrimEnd()
}

<#
.SYNOPSIS
    Insert or replace a markdown section in a file.
    - If the section header already exists: replace from that header to the next ## heading (or EOF).
    - If not: append before "## See Also" if present, otherwise at the end of the file.
#>
function Upsert-MarkdownSection {
    param(
        [string]$FilePath,
        [string]$SectionHeader,   # e.g. "## 📚 Scientific Foundations"
        [string]$SectionContent,  # Full section text including the header
        [switch]$DryRun,
        [switch]$CreateBackup
    )

    if (-not (Test-Path $FilePath)) {
        # Create a minimal file so we can insert
        Set-Content $FilePath "" -Encoding UTF8 -ErrorAction SilentlyContinue
    }

    $original = Get-Content $FilePath -Raw -ErrorAction SilentlyContinue
    if ($null -eq $original) { $original = "" }

    # Backup
    if ($CreateBackup -and -not $DryRun) {
        $backupPath = "$FilePath.bak"
        Set-Content $backupPath $original -Encoding UTF8 -ErrorAction SilentlyContinue
    }

    $newContent = ""

    # Escape the header for regex
    $escapedHeader = [regex]::Escape($SectionHeader)

    if ($original -match "(?ms)$escapedHeader") {
        # Section exists – replace from the header to the next ## heading or EOF
        $newContent = [regex]::Replace(
            $original,
            "(?ms)($escapedHeader.*?)(?=\n## |\Z)",
            ($SectionContent + "`n")
        )
    } else {
        # Section does not exist – insert before "## See Also" if present
        $seeAlsoPattern = '(?m)(^## See Also)'
        if ($original -match $seeAlsoPattern) {
            $newContent = [regex]::Replace(
                $original,
                $seeAlsoPattern,
                ($SectionContent + "`n`n" + '$1')
            )
        } else {
            # Append at end
            $newContent = $original.TrimEnd() + "`n`n" + $SectionContent + "`n"
        }
    }

    if ($DryRun) {
        Write-Info "[DryRun] Would write: $FilePath"
        return $true
    }

    try {
        Set-Content $FilePath $newContent -Encoding UTF8
        return $true
    } catch {
        Write-Err "Failed to write $FilePath : $_"
        return $false
    }
}

# ============================================================================
# PHASE 4: MODULE PROCESSING
# ============================================================================

<#
.SYNOPSIS
    Scan all source and markdown files in a module path and return raw citations.
#>
function Scan-ModuleFiles {
    param(
        [string]$ModulePath,
        [string]$ModuleNameLocal,
        [switch]$IncludeCommits,
        [int]$CommitLast
    )

    $all = [System.Collections.Generic.List[PSObject]]::new()

    # Source files
    $srcExts = @("*.cpp","*.c","*.h","*.hpp","*.cu","*.cuh","*.cc")
    foreach ($ext in $srcExts) {
        Get-ChildItem -Path $ModulePath -Filter $ext -Recurse -ErrorAction SilentlyContinue | ForEach-Object {
            $found = Parse-SourceCodeComments -FilePath $_.FullName
            foreach ($c in $found) { $all.Add($c) }
        }
    }

    # Markdown files (skip README section we are about to generate)
    Get-ChildItem -Path $ModulePath -Filter "*.md" -Recurse -ErrorAction SilentlyContinue | ForEach-Object {
        $found = Parse-MarkdownReferences -FilePath $_.FullName
        foreach ($c in $found) { $all.Add($c) }
    }

    # Git commits (optional)
    if ($IncludeCommits) {
        $repoRoot = Split-Path (Split-Path $ModulePath -Parent) -Parent
        $found = Parse-CommitMessages -RepoRoot $repoRoot -Last $CommitLast
        foreach ($c in $found) { $all.Add($c) }
    }

    return $all
}

<#
.SYNOPSIS
    Process a single module: scan → normalize → deduplicate → generate → insert.
    Returns a summary object.
#>
function Process-Module {
    param(
        [string]$ModuleNameLocal,
        [string]$ModulePath,
        [string]$ActionLocal,
        [switch]$DryRunLocal,
        [switch]$CreateBackupLocal,
        [int]$MaxCitations,
        [switch]$IncludeCommits,
        [int]$CommitLast
    )

    $summary = [PSCustomObject]@{
        Module       = $ModuleNameLocal
        Path         = $ModulePath
        FilesScanned = 0
        CitationsRaw = 0
        CitationsFinal = 0
        ReadmeUpdated = $false
        FutureEnhUpdated = $false
        Warnings     = [System.Collections.Generic.List[string]]::new()
    }

    # Count source files
    $srcExts = @("*.cpp","*.c","*.h","*.hpp","*.cu","*.cuh","*.cc","*.md")
    foreach ($ext in $srcExts) {
        $summary.FilesScanned += (Get-ChildItem -Path $ModulePath -Filter $ext -Recurse -ErrorAction SilentlyContinue | Measure-Object).Count
    }

    # Phase 1: Scan
    $rawCitations = Scan-ModuleFiles `
        -ModulePath $ModulePath `
        -ModuleNameLocal $ModuleNameLocal `
        -IncludeCommits:$IncludeCommits `
        -CommitLast $CommitLast

    $summary.CitationsRaw = $rawCitations.Count

    if ($ActionLocal -eq "scan") {
        return $summary
    }

    # Phase 1b: Normalize
    $normalized = [System.Collections.Generic.List[PSObject]]::new()
    foreach ($c in $rawCitations) {
        $nc = Normalize-Citation $c
        if (-not [string]::IsNullOrWhiteSpace($nc.Title)) {
            $normalized.Add($nc)
        } else {
            $summary.Warnings.Add("Empty title after normalization in $($nc.SourceFile)")
        }
    }

    # Phase 2: Deduplicate & limit
    $deduped = Deduplicate-Citations $normalized
    if ($deduped.Count -gt $MaxCitations) {
        $deduped = [System.Collections.Generic.List[PSObject]]($deduped | Select-Object -First $MaxCitations)
        $summary.Warnings.Add("Truncated to $MaxCitations citations (limit reached)")
    }

    if ($deduped.Count -eq 0) {
        $summary.CitationsFinal = 0
        return $summary
    }

    # Phase 2b: Number & generate IEEE text
    $numbered = Group-AndNumberCitations $deduped
    $summary.CitationsFinal = $numbered.Count

    if ($ActionLocal -eq "generate") {
        # Save JSON report
        $reportDir  = Join-Path (Split-Path $ModulePath -Parent | Split-Path -Parent) "tmp"
        if (-not (Test-Path $reportDir)) { New-Item -ItemType Directory -Path $reportDir -Force | Out-Null }
        $reportFile = Join-Path $reportDir "research_citations_$ModuleNameLocal.json"
        if (-not $DryRunLocal) {
            $numbered | ConvertTo-Json -Depth 5 | Set-Content $reportFile -Encoding UTF8
        }
        return $summary
    }

    # Phase 3: Insert
    $readmePath  = Join-Path $ModulePath "README.md"
    $futureEnhPath = Join-Path $ModulePath "FUTURE_ENHANCEMENTS.md"

    $sfSection  = Build-ScientificFoundationsSection -Citations $numbered -ModuleName $ModuleNameLocal
    $rrSection  = Build-ResearchReferencesSection    -Citations $numbered -ModuleName $ModuleNameLocal

    $summary.ReadmeUpdated = Upsert-MarkdownSection `
        -FilePath $readmePath `
        -SectionHeader "## 📚 Scientific Foundations" `
        -SectionContent $sfSection `
        -DryRun:$DryRunLocal `
        -CreateBackup:$CreateBackupLocal

    # FUTURE_ENHANCEMENTS.md may not exist – only write if it exists or skip gracefully
    if (Test-Path $futureEnhPath) {
        $summary.FutureEnhUpdated = Upsert-MarkdownSection `
            -FilePath $futureEnhPath `
            -SectionHeader "## 🔬 Research References" `
            -SectionContent $rrSection `
            -DryRun:$DryRunLocal `
            -CreateBackup:$CreateBackupLocal
    }

    return $summary
}

# ============================================================================
# ACTIONS
# ============================================================================

function Run-Stats {
    param([string[]]$ModulePaths)

    Write-Step "Research Citation Statistics"
    Write-Host ""

    $total = 0
    foreach ($mp in $ModulePaths) {
        $name = Split-Path $mp -Leaf
        $raw  = Scan-ModuleFiles -ModulePath $mp -ModuleNameLocal $name -IncludeCommits:$false -CommitLast 0
        $deduped = Deduplicate-Citations ([System.Collections.Generic.List[PSObject]]$raw)
        Write-Info "  $name : $($deduped.Count) citation(s)"
        $total += $deduped.Count
    }

    Write-Host ""
    Write-Ok "Total across $($ModulePaths.Count) modules: $total citation(s)"
}

function Run-Validate {
    param([string[]]$ModulePaths)

    Write-Step "Validating Research Citations"
    $errors = 0
    $total  = 0

    foreach ($mp in $ModulePaths) {
        $name = Split-Path $mp -Leaf
        $raw  = Scan-ModuleFiles -ModulePath $mp -ModuleNameLocal $name -IncludeCommits:$false -CommitLast 0

        foreach ($c in $raw) {
            $total++
            if ([string]::IsNullOrWhiteSpace($c.Title)) {
                Write-Warn "[$name] Empty title in $($c.SourceFile)"
                $errors++
            }
        }
    }

    Write-Host ""
    if ($errors -eq 0) {
        Write-Ok "Validated $total citation(s) across $($ModulePaths.Count) modules — no errors found."
    } else {
        Write-Warn "Validated $total citation(s): $errors issue(s) found."
    }
}

# ============================================================================
# MAIN ENTRY POINT
# ============================================================================

# Resolve source path
$resolvedSourcePath = $SourcePath
if (-not [System.IO.Path]::IsPathRooted($SourcePath)) {
    $resolvedSourcePath = Join-Path (Get-Location) $SourcePath
}

if (-not (Test-Path $resolvedSourcePath)) {
    Write-Err "Source path not found: $resolvedSourcePath"
    exit $script:ExitCodes.SourceNotFound
}

# Collect module paths
if (-not [string]::IsNullOrWhiteSpace($ModuleName)) {
    $targetPath = Join-Path $resolvedSourcePath $ModuleName
    if (-not (Test-Path $targetPath)) {
        Write-Err "Module not found: $targetPath"
        exit $script:ExitCodes.ModuleNotFound
    }
    $modulePaths = @($targetPath)
} else {
    $modulePaths = Get-ChildItem -Path $resolvedSourcePath -Directory | Select-Object -ExpandProperty FullName
}

if ($modulePaths.Count -eq 0) {
    Write-Warn "No modules found in $resolvedSourcePath"
    exit $script:ExitCodes.Success
}

# ---- stats ----
if ($Action -eq "stats") {
    Run-Stats -ModulePaths $modulePaths
    exit $script:ExitCodes.Success
}

# ---- validate ----
if ($Action -eq "validate") {
    Run-Validate -ModulePaths $modulePaths
    exit $script:ExitCodes.Success
}

# ---- scan / generate / insert / full ----
Write-Step "Research Citation Extraction (Action: $Action)"
Write-Info "Source Path : $resolvedSourcePath"
Write-Info "Modules     : $($modulePaths.Count)"
Write-Info "DryRun      : $DryRun"
Write-Host ""

# Backup root if needed (one archive for all)
if ($CreateBackup -and -not $DryRun -and ($Action -in @("insert","full"))) {
    $stamp      = (Get-Date -Format "yyyyMMdd_HHmmss")
    $backupRoot = Join-Path (Split-Path $resolvedSourcePath -Parent) "src.backup.$stamp"
    Write-Info "Creating backup → $backupRoot"
    try {
        Copy-Item -Recurse -Path $resolvedSourcePath -Destination $backupRoot -ErrorAction Stop
        Write-Ok "Backup created: $backupRoot"
    } catch {
        Write-Warn "Backup failed (continuing): $_"
    }
}

# Confirm (when not DryRun and not -Yes)
if (-not $DryRun -and -not $Yes -and ($Action -in @("insert","full"))) {
    $answer = Read-Host "  Proceed with $Action on $($modulePaths.Count) module(s)? [y/N]"
    if ($answer -notmatch '^[Yy]') {
        Write-Info "Aborted by user."
        exit $script:ExitCodes.Success
    }
}

# Process modules
$summaries = [System.Collections.Generic.List[PSObject]]::new()
$totalCitations = 0
$totalUpdated   = 0

foreach ($mp in $modulePaths) {
    $mName = Split-Path $mp -Leaf

    $sum = Process-Module `
        -ModuleNameLocal $mName `
        -ModulePath $mp `
        -ActionLocal $Action `
        -DryRunLocal:$DryRun `
        -CreateBackupLocal:$CreateBackup `
        -MaxCitations $MaxCitationsPerModule `
        -IncludeCommits:$IncludeCommitMessages `
        -CommitLast $CommitMessagesLast

    $summaries.Add($sum)
    $totalCitations += $sum.CitationsFinal

    $status = if ($sum.CitationsFinal -gt 0) { "[OK]" } else { "[--]" }
    Write-Host "  $status $mName : $($sum.CitationsRaw) raw → $($sum.CitationsFinal) final" -ForegroundColor $(if ($sum.CitationsFinal -gt 0) { "Green" } else { "Gray" })

    if ($sum.Warnings.Count -gt 0) {
        foreach ($w in $sum.Warnings) { Write-Warn "      $w" }
    }

    if ($sum.ReadmeUpdated -or $sum.FutureEnhUpdated) { $totalUpdated++ }
}

# Summary report
Write-Host ""
Write-Step "Summary"
Write-Info "Modules processed : $($summaries.Count)"
Write-Info "Modules updated   : $totalUpdated"
Write-Info "Total citations   : $totalCitations"
if ($DryRun) { Write-Warn "DryRun mode — no files were written." }

# Save JSON report for generate/full
if ($Action -in @("generate","full") -and -not $DryRun) {
    $reportDir  = Join-Path (Split-Path $resolvedSourcePath -Parent) "tmp"
    if (-not (Test-Path $reportDir)) { New-Item -ItemType Directory -Path $reportDir -Force | Out-Null }
    $stamp      = (Get-Date -Format "yyyyMMdd_HHmmss")
    $reportFile = Join-Path $reportDir "research_citations_report_$stamp.json"
    $summaries | ConvertTo-Json -Depth 5 | Set-Content $reportFile -Encoding UTF8
    Write-Ok "Report saved: $reportFile"
}

exit $script:ExitCodes.Success
