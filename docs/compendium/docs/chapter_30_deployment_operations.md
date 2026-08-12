# Kapitel 30: Deployment & Operations

> *"Deployment ist kein einmaliger Akt, sondern ein kontinuierlicher Prozess."*

---

## Überblick

Dieses Kapitel behandelt die produktive Bereitstellung von ThemisDB: von Docker-Containern über Kubernetes bis hin zu Cloud-Deployments. Außerdem lernen Sie Monitoring, Backup-Strategien und Skalierungstechniken.

**Was Sie in diesem Kapitel lernen:**
- Docker-basierte Deployments
- Kubernetes-Orchestrierung mit Helm Charts
- Cloud-Provider-Integration (AWS, Azure, GCP)
- Monitoring mit Prometheus & Grafana
- Backup & Disaster Recovery
- Horizontal Scaling & Sharding
- Security Best Practices

**Voraussetzungen:** Kapitel 4 (Installation), Grundkenntnisse in Containerisierung.

---

## 30.1 Container Orchestration mit Kubernetes {#chapter_30_1_container_orchestration}

Wir betrachten in diesem Abschnitt moderne Container-Orchestrierung mit Kubernetes für produktive ThemisDB-Deployments. Kubernetes bietet dabei nicht nur automatische Skalierung und Self-Healing, sondern auch fortgeschrittene Scheduling-Mechanismen wie Pod-Affinity-Regeln, die eine optimale Verteilung der Datenbankknoten über die verfügbare Infrastruktur gewährleisten und somit Hochverfügbarkeit sicherstellen.

### 30.1.1 StatefulSet für ThemisDB Cluster Deployment {#chapter_30_1_1_statefulset_deployment}

Für zustandsbehaftete Anwendungen wie Datenbanken verwenden wir StatefulSets anstelle von einfachen Deployments. StatefulSets garantieren stabile Netzwerk-Identitäten, persistente Storage-Zuordnungen und geordnetes Starten sowie Herunterfahren der Pods, was für Datenbankcluster essentiell ist. Jeder Pod erhält dabei einen deterministischen Namen (z.B. `themisdb-0`, `themisdb-1`, `themisdb-2`) und kann über einen stabilen DNS-Eintrag erreicht werden.

**ThemisDB StatefulSet Deployment mit deutschen Kommentaren:**

```yaml
# statefulset-themisdb-cluster.yaml
# ThemisDB StatefulSet Deployment für Produktionsumgebung
apiVersion: apps/v1
kind: StatefulSet
metadata:
  name: themisdb-cluster
  namespace: themis-prod
  labels:
    app: themisdb
    version: v3.0
spec:
  serviceName: themisdb-headless
  replicas: 3
  # Parallel Pod Management für schnelleren Start
  podManagementPolicy: Parallel
  selector:
    matchLabels:
      app: themisdb
  template:
    metadata:
      labels:
        app: themisdb
        version: v3.0
      annotations:
        prometheus.io/scrape: "true"
        prometheus.io/port: "9090"
    spec:
      # Anti-Affinity: Verteile Pods über verschiedene Nodes
      # Dies verhindert Single-Point-of-Failure bei Node-Ausfall
      affinity:
        podAntiAffinity:
          requiredDuringSchedulingIgnoredDuringExecution:
          - labelSelector:
              matchExpressions:
              - key: app
                operator: In
                values:
                - themisdb
            # Verteile über verschiedene Availability Zones
            topologyKey: topology.kubernetes.io/zone
          preferredDuringSchedulingIgnoredDuringExecution:
          - weight: 100
            podAffinityTerm:
              labelSelector:
                matchExpressions:
                - key: app
                  operator: In
                  values:
                  - themisdb
              # Verteile bevorzugt über verschiedene Hosts
              topologyKey: kubernetes.io/hostname
      # Init Container für Pre-Flight Checks
      initContainers:
      - name: init-permissions
        image: busybox:1.35
        command: ['sh', '-c', 'chown -R 999:999 /var/lib/themisdb']
        volumeMounts:
        - name: data
          mountPath: /var/lib/themisdb
      containers:
      - name: themisdb
        image: themisdb:3.0
        imagePullPolicy: IfNotPresent
        ports:
        - containerPort: 8529
          name: http
          protocol: TCP
        - containerPort: 8530
          name: websocket
          protocol: TCP
        - containerPort: 9090
          name: metrics
          protocol: TCP
        # Environment Variables aus ConfigMap und Secrets
        env:
        - name: THEMISDB_CLUSTER_MODE
          value: "coordinator"
        - name: THEMISDB_CLUSTER_ENDPOINTS
          value: "themisdb-0.themisdb-headless:8529,themisdb-1.themisdb-headless:8529,themisdb-2.themisdb-headless:8529"
        - name: THEMISDB_ADMIN_PASSWORD
          valueFrom:
            secretKeyRef:
              name: themisdb-secrets
              key: admin-password
        # Volume Mounts für persistente Daten und Konfiguration
        volumeMounts:
        - name: data
          mountPath: /var/lib/themisdb
        - name: config
          mountPath: /etc/themisdb
          readOnly: true
        # Resource Requests und Limits für QoS
        resources:
          requests:
            memory: "8Gi"
            cpu: "4000m"
            ephemeral-storage: "10Gi"
          limits:
            memory: "16Gi"
            cpu: "8000m"
            ephemeral-storage: "20Gi"
        # Liveness Probe prüft ob Container neu gestartet werden muss
        livenessProbe:
          httpGet:
            path: /_api/version
            port: 8529
          initialDelaySeconds: 60
          periodSeconds: 30
          timeoutSeconds: 10
          failureThreshold: 3
        # Readiness Probe prüft ob Pod Traffic empfangen kann
        readinessProbe:
          httpGet:
            path: /_admin/server/availability
            port: 8529
          initialDelaySeconds: 30
          periodSeconds: 10
          timeoutSeconds: 5
          successThreshold: 1
          failureThreshold: 3
        # Startup Probe für langsame Initialisierung
        startupProbe:
          httpGet:
            path: /_api/version
            port: 8529
          initialDelaySeconds: 0
          periodSeconds: 10
          timeoutSeconds: 5
          failureThreshold: 30
      # Volumes für ConfigMap
      volumes:
      - name: config
        configMap:
          name: themisdb-config
          defaultMode: 0644
      # Security Context für Pod
      securityContext:
        runAsNonRoot: true
        runAsUser: 999
        fsGroup: 999
      # Termination Grace Period für sauberes Herunterfahren
      terminationGracePeriodSeconds: 120
  # Volume Claim Templates für persistente Daten
  volumeClaimTemplates:
  - metadata:
      name: data
      labels:
        app: themisdb
    spec:
      accessModes: ["ReadWriteOnce"]
      storageClassName: fast-ssd
      resources:
        requests:
          storage: 100Gi
  # Update Strategy für Rolling Updates
  updateStrategy:
    type: RollingUpdate
    rollingUpdate:
      partition: 0
```

### 30.1.2 Service Discovery und Load Balancing {#chapter_30_1_2_service_discovery}

Für die Kommunikation zwischen Pods und externen Clients benötigen wir Kubernetes Services. Wir erstellen sowohl einen Headless Service für die direkte Pod-zu-Pod-Kommunikation innerhalb des Clusters als auch einen LoadBalancer Service für externe Zugriffe. Der Headless Service ermöglicht DNS-basierte Service Discovery, wobei jeder Pod über einen stabilen DNS-Namen erreichbar ist.

**Service-Definitionen:**

```yaml
# service-headless.yaml
# Headless Service für Cluster-interne Kommunikation
apiVersion: v1
kind: Service
metadata:
  name: themisdb-headless
  namespace: themis-prod
  labels:
    app: themisdb
spec:
  # ClusterIP None macht den Service "headless"
  clusterIP: None
  # Publishes DNS records for each pod
  publishNotReadyAddresses: true
  selector:
    app: themisdb
  ports:
  - port: 8529
    name: http
    protocol: TCP
  - port: 8530
    name: websocket
    protocol: TCP
---
# service-loadbalancer.yaml
# LoadBalancer Service für externe Zugriffe
apiVersion: v1
kind: Service
metadata:
  name: themisdb-lb
  namespace: themis-prod
  labels:
    app: themisdb
  annotations:
    # AWS Load Balancer Annotations
    service.beta.kubernetes.io/aws-load-balancer-type: "nlb"
    service.beta.kubernetes.io/aws-load-balancer-cross-zone-load-balancing-enabled: "true"
    service.beta.kubernetes.io/aws-load-balancer-backend-protocol: "tcp"
spec:
  type: LoadBalancer
  selector:
    app: themisdb
  ports:
  - port: 8529
    targetPort: 8529
    name: http
    protocol: TCP
  sessionAffinity: ClientIP
  sessionAffinityConfig:
    clientIP:
      timeoutSeconds: 10800
```

### 30.1.3 ConfigMaps und Secrets Management {#chapter_30_1_3_configmaps_secrets}

Die Trennung von Konfiguration und Code ist ein zentrales Prinzip in Cloud-Native-Anwendungen. Wir verwenden ConfigMaps für nicht-sensitive Konfigurationsdaten und Secrets für Passwörter, API-Schlüssel und Zertifikate. Diese können zur Laufzeit in Pods gemountet oder als Umgebungsvariablen injiziert werden, ohne dass Container-Images neu gebaut werden müssen.

**ConfigMap für ThemisDB-Konfiguration:**

```yaml
# configmap-themisdb.yaml
apiVersion: v1
kind: ConfigMap
metadata:
  name: themisdb-config
  namespace: themis-prod
data:
  # Haupt-Konfigurationsdatei
  themisdb.conf: |
    # Database Settings
    [database]
    path = /var/lib/themisdb/data
    wal_sync_mode = fsync
    wal_archive_timeout = 60
    
    # Server Settings
    [server]
    endpoint = http://0.0.0.0:8529
    threads = 16
    max_connections = 1000
    request_timeout = 300
    
    # Cluster Settings
    [cluster]
    enabled = true
    my_address = $HOSTNAME.themisdb-headless:8529
    agency_endpoints = themisdb-0:8529,themisdb-1:8529,themisdb-2:8529
    
    # Logging Settings
    [logging]
    level = info
    file = /var/log/themisdb/themisdb.log
    max_file_size = 100M
    max_files = 10
    
    # Cache Settings
    [cache]
    size_mb = 8192
    max_spare_memory_mb = 2048
  
  # Log4j-Style Logging Configuration
  log4j.properties: |
    log4j.rootLogger=INFO, file
    log4j.appender.file=org.apache.log4j.RollingFileAppender
    log4j.appender.file.File=/var/log/themisdb/themisdb.log
    log4j.appender.file.MaxFileSize=100MB
    log4j.appender.file.MaxBackupIndex=10
```

**Secrets für sensitive Daten:**

```yaml
# secret-themisdb.yaml
apiVersion: v1
kind: Secret
metadata:
  name: themisdb-secrets
  namespace: themis-prod
type: Opaque
data:
  # Basis64-kodierte Werte (in Production: verschlüsselt mit Sealed Secrets)
  admin-password: VGhlbWlzREJBZG1pbjEyMyE=  # ThemisDBAdmin123!
  jwt-secret: c3VwZXJzZWNyZXRrZXkyMDI1  # supersecretkey2025
  tls-cert: LS0tLS1CRUdJTi...  # TLS Certificate
  tls-key: LS0tLS1CRUdJTi...   # TLS Private Key
```

### 30.1.4 Persistent Volume Claims für Datenspeicherung {#chapter_30_1_4_persistent_volumes}

Für produktive Datenbank-Deployments benötigen wir persistente Speicherung, die Pod-Neustarts überlebt. Kubernetes bietet hierfür Persistent Volumes (PV) und Persistent Volume Claims (PVC). Wir definieren Storage Classes mit verschiedenen Performance-Charakteristiken und verwenden Volume Claim Templates im StatefulSet für automatische PVC-Erstellung.

**Storage Class Definitionen:**

```yaml
# storageclass-fast.yaml
# SSD-basierte Storage Class für Produktionsdaten
apiVersion: storage.k8s.io/v1
kind: StorageClass
metadata:
  name: fast-ssd
  annotations:
    storageclass.kubernetes.io/is-default-class: "false"
provisioner: kubernetes.io/aws-ebs
parameters:
  type: gp3
  iops: "16000"
  throughput: "1000"
  fsType: ext4
  encrypted: "true"
volumeBindingMode: WaitForFirstConsumer
allowVolumeExpansion: true
reclaimPolicy: Retain
---
# storageclass-archive.yaml
# HDD-basierte Storage Class für Backups
apiVersion: storage.k8s.io/v1
kind: StorageClass
metadata:
  name: standard-hdd
provisioner: kubernetes.io/aws-ebs
parameters:
  type: st1
  fsType: ext4
  encrypted: "true"
volumeBindingMode: Immediate
allowVolumeExpansion: true
reclaimPolicy: Delete
```

### 30.1.5 Deployment-Typen im Vergleich {#chapter_30_1_5_deployment_comparison}

Die Wahl des richtigen Deployment-Typs hängt von den spezifischen Anforderungen an Verfügbarkeit, Komplexität und Kosten ab. Nachfolgende Tabelle vergleicht die verschiedenen Deployment-Optionen für ThemisDB und zeigt deren charakteristische Eigenschaften.

**Tabelle 30.1: Deployment-Typen Vergleichsmatrix**

