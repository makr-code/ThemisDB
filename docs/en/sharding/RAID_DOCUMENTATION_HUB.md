# ThemisDB RAID Documentation Hub

Welcome to the ThemisDB RAID (Redundant Array of Independent Databases) documentation center. This guide helps you navigate the comprehensive RAID cluster documentation.

---

## 📚 Quick Navigation

### 🆘 Having Problems? Start Here
**→ [RAID_TROUBLESHOOTING_QUICK_GUIDE.md](RAID_TROUBLESHOOTING_QUICK_GUIDE.md)**
- 30-second health checks
- Common issues with immediate fixes
- Quick diagnostic commands
- For: Urgent production issues

### 🏗️ Need Architecture Details?
**→ [RAID_SHARD_REFERENCING_ARCHITECTURE.md](RAID_SHARD_REFERENCING_ARCHITECTURE.md)**
- Complete technical deep-dive
- Shard communication protocols
- Data distribution algorithms
- For: Engineers and architects

### 🐛 Tracking a Known Issue?
**→ [GITHUB_ISSUE_RAID_SETUP.md](GITHUB_ISSUE_RAID_SETUP.md)**
- Comprehensive issue documentation
- Known problems and solutions
- Configuration examples
- For: Issue tracking and resolution

---

## 🎯 Documentation by Use Case

### I Want to...

#### ...Set Up a RAID Cluster
1. Read [RAID_SHARD_REFERENCING_ARCHITECTURE.md](RAID_SHARD_REFERENCING_ARCHITECTURE.md) - Configuration Reference section
2. Review [GITHUB_ISSUE_RAID_SETUP.md](GITHUB_ISSUE_RAID_SETUP.md) - Solutions section
3. Check `../benchmarks/RAID_SHARDING_QUICKSTART.md` for quick start
4. Use `../docker/compose/docker-compose-sharding.yml` as template

#### ...Fix Prometheus Integration
1. Check [RAID_TROUBLESHOOTING_QUICK_GUIDE.md](RAID_TROUBLESHOOTING_QUICK_GUIDE.md) - Issue 2 & 3
2. Read [GITHUB_ISSUE_RAID_SETUP.md](GITHUB_ISSUE_RAID_SETUP.md) - Prometheus Integration Issues
3. Review `../PROMETHEUS_INTEGRATION_COMPLETE.md`
4. Verify `../docker/compose/prometheus.yml` configuration

#### ...Understand Shard Communication
1. Read [RAID_SHARD_REFERENCING_ARCHITECTURE.md](RAID_SHARD_REFERENCING_ARCHITECTURE.md) - Shard Communication Protocols
2. Review Wire Protocol section
3. Check REST API endpoints
4. See examples in architecture diagrams

#### ...Troubleshoot Failover Issues
1. Check [RAID_TROUBLESHOOTING_QUICK_GUIDE.md](RAID_TROUBLESHOOTING_QUICK_GUIDE.md) - Issue 6 & 7
2. Read [RAID_SHARD_REFERENCING_ARCHITECTURE.md](RAID_SHARD_REFERENCING_ARCHITECTURE.md) - Failure Detection and Recovery
3. Review RAID mode-specific failover procedures
4. Test with `../benchmarks/raid_sharding_test_suite.py`

#### ...Optimize Performance
1. Read [RAID_SHARD_REFERENCING_ARCHITECTURE.md](RAID_SHARD_REFERENCING_ARCHITECTURE.md) - Performance Considerations
2. Review configuration tuning options
3. Run benchmarks with `../benchmarks/bench_docker_raid_comprehensive.cpp`
4. Analyze results with `../benchmarks/analyze_raid_benchmarks.py`

---

## 📖 Document Overview

### Primary Documentation

