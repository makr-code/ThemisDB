# Load Balancer Integration Guide

**Version:** 1.0  
**Last Updated:** April 2026  
**Target Audience:** Platform Engineers, DevOps

---

## Overview

This guide provides configurations for integrating ThemisDB with various load balancers in Kubernetes environments.

---

## Table of Contents

1. [Load Balancer Options](#load-balancer-options)
2. [NGINX Ingress Controller](#nginx-ingress-controller)
3. [AWS Application Load Balancer](#aws-application-load-balancer)
4. [Google Cloud Load Balancer](#google-cloud-load-balancer)
5. [Istio Service Mesh](#istio-service-mesh)
6. [HAProxy](#haproxy)
7. [Health Checks](#health-checks)
8. [Session Affinity](#session-affinity)
9. [TLS Termination](#tls-termination)
10. [Monitoring & Metrics](#monitoring--metrics)

---

## Load Balancer Options

### Comparison Matrix

| Load Balancer | Layer | TLS | Auto-scaling | Complexity | Best For |
|---------------|-------|-----|--------------|------------|----------|
| NGINX Ingress | L7 | ✅ | ✅ | Low | Kubernetes-native |
| AWS ALB | L7 | ✅ | ✅ | Low | AWS EKS |
| GCP Load Balancer | L7 | ✅ | ✅ | Low | Google GKE |
| Istio | L7 | ✅ | ✅ | High | Service mesh |
| HAProxy | L4/L7 | ✅ | ⚠️ | Medium | On-premise |

---

## NGINX Ingress Controller

### Installation

```bash
# Install NGINX Ingress Controller
helm repo add ingress-nginx https://kubernetes.github.io/ingress-nginx
helm repo update

helm install ingress-nginx ingress-nginx/ingress-nginx \
  --namespace ingress-nginx \
  --create-namespace \
  --set controller.service.type=LoadBalancer \
  --set controller.metrics.enabled=true \
  --set controller.podAnnotations."prometheus\.io/scrape"="true"
```

### Ingress Configuration

```yaml
# themisdb-ingress.yaml
apiVersion: networking.k8s.io/v1
kind: Ingress
metadata:
  name: themisdb-ingress
  namespace: production
  annotations:
    # Basic configuration
    kubernetes.io/ingress.class: nginx
    nginx.ingress.kubernetes.io/rewrite-target: /
    
    # TLS configuration
    nginx.ingress.kubernetes.io/ssl-redirect: "true"
    nginx.ingress.kubernetes.io/force-ssl-redirect: "true"
    
    # Health check configuration
    nginx.ingress.kubernetes.io/health-check-path: /health
    nginx.ingress.kubernetes.io/health-check-interval: "10s"
    nginx.ingress.kubernetes.io/health-check-timeout: "5s"
    
    # Connection settings
    nginx.ingress.kubernetes.io/proxy-connect-timeout: "60"
    nginx.ingress.kubernetes.io/proxy-send-timeout: "60"
    nginx.ingress.kubernetes.io/proxy-read-timeout: "60"
    nginx.ingress.kubernetes.io/proxy-body-size: "10m"
    
    # Session affinity (sticky sessions)
    nginx.ingress.kubernetes.io/affinity: "cookie"
    nginx.ingress.kubernetes.io/session-cookie-name: "themisdb-session"
    nginx.ingress.kubernetes.io/session-cookie-expires: "3600"
    nginx.ingress.kubernetes.io/session-cookie-max-age: "3600"
    
    # Rate limiting
    nginx.ingress.kubernetes.io/limit-rps: "100"
    nginx.ingress.kubernetes.io/limit-connections: "20"
    
    # CORS (if needed)
    nginx.ingress.kubernetes.io/enable-cors: "true"
    nginx.ingress.kubernetes.io/cors-allow-origin: "https://example.com"
    
spec:
  tls:
  - hosts:
    - themisdb.example.com
    secretName: themisdb-tls
  rules:
  - host: themisdb.example.com
    http:
      paths:
      # Main API
      - path: /
        pathType: Prefix
        backend:
          service:
            name: themisdb
            port:
              number: 8080
      # Metrics endpoint (optional, can be separate)
      - path: /metrics
        pathType: Exact
        backend:
          service:
            name: themisdb
            port:
              number: 4318
```

### Apply Configuration

```bash
kubectl apply -f themisdb-ingress.yaml

# Verify ingress created
kubectl get ingress themisdb-ingress -n production

# Check ingress details
kubectl describe ingress themisdb-ingress -n production
```

---

## AWS Application Load Balancer

### Prerequisites

```bash
# Install AWS Load Balancer Controller
helm repo add eks https://aws.github.io/eks-charts
helm repo update

helm install aws-load-balancer-controller eks/aws-load-balancer-controller \
  --namespace kube-system \
  --set clusterName=<your-cluster-name> \
  --set serviceAccount.create=false \
  --set serviceAccount.name=aws-load-balancer-controller
```

### Ingress Configuration

```yaml
# themisdb-alb.yaml
apiVersion: networking.k8s.io/v1
kind: Ingress
metadata:
  name: themisdb-alb
  namespace: production
  annotations:
    # ALB configuration
    kubernetes.io/ingress.class: alb
    alb.ingress.kubernetes.io/scheme: internet-facing
    alb.ingress.kubernetes.io/target-type: ip
    
    # TLS
    alb.ingress.kubernetes.io/listen-ports: '[{"HTTP": 80}, {"HTTPS": 443}]'
    alb.ingress.kubernetes.io/ssl-redirect: "443"
    alb.ingress.kubernetes.io/certificate-arn: arn:aws:acm:region:account:certificate/xxx
    
    # Health check
    alb.ingress.kubernetes.io/healthcheck-path: /health
    alb.ingress.kubernetes.io/healthcheck-interval-seconds: "15"
    alb.ingress.kubernetes.io/healthcheck-timeout-seconds: "5"
    alb.ingress.kubernetes.io/healthy-threshold-count: "2"
    alb.ingress.kubernetes.io/unhealthy-threshold-count: "2"
    
    # Target group attributes
    alb.ingress.kubernetes.io/target-group-attributes: |
      stickiness.enabled=true,
      stickiness.lb_cookie.duration_seconds=3600,
      deregistration_delay.timeout_seconds=30
    
    # Security groups
    alb.ingress.kubernetes.io/security-groups: sg-xxxxx
    
    # Tags
    alb.ingress.kubernetes.io/tags: Environment=production,Application=themisdb
    
spec:
  tls:
  - hosts:
    - themisdb.example.com
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

### Apply Configuration

```bash
kubectl apply -f themisdb-alb.yaml

# Check ALB status
kubectl get ingress themisdb-alb -n production

# Get ALB DNS name
kubectl get ingress themisdb-alb -n production -o jsonpath='{.status.loadBalancer.ingress[0].hostname}'
```

---

## Google Cloud Load Balancer

### Ingress Configuration

```yaml
# themisdb-gclb.yaml
apiVersion: networking.k8s.io/v1
kind: Ingress
metadata:
  name: themisdb-gclb
  namespace: production
  annotations:
    # GCP configuration
    kubernetes.io/ingress.class: gce
    kubernetes.io/ingress.global-static-ip-name: themisdb-ip
    
    # TLS
    ingress.gcp.kubernetes.io/pre-shared-cert: themisdb-cert
    
    # Backend configuration
    cloud.google.com/backend-config: themisdb-backend-config
    
spec:
  tls:
  - secretName: themisdb-tls
    hosts:
    - themisdb.example.com
  rules:
  - host: themisdb.example.com
    http:
      paths:
      - path: /*
        pathType: ImplementationSpecific
        backend:
          service:
            name: themisdb
            port:
              number: 8080
---
# Backend configuration for GCP
apiVersion: cloud.google.com/v1
kind: BackendConfig
metadata:
  name: themisdb-backend-config
  namespace: production
spec:
  healthCheck:
    checkIntervalSec: 10
    timeoutSec: 5
    healthyThreshold: 2
    unhealthyThreshold: 3
    type: HTTP
    requestPath: /health
    port: 8080
  
  sessionAffinity:
    affinityType: "CLIENT_IP"
    affinityCookieTtlSec: 3600
  
  timeoutSec: 60
  
  connectionDraining:
    drainingTimeoutSec: 30
  
  cdn:
    enabled: false
  
  iap:
    enabled: false
```

---

## Istio Service Mesh

### Installation

```bash
# Install Istio
istioctl install --set profile=production -y

# Label namespace for Istio injection
kubectl label namespace production istio-injection=enabled
```

### Gateway & VirtualService Configuration

```yaml
# themisdb-gateway.yaml
apiVersion: networking.istio.io/v1beta1
kind: Gateway
metadata:
  name: themisdb-gateway
  namespace: production
spec:
  selector:
    istio: ingressgateway
  servers:
  - port:
      number: 443
      name: https
      protocol: HTTPS
    tls:
      mode: SIMPLE
      credentialName: themisdb-tls
    hosts:
    - themisdb.example.com
  - port:
      number: 80
      name: http
      protocol: HTTP
    hosts:
    - themisdb.example.com
    tls:
      httpsRedirect: true
---
# themisdb-virtualservice.yaml
apiVersion: networking.istio.io/v1beta1
kind: VirtualService
metadata:
  name: themisdb-vs
  namespace: production
spec:
  hosts:
  - themisdb.example.com
  gateways:
  - themisdb-gateway
  http:
  - match:
    - uri:
        prefix: /
    route:
    - destination:
        host: themisdb.production.svc.cluster.local
        port:
          number: 8080
      weight: 100
    timeout: 60s
    retries:
      attempts: 3
      perTryTimeout: 20s
      retryOn: 5xx,reset,connect-failure,refused-stream
---
# themisdb-destinationrule.yaml
apiVersion: networking.istio.io/v1beta1
kind: DestinationRule
metadata:
  name: themisdb-dr
  namespace: production
spec:
  host: themisdb.production.svc.cluster.local
  trafficPolicy:
    connectionPool:
      tcp:
        maxConnections: 100
      http:
        http1MaxPendingRequests: 50
        http2MaxRequests: 100
        maxRequestsPerConnection: 2
    loadBalancer:
      consistentHash:
        httpCookie:
          name: themisdb-session
          ttl: 3600s
    outlierDetection:
      consecutiveErrors: 5
      interval: 30s
      baseEjectionTime: 30s
      maxEjectionPercent: 50
      minHealthPercent: 40
```

### Apply Configuration

```bash
kubectl apply -f themisdb-gateway.yaml
kubectl apply -f themisdb-virtualservice.yaml
kubectl apply -f themisdb-destinationrule.yaml

# Verify configuration
istioctl analyze -n production
```

---

## HAProxy

### Configuration File

```haproxy
# /etc/haproxy/haproxy.cfg

global
    log /dev/log local0
    log /dev/log local1 notice
    chroot /var/lib/haproxy
    stats socket /run/haproxy/admin.sock mode 660 level admin
    stats timeout 30s
    user haproxy
    group haproxy
    daemon
    
    # TLS configuration
    tune.ssl.default-dh-param 2048
    ssl-default-bind-ciphers ECDHE-ECDSA-AES128-GCM-SHA256:ECDHE-RSA-AES128-GCM-SHA256
    ssl-default-bind-options ssl-min-ver TLSv1.2 no-tls-tickets

defaults
    log     global
    mode    http
    option  httplog
    option  dontlognull
    option  http-server-close
    option  forwardfor except 127.0.0.0/8
    option  redispatch
    retries 3
    timeout connect 5000
    timeout client  60000
    timeout server  60000
    errorfile 400 /etc/haproxy/errors/400.http
    errorfile 403 /etc/haproxy/errors/403.http
    errorfile 408 /etc/haproxy/errors/408.http
    errorfile 500 /etc/haproxy/errors/500.http
    errorfile 502 /etc/haproxy/errors/502.http
    errorfile 503 /etc/haproxy/errors/503.http
    errorfile 504 /etc/haproxy/errors/504.http

# Stats page
listen stats
    bind *:8404
    stats enable
    stats uri /stats
    stats refresh 10s
    stats admin if TRUE

# Frontend (HTTPS)
frontend themisdb_https
    bind *:443 ssl crt /etc/haproxy/certs/themisdb.pem
    mode http
    
    # Redirect HTTP to HTTPS
    redirect scheme https code 301 if !{ ssl_fc }
    
    # Headers
    http-request set-header X-Forwarded-Proto https
    http-request set-header X-Forwarded-For %[src]
    
    # ACLs
    acl is_health path /health
    acl is_metrics path /metrics
    
    # Health check bypass (no sticky session)
    use_backend themisdb_health if is_health
    
    # Default backend with sticky sessions
    default_backend themisdb_backend

# Backend
backend themisdb_backend
    mode http
    balance roundrobin
    
    # Sticky sessions (cookie-based)
    cookie THEMISDB_SESSION insert indirect nocache
    
    # Health check
    option httpchk GET /health
    http-check expect status 200
    
    # Servers
    server themisdb-1 10.0.1.10:8080 check cookie s1 inter 10s fall 3 rise 2
    server themisdb-2 10.0.1.11:8080 check cookie s2 inter 10s fall 3 rise 2
    server themisdb-3 10.0.1.12:8080 check cookie s3 inter 10s fall 3 rise 2

# Health check backend (no sticky session)
backend themisdb_health
    mode http
    balance roundrobin
    
    option httpchk GET /health
    http-check expect status 200
    
    server themisdb-1 10.0.1.10:8080 check
    server themisdb-2 10.0.1.11:8080 check
    server themisdb-3 10.0.1.12:8080 check
```

---

## Health Checks

### Health Check Endpoint

ThemisDB exposes `/health` endpoint:

```bash
curl http://localhost:8080/health
```

**Response (Healthy)**:
```json
{
  "status": "healthy",
  "version": "1.5.0",
  "uptime_seconds": 86400,
  "checks": {
    "database": "ok",
    "storage": "ok",
    "replication": "ok"
  }
}
```

### Configure Health Check

**NGINX**:
```yaml
nginx.ingress.kubernetes.io/health-check-path: /health
nginx.ingress.kubernetes.io/health-check-interval: "10s"
```

**AWS ALB**:
```yaml
alb.ingress.kubernetes.io/healthcheck-path: /health
alb.ingress.kubernetes.io/healthcheck-interval-seconds: "15"
```

**Kubernetes Probes**:
```yaml
livenessProbe:
  httpGet:
    path: /health
    port: 8080
  initialDelaySeconds: 30
  periodSeconds: 10
  
readinessProbe:
  httpGet:
    path: /health
    port: 8080
  initialDelaySeconds: 5
  periodSeconds: 5
```

---

## Session Affinity

### Why Session Affinity?

For workloads that benefit from connection reuse or maintain in-memory state.

### NGINX Configuration

```yaml
nginx.ingress.kubernetes.io/affinity: "cookie"
nginx.ingress.kubernetes.io/session-cookie-name: "themisdb-session"
nginx.ingress.kubernetes.io/session-cookie-max-age: "3600"
```

### Istio Configuration

```yaml
trafficPolicy:
  loadBalancer:
    consistentHash:
      httpCookie:
        name: themisdb-session
        ttl: 3600s
```

---

## TLS Termination

### Create TLS Secret

```bash
# Create TLS secret from certificate files
kubectl create secret tls themisdb-tls \
  --cert=themisdb.crt \
  --key=themisdb.key \
  --namespace production

# Or from cert-manager
kubectl apply -f - <<EOF
apiVersion: cert-manager.io/v1
kind: Certificate
metadata:
  name: themisdb-cert
  namespace: production
spec:
  secretName: themisdb-tls
  issuerRef:
    name: letsencrypt-prod
    kind: ClusterIssuer
  dnsNames:
  - themisdb.example.com
EOF
```

---

## Monitoring & Metrics

### Prometheus Metrics

Configure ServiceMonitor for load balancer metrics:

```yaml
apiVersion: monitoring.coreos.com/v1
kind: ServiceMonitor
metadata:
  name: nginx-ingress
  namespace: ingress-nginx
spec:
  selector:
    matchLabels:
      app.kubernetes.io/name: ingress-nginx
  endpoints:
  - port: metrics
    interval: 30s
```

### Key Metrics to Monitor

- `nginx_ingress_controller_requests` - Request count
- `nginx_ingress_controller_request_duration_seconds` - Latency
- `nginx_ingress_controller_response_size_bytes` - Response size
- `nginx_ingress_controller_upstream_latency_seconds` - Backend latency

---

## Troubleshooting

### Check Load Balancer Status

```bash
# NGINX
kubectl get svc -n ingress-nginx
kubectl logs -n ingress-nginx -l app.kubernetes.io/name=ingress-nginx

# AWS ALB
aws elbv2 describe-load-balancers --names themisdb-alb
aws elbv2 describe-target-health --target-group-arn <arn>

# Istio
istioctl proxy-status
kubectl logs -n istio-system -l app=istio-ingressgateway
```

### Common Issues

**502 Bad Gateway**: Backend pods not ready
```bash
kubectl get pods -n production
kubectl describe pod themisdb-0 -n production
```

**504 Gateway Timeout**: Backend timeout
```bash
# Increase timeout in ingress annotations
nginx.ingress.kubernetes.io/proxy-read-timeout: "120"
```

---

**Document Version:** 1.0  
**Last Updated:** 2026-04-06
