# ThemisDB Kubernetes Operator

This directory contains Kubernetes manifests and operator components for deploying ThemisDB in cloud-native environments.

## Quick Start

### Prerequisites
- Kubernetes 1.21+
- kubectl configured
- Helm 3.0+ (optional)

### Deploy ThemisDB

```bash
# Create namespace
kubectl create namespace themisdb

# Apply CRDs
kubectl apply -f crds/

# Create ThemisDB cluster
kubectl apply -f examples/themisdb-cluster.yaml
```

> **Note:** The operator controller is not yet implemented. 
> Currently only CRDs and example manifests are available.
> The controller implementation (Go-based) is planned for a future release.

## Directory Structure

```
kubernetes/
├── README.md                    # This file
├── crds/                        # Custom Resource Definitions
│   └── themisdb.vcc.io_themisdbs.yaml
├── examples/                    # Example configurations
│   ├── themisdb-cluster.yaml    # 3-node cluster
│   └── themisdb-single.yaml     # Single node
└── operator/ (planned)          # Operator deployment (future)
    ├── deployment.yaml
    ├── rbac.yaml
    └── service.yaml
```

## Custom Resources

### ThemisDB

```yaml
apiVersion: vcc.io/v1alpha1
kind: ThemisDB
metadata:
  name: my-cluster
spec:
  replicas: 3
  version: "1.0.0"
  storage:
    size: 100Gi
    storageClass: standard
  resources:
    requests:
      memory: "4Gi"
      cpu: "2"
    limits:
      memory: "8Gi"
      cpu: "4"
  sharding:
    enabled: true
    shards: 3
    replicationFactor: 2
  security:
    mtls: true
    rbac: true
  monitoring:
    prometheus: true
    grafana: true
```

## Features

- ✅ Automated deployment and scaling
- ✅ Rolling updates with zero downtime
- ✅ Automatic backup and recovery
- ✅ Integrated monitoring (Prometheus/Grafana)
- ✅ mTLS certificate management
- ✅ Horizontal Pod Autoscaling
- 📋 Multi-cluster federation (planned)
- 📋 Disaster recovery (planned)

## Configuration Options

| Parameter | Description | Default |
|-----------|-------------|---------|
| `spec.replicas` | Number of replicas | 3 |
| `spec.version` | ThemisDB version | latest |
| `spec.storage.size` | PVC size | 100Gi |
| `spec.sharding.enabled` | Enable sharding | false |
| `spec.security.mtls` | Enable mTLS | true |
| `spec.monitoring.prometheus` | Enable Prometheus | true |

## Troubleshooting

### Check CRD installation
```bash
kubectl get crd themisdbs.vcc.io
```

### Check cluster status
```bash
kubectl get themisdb -n themisdb
kubectl describe themisdb my-cluster -n themisdb
```

### Pod issues (when operator is deployed)
```bash
kubectl get pods -n themisdb
kubectl describe pod <pod-name> -n themisdb
```

## Contributing

See [CONTRIBUTING.md](../../CONTRIBUTING.md) for development guidelines.

## License

Apache 2.0 - See [LICENSE](../../LICENSE)
