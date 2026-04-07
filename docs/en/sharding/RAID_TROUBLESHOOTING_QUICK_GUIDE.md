# ThemisDB RAID Quick Troubleshooting Guide

## Purpose
Fast reference guide for diagnosing and fixing common RAID cluster issues.

---

## 🚨 Quick Diagnostics

### Check Cluster Health (30 seconds)

```bash
# 1. Check all containers are running
docker ps --filter "name=themis-raid" --format "table {{.Names}}\t{{.Status}}\t{{.Ports}}"

# 2. Check shard health endpoints
for port in 8080 8081 8082 8083 8084 8085 8086 8087; do
  echo "Port $port: $(curl -s http://localhost:$port/health | jq -r .status 2>/dev/null || echo 'FAILED')"
done

# 3. Check Prometheus targets
curl -s http://localhost:9090/api/v1/targets | \
  jq '.data.activeTargets[] | {job: .labels.job, health: .health}'

# 4. Check container logs for errors
docker-compose -f docker/compose/docker-compose-sharding.yml logs --tail=20 | grep -i error
```

---

## ❌ Common Issues

### Issue 1: Container Fails to Start

**Symptoms:**
- Container exits immediately
- "Exec format error" in logs
- REST API not responding

**Quick Fix:**
```bash
# Check if image is correct
docker inspect themisdb/themisdb:latest | grep Architecture

# Expected: "Architecture": "amd64" or "linux/amd64"
# If Windows: "Architecture": "windows/amd64" ❌

# Solution: Rebuild with Linux target
docker build -f Dockerfile.themis-metrics-enabled \
  -t themisdb/themisdb:metrics-enabled .

# Update docker-compose to use new image
sed -i 's/themisdb:latest/themisdb:metrics-enabled/g' \
  docker/compose/docker-compose-sharding.yml
```

---

### Issue 2: Metrics Endpoint Returns 404

**Symptoms:**
- `curl http://localhost:8080/metrics` → 404
- Prometheus targets show "down"
- Grafana shows "No Data"

**Quick Fix:**
```bash
# 1. Verify metrics are enabled
docker exec themis-raid0-shard1 env | grep THEMIS_ENABLE_METRICS
# Should show: THEMIS_ENABLE_METRICS=true

# 2. Check which port is actually serving metrics
docker exec themis-raid0-shard1 netstat -tlnp | grep -E '(8080|9090)'

# 3. Test internal metrics endpoint
docker exec themis-raid0-shard1 curl -s http://localhost:8080/metrics

# 4. If still failing, rebuild with metrics support
docker build -f Dockerfile.themis-metrics-enabled \
  -t themisdb/themisdb:metrics-enabled .
docker-compose -f docker/compose/docker-compose-sharding.yml up -d --force-recreate
```

---

### Issue 3: Prometheus Cannot Scrape Targets

**Symptoms:**
- All Prometheus targets show "down"
- Error: "connection refused" or "timeout"

**Quick Fix:**
```bash
# 1. Verify Prometheus can reach shards
docker exec themis-prometheus ping -c 2 themis-raid0-shard1

# 2. Check Prometheus configuration
docker exec themis-prometheus cat /etc/prometheus/prometheus.yml

# 3. Fix port in prometheus.yml (should be 8080, not 9090)
cat > docker/compose/prometheus.yml << 'EOF'
scrape_configs:
  - job_name: 'raid0-stripe'
    static_configs:
      - targets:
        - 'themis-raid0-shard1:8080'
        - 'themis-raid0-shard2:8080'
        - 'themis-raid0-shard3:8080'
    metrics_path: '/metrics'
    scrape_interval: 15s
EOF

# 4. Restart Prometheus
docker-compose -f docker/compose/docker-compose-sharding.yml restart prometheus
```

---

### Issue 4: Grafana Shows No Data

**Symptoms:**
- Grafana dashboard loads but panels empty
- "No data" or "N/A" in panels

**Quick Fix:**
```bash
# 1. Check Grafana can reach Prometheus
docker exec themis-grafana wget -qO- http://prometheus:9090/api/v1/query?query=up

# 2. Test Prometheus has data
curl -s 'http://localhost:9090/api/v1/query?query=themis_raid_io_bytes_total' | jq .

# 3. Check data source configuration
curl -s http://admin:admin@localhost:3000/api/datasources | jq .

# 4. Manually test query in Grafana:
# - Open http://localhost:3000
# - Go to Explore
# - Query: up{job="raid0-stripe"}
# - Should show time series data

# 5. If still no data, reload datasource
curl -X POST http://admin:admin@localhost:3000/api/admin/provisioning/datasources/reload
```

