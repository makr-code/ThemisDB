# ThemisDB Nightly Builds - Quick Reference

## 🚀 Quick Start

### Pull Latest Nightly

```bash
docker pull themisdb/server:nightly
```

### Run Nightly

```bash
docker run -d -p 18765:18765 -v themisdb-data:/data themisdb/server:nightly
```

## 📋 Available Tags

| Tag | Description | Example |
|-----|-------------|---------|
| `nightly` | Latest nightly build | `themisdb/server:nightly` |
| `nightly-YYYYMMDD` | Date-specific build | `themisdb/server:nightly-20231221` |
| `VERSION-nightly` | Version-specific nightly | `themisdb/server:1.3.0-nightly` |

## ⏰ Build Schedule

- **Runs**: Daily at 2:00 AM UTC
- **Duration**: 30-60 minutes
- **Output**: Docker images on DockerHub

## 📚 Documentation

| Document | Purpose | Language |
|----------|---------|----------|
| [deployment_nightly_builds.md](deployment_nightly_builds.md) | Complete technical guide | English |
| [deployment_nightly_builds_de.md](deployment_nightly_builds_de.md) | Vollständige Anleitung | Deutsch |
| [SETUP_NIGHTLY_BUILDS.md](SETUP_NIGHTLY_BUILDS.md) | Setup instructions | Deutsch |
| [OVERNIGHT_BUILDS_SUMMARY.md](OVERNIGHT_BUILDS_SUMMARY.md) | Implementation details | English |

## 🔧 Configuration (For Maintainers)

### Required Secrets

```yaml
DOCKER_USERNAME: your-dockerhub-username
DOCKER_TOKEN: your-dockerhub-access-token
```

### Manual Trigger

1. Go to Actions → "Nightly Build & DockerHub Push"
2. Click "Run workflow"
3. Configure:
   - ✅ Push to DockerHub
   - Platform: `linux/amd64` or `linux/amd64,linux/arm64`
   - LLM Support: Enable/Disable

## 🐛 Troubleshooting

### Image not found

```bash
# Verify tag exists
docker search themisdb/server
# Or check DockerHub
# https://hub.docker.com/r/themisdb/server/tags
```

### Pull fails

```bash
# Clean local images
docker system prune -a
# Retry pull
docker pull themisdb/server:nightly
```

### Build failed

1. Check [Actions tab](../../actions) for workflow status
2. Review build logs for errors
3. Verify secrets are configured correctly
4. Check DockerHub account permissions

## 💡 Use Cases

### Testing Latest Features

```bash
# Pull latest nightly
docker pull themisdb/server:nightly

# Run with test data
docker run -d \
  -p 18765:18765 \
  -v test-data:/data \
  --name themisdb-test \
  themisdb/server:nightly

# Check logs
docker logs -f themisdb-test
```

### Reproducing Issues

```bash
# Use specific date tag
docker pull themisdb/server:nightly-20231221

# Run and reproduce
docker run -it --rm \
  -p 18765:18765 \
  themisdb/server:nightly-20231221
```

### Docker Compose

```yaml
version: '3.8'
services:
  themisdb:
    image: themisdb/server:nightly
    ports:
      - "18765:18765"
      - "8080:8080"
    volumes:
      - themisdb-data:/data
    restart: unless-stopped

volumes:
  themisdb-data:
```

## ⚠️ Important Notes

- **Not for production**: Nightly builds may contain unstable code
- **Daily updates**: Tag `nightly` is updated daily
- **7-day retention**: Build artifacts kept for 7 days
- **Best effort**: No SLA or support guarantees

## 🔗 Links

- **DockerHub**: https://hub.docker.com/r/themisdb/server
- **Repository**: https://github.com/makr-code/ThemisDB
- **Issues**: https://github.com/makr-code/ThemisDB/issues
- **Actions**: https://github.com/makr-code/ThemisDB/actions

## 📝 Changelog

Check nightly changes:
```bash
# View container version
docker run --rm themisdb/server:nightly --version

# Check repository CHANGELOG
# https://github.com/makr-code/ThemisDB/blob/main/CHANGELOG.md
```

## 🆘 Support

For questions or issues:
1. Check [documentation](deployment_nightly_builds.md)
2. Search [existing issues](https://github.com/makr-code/ThemisDB/issues)
3. Create new issue with:
   - Nightly tag/date used
   - Error messages/logs
   - Steps to reproduce

---

**Last Updated**:  April 2026
**Workflow**: `.github/workflows/nightly-build.yml`
