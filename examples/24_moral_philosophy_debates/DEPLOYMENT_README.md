> **Hinweis:** Inhalt mit aktuellem Modulcode und -stand abgleichen.

# Ethics AI Framework - Production Deployment Guide

> **Historischer Stand:** 2026-01-31 — Inhalte nicht gegen aktuelle Quellen geprüft.

Complete production deployment configuration for the ThemisDB Ethics AI Framework with Docker, Kubernetes, and comprehensive monitoring.

## 📋 Table of Contents

- [Overview](#overview)
- [Components](#components)
- [Docker Deployment](#docker-deployment)
- [Kubernetes Deployment](#kubernetes-deployment)
- [Monitoring](#monitoring)
- [Scaling](#scaling)
- [Security](#security)
- [Troubleshooting](#troubleshooting)

## 🎯 Overview

This deployment setup provides:

- **Container orchestration** with Docker and Kubernetes
- **High availability** with multiple replicas and health checks
- **Auto-scaling** based on CPU, memory, and custom metrics
- **Load balancing** with NGINX
- **Monitoring** with Prometheus and Grafana
- **Production-ready security** with non-root users, resource limits, and network policies

> **Note:** Current Docker image: `docker run -d -p 8080:8080 themisdb/themisdb:latest`
> <!-- TODO: verify against current source -->

## 🏗️ Components

### Core Services

1. **ThemisDB** - Multi-model database for storing debates and arguments
2. **Ethics AI** - Main application with moral philosophy reasoning
3. **NGINX** - Load balancer for distributing traffic
4. **Prometheus** - Metrics collection and monitoring
5. **Grafana** - Visualization and dashboards
6. **Redis** - Caching and session management

### Python Deployment Manager

The `ethics_ai_production_deployment.py` module provides:

- `DockerContainerManager` - Docker lifecycle management
- `KubernetesOrchestrator` - K8s deployment orchestration
- `HealthCheckManager` - Service health monitoring
- `VersionManager` - Deployment versioning and rollback

## 🐳 Docker Deployment

### Quick Start

```bash
# Navigate to docker directory
cd /home/runner/work/ThemisDB/ThemisDB/docker

# Start the complete stack
docker-compose -f docker-compose.ethics.yml up -d

# Check status
docker-compose -f docker-compose.ethics.yml ps

# View logs
docker-compose -f docker-compose.ethics.yml logs -f ethics-ai-1
```

### Access Points

- **Ethics AI**: http://localhost:80 (load balanced)
- **Ethics AI Replica 1**: http://localhost:8081
- **Ethics AI Replica 2**: http://localhost:8082
- **ThemisDB**: http://localhost:8529
- **Prometheus**: http://localhost:9091
- **Grafana**: http://localhost:3000 (admin/admin)
- **Redis**: localhost:6379

### Container Management with Python

```python
from ethics_ai_production_deployment import (
    DockerContainerManager, 
    ContainerConfig,
    create_ethics_ai_stack
)

# Initialize manager
manager = DockerContainerManager()

# Create network
manager.create_network("ethics-ai-network")

# Deploy stack
stack = create_ethics_ai_stack()
for name, config in stack.items():
    success, result = manager.deploy_container(config)
    print(f"Deployed {name}: {result}")

# Monitor containers
stats = manager.get_container_stats("ethics-ai-service-1")
logs = manager.get_container_logs("ethics-ai-service-1", tail=100)
```

### Scaling Services

```bash
# Scale Ethics AI to 5 replicas
docker-compose -f docker-compose.ethics.yml up -d --scale ethics-ai-1=3 --scale ethics-ai-2=2

# Stop a service
docker-compose -f docker-compose.ethics.yml stop ethics-ai-1

# Remove all services
docker-compose -f docker-compose.ethics.yml down
```

## ☸️ Kubernetes Deployment

### Prerequisites

- Kubernetes cluster (1.25+)
- kubectl configured
- Storage class available
- (Optional) Prometheus Operator for advanced monitoring

### Deploy to Kubernetes

```bash
# Create namespace
kubectl create namespace ethics-ai

# Apply configurations
kubectl apply -f /home/runner/work/ThemisDB/ThemisDB/config/kubernetes/ethics-ai-configmap.yaml
kubectl apply -f /home/runner/work/ThemisDB/ThemisDB/config/kubernetes/ethics-ai-deployment.yaml
kubectl apply -f /home/runner/work/ThemisDB/ThemisDB/config/kubernetes/ethics-ai-service.yaml
kubectl apply -f /home/runner/work/ThemisDB/ThemisDB/config/kubernetes/ethics-ai-hpa.yaml

# Check deployment status
kubectl get all -n ethics-ai

# View logs
kubectl logs -n ethics-ai -l app=ethics-ai --tail=100 -f
```

### K8s Management with Python

```python
from ethics_ai_production_deployment import KubernetesOrchestrator

# Initialize orchestrator
k8s = KubernetesOrchestrator(namespace="ethics-ai")

# Apply manifests
success, output = k8s.apply_manifest("config/kubernetes/ethics-ai-deployment.yaml")

# Check deployment status
status = k8s.get_deployment_status("ethics-ai")
print(f"Replicas: {status['spec']['replicas']}")
print(f"Available: {status['status']['availableReplicas']}")

# Scale deployment
k8s.scale_deployment("ethics-ai", replicas=5)

# Rolling restart
k8s.rollout_restart("ethics-ai")

# Rollback if needed
k8s.rollout_undo("ethics-ai")
```

### Access Services

```bash
# Port forward to access locally
kubectl port-forward -n ethics-ai svc/ethics-ai 8080:80

# Access via ingress (configure DNS first)
# https://ethics-ai.example.com
```

## 📊 Monitoring

### Prometheus Metrics

The Ethics AI framework exposes metrics at `/metrics`:

**Application Metrics:**
- `ethics_ai_debates_total` - Total number of debates
- `ethics_ai_arguments_generated` - Arguments generated
- `ethics_ai_active_debates` - Currently active debates
- `ethics_ai_queue_depth` - Processing queue depth
- `http_requests_total` - HTTP request count
- `http_request_duration_seconds` - Request latency

**System Metrics:**
- CPU, memory, disk usage
- Network I/O
- Container statistics

### Health Monitoring with Python

```python
from ethics_ai_production_deployment import (
    HealthCheckManager,
    HealthCheckConfig
)

# Initialize manager
health_mgr = HealthCheckManager()

# Register health checks
health_mgr.register_health_check(
    "ethics-ai-1",
    HealthCheckConfig(
        endpoint="http://localhost:8081/health",
        interval_seconds=30,
        timeout_seconds=10,
        retries=3
    )
)

# Add alert callback
def on_health_change(service, old_status, new_status):
    print(f"{service}: {old_status.value} -> {new_status.value}")
    # Send alert to Slack, PagerDuty, etc.

health_mgr.add_alert_callback(on_health_change)

# Monitor continuously
health_mgr.monitor_continuously(interval_seconds=30)
```

### Grafana Dashboards

1. Access Grafana at http://localhost:3000
2. Login with admin/admin
3. Import dashboards from `/home/runner/work/ThemisDB/ThemisDB/grafana/dashboards/`
4. Configure Prometheus data source

## 📈 Scaling

### Auto-Scaling (Kubernetes)

The HPA configuration automatically scales based on:

- **CPU usage** > 70%
- **Memory usage** > 80%
- **Requests per second** > 100 per pod
- **Response latency** > 500ms
- **Active debates** > 10 per pod
- **Queue depth** > 50 items per pod

### Manual Scaling

**Docker:**
```bash
docker-compose -f docker-compose.ethics.yml up -d --scale ethics-ai-1=5
```

**Kubernetes:**
```bash
kubectl scale deployment ethics-ai -n ethics-ai --replicas=5
```

**Python:**
```python
k8s.scale_deployment("ethics-ai", replicas=5)
```

## 🔒 Security

### Best Practices Implemented

1. **Non-root containers** - All containers run as non-root user (UID 1000)
2. **Read-only filesystems** - Where possible
3. **Resource limits** - CPU and memory limits enforced
4. **Network policies** - Restrict pod-to-pod communication
5. **Security contexts** - Drop all capabilities, seccomp profile
6. **Secret management** - Sensitive data in Kubernetes Secrets
7. **TLS/SSL** - HTTPS support with cert-manager
8. **Rate limiting** - Prevent abuse

### Security Scanning

```bash
# Scan Docker images
docker scan themisdb/ethics-ai:latest

# Scan Kubernetes manifests
kubectl kube-score score config/kubernetes/ethics-ai-deployment.yaml
```

## 🔧 Troubleshooting

### Check Container Health

```bash
# Docker
docker ps
docker logs ethics-ai-service-1
docker exec -it ethics-ai-service-1 /bin/bash

# Kubernetes
kubectl get pods -n ethics-ai
kubectl describe pod <pod-name> -n ethics-ai
kubectl logs <pod-name> -n ethics-ai
kubectl exec -it <pod-name> -n ethics-ai -- /bin/bash
```

### Common Issues

**Pod not starting:**
```bash
# Check events
kubectl describe pod <pod-name> -n ethics-ai

# Check logs
kubectl logs <pod-name> -n ethics-ai

# Check resource constraints
kubectl top pods -n ethics-ai
```

**Database connection issues:**
```bash
# Test connectivity
kubectl run -it --rm debug --image=busybox --restart=Never -n ethics-ai -- sh
nc -zv themisdb 8529
```

**Performance issues:**
```bash
# Check metrics
kubectl top pods -n ethics-ai
kubectl top nodes

# Check HPA status
kubectl get hpa -n ethics-ai
kubectl describe hpa ethics-ai-hpa -n ethics-ai
```

### Version Management and Rollback

```python
from ethics_ai_production_deployment import VersionManager, DeploymentStatus

# Initialize manager
version_mgr = VersionManager()

# Register new deployment
version = version_mgr.register_deployment(
    version="v1.2.0",
    image_tag="1.2.0",
    config={"replicas": 3}
)

# Update status
version_mgr.update_status("v1.2.0", DeploymentStatus.HEALTHY)

# Get current version
current = version_mgr.get_current_version()
print(f"Current: {current.version}")

# Rollback to previous
previous = version_mgr.get_previous_version()
if previous:
    print(f"Rolling back to: {previous.version}")
    # Perform rollback...
```

## 📚 Additional Resources

- [ThemisDB Documentation](../../docs/)
- [Ethics AI Example](../)
- [Kubernetes Best Practices](https://kubernetes.io/docs/concepts/configuration/overview/)
- [Prometheus Documentation](https://prometheus.io/docs/)
- [Docker Documentation](https://docs.docker.com/)

## 🤝 Support

For issues and questions:
- Open an issue on GitHub
- Check the troubleshooting guide above
- Review logs and metrics in Grafana

## 📝 License

MIT License - See LICENSE file for details