---

### Issue 5: Shard Cannot Connect to Peers

**Symptoms:**
- "Unable to connect to peer" in logs
- "Connection refused" to other shards
- Cluster initialization fails

**Quick Fix:**
```bash
# 1. Check all shards are in same network
docker network inspect themis-network | \
  jq '.Containers | to_entries[] | {name: .value.Name, ip: .value.IPv4Address}'

# 2. Test connectivity between shards
docker exec themis-raid0-shard1 \
  curl -s http://themis-raid0-shard2:18765/health || echo "Cannot connect"

# 3. Verify THEMIS_SHARDS configuration
docker exec themis-raid0-shard1 env | grep THEMIS_SHARDS

# 4. Check if peer is actually listening
docker exec themis-raid0-shard2 netstat -tlnp | grep 18765

# 5. Restart cluster in order
docker-compose -f docker/compose/docker-compose-sharding.yml down
docker-compose -f docker/compose/docker-compose-sharding.yml up -d
```

---

### Issue 6: RAID1 Replication Lag

**Symptoms:**
- Primary and secondary out of sync
- High replication lag metrics
- Data inconsistency

**Quick Fix:**
```bash
# 1. Check replication lag
curl -s http://localhost:8083/metrics | grep replication_lag

# 2. Check network bandwidth
docker stats themis-raid1-primary themis-raid1-secondary --no-stream

# 3. Increase network buffer size
docker-compose -f docker/compose/docker-compose-sharding.yml down
# Edit docker-compose-sharding.yml:
# Add: THEMIS_NETWORK_BUFFER_SIZE: "268435456"  # 256 MB
docker-compose -f docker/compose/docker-compose-sharding.yml up -d

# 4. Force resync if necessary
curl -X POST http://localhost:8083/api/v1/admin/force-sync
```

---

### Issue 7: RAID5 Degraded Performance

**Symptoms:**
- Slow read operations
- High CPU usage on remaining shards
- "Degraded mode" in logs

**Quick Fix:**
```bash
# 1. Identify failed shard
curl -s http://localhost:9090/api/v1/query?query=themis_shard_health_status | \
  jq '.data.result[] | select(.value[1]=="0")'

# 2. Check if rebuild is in progress
docker logs themis-raid5-shard1 | grep -i rebuild

# 3. Start rebuild process (if not automatic)
curl -X POST http://localhost:8085/api/v1/admin/rebuild-shard \
  -d '{"failed_shard_id": "raid5-2"}'

# 4. Monitor rebuild progress
watch -n 5 'curl -s http://localhost:8085/metrics | grep rebuild_progress'
```

---

## 🔍 Diagnostic Commands

### Container Diagnostics
```bash
# View detailed container info
docker inspect themis-raid0-shard1 | jq .

# Check resource usage
docker stats themis-raid0-shard1 --no-stream

# View full logs
docker logs themis-raid0-shard1 --tail 100 --follow

# Execute shell in container
docker exec -it themis-raid0-shard1 /bin/bash

# Check listening ports
docker exec themis-raid0-shard1 netstat -tlnp
```

### Network Diagnostics
```bash
# List Docker networks
docker network ls

# Inspect network
docker network inspect themis-network

# Test DNS resolution
docker exec themis-raid0-shard1 nslookup themis-raid0-shard2

# Test connectivity
docker exec themis-raid0-shard1 ping -c 3 themis-raid0-shard2

# Test HTTP endpoint
docker exec themis-raid0-shard1 \
  curl -v http://themis-raid0-shard2:8080/health
```

### Metrics Diagnostics
```bash
# Check metrics endpoint
curl http://localhost:8080/metrics

# Query specific metric
curl -s http://localhost:8080/metrics | grep themis_raid_io_bytes_total

# Query Prometheus directly
curl -s 'http://localhost:9090/api/v1/query?query=up' | jq .

# Check Prometheus targets
curl -s http://localhost:9090/api/v1/targets | jq .

# Test Grafana API
curl -s http://admin:admin@localhost:3000/api/health
```

---

## 🛠️ Quick Fixes