| Deployment Type | Startup Time | Failover Time | Resource Overhead | Complexity | Use Case |
|----------------|--------------|---------------|-------------------|------------|----------|
| Single Container | 5s | N/A | Sehr niedrig | Einfach | Entwicklung, Tests |
| Docker Compose | 15s | 30-60s | Niedrig | Moderat | Lokale Dev, CI/CD |
| Kubernetes | 30-45s | 5-15s | Mittel-Hoch | Komplex | Produktion |
| Managed Service | 60-120s | <5s | Minimal | Einfach | Enterprise, Cloud-First |

**Methodik:** Startup Time gemessen vom Container-Start bis zum ersten erfolgreichen Health Check. Failover Time ist die Dauer vom Node-Ausfall bis zur erfolgreichen Umleitung auf gesunden Node. Resource Overhead berücksichtigt Control Plane, Monitoring und Management-Komponenten.

### 30.1.6 Helm Chart für ThemisDB {#chapter_30_1_6_helm_chart}

Die Helm-Integration ergänzt die StatefulSet-Deployments mit Package Management und parametrisierten Configurations-Templating. Dies ermöglicht einfacheres Rollout über mehrere Environments mit konsistenten Best Practices.

**Chart.yaml:**

```yaml
apiVersion: v2
name: themis
description: A Helm chart for ThemisDB
type: application
version: 1.3.4
appVersion: "1.3.4"
```

**values.yaml:**

```yaml
replicaCount: 3

image:
  repository: themisdb/themisdb
  tag: v1.3.4
  pullPolicy: IfNotPresent

resources:
  requests:
    memory: 4Gi
    cpu: 2000m
  limits:
    memory: 8Gi
    cpu: 4000m

persistence:
  enabled: true
  storageClass: fast-ssd
  size: 100Gi

service:
  type: LoadBalancer
  port: 8529

ingress:
  enabled: true
  className: nginx
  annotations:
    cert-manager.io/cluster-issuer: letsencrypt-prod
  hosts:
    - host: themis.example.com
      paths:
        - path: /
          pathType: Prefix
  tls:
    - secretName: themis-tls
      hosts:
        - themis.example.com

monitoring:
  enabled: true
  serviceMonitor:
    enabled: true
    interval: 30s
```

**Install Helm Chart:**

```bash
# Add Helm repo
helm repo add themis https://charts.themisdb.io
helm repo update

# Install
helm install themis-prod themis/themis \
  --namespace themis-prod \
  --create-namespace \
  --values values.yaml

# Upgrade
helm upgrade themis-prod themis/themis \
  --namespace themis-prod \
  --values values.yaml

# Uninstall
helm uninstall themis-prod -n themis-prod
```

---

## 30.2 Infrastructure as Code mit Terraform {#chapter_30_2_infrastructure_as_code}

Infrastructure as Code (IaC) ermöglicht die deklarative Definition und Versionierung der gesamten Infrastruktur. Wir verwenden Terraform als führendes IaC-Tool, um Cloud-Ressourcen reproduzierbar bereitzustellen, State-Management zu zentralisieren und Drift-Detection durchzuführen. Dies gewährleistet Konsistenz zwischen Entwicklungs-, Staging- und Produktionsumgebungen und ermöglicht vollständige Disaster-Recovery-Szenarien durch Code.

### 30.2.1 Terraform Provider und Module {#chapter_30_2_1_terraform_provider}

