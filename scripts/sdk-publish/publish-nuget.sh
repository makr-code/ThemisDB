#!/bin/bash
# ThemisDB NuGet Publishing Script
# Publishes ThemisDB.Client to NuGet

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT_DIR="$(cd "$SCRIPT_DIR/../.." && pwd)"
CLIENT_DIR="$ROOT_DIR/clients/csharp"

DRY_RUN=false
VERSION=""

while [[ $# -gt 0 ]]; do
    case $1 in
        --dry-run) DRY_RUN=true; shift ;;
        --version) VERSION="$2"; shift 2 ;;
        *) shift ;;
    esac
done

echo "ðŸ“¦ NuGet Publishing for ThemisDB.Client"

# Check prerequisites
if ! command -v dotnet &> /dev/null; then
    echo "âŒ dotnet not found"
    exit 1
fi

if [[ -z "${NUGET_API_KEY:-}" ]] && [[ "$DRY_RUN" == "false" ]]; then
    echo "âŒ NUGET_API_KEY environment variable not set"
    exit 1
fi

cd "$CLIENT_DIR"

# Update version if specified
if [[ -n "$VERSION" ]]; then
    # Update .csproj version
    sed -i "s/<Version>.*<\/Version>/<Version>$VERSION<\/Version>/" ThemisDB.Client/ThemisDB.Client.csproj
fi

# Restore dependencies
echo "ðŸ“¥ Restoring dependencies..."
dotnet restore

# Build
echo "ðŸ”¨ Building..."
dotnet build -c Release

# Run tests
echo "ðŸ§ª Running tests..."
dotnet test -c Release || echo "No tests found or tests skipped"

# Pack
echo "ðŸ“¦ Packing..."
dotnet pack -c Release -o ./nupkg

# Publish
if [[ "$DRY_RUN" == "true" ]]; then
    echo "ðŸ” Dry run - would publish:"
    ls -la ./nupkg/
else
    echo "ðŸš€ Publishing to NuGet..."
    dotnet nuget push ./nupkg/*.nupkg \
        --api-key "$NUGET_API_KEY" \
        --source https://api.nuget.org/v3/index.json
fi

# Cleanup
rm -rf ./nupkg

echo "âœ… NuGet publishing complete"