### Reset Entire Cluster
```bash
# Stop and remove everything
docker-compose -f docker/compose/docker-compose-sharding.yml down -v

# Remove dangling volumes (CAUTION: Data loss!)
docker volume prune -f

# Restart fresh
docker-compose -f docker/compose/docker-compose-sharding.yml up -d

# Wait for initialization
sleep 30

# Verify health
./scripts/verify-cluster-health.sh
```

### Rebuild Single Shard
```bash
# Stop shard
docker-compose -f docker/compose/docker-compose-sharding.yml \
  stop themis-raid0-shard2

# Remove volume (CAUTION: Data loss!)
docker volume rm themis-raid0-shard2-data

# Restart shard
docker-compose -f docker/compose/docker-compose-sharding.yml \
  up -d themis-raid0-shard2

# Monitor recovery
docker logs -f themis-raid0-shard2
```

### Force Prometheus Reload
```bash
# Reload configuration
docker exec themis-prometheus kill -HUP 1

# Or restart Prometheus
docker-compose -f docker/compose/docker-compose-sharding.yml \
  restart prometheus
```

### Clear Grafana Cache
```bash
# Restart Grafana
docker-compose -f docker/compose/docker-compose-sharding.yml \
  restart grafana

# Or clear cache manually
docker exec themis-grafana rm -rf /var/lib/grafana/cache/*
docker-compose -f docker/compose/docker-compose-sharding.yml \
  restart grafana
```

---

## 📊 Monitoring Queries

### Prometheus Queries

```promql
# Check if all shards are up
up{job=~"raid.*"}

# RAID I/O throughput
rate(themis_raid_io_bytes_total[5m])

# Operation latency (95th percentile)
histogram_quantile(0.95, rate(themis_operation_duration_seconds_bucket[5m]))

# Shard health status
themis_shard_health_status

# Operations per second
rate(themis_io_operations_total[1m])

# Replication lag (RAID1)
themis_replication_lag_seconds

# RAID5 rebuild progress
themis_raid5_rebuild_progress
```

---

## 🔧 Configuration Validation

### Validate Docker Compose
```bash
# Check syntax
docker-compose -f docker/compose/docker-compose-sharding.yml config

# Validate configuration
docker-compose -f docker/compose/docker-compose-sharding.yml config --quiet
```

### Validate Prometheus Config
```bash
# Check syntax
docker exec themis-prometheus promtool check config \
  /etc/prometheus/prometheus.yml

# Test scrape config
docker exec themis-prometheus promtool check scrape-config \
  /etc/prometheus/prometheus.yml
```

### Validate Environment Variables
```bash
# Check all required variables are set
for shard in raid0-shard1 raid0-shard2 raid0-shard3; do
  echo "=== $shard ==="
  docker exec themis-$shard env | grep THEMIS_ | sort
done
```

---

## 📞 Getting Help

### Log Collection for Bug Reports
```bash
# Collect all logs
mkdir -p /tmp/themis-debug
docker-compose -f docker/compose/docker-compose-sharding.yml logs \
  > /tmp/themis-debug/docker-compose-logs.txt

# Collect container info
docker ps -a > /tmp/themis-debug/containers.txt

# Collect network info
docker network inspect themis-network \
  > /tmp/themis-debug/network.json

# Collect metrics snapshot
curl -s http://localhost:9090/api/v1/targets \
  > /tmp/themis-debug/prometheus-targets.json

# Create archive
tar -czf themis-debug-$(date +%Y%m%d-%H%M%S).tar.gz \
  -C /tmp themis-debug/
```

### Support Channels
- GitHub Issues: https://github.com/makr-code/ThemisDB/issues
- Documentation: https://themisdb.readthedocs.io/
- Community: https://discord.gg/themisdb

---

## 📚 Related Documentation

- [GITHUB_ISSUE_RAID_SETUP.md](GITHUB_ISSUE_RAID_SETUP.md) - Detailed issue description
- [RAID_SHARD_REFERENCING_ARCHITECTURE.md](RAID_SHARD_REFERENCING_ARCHITECTURE.md) - Architecture details
- [PROMETHEUS_INTEGRATION_COMPLETE.md](../PROMETHEUS_INTEGRATION_COMPLETE.md) - Metrics setup
- [DOCKER_RAID_IMPLEMENTATION_SUMMARY.md](../benchmarks/DOCKER_RAID_IMPLEMENTATION_SUMMARY.md) - Implementation details

---

**Last Updated:** 2026-04-06  
**Version:** 1.0  
**Status:** Active

