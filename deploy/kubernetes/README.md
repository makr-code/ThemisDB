# ThemisDB Kubernetes Operator

This directory contains the Kubernetes operator and manifests for deploying
and managing ThemisDB clusters in cloud-native environments.  The operator
reconciles `ThemisDB` custom resources and automatically manages replication
topology, leader election, failover, and topology-aware scheduling.

## Quick Start

### Prerequisites
- Kubernetes 1.21+
- kubectl configured
- Helm 3.0+ (optional)

### Deploy the Operator

```bash
# Create the operator namespace
kubectl create namespace themisdb-system

# Apply CRDs
kubectl apply -f crds/

# Deploy the operator (RBAC + Deployment + Service)
kubectl apply -f operator/
```

### Create a ThemisDB Cluster

```bash
# Create cluster namespace
kubectl create namespace themisdb

# Deploy a 3-node HA cluster with automated topology management
kubectl apply -f examples/themisdb-ha-replication.yaml
```

### Check Cluster Status

```bash
# Show cluster status with topology info
kubectl get themisdb -n themisdb

# Detailed topology status
kubectl describe themisdb themisdb-ha -n themisdb
```

## Directory Structure

```
kubernetes/
├── README.md                              # This file
├── crds/                                  # Custom Resource Definitions
│   └── themisdb.vcc.io_themisdbs.yaml     # ThemisDB CRD with topology fields
├── operator/                              # Operator deployment manifests
│   ├── deployment.yaml                    # Operator Deployment (2 replicas, HA)
│   ├── rbac.yaml                          # ServiceAccount, ClusterRole, Binding
│   └── service.yaml                       # Metrics Service for Prometheus
└── examples/                             # Example configurations
    ├── themisdb-cluster.yaml              # 3-node cluster with sharding
    ├── themisdb-single.yaml               # Single node (dev/test)
    ├── themisdb-ha-replication.yaml       # 3-node cluster with full replication config
    ├── hpa-basic.yaml                     # HorizontalPodAutoscaler
    ├── hpa-gpu.yaml                       # HPA with GPU metrics
    ├── load-balancer.yaml                 # LoadBalancer service
    └── vpa.yaml                           # VerticalPodAutoscaler
```

The operator source code is located at `operator/` (Go module):

```
operator/
├── cmd/main.go                            # Entry point / manager setup
├── api/v1alpha1/
│   ├── types.go                           # ThemisDB CRD Go types
│   └── deepcopy.go                        # DeepCopy methods (runtime.Object)
└── internal/controller/
    ├── themisdb_controller.go             # Reconciler implementation
    ├── export_test.go                     # Exported helpers for testing
    └── themisdb_controller_test.go        # Unit tests (32 test cases)
```

## Custom Resources

### ThemisDB

The `ThemisDB` custom resource now includes a `spec.replication` block for
configuring the automated topology management:

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
  replication:
    mode: SEMI_SYNC          # SYNC | SEMI_SYNC | ASYNC
    minSyncReplicas: 2
    leaderElection:
      electionTimeoutMinMs: 150
      electionTimeoutMaxMs: 300
      heartbeatIntervalMs: 100
      leaderPreferencePriority: 1
    failover:
      enabled: true
      failureDetectionTimeoutMs: 5000
      maxFailoverAttempts: 3
      failoverCooldownMs: 30000
    lagThresholds:
      degradedLagMs: 5000
      criticalLagMs: 30000
      readShiftEnabled: true
    wal:
      compression: true
      syncOnCommit: true
      retentionHours: 168
    topologyAware:
      spreadAcrossZones: true
      spreadAcrossNodes: true
  security:
    mtls: true
    rbac: true
  monitoring:
    prometheus: true
    grafana: true
