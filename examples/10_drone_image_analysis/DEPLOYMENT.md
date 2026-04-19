# Deployment - Drohnenbild-Analyse

> **Historischer Stand:** 2026-01-31 — Inhalte nicht gegen aktuelle Quellen geprüft.

## 📋 Übersicht

Production Deployment Guide für das Drohnenbild-Analyse-System.

> **Note:** Current Docker image: `docker run -d -p 8080:8080 themisdb/themisdb:latest`
> <!-- TODO: verify against current source -->

## 🐳 Docker Setup

### Dockerfile

\`\`\`dockerfile
FROM nvidia/cuda:12.1.0-cudnn8-runtime-ubuntu22.04

# Install Python
RUN apt-get update && apt-get install -y \\
    python3.10 \\
    python3-pip \\
    libopencv-dev

# Install dependencies
COPY requirements.txt .
RUN pip3 install -r requirements.txt

# Copy application
COPY . /app
WORKDIR /app

# Download models
RUN python3 download_models.py

# Expose port
EXPOSE 8080

# Run
CMD ["python3", "main.py"]
\`\`\`

### Docker Compose

\`\`\`yaml
version: '3.8'

services:
  themisdb:
    image: themisdb:latest
    ports:
      - "8080:8080"
    volumes:
      - themisdb-data:/data
  
  drone-analyzer:
    build: .
    ports:
      - "5000:5000"
    environment:
      - THEMISDB_URL=http://themisdb:8080
      - CUDA_VISIBLE_DEVICES=0
    volumes:
      - ./images:/app/images
    deploy:
      resources:
        reservations:
          devices:
            - driver: nvidia
              count: 1
              capabilities: [gpu]

volumes:
  themisdb-data:
\`\`\`

## ☸️ Kubernetes Deployment

### Deployment YAML

\`\`\`yaml
apiVersion: apps/v1
kind: Deployment
metadata:
  name: drone-analyzer
spec:
  replicas: 3
  selector:
    matchLabels:
      app: drone-analyzer
  template:
    metadata:
      labels:
        app: drone-analyzer
    spec:
      containers:
      - name: analyzer
        image: drone-analyzer:latest
        ports:
        - containerPort: 5000
        resources:
          limits:
            nvidia.com/gpu: 1
            memory: "8Gi"
          requests:
            memory: "4Gi"
        env:
        - name: THEMISDB_URL
          value: "http://themisdb-service:8080"
\`\`\`

## 🔄 CI/CD Pipeline

### GitHub Actions

\`\`\`yaml
name: Deploy

on:
  push:
    branches: [main]

jobs:
  build-and-deploy:
    runs-on: ubuntu-latest
    steps:
      - uses: actions/checkout@v2
      
      - name: Build Docker image
        run: docker build -t drone-analyzer:latest .
      
      - name: Push to registry
        run: |
          docker push registry.example.com/drone-analyzer:latest
      
      - name: Deploy to K8s
        run: |
          kubectl apply -f k8s/deployment.yaml
\`\`\`

## 📊 Monitoring

### Prometheus Metrics

\`\`\`python
from prometheus_client import Counter, Histogram

image_processed_total = Counter(
    'images_processed_total',
    'Total images processed'
)

processing_duration = Histogram(
    'processing_duration_seconds',
    'Time spent processing image'
)
\`\`\`

## 🎓 Best Practices

1. **Security**
   - Verwende Secrets für API Keys
   - Aktiviere TLS
   - Implementiere Rate Limiting

2. **Scaling**
   - Horizontal Pod Autoscaling
   - Load Balancing
   - Caching Layer

3. **Monitoring**
   - Log Aggregation (ELK Stack)
   - Metrics (Prometheus + Grafana)
   - Alerting

## 📚 Weitere Dokumentation

- [ARCHITECTURE.md](ARCHITECTURE.md) - System-Design
- [PERFORMANCE_TUNING.md](PERFORMANCE_TUNING.md) - Optimierung
- [TROUBLESHOOTING.md](TROUBLESHOOTING.md) - Fehlerbehandlung
