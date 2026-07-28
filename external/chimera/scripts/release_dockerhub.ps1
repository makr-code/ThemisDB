param(
    [Parameter(Mandatory = $false)]
    [string]$Repository = "themisdb/chimera",

    [Parameter(Mandatory = $false)]
    [string]$Tag = "",

    [Parameter(Mandatory = $false)]
    [switch]$MultiArch,

    [Parameter(Mandatory = $false)]
    [string]$Platforms = "linux/amd64,linux/arm64",

    [Parameter(Mandatory = $false)]
    [switch]$AlsoLatest,

    [Parameter(Mandatory = $false)]
    [switch]$DryRun,

    [Parameter(Mandatory = $false)]
    [switch]$RequireSemverTag,

    [Parameter(Mandatory = $false)]
    [switch]$RequireCleanGit
)

$ErrorActionPreference = "Stop"

Write-Host "== CHIMERA Docker Release ==" -ForegroundColor Cyan
Write-Host "Repository: $Repository"

function Resolve-ReleaseTag {
    param([string]$InputTag)

    if (-not [string]::IsNullOrWhiteSpace($InputTag)) {
        return $InputTag.Trim().ToLowerInvariant()
    }

    $resolved = ""
    $source = ""

    if (Get-Command git -ErrorAction SilentlyContinue) {
        $exactTag = (git describe --tags --exact-match 2>$null)
        if ($LASTEXITCODE -eq 0 -and -not [string]::IsNullOrWhiteSpace($exactTag)) {
            $resolved = $exactTag.Trim()
            $source = "git exact tag"
        }

        if ([string]::IsNullOrWhiteSpace($resolved)) {
            $latestTag = (git describe --tags --abbrev=0 2>$null)
            $shortSha = (git rev-parse --short HEAD 2>$null)
            if ($LASTEXITCODE -eq 0 -and -not [string]::IsNullOrWhiteSpace($latestTag) -and -not [string]::IsNullOrWhiteSpace($shortSha)) {
                $resolved = "$($latestTag.Trim())-$($shortSha.Trim())"
                $source = "git latest tag + short sha"
            } elseif (-not [string]::IsNullOrWhiteSpace($shortSha)) {
                $resolved = "sha-$($shortSha.Trim())"
                $source = "git short sha"
            }
        }
    }

    if ([string]::IsNullOrWhiteSpace($resolved)) {
        $resolved = Get-Date -Format "yyyyMMdd-HHmmss"
        $source = "timestamp fallback"
    }

    $resolved = ($resolved -replace "[^A-Za-z0-9_.-]", "-").ToLowerInvariant()
    Write-Host "Tag automatisch abgeleitet ($source): $resolved" -ForegroundColor Yellow
    return $resolved
}

$Tag = Resolve-ReleaseTag -InputTag $Tag

if ($RequireSemverTag) {
    $semverPattern = '^v?\d+\.\d+\.\d+(-[0-9A-Za-z.-]+)?(\+[0-9A-Za-z.-]+)?$'
    if ($Tag -notmatch $semverPattern) {
        throw "RequireSemverTag aktiv: Tag '$Tag' ist kein gueltiger SemVer-Tag (erlaubt z. B. 1.2.3, v1.2.3, 1.2.3-rc1)."
    }
    Write-Host "SemVer-Tag validiert: $Tag" -ForegroundColor Green
}

if ($RequireCleanGit) {
    if (-not (Get-Command git -ErrorAction SilentlyContinue)) {
        throw "RequireCleanGit aktiv: git ist nicht verfuegbar."
    }

    $status = git status --porcelain 2>$null
    if ($LASTEXITCODE -ne 0) {
        throw "RequireCleanGit aktiv: git status konnte nicht ausgefuehrt werden."
    }

    if (-not [string]::IsNullOrWhiteSpace(($status | Out-String))) {
        throw "RequireCleanGit aktiv: Working Tree ist nicht sauber. Bitte zuerst committen oder stashen."
    }

    Write-Host "Git-Status validiert: Working Tree ist sauber." -ForegroundColor Green
}

Write-Host "Tag:        $Tag"

# Docker verfügbar?
if (-not (Get-Command docker -ErrorAction SilentlyContinue)) {
    throw "Docker CLI nicht gefunden. Bitte Docker Desktop installieren/starten."
}

# Eingeloggt?
$dockerInfo = docker info 2>$null
if ($LASTEXITCODE -ne 0) {
    throw "Docker Daemon nicht erreichbar. Docker Desktop starten."
}

Write-Host "Docker Daemon erreichbar." -ForegroundColor Green

if ($DryRun) {
    Write-Host "Dry-Run aktiv: es werden keine Build/Push-Kommandos ausgefuehrt." -ForegroundColor Yellow
}

if ($MultiArch) {
    Write-Host "Multi-Arch Build aktiv ($Platforms)" -ForegroundColor Yellow

    # Buildx Builder sicherstellen
    $builderName = "chimera-release-builder"
    if ($DryRun) {
        Write-Host "DRYRUN> docker buildx ls"
        Write-Host "DRYRUN> docker buildx create --name $builderName --use"
        Write-Host "DRYRUN> docker buildx inspect --bootstrap"
        Write-Host "DRYRUN> docker buildx build --platform $Platforms --tag $Repository`:$Tag $(if ($AlsoLatest) { "--tag $Repository`:latest" }) --push ."
    } else {
        $existing = docker buildx ls | Select-String -Pattern $builderName -SimpleMatch
        if (-not $existing) {
            docker buildx create --name $builderName --use | Out-Null
        } else {
            docker buildx use $builderName | Out-Null
        }
        docker buildx inspect --bootstrap | Out-Null

        # Build + Push
        docker buildx build `
            --platform $Platforms `
            --tag "$Repository`:$Tag" `
            $(if ($AlsoLatest) { "--tag $Repository`:latest" }) `
            --push `
            .

        if ($LASTEXITCODE -ne 0) {
            throw "Buildx Push fehlgeschlagen."
        }
    }
} else {
    Write-Host "Single-Arch Build (lokale Docker-Architektur)" -ForegroundColor Yellow

    if ($DryRun) {
        Write-Host "DRYRUN> docker build -t $Repository`:$Tag ."
        Write-Host "DRYRUN> docker push $Repository`:$Tag"
        if ($AlsoLatest) {
            Write-Host "DRYRUN> docker tag $Repository`:$Tag $Repository`:latest"
            Write-Host "DRYRUN> docker push $Repository`:latest"
        }
    } else {
        docker build -t "$Repository`:$Tag" .
        if ($LASTEXITCODE -ne 0) {
            throw "docker build fehlgeschlagen."
        }

        docker push "$Repository`:$Tag"
        if ($LASTEXITCODE -ne 0) {
            throw "docker push fehlgeschlagen."
        }

        if ($AlsoLatest) {
            docker tag "$Repository`:$Tag" "$Repository`:latest"
            docker push "$Repository`:latest"
            if ($LASTEXITCODE -ne 0) {
                throw "Push von latest fehlgeschlagen."
            }
        }
    }
}

if ($DryRun) {
    Write-Host "Dry-Run abgeschlossen (kein Build/Push ausgefuehrt)." -ForegroundColor Green
} else {
    Write-Host "Release erfolgreich veröffentlicht." -ForegroundColor Green
}
Write-Host "Nächste Schritte (QNAP):"
Write-Host "  1) .env.release.example -> .env"
Write-Host "  2) IMAGE_TAG auf $Tag setzen"
Write-Host "  3) docker compose -f docker-compose.release.yml pull"
Write-Host "  4) docker compose -f docker-compose.release.yml up -d"
