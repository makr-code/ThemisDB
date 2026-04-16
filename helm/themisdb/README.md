# ThemisDB Helm Chart

A Helm chart for deploying ThemisDB - Multi-Model Database with ACID Transactions on Kubernetes.

## Prerequisites

- Kubernetes 1.23+
- Helm 3.8+
- PV provisioner support in the underlying infrastructure (if persistence is enabled)

## Installation

### Add the repository (when available)

```bash
helm repo add themisdb https://makr-code.github.io/ThemisDB
helm repo update
```

### Install the chart

```bash
helm install themisdb themisdb/themisdb
```

### Install from local directory

```bash
helm install themisdb ./helm/themisdb
```

## Configuration

See `values.yaml` for the full list of configurable parameters.

### Common parameters

| Parameter | Description | Default |
|-----------|-------------|---------|
| `replicaCount` | Number of replicas | `1` |
| `image.repository` | Container image repository | `themisdb/themisdb` |
| `image.tag` | Container image tag | `Chart.AppVersion` |
| `service.type` | Kubernetes service type | `ClusterIP` |
| `service.port` | Service port | `8080` |
| `persistence.enabled` | Enable persistent storage | `true` |
| `persistence.size` | PVC size | `10Gi` |
| `resources.limits.memory` | Memory limit | `2Gi` |
| `resources.limits.cpu` | CPU limit | `1000m` |

Official community image on Docker Hub:

- `themisdb/themisdb`

The chart default already uses this repository. Override `image.repository` only if you want
to deploy from a different registry (for example a private mirror):

```yaml
image:
  repository: registry.example.com/themisdb/themisdb
  tag: latest
```

### Example: Custom values

```yaml
replicaCount: 3

resources:
  limits:
    cpu: 2000m
    memory: 4Gi
  requests:
    cpu: 500m
    memory: 1Gi

persistence:
  enabled: true
  size: 50Gi
  storageClass: "fast-ssd"
```

Install with custom values:

```bash
helm install themisdb themisdb/themisdb -f custom-values.yaml
```

## Upgrading

```bash
helm upgrade themisdb themisdb/themisdb
```

## Uninstalling

```bash
helm uninstall themisdb
```

> Note: This will not delete the PersistentVolumeClaim. To fully remove the data:
> ```bash
> kubectl delete pvc -l app.kubernetes.io/name=themisdb
> ```

## License

Apache License 2.0 - See [LICENSE](../../LICENSE) for details.
