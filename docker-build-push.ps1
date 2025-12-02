#!/usr/bin/env pwsh
# Build and push all ThemisDB Docker images to Docker Hub
# Requires: Docker login (docker login)

param(
    [string]$Version = '1.0.0',
    [string]$Registry = 'themisdb',
    [switch]$Push = $false,
    [switch]$NoBuildCache = $false
)

$ErrorActionPreference = 'Stop'

Write-Host '=== ThemisDB Docker Build & Push ===' -ForegroundColor Blue
Write-Host ''
Write-Host 'Version:' $Version
Write-Host 'Registry:' $Registry
Write-Host 'Push to Docker Hub:' $Push
Write-Host ''

# Check Docker login if pushing
if ($Push) {
    Write-Host 'Checking Docker Hub authentication...' -ForegroundColor Cyan
    $loginCheck = docker info 2>&1 | Select-String 'Username'
    if (-not $loginCheck) {
        Write-Error 'Not logged in to Docker Hub. Run: docker login'
        exit 1
    }
    Write-Host ' Docker Hub authenticated' -ForegroundColor Green
    Write-Host ''
}

# Build arguments
$buildArgs = @()
if ($NoBuildCache) {
    $buildArgs += '--no-cache'
}

# Function to build and optionally push an image
function Build-DockerImage {
    param(
        [string]$Dockerfile,
        [string]$Tag,
        [string[]]$ExtraTags = @(),
        [string]$Platform = 'linux/amd64'
    )
    
    Write-Host ''
    Write-Host 'Building:' $Tag -ForegroundColor Green
    Write-Host 'Dockerfile:' $Dockerfile
    Write-Host 'Platform:' $Platform
    Write-Host ''
    
    # Build image
    $cmd = @('build') + $buildArgs + @(
        '-f', $Dockerfile,
        '-t', $Tag,
        '--platform', $Platform,
        '.'
    )
    
    # Add extra tags
    foreach ($extraTag in $ExtraTags) {
        $cmd += '-t'
        $cmd += $extraTag
    }
    
    & docker @cmd
    
    if ($LASTEXITCODE -ne 0) {
        Write-Error 'Build failed for' $Tag
        exit 1
    }
    
    Write-Host ' Built successfully:' $Tag -ForegroundColor Green
    
    # Push if requested
    if ($Push) {
        Write-Host ''
        Write-Host 'Pushing:' $Tag -ForegroundColor Cyan
        docker push $Tag
        
        if ($LASTEXITCODE -ne 0) {
            Write-Error 'Push failed for' $Tag
            exit 1
        }
        
        # Push extra tags
        foreach ($extraTag in $ExtraTags) {
            Write-Host 'Pushing:' $extraTag -ForegroundColor Cyan
            docker push $extraTag
            
            if ($LASTEXITCODE -ne 0) {
                Write-Error 'Push failed for' $extraTag
                exit 1
            }
        }
        
        Write-Host ' Pushed successfully' -ForegroundColor Green
    }
}

# Build 1: Standard Ubuntu 22.04 (latest)
Build-DockerImage `
    -Dockerfile 'Dockerfile' `
    -Tag "$Registry/themisdb:$Version" `
    -ExtraTags @("$Registry/themisdb:latest") `
    -Platform 'linux/amd64'

# Build 2: QNAP Ubuntu 20.04
Build-DockerImage `
    -Dockerfile 'Dockerfile.qnap' `
    -Tag "$Registry/themisdb:$Version-qnap" `
    -ExtraTags @("$Registry/themisdb:qnap") `
    -Platform 'linux/amd64'

Write-Host ''
Write-Host '=== Build Complete ===' -ForegroundColor Green
Write-Host ''

if ($Push) {
    Write-Host 'Images pushed to Docker Hub:' -ForegroundColor Cyan
    Write-Host '  ' $Registry'/themisdb:' $Version
    Write-Host '  ' $Registry'/themisdb:latest'
    Write-Host '  ' $Registry'/themisdb:' $Version'-qnap'
    Write-Host '  ' $Registry'/themisdb:qnap'
} else {
    Write-Host 'Images built locally:' -ForegroundColor Cyan
    Write-Host '  ' $Registry'/themisdb:' $Version
    Write-Host '  ' $Registry'/themisdb:latest'
    Write-Host '  ' $Registry'/themisdb:' $Version'-qnap'
    Write-Host '  ' $Registry'/themisdb:qnap'
    Write-Host ''
    Write-Host 'To push to Docker Hub, run:' -ForegroundColor Yellow
    Write-Host '  .\docker-build-push.ps1 -Push' -ForegroundColor White
}

Write-Host ''
Write-Host 'Docker Hub page: https://hub.docker.com/r/themisdb/themisdb' -ForegroundColor Cyan
Write-Host ''
