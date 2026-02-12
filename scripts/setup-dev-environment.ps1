# Complete Development Environment Setup for ThemisDB (Windows)
# This script sets up everything needed for ThemisDB development

Write-Host "🚀 ThemisDB Complete Development Environment Setup" -ForegroundColor Green
Write-Host ""

# Function to check if a command exists
function Test-CommandExists {
    param($Command)
    try {
        if (Get-Command $Command -ErrorAction Stop) {
            return $true
        }
    }
    catch {
        return $false
    }
}

# Function to print status messages
function Write-Status {
    param($Message)
    Write-Host "==> $Message" -ForegroundColor Cyan
}

function Write-Success {
    param($Message)
    Write-Host "✓ $Message" -ForegroundColor Green
}

function Write-Warning {
    param($Message)
    Write-Host "⚠ $Message" -ForegroundColor Yellow
}

function Write-Error {
    param($Message)
    Write-Host "✗ $Message" -ForegroundColor Red
}

# Check prerequisites
Write-Status "Checking prerequisites..."

$MissingDeps = @()

if (-not (Test-CommandExists "git")) {
    $MissingDeps += "Git"
}

if (-not (Test-CommandExists "python")) {
    $MissingDeps += "Python 3"
}

if (-not (Test-CommandExists "cmake")) {
    $MissingDeps += "CMake"
}

if ($MissingDeps.Count -gt 0) {
    Write-Error "Missing required dependencies: $($MissingDeps -join ', ')"
    Write-Host ""
    Write-Host "Install them from:"
    Write-Host "  - Git: https://git-scm.com/download/win"
    Write-Host "  - Python: https://www.python.org/downloads/"
    Write-Host "  - CMake: https://cmake.org/download/"
    Write-Host "  - Visual Studio 2022: https://visualstudio.microsoft.com/"
    exit 1
}

Write-Success "All prerequisites installed"

# Check for Visual Studio
Write-Status "Checking for Visual Studio 2022..."
$VSPath = "C:\Program Files\Microsoft Visual Studio\2022"
if (Test-Path $VSPath) {
    Write-Success "Visual Studio 2022 found"
} else {
    Write-Warning "Visual Studio 2022 not found at default location"
    Write-Host "  Install from: https://visualstudio.microsoft.com/"
}

# 1. Install Pre-commit Hooks
Write-Status "Installing pre-commit hooks..."
try {
    pip install pre-commit
    pre-commit install
    Write-Success "Pre-commit hooks installed"
}
catch {
    Write-Warning "Failed to install pre-commit: $_"
}

# 2. Setup vcpkg
Write-Status "Setting up vcpkg..."
if (Test-Path "vcpkg") {
    if (-not (Test-Path "vcpkg\vcpkg.exe")) {
        Write-Status "Bootstrapping vcpkg..."
        Push-Location vcpkg
        .\bootstrap-vcpkg.bat -disableMetrics
        Pop-Location
        Write-Success "vcpkg bootstrapped"
    } else {
        Write-Success "vcpkg already bootstrapped"
    }
} else {
    Write-Error "vcpkg directory not found. Clone the repository with submodules:"
    Write-Host "  git submodule update --init --recursive"
    exit 1
}

# 3. Install vcpkg dependencies (optional)
$Response = Read-Host "Install vcpkg dependencies now? (y/n)"
if ($Response -match '^[Yy]') {
    Write-Status "Installing vcpkg dependencies (this may take 30-60 minutes)..."
    .\vcpkg\vcpkg.exe install --triplet x64-windows
    Write-Success "vcpkg dependencies installed"
} else {
    Write-Warning "Skipping vcpkg dependency installation"
    Write-Host "  Run later: .\vcpkg\vcpkg.exe install --triplet x64-windows"
}

# 4. Copy VSCode configuration (if not exists)
Write-Status "Setting up VSCode configuration..."
if (-not (Test-Path ".vscode")) {
    if (Test-Path ".vscode.example") {
        Copy-Item -Recurse .vscode.example .vscode
        Write-Success "VSCode configuration copied from .vscode.example"
    } else {
        Write-Warning "No .vscode.example found"
    }
} else {
    Write-Success ".vscode directory already exists"
}

# 5. Create initial build directory
Write-Status "Creating initial build directory..."
New-Item -ItemType Directory -Force -Path "build-msvc" | Out-Null
Write-Success "Build directory created: build-msvc"

# 6. Generate compile_commands.json
Write-Status "Configuring CMake..."
try {
    cmake -S . -B build-msvc -G "Visual Studio 17 2022" -A x64 -DCMAKE_BUILD_TYPE=Release
    Write-Success "CMake configured successfully"
}
catch {
    Write-Warning "CMake configuration failed: $_"
    Write-Host "  You may need to install dependencies first"
}

# 7. Setup Git configuration
Write-Status "Checking Git configuration..."
$GitUserName = git config --global user.name
$GitUserEmail = git config --global user.email

if (-not $GitUserName) {
    Write-Warning "Git user.name not set. Please configure:"
    Write-Host "  git config --global user.name 'Your Name'"
}

if (-not $GitUserEmail) {
    Write-Warning "Git user.email not set. Please configure:"
    Write-Host "  git config --global user.email 'your.email@example.com'"
}

# Summary
Write-Host ""
Write-Host "✓ Development environment setup complete!" -ForegroundColor Green
Write-Host ""
Write-Host "Next steps:"
Write-Host "  1. Open project in VSCode: code ."
Write-Host "  2. Install recommended extensions (VSCode will prompt)"
Write-Host "  3. Select CMake kit: Ctrl+Shift+P → 'CMake: Select a Kit'"
Write-Host "  4. Build: Press F7 or Ctrl+Shift+P → 'CMake: Build'"
Write-Host "  5. Run tests: Ctrl+Shift+P → 'CMake: Run Tests'"
Write-Host ""
Write-Host "Documentation:"
Write-Host "  - Setup Guide: SETUP.md"
Write-Host "  - Build Guide: .github\copilot\BUILD_GUIDE.md"
Write-Host "  - VSCode Guide: .github\copilot\VSCODE_CONTEXT.md"
Write-Host ""
Write-Host "Happy coding! 🎉" -ForegroundColor Green
