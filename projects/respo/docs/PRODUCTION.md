# RESPO Production Deployment Guide

## Overview

This guide covers deploying RESPO in a production environment.

## Hardware Requirements

### Minimum (Development)
- CPU: 4 cores
- RAM: 8 GB
- Storage: 50 GB SSD

### Recommended (Production)
- CPU: 16+ cores
- RAM: 32+ GB
- GPU: NVIDIA A100/H100 (for vLLM)
- Storage: 500 GB NVMe SSD

## Deployment Options

### 1. Docker Compose

```bash
cd projects/respo/docker
docker compose -f docker-compose.prod.yml up -d
```

### 2. Kubernetes (Helm)

```bash
cd projects/respo/helm

# Install with default values
helm install respo . -n respo --create-namespace

# Install with custom values
helm install respo . -f production-values.yaml -n respo

# Upgrade
helm upgrade respo . -f production-values.yaml -n respo
```

### 3. Air-Gapped Deployment

```bash
# Package for offline installation
./scripts/package-airgap.sh

# On target system
./scripts/deploy.sh airgap
```

## Configuration

### Environment Variables

```bash
# Required
VECTOR_STORE=chroma           # chroma, qdrant, themis
VLLM_URL=http://vllm:8000     # vLLM server URL
VLLM_MODEL=codellama/CodeLlama-7b-Instruct-hf

# Optional
LOG_LEVEL=INFO
CACHE_BACKEND=redis
REDIS_HOST=redis
REDIS_PORT=6379
```

### Vector Store Selection

| Store | Use Case | Requirements |
|-------|----------|--------------|
| ChromaDB | Small deployments (<100K docs) | No external deps |
| Qdrant | Medium deployments | Qdrant server |
| ThemisDB | Large with graph queries | ThemisDB server |

## Monitoring

### Prometheus Metrics

```yaml
# prometheus.yml
scrape_configs:
  - job_name: respo
    static_configs:
      - targets: ['respo-api:8080']
    metrics_path: /metrics
```

### Health Checks

```bash
# Liveness
curl http://respo-api:8080/health

# Readiness
curl http://respo-api:8080/health?detailed=true
```

## Backup Strategy

### Manual Backup

```python
from respo.backup import VectorStoreBackup

backup = VectorStoreBackup(store)
await backup.create("/backups/manual-backup.tar.gz")
```

### Scheduled Backup (Kubernetes CronJob)

```yaml
apiVersion: batch/v1
kind: CronJob
metadata:
  name: respo-backup
spec:
  schedule: "0 2 * * *"  # Daily at 2 AM
  jobTemplate:
    spec:
      template:
        spec:
          containers:
            - name: backup
              image: respo:latest
              command: ["respo", "backup", "--output", "/backups"]
```

### Restore

```python
from respo.backup import VectorStoreBackup

backup = VectorStoreBackup(store)
await backup.restore("/backups/manual-backup.tar.gz")
```

## Security

### Network Policies

```yaml
apiVersion: networking.k8s.io/v1
kind: NetworkPolicy
metadata:
  name: respo-network-policy
spec:
  podSelector:
    matchLabels:
      app: respo
  ingress:
    - from:
        - namespaceSelector:
            matchLabels:
              name: allowed-namespace
      ports:
        - port: 8080
```

### Secrets Management

```bash
# Create secrets
kubectl create secret generic respo-secrets \
  --from-literal=GITHUB_TOKEN=xxx \
  --from-literal=OPENAI_API_KEY=xxx \
  -n respo
```

## Scaling

### Horizontal Pod Autoscaler

```yaml
apiVersion: autoscaling/v2
kind: HorizontalPodAutoscaler
metadata:
  name: respo-hpa
spec:
  scaleTargetRef:
    apiVersion: apps/v1
    kind: Deployment
    name: respo-api
  minReplicas: 2
  maxReplicas: 10
  metrics:
    - type: Resource
      resource:
        name: cpu
        target:
          type: Utilization
          averageUtilization: 70
```

## Troubleshooting

### Common Issues

1. **OOM Errors**: Increase memory limits or use smaller models
2. **Slow Embeddings**: Enable GPU or use cached embeddings
3. **Connection Timeouts**: Check network policies and service discovery

### Logs

```bash
# Kubernetes
kubectl logs -f deployment/respo-api -n respo

# Docker
docker logs -f respo-api
```

## Support

For issues, please open a GitHub issue or contact the maintainers.
