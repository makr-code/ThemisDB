# ThemisDB Overnight Builds - Implementation Summary

## Problem Statement (Original in German)

> "Wie funktionieren overnight builds für die ThemisDB inklusive dockerhub push."

**Translation**: "How do overnight builds work for ThemisDB including DockerHub push?"

## Solution Overview

This implementation provides a fully automated overnight build system that:
1. Builds ThemisDB from source every night at 2 AM UTC
2. Creates Docker images with the latest code
3. Pushes images to DockerHub with appropriate tags
4. Supports both single and multi-architecture builds
5. Provides comprehensive monitoring and notifications

## What Was Implemented

### 1. GitHub Actions Workflow (`.github/workflows/nightly-build.yml`)

**Key Features**:
- **Scheduled Execution**: Runs daily at 2:00 AM UTC via cron schedule
- **Manual Triggering**: Can be manually triggered with custom options
- **Four-Phase Process**:
  - Setup: Version detection and build configuration
  - Binary Build: Compile ThemisDB with vcpkg and CMake
  - Docker Build: Create and push Docker images
  - Notification: Report status and provide usage instructions

**Build Optimization**:
- Disk space cleanup (removes ~19 GB of unused tools)
- vcpkg dependency caching (saves ~30-40 minutes on repeated builds)
- Docker layer caching via GitHub Actions cache
- Artifact retention (binaries kept for 7 days)

**Docker Tags Produced**:
- `themisdb/server:nightly` - Always latest nightly
- `themisdb/server:nightly-YYYYMMDD` - Date-specific (e.g., nightly-20231221)
- `themisdb/server:VERSION-nightly` - Version-specific (e.g., 1.3.0-nightly)

### 2. Documentation

**English Documentation** (`docs/deployment/deployment_nightly_builds.md`):
- Complete technical overview
- Build process details
- Usage instructions
- Configuration requirements
- Troubleshooting guide
- Platform support information

**German Documentation** (`docs/deployment/deployment_nightly_builds_de.md`):
- Full documentation in German
- Answers the original question: "Wie funktionieren overnight builds?"
- Detailed explanation of the build process
- Usage examples and best practices

**Setup Guide** (`docs/deployment/SETUP_NIGHTLY_BUILDS.md`):
- Step-by-step setup instructions (in German)
- DockerHub token creation
- GitHub secrets configuration
- Testing and verification procedures
- Troubleshooting common issues

### 3. Updated README Files

**Main README.md**:
- Added nightly build information to Docker installation section
- Included pull commands for nightly images
- Link to full nightly builds documentation

**Workflows README** (`.github/workflows/README.md`):
- Added nightly build to workflow overview table
- Documented schedule and trigger information
- Updated secrets table with Docker credentials
- Added usage examples

## How It Works

### Automatic Overnight Execution

```
Daily at 2:00 AM UTC:
├── Trigger: Cron schedule (0 2 * * *)
├── Phase 1: Setup (1-2 min)
│   ├── Read VERSION file (1.3.0)
│   ├── Generate build date (YYYYMMDD)
│   └── Configure push strategy
│
├── Phase 2: Build Binary (30-40 min)
│   ├── Free disk space (~5 min)
│   ├── Setup vcpkg cache (restore if available)
│   ├── Install system dependencies (~2 min)
│   ├── Configure CMake (~5 min)
│   ├── Build with Ninja (~20-30 min)
│   └── Upload binary artifact (~1 min)
│
├── Phase 3: Docker Build (5-10 min)
│   ├── Download binary artifact
│   ├── Setup QEMU for multi-arch
│   ├── Setup Docker Buildx
│   ├── Login to DockerHub
│   ├── Build image with Dockerfile.simple
│   ├── Push to DockerHub (3 tags)
│   └── Generate build summary
│
└── Phase 4: Notify (1 min)
    ├── Report build status
    └── Provide pull commands

Total Duration: ~35-50 minutes
```

### Manual Triggering

Users can also trigger builds manually via GitHub Actions UI:
- Navigate to Actions → "Nightly Build & DockerHub Push"
- Click "Run workflow"
- Configure options:
  - Push to DockerHub: Yes/No
  - Build platforms: linux/amd64 or linux/amd64,linux/arm64

## Configuration Requirements

### GitHub Secrets (Required)

| Secret | Purpose | Where to Get |
|--------|---------|--------------|
| `DOCKER_USERNAME` | DockerHub username | Your DockerHub account name |
| `DOCKER_TOKEN` | DockerHub access token | https://hub.docker.com/settings/security |

### Setup Instructions

1. **Create DockerHub Access Token**:
   - Go to https://hub.docker.com/settings/security
   - Click "New Access Token"
   - Grant "Read, Write, Delete" permissions
   - Copy the token (shown only once!)