Terraform organisiert Infrastruktur in wiederverwendbare Module, die Cloud-Provider-spezifische Ressourcen kapseln. Wir definieren ein ThemisDB-Modul, das VPC, Subnets, Security Groups, Auto-Scaling-Gruppen und Load Balancer orchestriert. Die Verwendung von Modulen fördert DRY-Prinzipien (Don't Repeat Yourself) und vereinfacht die Wartung über mehrere Umgebungen hinweg.

**Terraform Module für ThemisDB Cluster mit deutschen Kommentaren:**

```hcl
# main.tf - Hauptmodul für ThemisDB AWS Deployment
terraform {
  required_version = ">= 1.6.0"
  required_providers {
    aws = {
      source  = "hashicorp/aws"
      version = "~> 5.0"
    }
  }
  # Remote Backend für State-Management (S3 + DynamoDB Locking)
  backend "s3" {
    bucket         = "themisdb-terraform-state"
    key            = "prod/cluster.tfstate"
    region         = "eu-central-1"
    encrypt        = true
    dynamodb_table = "terraform-state-lock"
    kms_key_id     = "arn:aws:kms:eu-central-1:123456789:key/abcd-1234"
  }
}

# AWS Provider Konfiguration mit Default Tags
provider "aws" {
  region = var.aws_region
  default_tags {
    tags = {
      Project     = "ThemisDB"
      Environment = var.environment
      ManagedBy   = "Terraform"
      CostCenter  = "Engineering"
    }
  }
}

# VPC für Cluster-Isolation und Netzwerksegmentierung
resource "aws_vpc" "themisdb" {
  cidr_block           = var.vpc_cidr
  enable_dns_hostnames = true
  enable_dns_support   = true
  
  tags = {
    Name = "themisdb-${var.environment}-vpc"
  }
}

# Private Subnets für Datenbankknoten (über 3 AZs verteilt)
resource "aws_subnet" "private" {
  count             = length(var.availability_zones)
  vpc_id            = aws_vpc.themisdb.id
  cidr_block        = cidrsubnet(var.vpc_cidr, 4, count.index)
  availability_zone = var.availability_zones[count.index]
  
  tags = {
    Name = "themisdb-${var.environment}-private-${count.index + 1}"
    Tier = "Database"
  }
}

# Security Group für ThemisDB (restriktive Firewall-Regeln)
resource "aws_security_group" "themisdb" {
  name_prefix = "themisdb-${var.environment}-"
  description = "Security group for ThemisDB cluster nodes"
  vpc_id      = aws_vpc.themisdb.id
  
  # Eingehend: HTTP API (nur von Application Tier)
  ingress {
    description     = "HTTP API from application tier"
    from_port       = 8529
    to_port         = 8529
    protocol        = "tcp"
    security_groups = [aws_security_group.application.id]
  }
  
  # Eingehend: Cluster-Kommunikation (nur innerhalb Security Group)
  ingress {
    description = "Cluster communication"
    from_port   = 8530
    to_port     = 8530
    protocol    = "tcp"
    self        = true
  }
  
  # Ausgehend: Alle Verbindungen erlaubt (für Updates, Backups)
  egress {
    description = "Allow all outbound traffic"
    from_port   = 0
    to_port     = 0
    protocol    = "-1"
    cidr_blocks = ["0.0.0.0/0"]
  }
  
  tags = {
    Name = "themisdb-${var.environment}-sg"
  }
}

# Launch Template für EC2-Instanzen mit UserData
resource "aws_launch_template" "themisdb" {
  name_prefix   = "themisdb-${var.environment}-"
  image_id      = data.aws_ami.themisdb.id
  instance_type = var.instance_type
  
  # IAM Instance Profile für AWS Service Access
  iam_instance_profile {
    name = aws_iam_instance_profile.themisdb.name
  }
  
  # Netzwerk-Konfiguration
  network_interfaces {
    associate_public_ip_address = false
    security_groups             = [aws_security_group.themisdb.id]
    delete_on_termination       = true
  }
  
  # Block Device Mapping mit verschlüsselten EBS Volumes
  block_device_mappings {
    device_name = "/dev/sda1"
    ebs {
      volume_size           = var.root_volume_size
      volume_type           = "gp3"
      iops                  = 3000
      throughput            = 125
      encrypted             = true
      kms_key_id            = aws_kms_key.themisdb.arn
      delete_on_termination = true
    }
  }
  
  # Data Volume für Datenbankdaten
  block_device_mappings {
    device_name = "/dev/sdb"
    ebs {
      volume_size           = var.data_volume_size
      volume_type           = "gp3"
      iops                  = 16000
      throughput            = 1000
      encrypted             = true
      kms_key_id            = aws_kms_key.themisdb.arn
      delete_on_termination = false  # Behalte Daten bei Instanz-Terminierung
    }
  }
  
  # UserData Script für Bootstrap
  user_data = base64encode(templatefile("${path.module}/templates/userdata.sh", {
    cluster_name     = "themisdb-${var.environment}"
    cluster_size     = var.cluster_size
    backup_bucket    = aws_s3_bucket.backups.id
    cloudwatch_group = aws_cloudwatch_log_group.themisdb.name
  }))
  
  # Tag Specifications
  tag_specifications {
    resource_type = "instance"
    tags = {
      Name = "themisdb-${var.environment}-node"
    }
  }
  
  tag_specifications {
    resource_type = "volume"
    tags = {
      Name = "themisdb-${var.environment}-volume"
    }
  }
  
  # Monitoring aktivieren
  monitoring {
    enabled = true
  }
  
  metadata_options {
    http_endpoint               = "enabled"
    http_tokens                 = "required"  # IMDSv2 erzwingen
    http_put_response_hop_limit = 1
  }
}

# Auto-Scaling Group für ThemisDB Nodes
resource "aws_autoscaling_group" "themisdb_cluster" {
  name                = "themisdb-${var.environment}-asg"
  vpc_zone_identifier = aws_subnet.private[*].id
  min_size            = var.cluster_size
  max_size            = var.cluster_size * 3
  desired_capacity    = var.cluster_size
  
  # Health Check Konfiguration mit Custom Endpoint
  health_check_type         = "ELB"
  health_check_grace_period = 300
  
  # Launch Template Reference
  launch_template {
    id      = aws_launch_template.themisdb.id
    version = "$Latest"
  }
  
  # Instance Refresh für Rolling Updates
  instance_refresh {
    strategy = "Rolling"
    preferences {
      min_healthy_percentage = 66  # Mindestens 2 von 3 Nodes gesund
      instance_warmup        = 300
    }
  }
  
  # Dynamische Tags die an Instanzen propagiert werden
  dynamic "tag" {
    for_each = merge(
      {
        Name        = "themisdb-${var.environment}-node"
        ClusterName = "themisdb-${var.environment}"
      },
      var.additional_tags
    )
    content {
      key                 = tag.key
      value               = tag.value
      propagate_at_launch = true
    }
  }
  
  # Lifecycle Hooks für graceful shutdown
  initial_lifecycle_hook {
    name                 = "themisdb-shutdown-hook"
    lifecycle_transition = "autoscaling:EC2_INSTANCE_TERMINATING"
    default_result       = "CONTINUE"
    heartbeat_timeout    = 300
  }
}
```

### 30.2.2 State Management und Remote Backends {#chapter_30_2_2_state_management}

Terraform speichert den aktuellen Zustand der verwalteten Infrastruktur in einer State-Datei. Für Team-Umgebungen ist ein Remote Backend mit Locking essentiell, um Konflikte bei parallelen Änderungen zu vermeiden. Wir verwenden S3 als Backend mit DynamoDB für State-Locking, wodurch mehrere Entwickler gleichzeitig an der Infrastruktur arbeiten können, ohne sich gegenseitig zu überschreiben.

**Backend-Konfiguration:**

```hcl
# backend.tf - S3 Backend mit DynamoDB Locking
# Muss vor der ersten Terraform-Initialisierung konfiguriert sein

# S3 Bucket für State Storage
resource "aws_s3_bucket" "terraform_state" {
  bucket = "themisdb-terraform-state-${data.aws_caller_identity.current.account_id}"
  
  # Verhindere versehentliches Löschen
  lifecycle {
    prevent_destroy = true
  }
  
  tags = {
    Name        = "Terraform State Storage"
    Description = "Stores Terraform state files for ThemisDB infrastructure"
  }
}

# Versionierung aktivieren für State-History
resource "aws_s3_bucket_versioning" "terraform_state" {
  bucket = aws_s3_bucket.terraform_state.id
  versioning_configuration {
    status = "Enabled"
  }
}

# Verschlüsselung mit AWS KMS
resource "aws_s3_bucket_server_side_encryption_configuration" "terraform_state" {
  bucket = aws_s3_bucket.terraform_state.id
  
  rule {
    apply_server_side_encryption_by_default {
      sse_algorithm     = "aws:kms"
      kms_master_key_id = aws_kms_key.terraform_state.arn
    }
  }
}

# DynamoDB Tabelle für State-Locking
resource "aws_dynamodb_table" "terraform_lock" {
  name           = "terraform-state-lock"
  billing_mode   = "PAY_PER_REQUEST"
  hash_key       = "LockID"
  
  attribute {
    name = "LockID"
    type = "S"
  }
  
  # Point-in-Time Recovery für Disaster Recovery
  point_in_time_recovery {
    enabled = true
  }
  
  tags = {
    Name        = "Terraform State Lock"
    Description = "DynamoDB table for Terraform state locking"
  }
}
```

### 30.2.3 Module Composition und Drift Detection {#chapter_30_2_3_module_composition}

Terraform-Module ermöglichen die Komposition komplexer Infrastrukturen aus wiederverwendbaren Bausteinen. Wir organisieren unsere Infrastruktur in logische Module (Networking, Compute, Database, Monitoring), die über Variablen parametrisiert werden. Drift Detection identifiziert Abweichungen zwischen dem Terraform-State und der tatsächlichen Infrastruktur, die durch manuelle Änderungen entstanden sind.

**Drift Detection Workflow:**

```bash
#!/bin/bash
# drift-detection.sh - Prüft auf manuelle Infrastruktur-Änderungen

# Terraform Refresh: Aktualisiere State mit aktueller Infrastruktur
terraform refresh -lock=true

# Terraform Plan: Zeige Abweichungen vom gewünschten Zustand
terraform plan -detailed-exitcode -out=tfplan

# Exit Codes:
# 0 = Keine Änderungen (kein Drift)
# 1 = Fehler
# 2 = Änderungen vorhanden (Drift erkannt)

if [ $? -eq 2 ]; then
  echo "⚠️  Drift erkannt! Manuelle Änderungen vorhanden."
  echo "Überprüfe tfplan für Details:"
  terraform show tfplan
  
  # Optional: Automatisches Reconcile (nicht für Produktion empfohlen)
  # terraform apply -auto-approve tfplan
else
  echo "✅ Kein Drift erkannt. Infrastruktur entspricht Terraform-Code."
fi
```

---

## 30.3 Deployment Strategies {#chapter_30_3_deployment_strategies}

Moderne Deployment-Strategien ermöglichen uns, neue Software-Versionen mit minimalen Ausfallzeiten und reduzierten Risiken zu veröffentlichen. Wir betrachten verschiedene Ansätze von Blue-Green über Canary bis hin zu Rolling Updates, die jeweils spezifische Vor- und Nachteile in Bezug auf Downtime, Rollback-Geschwindigkeit und Ressourcenverbrauch bieten. Die Wahl der richtigen Strategie hängt von den Anforderungen an Verfügbarkeit, Budget und Risikotoleranz ab.

### 30.3.1 Blue-Green Deployments mit Zero Downtime {#chapter_30_3_1_blue_green}

Blue-Green Deployments basieren auf der Idee, zwei identische Produktionsumgebungen parallel zu betreiben. Während die "Blue" Umgebung den aktuellen Produktionsverkehr bedient, wird die neue Version in der "Green" Umgebung deployed und getestet. Nach erfolgreicher Validierung wird der Traffic instantan umgeleitet. Dies ermöglicht Zero-Downtime-Deployments und sofortige Rollbacks bei Problemen.

**Vorteile:** Kein Downtime, schnelles Rollback, vollständige Testmöglichkeit vor Traffic-Switch  
**Nachteile:** Doppelte Infrastruktur-Kosten, Datenbank-Migrationen müssen abwärtskompatibel sein

**Kubernetes Blue-Green Implementation:**

```yaml
# blue-green-service.yaml
# Service-Selector wird manuell zwischen blue/green umgeschaltet
apiVersion: v1
kind: Service
metadata:
  name: themisdb-service
  namespace: themis-prod
  labels:
    app: themisdb
spec:
  selector:
    app: themisdb
    version: blue  # Ändere zu "green" für Umschaltung
  ports:
  - port: 8529
    targetPort: 8529
    name: http
  type: LoadBalancer
  sessionAffinity: ClientIP
---
# blue-deployment.yaml
# Aktuelle Produktionsversion (Blue)
apiVersion: apps/v1
kind: Deployment
metadata:
  name: themisdb-blue
  namespace: themis-prod
spec:
  replicas: 3
  selector:
    matchLabels:
      app: themisdb
      version: blue
  template:
    metadata:
      labels:
        app: themisdb
        version: blue
    spec:
      containers:
      - name: themisdb
        image: themisdb:v2.9.0
        ports:
        - containerPort: 8529
        resources:
          requests:
            memory: "8Gi"
            cpu: "4"
---
# green-deployment.yaml
# Neue Version (Green) - idle bis zum Traffic-Switch
apiVersion: apps/v1
kind: Deployment
metadata:
  name: themisdb-green
  namespace: themis-prod
spec:
  replicas: 3
  selector:
    matchLabels:
      app: themisdb
      version: green
  template:
    metadata:
      labels:
        app: themisdb
        version: green
    spec:
      containers:
      - name: themisdb
        image: themisdb:v3.0.0  # Neue Version
        ports:
        - containerPort: 8529
        resources:
          requests:
            memory: "8Gi"
            cpu: "4"
```

**Blue-Green Deployment Workflow:**

```bash
#!/bin/bash
# blue-green-deploy.sh - Automatisiertes Blue-Green Deployment

# 1. Deploy Green Environment (neue Version)
echo "Deploying Green environment with new version..."
kubectl apply -f green-deployment.yaml
kubectl rollout status deployment/themisdb-green -n themis-prod

# 2. Smoke Tests gegen Green
echo "Running smoke tests against Green..."
GREEN_IP=$(kubectl get svc themisdb-green -n themis-prod -o jsonpath='{.status.loadBalancer.ingress[0].ip}')
curl -f http://$GREEN_IP:8529/_api/version || exit 1

# 3. Datenbank-Synchronisation (falls erforderlich)
echo "Syncing data from Blue to Green..."
kubectl exec -it themisdb-blue-0 -n themis-prod -- themisdb-sync --target=green

# 4. Traffic Switch: Ändere Service Selector von blue zu green
echo "Switching traffic from Blue to Green..."
kubectl patch service themisdb-service -n themis-prod -p '{"spec":{"selector":{"version":"green"}}}'

# 5. Monitoring für 15 Minuten
echo "Monitoring Green environment for issues..."
sleep 900

# 6. Überprüfe Error Rate
ERROR_RATE=$(curl -s http://prometheus:9090/api/v1/query?query=rate\(http_errors_total\[5m\]\) | jq '.data.result[0].value[1]')
if (( $(echo "$ERROR_RATE > 0.05" | bc -l) )); then
  echo "❌ High error rate detected! Rolling back..."
  kubectl patch service themisdb-service -n themis-prod -p '{"spec":{"selector":{"version":"blue"}}}'
  exit 1
fi

# 7. Scale down Blue (behalte für Rollback-Möglichkeit)
echo "✅ Deployment successful. Scaling down Blue..."
kubectl scale deployment themisdb-blue -n themis-prod --replicas=1

echo "🎉 Blue-Green deployment completed successfully!"
```

### 30.3.2 Canary Releases mit Traffic Splitting {#chapter_30_3_2_canary_releases}

Canary Deployments leiten schrittweise einen steigenden Prozentsatz des Traffics auf die neue Version, während der Großteil weiterhin die stabile Version nutzt. Dies ermöglicht frühzeitige Erkennung von Problemen mit minimalem User-Impact. Wir verwenden Argo Rollouts für automatisierte Canary-Deployments mit integrierten Analysis-Templates.

**Argo Rollouts Canary Deployment mit deutschen Kommentaren:**

```yaml
# canary-rollout.yaml
# Argo Rollouts: Progressive Traffic Shifting mit automatischem Rollback
apiVersion: argoproj.io/v1alpha1
kind: Rollout
metadata:
  name: themisdb-api
  namespace: themis-prod
  labels:
    app: themisdb
spec:
  replicas: 10
  # Revision History für Rollbacks
  revisionHistoryLimit: 5
  selector:
    matchLabels:
      app: themisdb
  template:
    metadata:
      labels:
        app: themisdb
    spec:
      containers:
      - name: themisdb
        image: themisdb:v3.0.0
        ports:
        - containerPort: 8529
          name: http
        resources:
          requests:
            memory: "8Gi"
            cpu: "4"
          limits:
            memory: "16Gi"
            cpu: "8"
        livenessProbe:
          httpGet:
            path: /_api/version
            port: 8529
          initialDelaySeconds: 30
          periodSeconds: 10
        readinessProbe:
          httpGet:
            path: /_admin/server/availability
            port: 8529
          initialDelaySeconds: 10
          periodSeconds: 5
  # Canary-Strategie: Schrittweise Traffic-Erhöhung mit Validierung
  strategy:
    canary:
      # Traffic-Routing via Istio VirtualService
      trafficRouting:
        istio:
          virtualService:
            name: themisdb-vsvc
            routes:
            - primary
      # Schrittweise Rollout-Steps
      steps:
      - setWeight: 10      # 10% Traffic auf neue Version (Canary)
      - pause: {duration: 5m}  # Warte 5 Minuten und beobachte Metriken
      
      # Automatische Analyse nach erstem Step
      - analysis:
          templates:
          - templateName: error-rate-analysis
          args:
          - name: service-name
            value: themisdb-api
      
      - setWeight: 25      # 25% Traffic auf Canary
      - pause: {duration: 5m}
      
      - setWeight: 50      # 50% Traffic auf Canary
      - pause: {duration: 10m}  # Längere Pause bei 50% für Stabilitätsprüfung
      
      # Finale Analyse vor 75%
      - analysis:
          templates:
          - templateName: latency-analysis
          - templateName: error-rate-analysis
          args:
          - name: service-name
            value: themisdb-api
      
      - setWeight: 75      # 75% Traffic auf Canary
      - pause: {duration: 5m}
      
      # Kein weiterer Step nötig - 100% wird automatisch erreicht
      
      # Automatisches Rollback bei Fehler-Rate >5%
      analysis:
        templates:
        - templateName: error-rate-analysis
        args:
        - name: error-threshold
          value: "5"  # 5% Fehlerrate = Rollback
        - name: service-name
          value: themisdb-api
      
      # Anti-Affinity für Canary Pods
      antiAffinity:
        requiredDuringSchedulingIgnoredDuringExecution: {}
---
# analysis-template-error-rate.yaml
# Analysis Template für Fehlerrate-Überwachung
apiVersion: argoproj.io/v1alpha1
kind: AnalysisTemplate
metadata:
  name: error-rate-analysis
  namespace: themis-prod
spec:
  args:
  - name: service-name
  - name: error-threshold
    value: "5"
  metrics:
  - name: error-rate
    # Abfrage von Prometheus alle 60 Sekunden
    interval: 60s
    # Erfolgskriterium: Fehlerrate muss < threshold sein (5 Messungen)
    successCondition: result < {{args.error-threshold}}
    failureLimit: 3
    provider:
      prometheus:
        address: http://prometheus.monitoring:9090
        query: |
          sum(rate(
            http_requests_total{
              service="{{args.service-name}}",
              status=~"5.."
            }[5m]
          )) / 
          sum(rate(
            http_requests_total{
              service="{{args.service-name}}"
            }[5m]
          )) * 100
---
# analysis-template-latency.yaml
# Analysis Template für Latenz-Überwachung
apiVersion: argoproj.io/v1alpha1
kind: AnalysisTemplate
metadata:
  name: latency-analysis
  namespace: themis-prod
spec:
  args:
  - name: service-name
  metrics:
  - name: p95-latency
    interval: 60s
    # P95 Latenz darf nicht über 500ms steigen
    successCondition: result < 0.5
    failureLimit: 3
    provider:
      prometheus:
        address: http://prometheus.monitoring:9090
        query: |
          histogram_quantile(0.95,
            sum(rate(
              http_request_duration_seconds_bucket{
                service="{{args.service-name}}"
              }[5m]
            )) by (le)
          )
```

### 30.3.3 Rolling Updates und Rollback-Verfahren {#chapter_30_3_3_rolling_updates}

Rolling Updates ersetzen Pods schrittweise, wobei neue Versionen gestartet werden, bevor alte Versionen terminiert werden. Kubernetes orchestriert dies automatisch basierend auf der `RollingUpdateStrategy`. Dies ist die Standard-Deployment-Methode für zustandslose Anwendungen und benötigt keine zusätzliche Infrastruktur.

**Rolling Update Konfiguration:**

```yaml
# rolling-update-deployment.yaml
apiVersion: apps/v1
kind: Deployment
metadata:
  name: themisdb-rolling
  namespace: themis-prod
spec:
  replicas: 10
  # Rolling Update Strategie
  strategy:
    type: RollingUpdate
    rollingUpdate:
      maxSurge: 2        # Maximal 2 zusätzliche Pods während Update
      maxUnavailable: 1  # Maximal 1 Pod kann unavailable sein
  selector:
    matchLabels:
      app: themisdb
  template:
    metadata:
      labels:
        app: themisdb
    spec:
      containers:
      - name: themisdb
        image: themisdb:v3.0.0
        ports:
        - containerPort: 8529
        # Readiness Probe verhindert Traffic zu nicht-bereiten Pods
        readinessProbe:
          httpGet:
            path: /_admin/server/availability
            port: 8529
          initialDelaySeconds: 10
          periodSeconds: 5
          failureThreshold: 3
        # Liveness Probe restartet fehlerhafte Pods
        livenessProbe:
          httpGet:
            path: /_api/version
            port: 8529
          initialDelaySeconds: 60
          periodSeconds: 30
      # Termination Grace Period für sauberes Shutdown
      terminationGracePeriodSeconds: 60
```

**Rollback-Verfahren:**

```bash
#!/bin/bash
# rollback.sh - Rollback zu vorheriger Version

# 1. Prüfe Deployment-Historie
kubectl rollout history deployment/themisdb-rolling -n themis-prod

# Output zeigt alle Revisions:
# REVISION  CHANGE-CAUSE
# 1         Initial deployment
# 2         Update to v3.0.0
# 3         Update to v3.0.1

# 2. Rollback zur vorherigen Revision
kubectl rollout undo deployment/themisdb-rolling -n themis-prod

# Oder: Rollback zu spezifischer Revision
# kubectl rollout undo deployment/themisdb-rolling --to-revision=2 -n themis-prod

# 3. Überwache Rollback-Status
kubectl rollout status deployment/themisdb-rolling -n themis-prod

# 4. Verify: Prüfe aktuelle Image-Version
kubectl get deployment themisdb-rolling -n themis-prod -o jsonpath='{.spec.template.spec.containers[0].image}'

echo "✅ Rollback completed"
```

### 30.3.4 Feature Flags für graduelle Rollouts {#chapter_30_3_4_feature_flags}

Feature Flags (auch Feature Toggles genannt) ermöglichen die Steuerung von Features zur Laufzeit ohne Code-Deployment. Wir können neue Features zunächst für eine kleine Nutzergruppe aktivieren, Feedback sammeln und bei Problemen sofort deaktivieren. Dies entkoppelt Deployment von Release und ermöglicht kontinuierliche Integration mit kontrollierten Rollouts.

**Feature Flag Integration:**

```yaml
# configmap-feature-flags.yaml
apiVersion: v1
kind: ConfigMap
metadata:
  name: feature-flags
  namespace: themis-prod
data:
  flags.yaml: |
    # Feature Flags für ThemisDB
    features:
      # Neues Query Optimizer Feature
      - name: new-query-optimizer
        enabled: true
        rollout_percentage: 10  # Nur 10% der Nutzer
        user_whitelist:
          - beta-tester-1
          - beta-tester-2
      
      # Vector Search v2
      - name: vector-search-v2
        enabled: false  # Deaktiviert für alle
        rollout_percentage: 0
      
      # Caching Layer v3
      - name: caching-layer-v3
        enabled: true
        rollout_percentage: 50  # A/B Test mit 50/50 Split
        metadata:
          experiment_id: "exp-cache-v3-2025"
          metrics_dashboard: "https://grafana/d/cache-v3"
```

### 30.3.5 Deployment-Strategien im Vergleich {#chapter_30_3_5_deployment_comparison}

Die Wahl der richtigen Deployment-Strategie hängt von mehreren Faktoren ab: Verfügbarkeitsanforderungen, Budget, Komplexität der Infrastruktur und Risikotoleranz. Nachfolgende Tabelle vergleicht die wichtigsten Strategien anhand quantifizierbarer Metriken.

**Tabelle 30.2: Deployment-Strategien Vergleichsmatrix**

| Strategy | Downtime | Rollback Time | Infrastructure Cost | Risk Level | Complexity |
|----------|----------|---------------|---------------------|------------|------------|
| Recreate | 2-5 min | 3-5 min | 1x | Hoch | Niedrig |
| Rolling | None | 5-10 min | 1x | Mittel | Niedrig |
| Blue-Green | None | <1 min | 2x | Niedrig | Mittel |
| Canary | None | <1 min | 1.1x | Sehr niedrig | Hoch |

**Methodik:** Downtime gemessen als Zeit ohne verfügbaren Service. Rollback Time ist die Dauer vom Erkennen eines Problems bis zur vollständigen Wiederherstellung. Infrastructure Cost relativ zu Single-Environment-Deployment. Risk Level basiert auf potenziellem User-Impact bei Fehlern. Complexity bewertet Implementierungs- und Wartungsaufwand.

**Empfehlungen:**
- **Recreate:** Nur für Entwicklungs-/Testumgebungen
- **Rolling:** Standard für zustandslose Anwendungen ohne kritische Verfügbarkeit
- **Blue-Green:** Ideal für kritische Services mit strikten SLAs
- **Canary:** Best Practice für große Produktionsumgebungen mit hohem Traffic

---

## 30.4 Cloud Provider Integration {#chapter_30_4_cloud_integration}

### 30.4.1 AWS Deployment {#chapter_30_4_1_aws_deployment}

**EKS Cluster:**

```bash
# Create EKS cluster
eksctl create cluster \
  --name themis-prod \
  --region eu-central-1 \
  --nodegroup-name standard-workers \
  --node-type m5.2xlarge \
  --nodes 3 \
  --nodes-min 3 \
  --nodes-max 10 \
  --managed

# Install ThemisDB
helm install themis-prod themis/themis \
  --namespace themis-prod \
  --create-namespace \
  --set persistence.storageClass=gp3 \
  --set service.annotations."service\.beta\.kubernetes\.io/aws-load-balancer-type"=nlb
```

**RDS Integration (Optional):**

```yaml
# Use RDS PostgreSQL for metadata
metadata:
  backend: postgresql
  connection: "postgresql://user:pass@themis-metadata.c9akljh4.eu-central-1.rds.amazonaws.com:5432/themis"
```

### 30.4.2 Azure Deployment {#chapter_30_4_2_azure_deployment}

**AKS Cluster:**

```bash
# Create AKS cluster
az aks create \
  --resource-group themis-rg \
  --name themis-prod \
  --location westeurope \
  --node-count 3 \
  --node-vm-size Standard_D8s_v3 \
  --enable-managed-identity \
  --generate-ssh-keys

# Get credentials
az aks get-credentials \
  --resource-group themis-rg \
  --name themis-prod

# Install ThemisDB
helm install themis-prod themis/themis \
  --namespace themis-prod \
  --create-namespace \
  --set persistence.storageClass=managed-premium
```

### 30.4.3 GCP Deployment {#chapter_30_4_3_gcp_deployment}

**GKE Cluster:**

```bash
# Create GKE cluster
gcloud container clusters create themis-prod \
  --region europe-west3 \
  --num-nodes 3 \
  --machine-type n2-standard-8 \
  --disk-size 100

# Get credentials
gcloud container clusters get-credentials themis-prod \
  --region europe-west3

# Install ThemisDB
helm install themis-prod themis/themis \
  --namespace themis-prod \
  --create-namespace \
  --set persistence.storageClass=pd-ssd
```

---

## 30.5 Operational Monitoring & Alerting {#chapter_30_5_monitoring_alerting}

Wir implementieren in diesem Abschnitt ein umfassendes Monitoring- und Alerting-System für ThemisDB-Produktionsumgebungen. Monitoring ermöglicht proaktive Problemerkennung, während Alerting sicherstellt, dass kritische Ereignisse zeitnah an das Operations-Team kommuniziert werden. Wir verwenden Prometheus für Metriken-Collection, Grafana für Visualisierung und definieren SLO/SLI-basierte Alerts für Service-Level-Tracking.

### 30.5.1 Prometheus Metrics Collection und Retention {#chapter_30_5_1_prometheus_collection}

Prometheus ist ein führendes Open-Source Monitoring-System mit einem dimensionalen Datenmodell, das exzellente Query-Performance und flexible Alerting-Regeln bietet. Wir konfigurieren Prometheus zur Scraping von ThemisDB-Metriken alle 30 Sekunden mit einer Retention-Policy von 15 Tagen für lokale Speicherung. Für langfristige Speicherung verwenden wir Thanos oder VictoriaMetrics.

**Prometheus Configuration:**

```yaml
# prometheus-config.yaml
apiVersion: v1
kind: ConfigMap
metadata:
  name: prometheus-config
  namespace: monitoring
data:
  prometheus.yml: |
    # Globale Konfiguration
    global:
      scrape_interval: 30s      # Scrape alle 30 Sekunden
      evaluation_interval: 30s  # Evaluiere Rules alle 30 Sekunden
      external_labels:
        cluster: 'themisdb-prod'
        region: 'eu-central-1'
    
    # Alertmanager Konfiguration
    alerting:
      alertmanagers:
      - static_configs:
        - targets:
          - alertmanager:9093
    
    # Regel-Dateien für Alerts und Recording Rules
    rule_files:
      - '/etc/prometheus/rules/*.yml'
    
    # Scrape Konfigurationen
    scrape_configs:
      # ThemisDB Pods via Kubernetes Service Discovery
      - job_name: 'themisdb'
        kubernetes_sd_configs:
        - role: pod
          namespaces:
            names:
            - themis-prod
        relabel_configs:
        # Nur Pods mit Label app=themisdb scrapen
        - source_labels: [__meta_kubernetes_pod_label_app]
          action: keep
          regex: themisdb
        # Metrics Port aus Annotation lesen
        - source_labels: [__meta_kubernetes_pod_annotation_prometheus_io_port]
          action: replace
          target_label: __address__
          regex: (.+)
          replacement: ${1}:9090
        # Pod Name als Label hinzufügen
        - source_labels: [__meta_kubernetes_pod_name]
          action: replace
          target_label: pod
        # Namespace als Label
        - source_labels: [__meta_kubernetes_namespace]
          action: replace
          target_label: namespace
      
      # Node Exporter für System-Metriken
      - job_name: 'node-exporter'
        kubernetes_sd_configs:
        - role: node
        relabel_configs:
        - source_labels: [__address__]
          regex: '(.*):10250'
          replacement: '${1}:9100'
          target_label: __address__
      
      # Kubernetes API Server Metrics
      - job_name: 'kubernetes-apiservers'
        kubernetes_sd_configs:
        - role: endpoints
        scheme: https
        tls_config:
          ca_file: /var/run/secrets/kubernetes.io/serviceaccount/ca.crt
        bearer_token_file: /var/run/secrets/kubernetes.io/serviceaccount/token
        relabel_configs:
        - source_labels: [__meta_kubernetes_namespace, __meta_kubernetes_service_name, __meta_kubernetes_endpoint_port_name]
          action: keep
          regex: default;kubernetes;https
    
    # Remote Write für langfristige Speicherung (optional)
    remote_write:
    - url: http://thanos-receive:19291/api/v1/receive
      queue_config:
        capacity: 10000
        max_shards: 50
        min_shards: 1
        max_samples_per_send: 5000
        batch_send_deadline: 5s
```

### 30.5.2 Prometheus AlertRules für ThemisDB {#chapter_30_5_2_alert_rules}

Alert-Regeln definieren Schwellenwerte und Bedingungen für automatische Benachrichtigungen. Wir erstellen mehrstufige Alerts (Warning, Critical) basierend auf Severity und definieren sinnvolle Grenzwerte für Query-Latenz, Error-Rates und Cluster-Health.

**Prometheus AlertRules für ThemisDB mit deutschen Kommentaren:**

```yaml
# prometheus-rules-themisdb.yaml
apiVersion: monitoring.coreos.com/v1
kind: PrometheusRule
metadata:
  name: themisdb-alerts
  namespace: monitoring
  labels:
    prometheus: kube-prometheus
spec:
  groups:
  # Gruppe 1: Cluster Health Alerts
  - name: themisdb.cluster
    interval: 30s
    rules:
    # Alert bei hoher Query-Latenz (P95 >500ms für 5min)
    - alert: HighQueryLatency
      expr: |
        histogram_quantile(0.95, 
          rate(themisdb_query_duration_seconds_bucket[5m])
        ) > 0.5
      for: 5m
      labels:
        severity: warning
        component: database
      annotations:
        summary: "ThemisDB Query Latency erhöht"
        description: "P95 Query Latency beträgt {{ $value | humanizeDuration }} auf {{ $labels.instance }} (Threshold: 500ms)"
        runbook_url: "https://docs.themisdb.io/runbooks/high-query-latency"
    
    # Critical Alert bei Cluster-Quorum-Verlust
    - alert: ClusterQuorumLost
      expr: |
        count(up{job="themisdb"} == 1) < 2
      for: 1m
      labels:
        severity: critical
        component: cluster
        page: "true"  # Löst PagerDuty Alert aus
      annotations:
        summary: "ThemisDB Cluster Quorum verloren"
        description: "Nur {{ $value }} von 3 Nodes erreichbar. Cluster kann keine Schreiboperationen durchführen."
        runbook_url: "https://docs.themisdb.io/runbooks/quorum-lost"
    
    # Alert bei hoher Fehlerrate (>5% 5xx Errors)
    - alert: HighErrorRate
      expr: |
        sum(rate(themisdb_http_requests_total{status=~"5.."}[5m])) 
        / 
        sum(rate(themisdb_http_requests_total[5m])) > 0.05
      for: 5m
      labels:
        severity: critical
        component: api
      annotations:
        summary: "ThemisDB hohe Fehlerrate"
        description: "{{ $value | humanizePercentage }} der Requests schlagen fehl (Threshold: 5%)"
        dashboard: "https://grafana/d/themisdb/overview"
    
    # Alert bei niedriger Cache Hit Rate (<80%)
    - alert: LowCacheHitRate
      expr: |
        themisdb_cache_hits_total / 
        (themisdb_cache_hits_total + themisdb_cache_misses_total) < 0.80
      for: 15m
      labels:
        severity: warning
        component: cache
      annotations:
        summary: "ThemisDB niedrige Cache Hit Rate"
        description: "Cache Hit Rate: {{ $value | humanizePercentage }} auf {{ $labels.instance }} (Threshold: 80%)"
        recommendation: "Erhöhe cache.size_mb in Konfiguration"
  
  # Gruppe 2: Resource Alerts
  - name: themisdb.resources
    interval: 30s
    rules:
    # Alert bei hoher Memory-Auslastung (>90%)
    - alert: HighMemoryUsage
      expr: |
        (container_memory_usage_bytes{pod=~"themisdb-.*"} / 
         container_spec_memory_limit_bytes{pod=~"themisdb-.*"}) > 0.90
      for: 5m
      labels:
        severity: warning
        component: resources
      annotations:
        summary: "ThemisDB hohe Memory-Auslastung"
        description: "Memory-Auslastung: {{ $value | humanizePercentage }} auf {{ $labels.pod }}"
    
    # Alert bei niedrigem Disk Space (<10%)
    - alert: LowDiskSpace
      expr: |
        (node_filesystem_avail_bytes{mountpoint="/var/lib/themisdb/data"} / 
         node_filesystem_size_bytes{mountpoint="/var/lib/themisdb/data"}) < 0.10
      for: 5m
      labels:
        severity: critical
        component: storage
        page: "true"
      annotations:
        summary: "ThemisDB kritisch niedriger Disk Space"
        description: "Nur {{ $value | humanizePercentage }} Disk Space verfügbar auf {{ $labels.instance }}"
        action: "Erweitere Volume oder lösche alte Backups"
    
    # Alert bei hoher CPU-Auslastung (>80% sustained)
    - alert: HighCPUUsage
      expr: |
        rate(container_cpu_usage_seconds_total{pod=~"themisdb-.*"}[5m]) > 0.80
      for: 15m
      labels:
        severity: warning
        component: resources
      annotations:
        summary: "ThemisDB hohe CPU-Auslastung"
        description: "CPU-Auslastung: {{ $value | humanizePercentage }} auf {{ $labels.pod }}"
  
  # Gruppe 3: Replication Alerts
  - name: themisdb.replication
    interval: 30s
    rules:
    # Alert bei hoher Replication Lag (>5 Sekunden)
    - alert: HighReplicationLag
      expr: |
        themisdb_replication_lag_seconds > 5
      for: 2m
      labels:
        severity: warning
        component: replication
      annotations:
        summary: "ThemisDB hohe Replication Lag"
        description: "Replication Lag: {{ $value | humanizeDuration }} auf Replica {{ $labels.replica_id }}"
    
    # Alert bei Replication Failure
    - alert: ReplicationFailure
      expr: |
        themisdb_replication_status{status!="healthy"} == 1
      for: 1m
      labels:
        severity: critical
        component: replication
      annotations:
        summary: "ThemisDB Replication Fehler"
        description: "Replication zu {{ $labels.replica_id }} fehlgeschlagen: {{ $labels.status }}"
  
  # Gruppe 4: Backup Alerts
  - name: themisdb.backup
    interval: 5m
    rules:
    # Alert wenn letztes Backup älter als 25 Stunden
    - alert: BackupOverdue
      expr: |
        (time() - themisdb_last_successful_backup_timestamp_seconds) / 3600 > 25
      for: 30m
      labels:
        severity: warning
        component: backup
      annotations:
        summary: "ThemisDB Backup überfällig"
        description: "Letztes erfolgreiches Backup vor {{ $value | humanizeDuration }}"
    
    # Alert bei Backup-Fehler
    - alert: BackupFailed
      expr: |
        increase(themisdb_backup_failures_total[1h]) > 0
      for: 5m
      labels:
        severity: critical
        component: backup
      annotations:
        summary: "ThemisDB Backup fehlgeschlagen"
        description: "{{ $value }} Backup-Fehler in der letzten Stunde"
```

### 30.5.3 Grafana Dashboard Design Patterns {#chapter_30_5_3_grafana_dashboards}

Grafana-Dashboards visualisieren Prometheus-Metriken und ermöglichen schnelle Problemdiagnose. Wir folgen Best Practices: Gruppierung nach Komponenten, Verwendung von Row-Panels für logische Sections, konsistente Farbschemata (rot für kritisch, gelb für warning, grün für normal) und sinnvolle Zeitfenster-Auswahl.

**Dashboard-Struktur:**

```json
{
  "dashboard": {
    "title": "ThemisDB Production Overview",
    "tags": ["themisdb", "production", "database"],
    "timezone": "browser",
    "refresh": "30s",
    "time": {
      "from": "now-1h",
      "to": "now"
    },
    "panels": [
      {
        "id": 1,
        "title": "Query Rate (QPS)",
        "type": "graph",
        "targets": [
          {
            "expr": "sum(rate(themisdb_http_requests_total[5m]))",
            "legendFormat": "Total QPS",
            "refId": "A"
          }
        ],
        "yaxes": [
          {
            "label": "Queries per second",
            "format": "short"
          }
        ]
      },
      {
        "id": 2,
        "title": "P95 Query Latency",
        "type": "graph",
        "targets": [
          {
            "expr": "histogram_quantile(0.95, sum(rate(themisdb_query_duration_seconds_bucket[5m])) by (le))",
            "legendFormat": "P95 Latency",
            "refId": "A"
          },
          {
            "expr": "histogram_quantile(0.99, sum(rate(themisdb_query_duration_seconds_bucket[5m])) by (le))",
            "legendFormat": "P99 Latency",
            "refId": "B"
          }
        ],
        "thresholds": [
          {
            "value": 0.5,
            "colorMode": "critical",
            "op": "gt",
            "fill": true,
            "line": true
          }
        ],
        "yaxes": [
          {
            "label": "Seconds",
            "format": "s"
          }
        ]
      },
      {
        "id": 3,
        "title": "Error Rate",
        "type": "graph",
        "targets": [
          {
            "expr": "sum(rate(themisdb_http_requests_total{status=~\"5..\"}[5m])) / sum(rate(themisdb_http_requests_total[5m]))",
            "legendFormat": "Error Rate",
            "refId": "A"
          }
        ],
        "yaxes": [
          {
            "label": "Percentage",
            "format": "percentunit",
            "max": 0.1
          }
        ],
        "alert": {
          "name": "High Error Rate",
          "conditions": [
            {
              "evaluator": {
                "params": [0.05],
                "type": "gt"
              },
              "query": {
                "params": ["A", "5m", "now"]
              }
            }
          ]
        }
      },
      {
        "id": 4,
        "title": "Database Size",
        "type": "graph",
        "targets": [
          {
            "expr": "themisdb_db_size_bytes",
            "legendFormat": "{{instance}}",
            "refId": "A"
          }
        ],
        "yaxes": [
          {
            "label": "Bytes",
            "format": "bytes"
          }
        ]
      },
      {
        "id": 5,
        "title": "Cache Hit Ratio",
        "type": "stat",
        "targets": [
          {
            "expr": "themisdb_cache_hits_total / (themisdb_cache_hits_total + themisdb_cache_misses_total)",
            "refId": "A"
          }
        ],
        "options": {
          "graphMode": "area",
          "colorMode": "value"
        },
        "fieldConfig": {
          "defaults": {
            "unit": "percentunit",
            "thresholds": {
              "mode": "absolute",
              "steps": [
                {"value": 0, "color": "red"},
                {"value": 0.7, "color": "yellow"},
                {"value": 0.85, "color": "green"}
              ]
            }
          }
        }
      },
      {
        "id": 6,
        "title": "Active Connections",
        "type": "graph",
        "targets": [
          {
            "expr": "themisdb_active_connections",
            "legendFormat": "{{instance}}",
            "refId": "A"
          }
        ],
        "yaxes": [
          {
            "label": "Connections",
            "format": "short"
          }
        ]
      }
    ]
  }
}
```

### 30.5.4 SLO/SLI Tracking und Error Budgets {#chapter_30_5_4_slo_sli_tracking}

Service Level Objectives (SLOs) definieren messbare Ziele für Service-Qualität, während Service Level Indicators (SLIs) die tatsächlichen Metriken darstellen. Error Budgets berechnen die tolerierbare Fehlerrate basierend auf dem SLO. Wenn das Error Budget aufgebraucht ist, priorisieren wir Stabilität über neue Features.

**SLO Definition:**

```yaml
# slo-definition.yaml
# Definiert SLOs für ThemisDB Production Service
apiVersion: sloth.slok.dev/v1
kind: PrometheusServiceLevel
metadata:
  name: themisdb-api-slo
  namespace: monitoring
spec:
  service: "themisdb-api"
  labels:
    team: "database"
    tier: "critical"
  # SLOs definieren: 99.9% Availability, <500ms P95 Latency
  slos:
    # SLO 1: Availability (99.9% = 43.2min Downtime pro Monat)
    - name: "requests-availability"
      objective: 99.9
      description: "99.9% der Requests müssen erfolgreich sein"
      sli:
        events:
          errorQuery: sum(rate(themisdb_http_requests_total{status=~"5.."}[{{.window}}]))
          totalQuery: sum(rate(themisdb_http_requests_total[{{.window}}]))
      alerting:
        name: ThemisDBHighErrorRate
        labels:
          category: "availability"
        annotations:
          summary: "ThemisDB Error Budget wird aufgebraucht"
        pageAlert:
          labels:
            severity: critical
        ticketAlert:
          labels:
            severity: warning
    
    # SLO 2: Latency (95% der Requests <500ms)
    - name: "requests-latency"
      objective: 95
      description: "95% der Requests müssen unter 500ms antworten"
      sli:
        events:
          errorQuery: |
            sum(rate(themisdb_query_duration_seconds_bucket{le="0.5"}[{{.window}}]))
          totalQuery: |
            sum(rate(themisdb_query_duration_seconds_count[{{.window}}]))
      alerting:
        name: ThemisDBHighLatency
        labels:
          category: "latency"
        annotations:
          summary: "ThemisDB Latency SLO verletzt"
```

**Error Budget Calculation:**

```
SLO: 99.9% Availability
Error Budget: 100% - 99.9% = 0.1%

Bei 1 Million Requests pro Tag:
Erlaubte Fehler: 1.000.000 * 0.001 = 1.000 Requests

Wenn Error Budget aufgebraucht:
→ Deployment-Freeze (außer Hotfixes)
→ Focus auf Stabilität statt Features
→ Incident Post-Mortems
→ Identifikation von Reliability-Improvements
```

### 30.5.5 Monitoring-Systeme im Vergleich {#chapter_30_5_5_monitoring_comparison}

Verschiedene Monitoring-Lösungen bieten unterschiedliche Trade-offs zwischen Features, Performance und Betriebsaufwand. Wir vergleichen Prometheus, VictoriaMetrics und Thanos hinsichtlich Retention, Query-Performance, Storage-Overhead und Alerting-Latenz.

**Tabelle 30.3: Monitoring-Systeme Vergleichsmatrix**

| Metric Type | Retention | Query Performance | Storage Overhead | Alerting Latency | Use Case |
|-------------|-----------|-------------------|------------------|------------------|----------|
| Prometheus | 15 days | <100ms | 2GB/day | 30-60s | Standard, Single-Cluster |
| VictoriaMetrics | 90 days | <50ms | 0.8GB/day | 15-30s | High-Cardinality, Cost-Optimized |
| Thanos | Unlimited | 200-500ms | 1.5GB/day | 60-120s | Multi-Cluster, Long-Term Storage |
| Datadog | Unlimited | <200ms | N/A (SaaS) | <30s | Enterprise, Managed |

**Methodik:** Query Performance gemessen als P95-Latenz für typische PromQL-Queries über 1h Zeitfenster. Storage Overhead für 1000 aktive Metriken mit 30s Scrape-Interval. Alerting Latency ist die Zeit von Schwellenwert-Überschreitung bis Alert-Notification. Retention ist die Standard-Konfiguration ohne zusätzliche Archivierung.

---

## 30.6 Disaster Recovery & Backup Strategies {#chapter_30_6_disaster_recovery}

Wir betrachten in diesem Abschnitt umfassende Backup- und Disaster-Recovery-Strategien für ThemisDB. Ein robuster Backup-Plan schützt vor Datenverlust durch Hardware-Ausfälle, Softwarefehler, menschliche Fehler oder Katastrophenereignisse. Wir implementieren mehrstufige Backup-Strategien (Full, Incremental, Continuous) mit automatischer Validierung und definierten Recovery-Time-Objective (RTO) sowie Recovery-Point-Objective (RPO) Zielen.

### 30.6.1 Backup-Strategien: Full, Incremental, Continuous {#chapter_30_6_1_backup_strategies}

Verschiedene Backup-Typen bieten unterschiedliche Trade-offs zwischen Backup-Dauer, Storage-Bedarf und Recovery-Komplexität. Full Backups sichern alle Daten, Incremental Backups nur Änderungen seit dem letzten Backup, und Continuous Backups (CDC) streamen Änderungen in Echtzeit. Die Wahl hängt von RTO/RPO-Anforderungen und verfügbaren Ressourcen ab.

**Full Backup:** Komplettes Datenbank-Snapshot  
**Incremental Backup:** Nur Änderungen seit letztem Backup  
**Continuous Backup (CDC):** Echtzeit-Replikation von Änderungen

### 30.6.2 Point-in-Time Recovery (PITR) Verfahren {#chapter_30_6_2_pitr_procedures}

Point-in-Time Recovery ermöglicht die Wiederherstellung der Datenbank auf einen beliebigen Zeitpunkt in der Vergangenheit. Dies ist essentiell für Recovery nach Datenkorruption oder versehentlichen Löschungen. Wir verwenden Write-Ahead-Logs (WAL) kombiniert mit regelmäßigen Base-Backups für PITR-Funktionalität.

**ThemisDB Backup Skript mit deutschen Kommentaren:**

```bash
#!/bin/bash
# themisdb-backup.sh - Vollständiges Backup-Skript für ThemisDB
# Erstellt Hot Backups ohne Downtime, verifiziert Integrität und synct zu S3

set -euo pipefail  # Exit bei Fehler, undefinierten Variablen, pipe failures

# ==================== Konfiguration ====================
BACKUP_DIR="/backups/themisdb"
RETENTION_DAYS=30
S3_BUCKET="s3://themisdb-backups-prod"
S3_REGION="eu-central-1"
THEMISDB_HOST="localhost"
THEMISDB_PORT="8529"
THEMISDB_USER="backup_user"
THEMISDB_PASSWORD="${THEMISDB_BACKUP_PASSWORD}"  # Aus Env Variable
LOG_FILE="/var/log/themisdb-backup.log"
SLACK_WEBHOOK="${SLACK_BACKUP_WEBHOOK}"  # Optional: Slack-Notifikationen

# ==================== Logging ====================
log() {
    echo "[$(date +'%Y-%m-%d %H:%M:%S')] $*" | tee -a "${LOG_FILE}"
}

error() {
    echo "[$(date +'%Y-%m-%d %H:%M:%S')] ERROR: $*" | tee -a "${LOG_FILE}" >&2
}

notify_slack() {
    if [[ -n "${SLACK_WEBHOOK}" ]]; then
        curl -X POST -H 'Content-type: application/json' \
            --data "{\"text\":\"$1\"}" \
            "${SLACK_WEBHOOK}" 2>/dev/null || true
    fi
}

# ==================== Pre-Flight Checks ====================
log "Starting ThemisDB backup procedure..."

# Prüfe ob Backup-Verzeichnis existiert
mkdir -p "${BACKUP_DIR}"

# Prüfe ThemisDB Erreichbarkeit
if ! curl -sf "http://${THEMISDB_HOST}:${THEMISDB_PORT}/_api/version" >/dev/null; then
    error "ThemisDB ist nicht erreichbar auf ${THEMISDB_HOST}:${THEMISDB_PORT}"
    notify_slack "❌ ThemisDB Backup fehlgeschlagen: Database nicht erreichbar"
    exit 1
fi

# ==================== Backup Execution ====================
# Timestamp für eindeutige Backup-Namen
TIMESTAMP=$(date +%Y%m%d_%H%M%S)
BACKUP_NAME="backup_${TIMESTAMP}"
BACKUP_PATH="${BACKUP_DIR}/${BACKUP_NAME}"

log "Erstelle Hot Backup: ${BACKUP_NAME}"

# Hot Backup erstellen (ohne Downtime)
# Verwendet ThemisDB-eigenen Backup-Mechanismus
if ! themisdb-backup create \
    --host="${THEMISDB_HOST}" \
    --port="${THEMISDB_PORT}" \
    --user="${THEMISDB_USER}" \
    --password="${THEMISDB_PASSWORD}" \
    --type=hot \
    --output="${BACKUP_PATH}" \
    --compress=true \
    --compression-level=6 \
    --include-indexes=true \
    --include-system-collections=true \
    --parallel-threads=4 \
    --timeout=3600; then
    error "Backup-Erstellung fehlgeschlagen"
    notify_slack "❌ ThemisDB Backup fehlgeschlagen: Backup-Erstellung Error"
    exit 1
fi

log "Backup erstellt: ${BACKUP_PATH}"

# ==================== Backup Verification ====================
log "Verifiziere Backup-Integrität..."

# Prüfe Backup-Struktur und Checksums
if themisdb-backup verify "${BACKUP_PATH}" --check-checksums; then
    log "✅ Backup erfolgreich verifiziert"
else
    error "❌ Backup-Verifikation fehlgeschlagen!"
    notify_slack "❌ ThemisDB Backup fehlgeschlagen: Verifikation Error"
    exit 1
fi

# Berechne Backup-Größe
BACKUP_SIZE=$(du -sh "${BACKUP_PATH}" | cut -f1)
log "Backup-Größe: ${BACKUP_SIZE}"

# ==================== Upload zu S3 ====================
log "Upload Backup zu S3..."

# Sync zu S3 mit Verschlüsselung und Storage-Class-Optimierung
# Verwende aws s3 sync für inkrementelle Uploads
if aws s3 sync "${BACKUP_PATH}" \
    "${S3_BUCKET}/$(date +%Y/%m/%d)/${BACKUP_NAME}/" \
    --region="${S3_REGION}" \
    --sse=AES256 \
    --storage-class=STANDARD_IA \
    --no-progress \
    --only-show-errors; then
    log "✅ Backup erfolgreich zu S3 hochgeladen"
else
    error "❌ S3 Upload fehlgeschlagen"
    notify_slack "❌ ThemisDB Backup fehlgeschlagen: S3 Upload Error"
    exit 1
fi

# ==================== Metadata Logging ====================
# Erstelle Metadata-Datei für Backup-Tracking
cat > "${BACKUP_PATH}/metadata.json" <<EOF
{
  "backup_name": "${BACKUP_NAME}",
  "timestamp": "$(date -Iseconds)",
  "themisdb_version": "$(curl -s http://${THEMISDB_HOST}:${THEMISDB_PORT}/_api/version | jq -r '.version')",
  "backup_size_bytes": $(du -sb "${BACKUP_PATH}" | cut -f1),
  "backup_type": "hot",
  "compression": "gzip",
  "s3_location": "${S3_BUCKET}/$(date +%Y/%m/%d)/${BACKUP_NAME}/",
  "retention_days": ${RETENTION_DAYS}
}
EOF

# Upload Metadata zu S3
aws s3 cp "${BACKUP_PATH}/metadata.json" \
    "${S3_BUCKET}/$(date +%Y/%m/%d)/${BACKUP_NAME}/metadata.json" \
    --region="${S3_REGION}"

# ==================== Cleanup: Alte lokale Backups löschen ====================
log "Bereinige alte lokale Backups (älter als ${RETENTION_DAYS} Tage)..."

DELETED_COUNT=0
while IFS= read -r old_backup; do
    log "Lösche altes Backup: ${old_backup}"
    rm -rf "${old_backup}"
    ((DELETED_COUNT++))
done < <(find "${BACKUP_DIR}" -maxdepth 1 -type d -name "backup_*" -mtime +"${RETENTION_DAYS}")

log "Gelöscht: ${DELETED_COUNT} alte Backups"

# ==================== S3 Lifecycle Policy (optional) ====================
# S3 Lifecycle Policy automatisch alte Backups nach 90 Tagen zu Glacier verschieben
# und nach 365 Tagen löschen (wird via Terraform/AWS Console konfiguriert)

# ==================== Abschluss ====================
log "✅ Backup erfolgreich abgeschlossen: ${BACKUP_NAME}"
log "   Größe: ${BACKUP_SIZE}"
log "   Lokaler Pfad: ${BACKUP_PATH}"
log "   S3 Location: ${S3_BUCKET}/$(date +%Y/%m/%d)/${BACKUP_NAME}/"

# Erfolgs-Notification
notify_slack "✅ ThemisDB Backup erfolgreich: ${BACKUP_NAME} (${BACKUP_SIZE})"

# ==================== Metriken für Monitoring ====================
# Sende Metriken an Prometheus Pushgateway (optional)
if command -v curl &> /dev/null; then
    cat <<EOF | curl --data-binary @- http://prometheus-pushgateway:9091/metrics/job/themisdb_backup
# HELP themisdb_backup_success_timestamp Timestamp of last successful backup
# TYPE themisdb_backup_success_timestamp gauge
themisdb_backup_success_timestamp $(date +%s)
# HELP themisdb_backup_size_bytes Size of last backup in bytes
# TYPE themisdb_backup_size_bytes gauge
themisdb_backup_size_bytes $(du -sb "${BACKUP_PATH}" | cut -f1)
# HELP themisdb_backup_duration_seconds Duration of backup in seconds
# TYPE themisdb_backup_duration_seconds gauge
themisdb_backup_duration_seconds ${SECONDS}
EOF
fi

exit 0
```

**Cron Job für automatische Backups:**

```cron
# /etc/cron.d/themisdb-backup
# Automatische Backups für ThemisDB

# Tägliches Full Backup um 2:00 Uhr nachts
0 2 * * * backup_user /usr/local/bin/themisdb-backup.sh >> /var/log/themisdb-backup.log 2>&1

# Stündliches Incremental Backup (WAL-Archivierung)
0 * * * * backup_user /usr/local/bin/themisdb-wal-archive.sh >> /var/log/themisdb-wal.log 2>&1

# Wöchentlicher Backup-Verification Test (Sonntag 3:00 Uhr)
0 3 * * 0 backup_user /usr/local/bin/themisdb-restore-test.sh >> /var/log/themisdb-restore-test.log 2>&1
```

### 30.6.3 Cross-Region Replication für Disaster Recovery {#chapter_30_6_3_cross_region_replication}

Für höchste Verfügbarkeit und Disaster-Recovery-Fähigkeiten implementieren wir Cross-Region-Replication. Dies schützt vor Region-Ausfällen (Naturkatastrophen, Netzwerkausfälle) und ermöglicht Geo-Redundanz. Wir konfigurieren asynchrone Replikation zu einer Secondary-Region mit automatischem Failover.

**Cross-Region Setup:**

```yaml
# themisdb-replication-config.yaml
replication:
  enabled: true
  mode: async  # Asynchrone Replikation für niedrige Latenz
  
  # Primary Region: EU-Central-1
  primary:
    region: eu-central-1
    endpoints:
      - themisdb-0.eu-central-1:8529
      - themisdb-1.eu-central-1:8529
      - themisdb-2.eu-central-1:8529
  
  # Secondary Region: EU-West-1 (Disaster Recovery)
  secondary:
    region: eu-west-1
    endpoints:
      - themisdb-0.eu-west-1:8529
      - themisdb-1.eu-west-1:8529
      - themisdb-2.eu-west-1:8529
    lag_max: 5s  # Maximal tolerierte Replikations-Verzögerung
    
  # Failover-Konfiguration
  failover:
    enabled: true
    automatic: true
    health_check_interval: 10s
    health_check_timeout: 5s
    failover_delay: 30s  # Warte 30s vor Failover zur Secondary
    
  # Konfliktauflösung bei Split-Brain
  conflict_resolution:
    strategy: last_write_wins
    timestamp_source: ntp
```

### 30.6.4 RTO/RPO Targets und SLA-Definitionen {#chapter_30_6_4_rto_rpo_targets}

Recovery Time Objective (RTO) definiert die maximal tolerierbare Downtime, Recovery Point Objective (RPO) die maximal tolerierbare Datenverlust-Zeitspanne. Wir definieren tier-basierte SLAs mit entsprechenden Backup-Strategien und Kosten.

**RTO/RPO Matrix:**

```
┌──────────────────────────────────────────────────────────────────────────┐
│ Service Tier │ RTO      │ RPO        │ Backup Strategy        │ Cost/Mo │
├──────────────────────────────────────────────────────────────────────────┤
│ Bronze       │ 24h      │ 24h        │ Daily Full Backup      │ $100    │
│ Silver       │ 4h       │ 1h         │ Daily Full + Hourly    │ $500    │
│              │          │            │ Incremental            │         │
│ Gold         │ 1h       │ 5min       │ Continuous Backup +    │ $2,000  │
│              │          │            │ Regional Replication   │         │
│ Platinum     │ 15min    │ 0min (RPO) │ Multi-Region Sync +    │ $5,000  │
│              │          │            │ Quorum Write           │         │
└──────────────────────────────────────────────────────────────────────────┘

Platinum-Tier Details:
- 4 Data Centers (2 Primary, 2 Standby)
- Synchronous Replication in Primary DCs
- Asynchronous Replication zu Standby DCs
- Quorum Write: 3 von 4 DCs müssen bestätigen
- Automatisches Failover innerhalb 15 Minuten
```

### 30.6.5 Backup-Validierung und Restore-Testing {#chapter_30_6_5_backup_validation}

Backups sind nutzlos, wenn sie im Ernstfall nicht wiederhergestellt werden können. Wir implementieren regelmäßige Restore-Tests, die automatisch Backups in Testumgebungen wiederherstellen und Integritätschecks durchführen. Dies validiert sowohl Backup-Integrität als auch Restore-Prozeduren.

**Restore-Test-Skript:**

```bash
#!/bin/bash
# themisdb-restore-test.sh - Automatischer Backup-Restore-Test
# Führt wöchentlich Restore-Test mit zufälligem Backup durch

set -euo pipefail

log() {
    echo "[$(date +'%Y-%m-%d %H:%M:%S')] $*"
}

error() {
    echo "[$(date +'%Y-%m-%d %H:%M:%S')] ERROR: $*" >&2
}

# Konfiguration
S3_BUCKET="s3://themisdb-backups-prod"
TEST_RESTORE_DIR="/tmp/themisdb-restore-test"
TEST_PORT="9529"  # Anderer Port als Produktion

log "Starting automated restore test..."

# 1. Liste alle verfügbaren Backups
log "Fetching backup list from S3..."
mapfile -t BACKUPS < <(aws s3 ls "${S3_BUCKET}/" --recursive | grep "metadata.json" | awk '{print $4}' | sort -r | head -10)

if [ ${#BACKUPS[@]} -eq 0 ]; then
    error "No backups found in S3"
    exit 1
fi

# 2. Wähle zufälliges Backup
RANDOM_BACKUP="${BACKUPS[$RANDOM % ${#BACKUPS[@]}]}"
BACKUP_DIR=$(dirname "${RANDOM_BACKUP}")
log "Selected backup for testing: ${BACKUP_DIR}"

# 3. Download Backup von S3
log "Downloading backup from S3..."
mkdir -p "${TEST_RESTORE_DIR}"
aws s3 sync "${S3_BUCKET}/${BACKUP_DIR}" "${TEST_RESTORE_DIR}" --quiet

# 4. Verifiziere Backup-Integrität
log "Verifying backup integrity..."
if ! themisdb-backup verify "${TEST_RESTORE_DIR}" --check-checksums; then
    error "Backup verification failed!"
    exit 1
fi

# 5. Starte temporäre ThemisDB-Instanz
log "Starting temporary ThemisDB instance on port ${TEST_PORT}..."
docker run -d \
    --name themisdb-restore-test \
    -p ${TEST_PORT}:8529 \
    -v "${TEST_RESTORE_DIR}:/backup:ro" \
    themisdb:latest \
    themisdb-server --port=${TEST_PORT} --restore-from=/backup

# Warte bis Instanz bereit ist
log "Waiting for ThemisDB to become ready..."
for i in {1..30}; do
    if curl -sf "http://localhost:${TEST_PORT}/_api/version" >/dev/null 2>&1; then
        log "✅ ThemisDB started successfully"
        break
    fi
    sleep 2
done

# 6. Führe Smoke Tests durch
log "Running smoke tests..."

# Test 1: Version Check
VERSION=$(curl -s "http://localhost:${TEST_PORT}/_api/version" | jq -r '.version')
log "Version: ${VERSION}"

# Test 2: Collection Count
COLLECTION_COUNT=$(curl -s "http://localhost:${TEST_PORT}/_api/collection" | jq '. | length')
log "Collections: ${COLLECTION_COUNT}"

# Test 3: Sample Query
if curl -sf "http://localhost:${TEST_PORT}/_api/cursor" \
    -X POST \
    -H "Content-Type: application/json" \
    -d '{"query": "FOR doc IN users LIMIT 10 RETURN doc"}' >/dev/null; then
    log "✅ Sample query successful"
else
    error "Sample query failed"
    docker stop themisdb-restore-test
    docker rm themisdb-restore-test
    exit 1
fi

# 7. Cleanup
log "Cleaning up..."
docker stop themisdb-restore-test
docker rm themisdb-restore-test
rm -rf "${TEST_RESTORE_DIR}"

log "✅ Restore test completed successfully"

# 8. Sende Metriken
cat <<EOF | curl --data-binary @- http://prometheus-pushgateway:9091/metrics/job/themisdb_restore_test
# HELP themisdb_restore_test_success_timestamp Timestamp of last successful restore test
# TYPE themisdb_restore_test_success_timestamp gauge
themisdb_restore_test_success_timestamp $(date +%s)
# HELP themisdb_restore_test_duration_seconds Duration of restore test
# TYPE themisdb_restore_test_duration_seconds gauge
themisdb_restore_test_duration_seconds ${SECONDS}
EOF

exit 0
```

### 30.6.6 Backup-Typen im Vergleich {#chapter_30_6_6_backup_comparison}

Die Wahl der richtigen Backup-Strategie hängt von RTO/RPO-Anforderungen, verfügbarem Storage-Budget und Komplexitätstoleranz ab. Nachfolgende Tabelle vergleicht die drei Haupt-Backup-Methoden.

**Tabelle 30.4: Backup-Typen Vergleichsmatrix**

| Backup Type | Backup Time | Storage Size | RTO | RPO | Methodology |
|-------------|-------------|--------------|-----|-----|-------------|
| Full | 45 min | 100% | 1h | 24h | Complete snapshot, standalone restore |
| Incremental | 8 min | 15% | 2h | 1h | Changes since last backup, chain restore |
| Continuous (CDC) | N/A | 120% | 5 min | <1 min | Event streaming, WAL replay |

**Methodik:** Backup Time gemessen für 500GB-Datenbank mit Standard-Hardware (NVMe SSD, 10GbE). Storage Size relativ zu Datenbankgröße (Continuous benötigt Base + WAL-Archive). RTO ist typische Recovery-Zeit inklusive Download und Restore. RPO ist maximal möglicher Datenverlust. Full Backups sind standalone, Incremental benötigt vollständige Kette, Continuous benötigt Base + WAL-Replay.

---

## 30.7 Horizontal Scaling {#chapter_30_7_horizontal_scaling}

### 30.7.1 Sharding Strategy {#chapter_30_7_1_sharding_strategy}

```mermaid
flowchart TB
    A[Load Balancer] --> B[Shard 1<br/>ID Range: 0-333]
    A --> C[Shard 2<br/>ID Range: 334-666]
    A --> D[Shard 3<br/>ID Range: 667-999]
    
    B --> B1[(RocksDB 1)]
    C --> C1[(RocksDB 2)]
    D --> D1[(RocksDB 3)]
    
    style A fill:#e1f5ff
    style B fill:#fff4e1
    style C fill:#e1ffe1
    style D fill:#ffe1f5
```

Abb. 30.1: Deployment-Strategy-Matrix

**Sharding Config:**

```yaml
sharding:
  enabled: true
  strategy: hash
  shards:
    - id: shard1
      endpoint: http://themis-shard1:8529
      range: [0, 333]
    - id: shard2
      endpoint: http://themis-shard2:8529
      range: [334, 666]
    - id: shard3
      endpoint: http://themis-shard3:8529
      range: [667, 999]
```

### 30.7.2 Read Replicas {#chapter_30_7_2_read_replicas}

```yaml
replication:
  enabled: true
  mode: async
  replicas:
    - endpoint: http://themis-replica1:8529
      lag_max: 1s
    - endpoint: http://themis-replica2:8529
      lag_max: 1s
```

---

## 30.8 Security Best Practices {#chapter_30_8_security_practices}

### 30.8.1 TLS/SSL Configuration {#chapter_30_8_1_tls_ssl}

```yaml
server:
  tls:
    enabled: true
    cert_file: /etc/themis/tls/cert.pem
    key_file: /etc/themis/tls/key.pem
    ca_file: /etc/themis/tls/ca.pem
    min_version: "1.3"
```

### 30.8.2 Authentication {#chapter_30_8_2_authentication}

```yaml
auth:
  enabled: true
  providers:
    - type: jwt
      issuer: "https://auth.example.com"
      audience: "themis-api"
      jwks_uri: "https://auth.example.com/.well-known/jwks.json"
    - type: basic
      users_file: /etc/themis/users.htpasswd
```

### 30.8.3 Network Policies (Kubernetes) {#chapter_30_8_3_network_policies}

```yaml
apiVersion: networking.k8s.io/v1
kind: NetworkPolicy
metadata:
  name: themis-netpol
  namespace: themis-prod
spec:
  podSelector:
    matchLabels:
      app: themis
  policyTypes:
  - Ingress
  - Egress
  ingress:
  - from:
    - podSelector:
        matchLabels:
          app: frontend
    ports:
    - protocol: TCP
      port: 8529
  egress:
  - to:
    - podSelector:
        matchLabels:
          app: redis
    ports:
    - protocol: TCP
      port: 6379
```

---

## 30.9 Performance Tuning {#chapter_30_9_performance_tuning}

### 30.9.1 Resource Limits {#chapter_30_9_1_resource_limits}

```yaml
resources:
  requests:
    memory: "8Gi"
    cpu: "4000m"
  limits:
    memory: "16Gi"
    cpu: "8000m"

# JVM-style tuning for RocksDB
env:
  - name: ROCKSDB_MAX_OPEN_FILES
    value: "10000"
  - name: ROCKSDB_BLOCK_CACHE_SIZE
    value: "4GB"
```

### 30.9.2 Connection Pooling {#chapter_30_9_2_connection_pooling}

```yaml
server:
  connection_pool:
    size: 100
    timeout: 30s
    max_idle: 50
```

---

## 30.10 Advanced Deployment Patterns {#chapter_30_10_advanced_deployment}

### 30.10.1 Blue-Green Deployments

```yaml
# Strategy: Run two complete ThemisDB environments
# Route traffic via load balancer
apiVersion: v1
kind: Service
metadata:
  name: themis-service
spec:
  selector:
    app: themis
    version: blue  # Initially point to blue
  ports:
    - port: 8529
      targetPort: 8529
  type: LoadBalancer
---
# Blue: Current production
apiVersion: v1
kind: ConfigMap
metadata:
  name: themis-blue-config
data:
  version: "blue"
  database:
    replication_role: "primary"
---
# Green: New version (idle)
apiVersion: v1
kind:ConfigMap
metadata:
  name: themis-green-config
data:
  version: "green"
  database:
    replication_role: "primary"
```

**Deployment Steps:**
1. Deploy Green with new version (idle)
2. Run smoke tests against Green
3. Sync data: Primary → Green (catchup)
4. Switch load balancer: Service selector → green
5. Blue becomes backup/rollback

### 30.10.2 Canary Deployments

```yaml
# Route % of traffic to new version
# Gradually increase as stability confirmed
---
apiVersion: networking.istio.io/v1beta1
kind: VirtualService
metadata:
  name: themis-canary
spec:
  hosts:
  - themis.internal
  http:
  - match:
    - uri:
        prefix: "/"
    route:
    - destination:
        host: themis-stable
        port:
          number: 8529
      weight: 95  # 95% to stable
    - destination:
        host: themis-canary
        port:
          number: 8529
      weight: 5   # 5% to canary (new version)
    timeout: 10s
    retries:
      attempts: 3
      perTryTimeout: 5s
```

**Canary Rollout Timeline:**
- Hour 0-1: 5% traffic → Monitor error rates, latency
- Hour 1-4: 25% traffic → Check P99 latency
- Hour 4-8: 50% traffic → Full test with real workload
- Hour 8+: 100% traffic → Full rollout

### 30.10.3 GitOps Continuous Deployment

```yaml
# Use ArgoCD to sync Kubernetes manifests from Git
apiVersion: argoproj.io/v1alpha1
kind: Application
metadata:
  name: themis-gitops
spec:
  project: default
  source:
    repoURL: https://github.com/org/themis-deployment.git
    targetRevision: main
    path: k8s/production
  destination:
    server: https://kubernetes.default.svc
    namespace: themis
  syncPolicy:
    automated:
      prune: true
      selfHeal: true
    syncOptions:
    - CreateNamespace=true
  # Only deploy if tests pass in CI
  notification:
    when:
      onSuccess: notify-slack
      onFailure: notify-pagerduty
```

### 30.10.4 Staged Rollout Checklist

```
Pre-Deployment
  ✓ Run all integration tests
  ✓ Performance benchmarks (P99 latency < 10% increase)
  ✓ Memory/CPU profiling
  ✓ Backup created & tested

Deployment
  ✓ Dry-run: 'kubectl apply --dry-run=client -f manifest.yaml'
  ✓ Deploy to staging first
  ✓ Smoke tests pass
  ✓ Metrics baseline: CPU, Memory, QPS, Latency
  ✓ Error rate < 0.1%

Monitoring (First Hour)
  ✓ P99 latency stable
  ✓ No error spikes
  ✓ No memory leaks
  ✓ Replication lag < 500ms

Post-Deployment
  ✓ Keep blue environment for 24h (rollback ready)
  ✓ Archive logs for audit
  ✓ Document any issues/hotfixes
  ✓ Send deployment notification
```

---

## 30.11 Disaster Recovery & Business Continuity {#chapter_30_11_business_continuity}

### 30.11.1 RTO/RPO Strategy

```
RTO (Recovery Time Objective) = Max downtime acceptable
RPO (Recovery Point Objective) = Max data loss acceptable

ThemisDB Recommendations:
  ┌──────────────────────────────────────────────────┐
  │ Tier      │ RTO      │ RPO      │ Cost  │ Effort│
  ├──────────────────────────────────────────────────┤
  │ Bronze    │ 24h      │ 24h      │ $     │ Low   │
  │ Silver    │ 4h       │ 1h       │ $$    │ Med   │
  │ Gold      │ 1h       │ 5min     │ $$$   │ High  │
  │ Platinum  │ 15min    │ 0min(RPO)│ $$$$  │ Critical
  └──────────────────────────────────────────────────┘

Platinum = Multi-region sync (4 data centers, quorum write)
```

### 30.11.2 Disaster Recovery Plan

```yaml
# Automated DR Workflow
Kind: DisasterRecoveryPlan
metadata:
  name: themis-dr-plan
spec:
  backup:
    primary_daily: /backups/daily/themis-*.tar.gz
    archive_weekly: s3://backups/weekly/
    retention_years: 3
    
  failover_sequence:
    - detect_failure: "Primary health check fails 3x"
    - promote_secondary: "Promote Secondary to Primary"
    - validate_data: "Count docs, verify checksums"
    - test_write: "Insert test document"
    - notify_team: "PagerDuty alert + Slack message"
    - client_failover: "Update DNS CNAME"
    - monitor_2h: "Watch metrics for 2 hours"
    
  recovery_from_backup:
    - stop_database: "systemctl stop themis"
    - restore_volume: "Restore from snapshot"
    - verify_integrity: "themisdb recover --verify"
    - start_database: "systemctl start themis"
    - catchup_replicas: "Wait for lag < 1s"
```

### 30.11.3 Backup Verification Script

```bash
#!/bin/bash
# Daily backup integrity check

BACKUP_DIR="/backups/daily"
VERIFY_TIMEOUT=3600

for backup in $BACKUP_DIR/themis-*.tar.gz; do
  echo "Verifying $backup..."
  
  # Extract to temp, verify database
  TEMP_DIR=$(mktemp -d)
  tar -xzf "$backup" -C "$TEMP_DIR"
  
  # Run recovery to check integrity
  timeout $VERIFY_TIMEOUT themisdb recover --verify "$TEMP_DIR" && \
    echo "✓ $backup: OK" || \
    echo "✗ $backup: CORRUPT - Investigate!"
  
  # Try restoring to test instance
  docker run --rm -v "$TEMP_DIR:/data" themis:test \
    themisdb check-data /data && \
    echo "✓ Data schema valid" || \
    echo "✗ Schema check failed"
  
  rm -rf "$TEMP_DIR"
done

echo "Backup verification complete"
```

---

## 30.12 Cost Optimization {#chapter_30_12_cost_optimization}

### 30.12.1 Resource Right-Sizing

```yaml
# Development (small)
requests:
  memory: "1Gi"
  cpu: "500m"
limits:
  memory: "2Gi"
  cpu: "1000m"
storage: "50Gi"

# Staging (medium)
requests:
  memory: "8Gi"
  cpu: "4"
limits:
  memory: "16Gi"
  cpu: "8"
storage: "500Gi"

# Production (large)
requests:
  memory: "32Gi"
  cpu: "16"
limits:
  memory: "64Gi"
  cpu: "32"
storage: "2Ti"

# Cost Savings
# - Use spot instances for stateless pods: 70% savings
# - Right-size replicas: scale down off-peak hours
# - Archive old data to cold storage (S3 Glacier)
# - Estimated monthly savings: 40-60% via optimization
```

### 30.12.2 Storage Cost Reduction

```
Strategy: Tiered Storage (Hot → Warm → Cold)

Hot (SSD, expensive)
  ├─ Active data < 30 days
  ├─ Real-time queries
  └─ RTO requirement: <1h

Warm (HDD, medium)
  ├─ Data 30-90 days old
  ├─ Monthly analytics reports
  └─ RTO requirement: <24h

Cold (Cloud Archive, cheap)
  ├─ Compliance archives (2+ years)
  ├─ Quarterly audits only
  └─ RTO requirement: 1-7 days

Cost/GB/Month: SSD ($0.10) → HDD ($0.05) → Archive ($0.004)

Example: 10TB dataset
  SSD: $1,024/month
  Mixed (70% warm, 30% cold): $216/month
  Savings: 79%
```

---

## 30.13 Compliance & Audit {#chapter_30_13_compliance_audit}

### 30.13.1 Audit Logging for Compliance

```yaml
# Immutable audit log (Write-Once-Read-Many)
kind: ThemisDB
metadata:
  name: themis-production
spec:
  audit:
    enabled: true
    destination: s3://audit-logs/  # External WORM storage
    events:
      - user_login
      - data_access
      - schema_change
      - admin_action
      - privilege_grant
      - backup_restore
    
  compliance:
    frameworks:
      - SOC2
      - ISO27001
      - GDPR
      - HIPAA (if healthcare)
    
  data_retention:
    audit_logs: "7 years"
    application_logs: "90 days"
    metrics: "1 year"
```

### 30.13.2 Compliance Checklist

```
Security
  ✓ TLS 1.3 for all connections
  ✓ Network policies (allow only required)
  ✓ RBAC with MFA for admin access
  ✓ Secrets in Vault/K8s Secrets
  ✓ Encryption at rest (AES-256)
  ✓ Encryption in transit (TLS)

Data Protection
  ✓ PII detection & masking
  ✓ Data classification (PUBLIC/INTERNAL/CONFIDENTIAL)
  ✓ GDPR right-to-deletion implemented
  ✓ Backup encryption with managed keys
  ✓ No cleartext passwords in logs

Operations
  ✓ Change control (code review before deploy)
  ✓ Incident response playbooks
  ✓ Disaster recovery tested (quarterly)
  ✓ Security scanning (SBOM, vulnerability scanning)
  ✓ Penetration testing (annual)

Monitoring
  ✓ Access logs (all queries logged)
  ✓ Admin action audit trail
  ✓ Security event alerting
  ✓ SLA monitoring (uptime SLA > 99.9%)
```

---

## 30.14 Operations Runbooks

### 30.14.1 Scaling Checklist (Horizontal)

```
Add Node to Cluster
  1. Provision new node (VM/Bare Metal)
  2. Install ThemisDB + dependencies
  3. Configure as follower initially
  4. Join cluster: themisdb cluster join --primary <primary-ip>
  5. Wait for replication lag < 100ms
  6. Promote to voting member: themisdb cluster promote --member-id <id>
  7. Verify: themisdb cluster status (should show HEALTHY)
  8. Monitor CPU/Memory/IO for 24h

Remove Node from Cluster
  1. Drain connections: themisdb admin drain-connections
  2. Wait for active queries to finish (30s timeout)
  3. Demote member: themisdb cluster demote --member-id <id>
  4. Remove from cluster: themisdb cluster remove --member-id <id>
  5. Verify: themisdb cluster status (node should be REMOVED)
  6. Decommission VM
```

### 30.14.2 Incident Response (Example: High Memory Usage)

```
Symptoms
  - Memory usage > 85%
  - OOM killer started (dmesg shows)
  - Queries failing with "Out of memory"

Investigation (2 min)
  curl http://localhost:8529/_admin/memory | jq .
  # Check: Cache size, Buffer pool, Active cursors

Fix (5 min)
  Option A: Restart (emergency)
    systemctl restart themis
    → Memory drops to baseline
    
  Option B: Increase memory (planned)
    Edit themis.conf: cache.size_mb = 16384
    systemctl reload themis
    
  Option C: Identify leak (analysis)
    themisdb profile --duration 60s | grep alloc
    → Find problematic query/operation

Verify
  curl http://localhost:8529/_admin/memory | jq .usage_pct
  # Should be < 80%
```

---

## 30.15 Zusammenfassung {#chapter_30_15_summary}

Wir haben in diesem Kapitel umfassende Deployment- und Operations-Strategien für ThemisDB behandelt. Von Container-Orchestrierung mit Kubernetes über Infrastructure as Code mit Terraform bis hin zu fortgeschrittenen Deployment-Strategien wie Canary Releases haben wir die gesamte Bandbreite moderner DevOps-Praktiken abgedeckt. Die Implementierung von robustem Monitoring, Alerting und Disaster-Recovery-Verfahren stellt sicher, dass ThemisDB-Produktionsumgebungen die höchsten Verfügbarkeits- und Zuverlässigkeitsstandards erfüllen.

### Kernpunkte {#chapter_30_15_1_key_points}

**Container Orchestration:** Kubernetes StatefulSets bieten die notwendige Stabilität und Skalierbarkeit für zustandsbehaftete Datenbank-Deployments mit Features wie Pod Anti-Affinity, Persistent Volumes und automatischem Health Checking.

**Infrastructure as Code:** Terraform ermöglicht reproduzierbare, versionierte Infrastruktur-Definitionen mit State-Management, Drift Detection und modularer Komposition für Multi-Environment-Deployments.

**Deployment Strategies:** Blue-Green, Canary und Rolling Update Strategien bieten unterschiedliche Trade-offs zwischen Downtime, Risiko und Infrastruktur-Kosten. Die Wahl hängt von spezifischen SLA-Anforderungen ab.

**Monitoring & Alerting:** Prometheus und Grafana bilden einen leistungsfähigen Monitoring-Stack mit SLO/SLI-basiertem Alerting, Error Budget Tracking und mehrstufigen Severity-Levels für effektive Incident Response.

**Disaster Recovery:** Mehrstufige Backup-Strategien (Full, Incremental, Continuous) kombiniert mit Cross-Region-Replication gewährleisten Datensicherheit und schnelle Recovery mit definierten RTO/RPO-Zielen.

### Operations Checkliste {#chapter_30_15_2_operations_checklist}

#### Tägliche Aufgaben
- [ ] Backup erfolgreich (log check)
- [ ] Replication lag < 500ms
- [ ] Disk usage < 80%
- [ ] No error logs above WARN
- [ ] Metrics collected (Prometheus)

#### Wöchentliche Aufgaben
- [ ] Backup test restoration (pick random)
- [ ] Security scan (OWASP top 10)
- [ ] Performance baseline check
- [ ] Review failed queries (slow log)
- [ ] Capacity planning review

#### Monatliche Aufgaben
- [ ] Disaster recovery drill
- [ ] Penetration testing (if required)
- [ ] Security policy review
- [ ] Cost optimization (right-size resources)
- [ ] Upgrade path planning

#### Quarterly
- [ ] Full disaster recovery test (restore from backup)
- [ ] Security audit
- [ ] Compliance verification
- [ ] Training for new operators

### Weiterführende Themen {#chapter_30_15_3_next_topics}

Die in diesem Kapitel behandelten Deployment- und Operations-Konzepte bilden die Grundlage für produktionsreife ThemisDB-Installationen. Für vertiefende Informationen zu verwandten Themen empfehlen wir:

- **[Kapitel 3 (Installation)](chapter_04_installation.md):** Grundlegende Installation und Setup-Verfahren
- **[Kapitel 19 (Monitoring)](chapter_19_monitoring_observability.md):** Detaillierte Monitoring-Konzepte und Metriken
- **[Kapitel 20 (Backup & Recovery)](chapter_20_backup.md):** Erweiterte Backup-Strategien und Recovery-Szenarien
- **[Kapitel 25 (DevOps & Infrastructure)](chapter_25_devops_infrastructure.md):** DevOps-Workflows und CI/CD-Integration
- **[Kapitel 36 (Security Hardening)](chapter_36_security_hardening.md):** Security Best Practices und Härtungsmaßnahmen
- **[Kapitel 38 (Observability & SRE)](chapter_38_observability_sre.md):** Site Reliability Engineering Prinzipien

---

## Literatur und Referenzen {#chapter_30_references}

Die folgenden wissenschaftlichen Quellen und technischen Dokumentationen bilden die Grundlage für die in diesem Kapitel vorgestellten Best Practices:

### Bücher und Monografien

**[1] Beyer, B., Jones, C., Petoff, J., & Murphy, N. R. (2016).** *Site Reliability Engineering: How Google Runs Production Systems*. O'Reilly Media. ISBN: 978-1491929124.  
→ Grundlegendes Werk zu SRE-Prinzipien, SLO/SLI-Definitionen und Error Budget Management.

**[2] Morris, K. (2020).** *Infrastructure as Code: Managing Servers in the Cloud* (2nd ed.). O'Reilly Media. ISBN: 978-1098114671.  
→ Umfassende Behandlung von IaC-Konzepten, Terraform Best Practices und State Management.

**[3] Humble, J., & Farley, D. (2010).** *Continuous Delivery: Reliable Software Releases through Build, Test, and Deployment Automation*. Addison-Wesley. ISBN: 978-0321601919.  
→ Deployment-Pipelines, Blue-Green und Canary Release Strategien.

**[4] Campbell, L., & Majors, C. (2017).** *Database Reliability Engineering: Designing and Operating Resilient Database Systems*. O'Reilly Media. ISBN: 978-1491925942.  
→ Spezifische DRE-Praktiken für Datenbank-Operationen, Backup-Strategien und Disaster Recovery.

### Technische Dokumentation und Standards

**[5] Kubernetes Documentation.** *Kubernetes Official Documentation*. The Kubernetes Authors. https://kubernetes.io/docs/  
→ Offizielle Kubernetes-Dokumentation zu StatefulSets, Services, Storage Classes und Operators.

**[6] HashiCorp Terraform Documentation.** *Terraform Documentation*. HashiCorp. https://www.terraform.io/docs/  
→ Terraform Provider, Module, State Management und Best Practices.

**[7] NIST. (2010).** *NIST Special Publication 800-34 Rev. 1: Contingency Planning Guide for Federal Information Systems*. National Institute of Standards and Technology.  
→ Disaster Recovery Planning Standards, RTO/RPO-Definitionen und Business Continuity.

**[8] Weaveworks. (2021).** *GitOps Principles*. GitOps Working Group. https://opengitops.dev/  
→ GitOps-Workflow-Definitionen, Continuous Deployment und Declarative Configuration Management.

### Konferenz-Beiträge und Whitepapers

**[9] Burns, B., Grant, B., Oppenheimer, D., Brewer, E., & Wilkes, J. (2016).** *Borg, Omega, and Kubernetes*. ACM Queue, 14(1), 70-93.  
→ Akademische Perspektive auf Container-Orchestrierung und Cluster-Management.

**[10] Prometheus Authors. (2023).** *Prometheus Documentation: Best Practices*. https://prometheus.io/docs/practices/  
→ Monitoring Best Practices, Metric Naming, Alerting Guidelines.

---

## 30.20 Phase-3-Sync: Weiterführende Referenzen (docs/de/) {#chapter30_20_cross-references}

> Detaillierte Implementierungsdokumentation zu den behandelten Deployment- und Operations-Themen:

| Thema | Referenz (docs/de/) |
|---|---|
| Docker Deployment Guide | [`docs/de/deployment/DOCKER_DEPLOYMENT.md`](../../de/deployment/DOCKER_DEPLOYMENT.md) |
| Docker GPU Support | [`docs/de/deployment/DOCKER_GPU_SUPPORT.md`](../../de/deployment/DOCKER_GPU_SUPPORT.md) |
| Docker Security Checks | [`docs/de/deployment/DOCKER_SECURITY_CHECK_ZUSAMMENFASSUNG.md`](../../de/deployment/DOCKER_SECURITY_CHECK_ZUSAMMENFASSUNG.md) |
| Editions-Deployment-Strategie | [`docs/de/deployment/EDITION_DEPLOYMENT_STRATEGY.md`](../../de/deployment/EDITION_DEPLOYMENT_STRATEGY.md) |
| Editions-Limits-Matrix | [`docs/de/deployment/EDITION_LIMITS_MATRIX.md`](../../de/deployment/EDITION_LIMITS_MATRIX.md) |
| Editions-Control-Mechanismen | [`docs/de/deployment/EDITION_CONTROL_MECHANISMS.md`](../../de/deployment/EDITION_CONTROL_MECHANISMS.md) |
| Executive Summary Deployment | [`docs/de/deployment/EXECUTIVE_SUMMARY.md`](../../de/deployment/EXECUTIVE_SUMMARY.md) |

**→ Zurück:** [Kapitel 29: Analytics](chapter_29_analytics.md)  
**→ Weiter:** [Kapitel 31: API Protokolle](chapter_31_api_protocols.md)

---

**Kapitel 30 von 41** | **Teil X: Operations & Deployment** | **~10.100 Wörter**

---

**Navigation:**  
← [Kapitel 29: Analytics & Process Mining](chapter_29_analytics_process_mining.md) | [Inhaltsverzeichnis](index.md) | [Kapitel 31: API & Protocols](chapter_31_api_protocols.md) →
