# Docker Security Audit Report
**Date:** 2026-01-08  
**Version:** ThemisDB 1.4.0  
**Auditor:** Automated Security Review

## Executive Summary

This document provides a comprehensive security audit of all Docker configurations in the ThemisDB repository. The audit identified several security issues ranging from high to low severity, and provides recommendations for remediation.

## Audit Scope

- All Dockerfiles in `/docker` directory (20+ files)
- Docker Compose files across the repository
- Entrypoint scripts and shell scripts
- Base images and their dependencies
- Container runtime configurations

## Vulnerabilities Identified and Fixed

### ✅ FIXED: High Severity Issues

#### 1. Privilege Escalation Risk in Entrypoint Script
**Status:** FIXED  
**File:** `docker/entrypoint.sh`  
**Issue:** Script created directories with overly permissive 0775 permissions  

**Before:**
```bash
chmod 0775 /data /data/themis_server /data/vector_indexes /var/log/themis || true
```

**After:**
```bash
# Use 0750 permissions (rwxr-x---) to prevent world access
chmod 0750 /data /data/themis_server /data/vector_indexes /var/log/themis || true
```

**Impact:** Prevents unauthorized users from accessing or modifying data directories.

#### 2. Container Running as Root in Docker Compose
**Status:** FIXED  
**File:** `docker/docker-compose.qnap.yml`  
**Issue:** Container explicitly configured to run as root (user: "0:0")

**Before:**
```yaml
user: "0:0"  # Running as root
```

**After:**
```yaml
user: "999:999"  # Running as themis user
```

**Impact:** Follows principle of least privilege, reduces attack surface.

### ✅ FIXED: Medium Severity Issues

#### 3. Missing Non-Root User in Runtime Stage
**Status:** FIXED  
**Files:**
- `docker/Dockerfile.optimized-local`
- `docker/Dockerfile.minimal`
- `docker/Dockerfile.benchmark`

**Issue:** Containers ran as root by default

**Fix Applied:**
```dockerfile
# Create non-root user for security
RUN groupadd -r themis --gid=999 && \
    useradd -r -g themis --uid=999 --home-dir=/data --shell=/bin/bash themis

# Run as non-root user
USER themis
```

**Impact:** All containers now run as non-root user by default.

#### 4. Outdated Base Image
**Status:** FIXED  
**File:** `docker/Dockerfile.qnap`  
**Issue:** Used Ubuntu 20.04 which approaches end-of-life

**Before:**
```dockerfile
FROM ubuntu:20.04 AS build
```

**After:**
```dockerfile
FROM ubuntu:22.04 AS build
```

**Impact:** Reduces exposure to unpatched vulnerabilities in older base images.

## Remaining Security Considerations

### ℹ️ Information: Health Check HTTP Usage
**Status:** ACCEPTED (NOT A VULNERABILITY)  
**Location:** Multiple Dockerfiles  
**Details:** Health checks use HTTP to localhost

```dockerfile
HEALTHCHECK --interval=30s --timeout=5s --start-period=10s --retries=3 \
  CMD curl -f http://localhost:8080/health || exit 1
```

**Reasoning:** Using HTTP for localhost health checks is acceptable as:
- Traffic never leaves the container
- Health endpoint should be publicly accessible for monitoring
- TLS adds unnecessary overhead for localhost communication
- Industry standard practice (Kubernetes, Docker Swarm, etc.)

**Note:** External health checks and monitoring should use HTTPS when available.

### ℹ️ Information: Python Base Image
**Status:** ACCEPTABLE  
**File:** `scripts/railway/Dockerfile.simulator`  
**Details:** Uses `python:3.12-slim` without digest pinning

**Note:** While version-specific tags (e.g., `3.12.12-slim`) provide better reproducibility, using `python:3.12-slim` is acceptable for non-production images. The official Python images are well-maintained and regularly updated.

**Recommendation:** For production deployments, consider pinning to specific versions or digests.

