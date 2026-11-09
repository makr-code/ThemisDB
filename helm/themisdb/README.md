---
Author: "ThemisDB Team"
Created: "2026-10-09"
Last Updated: "2026-11-09"
Status: "active"
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "GitHub Copilot"
Human_Review: false
Human_Author: "ThemisDB Team"
---

> Exact model backend not verifiable from final document text alone.

# ThemisDB Helm Chart

A Helm chart for deploying ThemisDB - Multi-Model Database with ACID Transactions on Kubernetes.

## Prerequisites

- Kubernetes 1.23+
- Helm 3.8+
- PV provisioner support in the underlying infrastructure (if persistence is enabled)
- [Prometheus Operator](https://github.com/prometheus-operator/prometheus-operator) (optional, required for ServiceMonitor)
- [VPA controller](https://github.com/kubernetes/autoscaler/tree/master/vertical-pod-autoscaler) (optional, required for VPA)
- Grafana with [grafana-sc-dashboard sidecar](https://github.com/grafana/helm-charts/tree/main/charts/grafana) (optional, required for automatic dashboard import)

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

### Core parameters

| Parameter | Description | Default |
|-----------|-------------|---------|
| `replicaCount` | Number of replicas | `1` |
| `image.repository` | Container image repository | `themisdb/themisdb` |
| `image.tag` | Container image tag | `Chart.AppVersion` |
| `service.type` | Kubernetes service type | `ClusterIP` |
| `service.port` | HTTP API service port | `8080` |
| `service.metricsPort` | Prometheus metrics port | `9091` |
| `persistence.enabled` | Enable persistent storage | `true` |
| `persistence.size` | PVC size | `10Gi` |
| `resources.limits.memory` | Memory limit | `2Gi` |
| `resources.limits.cpu` | CPU limit | `1000m` |
| `podDisruptionBudget.enabled` | Create a PodDisruptionBudget | `false` |
| `podDisruptionBudget.minAvailable` | Minimum pods available during a voluntary disruption | `1` |
| `topologySpreadConstraints` | Spread replicas across configured topology domains | `[]` |

### Autoscaling (HPA)

| Parameter | Description | Default |
|-----------|-------------|---------|
| `autoscaling.enabled` | Enable HPA | `false` |
| `autoscaling.minReplicas` | Minimum replicas | `1` |
| `autoscaling.maxReplicas` | Maximum replicas | `10` |
| `autoscaling.targetCPUUtilizationPercentage` | CPU target | `75` |
| `autoscaling.targetMemoryUtilizationPercentage` | Memory target | `80` |

### Vertical Pod Autoscaler (VPA)

| Parameter | Description | Default |
|-----------|-------------|---------|
| `vpa.enabled` | Enable VPA | `false` |
| `vpa.updateMode` | VPA update mode (`Off` / `Initial` / `Recreate` / `Auto`) | `"Off"` |
| `vpa.minAllowed.cpu` | Minimum CPU recommendation | `100m` |
| `vpa.minAllowed.memory` | Minimum memory recommendation | `256Mi` |
| `vpa.maxAllowed.cpu` | Maximum CPU recommendation | `4000m` |
| `vpa.maxAllowed.memory` | Maximum memory recommendation | `8Gi` |

### Metrics & ServiceMonitor

| Parameter | Description | Default |
|-----------|-------------|---------|
| `metrics.enabled` | Enable metrics endpoint | `true` |
| `metrics.serviceMonitor.enabled` | Create ServiceMonitor (requires Prometheus Operator) | `false` |
| `metrics.serviceMonitor.namespace` | Namespace for ServiceMonitor | release namespace |
| `metrics.serviceMonitor.interval` | Scrape interval | `30s` |
| `metrics.serviceMonitor.scrapeTimeout` | Scrape timeout | `10s` |

### NetworkPolicy

| Parameter | Description | Default |
|-----------|-------------|---------|
| `networkPolicy.enabled` | Enable NetworkPolicy | `false` |
| `networkPolicy.prometheusNamespace` | Namespace allowed to scrape metrics | `"monitoring"` |
| `networkPolicy.allowAllIngress` | Allow all ingress (no podSelector filter) | `false` |
| `networkPolicy.storageEgressPorts` | TCP peer ports allowed in egress | `[8766]` |
| `networkPolicy.storageUdpEgressPorts` | UDP peer ports allowed in egress | `[8769]` |
| `networkPolicy.additionalIngressPeers` | Additional NetworkPolicy peers for API ingress | `[]` |
| `networkPolicy.dnsNamespaceSelector` / `dnsPodSelector` | Cluster DNS destination selectors | `kube-system` / `k8s-app=kube-dns` |
| `networkPolicy.allowHttpsEgress` | Allow HTTPS (443) egress | `false` |

### Grafana Dashboard ConfigMaps

| Parameter | Description | Default |
|-----------|-------------|---------|
| `grafana.dashboards.enabled` | Create Grafana dashboard ConfigMaps | `false` |
| `grafana.dashboards.namespace` | Namespace for ConfigMaps | release namespace |
| `grafana.dashboards.label` | Label key for Grafana sidecar discovery | `grafana_dashboard` |
| `grafana.dashboards.labelValue` | Label value | `"1"` |
| `grafana.dashboards.categories.llm` | Include LLM dashboards | `true` |
| `grafana.dashboards.categories.performance` | Include Performance dashboards | `true` |
| `grafana.dashboards.categories.security` | Include Security dashboards | `true` |
| `grafana.dashboards.categories.operations` | Include Operations dashboards | `true` |
| `grafana.dashboards.categories.compliance` | Include Compliance dashboards | `true` |
| `grafana.dashboards.categories.plugins` | Include Plugin dashboards | `false` |
| `grafana.dashboards.categories.bt4` | Include BT4/FLARE dashboards | `false` |

### Service Mesh (Istio)

| Parameter | Description | Default |
|-----------|-------------|---------|
| `serviceMesh.istioInject` | Inject Istio sidecar | `false` |
| `serviceMesh.trustSidecarMTLS` | Delegate mTLS to Envoy | `false` |
| `serviceMesh.drainTimeoutMs` | Envoy drain timeout on shutdown | `5000` |

## Image

Official community image on Docker Hub: `themisdb/themisdb`

The chart default already uses this repository. Override `image.repository` only if you want
to deploy from a different registry (for example a private mirror):

```yaml
image:
  repository: registry.example.com/themisdb/themisdb
  tag: latest
```

## Production availability and network policy profile

The chart defaults remain suitable for development: one replica, no disruption
budget or scheduling constraints, and NetworkPolicy disabled. To opt in to the
production HA/network baseline, use the separate profile:

```bash
helm install themisdb ./helm/themisdb \
  --namespace production --create-namespace \
  --values ./helm/themisdb/values-production.yaml
```

The profile sets three replicas, `minAvailable: 2`, preferred pod anti-affinity,
and hostname topology spread. The enabled NetworkPolicy permits API traffic
from pods in the release namespace, metrics traffic from the configured
monitoring namespace, and health probes on the health port. Add
`networkPolicy.additionalIngressPeers` for ingress controllers or clients in
other namespaces. Egress is limited to cluster DNS on TCP/UDP 53 and the
configured TCP/UDP ports to pods in this release; HTTPS is denied unless
explicitly enabled. NetworkPolicy selects the ThemisDB pods for both ingress
and egress, so other ingress and egress traffic is denied by default. The
health-probe rule is port-scoped because node-originated probe identity varies
by CNI. Verify the DNS selector labels and required replication ports against
the target cluster and CNI before rollout.

The profile intentionally does not enable ServiceMonitor or PrometheusRule;
monitoring resources are tracked separately in
[makr-code/ThemisDB#7532](https://github.com/makr-code/ThemisDB/issues/7532).
The chart's single `Deployment` storage volume is not converted into per-pod
storage by this profile. Production operators must provision and validate a
storage/replication design suitable for multiple replicas before rollout.

## Upgrading

```bash
helm upgrade themisdb themisdb/themisdb
```

### Upgrade notes

**0.1.x → 0.2.x**
- `service.metricsPort` default changed from `4318` to `9091` (Prometheus-compatible).
  If you relied on the OTel HTTP port `4318` as the scrape target, update your
  `values.yaml` accordingly.
- ServiceMonitor now references the named port `metrics` on the Service.
  Ensure `metrics.serviceMonitor.enabled=true` and the Prometheus Operator is installed.
- New optional resources: `NetworkPolicy`, `VPA`, Grafana `ConfigMaps`.
  All are disabled by default — no action needed on upgrade.

### Production profile upgrade and rollback

Applying `values-production.yaml` to an existing release changes the Deployment
to three replicas and creates a PDB and NetworkPolicy. Confirm that at least
three suitable nodes are available, configure `additionalIngressPeers` and DNS
selectors for the cluster, and ensure the data volume supports the planned
replica/replication model before upgrading. Existing clients outside the
release namespace may lose API access until explicitly allowlisted. The PDB
can block voluntary node drains when fewer than two pods are healthy; resolve
readiness issues before maintenance rather than bypassing the budget.

Rollback with the previous values file or `helm rollback <release> <revision>`.
Helm removes the profile's PDB and NetworkPolicy when rolling back to values
that disable them; network isolation is therefore relaxed and replica count
returns to the selected prior value. Confirm the application and client paths
after rollback. Do not treat Helm rollback as a data rollback.

## Uninstalling

```bash
helm uninstall themisdb
```

> Note: This will not delete the PersistentVolumeClaim. To fully remove the data:
> ```bash
> kubectl delete pvc -l app.kubernetes.io/name=themisdb
> ```

## Dependencies

This chart has no hard Helm dependencies, but the following operators unlock optional features:

| Feature | Operator / Component |
|---------|---------------------|
| `metrics.serviceMonitor` | [Prometheus Operator](https://github.com/prometheus-operator/prometheus-operator) |
| `vpa` | [VPA controller](https://github.com/kubernetes/autoscaler/tree/master/vertical-pod-autoscaler) |
| `grafana.dashboards` | [Grafana chart](https://github.com/grafana/helm-charts) with `sidecar.dashboards.enabled=true` |
| `serviceMesh.istioInject` | [Istio](https://istio.io/) control plane |

## License

Apache License 2.0 - See [LICENSE](../../LICENSE) for details.
