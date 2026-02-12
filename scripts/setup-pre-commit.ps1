# Setup script for pre-commit hooks
# This script installs pre-commit and configures it for ThemisDB

Write-Host "🚀 Setting up pre-commit hooks for ThemisDB..." -ForegroundColor Green

# Check if Python is installed
if (-not (Get-Command python -ErrorAction SilentlyContinue)) {
    Write-Host "❌ Error: Python 3 is required but not installed." -ForegroundColor Red
    Write-Host "Please install Python 3 from https://www.python.org/ and try again."
    exit 1
}

# Check if pip is installed
if (-not (Get-Command pip -ErrorAction SilentlyContinue)) {
    Write-Host "❌ Error: pip is required but not installed." -ForegroundColor Red
    Write-Host "Please install pip and try again."
    exit 1
}

# Install pre-commit
Write-Host "📦 Installing pre-commit..." -ForegroundColor Cyan
pip install pre-commit

# Verify installation
if (-not (Get-Command pre-commit -ErrorAction SilentlyContinue)) {
    Write-Host "⚠️  pre-commit was installed but not found in PATH." -ForegroundColor Yellow
    Write-Host "Please restart your terminal or add Python Scripts to PATH."
    exit 1
}

# Install git hooks
Write-Host "🪝 Installing git hooks..." -ForegroundColor Cyan
pre-commit install

# Run hooks on all files (optional)
Write-Host ""
Write-Host "✅ Pre-commit hooks installed successfully!" -ForegroundColor Green
Write-Host ""
$response = Read-Host "Would you like to run hooks on all files now? (y/n)"
if ($response -match '^[yY]') {
    Write-Host "🔍 Running pre-commit on all files..." -ForegroundColor Cyan
    pre-commit run --all-files
    if ($LASTEXITCODE -ne 0) {
        Write-Host "⚠️  Some hooks failed. Please review and fix." -ForegroundColor Yellow
    }
} else {
    Write-Host "Skipping initial run. Hooks will run automatically on commit."
}

Write-Host ""
Write-Host "🎉 Setup complete!" -ForegroundColor Green
Write-Host ""
Write-Host "Pre-commit hooks are now active. They will run automatically before each commit."
Write-Host "To manually run hooks: pre-commit run --all-files"
Write-Host "To update hooks: pre-commit autoupdate"
