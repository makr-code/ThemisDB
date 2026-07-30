# Migrate local admin tools to remote repo and replace with submodule
# Usage: .\migrate_admin_tools_to_submodule.ps1 -RemoteUrl <git-url> -Prefix <local-path>
param(
    [Parameter(Mandatory=$true)]
    [string]$RemoteUrl,
    [Parameter(Mandatory=$false)]
    [string]$LocalPath = "projects/Themis.AdminTools.Shared",
    [Parameter(Mandatory=$false)]
    [string]$TempBranch = "admin-tools-split"
)

Write-Host "Creating subtree split for $LocalPath -> branch $TempBranch"
git subtree split --prefix=$LocalPath -b $TempBranch
if($LASTEXITCODE -ne 0){
    Write-Error "git subtree split failed"
    exit 1
}

Write-Host "Adding remote $RemoteUrl as admintools"
$existing = git remote | Select-String -Pattern '^admintools$' -Quiet
if(-not $existing){
    git remote add admintools $RemoteUrl
} else {
    Write-Host "remote admintools already exists"
}

Write-Host "Pushing split branch to remote main"
git push admintools $TempBranch:main
if($LASTEXITCODE -ne 0){
    Write-Error "git push to remote failed"
    exit 1
}

Write-Host "Remove local folder $LocalPath and add submodule"
git rm -r --cached $LocalPath
Remove-Item -Recurse -Force $LocalPath

git submodule add $RemoteUrl $LocalPath
if($LASTEXITCODE -ne 0){
    Write-Error "git submodule add failed"
    exit 1
}

git add .gitmodules $LocalPath
git commit -m "Replace local admin tools with submodule projects/themis_admin_tools"
Write-Host "Done. Don't forget to push your commits: git push origin HEAD"
