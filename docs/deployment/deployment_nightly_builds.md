# Nightly Builds and DockerHub Push

## Overview

ThemisDB implements automated overnight (nightly) builds that compile the latest code and push Docker images to DockerHub. This ensures that the latest development version is always available for testing and deployment.

## Build Schedule

- **Trigger**: Automatically runs every day at **2:00 AM UTC** (only if changes detected in last 24 hours)
- **Duration**: Approximately 30-60 minutes (depending on cache availability)
- **Output**: Docker images tagged as `nightly` on DockerHub
- **Smart Build**: Skips build if no code changes detected, saving resources

## Workflow Details

### Workflow File

The nightly build is configured in `.github/workflows/nightly-build.yml`

### Build Process

1. **Change Detection Phase**
   - Checks for commits in the last 24 hours
   - Skips build if no changes detected (scheduled runs only)
   - Manual triggers can force build even without changes

2. **Changelog Generation Phase**
   - Automatically analyzes all changes since last build
   - Documents commits, changed files, and categories
   - Generates `NIGHTLY_CHANGELOG_YYYY-MM-DD.md` in `nightly-changelogs/` directory
   - Commits changelog to repository for historical tracking
   - Uploads changelog as workflow artifact

3. **Setup Phase**
   - Determines version from `VERSION` file
   - Generates build date (YYYYMMDD format)
   - Configures push strategy

4. **Binary Build Phase**
   - Cleans up disk space on the runner
   - Uses vcpkg for dependency management
   - Builds ThemisDB server binary with CMake/Ninja
   - Caches build artifacts for faster subsequent builds
   - Uploads binary artifact for Docker build

5. **Docker Build Phase**
   - Downloads the pre-built binary
   - Sets up multi-platform support (QEMU + Buildx)
   - Builds Docker image using `Dockerfile.simple`
   - Pushes to DockerHub with multiple tags
   - Generates build summary report

6. **Notification Phase**
   - Reports build status or skip reason
   - Confirms changelog generation
   - Provides pull commands for the new image

## Docker Image Tags

Each nightly build produces three Docker tags:

- `themisdb/server:nightly` - Always points to the latest nightly build
- `themisdb/server:nightly-YYYYMMDD` - Date-specific nightly build (e.g., `nightly-20231221`)
- `themisdb/server:VERSION-nightly` - Version-specific nightly (e.g., `1.3.0-nightly`)

## Automatic Changelog Generation

Each nightly build automatically generates a detailed changelog documenting all changes since the last build.

### What's Included

The nightly changelog contains:

- **Build metadata**: Date, time, and version information
- **Commit list**: All commits with authors and hashes
- **Changed files**: Complete list of modified files with change types (Modified, Added, Deleted)
- **File statistics**: Count of changed files by type (C++, Python, Markdown, YAML, etc.)
- **Change categories**: Automatic categorization of commits:
  - Features (commits starting with "feat", "feature", "add")
  - Bug fixes (commits starting with "fix", "bugfix")
  - Documentation (commits starting with "docs", "doc", "documentation")
  - Refactoring (commits starting with "refactor", "refact")
- **Docker tags**: List of available Docker image tags for the build

### Location

Changelogs are stored in the `nightly-changelogs/` directory with the naming pattern:
```
nightly-changelogs/NIGHTLY_CHANGELOG_YYYY-MM-DD.md
```

Example: `nightly-changelogs/NIGHTLY_CHANGELOG_2025-12-21.md`

### Accessing Changelogs

**View latest nightly changelog**:
```bash
# Clone repository
git clone https://github.com/makr-code/ThemisDB.git
cd ThemisDB

# View most recent changelog
ls -t nightly-changelogs/*.md | head -1 | xargs cat
```

**View specific date**:
```bash
cat nightly-changelogs/NIGHTLY_CHANGELOG_2025-12-21.md
```

**Download from workflow artifacts**:
Changelogs are also available as workflow artifacts for 30 days after each build. Go to the Actions tab → Select workflow run → Download "nightly-changelog" artifact.

### Example Changelog

```markdown
# Nightly Build Changelog - 2025-12-21

**Build Time**: 2025-12-21 02:00:15 UTC
**Version**: 1.3.0

## Changes Since Last Build

### Commits

- Add feature X (abc123) - John Doe
- Fix bug Y (def456) - Jane Smith

### Changed Files

\`\`\`
M    src/core/database.cpp
A    src/features/new_feature.cpp
\`\`\`

### File Statistics

- C++ files: 15
- Python files: 3

### Change Categories

- Features: 5 commits
- Bug Fixes: 2 commits
```

## Usage

### Pulling the Latest Nightly Build

```bash
docker pull themisdb/server:nightly
```

### Running a Nightly Build

```bash
docker run -d \
  -p 8080:8080 \
  -p 18765:18765 \
  -v themisdb-data:/data \
  --name themisdb-nightly \
  themisdb/server:nightly
```

### Using a Specific Nightly Build

