> **Aktueller Build-Flow:** `cmake --preset linux-ninja-release && cmake --build --preset linux-ninja-release`

# Production Deployment Guide - IoT & Drone Examples

This guide provides step-by-step instructions for deploying the IoT Sensor Network and Drone Image Analysis examples in a production environment.

## Table of Contents
1. [Prerequisites](#prerequisites)
2. [Infrastructure Setup](#infrastructure-setup)
3. [IoT Sensor Network Deployment](#iot-sensor-network-deployment)
4. [Drone Image Analysis Deployment](#drone-image-analysis-deployment)
5. [Monitoring & Operations](#monitoring--operations)
6. [Scaling Strategies](#scaling-strategies)
7. [Security Hardening](#security-hardening)
8. [Troubleshooting](#troubleshooting)

## Prerequisites

### Hardware Requirements

#### IoT Sensor Network
- **Minimum**: 4 CPU cores, 8GB RAM, 100GB SSD
- **Recommended**: 8 CPU cores, 16GB RAM, 500GB NVMe SSD
- **Production**: 16 CPU cores, 32GB RAM, 1TB NVMe SSD

#### Drone Image Analysis
- **Minimum**: 8 CPU cores, 16GB RAM, 500GB SSD, GPU with 8GB VRAM
- **Recommended**: 16 CPU cores, 32GB RAM, 1TB NVMe SSD, GPU with 16GB VRAM
- **Production**: 32 CPU cores, 64GB RAM, 2TB NVMe SSD, GPU with 24GB VRAM

### Software Requirements
- Ubuntu 22.04 LTS or later (recommended)
- Docker 24.0+ and Docker Compose 2.20+
- Python 3.10+
- ThemisDB 1.3.0+
- MQTT Broker (Mosquitto or HiveMQ)
- NVIDIA Driver 525+ (for GPU acceleration)

## Infrastructure Setup

### 1. ThemisDB Installation

```bash
# Install ThemisDB from source
git clone https://github.com/makr-code/ThemisDB.git
cd ThemisDB

# Build with all features enabled
cmake -B build \
  -DTHEMIS_ENABLE_LLM=ON \
  -DTHEMIS_ENABLE_GPU=ON \
  -DTHEMIS_ENABLE_CUDA=ON \
  -DTHEMIS_ENABLE_VISION=ON \
  -DTHEMIS_BUILD_TESTS=OFF
<!-- Legacy: prefer cmake --preset -->

cmake --build build --parallel $(nproc)
sudo cmake --install build
```

### 2. MQTT Broker Setup (for IoT)

```bash
# Install Mosquitto
sudo apt-get update
sudo apt-get install -y mosquitto mosquitto-clients

# Configure for production
sudo cat > /etc/mosquitto/conf.d/production.conf << 'EOF'
# Network settings
listener 1883 0.0.0.0
listener 8883 0.0.0.0
protocol mqtt

# Security
allow_anonymous false
password_file /etc/mosquitto/passwd

# TLS/SSL
cafile /etc/mosquitto/certs/ca.crt
certfile /etc/mosquitto/certs/server.crt
keyfile /etc/mosquitto/certs/server.key
require_certificate false

# Performance
max_connections 10000
max_queued_messages 1000
persistence true
persistence_location /var/lib/mosquitto/
EOF

# Create users
sudo mosquitto_passwd -c /etc/mosquitto/passwd iot_gateway
sudo mosquitto_passwd /etc/mosquitto/passwd sensor_client

# Restart service
sudo systemctl restart mosquitto
sudo systemctl enable mosquitto
```

### 3. Docker Compose Setup

Create `docker-compose.yml`:

```yaml
version: '3.8'

services:
  themisdb:
    image: themisdb:latest
    container_name: themisdb-production
    ports:
      - "18765:18765"  # HTTP API
      - "19876:19876"  # gRPC
    volumes:
      - themisdb-data:/var/lib/themisdb
      - ./config:/etc/themisdb
    environment:
      - THEMIS_ENABLE_GPU=true
      - THEMIS_ENABLE_LLM=true
      - CUDA_VISIBLE_DEVICES=0
    deploy:
      resources:
        reservations:
          devices:
            - driver: nvidia
              count: 1
              capabilities: [gpu]
    restart: unless-stopped

  mosquitto:
    image: eclipse-mosquitto:2.0
    container_name: mqtt-broker
    ports:
      - "1883:1883"
      - "8883:8883"
    volumes:
      - ./mosquitto/config:/mosquitto/config
      - ./mosquitto/data:/mosquitto/data
      - ./mosquitto/log:/mosquitto/log
      - ./certs:/mosquitto/certs
    restart: unless-stopped

  prometheus:
    image: prom/prometheus:latest
    container_name: prometheus
    ports:
      - "9090:9090"
    volumes:
      - ./prometheus.yml:/etc/prometheus/prometheus.yml
      - prometheus-data:/prometheus
    command:
      - '--config.file=/etc/prometheus/prometheus.yml'
      - '--storage.tsdb.path=/prometheus'
    restart: unless-stopped

  grafana:
    image: grafana/grafana:latest
    container_name: grafana
    ports:
      - "3000:3000"
    volumes:
      - grafana-data:/var/lib/grafana
      - ./grafana/dashboards:/etc/grafana/provisioning/dashboards
    environment:
      - GF_SECURITY_ADMIN_PASSWORD=changeme
    restart: unless-stopped

volumes:
  themisdb-data:
  prometheus-data:
  grafana-data:
```

## IoT Sensor Network Deployment

### 1. Configuration

Create `config/iot_config.yaml`:

```yaml
mqtt:
  broker: "mqtt-broker"
  port: 1883
  username: "iot_gateway"
  password: "${MQTT_PASSWORD}"
  tls: true
  ca_cert: "/etc/certs/ca.crt"

themisdb:
  endpoint: "http://themisdb-production:18765"
  database: "iot_production"
  auth:
    username: "iot_service"
    password: "${THEMISDB_PASSWORD}"

sensors:
  simulation: false  # Set to false in production
  mqtt_topics:
    - "sensors/temperature/#"
    - "sensors/humidity/#"
    - "sensors/pressure/#"
    - "sensors/motion/#"

processing:
  batch_size: 100
  flush_interval_ms: 1000
  enable_ml_anomaly_detection: true
  ml_model_path: "/models/anomaly_detector.pkl"

cep:
  enabled: true
  patterns:
    - name: "high_temperature_sustained"
      condition: "temperature > 45 for 10 minutes"
      severity: "critical"
      actions: ["email", "sms", "webhook"]
    
    - name: "sensor_offline"
      condition: "no data for 5 minutes"
      severity: "warning"
      actions: ["email"]

alerts:
  email:
    smtp_host: "smtp.gmail.com"
    smtp_port: 587
    from_address: "alerts@example.com"
    to_addresses: ["ops@example.com"]
  
  webhook:
    url: "https://hooks.slack.com/services/YOUR/WEBHOOK/URL"
    method: "POST"

monitoring:
  prometheus_port: 9100
  metrics_interval_seconds: 15
```

### 2. Deploy Application

```bash
# Clone repository
git clone https://github.com/makr-code/ThemisDB.git
cd ThemisDB/examples/09_iot_sensor_network

# Create virtual environment
python3 -m venv venv
source venv/bin/activate

# Install dependencies
pip install -r requirements.txt

# Set environment variables
export MQTT_PASSWORD="your_secure_password"
export THEMISDB_PASSWORD="your_secure_password"

# Train ML model (first time only)
python train_anomaly_detector.py

# Run in production mode
python main.py --config /path/to/iot_config.yaml --production
```

### 3. Systemd Service

Create `/etc/systemd/system/iot-sensor-network.service`:

```ini
[Unit]
Description=IoT Sensor Network Service
After=network.target docker.service
Requires=docker.service

[Service]
Type=simple
User=themis
WorkingDirectory=/opt/themisdb/examples/09_iot_sensor_network
Environment="MQTT_PASSWORD=your_password"
Environment="THEMISDB_PASSWORD=your_password"
ExecStart=/opt/themisdb/examples/09_iot_sensor_network/venv/bin/python main.py --config /etc/themis/iot_config.yaml --production
Restart=always
RestartSec=10

[Install]
WantedBy=multi-user.target
```

Enable and start:
```bash
sudo systemctl daemon-reload
sudo systemctl enable iot-sensor-network
sudo systemctl start iot-sensor-network
```

## Drone Image Analysis Deployment

### 1. Configuration

Create `config/drone_config.yaml`:

```yaml
themisdb:
  endpoint: "http://themisdb-production:18765"
  database: "drone_production"
  auth:
    username: "drone_service"
    password: "${THEMISDB_PASSWORD}"

storage:
  images_path: "/data/drone_images"
  thumbnails_path: "/data/drone_thumbnails"
  max_image_size_mb: 50
  compression_quality: 85

processing:
  gpu_enabled: true
  gpu_device_id: 0
  batch_size: 8
  num_workers: 4
  
  backends:
    yolo:
      model_path: "/models/yolov8n.pt"
      confidence_threshold: 0.5
      nms_threshold: 0.4
    
    clip:
      model_name: "ViT-B/32"
      embedding_dim: 512
    
    llm:
      model_path: "/models/llava-7b-q4_0.gguf"
      context_size: 2048
      gpu_layers: 35

geo:
  enable_spatial_index: true
  default_srid: 4326  # WGS84
  enable_h3_indexing: true
  h3_resolution: 9

analysis:
  enable_object_detection: true
  enable_scene_classification: true
  enable_llm_descriptions: true
  enable_change_detection: true
  
  object_classes:
    - "vehicle"
    - "building"
    - "person"
    - "tree"
    - "road"
    - "water"

monitoring:
  prometheus_port: 9101
  metrics_interval_seconds: 15
  log_level: "INFO"
```

### 2. Model Download

```bash
# Download YOLO model
wget https://github.com/ultralytics/assets/releases/download/v0.0.0/yolov8n.pt \
  -O /models/yolov8n.pt

# Download LLaVA model (example)
wget https://huggingface.co/mys/ggml_llava-v1.5-7b/resolve/main/ggml-model-q4_0.gguf \
  -O /models/llava-7b-q4_0.gguf
```

### 3. Deploy Application

```bash
cd ThemisDB/examples/10_drone_image_analysis

# Create virtual environment
python3 -m venv venv
source venv/bin/activate

# Install dependencies
pip install -r requirements.txt

# Set environment variables
export THEMISDB_PASSWORD="your_secure_password"

# Run in production mode
python main.py --config /path/to/drone_config.yaml --production --no-gui
```

### 4. API Server Mode

For production, run as headless API server:

```python
# api_server.py
from flask import Flask, request, jsonify
from drone_analyzer import DroneAnalyzer
import os

app = Flask(__name__)
analyzer = DroneAnalyzer(config_path="/etc/themis/drone_config.yaml")

@app.route('/api/analyze', methods=['POST'])
def analyze_image():
    if 'image' not in request.files:
        return jsonify({'error': 'No image provided'}), 400
    
    image_file = request.files['image']
    lat = float(request.form.get('lat', 0))
    lon = float(request.form.get('lon', 0))
    altitude = float(request.form.get('altitude', 0))
    
    result = analyzer.analyze_image(
        image_file,
        location={'lat': lat, 'lon': lon, 'altitude': altitude}
    )
    
    return jsonify(result)

@app.route('/api/search', methods=['POST'])
def search_images():
    query = request.json.get('query', '')
    lat = request.json.get('lat')
    lon = request.json.get('lon')
    radius_km = request.json.get('radius_km', 10)
    
    results = analyzer.search_similar(
        query=query,
        location={'lat': lat, 'lon': lon} if lat and lon else None,
        radius_km=radius_km
    )
    
    return jsonify(results)

if __name__ == '__main__':
    app.run(host='0.0.0.0', port=5000, threaded=True)
```

## Monitoring & Operations

### 1. Prometheus Metrics

Create `prometheus.yml`:

```yaml
global:
  scrape_interval: 15s
  evaluation_interval: 15s

scrape_configs:
  - job_name: 'themisdb'
    static_configs:
      - targets: ['themisdb-production:9090']
  
  - job_name: 'iot_sensors'
    static_configs:
      - targets: ['localhost:9100']
  
  - job_name: 'drone_analysis'
    static_configs:
      - targets: ['localhost:9101']
```

### 2. Grafana Dashboards

Import pre-built dashboards from `grafana/dashboards/`:
- `iot_sensor_dashboard.json` - IoT sensor metrics
- `drone_analysis_dashboard.json` - Drone processing metrics
- `themisdb_performance.json` - Database performance

### 3. Log Aggregation

Use Loki for log aggregation:

```yaml
# docker-compose.yml (add to existing)
  loki:
    image: grafana/loki:latest
    ports:
      - "3100:3100"
    volumes:
      - ./loki-config.yaml:/etc/loki/local-config.yaml
      - loki-data:/loki
    restart: unless-stopped

  promtail:
    image: grafana/promtail:latest
    volumes:
      - /var/log:/var/log
      - ./promtail-config.yaml:/etc/promtail/config.yml
    restart: unless-stopped
```

## Scaling Strategies

### Horizontal Scaling

1. **Load Balancer Setup**
```nginx
# nginx.conf
upstream themisdb_cluster {
    least_conn;
    server themisdb-node1:18765;
    server themisdb-node2:18765;
    server themisdb-node3:18765;
}

server {
    listen 80;
    location / {
        proxy_pass http://themisdb_cluster;
    }
}
```

2. **Sharding Configuration**
```yaml
# sharding_config.yaml
shards:
  - id: "shard1"
    range: [0, 1000000]
    endpoint: "themisdb-shard1:18765"
  
  - id: "shard2"
    range: [1000001, 2000000]
    endpoint: "themisdb-shard2:18765"
  
  - id: "shard3"
    range: [2000001, 3000000]
    endpoint: "themisdb-shard3:18765"
```

### Vertical Scaling

- Increase CPU/RAM allocation
- Add GPU nodes for ML workloads
- Use NVMe SSDs for storage

## Security Hardening

### 1. Network Security

```bash
# Firewall rules
sudo ufw allow 22/tcp    # SSH
sudo ufw allow 18765/tcp # ThemisDB
sudo ufw allow 8883/tcp  # MQTT TLS
sudo ufw deny 1883/tcp   # Block non-TLS MQTT
sudo ufw enable
```

### 2. TLS/SSL Certificates

```bash
# Generate certificates using Let's Encrypt
sudo certbot certonly --standalone -d themisdb.example.com
sudo certbot certonly --standalone -d mqtt.example.com
```

### 3. Authentication

- Use strong passwords (min 20 characters)
- Enable 2FA for admin access
- Rotate API keys monthly
- Use JWT tokens with short expiry

## Troubleshooting

### Common Issues

1. **High Memory Usage**
   - Reduce batch sizes
   - Enable memory limits in Docker
   - Use memory profiling tools

2. **GPU Out of Memory**
   - Reduce model size (use quantized models)
   - Decrease batch size
   - Enable gradient checkpointing

3. **MQTT Connection Issues**
   - Check broker logs: `journalctl -u mosquitto`
   - Verify TLS certificates
   - Test with `mosquitto_sub` client

4. **Slow Image Processing**
   - Check GPU utilization: `nvidia-smi`
   - Enable batch processing
   - Use async processing queue

### Health Checks

```bash
# ThemisDB health
curl http://localhost:18765/health

# MQTT broker status
sudo systemctl status mosquitto

# Check logs
journalctl -u iot-sensor-network -f
journalctl -u drone-analysis -f
```

## Backup & Recovery

```bash
# Backup ThemisDB data
themisdb backup create --name production-$(date +%Y%m%d)

# Upload to cloud storage
aws s3 cp /var/lib/themisdb/backups/ s3://backups/themisdb/ --recursive

# Restore from backup
themisdb backup restore --name production-20250207
```

## Production Checklist

- [ ] ThemisDB installed and configured
- [ ] MQTT broker secured with TLS
- [ ] Monitoring stack deployed (Prometheus + Grafana)
- [ ] Log aggregation configured
- [ ] Backups scheduled
- [ ] Security hardening applied
- [ ] Load testing completed
- [ ] Documentation updated
- [ ] Runbook created
- [ ] On-call rotation defined

---

**Last Updated**: February 7, 2026  
**Version**: 1.0  
**Maintainer**: ThemisDB Team
