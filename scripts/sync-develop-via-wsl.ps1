param(
    [string]$RepoRoot = (Get-Location).Path,
    [string]$Branch = "develop",
    [string]$Remote = "origin",
    [switch]$DryRun,
    [switch]$AutoStash = $true,
    [ValidateSet("stop-on-conflict", "prefer-local")]
    [string]$ResolveMode = "prefer-local"
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

function Convert-WindowsPathToWsl {
    param([Parameter(Mandatory = $true)][string]$WindowsPath)

    $full = [System.IO.Path]::GetFullPath($WindowsPath)
    $normalized = $full -replace '\\', '/'

    if ($normalized -notmatch '^[A-Za-z]:') {
        throw "Nur absolute Windows-Pfade mit Laufwerksbuchstaben werden unterstuetzt: $WindowsPath"
    }

    $drive = $normalized.Substring(0, 1).ToLowerInvariant()
    $rest = $normalized.Substring(2)
    return "/mnt/$drive$rest"
}

Write-Host "[1/8] Voraussetzungen pruefen ..."
if (-not (Get-Command git -ErrorAction SilentlyContinue)) {
    throw "git ist nicht im PATH verfuegbar."
}
if (-not (Get-Command wsl -ErrorAction SilentlyContinue)) {
    throw "wsl ist nicht im PATH verfuegbar."
}

if (-not (Test-Path $RepoRoot)) {
    throw "RepoRoot existiert nicht: $RepoRoot"
}

Push-Location $RepoRoot
$createdStash = $false
$createdStashRef = $null
try {
    git rev-parse --is-inside-work-tree | Out-Null

    Write-Host "[2/8] Arbeitsbaum pruefen ..."
    $porcelain = git status --porcelain --untracked-files=no
    if ($porcelain) {
        if (-not $AutoStash) {
            throw "Arbeitsbaum ist nicht sauber. Bitte zuerst committen/stashen oder -AutoStash nutzen."
        }

        Write-Host "    Arbeitsbaum ist nicht sauber -> Auto-Stash aktiv."
        $stashMessage = "auto-sync-wsl-$([DateTime]::UtcNow.ToString('yyyyMMdd_HHmmss'))"
        git stash push -m $stashMessage | Out-Null

        $createdStashRef = (git stash list | Select-String -Pattern $stashMessage | Select-Object -First 1).ToString().Split(':')[0]
        if (-not $createdStashRef) {
            throw "Auto-Stash konnte nicht erstellt werden."
        }
        $createdStash = $true
    }

    Write-Host "[3/8] Remote-Daten aktualisieren ..."
    git fetch $Remote

    $tracking = "$Remote/$Branch"
    $ahead = [int](git rev-list --count "$tracking..$Branch")
    $behind = [int](git rev-list --count "$Branch..$tracking")

    Write-Host "    Branch-Status: ahead=$ahead, behind=$behind"

    Write-Host "[4/8] Patch-Set erzeugen ..."
    $stamp = Get-Date -Format "yyyyMMdd_HHmmss"
    $patchRoot = Join-Path $env:TEMP ("themis_wsl_sync_$stamp")
    $patchDir = Join-Path $patchRoot "patches"
    New-Item -ItemType Directory -Path $patchDir -Force | Out-Null

    if ($ahead -gt 0) {
        git format-patch --quiet --output-directory "$patchDir" "$tracking..$Branch"
    }

    $repoUrl = (git remote get-url $Remote).Trim()
    $gitUserName = (git config --get user.name)
    $gitUserEmail = (git config --get user.email)

    if (-not $gitUserName -or -not $gitUserEmail) {
        throw "Lokale git user.name/user.email sind nicht gesetzt."
    }

    $patchDirWsl = Convert-WindowsPathToWsl -WindowsPath $patchDir
    $workspaceWsl = "/tmp/themis-sync-$stamp"

    Write-Host "[5/8] WSL-Sync-Skript vorbereiten ..."
    $wslScriptPath = Join-Path $patchRoot "sync.sh"

    $wslScript = @'
#!/usr/bin/env bash
set -euo pipefail

REPO_URL='__REPO_URL__'
BRANCH='__BRANCH__'
REMOTE='__REMOTE__'
PATCH_DIR='__PATCH_DIR__'
WORKDIR='__WORKDIR__'
DRY_RUN='__DRY_RUN__'
RESOLVE_MODE='__RESOLVE_MODE__'
GIT_USER_NAME='__GIT_USER_NAME__'
GIT_USER_EMAIL='__GIT_USER_EMAIL__'

rm -rf "$WORKDIR"
git clone "$REPO_URL" "$WORKDIR"
cd "$WORKDIR"

git config user.name "$GIT_USER_NAME"
git config user.email "$GIT_USER_EMAIL"

git checkout "$BRANCH"
git fetch "$REMOTE"
git pull --rebase "$REMOTE" "$BRANCH"

shopt -s nullglob
patches=("$PATCH_DIR"/*.patch)
if (( ${#patches[@]} > 0 )); then
    if ! git am --3way "${patches[@]}"; then
        if [[ "$RESOLVE_MODE" != "prefer-local" ]]; then
            echo "git am Konflikt. Abbruch (ResolveMode=$RESOLVE_MODE)."
            exit 1
        fi

        echo "git am Konflikte erkannt -> automatische Aufloesung (lokale Commits bevorzugen)."
        while [[ -d .git/rebase-apply ]]; do
            conflict_files="$(git diff --name-only --diff-filter=U || true)"
            if [[ -n "$conflict_files" ]]; then
                while IFS= read -r f; do
                    [[ -n "$f" ]] || continue
                    git checkout --theirs -- "$f" || true
                    git add -- "$f"
                done <<< "$conflict_files"
            fi

            if git am --continue; then
                continue
            fi

            conflict_files="$(git diff --name-only --diff-filter=U || true)"
            if [[ -z "$conflict_files" ]]; then
                echo "git am --continue fehlgeschlagen ohne offene Konfliktdateien."
                exit 1
            fi
        done
    fi
fi

if [[ "$DRY_RUN" == "1" ]]; then
  echo "DRY-RUN: Kein Push ausgefuehrt."
  git status --short
  git log --oneline --decorate -n 10
  exit 0
fi

git push "$REMOTE" "$BRANCH"
echo "SYNC_DONE"
'@

    $wslScript = $wslScript.Replace("__REPO_URL__", $repoUrl)
    $wslScript = $wslScript.Replace("__BRANCH__", $Branch)
    $wslScript = $wslScript.Replace("__REMOTE__", $Remote)
    $wslScript = $wslScript.Replace("__PATCH_DIR__", $patchDirWsl)
    $wslScript = $wslScript.Replace("__WORKDIR__", $workspaceWsl)
    $wslScript = $wslScript.Replace("__DRY_RUN__", ($(if ($DryRun) { "1" } else { "0" })))
    $wslScript = $wslScript.Replace("__RESOLVE_MODE__", $ResolveMode)
    $wslScript = $wslScript.Replace("__GIT_USER_NAME__", $gitUserName)
    $wslScript = $wslScript.Replace("__GIT_USER_EMAIL__", $gitUserEmail)

    $wslScriptLf = $wslScript -replace "`r`n", "`n"
    $utf8NoBom = New-Object System.Text.UTF8Encoding($false)
    [System.IO.File]::WriteAllText($wslScriptPath, $wslScriptLf, $utf8NoBom)

    $wslScriptPathWsl = Convert-WindowsPathToWsl -WindowsPath $wslScriptPath

    Write-Host "[6/8] WSL-Skript ausfuehren ..."
    wsl bash "$wslScriptPathWsl"

    Write-Host "[7/8] Ergebnis lokal aktualisieren ..."
    git fetch $Remote
    git status -sb

    Write-Host "[8/8] Fertig."
    if ($DryRun) {
        Write-Host "Dry-Run beendet. Kein Push wurde ausgefuehrt."
    }
}
finally {
    if ($patchRoot -and (Test-Path $patchRoot)) {
        Remove-Item -Path $patchRoot -Recurse -Force -ErrorAction SilentlyContinue
    }
    if ($createdStash -and $createdStashRef) {
        Write-Host "[Cleanup] Auto-Stash wiederherstellen ($createdStashRef) ..."
        try {
            git stash pop $createdStashRef | Out-Null
        }
        catch {
            Write-Warning "Auto-Stash konnte nicht automatisch wiederhergestellt werden. Bitte manuell pruefen: git stash list"
        }
    }
    Pop-Location
}
