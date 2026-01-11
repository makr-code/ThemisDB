---
name: LoRA Framework - Docker Compose Development Environment
about: Add Docker Compose configuration for LoRA framework development and testing
title: '[ENHANCEMENT] Add Docker Compose for LoRA Framework Dev/Test Environment'
labels: ['enhancement', 'infrastructure', 'docker', 'lora-framework']
assignees: ''
---

## Description

Create a comprehensive Docker Compose configuration to provide a complete development and testing environment for the LoRA framework, including all dependencies and monitoring infrastructure.

## Motivation

A Docker Compose setup would:
- Enable quick onboarding for new developers (< 5 minutes)
- Provide consistent development environments across teams
- Include all required services (ThemisDB, Prometheus, Grafana, test databases)
- Support integration testing with real dependencies
- Facilitate CI/CD with containerized builds
- Enable local testing of production-like configurations

## Proposed Solution

Create Docker Compose configuration files in `docker/`:

### Main Configuration
**File**: `docker/docker-compose.yml`

**Services**:
1. **themisdb**: ThemisDB with LoRA framework
2. **prometheus**: Metrics collection
3. **grafana**: Metrics visualization (with pre-loaded dashboards)
4. **test-db**: Test database for integration tests
5. **vcpkg-builder**: Build environment with all dependencies

### Development Configuration
**File**: `docker/docker-compose.dev.yml`
- Hot-reload support for code changes
- Debug builds with symbols
- Volume mounts for local development
- Port forwarding for debugging tools

### Testing Configuration
**File**: `docker/docker-compose.test.yml`
- Minimal services for CI/CD
- Fast startup times
- Ephemeral storage
- Automated test execution

## Implementation Details

### Service Definitions

#### ThemisDB Service
```yaml
themisdb:
  build:
    context: ..
    dockerfile: docker/Dockerfile.themisdb
  ports:
    - "8529:8529"  # HTTP API
    - "9090:9090"  # Metrics
  volumes:
    - themisdb-data:/var/lib/themisdb
    - ./config:/etc/themisdb
  environment:
    - THEMIS_ENABLE_LLM=true
    - THEMIS_LORA_ENABLED=true
  depends_on:
    - prometheus
```

#### Prometheus Service
```yaml
prometheus:
  image: prom/prometheus:latest
  ports:
    - "9091:9090"
  volumes:
    - ./prometheus/prometheus.yml:/etc/prometheus/prometheus.yml
    - prometheus-data:/prometheus
  command:
    - '--config.file=/etc/prometheus/prometheus.yml'
```

#### Grafana Service
```yaml
grafana:
  image: grafana/grafana:latest
  ports:
    - "3000:3000"
  volumes:
    - ./grafana/dashboards:/etc/grafana/provisioning/dashboards
    - ./grafana/datasources:/etc/grafana/provisioning/datasources
    - grafana-data:/var/lib/grafana
  environment:
    - GF_SECURITY_ADMIN_PASSWORD=admin
  depends_on:
    - prometheus
```

### Dockerfiles

#### Dockerfile.themisdb
```dockerfile
FROM ubuntu:22.04
RUN apt-get update && apt-get install -y \
    cmake ninja-build gcc-11 g++-11 \
    libssl-dev zlib1g-dev librocksdb-dev \
    libfmt-dev libspdlog-dev nlohmann-json3-dev
COPY . /app
WORKDIR /app/build
RUN cmake .. -GNinja -DTHEMIS_BUILD_TESTS=ON && ninja
CMD ["./themisdb", "--config", "/etc/themisdb/config.yaml"]
```

### Configuration Files

**`docker/prometheus/prometheus.yml`**:
```yaml
scrape_configs:
  - job_name: 'themisdb-lora'
    static_configs:
      - targets: ['themisdb:9090']
```

**`docker/grafana/datasources/prometheus.yml`**:
```yaml
datasources:
  - name: Prometheus
    type: prometheus
    url: http://prometheus:9090
    isDefault: true
```

## Tasks

### Core Setup
- [ ] Create `docker/docker-compose.yml` with all services
- [ ] Create `docker/docker-compose.dev.yml` for development
- [ ] Create `docker/docker-compose.test.yml` for testing
- [ ] Create `docker/Dockerfile.themisdb` for ThemisDB build
- [ ] Create `docker/.dockerignore` to exclude unnecessary files

### Configuration
- [ ] Add Prometheus configuration (`docker/prometheus/prometheus.yml`)
- [ ] Add Grafana data source provisioning (`docker/grafana/datasources/`)
- [ ] Add Grafana dashboard provisioning (`docker/grafana/dashboards/`)
- [ ] Add ThemisDB configuration (`docker/config/themisdb.yaml`)
- [ ] Add environment variable templates (`.env.example`)

### Scripts
- [ ] Create `docker/scripts/start.sh` - Start all services
- [ ] Create `docker/scripts/stop.sh` - Stop and cleanup
- [ ] Create `docker/scripts/test.sh` - Run integration tests
- [ ] Create `docker/scripts/logs.sh` - View service logs
- [ ] Create `docker/scripts/reset.sh` - Reset all volumes

### Documentation
- [ ] Create `docker/README.md` with usage instructions
- [ ] Add quick start guide (5-minute setup)
- [ ] Document service ports and URLs
- [ ] Add troubleshooting section
- [ ] Include performance tuning tips

### Integration
- [ ] Update `.github/workflows/lora-framework-ci.yml` to use Docker
- [ ] Add Docker-based build job to CI/CD
- [ ] Add Docker-based integration test job
- [ ] Update `LORA_BUILD_GUIDE.md` with Docker instructions

## Acceptance Criteria

- [ ] Complete Docker Compose setup with 5 services
- [ ] One-command startup: `docker-compose up`
- [ ] All services healthy and reachable
- [ ] Grafana accessible at `http://localhost:3000`
- [ ] Prometheus accessible at `http://localhost:9091`
- [ ] ThemisDB accessible at `http://localhost:8529`
- [ ] Pre-loaded Grafana dashboards showing LoRA metrics
- [ ] Volume persistence for data
- [ ] Documentation complete with examples
- [ ] CI/CD integration working

## Documentation Requirements

Create `docker/README.md` covering:
- Prerequisites (Docker, Docker Compose)
- Quick start (< 5 minutes)
- Service URLs and default credentials
- Volume management
- Network configuration
- Development workflow
- Testing workflow
- Troubleshooting common issues
- Performance optimization
- Production deployment considerations

Update existing documentation:
- Add Docker section to `LORA_BUILD_GUIDE.md`
- Update `README.md` with Docker quick start
- Add Docker examples to `LORA_USAGE_EXAMPLES.md`

## Related Files

- `CMakeLists.txt` - Build configuration
- `tests/CMakeLists.txt` - Test configuration
- `.github/workflows/lora-framework-ci.yml` - CI/CD pipeline
- `LORA_BUILD_GUIDE.md` - Build documentation

## References

- Docker Compose documentation: https://docs.docker.com/compose/
- Docker best practices: https://docs.docker.com/develop/dev-best-practices/

## Priority

**Medium** - Improves developer experience but not blocking for production use.

## Estimated Effort

**Medium** (6-8 hours)
- Docker Compose files: 2 hours
- Dockerfiles: 1 hour
- Configuration files: 1 hour
- Scripts: 1 hour
- Testing: 1-2 hours
- Documentation: 2 hours
