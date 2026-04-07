# Scaling Runbook

**Version:** 1.0  
**Last Updated:** April 2026  
**Target Audience:** Operations Teams, SREs

---

## Overview

This runbook provides procedures for scaling ThemisDB horizontally and vertically.

### Scaling Methods

| Method | Use Case | Downtime | Complexity |
|--------|----------|----------|------------|
| Horizontal (add nodes) | Increase capacity | Zero | Medium |
| Vertical (bigger nodes) | Increase per-node power | Minimal | Low |
| Auto-scaling | Dynamic workloads | Zero | Low |

---

## Horizontal Scaling (Scale Out)

### When to Scale Out

Indicators:
- CPU utilization > 75% sustained
- Memory utilization > 80%
- Storage capacity > 70%
- Increased latency (p95 > 100ms)
- Request queue depth increasing

### Scale Out Procedure

#### Step 1: Plan Capacity Addition

```bash
# Assess current capacity
themisdb-cli cluster capacity

# Calculate target capacity
# Current: 3 nodes, 100GB each = 300GB
# Target: 5 nodes, 100GB each = 500GB (67% increase)

# Verify resources available
kubectl describe nodes | grep -A 5 "Allocated resources"
```

#### Step 2: Add New Nodes

**Kubernetes Deployment:**

```bash
# Scale deployment
kubectl scale deployment themisdb \
  --replicas=5 \
  --namespace production

# Wait for pods to be ready
kubectl wait --for=condition=ready pod \
  --selector app=themisdb \
  --timeout=600s \
  --namespace production

# Verify new pods
kubectl get pods -n production -l app=themisdb
```

**Helm Chart:**

```bash
# Update replica count
helm upgrade themisdb ./helm/themisdb \
  --set replicaCount=5 \
  --namespace production \
  --wait \
  --timeout 10m

# Verify deployment
helm status themisdb -n production
```

#### Step 3: Initialize New Nodes

```bash
# Check cluster status
themisdb-cli cluster status

# New nodes should auto-join cluster
# Wait for nodes to be ready
themisdb-cli cluster wait-for-nodes --count 5 --timeout 600s

# Verify node health
themisdb-cli node health --all
```

#### Step 4: Rebalance Shards

```bash
# Initiate shard rebalancing
themisdb-cli cluster rebalance \
  --strategy even \
  --max-parallel 2 \
  --throttle-mbps 100

# Monitor rebalancing progress
watch -n 5 'themisdb-cli cluster rebalance-status'

# Expected output:
# Rebalancing in progress...
# Moved: 45/120 shards (37%)
# Estimated time remaining: 15m
```

#### Step 5: Verify Scale Out

```bash
# Check shard distribution
themisdb-cli shards distribution

# Expected: Even distribution across 5 nodes
# Node-1: 24 shards (20%)
# Node-2: 24 shards (20%)
# Node-3: 24 shards (20%)
# Node-4: 24 shards (20%)
# Node-5: 24 shards (20%)

# Run load test
themisdb-cli test load \
  --duration 10m \
  --threads 100 \
  --verify-distribution

# Verify performance improvement
themisdb-cli metrics compare \
  --before pre-scale.json \
  --after now
```

---

## Horizontal Scaling (Scale In)

### When to Scale In

Indicators:
- CPU utilization < 30% sustained for 7 days
- Cost optimization opportunity
- Over-provisioned for current load

### Scale In Procedure

```bash
# Step 1: Identify nodes to remove
themisdb-cli cluster recommend-scale-in
# Suggests: Remove node-4, node-5 (lowest utilization)

# Step 2: Drain nodes
for NODE in node-4 node-5; do
  themisdb-cli node drain $NODE \
    --graceful-timeout 600s \
    --move-shards \
    --wait
done

# Step 3: Verify shards moved
themisdb-cli shards list --filter node=node-4
# Expected: No shards on node-4

# Step 4: Remove nodes from cluster
for NODE in node-4 node-5; do
  themisdb-cli cluster remove-node $NODE --confirm
done

# Step 5: Scale down deployment
kubectl scale deployment themisdb --replicas=3 --namespace production

# Step 6: Verify cluster health
themisdb-cli cluster health
```

---

## Vertical Scaling (Scale Up)