## Security Best Practices Observed

The audit found that ThemisDB follows many Docker security best practices:

✅ **Multi-stage builds** - All production Dockerfiles use multi-stage builds to minimize final image size  
✅ **Security updates** - Base images updated with `apt-get upgrade -y`  
✅ **Non-root user** - Runtime containers use dedicated `themis` user (UID 999)  
✅ **Minimal dependencies** - Only necessary runtime dependencies installed  
✅ **Clean layers** - APT lists and temporary files cleaned after installation  
✅ **HTTPS for downloads** - External resources fetched over HTTPS  
✅ **No hardcoded secrets** - No credentials found in Dockerfiles  
✅ **Health checks** - All production images include health checks  
✅ **Proper file permissions** - Files have appropriate ownership and permissions  
✅ **Volume declarations** - Data volumes properly declared  
✅ **.dockerignore present** - Build context properly filtered  
✅ **Pinned base images** - Using specific Ubuntu versions (24.04, 22.04)

## Additional Recommendations

While not critical, the following improvements are recommended for enhanced security:

### High Priority
1. **Add vulnerability scanning to CI/CD pipeline**
   - Use tools like Trivy, Snyk, or Docker Scout
   - Scan images before pushing to registry
   - Block deployments with critical vulnerabilities

2. **Implement image signing**
   - Sign production images with Docker Content Trust
   - Verify signatures before deployment

### Medium Priority
3. **Document exposed ports**
   - Create documentation explaining the purpose of each exposed port
   - Include firewall recommendations

4. **Add runtime security**
   - Consider using AppArmor/SELinux profiles
   - Document seccomp profiles for containers

5. **Secret management**
   - Document use of Docker secrets or external secret management
   - Provide examples for Kubernetes secrets integration

### Low Priority
6. **Consider distroless images**
   - Evaluate Google's distroless base images for smaller attack surface
   - May require significant refactoring

7. **Implement read-only root filesystem**
   - Where possible, run containers with read-only root filesystem
   - Use tmpfs for necessary writable paths

## Testing Performed

The following validation was performed after fixes:

- ✅ All Dockerfiles parse correctly
- ✅ Multi-stage builds maintain proper structure
- ✅ Non-root user declarations are valid
- ✅ File permissions are appropriate
- ✅ No syntax errors in shell scripts

## Compliance

The Docker configurations align with:
- CIS Docker Benchmark guidelines
- OWASP Docker Security Cheat Sheet
- Docker Security Best Practices
- Principle of Least Privilege

## Conclusion

The ThemisDB Docker infrastructure demonstrates a strong security posture with good adherence to security best practices. The critical issues identified during this audit have been resolved:

- **4 files modified** to fix security issues
- **0 critical vulnerabilities** remaining
- **0 high severity issues** remaining
- **0 medium severity issues** remaining

The remaining items are either accepted risks (localhost HTTP health checks) or recommendations for future enhancements (CI/CD scanning, image signing).

## Change Log

### 2026-01-08 - Security Fixes Applied
- Fixed directory permissions in entrypoint.sh (0775 → 0750)
- Added USER directive to 3 Dockerfiles missing it
- Changed docker-compose.qnap.yml to run as non-root user
- Updated Dockerfile.qnap base image from Ubuntu 20.04 to 22.04
- Added security documentation

## References

- [CIS Docker Benchmark](https://www.cisecurity.org/benchmark/docker)
- [OWASP Docker Security Cheat Sheet](https://cheatsheetseries.owasp.org/cheatsheets/Docker_Security_Cheat_Sheet.html)
- [Docker Security Best Practices](https://docs.docker.com/develop/security-best-practices/)
- [NIST Application Container Security Guide](https://nvlpubs.nist.gov/nistpubs/SpecialPublications/NIST.SP.800-190.pdf)

---
**Last Updated:** 2026-04-06  
**Next Review:** Recommended within 6 months or upon significant changes to Docker configurations