```bash
# Use a specific date
docker pull themisdb/server:nightly-20231221

# Run with specific configuration
docker run -d \
  -p 18765:18765 \
  -v $(pwd)/config.json:/etc/themis/config.json \
  -v themisdb-data:/data \
  themisdb/server:nightly-20231221
```

## Manual Triggering

The workflow can also be triggered manually via GitHub Actions:

1. Go to the repository's "Actions" tab
2. Select "Nightly Build & DockerHub Push" workflow
3. Click "Run workflow"
4. Configure options:
   - **Push to DockerHub**: Enable/disable pushing to DockerHub
   - **Build platforms**: Choose `linux/amd64` (faster) or `linux/amd64,linux/arm64` (multi-arch)
   - **Enable LLM support**: Build with llama.cpp integration
   - **Force build**: Force build even if no changes detected in last 24 hours

## Configuration Requirements

### GitHub Secrets

The following secrets must be configured in the repository settings:

| Secret | Purpose | Required |
|--------|---------|----------|
| `DOCKER_USERNAME` | DockerHub username | Yes |
| `DOCKER_TOKEN` | DockerHub access token | Yes |

### Setting Up Secrets

1. Go to your repository settings
2. Navigate to "Secrets and variables" → "Actions"
3. Add the required secrets:
   - `DOCKER_USERNAME`: Your DockerHub username
   - `DOCKER_TOKEN`: Generate a token at https://hub.docker.com/settings/security

## Build Optimization

### Caching Strategy

The workflow uses GitHub Actions cache to speed up builds:

- **vcpkg cache**: Caches compiled dependencies (~1-2 GB)
- **Docker layer cache**: Caches Docker build layers
- **Build artifacts**: Binary artifacts are cached between jobs

### Disk Space Management

The workflow automatically cleans up unnecessary files to free disk space:
- Removes .NET SDK (~2 GB)
- Removes Android SDK (~8 GB)
- Removes GHC (~4 GB)
- Removes CodeQL (~5 GB)

## Monitoring Build Status

### GitHub Actions UI

1. Go to the repository's "Actions" tab
2. Select "Nightly Build & DockerHub Push"
3. View recent workflow runs and their status

### Build Artifacts

Each build produces:
- Binary artifact (`themis_server_nightly`) - retained for 7 days
- Build summary in the workflow run summary
- Docker images on DockerHub

### Notifications

Build status is reported in the workflow summary with:
- Version information
- Build date and time
- Platform details
- Docker tags generated
- Pull commands for the image

## Troubleshooting

### Build Failures

Common issues and solutions:

1. **vcpkg dependency failures**
   - Clear the cache: Manual workflow run with cache cleared
   - Check vcpkg baseline version in `vcpkg-configuration.json`

2. **Docker push failures**
   - Verify `DOCKER_USERNAME` and `DOCKER_TOKEN` secrets are set correctly
   - Check DockerHub rate limits
   - Ensure DockerHub credentials have push permissions

3. **Disk space issues**
   - The workflow automatically cleans up space
   - If still failing, reduce cache size or build on self-hosted runner

4. **Binary build failures**
   - Check CMake configuration logs
   - Verify all dependencies are available in vcpkg
   - Review compiler errors in the build logs

### Checking Build Logs

1. Go to the failed workflow run
2. Expand the failed job
3. Review the step logs for error messages
4. Common log locations:
   - CMake configuration: "Configure CMake" step
   - Build errors: "Build ThemisDB" step
   - Docker errors: "Build and push Docker image" step

## Platform Support

### Current Support

- **linux/amd64**: Fully supported (default)
- **linux/arm64**: Supported (via manual trigger with multi-arch option)

### Single-Platform Builds (Default)

For faster builds, the automatic nightly builds use `linux/amd64` only.

### Multi-Platform Builds (Optional)

To build for multiple architectures, trigger manually and select the multi-arch option. This takes longer but produces images for both AMD64 and ARM64.

## Integration with Release Pipeline

Nightly builds are separate from release builds:

- **Nightly builds**: Development snapshots, tagged with `nightly`
- **Release builds**: Stable releases, triggered by version tags (e.g., `v1.3.0`)
- **Release workflow**: `.github/workflows/release.yml`

## Best Practices

1. **Using Nightly Builds**
   - Use for testing latest features
   - Not recommended for production deployments
   - May contain unstable or experimental code

2. **Updating from Nightly**
   - Always pull the latest before testing: `docker pull themisdb/server:nightly`
   - Check build date to ensure you have the latest version
   - Review the CHANGELOG.md for recent changes

3. **Reporting Issues**
   - Include the nightly build date/tag in bug reports
   - Provide relevant logs from the container
   - Check if the issue exists in the latest nightly before reporting

## See Also

- [Docker Multi-Arch Deployment](deployment_docker_multiarch.md)
- [CI/CD Multi-Arch Strategy](deployment_cicd_multiarch.md)
- [Build Strategy Guide](../guides/guides_build_strategy.md)
- [Deployment Strategy](deployment_strategy.md)