### When to Scale Up

Indicators:
- Memory pressure (frequent swapping)
- CPU-bound queries
- Large dataset queries slow
- Horizontal scaling not feasible

### Scale Up Procedure

#### Step 1: Plan Resource Increase

```bash
# Current resources
kubectl describe pod themisdb-0 | grep -A 10 "Requests:"
# Current: 4 CPU, 16GB RAM

# Target resources
# Target: 8 CPU, 32GB RAM

# Verify node capacity
kubectl describe node | grep "Allocatable:"
```

#### Step 2: Update Resource Limits

```bash
# Update Helm values
cat > scale-up-values.yaml <<EOF
resources:
  requests:
    cpu: 8000m
    memory: 32Gi
  limits:
    cpu: 8000m
    memory: 32Gi
EOF

# Apply update with rolling restart
helm upgrade themisdb ./helm/themisdb \
  --values production-values.yaml \
  --values scale-up-values.yaml \
  --namespace production
```

#### Step 3: Rolling Restart with New Resources

```bash
# Helm automatically performs rolling restart
# Monitor progress
kubectl rollout status deployment/themisdb -n production

# Verify new resource allocation
kubectl describe pod themisdb-0 | grep -A 10 "Requests:"
# Expected: 8 CPU, 32GB RAM
```

#### Step 4: Tune for New Resources

```bash
# Update ThemisDB configuration for more resources
themisdb-cli config update \
  --set server.worker_threads=16 \
  --set storage.block_cache_size_mb=16384 \
  --set storage.memtable_size_mb=2048

# Restart required for these changes
kubectl rollout restart deployment/themisdb -n production
```

#### Step 5: Verify Performance

```bash
# Run benchmark with new resources
themisdb-cli benchmark \
  --suite standard \
  --output post-scale-up.json

# Compare to baseline
themisdb-cli benchmark compare \
  --baseline pre-scale-up.json \
  --current post-scale-up.json
```

---

## Auto-Scaling (HPA)

### Enable Auto-Scaling

```bash
# Update Helm chart to enable HPA
cat > hpa-values.yaml <<EOF
autoscaling:
  enabled: true
  minReplicas: 3
  maxReplicas: 10
  targetCPUUtilizationPercentage: 75
  targetMemoryUtilizationPercentage: 80
EOF

# Apply HPA configuration
helm upgrade themisdb ./helm/themisdb \
  --values production-values.yaml \
  --values hpa-values.yaml \
  --namespace production

# Verify HPA created
kubectl get hpa -n production
```

### Monitor Auto-Scaling

```bash
# Watch HPA status
watch -n 5 'kubectl get hpa themisdb -n production'

# Expected output:
# NAME      REFERENCE          TARGETS    MINPODS   MAXPODS   REPLICAS
# themisdb  Deployment/themisdb 65%/75%   3         10        3

# View HPA events
kubectl describe hpa themisdb -n production

# Monitor scaling events
kubectl get events -n production --sort-by='.lastTimestamp' | grep HPA
```

### Configure Custom Metrics for HPA

```bash
# Create custom metrics for auto-scaling
cat > custom-metrics.yaml <<EOF
apiVersion: v1
kind: ConfigMap
metadata:
  name: themisdb-custom-metrics
  namespace: production
data:
  metrics: |
    - type: Pods
      pods:
        metric:
          name: themisdb_requests_per_second
        target:
          type: AverageValue
          averageValue: "1000"
    - type: Pods
      pods:
        metric:
          name: themisdb_p95_latency_ms
        target:
          type: AverageValue
          averageValue: "100"
EOF

kubectl apply -f custom-metrics.yaml

# Update HPA with custom metrics
kubectl patch hpa themisdb -n production --patch "$(cat custom-hpa-patch.yaml)"
```

---

## Storage Scaling

### Expand Persistent Volumes

```bash
# Check current PVC size
kubectl get pvc -n production

# Expand PVC (if storage class supports it)
kubectl patch pvc themisdb-data-themisdb-0 \
  -n production \
  --patch '{"spec":{"resources":{"requests":{"storage":"200Gi"}}}}'

# Verify expansion
kubectl describe pvc themisdb-data-themisdb-0 -n production

# If storage class doesn't support expansion:
# 1. Create new PVC with larger size
# 2. Restore data to new PVC
# 3. Update pod to use new PVC
```

