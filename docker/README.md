# ThemisDB LoRA Framework - Docker Compose Setup

> **⚠️ Hotfix notice (2026-04-08):** Tag `1.8.1-rc1` crashes at startup
> with `Exit 139` (SIGSEGV in `rocksdb::ImmutableDBOptions::Dump`). The fix is
> released in `themisdb/themisdb:1.8.1-rc2-community-binary-x64` (canonical)
> and `themisdb/themisdb:latest` (stable alias); use that tag or newer.
> See [CHANGELOG.md](../CHANGELOG.md) for the full list of fixes applied.

Complete Docker Compose environment for developing and testing the ThemisDB LoRA framework.

## 🚀 Quick Start (< 5 minutes)

### Prerequisites

- **Docker** ≥ 20.10
- **Docker Compose** ≥ 2.0
- **Hardware**: 4GB RAM minimum, 8GB recommended

### Start the Environment

```bash
# Clone the repository (if not already done)
git clone https://github.com/makr-code/ThemisDB.git
cd ThemisDB/docker

# Start all services
./scripts/start.sh
```

That's it! The environment is now running.

## 📍 Access URLs

Once started, access these services:

| Service | URL | Credentials |
|---------|-----|-------------|
| **ThemisDB API** | http://localhost:8529 | None (dev mode) |
| **Prometheus** | http://localhost:9091 | None |
| **Grafana** | http://localhost:3000 | admin / admin |

## 🏗️ Architecture

The Docker Compose setup includes:

### Services

1. **themisdb** - Main ThemisDB instance with LoRA framework
   - Ports: 8529 (HTTP), 8765 (AQL), 9090 (metrics)
   - Built from `Dockerfile.themisdb`
   - Persistent volumes for data and logs

2. **prometheus** - Metrics collection and monitoring
   - Port: 9091 (offset to avoid conflict)
   - Scrapes metrics from ThemisDB every 15s
   - 30-day retention by default

3. **grafana** - Metrics visualization
   - Port: 3000
   - Pre-configured with Prometheus datasource
   - Pre-loaded LoRA framework dashboard

4. **test-db** - Isolated test database instance
   - Port: 8530
   - Used for integration tests
   - Separate data volume

5. **vcpkg-builder** - Dependency cache service
   - Caches vcpkg packages across builds
   - Speeds up rebuild times

### Network

All services run on a private bridge network (`themis-network`) with subnet `172.28.0.0/16`.

### Volumes

Persistent data stored in Docker volumes:
- `themisdb-data` - Database files
- `themisdb-logs` - Application logs
- `prometheus-data` - Metrics history
- `grafana-data` - Dashboards and settings
- `vcpkg-cache` - Build dependencies cache

## 🎯 Usage Modes

### Production Mode (Default)

Start with optimized builds and production settings:

```bash
./scripts/start.sh prod
```

### Development Mode

Start with debug builds, hot-reload, and development tools:

```bash
./scripts/start.sh dev
```

Features:
- Source code mounted for live editing
- Debug symbols enabled
- AddressSanitizer for memory debugging
- Additional debugging ports exposed
- Development tools container included

Access development container:
```bash
docker-compose exec dev-tools bash
```

### Test Mode

Run automated tests in isolated environment:

```bash
./scripts/start.sh test
```

Features:
- Runs all LoRA framework tests
- Uses ephemeral storage (tmpfs)
- Exits automatically after tests
- Resource limits for CI environments

## 📝 Configuration Files

### ThemisDB Configuration

Edit `config/themisdb.yaml` to customize:
- Server ports and networking
- Storage and database settings
- LoRA framework parameters
- Logging levels
- Performance tuning

Example:
```yaml
llm:
  lora:
    enabled: true
    adapters:
      max_loaded: 5
      cache_size_mb: 1024
```

### Prometheus Configuration

Edit `prometheus/prometheus.yml` to:
- Add scrape targets
- Configure retention policies
- Set up alerting rules

### Grafana Dashboards

Add custom dashboards to `grafana/dashboards/`. They will be auto-loaded on startup.

## 🛠️ Common Operations

### View Logs

```bash
# All services
./scripts/logs.sh

# Specific service
./scripts/logs.sh themisdb

# Follow logs
./scripts/logs.sh -f
```

### Stop Services

```bash
# Stop services (preserve data)
./scripts/stop.sh

# Stop and remove all data
./scripts/stop.sh --clean
```

### Reset Environment

```bash
# WARNING: Deletes all data!
./scripts/reset.sh
```

### Run Tests

```bash
./scripts/test.sh
```

### Rebuild Services

```bash
# Rebuild specific service
docker-compose build themisdb

# Rebuild all services
docker-compose build

# Rebuild and restart
docker-compose up -d --build
```

### Execute Commands in Container

```bash
# ThemisDB shell
docker-compose exec themisdb bash

# Run tests manually
docker-compose exec themisdb /usr/local/bin/test_lora_framework

# Run benchmarks
docker-compose exec themisdb /usr/local/bin/bench_lora_framework
```

## 📊 Monitoring

### Grafana Dashboards