2. **Add GitHub Secrets**:
   - Go to repository Settings → Secrets and variables → Actions
   - Add `DOCKER_USERNAME` with your DockerHub username
   - Add `DOCKER_TOKEN` with the access token from step 1

3. **Verify Setup**:
   - Manually trigger the workflow to test
   - Check DockerHub for the pushed images
   - Monitor the Actions tab for build status

## Usage Examples

### Pull Latest Nightly Build

```bash
docker pull themisdb/server:nightly
```

### Run Nightly Build

```bash
docker run -d \
  -p 8080:8080 \
  -p 18765:18765 \
  -v themisdb-data:/data \
  --name themisdb-nightly \
  themisdb/server:nightly
```

### Use Specific Date

```bash
docker pull themisdb/server:nightly-20231221
docker run -d -p 18765:18765 -v themisdb-data:/data themisdb/server:nightly-20231221
```

## Benefits

### For Developers
- Always have access to latest code without manual builds
- Easy testing of new features in Docker environment
- No need to set up local build environment

### For Testers
- Daily snapshots for continuous testing
- Date-specific tags allow testing specific builds
- Easy to report issues with specific nightly version

### For DevOps
- Automated build process reduces manual work
- Consistent Docker images for deployment
- Cache optimization reduces build times and costs

## Monitoring and Maintenance

### Build Status
- Check GitHub Actions tab for workflow runs
- Email notifications for failed builds (if configured)
- Build summary in workflow run shows all details

### DockerHub
- Images visible at https://hub.docker.com/r/themisdb/server
- Check "Tags" tab for all nightly versions
- Monitor image sizes and push times

### Troubleshooting
- Review workflow logs for build errors
- Check secrets configuration if push fails
- Verify DockerHub account has sufficient permissions
- Clear caches if dependency issues occur

## Architecture Decisions

### Why Dockerfile.simple?
- Faster builds using pre-built binary
- Reduces Docker build time from 30+ minutes to ~5 minutes
- Binary build happens separately with full caching support

### Why 2 AM UTC?
- Overnight for most time zones
- Low GitHub Actions usage (faster runners)
- Results available in morning for US/EU teams

### Why Three Tags?
- `nightly`: Always points to latest (convenience)
- `nightly-YYYYMMDD`: Date-specific (reproducibility)
- `VERSION-nightly`: Version-specific (clarity)

### Why vcpkg Caching?
- Dependency builds take 30-40 minutes
- Cache reduces this to ~5 minutes
- Saves GitHub Actions minutes and costs

## Files Modified/Created

### New Files
1. `.github/workflows/nightly-build.yml` - Main workflow
2. `docs/deployment/deployment_nightly_builds.md` - English documentation
3. `docs/deployment/deployment_nightly_builds_de.md` - German documentation
4. `docs/deployment/SETUP_NIGHTLY_BUILDS.md` - Setup guide

### Modified Files
1. `README.md` - Added nightly build information
2. `.github/workflows/README.md` - Updated workflow overview and secrets

### Existing Files Used
1. `docker/Dockerfile.simple` - Docker image build
2. `docker/entrypoint.sh` - Container entrypoint
3. `config/config.qnap.json` - Default configuration
4. `VERSION` - Version number source

## Testing Recommendations

### Before First Automated Run
1. Manually trigger the workflow with push disabled
2. Verify binary builds successfully
3. Check Docker image builds without errors
4. Enable push and test with actual DockerHub credentials
5. Verify all three tags are created on DockerHub

### Ongoing Monitoring
1. Check workflow runs weekly
2. Pull and test nightly images monthly
3. Review build times for performance regression
4. Monitor disk space usage and cache sizes

## Future Enhancements

Potential improvements for the future:
- Multi-architecture builds by default (linux/amd64, linux/arm64)
- Slack/Discord notifications for build status
- Automated testing after each nightly build
- Performance benchmarking of nightly builds
- Retention policy for old nightly images on DockerHub
- Build matrix for different configurations (with/without LLM, etc.)

## References

- [GitHub Actions Workflow Syntax](https://docs.github.com/en/actions/using-workflows/workflow-syntax-for-github-actions)
- [Docker Build Push Action](https://github.com/docker/build-push-action)
- [DockerHub Access Tokens](https://docs.docker.com/docker-hub/access-tokens/)
- [vcpkg Documentation](https://vcpkg.io/)

## Support

For issues or questions:
- GitHub Issues: https://github.com/makr-code/ThemisDB/issues
- Documentation: `docs/deployment/deployment_nightly_builds.md`
- Setup Guide: `docs/deployment/SETUP_NIGHTLY_BUILDS.md`