| Document | Size | Purpose | Audience |
|----------|------|---------|----------|
| [RAID_TROUBLESHOOTING_QUICK_GUIDE.md](RAID_TROUBLESHOOTING_QUICK_GUIDE.md) | 11KB | Fast problem resolution | DevOps, SRE |
| [RAID_SHARD_REFERENCING_ARCHITECTURE.md](RAID_SHARD_REFERENCING_ARCHITECTURE.md) | 23KB | Technical deep-dive | Engineers, Architects |
| [GITHUB_ISSUE_RAID_SETUP.md](GITHUB_ISSUE_RAID_SETUP.md) | 21KB | Issue tracking | Project managers, QA |

### Supporting Documentation

| Document | Location | Purpose |
|----------|----------|---------|
| DOCKER_RAID_IMPLEMENTATION_SUMMARY.md | `../benchmarks/` | Implementation details |
| RAID_SHARDING_QUICKSTART.md | `../benchmarks/` | Quick start guide |
| RAID_SHARDING_TEST_PLAN.md | `../benchmarks/` | Testing strategy |
| PROMETHEUS_INTEGRATION_COMPLETE.md | `../` | Metrics setup |
| docker-compose-sharding.yml | `../docker/compose/` | Docker configuration |

---

## 🔍 Key Topics Index