Access Grafana at http://localhost:3000 (admin/admin).

Pre-loaded dashboards:
- **LoRA Framework Overview** - Key metrics and performance
  - Adapter load times
  - Memory usage
  - Cache hit rates
  - Inference latency

### Prometheus Metrics

Access Prometheus at http://localhost:9091.

Key LoRA metrics:
- `lora_adapter_load_time_seconds` - Time to load adapters
- `lora_adapter_memory_usage_bytes` - Memory per adapter
- `lora_inference_latency_milliseconds` - Inference time
- `lora_cache_hit_rate` - Cache efficiency
- `lora_adapters_loaded_count` - Number of loaded adapters
- `lora_training_progress` - Training job progress

## 🔧 Troubleshooting

### Services Won't Start

**Check ports are available:**
```bash
# Check if ports are in use
lsof -i :8529
lsof -i :9091
lsof -i :3000

# Stop conflicting services or change ports in docker-compose.yml
```

**Check Docker resources:**
```bash
# Ensure Docker has enough resources (Settings > Resources)
# Minimum: 4GB RAM, 2 CPUs
```

### Build Failures

**Clear Docker cache:**
```bash
docker-compose build --no-cache
docker system prune -a
```

**Check vcpkg cache:**
```bash
# Remove vcpkg cache if corrupted
docker volume rm themisdb-lora_vcpkg-cache
docker volume rm themisdb-lora_vcpkg-downloads
```

### Services Crash or Restart

**Check logs:**
```bash
./scripts/logs.sh themisdb
```

**Check resource usage:**
```bash
docker stats
```

**Increase memory limits:**
Edit `docker-compose.yml` and increase memory limits.

### Grafana Dashboard Not Loading

**Re-provision dashboards:**
```bash
docker-compose restart grafana

# Or manually import from grafana/dashboards/
```

### Database Connection Errors

**Verify service is healthy:**
```bash
docker-compose ps
curl http://localhost:8529/health
```

**Wait for startup:**
ThemisDB needs 30-40 seconds to fully initialize.

### Test Failures

**Run tests with verbose output:**
```bash
docker-compose exec themisdb /usr/local/bin/test_lora_framework --gtest_verbose=true
```

**Check test database:**
```bash
curl http://localhost:8530/health
```

## ⚡ Performance Optimization

### Build Performance

1. **Use cached builds:**
   ```bash
   # vcpkg cache is preserved across builds
   docker-compose build  # Fast after first build
   ```

2. **Parallel builds:**
   ```bash
   # Already configured in Dockerfile with -j$(nproc)
   ```

3. **Layer caching:**
   ```bash
   # Dockerfile uses multi-stage builds for optimal caching
   ```

### Runtime Performance

1. **Allocate more resources:**
   Edit `.env` or `docker-compose.yml`:
   ```yaml
   deploy:
     resources:
       limits:
         cpus: '4'
         memory: 8G
   ```

2. **Use SSD for volumes:**
   Docker volumes should be on SSD for best performance.

3. **Tune ThemisDB settings:**
   Edit `config/themisdb.yaml`:
   ```yaml
   performance:
     thread_pool_size: 8
     query_cache_size_mb: 512
   ```

## 🚢 Production Deployment

### Important Notes

⚠️ **This setup is designed for development and testing, not production!**

For production deployment:

1. **Use proper secrets management** (not hardcoded passwords)
2. **Enable authentication and TLS**
3. **Configure proper resource limits**
4. **Set up monitoring and alerting**
5. **Use orchestration (Kubernetes, Docker Swarm)**
6. **Configure backups and disaster recovery**

### Production Checklist

- [ ] Change default passwords
- [ ] Enable authentication (Grafana, ThemisDB)
- [ ] Configure TLS/SSL certificates
- [ ] Set up proper logging and log rotation
- [ ] Configure resource limits and health checks
- [ ] Set up monitoring and alerting
- [ ] Configure automated backups
- [ ] Test disaster recovery procedures
- [ ] Set up proper networking and firewalls
- [ ] Review security best practices

## 🔗 Related Documentation

- [LoRA Build Guide](../LORA_BUILD_GUIDE.md) - Building without Docker
- [LoRA Testing Guide](../LORA_TESTING_AND_METRICS_GUIDE.md) - Detailed testing
- [LoRA Usage Examples](../LORA_USAGE_EXAMPLES.md) - API usage
- [Docker Build Strategy](./DOCKER_BUILD_STRATEGY_QUICKREF.md) - Build details

## 🤝 Contributing

When making changes:

1. Test with all three modes (prod, dev, test)
2. Update this README if adding new features
3. Validate configurations before committing
4. Keep Docker images lean and efficient

## 📄 License

This Docker setup is part of ThemisDB and follows the same license (MIT).

## 💬 Support

- **Issues**: [GitHub Issues](https://github.com/makr-code/ThemisDB/issues)
- **Discussions**: [GitHub Discussions](https://github.com/makr-code/ThemisDB/discussions)
- **Documentation**: [ThemisDB Docs](https://github.com/makr-code/ThemisDB/tree/main/docs)

---

**Happy Developing! 🚀**