---

## Load Balancer Scaling

### Scale Load Balancer Capacity

```bash
# For cloud load balancers
# AWS ALB/NLB - Automatically scales
# GCP Load Balancer - Automatically scales

# For NGINX Ingress Controller
kubectl scale deployment nginx-ingress-controller \
  --replicas=5 \
  --namespace ingress-nginx

# For Istio
kubectl scale deployment istiod \
  --replicas=3 \
  --namespace istio-system
```

---

## Capacity Planning

### Calculate Required Capacity

```bash
# Gather metrics for capacity planning
themisdb-cli metrics export \
  --period 30d \
  --metrics cpu,memory,storage,qps,latency \
  --output capacity-metrics.json

# Generate capacity forecast
themisdb-cli capacity forecast \
  --input capacity-metrics.json \
  --growth-rate 20% \
  --horizon 90d

# Expected output:
# Current capacity: 3 nodes, 300GB, 10K QPS
# Forecast (90 days):
# - Expected QPS: 14.4K (44% growth)
# - Expected storage: 432GB (44% growth)
# - Recommended: Scale to 5 nodes
# - Timeline: Scale by 2026-03-15
```

### Right-Sizing Recommendations

```bash
# Analyze utilization and get recommendations
themisdb-cli cluster analyze \
  --period 30d \
  --output recommendations.json

# Expected recommendations:
# {
#   "current": {
#     "nodes": 3,
#     "cpu_utilization": "45%",
#     "memory_utilization": "62%",
#     "monthly_cost": "$750"
#   },
#   "recommendations": [
#     {
#       "action": "scale_down",
#       "target_nodes": 2,
#       "expected_utilization": "67%",
#       "monthly_savings": "$250",
#       "risk": "low"
#     }
#   ]
# }
```

---

## Troubleshooting

### New Nodes Not Joining Cluster

```bash
# Check node logs
kubectl logs themisdb-4 -n production

# Verify networking
themisdb-cli network check --from node-4 --to node-1

# Check cluster configuration
themisdb-cli cluster config

# Force node to join
themisdb-cli cluster add-node node-4 --force
```

### Rebalancing Taking Too Long

```bash
# Check rebalancing progress
themisdb-cli cluster rebalance-status --verbose

# Increase parallelism
themisdb-cli cluster rebalance-config \
  --max-parallel 4 \
  --throttle-mbps 200

# Resume rebalancing
themisdb-cli cluster rebalance-resume
```

### Auto-Scaling Not Working

```bash
# Check HPA status
kubectl describe hpa themisdb -n production

# Verify metrics server
kubectl get apiservice v1beta1.metrics.k8s.io -o yaml

# Check custom metrics
kubectl get --raw "/apis/custom.metrics.k8s.io/v1beta1/namespaces/production/pods/*/themisdb_requests_per_second"

# Force HPA evaluation
kubectl patch hpa themisdb -n production --patch '{"metadata":{"annotations":{"force-sync":"'$(date +%s)'"}}}'
```

---

## Best Practices

### Scaling Guidelines

1. **Always test in staging first**
2. **Scale during low-traffic periods**
3. **Monitor closely after scaling**
4. **Keep 20-30% capacity headroom**
5. **Document all scaling operations**
6. **Review capacity monthly**

### Scaling Limits

| Resource | Minimum | Maximum | Recommended |
|----------|---------|---------|-------------|
| Nodes | 1 | 100 | 3-10 |
| CPU per node | 2 cores | 64 cores | 8-16 cores |
| Memory per node | 4 GB | 512 GB | 16-64 GB |
| Storage per node | 10 GB | 10 TB | 100-500 GB |

---

## Success Criteria

- [ ] Target capacity achieved
- [ ] Cluster health: HEALTHY
- [ ] Shards evenly distributed
- [ ] Performance improved/maintained
- [ ] No data loss during scaling
- [ ] All tests passing
- [ ] Costs within budget

---

**Capacity Planning**: Review monthly  
**Auto-Scaling Tuning**: Review quarterly  
**Performance Baseline**: Update after each scaling operation