### Architecture
- **RAID Modes**: [Architecture Doc](RAID_SHARD_REFERENCING_ARCHITECTURE.md#raid-modes-and-data-distribution)
  - RAID 0 (Striping)
  - RAID 1 (Mirroring)
  - RAID 5 (Parity)
- **Shard Addressing**: [Architecture Doc](RAID_SHARD_REFERENCING_ARCHITECTURE.md#shard-addressing-and-discovery)
- **Communication Protocols**: [Architecture Doc](RAID_SHARD_REFERENCING_ARCHITECTURE.md#shard-communication-protocols)

### Configuration
- **Environment Variables**: [Architecture Doc](RAID_SHARD_REFERENCING_ARCHITECTURE.md#configuration-reference)
- **Docker Compose**: [Issue Doc](GITHUB_ISSUE_RAID_SETUP.md#proposed-solutions)
- **Prometheus Config**: [Issue Doc](GITHUB_ISSUE_RAID_SETUP.md#monitoring-stack-configuration)

### Troubleshooting
- **Quick Diagnostics**: [Troubleshooting Guide](RAID_TROUBLESHOOTING_QUICK_GUIDE.md#quick-diagnostics)
- **Common Issues**: [Troubleshooting Guide](RAID_TROUBLESHOOTING_QUICK_GUIDE.md#common-issues)
- **Diagnostic Commands**: [Troubleshooting Guide](RAID_TROUBLESHOOTING_QUICK_GUIDE.md#diagnostic-commands)

### Operations
- **Failure Detection**: [Architecture Doc](RAID_SHARD_REFERENCING_ARCHITECTURE.md#failure-detection-and-recovery)
- **Consistency Models**: [Architecture Doc](RAID_SHARD_REFERENCING_ARCHITECTURE.md#consistency-and-coordination)
- **Performance Tuning**: [Architecture Doc](RAID_SHARD_REFERENCING_ARCHITECTURE.md#performance-considerations)

---

## 🚀 Quick Start Checklist

### First Time Setup

- [ ] Read [RAID_SHARD_REFERENCING_ARCHITECTURE.md](RAID_SHARD_REFERENCING_ARCHITECTURE.md) - Overview section
- [ ] Review `../docker/compose/docker-compose-sharding.yml`
- [ ] Understand RAID modes and choose one for your use case
- [ ] Configure environment variables
- [ ] Start cluster: `docker-compose -f docker-compose-sharding.yml up -d`
- [ ] Run health check: See [Quick Diagnostics](RAID_TROUBLESHOOTING_QUICK_GUIDE.md#quick-diagnostics)
- [ ] Verify Prometheus: `curl http://localhost:9090/targets`
- [ ] Check Grafana: http://localhost:3000

### When Issues Occur

- [ ] Check [RAID_TROUBLESHOOTING_QUICK_GUIDE.md](RAID_TROUBLESHOOTING_QUICK_GUIDE.md) first
- [ ] Run quick diagnostics section
- [ ] Match your issue to common issues list
- [ ] Apply quick fix
- [ ] If unresolved, check [GITHUB_ISSUE_RAID_SETUP.md](GITHUB_ISSUE_RAID_SETUP.md)
- [ ] Collect logs: See [Getting Help](RAID_TROUBLESHOOTING_QUICK_GUIDE.md#getting-help)
- [ ] Open GitHub issue with collected information

---

## 🎓 Learning Path

### Beginner (1-2 hours)
1. Read this README
2. Quick start: `../benchmarks/RAID_SHARDING_QUICKSTART.md`
3. Start simple cluster
4. Basic troubleshooting: [Common Issues](RAID_TROUBLESHOOTING_QUICK_GUIDE.md#common-issues)

### Intermediate (4-6 hours)
1. Deep-dive: [Architecture Overview](RAID_SHARD_REFERENCING_ARCHITECTURE.md#overview)
2. Understand RAID modes in detail
3. Configure Prometheus/Grafana
4. Run benchmark suite: `../benchmarks/bench_docker_raid_comprehensive.cpp`
5. Practice failover scenarios

### Advanced (8+ hours)
1. Complete architecture study: [Full Architecture Doc](RAID_SHARD_REFERENCING_ARCHITECTURE.md)
2. Implement custom RAID configuration
3. Performance optimization
4. Write custom benchmarks
5. Contribute improvements

---

## 📊 Common Scenarios

### Scenario 1: Fresh Installation
```bash
# 1. Clone repository
git clone https://github.com/makr-code/ThemisDB.git
cd ThemisDB

# 2. Build Docker image with metrics
docker build -f Dockerfile.themis-metrics-enabled \
  -t themisdb/themisdb:metrics-enabled .

# 3. Start RAID cluster
cd docker/compose
docker-compose -f docker-compose-sharding.yml up -d

# 4. Verify health
for port in 8080 8081 8082 8083 8084 8085 8086 8087; do
  curl http://localhost:$port/health
done

# 5. Check Grafana
open http://localhost:3000
```

**Documentation:**
- [GITHUB_ISSUE_RAID_SETUP.md](GITHUB_ISSUE_RAID_SETUP.md) - Solution 1: Fix Docker Image Build
- [RAID_TROUBLESHOOTING_QUICK_GUIDE.md](RAID_TROUBLESHOOTING_QUICK_GUIDE.md) - Verification Steps

### Scenario 2: Metrics Not Working
```bash
# Quick diagnostic
curl http://localhost:8080/metrics

# If 404 or connection refused:
# 1. Check container logs
docker logs themis-raid0-shard1

# 2. Verify metrics are enabled
docker exec themis-raid0-shard1 env | grep THEMIS_ENABLE_METRICS

# 3. Fix and restart
# See troubleshooting guide Issue 2
```

**Documentation:**
- [RAID_TROUBLESHOOTING_QUICK_GUIDE.md](RAID_TROUBLESHOOTING_QUICK_GUIDE.md) - Issue 2: Metrics Endpoint Returns 404
- [GITHUB_ISSUE_RAID_SETUP.md](GITHUB_ISSUE_RAID_SETUP.md) - Prometheus Integration Issues

### Scenario 3: Shard Failure
```bash
# Detect failure
docker ps | grep themis-raid

# Check cluster health
curl http://localhost:9090/api/v1/query?query=themis_shard_health_status

# Recovery depends on RAID mode:
# RAID1: Automatic failover
# RAID5: Rebuild from parity
# RAID0: Manual intervention required

# See architecture doc for details
```

**Documentation:**
- [RAID_SHARD_REFERENCING_ARCHITECTURE.md](RAID_SHARD_REFERENCING_ARCHITECTURE.md) - Failure Detection and Recovery
- [RAID_TROUBLESHOOTING_QUICK_GUIDE.md](RAID_TROUBLESHOOTING_QUICK_GUIDE.md) - Issue 7: RAID5 Degraded Performance

---

## 🔗 External Resources

### ThemisDB Resources
- [Main README](../README.md)
- [Docker Build Guide](../DOCKER_BUILD_GUIDE.md)
- [Contributing Guide](../CONTRIBUTING.md)
- [Security Policy](../SECURITY.md)

### RAID Concepts
- [Wikipedia: RAID](https://en.wikipedia.org/wiki/RAID)
- [Consistent Hashing](https://en.wikipedia.org/wiki/Consistent_hashing)
- [CAP Theorem](https://en.wikipedia.org/wiki/CAP_theorem)

### Monitoring
- [Prometheus Documentation](https://prometheus.io/docs/)
- [Grafana Documentation](https://grafana.com/docs/)
- [Docker Monitoring](https://docs.docker.com/config/containers/runmetrics/)

### Distributed Systems
- [Designing Data-Intensive Applications](https://dataintensive.net/) - Martin Kleppmann
- [Google SRE Book](https://sre.google/books/)
- [Raft Consensus](https://raft.github.io/)

---

## 💡 Best Practices

### Configuration
✅ **DO:**
- Use consistent Docker images across all shards
- Enable metrics on all nodes
- Configure health checks
- Set appropriate resource limits
- Use explicit port mappings

❌ **DON'T:**
- Mix Windows and Linux binaries
- Disable metrics in production
- Use default passwords
- Skip health checks
- Ignore warning logs

### Operations
✅ **DO:**
- Monitor all shards continuously
- Test failover procedures regularly
- Keep configuration in version control
- Document custom configurations
- Run benchmarks before production

❌ **DON'T:**
- Skip backup procedures
- Ignore replication lag
- Deploy without testing
- Modify configuration manually
- Restart all shards simultaneously

### Troubleshooting
✅ **DO:**
- Start with quick diagnostics
- Check logs first
- Collect complete information
- Test fixes in staging
- Document solutions

❌ **DON'T:**
- Make blind changes
- Skip verification steps
- Ignore error messages
- Test directly in production
- Forget to backup before fixes

---

## 🆘 Getting Help

### Before Opening an Issue

1. Check [RAID_TROUBLESHOOTING_QUICK_GUIDE.md](RAID_TROUBLESHOOTING_QUICK_GUIDE.md)
2. Review [GITHUB_ISSUE_RAID_SETUP.md](GITHUB_ISSUE_RAID_SETUP.md) - Known Issues
3. Search existing GitHub issues
4. Collect diagnostic information:
   ```bash
   # Use log collection script
   mkdir -p /tmp/themis-debug
   docker-compose logs > /tmp/themis-debug/logs.txt
   docker ps -a > /tmp/themis-debug/containers.txt
   curl -s http://localhost:9090/api/v1/targets > /tmp/themis-debug/targets.json
   ```

### Support Channels

- **GitHub Issues**: https://github.com/makr-code/ThemisDB/issues
- **Discussions**: https://github.com/makr-code/ThemisDB/discussions
- **Documentation**: https://themisdb.readthedocs.io/
- **Community**: https://discord.gg/themisdb (if available)

### Issue Template

When opening an issue, include:
- ThemisDB version
- Docker version
- Operating system
- Configuration files (sanitized)
- Complete error logs
- Steps to reproduce
- Expected vs actual behavior

---

## 🔄 Document Updates

This documentation is actively maintained. Last update: **2026-01-04**

### Recent Changes
- **2026-01-04**: Initial RAID documentation release
  - Created comprehensive issue documentation
  - Added technical architecture guide
  - Published quick troubleshooting reference
  - Updated documentation index

### Planned Updates
- Add more architecture diagrams
- Create video tutorials
- Add Kubernetes deployment guide
- Expand benchmark documentation
- Add performance tuning cookbook

---

## 📝 Contributing to Documentation

Found an error? Have a suggestion? We welcome contributions!

1. Fork the repository
2. Edit markdown files
3. Test your changes
4. Submit a pull request

See [CONTRIBUTING.md](../CONTRIBUTING.md) for detailed guidelines.

---

**Documentation Hub Version:** 1.0  
**Last Updated:** 2026-04-06  
**Maintained by:** ThemisDB Team

