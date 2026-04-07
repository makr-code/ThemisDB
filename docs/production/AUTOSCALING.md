# Auto-Scaling Guide for ThemisDB

**Version:** 1.0  
**Last Updated:** April 2026  
**Target Audience:** DevOps Engineers, SREs, Platform Engineers

## Table of Contents

1. [Overview](#overview)
2. [Horizontal Pod Autoscaling (HPA)](#horizontal-pod-autoscaling-hpa)
3. [Vertical Pod Autoscaling (VPA)](#vertical-pod-autoscaling-vpa)
4. [Load Balancer Integration](#load-balancer-integration)
5. [GPU-Aware Scaling](#gpu-aware-scaling)
6. [Best Practices](#best-practices)
7. [Troubleshooting](#troubleshooting)

---

## Overview

ThemisDB supports multiple auto-scaling strategies to optimize resource utilization and maintain performance under varying loads.

### Scaling Strategies

| Strategy | Use Case | Pros | Cons |
|----------|----------|------|------|
| **HPA** | Variable request load | Handles traffic spikes, cost-effective | Slower for GPU workloads |
| **VPA** | Optimize resource allocation | Right-sizes pods automatically | Requires pod restart |
| **Manual** | Predictable workloads | Full control, no overhead | Requires manual management |
| **Scheduled** | Known traffic patterns | Proactive scaling | Less flexible |

### Prerequisites

- Kubernetes 1.23+
- Metrics Server installed
- Prometheus (for custom metrics)
- NVIDIA Device Plugin (for GPU workloads)

---

## Horizontal Pod Autoscaling (HPA)

### Basic HPA Setup

**Install Metrics Server:**
```bash
kubectl apply -f https://github.com/kubernetes-sigs/metrics-server/releases/latest/download/components.yaml
```

**Deploy ThemisDB with HPA (Helm):**
```bash
helm install themisdb ./helm/themisdb \
  --set autoscaling.enabled=true \
  --set autoscaling.minReplicas=2 \
  --set autoscaling.maxReplicas=10 \
  --set autoscaling.targetCPUUtilizationPercentage=70
```

**Deploy ThemisDB with HPA (kubectl):**
```bash
# Apply deployment
kubectl apply -f deploy/kubernetes/examples/themisdb-cluster.yaml

# Apply HPA
kubectl apply -f deploy/kubernetes/examples/hpa-basic.yaml
```

### HPA Configuration

```yaml
# hpa-basic.yaml
apiVersion: autoscaling/v2
kind: HorizontalPodAutoscaler
metadata:
  name: themisdb-hpa
spec:
  scaleTargetRef:
    apiVersion: apps/v1
    kind: Deployment
    name: themisdb
  
  minReplicas: 2
  maxReplicas: 10
  
  metrics:
    # CPU-based scaling
    - type: Resource
      resource:
        name: cpu
        target:
          type: Utilization
          averageUtilization: 70
    
    # Memory-based scaling
    - type: Resource
      resource:
        name: memory
        target:
          type: Utilization
          averageUtilization: 80
  
  behavior:
    scaleDown:
      stabilizationWindowSeconds: 300  # 5 min
      policies:
        - type: Percent
          value: 50  # Remove max 50% of pods
          periodSeconds: 60
    
    scaleUp:
      stabilizationWindowSeconds: 0  # Immediate
      policies:
        - type: Percent
          value: 100  # Double pods
          periodSeconds: 15
```

### Custom Metrics Scaling

**Prerequisites:**
- Prometheus Adapter installed
- Prometheus scraping ThemisDB metrics

**Install Prometheus Adapter:**
```bash
helm repo add prometheus-community https://prometheus-community.github.io/helm-charts
helm install prometheus-adapter prometheus-community/prometheus-adapter \
  --set prometheus.url=http://prometheus-server.monitoring.svc
```

**Configure Custom Metrics HPA:**
```yaml
apiVersion: autoscaling/v2
kind: HorizontalPodAutoscaler
metadata:
  name: themisdb-custom-metrics-hpa
spec:
  scaleTargetRef:
    apiVersion: apps/v1
    kind: Deployment
    name: themisdb
  
  minReplicas: 2
  maxReplicas: 20
  
  metrics:
    # Scale based on requests per second
    - type: Pods
      pods:
        metric:
          name: themisdb_requests_per_second
        target:
          type: AverageValue
          averageValue: "1000"
    
    # Scale based on queue depth
    - type: Pods
      pods:
        metric:
          name: themisdb_queue_depth
        target:
          type: AverageValue
          averageValue: "50"
    
    # Scale based on P95 latency
    - type: Pods
      pods:
        metric:
          name: themisdb_p95_latency_milliseconds
        target:
          type: AverageValue
          averageValue: "100"  # Scale if P95 > 100ms
```

### Monitoring HPA

```bash
# Check HPA status
kubectl get hpa themisdb-hpa

# Watch HPA in real-time
kubectl get hpa themisdb-hpa --watch

# Detailed HPA information
kubectl describe hpa themisdb-hpa

# View HPA events
kubectl get events --field-selector involvedObject.name=themisdb-hpa
```

**Expected Output:**
```
NAME            REFERENCE             TARGETS              MINPODS   MAXPODS   REPLICAS
themisdb-hpa    Deployment/themisdb   45%/70%, 60%/80%    2         10        3
```

---

## Vertical Pod Autoscaling (VPA)

### VPA Setup

**Install VPA:**
```bash
git clone https://github.com/kubernetes/autoscaler.git
cd autoscaler/vertical-pod-autoscaler
./hack/vpa-up.sh
```

**Deploy VPA for ThemisDB (Helm):**
```bash
helm install themisdb ./helm/themisdb \
  --set vpa.enabled=true \
  --set vpa.updateMode=Auto
```

**Deploy VPA for ThemisDB (kubectl):**
```bash
kubectl apply -f deploy/kubernetes/examples/vpa.yaml
```

### VPA Configuration

```yaml
# vpa.yaml
apiVersion: autoscaling.k8s.io/v1
kind: VerticalPodAutoscaler
metadata:
  name: themisdb-vpa
spec:
  targetRef:
    apiVersion: apps/v1
    kind: Deployment
    name: themisdb
  
  updatePolicy:
    updateMode: "Auto"  # Options: Off, Initial, Recreate, Auto
  
  resourcePolicy:
    containerPolicies:
      - containerName: themisdb
        minAllowed:
          cpu: 500m
          memory: 1Gi
        maxAllowed:
          cpu: 8
          memory: 32Gi
        controlledResources: ["cpu", "memory"]
        controlledValues: RequestsAndLimits
```

### VPA Update Modes

| Mode | Description | When to Use |
|------|-------------|-------------|
| **Off** | Only recommendations, no updates | Testing VPA recommendations |
| **Initial** | Apply recommendations on pod creation only | Conservative approach |
| **Recreate** | Evict and recreate pods with new resources | Acceptable downtime |
| **Auto** | Like Recreate, but respects PDBs | Production (with PDB) |

### VPA Recommendations

```bash
# View VPA recommendations
kubectl describe vpa themisdb-vpa

# Get recommendations in JSON
kubectl get vpa themisdb-vpa -o jsonpath='{.status.recommendation}'
```

**Example Output:**
```yaml
status:
  recommendation:
    containerRecommendations:
    - containerName: themisdb
      lowerBound:
        cpu: 1
        memory: 2Gi
      target:
        cpu: 2
        memory: 4Gi
      upperBound:
        cpu: 4
        memory: 8Gi
```

---

## Load Balancer Integration

### NGINX Ingress

**Install NGINX Ingress Controller:**
```bash
helm repo add ingress-nginx https://kubernetes.github.io/ingress-nginx
helm install nginx-ingress ingress-nginx/ingress-nginx \
  --set controller.metrics.enabled=true \
  --set controller.podAnnotations."prometheus\.io/scrape"=true
```

**Deploy with Load Balancer:**
```bash
kubectl apply -f deploy/kubernetes/examples/load-balancer.yaml
```

**Configuration:**
```yaml
apiVersion: networking.k8s.io/v1
kind: Ingress
metadata:
  name: themisdb-ingress
  annotations:
    # Load balancing algorithm
    nginx.ingress.kubernetes.io/load-balance: "least_conn"
    
    # Session affinity
    nginx.ingress.kubernetes.io/affinity: "cookie"
    nginx.ingress.kubernetes.io/session-cookie-name: "themisdb-session"
    
    # Rate limiting
    nginx.ingress.kubernetes.io/limit-rps: "100"
    nginx.ingress.kubernetes.io/limit-connections: "50"
spec:
  ingressClassName: nginx
  rules:
    - host: themisdb.example.com
      http:
        paths:
          - path: /
            pathType: Prefix
            backend:
              service:
                name: themisdb
                port:
                  number: 8080
```

### Cloud Provider Load Balancers

**AWS Network Load Balancer:**
```yaml
apiVersion: v1
kind: Service
metadata:
  name: themisdb-lb
  annotations:
    service.beta.kubernetes.io/aws-load-balancer-type: "nlb"
    service.beta.kubernetes.io/aws-load-balancer-cross-zone-load-balancing-enabled: "true"
spec:
  type: LoadBalancer
  ports:
    - port: 80
      targetPort: 8080
  selector:
    app: themisdb
```

**GCP Load Balancer:**
```yaml
apiVersion: v1
kind: Service
metadata:
  name: themisdb-lb
  annotations:
    cloud.google.com/load-balancer-type: "Internal"
spec:
  type: LoadBalancer
  loadBalancerIP: 10.0.0.100
  ports:
    - port: 80
      targetPort: 8080
  selector:
    app: themisdb
```

---

## GPU-Aware Scaling

### Challenges

- GPUs are expensive and scarce
- GPU pod startup time is significant (1-2 minutes)
- GPU memory is fixed per device
- GPU workloads benefit from batching

### Strategy

Use a combination of:
1. **Request queue monitoring** - Scale before saturation
2. **Conservative scale-down** - Avoid thrashing
3. **Pod Disruption Budgets** - Maintain availability

### GPU HPA Configuration

```yaml
apiVersion: autoscaling/v2
kind: HorizontalPodAutoscaler
metadata:
  name: themisdb-gpu-hpa
spec:
  scaleTargetRef:
    apiVersion: apps/v1
    kind: Deployment
    name: themisdb-gpu
  
  minReplicas: 1
  maxReplicas: 5  # Limited by GPU availability
  
  metrics:
    # GPU utilization
    - type: Pods
      pods:
        metric:
          name: themisdb_gpu_utilization_percent
        target:
          type: AverageValue
          averageValue: "85"
    
    # Request queue depth (leading indicator)
    - type: Pods
      pods:
        metric:
          name: themisdb_request_queue_depth
        target:
          type: AverageValue
          averageValue: "20"
  
  behavior:
    scaleDown:
      stabilizationWindowSeconds: 600  # 10 min (conservative)
      policies:
        - type: Pods
          value: 1  # Remove only 1 GPU pod at a time
          periodSeconds: 120
    
    scaleUp:
      stabilizationWindowSeconds: 30  # Quick scale up
      policies:
        - type: Pods
          value: 1  # Add 1 GPU pod at a time
          periodSeconds: 30
```

### Pod Disruption Budget

```yaml
apiVersion: policy/v1
kind: PodDisruptionBudget
metadata:
  name: themisdb-pdb
spec:
  minAvailable: 1  # Keep at least 1 pod running
  selector:
    matchLabels:
      app: themisdb
```

### Node Autoscaling with GPU

**GKE Autopilot / EKS with Karpenter:**
```yaml
# Karpenter Provisioner for GPU nodes
apiVersion: karpenter.sh/v1alpha5
kind: Provisioner
metadata:
  name: gpu-provisioner
spec:
  requirements:
    - key: node.kubernetes.io/instance-type
      operator: In
      values: ["p3.2xlarge", "p3.8xlarge"]  # AWS GPU instances
    
    - key: karpenter.sh/capacity-type
      operator: In
      values: ["spot", "on-demand"]
  
  limits:
    resources:
      nvidia.com/gpu: 32  # Max 32 GPUs
  
  ttlSecondsAfterEmpty: 600  # Wait 10 min before removing empty nodes
```

---

## Best Practices

### General Guidelines

1. **Start Conservative**
   - Begin with higher min replicas
   - Use longer stabilization windows
   - Monitor for a week before adjusting

2. **Set Appropriate Thresholds**
   - CPU: 60-70% for stateless apps
   - Memory: 70-80% (avoid OOM)
   - Custom metrics: Based on SLA targets

3. **Use Multiple Metrics**
   - Combine CPU/memory with custom metrics
   - Use leading indicators (queue depth) for faster response

4. **Implement Pod Disruption Budgets**
   - Prevent too many pods from being evicted
   - Maintain service availability during scaling

### Scaling Behavior

**Scale-Up:**
- Be aggressive (quick response to increased load)
- Use short stabilization windows (0-30s)
- Allow large percentage increases (100%+)

**Scale-Down:**
- Be conservative (avoid thrashing)
- Use long stabilization windows (5-10 min)
- Limit rate of decrease (25-50%)

### Resource Requests and Limits

```yaml
resources:
  requests:
    cpu: "1"      # HPA uses this for % calculation
    memory: "2Gi"
  limits:
    cpu: "2"      # Allow bursting
    memory: "4Gi" # Prevent OOM
```

**Guidelines:**
- Set requests based on typical usage
- Set limits 1.5-2x requests
- VPA will adjust these over time

---

## Troubleshooting

### HPA Not Scaling

**Check metrics availability:**
```bash
kubectl get --raw /apis/metrics.k8s.io/v1beta1/nodes
kubectl top nodes
kubectl top pods -n themisdb
```

**Check HPA status:**
```bash
kubectl describe hpa themisdb-hpa

# Look for errors in conditions:
# - AbleToScale: False - HPA can't scale
# - ScalingActive: False - Metrics not available
# - ScalingLimited: True - At min/max replicas
```

**Common Issues:**

1. **Metrics Server not installed**
   ```bash
   kubectl get deployment metrics-server -n kube-system
   ```

2. **Invalid resource requests**
   - HPA requires CPU/memory requests to be set
   - Check pod spec for resource requests

3. **Custom metrics not available**
   ```bash
   kubectl get apiservice v1beta1.custom.metrics.k8s.io
   kubectl get --raw /apis/custom.metrics.k8s.io/v1beta1
   ```

### Scaling Thrashing

**Symptoms:**
- Frequent scale up/down cycles
- Pods constantly being created/terminated

**Solutions:**

1. **Increase stabilization windows:**
   ```yaml
   behavior:
     scaleDown:
       stabilizationWindowSeconds: 600  # Increase to 10 min
   ```

2. **Adjust thresholds:**
   - Lower scale-up threshold
   - Raise scale-down threshold
   - Create hysteresis

3. **Use min replicas buffer:**
   - Set minReplicas higher than typical load
   - Reduces frequency of scaling events

### GPU Pods Not Scaling

**Check GPU availability:**
```bash
kubectl describe nodes | grep -A 5 "nvidia.com/gpu"
```

**Check resource requests:**
```yaml
resources:
  limits:
    nvidia.com/gpu: 1
```

**Check node taints:**
```bash
kubectl get nodes -o json | jq '.items[].spec.taints'
```

---

## Monitoring and Observability

### Key Metrics to Monitor

```promql
# Current replicas
kube_deployment_status_replicas{deployment="themisdb"}

# Desired replicas (HPA target)
kube_hpa_status_desired_replicas{hpa="themisdb-hpa"}

# Scaling events
rate(kube_hpa_status_condition{condition="ScalingLimited"}[5m])

# Resource utilization vs target
kube_hpa_status_current_metrics_average_utilization /
kube_hpa_spec_target_metric
```

### Grafana Dashboard

Create dashboard with panels:
1. Current vs Desired Replicas
2. Resource Utilization (CPU/Memory)
3. Custom Metrics Trends
4. Scaling Events Timeline
5. Cost Impact (replicas × cost per pod)

---

## Examples

### Complete Setup Example

```bash
# 1. Deploy ThemisDB
helm install themisdb ./helm/themisdb \
  --set autoscaling.enabled=true \
  --set autoscaling.minReplicas=2 \
  --set autoscaling.maxReplicas=10

# 2. Verify deployment
kubectl get deployment themisdb
kubectl get hpa themisdb

# 3. Generate load to test scaling
kubectl run -it --rm load-generator --image=busybox --restart=Never -- /bin/sh
# Inside pod:
while true; do wget -q -O- http://themisdb:8080/v1/inference; done

# 4. Watch scaling in action
kubectl get hpa themisdb --watch

# 5. Check scaling events
kubectl get events --sort-by='.lastTimestamp' | grep themisdb-hpa
```

---

## Related Documentation

- [Deployment Guide](DEPLOYMENT.md)
- [Monitoring Guide](MONITORING.md)
- [SLA Monitoring](SLA_MONITORING.md)
- [Kubernetes Examples](../deploy/kubernetes/examples/)

---

**Document Version:** 1.0  
**Last Updated:** April 2026  
**Next Review:** April 2026