```

## Operator – Automated Topology Management

The operator continuously reconciles each `ThemisDB` resource and manages:

| Feature | Description |
|---------|-------------|
| **StatefulSet ownership** | Operator creates and owns the `StatefulSet`; rolling updates are applied automatically when spec changes |
| **Headless Service** | Per-cluster headless Service for stable pod DNS (`<pod>.<cluster>-headless.<ns>.svc.cluster.local`) |
| **PodDisruptionBudget** | Quorum-safe PDB: at most `floor((replicas-1)/2)` pods may be unavailable simultaneously |
| **Replication ConfigMap** | All replication topology parameters are materialised in a `ConfigMap` mounted by each pod |
| **Topology status** | Operator observes pod annotations (`vcc.io/replication-role`, `vcc.io/replication-lag-ms`) and updates `.status.replicationTopology` |
| **Lag-based routing** | Replicas exceeding `degradedLagMs` are listed in `.status.replicationTopology.laggingReplicas` for upstream routers |
| **Failover detection** | Unready or terminating followers trigger a fast re-queue cycle (every 5 s) until the topology stabilises |
| **Zone-aware placement** | `TopologySpreadConstraints` keep replicas balanced across zones and nodes |
| **Leader election (operator HA)** | Two operator replicas run; only one is active via Kubernetes Lease-based leader election |

## Status Conditions

The operator sets three status conditions on each `ThemisDB`:

| Condition | `True` when |
|-----------|-------------|
| `Available` | All desired replicas are ready |
| `TopologyReady` | A leader is known and `inSyncReplicas ≥ minSyncReplicas` |
| `Degraded` | At least one replica exceeds `degradedLagMs` |

## Pod Annotations (set by ThemisDB process)

ThemisDB pods publish their replication state via pod annotations that the
operator reads during topology reconciliation:

| Annotation | Example | Description |
|------------|---------|-------------|
| `vcc.io/replication-role` | `LEADER` | Current role (`LEADER`, `FOLLOWER`, `CANDIDATE`, `OBSERVER`) |
| `vcc.io/replication-lag-ms` | `350` | Replication lag behind leader in milliseconds |

## Configuration Reference

### `spec.replication`

| Parameter | Description | Default |
|-----------|-------------|---------|
| `mode` | Replication mode (`SYNC`, `SEMI_SYNC`, `ASYNC`) | `SEMI_SYNC` |
| `minSyncReplicas` | Minimum in-sync replicas for quorum writes | `1` |
| `leaderElection.electionTimeoutMinMs` | Min election timeout (ms) | `150` |
| `leaderElection.electionTimeoutMaxMs` | Max election timeout (ms) | `300` |
| `leaderElection.heartbeatIntervalMs` | Leader heartbeat interval (ms) | `100` |
| `leaderElection.leaderPreferencePriority` | Priority for leader preference | `1` |
| `failover.enabled` | Enable automatic leader failover | `true` |
| `failover.failureDetectionTimeoutMs` | Timeout before node is declared failed | `5000` |
| `failover.maxFailoverAttempts` | Max failover attempts before giving up | `3` |
| `failover.failoverCooldownMs` | Min interval between failovers (ms) | `30000` |
| `lagThresholds.degradedLagMs` | Lag (ms) above which replica is degraded | `5000` |
| `lagThresholds.criticalLagMs` | Lag (ms) above which replica is excluded from reads | `30000` |
| `lagThresholds.readShiftEnabled` | Shift reads away from lagging replicas | `true` |
| `wal.compression` | Enable Zstd WAL compression | `true` |
| `wal.syncOnCommit` | fsync WAL on every commit | `true` |
| `wal.retentionHours` | Hours to retain WAL files | `168` |
| `topologyAware.spreadAcrossZones` | Spread pods across availability zones | `true` |
| `topologyAware.spreadAcrossNodes` | Spread pods across Kubernetes nodes | `true` |

## Troubleshooting

### Check operator logs
```bash
kubectl logs -n themisdb-system -l app.kubernetes.io/name=themisdb-operator -f
```

### Check CRD installation
```bash
kubectl get crd themisdbs.vcc.io
```

### Check cluster status
```bash
kubectl get themisdb -n themisdb -o wide
kubectl describe themisdb themisdb-ha -n themisdb
```

### Check topology status
```bash
kubectl get themisdb themisdb-ha -n themisdb \
  -o jsonpath='{.status.replicationTopology}' | jq .
```

### Pod issues
```bash
kubectl get pods -n themisdb -l app.kubernetes.io/name=themisdb
kubectl describe pod <pod-name> -n themisdb
kubectl logs <pod-name> -n themisdb
```

## Contributing

See [CONTRIBUTING.md](../../CONTRIBUTING.md) for development guidelines.

## License

Apache 2.0 – See [LICENSE](../../LICENSE)

