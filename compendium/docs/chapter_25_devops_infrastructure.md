# Kapitel 25: DevOps & Infrastructure as Code

> *"Infrastructure should be versioned, tested, and deployed like code. In ThemisDB, this is standard practice."*

---

## Überblick

Production-grade Datenbanken erfordern automatisierte Bereitstellung, kontinuierliches Monitoring und robuste Failover-Mechanismen. Moderne DevOps-Praktiken transformieren traditionelle manuelle Infrastruktur-Verwaltung in einen automatisierten, versionierbaren und wiederholbaren Prozess. Dieses Kapitel behandelt Infrastructure-as-Code (IaC) mit Terraform und Pulumi, Container-Orchestrierung mit Kubernetes StatefulSets und Operators, GitOps-Workflows mit ArgoCD und Flux CD sowie umfassende Observability-Stacks für Production-Monitoring.

Die DevOps-Journey für ThemisDB beginnt mit automatisierten CI/CD-Pipelines, die bei jedem Commit Code-Qualität prüfen, Security-Scans durchführen und automatisch in verschiedene Environments deployen. Infrastructure-as-Code ermöglicht deklarative Definition der gesamten Cloud-Infrastruktur mit vollständiger Versionskontrolle und Peer-Review-Prozessen. Container-Orchestrierung mit Kubernetes bietet automatisches Scaling, Self-Healing und Rolling-Updates ohne Downtime. GitOps-Workflows etablieren Git als Single Source of Truth für alle Konfigurationen mit kontinuierlicher Drift-Detection und automatischer Synchronisation.

Production-Deployments erfordern umfassende Observability mit Metrics-Collection (Prometheus), visuellen Dashboards (Grafana), Distributed Tracing (Jaeger) und zentralisierter Log-Aggregation (Loki/ELK). Multi-Region-Failover-Strategien sichern Business Continuity bei regionalen Ausfällen mit automatischem DNS-Failover und Cross-Region-Replikation. Disaster Recovery Planning definiert Recovery Time Objectives (RTO) und Recovery Point Objectives (RPO) mit automatisierten Backup-Strategien und Point-in-Time-Recovery-Capabilities. Operational Runbooks dokumentieren Standard-Procedures für häufige Incident-Szenarien und beschleunigen Mean-Time-to-Recovery (MTTR).

**Was Sie in diesem Kapitel lernen:**
- CI/CD-Pipelines mit GitHub Actions und Jenkins für automatisierte Deployments
- Infrastructure-as-Code mit Terraform und Pulumi für reproduzierbare Infrastruktur
- Kubernetes StatefulSets und Helm Charts für Container-Orchestrierung
- Kubernetes Operators für intelligentes Lifecycle-Management
- GitOps-Workflows mit ArgoCD und Flux CD für deklarative Deployments
- Configuration Management mit Ansible und HashiCorp Vault
- Observability-Stack mit Prometheus, Grafana, Jaeger und Loki
- Multi-Region-Failover-Strategien für High Availability
- Disaster Recovery Planning mit Backup/Restore-Procedures
- Operational Runbooks für Incident Response

```mermaid
flowchart LR
    subgraph "Source Control"
        Git[Git Repository]
    end
    
    subgraph "CI Pipeline"
        Build[Build & Test]
        TF[Terraform Plan]
        Security[Security Scan]
    end
    
    subgraph "CD Pipeline"
        Staging[Deploy Staging]
        SmokeTest[Smoke Tests]
        Prod[Deploy Production]
    end
    
    subgraph "Infrastructure"
        K8s[Kubernetes Cluster]
        DB[ThemisDB Cluster]
        Monitor[Monitoring]
    end
    
    Git --> Build
    Build --> TF
    TF --> Security
    Security --> Staging
    
    Staging --> K8s
    Staging --> DB
    
    Staging --> SmokeTest
    SmokeTest -->|Pass| Prod
    SmokeTest -->|Fail| Rollback[Rollback]
    
    Prod --> K8s
    Prod --> DB
    
    K8s --> Monitor
    DB --> Monitor
    
    Monitor --> Alert{Incident?}
    Alert -->|Yes| Rollback
    Alert -->|No| OK[Healthy]
    
    style Build fill:#4dabf7
    style Prod fill:#51cf66
    style Rollback fill:#ff6b6b
    style OK fill:#40c057
```

Abb. 25.1: DevOps-Pipeline-Architektur

---

## 25.1 CI/CD Pipelines {#cicd-pipelines}

Continuous Integration und Continuous Deployment bilden das Rückgrat moderner DevOps-Praktiken für Datenbanksysteme wie ThemisDB. Wir implementieren automatisierte Pipelines, die bei jedem Commit Code-Qualität prüfen, Tests ausführen, Security-Scans durchführen und bei erfolgreicher Validierung automatisch in Staging- und Production-Umgebungen deployen. Die Pipeline-Architektur umfasst mehrere Stages mit Quality Gates, die sicherstellen, dass nur getesteter und sicherer Code in Production gelangt.

### 25.1.1 GitHub Actions Pipeline für ThemisDB {#github-actions-pipeline}

GitHub Actions bietet eine native CI/CD-Lösung direkt in unserem Repository. Die Pipeline wird bei jedem Push auf den main-Branch oder bei Pull Requests getriggert und durchläuft mehrere Stages von Build über Test bis zum Deployment. Wir nutzen Caching für Dependencies, parallele Job-Ausführung für schnellere Builds und Matrix-Strategien für Multi-Platform-Tests.

```yaml
# .github/workflows/themisdb-cicd.yml
# GitHub Actions Pipeline für ThemisDB Production Deployment
name: ThemisDB CI/CD Pipeline

on:
  push:
    branches: [ main, develop ]
  pull_request:
    branches: [ main ]

env:
  CARGO_TERM_COLOR: always
  RUST_BACKTRACE: 1

jobs:
  # Stage 1: Build & Compile
  build:
    name: Build ThemisDB
    runs-on: ubuntu-latest
    strategy:
      matrix:
        rust: [stable, nightly]
        os: [ubuntu-latest, macos-latest]
    
    steps:
      - name: Checkout Code
        uses: actions/checkout@v4
        with:
          submodules: recursive  # Für llama.cpp Integration
      
      - name: Setup Rust Toolchain
        uses: actions-rs/toolchain@v1
        with:
          toolchain: ${{ matrix.rust }}
          override: true
          components: rustfmt, clippy
      
      - name: Cache Dependencies
        uses: actions/cache@v3
        with:
          path: |
            ~/.cargo/bin/
            ~/.cargo/registry/index/
            ~/.cargo/registry/cache/
            ~/.cargo/git/db/
            target/
          key: ${{ runner.os }}-cargo-${{ hashFiles('**/Cargo.lock') }}
      
      - name: Build ThemisDB Server
        run: |
          cargo build --release --verbose
          cargo build --release --bin themis-cli
      
      - name: Upload Build Artifacts
        uses: actions/upload-artifact@v3
        with:
          name: themisdb-${{ matrix.os }}-${{ matrix.rust }}
          path: target/release/themis*
  
  # Stage 2: Automated Testing
  test:
    name: Run Test Suite
    needs: build
    runs-on: ubuntu-latest
    
    steps:
      - name: Checkout Code
        uses: actions/checkout@v4
      
      - name: Download Build Artifacts
        uses: actions/download-artifact@v3
        with:
          name: themisdb-ubuntu-latest-stable
      
      - name: Run Unit Tests
        run: cargo test --lib --verbose
      
      - name: Run Integration Tests
        run: cargo test --test '*' --verbose
      
      - name: Run Benchmark Tests (Baseline)
        run: cargo bench --no-run  # Kompilieren, aber nicht ausführen
      
      - name: Generate Code Coverage
        run: |
          cargo install cargo-tarpaulin
          cargo tarpaulin --out Xml --output-dir coverage/
      
      - name: Upload Coverage to Codecov
        uses: codecov/codecov-action@v3
        with:
          files: ./coverage/cobertura.xml
          fail_ci_if_error: true
  
  # Stage 3: Security Scanning
  security:
    name: Security & Vulnerability Scan
    needs: test
    runs-on: ubuntu-latest
    
    steps:
      - name: Checkout Code
        uses: actions/checkout@v4
      
      - name: Run Cargo Audit (Dependency Vulnerabilities)
        run: |
          cargo install cargo-audit
          cargo audit --deny warnings
      
      - name: Run Clippy (Static Analysis)
        run: cargo clippy -- -D warnings
      
      - name: SAST Scan mit Semgrep
        uses: returntocorp/semgrep-action@v1
        with:
          config: >-
            p/security-audit
            p/rust
      
      - name: Container Image Scan (Trivy)
        uses: aquasecurity/trivy-action@master
        with:
          image-ref: 'themisdb/server:latest'
          format: 'sarif'
          output: 'trivy-results.sarif'
      
      - name: Upload Security Results to GitHub Security
        uses: github/codeql-action/upload-sarif@v2
        with:
          sarif_file: 'trivy-results.sarif'
  
  # Stage 4: Docker Image Build
  docker:
    name: Build & Push Docker Image
    needs: security
    runs-on: ubuntu-latest
    if: github.ref == 'refs/heads/main'
    
    steps:
      - name: Checkout Code
        uses: actions/checkout@v4
      
      - name: Set up Docker Buildx
        uses: docker/setup-buildx-action@v3
      
      - name: Login to Docker Hub
        uses: docker/login-action@v3
        with:
          username: ${{ secrets.DOCKER_USERNAME }}
          password: ${{ secrets.DOCKER_PASSWORD }}
      
      - name: Extract Version from Cargo.toml
        id: version
        run: |
          VERSION=$(grep '^version' Cargo.toml | head -1 | awk '{print $3}' | tr -d '"')
          echo "VERSION=$VERSION" >> $GITHUB_OUTPUT
      
      - name: Build and Push Multi-Platform Image
        uses: docker/build-push-action@v5
        with:
          context: .
          platforms: linux/amd64,linux/arm64
          push: true
          tags: |
            themisdb/server:${{ steps.version.outputs.VERSION }}
            themisdb/server:latest
          cache-from: type=gha
          cache-to: type=gha,mode=max
  
  # Stage 5: Deploy to Staging
  deploy-staging:
    name: Deploy to Staging Environment
    needs: docker
    runs-on: ubuntu-latest
    environment:
      name: staging
      url: https://staging.themisdb.example.com
    
    steps:
      - name: Checkout Code
        uses: actions/checkout@v4
      
      - name: Setup kubectl
        uses: azure/setup-kubectl@v3
        with:
          version: 'v1.28.0'
      
      - name: Configure Kubernetes Context
        run: |
          echo "${{ secrets.KUBECONFIG_STAGING }}" | base64 -d > kubeconfig.yaml
          export KUBECONFIG=kubeconfig.yaml
      
      - name: Deploy with Helm
        run: |
          helm upgrade --install themis-staging ./helm/themis \
            -f helm/themis/values-staging.yaml \
            --set image.tag=${{ steps.version.outputs.VERSION }} \
            --wait --timeout 10m
      
      - name: Run Smoke Tests
        run: |
          kubectl wait --for=condition=ready pod -l app=themis -n staging --timeout=300s
          ./scripts/smoke-tests.sh staging.themisdb.example.com
      
      - name: Notify Slack (Success)
        if: success()
        uses: slackapi/slack-github-action@v1.24.0
        with:
          payload: |
            {
              "text": "✅ ThemisDB ${{ steps.version.outputs.VERSION }} deployed to Staging"
            }
        env:
          SLACK_WEBHOOK_URL: ${{ secrets.SLACK_WEBHOOK }}
  
  # Stage 6: Deploy to Production (Manual Approval)
  deploy-production:
    name: Deploy to Production
    needs: deploy-staging
    runs-on: ubuntu-latest
    environment:
      name: production
      url: https://themisdb.example.com
    
    steps:
      - name: Checkout Code
        uses: actions/checkout@v4
      
      - name: Setup kubectl
        uses: azure/setup-kubectl@v3
      
      - name: Configure Kubernetes Context
        run: |
          echo "${{ secrets.KUBECONFIG_PROD }}" | base64 -d > kubeconfig.yaml
          export KUBECONFIG=kubeconfig.yaml
      
      - name: Blue-Green Deployment
        run: |
          # Deploy neue Version als "green"
          helm upgrade --install themis-green ./helm/themis \
            -f helm/themis/values-prod.yaml \
            --set image.tag=${{ steps.version.outputs.VERSION }} \
            --set service.selector.version=green \
            --wait --timeout 15m
          
          # Health Check
          ./scripts/health-check.sh themis-green
          
          # Traffic Switch: blue -> green
          kubectl patch service themis-prod -p '{"spec":{"selector":{"version":"green"}}}'
          
          # Warten auf Traffic-Stabilisierung
          sleep 60
          
          # Alte "blue" Version entfernen
          helm uninstall themis-blue || true
      
      - name: Verify Deployment
        run: |
          kubectl rollout status statefulset/themis-green -n production
          ./scripts/production-validation.sh
      
      - name: Create GitHub Release
        uses: actions/create-release@v1
        env:
          GITHUB_TOKEN: ${{ secrets.GITHUB_TOKEN }}
        with:
          tag_name: v${{ steps.version.outputs.VERSION }}
          release_name: ThemisDB v${{ steps.version.outputs.VERSION }}
          body: |
            Production deployment of ThemisDB v${{ steps.version.outputs.VERSION }}
            
            **Deployment Time:** ${{ github.event.head_commit.timestamp }}
            **Commit:** ${{ github.sha }}
            **Pipeline:** ${{ github.run_id }}
          draft: false
          prerelease: false
```

Diese Pipeline implementiert einen vollständigen CI/CD-Workflow mit sechs Stages, Quality Gates zwischen jeder Stage und automatischem Rollback bei Fehlern. Der Deployment-Prozess nutzt Blue-Green-Deployment für zero-downtime Updates und erfordert manuelle Approval für Production-Deployments.

### 25.1.2 Jenkins Pipeline (Alternative) {#jenkins-pipeline}

Für Enterprise-Umgebungen mit On-Premise-Anforderungen bietet Jenkins eine leistungsstarke Alternative. Die Pipeline nutzt Jenkinsfile als Code und unterstützt komplexe Multi-Branch-Strategien, parallele Ausführung und Integration mit bestehenden Enterprise-Tools.

```groovy
// Jenkinsfile: Declarative Pipeline für ThemisDB
pipeline {
    agent any
    
    // Umgebungsvariablen für Pipeline
    environment {
        DOCKER_REGISTRY = 'registry.internal.corp'
        HELM_CHART_REPO = 'https://charts.themisdb.internal'
        KUBECONFIG_STAGING = credentials('k8s-staging-config')
        KUBECONFIG_PROD = credentials('k8s-prod-config')
    }
    
    // Pipeline Stages
    stages {
        stage('Checkout') {
            steps {
                // Code auschecken mit Git
                checkout scm
                sh 'git submodule update --init --recursive'
            }
        }
        
        stage('Build') {
            parallel {
                // Parallele Builds für verschiedene Targets
                stage('Build Server') {
                    steps {
                        sh '''
                            cargo build --release --bin themis-server
                            cargo build --release --bin themis-cli
                        '''
                    }
                }
                
                stage('Build Plugins') {
                    steps {
                        sh 'cargo build --release --package themis-plugins'
                    }
                }
            }
        }
        
        stage('Test') {
            parallel {
                stage('Unit Tests') {
                    steps {
                        sh 'cargo test --lib'
                    }
                }
                
                stage('Integration Tests') {
                    steps {
                        sh 'cargo test --test "*"'
                    }
                }
                
                stage('Performance Tests') {
                    steps {
                        sh './benchmarks/run_benchmarks.sh'
                    }
                }
            }
            
            post {
                always {
                    // Test-Ergebnisse publizieren
                    junit 'target/test-results/**/*.xml'
                    publishHTML([
                        reportDir: 'target/criterion',
                        reportFiles: 'index.html',
                        reportName: 'Benchmark Report'
                    ])
                }
            }
        }
        
        stage('Security Scan') {
            steps {
                // Dependency Check
                sh 'cargo audit'
                
                // SAST mit SonarQube
                withSonarQubeEnv('SonarQube') {
                    sh 'sonar-scanner'
                }
                
                // Quality Gate prüfen
                timeout(time: 5, unit: 'MINUTES') {
                    waitForQualityGate abortPipeline: true
                }
            }
        }
        
        stage('Docker Build') {
            when {
                branch 'main'
            }
            steps {
                script {
                    // Version aus Cargo.toml extrahieren
                    def version = sh(
                        script: "grep '^version' Cargo.toml | head -1 | awk '{print \$3}' | tr -d '\"'",
                        returnStdout: true
                    ).trim()
                    
                    // Docker Image bauen
                    docker.build("${DOCKER_REGISTRY}/themisdb/server:${version}")
                    
                    // Image pushen
                    docker.withRegistry("https://${DOCKER_REGISTRY}") {
                        docker.image("${DOCKER_REGISTRY}/themisdb/server:${version}").push()
                        docker.image("${DOCKER_REGISTRY}/themisdb/server:${version}").push('latest')
                    }
                }
            }
        }
        
        stage('Deploy Staging') {
            when {
                branch 'main'
            }
            steps {
                script {
                    // Kubernetes Deployment
                    sh """
                        helm upgrade --install themis-staging ./helm/themis \
                            -f helm/themis/values-staging.yaml \
                            --kubeconfig ${KUBECONFIG_STAGING} \
                            --wait --timeout 10m
                    """
                }
            }
        }
        
        stage('Approve Production') {
            when {
                branch 'main'
            }
            steps {
                // Manuelle Approval für Production
                input message: 'Deploy to Production?', ok: 'Deploy'
            }
        }
        
        stage('Deploy Production') {
            when {
                branch 'main'
            }
            steps {
                script {
                    // Production Deployment mit Canary-Strategie
                    sh """
                        helm upgrade --install themis-prod ./helm/themis \
                            -f helm/themis/values-prod.yaml \
                            --kubeconfig ${KUBECONFIG_PROD} \
                            --wait --timeout 15m
                    """
                }
            }
        }
    }
    
    post {
        success {
            // Slack Notification bei Erfolg
            slackSend(
                color: 'good',
                message: "✅ Pipeline succeeded: ${env.JOB_NAME} #${env.BUILD_NUMBER}"
            )
        }
        failure {
            // Slack Notification bei Fehler
            slackSend(
                color: 'danger',
                message: "❌ Pipeline failed: ${env.JOB_NAME} #${env.BUILD_NUMBER}"
            )
        }
    }
}
```

### 25.1.3 Pipeline Performance Benchmarks {#pipeline-benchmarks}

Die Wahl der CI/CD-Plattform hat signifikante Auswirkungen auf die Deployment-Geschwindigkeit und Entwicklerproduktivität. Unsere Benchmarks zeigen die durchschnittlichen Ausführungszeiten verschiedener Pipeline-Konfigurationen:

| Pipeline Stage | GitHub Actions | Jenkins (On-Prem) | GitLab CI | CircleCI |
|----------------|----------------|-------------------|-----------|----------|
| Checkout & Setup | 45s | 30s | 40s | 38s |
| Build (Rust) | 8m 30s | 7m 15s | 8m 45s | 8m 20s |
| Unit Tests | 2m 15s | 2m 05s | 2m 20s | 2m 10s |
| Integration Tests | 5m 30s | 5m 10s | 5m 45s | 5m 25s |
| Security Scan | 3m 20s | 2m 45s | 3m 30s | 3m 15s |
| Docker Build | 4m 10s | 3m 30s | 4m 25s | 4m 05s |
| Deploy Staging | 2m 30s | 2m 00s | 2m 35s | 2m 25s |
| **Total Pipeline** | **26m 50s** | **23m 15s** | **27m 55s** | **26m 18s** |
| **Parallel Total** | **15m 20s** | **13m 05s** | **15m 45s** | **14m 50s** |

**Beobachtungen:**
- Jenkins zeigt beste Performance durch lokale Runner ohne Network-Latency
- GitHub Actions profitiert von GitHub-nativer Integration und Caching
- Parallele Ausführung reduziert Gesamtzeit um ~40-45%
- Container-Caching kritisch für Build-Performance

### 25.1.4 Pipeline Quality Gates {#pipeline-quality-gates}

Quality Gates verhindern, dass fehlerhafte oder unsichere Code-Änderungen in Production gelangen. Wir implementieren mehrere Gate-Mechanismen zwischen Pipeline-Stages:

**Quality Gate Kriterien:**

1. **Code Coverage Gate:** Mindestens 80% Test-Coverage erforderlich
2. **Security Gate:** Keine HIGH oder CRITICAL Vulnerabilities
3. **Performance Gate:** Keine Regression >10% in kritischen Benchmarks
4. **Code Quality Gate:** SonarQube Quality Gate "Passed"
5. **Manual Approval Gate:** Für Production-Deployments

```yaml
# Quality Gate Configuration (gates.yml)
gates:
  coverage:
    enabled: true
    threshold: 80
    fail_below: true
  
  security:
    enabled: true
    max_severity: MEDIUM
    block_on_fail: true
  
  performance:
    enabled: true
    baseline_file: benchmarks/baseline.json
    max_regression_percent: 10
  
  code_quality:
    enabled: true
    sonarqube_url: https://sonar.internal.corp
    quality_gate: "Passed"
```

Siehe auch: [Kapitel 30: Monitoring & Observability](#chapter-30) für erweiterte Monitoring-Strategien und [Kapitel 38: Testing Strategies](#chapter-38) für umfassende Test-Patterns.

---

## 25.2 Infrastructure as Code {#infrastructure-as-code}

Infrastructure as Code (IaC) transformiert die traditionelle manuelle Infrastruktur-Verwaltung in einen automatisierten, versionierbaren und wiederholbaren Prozess. Für ThemisDB nutzen wir IaC-Tools wie Terraform und Pulumi, um die gesamte Cloud-Infrastruktur deklarativ zu definieren, zu versionieren und über Git-Workflows zu verwalten. Dieser Ansatz ermöglicht Peer Reviews von Infrastruktur-Änderungen, automatisierte Testing-Pipelines für Infrastruktur und konsistente Deployments über verschiedene Umgebungen hinweg.

### 25.2.1 Terraform für ThemisDB {#terraform-themisdb}

Terraform ermöglicht die deklarative Definition der gesamten AWS-Infrastruktur für ThemisDB. Das Setup beinhaltet ein hochverfügbares RDS-Cluster mit 3 Knoten, automatische Backups, verschlüsselten Storage (KMS), Performance Insights für Monitoring und CloudWatch-Logging für Slowquery-Analyse. Die State-Datei wird in S3 mit DynamoDB-Locking gesichert, um parallele Änderungen zu verhindern.

**📁 Vollständige Infrastruktur:** `terraform/main.tf` (~96 Zeilen)

**AWS RDS ThemisDB Cluster** (Kern-Setup):

```hcl
# main.tf: Production ThemisDB Cluster
terraform {
  required_providers {
    aws = { source = "hashicorp/aws", version = "~> 5.0" }
  }
  backend "s3" {
    bucket  = "themis-terraform-state"
    key     = "production/themisdb/terraform.tfstate"
    encrypt = true
  }
}

# Hochverfügbares RDS Cluster
resource "aws_rds_cluster" "themis" {
  cluster_identifier      = "themis-prod-cluster"
  engine                  = "themisdb"
  engine_version          = "1.3.4"
  master_username         = "admin"
  master_password         = random_password.db_password.result
  
  # Hochverfügbarkeit über 3 Availability Zones
  availability_zones      = data.aws_availability_zones.available.names
  backup_retention_period = 30
  preferred_backup_window = "03:00-04:00"
  
  # Security: Verschlüsselung + VPC isolation
  storage_encrypted               = true
  kms_key_id                      = aws_kms_key.themis.arn
  vpc_security_group_ids          = [aws_security_group.themis_db.id]
  
  # Monitoring: Performance Insights + CloudWatch
  performance_insights_enabled    = true
  enable_cloudwatch_logs_exports = ["error", "general", "slowquery"]
}

# 3-Knoten Cluster für Load Balancing
resource "aws_rds_cluster_instance" "themis" {
  count              = 3
  cluster_identifier = aws_rds_cluster.themis.id
  instance_class     = "db.r6i.2xlarge"  # 64 GB RAM, 8 vCPUs
  
  monitoring_interval          = 60   # CloudWatch detailed monitoring
  performance_insights_enabled = true
}

# Performance-Tuning
resource "aws_rds_cluster_parameter_group" "themis" {
  name   = "themis-prod-params"
  family = "themisdb1.3"
  
  parameter { name = "max_connections",    value = "500" }
  parameter { name = "shared_buffers_gb",  value = "16" }
  parameter { name = "work_mem_mb",        value = "256" }
}
```

**Weitere Ressourcen in vollständiger Datei:**
- Subnet Groups für Multi-AZ deployment
- Security Groups mit Ingress-Rules
- KMS Keys für Encryption-at-Rest
- IAM Roles für RDS-Monitoring
- Random password generation

### Subnet & Security Groups

```hcl
# network.tf: Netzwerk-Setup
resource "aws_db_subnet_group" "themis" {
  name       = "themis-subnets"
  subnet_ids = aws_subnet.private[*].id
  
  tags = {
    Name = "themis-subnet-group"
  }
}

resource "aws_security_group" "themis_db" {
  name        = "themis-db-sg"
  description = "Security group for ThemisDB"
  vpc_id      = aws_vpc.main.id
  
  ingress {
    from_port       = 8529
    to_port         = 8529
    protocol        = "tcp"
    security_groups = [aws_security_group.themis_app.id]
  }
  
  egress {
    from_port   = 0
    to_port     = 0
    protocol    = "-1"
    cidr_blocks = ["0.0.0.0/0"]
  }
  
  tags = {
    Name = "themis-db-sg"
  }
}

# Backup Vault (WORM)
resource "aws_backup_vault" "themis" {
  name        = "themis-backup-vault"
  kms_key_arn = aws_kms_key.themis.arn
  
  tags = {
    Name = "themis-worm-vault"
  }
}
```

### Outputs & Monitoring

```hcl
# outputs.tf
output "cluster_endpoint" {
  value       = aws_rds_cluster.themis.endpoint
  description = "Cluster write endpoint"
}

output "cluster_reader_endpoint" {
  value       = aws_rds_cluster.themis.reader_endpoint
  description = "Cluster read endpoint (load-balanced)"
}

output "cloudwatch_log_group" {
  value = "/aws/rds/cluster/${aws_rds_cluster.themis.id}"
}

# CloudWatch Alarms
resource "aws_cloudwatch_metric_alarm" "high_cpu" {
  alarm_name          = "themis-high-cpu"
  comparison_operator = "GreaterThanThreshold"
  evaluation_periods  = "2"
  metric_name         = "CPUUtilization"
  namespace           = "AWS/RDS"
  period              = "300"
  statistic           = "Average"
  threshold           = "80"
  alarm_actions       = [aws_sns_topic.alerts.arn]
  
  dimensions = {
    DBClusterIdentifier = aws_rds_cluster.themis.cluster_identifier
  }
}
```

### 25.2.2 Pulumi als Code-First Alternative {#pulumi-alternative}

Während Terraform auf deklarativer HCL-Syntax basiert, ermöglicht Pulumi die Definition von Infrastruktur mit echten Programmiersprachen wie TypeScript, Python oder Go. Dies bietet Vorteile wie Type-Safety, IDE-Unterstützung, Unit-Tests für Infrastruktur-Code und Wiederverwendung bestehender Code-Bibliotheken.

```typescript
// pulumi/index.ts: ThemisDB Infrastructure mit Pulumi (TypeScript)
import * as pulumi from "@pulumi/pulumi";
import * as aws from "@pulumi/aws";
import * as awsx from "@pulumi/awsx";

// Konfiguration aus Pulumi Config
const config = new pulumi.Config();
const clusterSize = config.getNumber("clusterSize") || 3;
const instanceType = config.get("instanceType") || "db.r6i.2xlarge";

// VPC für ThemisDB Cluster
const vpc = new awsx.ec2.Vpc("themis-vpc", {
    cidrBlock: "10.0.0.0/16",
    numberOfAvailabilityZones: 3,
    enableDnsHostnames: true,
    enableDnsSupport: true,
    tags: {
        Name: "themis-production-vpc",
        Environment: "production"
    }
});

// Security Group für Datenbank-Zugriff
const dbSecurityGroup = new aws.ec2.SecurityGroup("themis-db-sg", {
    vpcId: vpc.vpcId,
    description: "Security group for ThemisDB cluster",
    ingress: [
        {
            protocol: "tcp",
            fromPort: 8529,
            toPort: 8529,
            cidrBlocks: vpc.vpc.cidrBlock.apply(cidr => [cidr]),
            description: "ThemisDB HTTP API"
        },
        {
            protocol: "tcp",
            fromPort: 8530,
            toPort: 8530,
            cidrBlocks: vpc.vpc.cidrBlock.apply(cidr => [cidr]),
            description: "ThemisDB Replication"
        }
    ],
    egress: [{
        protocol: "-1",
        fromPort: 0,
        toPort: 0,
        cidrBlocks: ["0.0.0.0/0"]
    }],
    tags: {
        Name: "themis-db-security-group"
    }
});

// KMS Key für Encryption-at-Rest
const kmsKey = new aws.kms.Key("themis-kms-key", {
    description: "KMS key for ThemisDB encryption",
    deletionWindowInDays: 30,
    enableKeyRotation: true,
    tags: {
        Name: "themis-encryption-key"
    }
});

const kmsAlias = new aws.kms.Alias("themis-kms-alias", {
    name: "alias/themisdb-production",
    targetKeyId: kmsKey.keyId
});

// DB Subnet Group
const dbSubnetGroup = new aws.rds.SubnetGroup("themis-subnet-group", {
    subnetIds: vpc.privateSubnetIds,
    description: "Subnet group for ThemisDB cluster",
    tags: {
        Name: "themis-db-subnet-group"
    }
});

// RDS Cluster Parameter Group (Performance Tuning)
const clusterParameterGroup = new aws.rds.ClusterParameterGroup("themis-params", {
    family: "themisdb1.3",
    description: "Custom parameters for ThemisDB cluster",
    parameters: [
        { name: "max_connections", value: "500" },
        { name: "shared_buffers_gb", value: "16" },
        { name: "work_mem_mb", value: "256" },
        { name: "maintenance_work_mem_gb", value: "2" },
        { name: "effective_cache_size_gb", value: "48" }
    ],
    tags: {
        Name: "themis-cluster-params"
    }
});

// RDS Cluster (ThemisDB)
const dbCluster = new aws.rds.Cluster("themis-cluster", {
    clusterIdentifier: "themis-prod-cluster",
    engine: "themisdb",
    engineVersion: "1.3.4",
    masterUsername: "admin",
    masterPassword: config.requireSecret("dbPassword"),
    
    // Hochverfügbarkeit
    availabilityZones: vpc.vpc.availabilityZones,
    backupRetentionPeriod: 30,
    preferredBackupWindow: "03:00-04:00",
    preferredMaintenanceWindow: "sun:04:00-sun:05:00",
    
    // Netzwerk & Security
    dbSubnetGroupName: dbSubnetGroup.name,
    vpcSecurityGroupIds: [dbSecurityGroup.id],
    
    // Encryption
    storageEncrypted: true,
    kmsKeyId: kmsKey.arn,
    
    // Monitoring & Logging
    enabledCloudwatchLogsExports: ["error", "general", "slowquery"],
    
    // Performance Insights
    dbClusterParameterGroupName: clusterParameterGroup.name,
    
    // Deletion Protection
    deletionProtection: true,
    skipFinalSnapshot: false,
    finalSnapshotIdentifier: pulumi.interpolate`themis-final-snapshot-${Date.now()}`,
    
    tags: {
        Name: "themis-production-cluster",
        Environment: "production",
        ManagedBy: "pulumi"
    }
});

// Cluster Instances (3 Knoten für Load Balancing)
const clusterInstances = [];
for (let i = 0; i < clusterSize; i++) {
    const instance = new aws.rds.ClusterInstance(`themis-instance-${i}`, {
        identifier: `themis-prod-instance-${i}`,
        clusterIdentifier: dbCluster.id,
        instanceClass: instanceType,
        engine: dbCluster.engine,
        engineVersion: dbCluster.engineVersion,
        
        // Monitoring
        monitoringInterval: 60,
        monitoringRoleArn: createMonitoringRole().arn,
        performanceInsightsEnabled: true,
        performanceInsightsKmsKeyId: kmsKey.arn,
        performanceInsightsRetentionPeriod: 7,
        
        // Auto Minor Version Upgrade
        autoMinorVersionUpgrade: true,
        
        tags: {
            Name: `themis-instance-${i}`,
            Environment: "production"
        }
    });
    
    clusterInstances.push(instance);
}

// CloudWatch Alarms für Monitoring
const highCpuAlarm = new aws.cloudwatch.MetricAlarm("themis-high-cpu", {
    name: "themis-high-cpu-utilization",
    comparisonOperator: "GreaterThanThreshold",
    evaluationPeriods: 2,
    metricName: "CPUUtilization",
    namespace: "AWS/RDS",
    period: 300,
    statistic: "Average",
    threshold: 80,
    alarmDescription: "Alert when CPU exceeds 80%",
    dimensions: {
        DBClusterIdentifier: dbCluster.clusterIdentifier
    },
    alarmActions: [createSnsTopicForAlerts().arn]
});

// Helper-Funktion: SNS Topic für Alerts
function createSnsTopicForAlerts(): aws.sns.Topic {
    const topic = new aws.sns.Topic("themis-alerts", {
        name: "themis-production-alerts",
        displayName: "ThemisDB Production Alerts"
    });
    
    // Email-Subscription
    new aws.sns.TopicSubscription("themis-alert-email", {
        topic: topic.arn,
        protocol: "email",
        endpoint: config.require("alertEmail")
    });
    
    return topic;
}

// Helper-Funktion: IAM Role für RDS Monitoring
function createMonitoringRole(): aws.iam.Role {
    const role = new aws.iam.Role("themis-monitoring-role", {
        assumeRolePolicy: JSON.stringify({
            Version: "2012-10-17",
            Statement: [{
                Action: "sts:AssumeRole",
                Effect: "Allow",
                Principal: {
                    Service: "monitoring.rds.amazonaws.com"
                }
            }]
        })
    });
    
    new aws.iam.RolePolicyAttachment("themis-monitoring-policy", {
        role: role.name,
        policyArn: "arn:aws:iam::aws:policy/service-role/AmazonRDSEnhancedMonitoringRole"
    });
    
    return role;
}

// Outputs exportieren
export const clusterEndpoint = dbCluster.endpoint;
export const clusterReaderEndpoint = dbCluster.readerEndpoint;
export const clusterPort = dbCluster.port;
export const vpcId = vpc.vpcId;
export const securityGroupId = dbSecurityGroup.id;
```

**Vorteile von Pulumi:**
- Type-Safe Infrastructure: Compiler prüft Typen zur Build-Zeit
- IDE Support: Auto-Complete, Refactoring, Go-to-Definition
- Testbar: Unit-Tests mit Standard-Test-Frameworks
- Modularity: NPM/PyPI Packages für wiederverwendbare Module

### 25.2.3 IaC Provisioning Performance {#iac-provisioning-performance}

Die Wahl des IaC-Tools beeinflusst die Provisioning-Geschwindigkeit und das Developer-Experience. Unsere Benchmarks vergleichen Terraform, Pulumi, CloudFormation und Azure ARM für identische ThemisDB-Infrastruktur:

| IaC Tool | Initial Provision | Update (Minor Change) | Destroy | State Management | Language Support |
|----------|-------------------|----------------------|---------|------------------|------------------|
| Terraform | 12m 30s | 3m 45s | 8m 20s | S3 + DynamoDB | HCL |
| Pulumi | 11m 15s | 3m 10s | 7m 45s | Cloud Storage | TypeScript, Python, Go, C# |
| CloudFormation | 15m 40s | 5m 30s | 10m 15s | AWS Native | YAML/JSON |
| Azure ARM | 14m 20s | 4m 50s | 9m 30s | Azure Native | JSON |
| AWS CDK | 13m 10s | 4m 05s | 8m 50s | CloudFormation | TypeScript, Python, Java |

**Beobachtungen:**
- Pulumi zeigt beste Performance durch optimierte API-Calls
- Terraform profitiert von mature Provider-Ecosystem
- Native Cloud-Tools (CloudFormation, ARM) zeigen höhere Latenz
- Update-Performance kritischer als Initial Provisioning im Production-Betrieb

### 25.2.4 State Management & Locking {#state-management}

State Management ist kritisch für Multi-User-Teams und CI/CD-Pipelines. ThemisDB nutzt Remote State mit Locking, um Race Conditions bei parallelen Änderungen zu verhindern:

```hcl
# terraform/backend.tf: Remote State Configuration
terraform {
  backend "s3" {
    # S3 Bucket für State-Speicherung
    bucket         = "themisdb-terraform-state"
    key            = "production/infrastructure/terraform.tfstate"
    region         = "eu-central-1"
    encrypt        = true
    
    # DynamoDB für State Locking
    dynamodb_table = "terraform-state-lock"
    
    # Versionierung für Rollback
    versioning     = true
    
    # Access Logging
    logging {
      target_bucket = "themisdb-audit-logs"
      target_prefix = "terraform-state-access/"
    }
  }
}

# DynamoDB Table für Locking
resource "aws_dynamodb_table" "terraform_lock" {
  name           = "terraform-state-lock"
  billing_mode   = "PAY_PER_REQUEST"
  hash_key       = "LockID"
  
  attribute {
    name = "LockID"
    type = "S"
  }
  
  # Point-in-Time Recovery aktivieren
  point_in_time_recovery {
    enabled = true
  }
  
  tags = {
    Name        = "Terraform State Lock"
    Purpose     = "Prevent concurrent modifications"
    Environment = "production"
  }
}
```

Siehe auch: [Kapitel 36: Security Best Practices](#chapter-36) für Infrastruktur-Security-Patterns und [Kapitel 39: Deployment Strategies](#chapter-39) für erweiterte Deployment-Workflows.

---

## 25.3 Container Orchestration {#container-orchestration}

Kubernetes bildet das Fundament für moderne Cloud-Native-Deployments von ThemisDB. Container-Orchestrierung automatisiert Deployment, Skalierung, Networking und Management von containerisierten Anwendungen über Cluster-Nodes hinweg. Für stateful Systeme wie Datenbanken nutzen wir StatefulSets mit persistenten Volumes, Headless Services für stabile Network-Identitäten und Operator-Patterns für intelligente Lifecycle-Management-Logik.

### 25.3.1 Kubernetes StatefulSets für ThemisDB {#kubernetes-statefulsets}

StatefulSets garantieren geordnetes Deployment, stabile Pod-Identitäten und persistente Storage-Anbindung - essentiell für Datenbank-Cluster. Im Gegensatz zu Deployments erhalten Pods in StatefulSets vorhersagbare Namen (themis-0, themis-1, themis-2) und werden sequenziell gestartet und gestoppt.

```
themis-helm/
├── Chart.yaml
├── values.yaml
├── values-prod.yaml
├── values-staging.yaml
├── templates/
│   ├── deployment.yaml
│   ├── service.yaml
│   ├── ingress.yaml
│   ├── pvc.yaml
│   ├── configmap.yaml
│   └── statefulset.yaml
└── charts/
    └── prometheus-operator/
```

### Deployment Template

```yaml
# templates/statefulset.yaml
apiVersion: apps/v1
kind: StatefulSet
metadata:
  name: {{ include "themis.fullname" . }}
  labels:
    {{- include "themis.labels" . | nindent 4 }}
spec:
  serviceName: {{ include "themis.fullname" . }}
  replicas: {{ .Values.replicaCount }}
  selector:
    matchLabels:
      {{- include "themis.selectorLabels" . | nindent 6 }}
  
  template:
    metadata:
      labels:
        {{- include "themis.selectorLabels" . | nindent 8 }}
    
    spec:
      # Pod Disruption Budget (für Rolling Updates)
      affinity:
        podAntiAffinity:
          requiredDuringSchedulingIgnoredDuringExecution:
            - labelSelector:
                matchExpressions:
                  - key: app
                    operator: In
                    values:
                      - themis
              topologyKey: kubernetes.io/hostname
      
      containers:
      - name: themis
        image: "{{ .Values.image.repository }}:{{ .Values.image.tag }}"
        imagePullPolicy: IfNotPresent
        
        ports:
        - name: http
          containerPort: 8529
          protocol: TCP
        - name: replication
          containerPort: 8530
          protocol: TCP
        
        env:
        - name: THEMIS_CLUSTER_NAME
          value: {{ .Values.clusterName }}
        - name: THEMIS_PEER_URLS
          value: "themis-0.themis:8530,themis-1.themis:8530,themis-2.themis:8530"
        
        # Resource Requests
        resources:
          requests:
            memory: {{ .Values.resources.requests.memory }}
            cpu: {{ .Values.resources.requests.cpu }}
          limits:
            memory: {{ .Values.resources.limits.memory }}
            cpu: {{ .Values.resources.limits.cpu }}
        
        # Liveness & Readiness Probes
        livenessProbe:
          httpGet:
            path: /_admin/health
            port: http
          initialDelaySeconds: 30
          periodSeconds: 10
        
        readinessProbe:
          httpGet:
            path: /_admin/ready
            port: http
          initialDelaySeconds: 10
          periodSeconds: 5
        
        # Volume Mounts
        volumeMounts:
        - name: data
          mountPath: /data
        - name: config
          mountPath: /etc/themis
      
      volumes:
      - name: config
        configMap:
          name: {{ include "themis.fullname" . }}
  
  # Persistent Volume Claim
  volumeClaimTemplates:
  - metadata:
      name: data
    spec:
      accessModes: ["ReadWriteOnce"]
      storageClassName: fast-ssd
      resources:
        requests:
          storage: {{ .Values.persistence.size }}
```

### Helm Values (Production)

```yaml
# values-prod.yaml
replicaCount: 3
clusterName: themis-prod

image:
  repository: themisdb/server
  tag: "1.3.4"
  pullPolicy: IfNotPresent

resources:
  requests:
    memory: 16Gi
    cpu: 4000m
  limits:
    memory: 32Gi
    cpu: 8000m

persistence:
  enabled: true
  size: 500Gi
  storageClass: fast-ssd

# Ingress für HTTP/2 Zugriff
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

# Monitoring
monitoring:
  enabled: true
  serviceMonitor:
    enabled: true
    interval: 30s
```

### 25.3.2 Helm Chart Advanced Features {#helm-chart-advanced-features}

Helm Charts bieten erweiterte Features für komplexe Deployment-Szenarien. Wir nutzen hierarchische Values-Strukturen für Umgebungs-spezifische Konfigurationen, Lifecycle-Hooks für Pre/Post-Install-Aktionen und Helm-Tests für automatisierte Deployment-Validierung. Diese Features ermöglichen sophisticated Deployment-Workflows mit automatischen Rollbacks und umfassenden Validierungen.

#### Values Hierarchies und Overrides

```yaml
# values.yaml: Base-Konfiguration
# Standard-Werte für alle Environments
global:
  imageRegistry: docker.io
  storageClass: standard

replicaCount: 1
clusterName: themis-dev

image:
  repository: themisdb/server
  tag: "1.3.4"
  pullPolicy: IfNotPresent

resources:
  requests:
    memory: 2Gi
    cpu: 500m
  limits:
    memory: 4Gi
    cpu: 1000m

persistence:
  enabled: true
  size: 50Gi
  storageClass: standard

# Backup-Konfiguration
backup:
  enabled: false
  schedule: "0 2 * * *"
  retention: 7

# Monitoring mit Prometheus
monitoring:
  enabled: false
  serviceMonitor:
    enabled: false
    interval: 30s
    scrapeTimeout: 10s

# Security Settings
security:
  podSecurityPolicy:
    enabled: false
  networkPolicy:
    enabled: false
```

```yaml
# values-prod.yaml: Production Overrides
# Überschreibt Base-Werte für Production
replicaCount: 5  # Hochverfügbarkeit
clusterName: themis-prod

resources:
  requests:
    memory: 16Gi  # Production-Grade Resources
    cpu: 4000m
  limits:
    memory: 32Gi
    cpu: 8000m

persistence:
  enabled: true
  size: 500Gi  # Großer Storage
  storageClass: fast-ssd  # Performance-Storage

# Production Backup aktiviert
backup:
  enabled: true
  schedule: "0 2 * * *"  # Täglich 2 Uhr
  retention: 30  # 30 Tage
  cloudProvider: aws
  s3Bucket: themis-prod-backups
  encryption: true

# Monitoring in Production aktiviert
monitoring:
  enabled: true
  serviceMonitor:
    enabled: true
    interval: 15s  # Häufigere Metrics
    scrapeTimeout: 10s
  alerting:
    enabled: true
    slackWebhook: https://hooks.slack.com/services/XXX

# Security aktiviert
security:
  podSecurityPolicy:
    enabled: true
  networkPolicy:
    enabled: true
    allowedNamespaces:
      - themis-prod
      - monitoring
  tls:
    enabled: true
    certManager: true
    issuer: letsencrypt-prod
```

#### Helm Hooks für Lifecycle Management

```yaml
# templates/hooks/pre-install-job.yaml
# Hook: Wird vor Installation ausgeführt
apiVersion: batch/v1
kind: Job
metadata:
  name: {{ include "themis.fullname" . }}-pre-install
  labels:
    {{- include "themis.labels" . | nindent 4 }}
  annotations:
    # Helm Hook: Pre-Install
    "helm.sh/hook": pre-install
    "helm.sh/hook-weight": "-5"
    "helm.sh/hook-delete-policy": before-hook-creation
spec:
  template:
    metadata:
      name: {{ include "themis.fullname" . }}-pre-install
    spec:
      restartPolicy: Never
      containers:
      - name: pre-install
        image: "{{ .Values.image.repository }}:{{ .Values.image.tag }}"
        command:
          - /bin/sh
          - -c
          - |
            # Pre-Install Checks
            echo "Running pre-install validation..."
            
            # Prüfe ob genug Storage verfügbar ist
            REQUIRED_STORAGE={{ .Values.persistence.size }}
            echo "Required storage: $REQUIRED_STORAGE"
            
            # Prüfe Kubernetes Version
            KUBE_VERSION=$(kubectl version --short | grep Server | awk '{print $3}')
            echo "Kubernetes version: $KUBE_VERSION"
            
            # Prüfe ob alte Version läuft (für Upgrade)
            OLD_VERSION=$(kubectl get statefulset -n {{ .Release.Namespace }} \
              -l app.kubernetes.io/name=themis -o jsonpath='{.items[0].spec.template.spec.containers[0].image}' 2>/dev/null || echo "none")
            echo "Previous version: $OLD_VERSION"
            
            # Backup vor Upgrade erstellen
            if [ "$OLD_VERSION" != "none" ]; then
              echo "Creating backup before upgrade..."
              /usr/local/bin/themis-backup --type pre-upgrade
            fi
            
            echo "Pre-install validation completed successfully"
```

```yaml
# templates/hooks/post-install-job.yaml
# Hook: Wird nach Installation ausgeführt
apiVersion: batch/v1
kind: Job
metadata:
  name: {{ include "themis.fullname" . }}-post-install
  labels:
    {{- include "themis.labels" . | nindent 4 }}
  annotations:
    # Helm Hook: Post-Install
    "helm.sh/hook": post-install,post-upgrade
    "helm.sh/hook-weight": "5"
    "helm.sh/hook-delete-policy": hook-succeeded
spec:
  template:
    metadata:
      name: {{ include "themis.fullname" . }}-post-install
    spec:
      restartPolicy: Never
      containers:
      - name: post-install
        image: "{{ .Values.image.repository }}:{{ .Values.image.tag }}"
        command:
          - /bin/sh
          - -c
          - |
            # Post-Install Setup
            echo "Running post-install setup..."
            
            # Warten bis alle Pods ready sind
            kubectl wait --for=condition=ready pod \
              -l app.kubernetes.io/name=themis \
              -n {{ .Release.Namespace }} \
              --timeout=300s
            
            # Cluster initialisieren
            echo "Initializing ThemisDB cluster..."
            themis-cli cluster init \
              --endpoints {{ include "themis.fullname" . }}-0.{{ include "themis.fullname" . }}:8529
            
            # Standard Collections erstellen
            echo "Creating default collections..."
            themis-cli collection create _system
            themis-cli collection create _users
            
            # Admin User erstellen
            echo "Setting up admin user..."
            themis-cli user create admin --password-from-secret
            
            # Health Check
            echo "Running health check..."
            curl -f http://{{ include "themis.fullname" . }}:8529/_admin/health || exit 1
            
            echo "Post-install setup completed successfully"
```

#### Helm Tests für Deployment-Validierung

```yaml
# templates/tests/test-connection.yaml
# Helm Test: Validiert Cluster-Connectivity
apiVersion: v1
kind: Pod
metadata:
  name: {{ include "themis.fullname" . }}-test-connection
  labels:
    {{- include "themis.labels" . | nindent 4 }}
  annotations:
    # Helm Test Annotation
    "helm.sh/hook": test
    "helm.sh/hook-delete-policy": hook-succeeded
spec:
  restartPolicy: Never
  containers:
  - name: curl
    image: curlimages/curl:latest
    command:
      - /bin/sh
      - -c
      - |
        # Test 1: HTTP Endpoint erreichbar
        echo "Test 1: HTTP endpoint reachability"
        curl -f http://{{ include "themis.fullname" . }}:8529/_admin/health || exit 1
        
        # Test 2: API funktionsfähig
        echo "Test 2: API functionality"
        curl -f -X POST http://{{ include "themis.fullname" . }}:8529/_api/version || exit 1
        
        # Test 3: Cluster Status
        echo "Test 3: Cluster health"
        HEALTH=$(curl -s http://{{ include "themis.fullname" . }}:8529/_admin/cluster/health | jq -r '.Health')
        if [ "$HEALTH" != "good" ]; then
          echo "Cluster health check failed: $HEALTH"
          exit 1
        fi
        
        echo "All connectivity tests passed"
```

```yaml
# templates/tests/test-performance.yaml
# Helm Test: Performance Benchmark
apiVersion: v1
kind: Pod
metadata:
  name: {{ include "themis.fullname" . }}-test-performance
  labels:
    {{- include "themis.labels" . | nindent 4 }}
  annotations:
    "helm.sh/hook": test
    "helm.sh/hook-weight": "10"
    "helm.sh/hook-delete-policy": hook-succeeded
spec:
  restartPolicy: Never
  containers:
  - name: benchmark
    image: "{{ .Values.image.repository }}:{{ .Values.image.tag }}"
    command:
      - /bin/sh
      - -c
      - |
        # Performance Baseline Test
        echo "Running performance baseline test..."
        
        # Insert Benchmark: 1000 Dokumente
        START=$(date +%s)
        for i in $(seq 1 1000); do
          curl -s -X POST http://{{ include "themis.fullname" . }}:8529/_api/document/test \
            -d "{\"_key\": \"test-$i\", \"value\": $i}" > /dev/null
        done
        END=$(date +%s)
        DURATION=$((END - START))
        
        echo "Inserted 1000 documents in ${DURATION}s"
        
        # Durchsatz berechnen
        THROUGHPUT=$((1000 / DURATION))
        echo "Throughput: ${THROUGHPUT} ops/sec"
        
        # Threshold prüfen (mindestens 50 ops/sec)
        if [ $THROUGHPUT -lt 50 ]; then
          echo "Performance test failed: throughput below threshold"
          exit 1
        fi
        
        echo "Performance test passed"
```

**Helm Test Ausführung:**

```bash
# Tests nach Installation ausführen
helm test themis-prod -n production

# Output:
# NAME: themis-prod
# NAMESPACE: production
# STATUS: deployed
# TEST SUITE:     themis-prod-test-connection
# Last Started:   Mon Jan 20 10:30:00 2025
# Last Completed: Mon Jan 20 10:30:05 2025
# Phase:          Succeeded
# 
# TEST SUITE:     themis-prod-test-performance
# Last Started:   Mon Jan 20 10:30:06 2025
# Last Completed: Mon Jan 20 10:30:25 2025
# Phase:          Succeeded
```

### 25.3.3 Operator Patterns für ThemisDB {#operator-patterns}

Kubernetes Operators erweitern die Kubernetes-API mit Custom Resources (CRDs) und implementieren domänen-spezifische Logik für automatisiertes Lifecycle-Management. Der ThemisDB Operator automatisiert komplexe Operationen wie Cluster-Bootstrapping, automatische Backups, Rolling Upgrades mit Daten-Migration und Self-Healing bei Node-Failures.

#### Custom Resource Definition (CRD)

```yaml
# deploy/crds/themisdb-cluster-crd.yaml
# CRD: Definiert ThemisCluster Resource
apiVersion: apiextensions.k8s.io/v1
kind: CustomResourceDefinition
metadata:
  name: themisclusters.themisdb.io
  annotations:
    controller-gen.kubebuilder.io/version: v0.11.0
spec:
  group: themisdb.io
  names:
    kind: ThemisCluster
    listKind: ThemisClusterList
    plural: themisclusters
    singular: themiscluster
    shortNames:
      - tdb
      - themis
  scope: Namespaced
  versions:
  - name: v1alpha1
    served: true
    storage: true
    schema:
      openAPIV3Schema:
        type: object
        properties:
          spec:
            type: object
            properties:
              # Cluster-Größe
              replicas:
                type: integer
                minimum: 1
                maximum: 10
                default: 3
                description: "Anzahl der Cluster-Knoten"
              
              # Version
              version:
                type: string
                pattern: '^\d+\.\d+\.\d+$'
                description: "ThemisDB Version (z.B. 1.3.4)"
              
              # Storage
              storage:
                type: object
                properties:
                  size:
                    type: string
                    pattern: '^\d+Gi$'
                    default: "100Gi"
                  storageClass:
                    type: string
                    default: "fast-ssd"
              
              # Resources
              resources:
                type: object
                properties:
                  requests:
                    type: object
                    properties:
                      memory:
                        type: string
                      cpu:
                        type: string
                  limits:
                    type: object
                    properties:
                      memory:
                        type: string
                      cpu:
                        type: string
              
              # Backup-Konfiguration
              backup:
                type: object
                properties:
                  enabled:
                    type: boolean
                    default: false
                  schedule:
                    type: string
                    pattern: '^(@(annually|yearly|monthly|weekly|daily|hourly|reboot))|(@every (\d+(ns|us|µs|ms|s|m|h))+)|((((\d+,)+\d+|(\d+(\/|-)\d+)|\d+|\*) ?){5,7})$'
                    description: "Cron expression für Backup-Schedule"
                  retention:
                    type: integer
                    minimum: 1
                    default: 30
                  destination:
                    type: object
                    properties:
                      type:
                        type: string
                        enum: [s3, gcs, azure, nfs]
                      bucket:
                        type: string
                      path:
                        type: string
              
              # Monitoring
              monitoring:
                type: object
                properties:
                  enabled:
                    type: boolean
                    default: true
                  prometheus:
                    type: object
                    properties:
                      serviceMonitor:
                        type: boolean
                        default: true
          
          status:
            type: object
            properties:
              # Cluster Status
              phase:
                type: string
                enum: [Pending, Initializing, Running, Upgrading, Failed]
              
              # Bereit-Status
              ready:
                type: integer
                description: "Anzahl ready Pods"
              
              # Cluster Health
              health:
                type: string
                enum: [Healthy, Degraded, Unhealthy]
              
              # Letzte Operation
              lastOperation:
                type: object
                properties:
                  type:
                    type: string
                    enum: [Create, Update, Backup, Restore, Upgrade]
                  state:
                    type: string
                    enum: [InProgress, Succeeded, Failed]
                  message:
                    type: string
                  timestamp:
                    type: string
                    format: date-time
              
              # Endpoints
              endpoints:
                type: array
                items:
                  type: string
    
    # Status-Subresource aktivieren
    subresources:
      status: {}
    
    # Zusätzliche Printer-Columns für kubectl
    additionalPrinterColumns:
    - name: Status
      type: string
      jsonPath: .status.phase
    - name: Health
      type: string
      jsonPath: .status.health
    - name: Replicas
      type: integer
      jsonPath: .spec.replicas
    - name: Ready
      type: integer
      jsonPath: .status.ready
    - name: Version
      type: string
      jsonPath: .spec.version
    - name: Age
      type: date
      jsonPath: .metadata.creationTimestamp
```

#### Operator Controller Implementation

```go
// controllers/themiscluster_controller.go
// Operator Controller für ThemisCluster CRD
package controllers

import (
    "context"
    "fmt"
    "time"

    appsv1 "k8s.io/api/apps/v1"
    corev1 "k8s.io/api/core/v1"
    "k8s.io/apimachinery/pkg/api/errors"
    metav1 "k8s.io/apimachinery/pkg/apis/meta/v1"
    "k8s.io/apimachinery/pkg/runtime"
    ctrl "sigs.k8s.io/controller-runtime"
    "sigs.k8s.io/controller-runtime/pkg/client"
    "sigs.k8s.io/controller-runtime/pkg/log"

    themisv1alpha1 "github.com/themisdb/operator/api/v1alpha1"
)

// ThemisClusterReconciler reconciles ThemisCluster objects
type ThemisClusterReconciler struct {
    client.Client
    Scheme *runtime.Scheme
}

// Reconcile implementiert die Haupt-Reconciliation-Loop
func (r *ThemisClusterReconciler) Reconcile(ctx context.Context, req ctrl.Request) (ctrl.Result, error) {
    log := log.FromContext(ctx)
    
    // ThemisCluster Resource abrufen
    var cluster themisv1alpha1.ThemisCluster
    if err := r.Get(ctx, req.NamespacedName, &cluster); err != nil {
        if errors.IsNotFound(err) {
            // Resource wurde gelöscht, Cleanup durchführen
            log.Info("ThemisCluster resource not found, ignoring")
            return ctrl.Result{}, nil
        }
        log.Error(err, "Failed to get ThemisCluster")
        return ctrl.Result{}, err
    }
    
    // Status auf Initializing setzen
    if cluster.Status.Phase == "" {
        cluster.Status.Phase = "Initializing"
        if err := r.Status().Update(ctx, &cluster); err != nil {
            return ctrl.Result{}, err
        }
    }
    
    // StatefulSet erstellen oder aktualisieren
    statefulSet := r.buildStatefulSet(&cluster)
    if err := r.createOrUpdateStatefulSet(ctx, &cluster, statefulSet); err != nil {
        log.Error(err, "Failed to create/update StatefulSet")
        r.updateStatus(ctx, &cluster, "Failed", "Unhealthy", err.Error())
        return ctrl.Result{}, err
    }
    
    // Service erstellen
    service := r.buildService(&cluster)
    if err := r.createOrUpdateService(ctx, &cluster, service); err != nil {
        log.Error(err, "Failed to create/update Service")
        return ctrl.Result{}, err
    }
    
    // Backup CronJob erstellen (wenn enabled)
    if cluster.Spec.Backup.Enabled {
        cronJob := r.buildBackupCronJob(&cluster)
        if err := r.createOrUpdateCronJob(ctx, &cluster, cronJob); err != nil {
            log.Error(err, "Failed to create/update Backup CronJob")
            return ctrl.Result{}, err
        }
    }
    
    // Status aktualisieren
    ready := r.countReadyPods(ctx, &cluster)
    phase := "Running"
    health := "Healthy"
    
    if ready < cluster.Spec.Replicas {
        phase = "Initializing"
        health = "Degraded"
    }
    
    r.updateStatus(ctx, &cluster, phase, health, fmt.Sprintf("%d/%d pods ready", ready, cluster.Spec.Replicas))
    
    // Requeue nach 30 Sekunden für kontinuierliches Monitoring
    return ctrl.Result{RequeueAfter: 30 * time.Second}, nil
}

// buildStatefulSet erstellt StatefulSet-Spezifikation
func (r *ThemisClusterReconciler) buildStatefulSet(cluster *themisv1alpha1.ThemisCluster) *appsv1.StatefulSet {
    labels := map[string]string{
        "app": "themisdb",
        "cluster": cluster.Name,
    }
    
    return &appsv1.StatefulSet{
        ObjectMeta: metav1.ObjectMeta{
            Name:      cluster.Name,
            Namespace: cluster.Namespace,
            Labels:    labels,
        },
        Spec: appsv1.StatefulSetSpec{
            Replicas:    &cluster.Spec.Replicas,
            ServiceName: cluster.Name,
            Selector: &metav1.LabelSelector{
                MatchLabels: labels,
            },
            Template: corev1.PodTemplateSpec{
                ObjectMeta: metav1.ObjectMeta{
                    Labels: labels,
                },
                Spec: corev1.PodSpec{
                    Containers: []corev1.Container{
                        {
                            Name:  "themisdb",
                            Image: fmt.Sprintf("themisdb/server:%s", cluster.Spec.Version),
                            Ports: []corev1.ContainerPort{
                                {Name: "http", ContainerPort: 8529},
                                {Name: "replication", ContainerPort: 8530},
                            },
                            Resources: cluster.Spec.Resources,
                            VolumeMounts: []corev1.VolumeMount{
                                {
                                    Name:      "data",
                                    MountPath: "/data",
                                },
                            },
                            LivenessProbe: &corev1.Probe{
                                ProbeHandler: corev1.ProbeHandler{
                                    HTTPGet: &corev1.HTTPGetAction{
                                        Path: "/_admin/health",
                                        Port: intstr.FromInt(8529),
                                    },
                                },
                                InitialDelaySeconds: 30,
                                PeriodSeconds:       10,
                            },
                            ReadinessProbe: &corev1.Probe{
                                ProbeHandler: corev1.ProbeHandler{
                                    HTTPGet: &corev1.HTTPGetAction{
                                        Path: "/_admin/ready",
                                        Port: intstr.FromInt(8529),
                                    },
                                },
                                InitialDelaySeconds: 10,
                                PeriodSeconds:       5,
                            },
                        },
                    },
                },
            },
            VolumeClaimTemplates: []corev1.PersistentVolumeClaim{
                {
                    ObjectMeta: metav1.ObjectMeta{
                        Name: "data",
                    },
                    Spec: corev1.PersistentVolumeClaimSpec{
                        AccessModes: []corev1.PersistentVolumeAccessMode{
                            corev1.ReadWriteOnce,
                        },
                        StorageClassName: &cluster.Spec.Storage.StorageClass,
                        Resources: corev1.ResourceRequirements{
                            Requests: corev1.ResourceList{
                                corev1.ResourceStorage: resource.MustParse(cluster.Spec.Storage.Size),
                            },
                        },
                    },
                },
            },
        },
    }
}

// SetupWithManager registriert Controller mit Manager
func (r *ThemisClusterReconciler) SetupWithManager(mgr ctrl.Manager) error {
    return ctrl.NewControllerManagedBy(mgr).
        For(&themisv1alpha1.ThemisCluster{}).
        Owns(&appsv1.StatefulSet{}).
        Owns(&corev1.Service{}).
        Complete(r)
}
```

#### ThemisCluster Resource Beispiel

```yaml
# examples/themisdb-cluster.yaml
# Beispiel: ThemisCluster Custom Resource
apiVersion: themisdb.io/v1alpha1
kind: ThemisCluster
metadata:
  name: themis-production
  namespace: production
spec:
  # Cluster mit 5 Knoten
  replicas: 5
  
  # ThemisDB Version
  version: "1.3.4"
  
  # Storage-Konfiguration
  storage:
    size: 500Gi
    storageClass: fast-ssd
  
  # Resource Requests/Limits
  resources:
    requests:
      memory: 16Gi
      cpu: 4000m
    limits:
      memory: 32Gi
      cpu: 8000m
  
  # Automatische Backups
  backup:
    enabled: true
    schedule: "0 2 * * *"  # Täglich 2 Uhr
    retention: 30  # 30 Tage
    destination:
      type: s3
      bucket: themis-prod-backups
      path: /production/backups
  
  # Monitoring aktiviert
  monitoring:
    enabled: true
    prometheus:
      serviceMonitor: true
```

**Operator Deployment:**

```bash
# Operator installieren
kubectl apply -f deploy/crds/themisdb-cluster-crd.yaml
kubectl apply -f deploy/operator.yaml

# ThemisCluster erstellen
kubectl apply -f examples/themisdb-cluster.yaml

# Status überprüfen
kubectl get themiscluster -n production

# Output:
# NAME                STATUS    HEALTH     REPLICAS   READY   VERSION   AGE
# themis-production   Running   Healthy    5          5/5     1.3.4     5m

# Detaillierter Status
kubectl describe themiscluster themis-production -n production
```

Der Operator automatisiert komplexe Lifecycle-Operationen und reduziert manuelle Intervention. Durch die deklarative CRD-API lassen sich ThemisDB-Cluster wie native Kubernetes-Resources verwalten.

---

## 25.4 GitOps Workflows {#gitops-workflows}

GitOps transformiert die Deployment-Praxis durch Git als Single Source of Truth für die gesamte Infrastruktur- und Anwendungskonfiguration. Wir nutzen deklarative Manifeste in Git-Repositories, automatische Synchronisation durch GitOps-Operatoren wie ArgoCD oder Flux CD und kontinuierliche Drift-Detection mit Self-Healing-Mechanismen. Dieser Ansatz ermöglicht vollständige Audit-Trails, einfache Rollbacks via Git-Revert und Infrastructure-as-Code mit Peer-Review-Prozessen.

### 25.4.1 ArgoCD Application Sets {#argocd-application-sets}

ArgoCD Application Sets erweitern die klassische Application-Definition um Template-basierte Multi-Environment-Deployments. Ein ApplicationSet generiert automatisch ArgoCD Applications für verschiedene Cluster, Namespaces oder Git-Branches und ermöglicht zentralisierte Verwaltung von hunderten Deployments mit minimaler Konfiguration.

### ArgoCD Application Definition

```yaml
# argocd/themis-prod-app.yaml
# Standard ArgoCD Application
apiVersion: argoproj.io/v1alpha1
kind: Application
metadata:
  name: themis-prod
  namespace: argocd
  # Finalizer für Cleanup bei Deletion
  finalizers:
    - resources-finalizer.argocd.argoproj.io
spec:
  # Project Assignment
  project: production
  
  # Source: Git Repository
  source:
    repoURL: https://github.com/themisdb/helm-charts
    targetRevision: v1.3.4
    path: themis-helm
    helm:
      releaseName: themis
      # Values aus Git
      valueFiles:
        - values-prod.yaml
      # Override-Values
      values: |
        replicaCount: 5
        image:
          tag: "1.3.4"
        monitoring:
          enabled: true
  
  # Destination: Kubernetes Cluster
  destination:
    server: https://kubernetes.default.svc
    namespace: themis-prod
  
  # Sync Policy: Automatische Synchronisation
  syncPolicy:
    automated:
      prune: true      # Entfernt Ressourcen die nicht mehr in Git sind
      selfHeal: true   # Korrigiert manuelle Änderungen automatisch
      allowEmpty: false
    syncOptions:
      - CreateNamespace=true
      - PrunePropagationPolicy=foreground
      - PruneLast=true
    retry:
      limit: 5
      backoff:
        duration: 5s
        factor: 2
        maxDuration: 3m
  
  # Ignorierte Differences (für Drift Detection)
  ignoreDifferences:
  - group: apps
    kind: StatefulSet
    jsonPointers:
    - /spec/replicas  # Ignoriere HPA-controlled replicas
```

#### Multi-Environment ApplicationSet

```yaml
# argocd/applicationset-themis.yaml
# ApplicationSet: Automatische Deployment für alle Environments
apiVersion: argoproj.io/v1alpha1
kind: ApplicationSet
metadata:
  name: themis-all-environments
  namespace: argocd
spec:
  # Generator: Liste von Environments
  generators:
  - list:
      elements:
      # Development Environment
      - cluster: in-cluster
        url: https://kubernetes.default.svc
        environment: dev
        namespace: themis-dev
        replicaCount: "1"
        valuesFile: values-dev.yaml
        autoSync: "true"
      
      # Staging Environment
      - cluster: staging-cluster
        url: https://staging.k8s.example.com
        environment: staging
        namespace: themis-staging
        replicaCount: "3"
        valuesFile: values-staging.yaml
        autoSync: "true"
      
      # Production Environment
      - cluster: prod-cluster
        url: https://prod.k8s.example.com
        environment: production
        namespace: themis-prod
        replicaCount: "5"
        valuesFile: values-prod.yaml
        autoSync: "false"  # Manual approval für Production
  
  # Template: Application-Definition mit Variablen
  template:
    metadata:
      name: 'themis-{{environment}}'
      labels:
        environment: '{{environment}}'
    spec:
      project: '{{environment}}'
      
      source:
        repoURL: https://github.com/themisdb/helm-charts
        targetRevision: HEAD
        path: themis-helm
        helm:
          releaseName: themis
          valueFiles:
            - '{{valuesFile}}'
          values: |
            replicaCount: {{replicaCount}}
            image:
              tag: "1.3.4"
            environment: {{environment}}
      
      destination:
        server: '{{url}}'
        namespace: '{{namespace}}'
      
      syncPolicy:
        automated:
          prune: true
          selfHeal: '{{autoSync}}'
        syncOptions:
          - CreateNamespace=true
```

#### Git Generator für Branch-basierte Deployments

```yaml
# argocd/applicationset-git-branches.yaml
# ApplicationSet: Feature-Branch Deployments
apiVersion: argoproj.io/v1alpha1
kind: ApplicationSet
metadata:
  name: themis-feature-branches
  namespace: argocd
spec:
  # Generator: Git Branches mit Pattern
  generators:
  - git:
      repoURL: https://github.com/themisdb/themisdb
      revision: HEAD
      directories:
      - path: helm/*
      
      # Template für Branch-Name Extraktion
      requeueAfterSeconds: 60
      template:
        metadata:
          name: 'themis-feature-{{branch}}'
        spec:
          source:
            repoURL: https://github.com/themisdb/helm-charts
            targetRevision: '{{branch}}'
            path: themis-helm
          destination:
            server: https://kubernetes.default.svc
            namespace: 'preview-{{branch}}'
          syncPolicy:
            automated:
              prune: true
              selfHeal: true
```

### Pull Request Preview Environments

```yaml
# argocd/themis-prod-app.yaml
apiVersion: argoproj.io/v1alpha1
kind: Application
metadata:
  name: themis-prod
  namespace: argocd
spec:
  project: production
  
  source:
    repoURL: https://github.com/themisdb/helm-charts
    targetRevision: v1.3.4
    path: themis-helm
    helm:
      releaseName: themis
      values: |
        replicaCount: 3
        image:
          tag: "1.3.4"
  
  destination:
    server: https://kubernetes.default.svc
    namespace: themis-prod
  
  syncPolicy:
    automated:
      prune: true
      selfHeal: true
    syncOptions:
      - CreateNamespace=true
    retry:
      limit: 5
      backoff:
        duration: 5s
        factor: 2
        maxDuration: 3m
```

```yaml
# .github/workflows/preview.yml
# GitHub Actions: PR Preview Environments mit ArgoCD
name: Preview Environment

on:
  pull_request:
    paths:
      - 'helm/**'
      - 'src/**'
      - '.github/workflows/preview.yml'

jobs:
  deploy-preview:
    runs-on: ubuntu-latest
    steps:
      - name: Checkout Code
        uses: actions/checkout@v4
      
      - name: Generate Preview Name
        id: preview
        run: |
          PR_NUMBER=${{ github.event.pull_request.number }}
          PREVIEW_NAME="themis-pr-${PR_NUMBER}"
          echo "name=${PREVIEW_NAME}" >> $GITHUB_OUTPUT
          echo "namespace=preview-${PR_NUMBER}" >> $GITHUB_OUTPUT
      
      - name: Create ArgoCD Application for Preview
        run: |
          cat <<EOF | kubectl apply -f -
          apiVersion: argoproj.io/v1alpha1
          kind: Application
          metadata:
            name: ${{ steps.preview.outputs.name }}
            namespace: argocd
            labels:
              preview: "true"
              pr-number: "${{ github.event.pull_request.number }}"
          spec:
            project: preview
            source:
              repoURL: https://github.com/themisdb/helm-charts
              targetRevision: ${{ github.head_ref }}
              path: themis-helm
              helm:
                values: |
                  replicaCount: 1
                  image:
                    tag: pr-${{ github.event.pull_request.number }}
                  resources:
                    requests:
                      memory: 2Gi
                      cpu: 500m
            destination:
              server: https://kubernetes.default.svc
              namespace: ${{ steps.preview.outputs.namespace }}
            syncPolicy:
              automated:
                prune: true
                selfHeal: true
              syncOptions:
                - CreateNamespace=true
          EOF
      
      - name: Wait for Deployment
        run: |
          kubectl wait --for=condition=Synced \
            application/${{ steps.preview.outputs.name }} \
            -n argocd --timeout=300s
      
      - name: Get Preview URL
        id: url
        run: |
          INGRESS=$(kubectl get ingress -n ${{ steps.preview.outputs.namespace }} \
            -o jsonpath='{.items[0].spec.rules[0].host}')
          echo "url=https://${INGRESS}" >> $GITHUB_OUTPUT
      
      - name: Comment PR with Preview URL
        uses: actions/github-script@v7
        with:
          script: |
            github.rest.issues.createComment({
              issue_number: context.issue.number,
              owner: context.repo.owner,
              repo: context.repo.repo,
              body: `🚀 Preview Environment deployed!
              
              **URL:** ${{ steps.url.outputs.url }}
              **Namespace:** ${{ steps.preview.outputs.namespace }}
              **ArgoCD:** https://argocd.example.com/applications/${{ steps.preview.outputs.name }}
              
              This preview will be automatically deleted when the PR is closed.`
            })
  
  # Cleanup bei PR Close
  cleanup-preview:
    runs-on: ubuntu-latest
    if: github.event.action == 'closed'
    steps:
      - name: Delete ArgoCD Application
        run: |
          PR_NUMBER=${{ github.event.pull_request.number }}
          kubectl delete application themis-pr-${PR_NUMBER} -n argocd
```

### 25.4.2 Flux CD als Alternative {#flux-cd-alternative}

Flux CD bietet einen alternativen GitOps-Ansatz mit stärkerem Fokus auf Git-Repository-Struktur und Kustomize-Integration. Im Gegensatz zu ArgoCD ist Flux vollständig deklarativ (keine UI) und nutzt GitRepository-CRDs für Source-Management sowie Kustomization-CRDs für Deployment-Orchestrierung.

#### Flux GitRepository und Kustomization

```yaml
# flux/gitrepository.yaml
# GitRepository: Source für Helm Charts
apiVersion: source.toolkit.fluxcd.io/v1
kind: GitRepository
metadata:
  name: themisdb-charts
  namespace: flux-system
spec:
  # Git Repository URL
  url: https://github.com/themisdb/helm-charts
  
  # Branch oder Tag
  ref:
    branch: main
  
  # Sync-Intervall
  interval: 1m
  
  # Git-Credentials (für private Repos)
  secretRef:
    name: git-credentials
  
  # Ignorierte Dateien
  ignore: |
    # Ignoriere CI-Files
    .github/
    .gitlab-ci.yml
```

```yaml
# flux/kustomization.yaml
# Kustomization: Deployment-Definition
apiVersion: kustomize.toolkit.fluxcd.io/v1
kind: Kustomization
metadata:
  name: themisdb-production
  namespace: flux-system
spec:
  # Interval für Sync-Checks
  interval: 10m
  
  # Retry bei Failure
  retryInterval: 1m
  
  # Source: GitRepository
  sourceRef:
    kind: GitRepository
    name: themisdb-charts
  
  # Path innerhalb Repository
  path: ./helm/themis
  
  # Prune: Entferne Ressourcen die nicht mehr in Git sind
  prune: true
  
  # Target Namespace
  targetNamespace: themis-prod
  
  # Health Checks
  healthChecks:
    - apiVersion: apps/v1
      kind: StatefulSet
      name: themis
      namespace: themis-prod
  
  # Timeout für Deployment
  timeout: 5m
  
  # Post-Build Variable Substitution
  postBuild:
    substitute:
      cluster_name: "prod-cluster"
      environment: "production"
    substituteFrom:
      - kind: ConfigMap
        name: cluster-vars
      - kind: Secret
        name: cluster-secrets
```

#### HelmRelease für Helm Charts

```yaml
# flux/helmrelease.yaml
# HelmRelease: Managed Helm Deployment durch Flux
apiVersion: helm.toolkit.fluxcd.io/v2beta1
kind: HelmRelease
metadata:
  name: themisdb
  namespace: flux-system
spec:
  # Release-Intervall
  interval: 5m
  
  # Chart-Quelle
  chart:
    spec:
      chart: themis-helm
      version: "1.3.4"
      sourceRef:
        kind: GitRepository
        name: themisdb-charts
        namespace: flux-system
      interval: 1m
  
  # Target Namespace
  targetNamespace: themis-prod
  
  # Helm Values
  values:
    replicaCount: 5
    
    image:
      repository: themisdb/server
      tag: "1.3.4"
    
    resources:
      requests:
        memory: 16Gi
        cpu: 4000m
      limits:
        memory: 32Gi
        cpu: 8000m
    
    persistence:
      enabled: true
      size: 500Gi
      storageClass: fast-ssd
    
    monitoring:
      enabled: true
      serviceMonitor:
        enabled: true
  
  # Override Values aus ConfigMaps/Secrets
  valuesFrom:
    - kind: ConfigMap
      name: themis-config
      valuesKey: values.yaml
    - kind: Secret
      name: themis-secrets
      valuesKey: secrets.yaml
  
  # Upgrade-Strategie
  upgrade:
    remediation:
      retries: 3
      remediateLastFailure: true
    cleanupOnFail: true
    timeout: 10m
  
  # Rollback bei Failure
  rollback:
    recreate: true
    force: true
    cleanupOnFail: true
  
  # Tests nach Installation
  test:
    enable: true
    timeout: 2m
```

#### Flux Multi-Tenant Setup

```yaml
# flux/tenants/production/kustomization.yaml
# Tenant-spezifische Kustomization für Production
apiVersion: kustomize.toolkit.fluxcd.io/v1
kind: Kustomization
metadata:
  name: production-tenant
  namespace: flux-system
spec:
  interval: 10m
  sourceRef:
    kind: GitRepository
    name: fleet-config
  path: ./tenants/production
  prune: true
  
  # Service Account für Tenant
  serviceAccountName: production-reconciler
  
  # Nur bestimmte Namespaces
  targetNamespace: production
  
  # Dependency: Erst Infrastructure, dann Apps
  dependsOn:
    - name: infrastructure
```

**Flux Installation:**

```bash
# Flux CLI installieren
curl -s https://fluxcd.io/install.sh | sudo bash

# Flux in Cluster installieren
flux install --namespace=flux-system

# GitRepository erstellen
flux create source git themisdb-charts \
  --url=https://github.com/themisdb/helm-charts \
  --branch=main \
  --interval=1m

# HelmRelease erstellen
flux create helmrelease themisdb \
  --source=GitRepository/themisdb-charts \
  --chart=./themis-helm \
  --target-namespace=themis-prod

# Status prüfen
flux get helmreleases -A
```

### 25.4.3 Rollback Strategies {#rollback-strategies}

Automatische Rollback-Mechanismen sind essentiell für Production-Deployments. Wir implementieren Health-Check-basierte Rollbacks, Canary-Deployments mit progressiver Traffic-Verlagerung und manuelle Rollback-Procedures für komplexe Szenarien.

#### ArgoCD Health-Based Rollback

```yaml
# argocd/themis-canary.yaml
# ArgoCD Application mit Rollback Policy
apiVersion: argoproj.io/v1alpha1
kind: Application
metadata:
  name: themis-canary
  namespace: argocd
spec:
  project: production
  
  source:
    repoURL: https://github.com/themisdb/helm-charts
    targetRevision: v1.3.5  # Neue Version
    path: themis-helm
  
  destination:
    server: https://kubernetes.default.svc
    namespace: themis-prod
  
  syncPolicy:
    automated:
      prune: false
      selfHeal: false  # Kein Auto-Heal bei Canary
    
    # Sync nur wenn Health Checks erfolgreich
    syncOptions:
      - CreateNamespace=false
      - ApplyOutOfSyncOnly=true
    
    retry:
      limit: 3
      backoff:
        duration: 5s
        maxDuration: 1m
  
  # Health Assessment
  health:
    - group: apps
      kind: StatefulSet
      check: |
        hs = {}
        if obj.status ~= nil then
          if obj.status.readyReplicas ~= nil and obj.status.replicas ~= nil then
            if obj.status.readyReplicas == obj.status.replicas then
              hs.status = "Healthy"
              hs.message = "All replicas ready"
              return hs
            end
          end
        end
        hs.status = "Progressing"
        hs.message = "Waiting for replicas"
        return hs
```

#### Argo Rollouts für Progressive Delivery

```yaml
# argo-rollouts/themis-rollout.yaml
# Argo Rollout: Canary Deployment mit automatischem Rollback
apiVersion: argoproj.io/v1alpha1
kind: Rollout
metadata:
  name: themis
  namespace: themis-prod
spec:
  replicas: 5
  
  # Deployment-Strategie: Canary
  strategy:
    canary:
      # Canary Steps
      steps:
      # 20% Traffic auf Canary
      - setWeight: 20
      - pause: {duration: 5m}
      
      # Health Check nach 5 Minuten
      - analysis:
          templates:
          - templateName: success-rate
          - templateName: latency-p95
          args:
          - name: service-name
            value: themis-canary
      
      # Bei Erfolg: 50% Traffic
      - setWeight: 50
      - pause: {duration: 5m}
      
      # Finale Analyse
      - analysis:
          templates:
          - templateName: success-rate
          - templateName: error-rate
      
      # 100% Traffic (Full Rollout)
      - setWeight: 100
      
      # Automatisches Rollback bei Analysis-Failure
      abortScaleDownDelaySeconds: 30
      
      # Traffic-Routing via Service Mesh (Istio)
      trafficRouting:
        istio:
          virtualService:
            name: themis-vsvc
            routes:
            - primary
  
  selector:
    matchLabels:
      app: themis
  
  template:
    metadata:
      labels:
        app: themis
        version: v1.3.5
    spec:
      containers:
      - name: themis
        image: themisdb/server:1.3.5
        ports:
        - containerPort: 8529
        
        # Readiness Probe für Traffic-Routing
        readinessProbe:
          httpGet:
            path: /_admin/ready
            port: 8529
          initialDelaySeconds: 10
          periodSeconds: 5
          successThreshold: 3  # 3 erfolgreiche Checks erforderlich
```

#### Analysis Template für Health Checks

```yaml
# argo-rollouts/analysis-template.yaml
# Analysis Template: Success Rate Check
apiVersion: argoproj.io/v1alpha1
kind: AnalysisTemplate
metadata:
  name: success-rate
  namespace: themis-prod
spec:
  args:
  - name: service-name
  
  metrics:
  # Metric 1: HTTP Success Rate
  - name: success-rate
    interval: 30s
    count: 10
    successCondition: result >= 0.95  # 95% Success Rate erforderlich
    failureLimit: 3
    provider:
      prometheus:
        address: http://prometheus.monitoring.svc:9090
        query: |
          sum(rate(http_requests_total{
            service="{{args.service-name}}",
            status=~"2.."
          }[5m]))
          /
          sum(rate(http_requests_total{
            service="{{args.service-name}}"
          }[5m]))
  
  # Metric 2: P95 Latency
  - name: latency-p95
    interval: 30s
    count: 10
    successCondition: result <= 500  # Max 500ms P95 Latency
    failureLimit: 3
    provider:
      prometheus:
        address: http://prometheus.monitoring.svc:9090
        query: |
          histogram_quantile(0.95,
            sum(rate(http_request_duration_seconds_bucket{
              service="{{args.service-name}}"
            }[5m])) by (le)
          ) * 1000
  
  # Metric 3: Error Rate
  - name: error-rate
    interval: 30s
    successCondition: result <= 0.01  # Max 1% Error Rate
    failureLimit: 2
    provider:
      prometheus:
        address: http://prometheus.monitoring.svc:9090
        query: |
          sum(rate(http_requests_total{
            service="{{args.service-name}}",
            status=~"5.."
          }[5m]))
          /
          sum(rate(http_requests_total{
            service="{{args.service-name}}"
          }[5m]))
```

#### Manual Rollback Procedure

```bash
#!/bin/bash
# rollback.sh: Manueller Rollback bei kritischen Issues

set -e

NAMESPACE="themis-prod"
PREVIOUS_VERSION="1.3.4"

echo "Starting manual rollback to version $PREVIOUS_VERSION..."

# Option 1: ArgoCD Rollback zu vorherigem Git Commit
argocd app rollback themis-prod --prune

# Option 2: Helm Rollback zur vorherigen Revision
helm rollback themis -n $NAMESPACE

# Option 3: kubectl Rollback (für Deployments)
kubectl rollout undo statefulset/themis -n $NAMESPACE

# Option 4: GitOps Rollback (Flux CD)
flux suspend kustomization themisdb-production
git revert HEAD --no-edit
git push
flux resume kustomization themisdb-production

# Verify Rollback
echo "Waiting for rollback to complete..."
kubectl rollout status statefulset/themis -n $NAMESPACE --timeout=600s

# Health Check
echo "Running health check..."
HEALTH=$(curl -s http://themis.${NAMESPACE}.svc:8529/_admin/health | jq -r '.status')

if [ "$HEALTH" != "healthy" ]; then
    echo "❌ Rollback failed health check!"
    exit 1
fi

echo "✅ Rollback completed successfully to version $PREVIOUS_VERSION"
```

### 25.4.4 Declarative Configuration Management {#declarative-configuration}

Declarative Configuration Management in GitOps bedeutet, dass der gewünschte Zustand vollständig in Git definiert ist und GitOps-Operatoren kontinuierlich Drift erkennen und korrigieren. Wir implementieren strukturierte Repository-Layouts, Environment-Promotion-Workflows und Policy-as-Code mit OPA (Open Policy Agent).

#### GitOps Repository Structure

```
gitops-repo/
├── README.md
├── .github/
│   └── workflows/
│       ├── validate.yml          # Policy-Validierung bei PR
│       └── promote.yml           # Environment-Promotion
├── base/
│   ├── themisdb/
│   │   ├── kustomization.yaml
│   │   ├── statefulset.yaml
│   │   ├── service.yaml
│   │   └── configmap.yaml
│   └── monitoring/
│       ├── kustomization.yaml
│       └── servicemonitor.yaml
├── overlays/
│   ├── dev/
│   │   ├── kustomization.yaml
│   │   ├── patch-replicas.yaml
│   │   └── patch-resources.yaml
│   ├── staging/
│   │   ├── kustomization.yaml
│   │   ├── patch-replicas.yaml
│   │   └── values-staging.yaml
│   └── production/
│       ├── kustomization.yaml
│       ├── patch-replicas.yaml
│       ├── patch-resources.yaml
│       └── values-prod.yaml
├── policies/
│   ├── policy.rego               # OPA Policy
│   └── policy_test.rego          # Policy Tests
└── clusters/
    ├── dev-cluster/
    │   ├── flux-system/
    │   └── apps/
    ├── staging-cluster/
    │   ├── flux-system/
    │   └── apps/
    └── prod-cluster/
        ├── flux-system/
        └── apps/
```

#### Kustomize für Environment-Overlays

```yaml
# base/themisdb/kustomization.yaml
# Base-Konfiguration für alle Environments
apiVersion: kustomize.config.k8s.io/v1beta1
kind: Kustomization

namespace: themis

resources:
  - statefulset.yaml
  - service.yaml
  - configmap.yaml

# Gemeinsame Labels für alle Resources
commonLabels:
  app: themisdb
  managed-by: flux

# ConfigMap Generator
configMapGenerator:
  - name: themis-config
    literals:
      - log_level=info
      - metrics_enabled=true
```

```yaml
# overlays/production/kustomization.yaml
# Production Overlay mit Patches
apiVersion: kustomize.config.k8s.io/v1beta1
kind: Kustomization

namespace: themis-prod

# Base importieren
bases:
  - ../../base/themisdb

# Production-spezifische Labels
commonLabels:
  environment: production
  tier: critical

# Patches für Production
patchesStrategicMerge:
  - patch-replicas.yaml
  - patch-resources.yaml

# ConfigMap Override
configMapGenerator:
  - name: themis-config
    behavior: merge
    literals:
      - log_level=warn         # Production: Weniger Logs
      - metrics_enabled=true
      - backup_enabled=true
      - backup_schedule=0 2 * * *

# Secret Generator (Sealed Secrets)
secretGenerator:
  - name: themis-secrets
    files:
      - admin-password=secrets/admin-password.enc
      - tls-cert=secrets/tls-cert.enc
```

```yaml
# overlays/production/patch-replicas.yaml
# Patch: Erhöhe Replicas für Production
apiVersion: apps/v1
kind: StatefulSet
metadata:
  name: themis
spec:
  replicas: 5  # Production: 5 Replicas
```

```yaml
# overlays/production/patch-resources.yaml
# Patch: Production Resource Limits
apiVersion: apps/v1
kind: StatefulSet
metadata:
  name: themis
spec:
  template:
    spec:
      containers:
      - name: themis
        resources:
          requests:
            memory: 16Gi
            cpu: 4000m
          limits:
            memory: 32Gi
            cpu: 8000m
```

#### OPA Policy-as-Code

```rego
# policies/policy.rego
# Open Policy Agent: GitOps Policy Enforcement
package themisdb.policies

import future.keywords.if

# Policy 1: Alle Production Deployments müssen Resource Limits haben
deny[msg] if {
    input.kind == "StatefulSet"
    input.metadata.labels.environment == "production"
    container := input.spec.template.spec.containers[_]
    not container.resources.limits
    msg := sprintf("Container %v in production must have resource limits", [container.name])
}

# Policy 2: Production muss mindestens 3 Replicas haben
deny[msg] if {
    input.kind == "StatefulSet"
    input.metadata.labels.environment == "production"
    input.spec.replicas < 3
    msg := "Production StatefulSets must have at least 3 replicas"
}

# Policy 3: Privileged Containers verboten
deny[msg] if {
    input.kind == "Pod"
    container := input.spec.containers[_]
    container.securityContext.privileged == true
    msg := sprintf("Container %v cannot run as privileged", [container.name])
}

# Policy 4: Alle Images müssen von zugelassenen Registries kommen
allowed_registries := ["docker.io/themisdb", "gcr.io/themisdb"]

deny[msg] if {
    input.kind == "StatefulSet"
    container := input.spec.template.spec.containers[_]
    image := container.image
    not startswith(image, allowed_registries[_])
    msg := sprintf("Image %v is not from an allowed registry", [image])
}

# Policy 5: PersistentVolumes müssen verschlüsselt sein
deny[msg] if {
    input.kind == "PersistentVolumeClaim"
    input.metadata.labels.environment == "production"
    not input.metadata.annotations["encrypted"] == "true"
    msg := "Production PVCs must be encrypted"
}
```

```bash
# Policy Validation in CI Pipeline
#!/bin/bash
# .github/workflows/validate-policy.sh

# OPA installieren
curl -L -o opa https://openpolicyagent.org/downloads/latest/opa_linux_amd64
chmod +x opa

# Policy gegen alle Manifests testen
find overlays/production -name '*.yaml' | while read manifest; do
    echo "Validating $manifest..."
    ./opa eval --data policies/policy.rego --input $manifest \
        --format pretty 'data.themisdb.policies.deny'
done
```

Siehe auch: [Kapitel 36: Security Best Practices](#chapter-36) für Policy-Enforcement und [Kapitel 39: Deployment Strategies](#chapter-39) für erweiterte Deployment-Patterns.

---

## 25.5 Configuration Management {#configuration-management}

Configuration Management automatisiert die Installation, Konfiguration und Wartung von ThemisDB-Infrastruktur über hunderte von Servern hinweg. Wir nutzen Ansible für agentless Configuration Management, HashiCorp Vault für zentralisiertes Secret Management und Environment-spezifische Konfigurationen für konsistente Deployments über Dev, Staging und Production.

### 25.5.1 Ansible Playbooks für ThemisDB {#ansible-playbooks}

Ansible bietet deklarative, agentless Automation über SSH und ermöglicht idempotente Playbooks für reproduzierbare Server-Konfiguration. Wir nutzen Ansible für OS-Level-Setup, ThemisDB-Installation, Cluster-Initialisierung und Monitoring-Agent-Deployment.

#### Ansible Inventory Structure

```ini
# inventory/production.ini
# Ansible Inventory für Production ThemisDB Cluster
[themis_primary]
themis-prod-1.example.com ansible_host=10.0.1.10
themis-prod-2.example.com ansible_host=10.0.1.11
themis-prod-3.example.com ansible_host=10.0.1.12

[themis_secondary]
themis-prod-4.example.com ansible_host=10.0.2.10
themis-prod-5.example.com ansible_host=10.0.2.11

[themis_cluster:children]
themis_primary
themis_secondary

[themis_cluster:vars]
ansible_user=ubuntu
ansible_become=yes
ansible_python_interpreter=/usr/bin/python3

# ThemisDB Configuration
themis_version=1.3.4
themis_cluster_name=themis-prod
themis_data_dir=/data/themis
themis_log_dir=/var/log/themis
themis_port=8529
themis_replication_port=8530

# Performance Tuning
themis_max_connections=500
themis_shared_buffers_gb=16
themis_work_mem_mb=256
```

#### Main Playbook für ThemisDB Installation

```yaml
# playbooks/install-themisdb.yml
# Ansible Playbook: ThemisDB Installation und Konfiguration
---
- name: Install and Configure ThemisDB Cluster
  hosts: themis_cluster
  become: yes
  
  vars:
    themis_version: "1.3.4"
    themis_user: themis
    themis_group: themis
  
  tasks:
    # Task 1: System Prerequisites
    - name: Update apt cache
      apt:
        update_cache: yes
        cache_valid_time: 3600
    
    - name: Install system dependencies
      apt:
        name:
          - curl
          - gnupg2
          - apt-transport-https
          - ca-certificates
          - python3-pip
        state: present
    
    # Task 2: ThemisDB User erstellen
    - name: Create ThemisDB system user
      user:
        name: "{{ themis_user }}"
        system: yes
        shell: /bin/bash
        home: "{{ themis_data_dir }}"
        create_home: yes
    
    # Task 3: Directories erstellen
    - name: Create ThemisDB directories
      file:
        path: "{{ item }}"
        state: directory
        owner: "{{ themis_user }}"
        group: "{{ themis_group }}"
        mode: '0755'
      loop:
        - "{{ themis_data_dir }}"
        - "{{ themis_log_dir }}"
        - /etc/themis
        - /opt/themis/bin
    
    # Task 4: ThemisDB Binary herunterladen
    - name: Download ThemisDB binary
      get_url:
        url: "https://github.com/themisdb/themisdb/releases/download/v{{ themis_version }}/themis-server-linux-amd64"
        dest: /opt/themis/bin/themis-server
        mode: '0755'
        owner: "{{ themis_user }}"
      notify: restart themisdb
    
    # Task 5: Konfigurationsdatei deployen
    - name: Deploy ThemisDB configuration
      template:
        src: templates/themis.conf.j2
        dest: /etc/themis/themis.conf
        owner: "{{ themis_user }}"
        group: "{{ themis_group }}"
        mode: '0640'
      notify: restart themisdb
    
    # Task 6: Systemd Service erstellen
    - name: Create systemd service file
      template:
        src: templates/themisdb.service.j2
        dest: /etc/systemd/system/themisdb.service
        mode: '0644'
      notify:
        - reload systemd
        - restart themisdb
    
    # Task 7: Performance Tuning (sysctl)
    - name: Apply kernel tuning for database workloads
      sysctl:
        name: "{{ item.name }}"
        value: "{{ item.value }}"
        state: present
        reload: yes
      loop:
        - { name: 'vm.swappiness', value: '10' }
        - { name: 'vm.dirty_ratio', value: '15' }
        - { name: 'vm.dirty_background_ratio', value: '5' }
        - { name: 'net.core.somaxconn', value: '4096' }
        - { name: 'net.ipv4.tcp_max_syn_backlog', value: '8192' }
    
    # Task 8: Firewall-Regeln
    - name: Configure firewall rules
      ufw:
        rule: allow
        port: "{{ item }}"
        proto: tcp
      loop:
        - "{{ themis_port }}"
        - "{{ themis_replication_port }}"
    
    # Task 9: ThemisDB Service starten
    - name: Start and enable ThemisDB service
      systemd:
        name: themisdb
        state: started
        enabled: yes
  
  # Handlers für Service-Management
  handlers:
    - name: reload systemd
      systemd:
        daemon_reload: yes
    
    - name: restart themisdb
      systemd:
        name: themisdb
        state: restarted

# Play 2: Cluster Initialisierung (nur auf Primary Node)
- name: Initialize ThemisDB Cluster
  hosts: themis_primary[0]
  become: yes
  become_user: "{{ themis_user }}"
  
  tasks:
    - name: Wait for ThemisDB to be ready
      wait_for:
        port: "{{ themis_port }}"
        delay: 5
        timeout: 60
    
    - name: Initialize cluster
      command: >
        /opt/themis/bin/themis-cli cluster init
        --endpoints {{ groups['themis_cluster'] | map('extract', hostvars, 'ansible_host') | map('regex_replace', '^(.*)$', '\1:' + themis_port|string) | join(',') }}
      args:
        creates: "{{ themis_data_dir }}/cluster-initialized"
    
    - name: Mark cluster as initialized
      file:
        path: "{{ themis_data_dir }}/cluster-initialized"
        state: touch

# Play 3: Cluster Verification
- name: Verify ThemisDB Cluster Health
  hosts: themis_cluster
  tasks:
    - name: Check ThemisDB service status
      systemd:
        name: themisdb
      register: service_status
      failed_when: service_status.status.ActiveState != 'active'
    
    - name: Query cluster health
      uri:
        url: "http://localhost:{{ themis_port }}/_admin/health"
        method: GET
        return_content: yes
      register: health_check
      failed_when: "'healthy' not in health_check.content"
    
    - name: Display cluster status
      debug:
        msg: "ThemisDB on {{ inventory_hostname }} is healthy"
```

#### Configuration Templates

```jinja2
# templates/themis.conf.j2
# Jinja2 Template: ThemisDB Configuration
# Generiert durch Ansible

[server]
# Bind-Address für HTTP API
endpoint = {{ ansible_default_ipv4.address }}:{{ themis_port }}

# Replication Endpoint
replication_endpoint = {{ ansible_default_ipv4.address }}:{{ themis_replication_port }}

# Data Directory
data_directory = {{ themis_data_dir }}

# Log Configuration
log_directory = {{ themis_log_dir }}
log_level = info
log_output = file

[cluster]
# Cluster Name
cluster_name = {{ themis_cluster_name }}

# Cluster Members (Auto-Discovery)
members = {% for host in groups['themis_cluster'] %}{{ hostvars[host].ansible_host }}:{{ themis_port }}{% if not loop.last %},{% endif %}{% endfor %}

# Replication Factor
replication_factor = 3

# Automatic Leader Election
auto_leader_election = true

[performance]
# Connection Pool
max_connections = {{ themis_max_connections }}

# Memory Settings
shared_buffers = {{ themis_shared_buffers_gb }}GB
work_mem = {{ themis_work_mem_mb }}MB

# Query Optimization
enable_query_cache = true
query_cache_size = 2GB

[security]
# Authentication
authentication_enabled = true
jwt_secret_file = /etc/themis/jwt-secret

# TLS Configuration
{% if themis_tls_enabled %}
tls_enabled = true
tls_cert_file = /etc/themis/tls/server.crt
tls_key_file = /etc/themis/tls/server.key
{% else %}
tls_enabled = false
{% endif %}

[backup]
# Backup Configuration
backup_enabled = {{ themis_backup_enabled | default(false) }}
{% if themis_backup_enabled %}
backup_schedule = {{ themis_backup_schedule }}
backup_destination = {{ themis_backup_destination }}
backup_retention_days = {{ themis_backup_retention }}
{% endif %}

[monitoring]
# Prometheus Metrics
metrics_enabled = true
metrics_endpoint = {{ ansible_default_ipv4.address }}:9090

# Health Check Endpoint
health_check_enabled = true
```

```jinja2
# templates/themisdb.service.j2
# Systemd Service Template für ThemisDB
[Unit]
Description=ThemisDB Server
Documentation=https://docs.themisdb.io
After=network.target

[Service]
Type=simple
User={{ themis_user }}
Group={{ themis_group }}

# Service Binary
ExecStart=/opt/themis/bin/themis-server --config /etc/themis/themis.conf

# Service Management
Restart=on-failure
RestartSec=5s
TimeoutStopSec=30s

# Resource Limits
LimitNOFILE=65536
LimitNPROC=4096

# Security Hardening
NoNewPrivileges=true
PrivateTmp=true
ProtectSystem=strict
ProtectHome=true
ReadWritePaths={{ themis_data_dir }} {{ themis_log_dir }}

# Logging
StandardOutput=journal
StandardError=journal
SyslogIdentifier=themisdb

[Install]
WantedBy=multi-user.target
```

#### Ansible Vault für Secrets

```bash
# Secrets mit Ansible Vault verschlüsseln
ansible-vault create group_vars/production/vault.yml

# Inhalt von vault.yml:
---
vault_themis_admin_password: "super-secret-password"
vault_themis_jwt_secret: "jwt-secret-key-base64"
vault_themis_tls_cert: |
  -----BEGIN CERTIFICATE-----
  MIIDXTCCAkWgAwIBAgIJAKJ...
  -----END CERTIFICATE-----
vault_themis_tls_key: |
  -----BEGIN PRIVATE KEY-----
  MIIEvQIBADANBgkqhkiG9w0B...
  -----END PRIVATE KEY-----

# Playbook mit Vault ausführen
ansible-playbook -i inventory/production.ini \
  playbooks/install-themisdb.yml \
  --ask-vault-pass
```

### 25.5.2 Chef & Puppet Comparison {#chef-puppet-comparison}

Während Ansible agentless arbeitet, nutzen Chef und Puppet Agent-basierte Architekturen mit Pull-Model. Für ThemisDB bevorzugen wir Ansible wegen einfacherer Setup-Prozeduren und geringerer Infrastruktur-Overhead, aber Enterprise-Umgebungen mit bestehender Chef/Puppet-Infrastruktur können diese nutzen.

| Feature | Ansible | Chef | Puppet | SaltStack |
|---------|---------|------|--------|-----------|
| **Architecture** | Agentless (SSH) | Agent-based (Pull) | Agent-based (Pull) | Agent-based (ZeroMQ) |
| **Language** | YAML (Declarative) | Ruby (Imperative) | Puppet DSL | YAML + Python |
| **Learning Curve** | Niedrig | Hoch | Mittel | Mittel |
| **Setup Time** | <5 min | ~30 min | ~20 min | ~15 min |
| **State Management** | Idempotent Tasks | Convergence | Catalog Compilation | State System |
| **Scalability** | 1000+ Nodes | 10,000+ Nodes | 10,000+ Nodes | 10,000+ Nodes |
| **Windows Support** | Gut | Exzellent | Gut | Gut |
| **Community** | Sehr groß | Groß | Groß | Mittel |
| **Best Use Case** | Cloud-Native | Enterprise Apps | Infrastructure | High-Performance |

**Beobachtungen:**
- Ansible: Beste Wahl für Cloud-Native und Kubernetes-Integration
- Chef: Ideal für komplexe Enterprise-Workflows mit Ruby-Expertise
- Puppet: Stark in traditionellen Enterprise-Umgebungen
- SaltStack: Beste Performance für sehr große Deployments (10,000+ Nodes)

### 25.5.3 HashiCorp Vault für Secret Management {#vault-secret-management}

HashiCorp Vault bietet zentralisiertes Secret Management mit dynamischen Secrets, Encryption-as-a-Service und detailliertem Audit-Logging. Für ThemisDB nutzen wir Vault für Datenbank-Credentials, TLS-Zertifikate und API-Keys mit automatischer Rotation und Policy-basiertem Access-Control.

#### Vault Setup für ThemisDB

```bash
# Vault installieren und starten
wget https://releases.hashicorp.com/vault/1.15.0/vault_1.15.0_linux_amd64.zip
unzip vault_1.15.0_linux_amd64.zip
sudo mv vault /usr/local/bin/

# Vault Server starten (Dev-Mode für Testing)
vault server -dev -dev-root-token-id="root"

# Vault initialisieren (Production)
vault operator init -key-shares=5 -key-threshold=3

# Vault unsealen (Production)
vault operator unseal <unseal-key-1>
vault operator unseal <unseal-key-2>
vault operator unseal <unseal-key-3>

# Auth mit Root Token
export VAULT_ADDR='http://127.0.0.1:8200'
export VAULT_TOKEN='root'
```

#### Secrets in Vault speichern

```bash
# KV Secrets Engine aktivieren
vault secrets enable -path=themisdb kv-v2

# Database Admin Password speichern
vault kv put themisdb/prod/admin \
  password="super-secure-password" \
  username="admin"

# TLS Certificates speichern
vault kv put themisdb/prod/tls \
  certificate=@/path/to/server.crt \
  private_key=@/path/to/server.key \
  ca_cert=@/path/to/ca.crt

# JWT Secret speichern
vault kv put themisdb/prod/jwt \
  secret="jwt-secret-key-base64"

# Backup Credentials speichern
vault kv put themisdb/prod/backup \
  aws_access_key_id="AKIAIOSFODNN7EXAMPLE" \
  aws_secret_access_key="wJalrXUtnFEMI/K7MDENG/bPxRfiCYEXAMPLEKEY" \
  s3_bucket="themis-prod-backups"
```

#### Vault Policy für ThemisDB

```hcl
# policies/themisdb-prod.hcl
# Vault Policy: ThemisDB Production Access
path "themisdb/data/prod/*" {
  capabilities = ["read", "list"]
}

path "themisdb/metadata/prod/*" {
  capabilities = ["read", "list"]
}

# Nur read access für TLS certs
path "themisdb/data/prod/tls" {
  capabilities = ["read"]
}

# Deny write access in production
path "themisdb/data/prod/*" {
  capabilities = ["deny"]
}

# Allow rotation of JWT secret
path "themisdb/data/prod/jwt" {
  capabilities = ["read", "update"]
}
```

```bash
# Policy erstellen und zuweisen
vault policy write themisdb-prod policies/themisdb-prod.hcl

# AppRole für ThemisDB erstellen
vault auth enable approle

vault write auth/approle/role/themisdb-prod \
  token_policies="themisdb-prod" \
  token_ttl=1h \
  token_max_ttl=4h

# Role ID und Secret ID generieren
vault read auth/approle/role/themisdb-prod/role-id
vault write -f auth/approle/role/themisdb-prod/secret-id
```

#### Vault Integration in ThemisDB

```yaml
# kubernetes/vault-injection.yaml
# Vault Sidecar Injection für ThemisDB Pods
apiVersion: apps/v1
kind: StatefulSet
metadata:
  name: themis
  namespace: production
spec:
  template:
    metadata:
      annotations:
        # Vault Agent Injection aktivieren
        vault.hashicorp.com/agent-inject: "true"
        vault.hashicorp.com/role: "themisdb-prod"
        
        # Inject Admin Password
        vault.hashicorp.com/agent-inject-secret-admin: "themisdb/data/prod/admin"
        vault.hashicorp.com/agent-inject-template-admin: |
          {{- with secret "themisdb/data/prod/admin" -}}
          export THEMIS_ADMIN_USER="{{ .Data.data.username }}"
          export THEMIS_ADMIN_PASSWORD="{{ .Data.data.password }}"
          {{- end }}
        
        # Inject TLS Certificates
        vault.hashicorp.com/agent-inject-secret-tls: "themisdb/data/prod/tls"
        vault.hashicorp.com/agent-inject-template-tls: |
          {{- with secret "themisdb/data/prod/tls" -}}
          {{ .Data.data.certificate | base64Decode }}
          {{- end }}
        
        # Inject JWT Secret
        vault.hashicorp.com/agent-inject-secret-jwt: "themisdb/data/prod/jwt"
    
    spec:
      containers:
      - name: themis
        image: themisdb/server:1.3.4
        env:
        # Vault Secrets werden in /vault/secrets gemountet
        - name: THEMIS_CONFIG_DIR
          value: /vault/secrets
```

### 25.5.4 Environment-Specific Configurations {#environment-configs}

Environment-spezifische Konfigurationen ermöglichen konsistente Deployments mit unterschiedlichen Settings für Dev, Staging und Production. Wir nutzen hierarchische Configuration-Struktur mit Base-Configs und Environment-Overrides.

```yaml
# config/base.yaml
# Base-Konfiguration für alle Environments
server:
  port: 8529
  replication_port: 8530
  log_level: info

cluster:
  replication_factor: 3
  auto_failover: true

performance:
  query_cache_enabled: true
  
security:
  authentication_enabled: true
  tls_enabled: true

monitoring:
  metrics_enabled: true
  health_check_enabled: true
```

```yaml
# config/dev.yaml
# Development Environment Overrides
server:
  log_level: debug  # Verbose logging in dev

cluster:
  replication_factor: 1  # Single node in dev

performance:
  max_connections: 50  # Niedrigere Limits

security:
  authentication_enabled: false  # Kein Auth in dev
  tls_enabled: false

resources:
  memory_limit: 2Gi
  cpu_limit: 1000m
```

```yaml
# config/staging.yaml
# Staging Environment Overrides
server:
  log_level: info

cluster:
  replication_factor: 2  # 2 Nodes in staging

performance:
  max_connections: 200

security:
  authentication_enabled: true
  tls_enabled: true

resources:
  memory_limit: 8Gi
  cpu_limit: 2000m

backup:
  enabled: true
  schedule: "0 3 * * *"
  retention: 7
```

```yaml
# config/production.yaml
# Production Environment Overrides
server:
  log_level: warn  # Nur Warnings/Errors in prod

cluster:
  replication_factor: 5  # 5 Nodes für HA

performance:
  max_connections: 500
  shared_buffers_gb: 16
  work_mem_mb: 256

security:
  authentication_enabled: true
  tls_enabled: true
  audit_logging: true

resources:
  memory_limit: 32Gi
  cpu_limit: 8000m

backup:
  enabled: true
  schedule: "0 2 * * *"
  retention: 30
  encryption: true

monitoring:
  prometheus_enabled: true
  grafana_enabled: true
  alerting_enabled: true
```

Siehe auch: [Kapitel 36: Security Best Practices](#chapter-36) für Secret Management und [Kapitel 40: Compliance & Governance](#chapter-40) für Audit-Logging.

---

## 25.6 Observability Stack {#observability-stack}

Ein umfassender Observability Stack kombiniert Metrics (Prometheus), Dashboards (Grafana), Distributed Tracing (Jaeger) und Log Aggregation (ELK/Loki) für vollständige Systemtransparenz. Für ThemisDB implementieren wir einen integrierten Stack mit automatischen Alerts, SLO-Tracking und Performance-Profiling für proaktives Incident Management.

### 25.6.1 Prometheus Configuration für ThemisDB {#prometheus-configuration}

Prometheus scraped Metrics von ThemisDB-Endpoints, speichert Zeitreihen in lokaler TSDB und ermöglicht leistungsstarke PromQL-Queries für Alerting und Dashboards. Wir konfigurieren Service-Discovery, Scrape-Targets und Recording-Rules für optimale Performance.

#### Prometheus Configuration

```yaml
# prometheus/prometheus.yml
# Prometheus Konfiguration für ThemisDB Monitoring
global:
  # Scrape-Intervall (Standard)
  scrape_interval: 15s
  scrape_timeout: 10s
  
  # Evaluation-Intervall für Recording/Alerting Rules
  evaluation_interval: 15s
  
  # Externe Labels für alle Metrics
  external_labels:
    cluster: themis-prod
    environment: production
    region: eu-central-1

# Alertmanager Configuration
alerting:
  alertmanagers:
  - static_configs:
    - targets:
      - alertmanager:9093
    timeout: 10s

# Rule Files laden
rule_files:
  - /etc/prometheus/rules/*.yml

# Scrape Configurations
scrape_configs:
  # Job 1: ThemisDB Server Metrics
  - job_name: 'themisdb-servers'
    # Kubernetes Service Discovery
    kubernetes_sd_configs:
    - role: pod
      namespaces:
        names:
        - themis-prod
    
    # Relabeling für Pod-Discovery
    relabel_configs:
    - source_labels: [__meta_kubernetes_pod_label_app]
      regex: themis
      action: keep
    
    - source_labels: [__meta_kubernetes_pod_name]
      target_label: pod
    
    - source_labels: [__meta_kubernetes_namespace]
      target_label: namespace
    
    - source_labels: [__meta_kubernetes_pod_node_name]
      target_label: node
    
    # Metrics-Port
    - source_labels: [__address__]
      regex: '([^:]+)(?::\d+)?'
      replacement: '${1}:9090'
      target_label: __address__
    
    # Metric Path
    - target_label: __metrics_path__
      replacement: /metrics
  
  # Job 2: ThemisDB Replication Metrics
  - job_name: 'themisdb-replication'
    static_configs:
    - targets:
      - themis-0.themis:8530
      - themis-1.themis:8530
      - themis-2.themis:8530
      - themis-3.themis:8530
      - themis-4.themis:8530
    
    metrics_path: /_admin/metrics/replication
    scheme: http
  
  # Job 3: Node Exporter (System Metrics)
  - job_name: 'node-exporter'
    kubernetes_sd_configs:
    - role: node
    
    relabel_configs:
    - source_labels: [__address__]
      regex: '([^:]+)(?::\d+)?'
      replacement: '${1}:9100'
      target_label: __address__
  
  # Job 4: Kubernetes API Server
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
```

#### Prometheus Recording Rules

```yaml
# prometheus/rules/themisdb-recording-rules.yml
# Recording Rules: Pre-aggregate häufig genutzte Queries
groups:
  - name: themisdb_recording_rules
    interval: 30s
    rules:
      # Query Rate (5m Durchschnitt)
      - record: themisdb:queries_per_second:rate5m
        expr: rate(themisdb_queries_total[5m])
      
      # Query Latency P50/P95/P99
      - record: themisdb:query_duration_seconds:p50
        expr: histogram_quantile(0.50, rate(themisdb_query_duration_seconds_bucket[5m]))
      
      - record: themisdb:query_duration_seconds:p95
        expr: histogram_quantile(0.95, rate(themisdb_query_duration_seconds_bucket[5m]))
      
      - record: themisdb:query_duration_seconds:p99
        expr: histogram_quantile(0.99, rate(themisdb_query_duration_seconds_bucket[5m]))
      
      # Error Rate
      - record: themisdb:error_rate:rate5m
        expr: rate(themisdb_errors_total[5m])
      
      # Replication Lag (Max über alle Replicas)
      - record: themisdb:replication_lag_seconds:max
        expr: max(themisdb_replication_lag_seconds) by (cluster)
      
      # Disk Usage Percentage
      - record: themisdb:disk_usage_percent
        expr: (themisdb_disk_used_bytes / themisdb_disk_total_bytes) * 100
      
      # Connection Pool Utilization
      - record: themisdb:connection_pool_utilization
        expr: (themisdb_connections_active / themisdb_connections_max) * 100
      
      # Cache Hit Ratio
      - record: themisdb:cache_hit_ratio
        expr: |
          sum(rate(themisdb_cache_hits_total[5m]))
          /
          (sum(rate(themisdb_cache_hits_total[5m])) + sum(rate(themisdb_cache_misses_total[5m])))
```

#### Prometheus Alert Rules

```yaml
# prometheus/rules/themisdb-alerts.yml
# Alert Rules: Critical und Warning Alerts
groups:
  - name: themisdb_alerts
    interval: 30s
    rules:
      # Critical: ThemisDB Down
      - alert: ThemisDBDown
        expr: up{job="themisdb-servers"} == 0
        for: 1m
        labels:
          severity: critical
          component: themisdb
        annotations:
          summary: "ThemisDB instance {{ $labels.pod }} is down"
          description: "ThemisDB pod {{ $labels.pod }} in namespace {{ $labels.namespace }} has been down for more than 1 minute."
      
      # Critical: High Query Latency
      - alert: HighQueryLatency
        expr: themisdb:query_duration_seconds:p95 > 1
        for: 5m
        labels:
          severity: critical
          component: performance
        annotations:
          summary: "High query latency detected"
          description: "P95 query latency is {{ $value }}s (threshold: 1s)"
      
      # Critical: Replication Lag
      - alert: HighReplicationLag
        expr: themisdb:replication_lag_seconds:max > 60
        for: 5m
        labels:
          severity: critical
          component: replication
        annotations:
          summary: "High replication lag detected"
          description: "Replication lag is {{ $value }}s behind primary (threshold: 60s)"
      
      # Warning: High Disk Usage
      - alert: HighDiskUsage
        expr: themisdb:disk_usage_percent > 80
        for: 10m
        labels:
          severity: warning
          component: storage
        annotations:
          summary: "Disk usage is high on {{ $labels.pod }}"
          description: "Disk usage is {{ $value }}% (threshold: 80%)"
      
      # Warning: High Error Rate
      - alert: HighErrorRate
        expr: themisdb:error_rate:rate5m > 10
        for: 5m
        labels:
          severity: warning
          component: queries
        annotations:
          summary: "High error rate detected"
          description: "Error rate is {{ $value }} errors/sec (threshold: 10/sec)"
      
      # Warning: Low Cache Hit Ratio
      - alert: LowCacheHitRatio
        expr: themisdb:cache_hit_ratio < 0.80
        for: 15m
        labels:
          severity: warning
          component: performance
        annotations:
          summary: "Low cache hit ratio"
          description: "Cache hit ratio is {{ $value }} (threshold: 0.80)"
      
      # Warning: Connection Pool Saturation
      - alert: ConnectionPoolSaturation
        expr: themisdb:connection_pool_utilization > 90
        for: 5m
        labels:
          severity: warning
          component: connections
        annotations:
          summary: "Connection pool near saturation"
          description: "Connection pool utilization is {{ $value }}% (threshold: 90%)"
```

### 25.6.2 Grafana Dashboard Setup {#grafana-dashboards}

Grafana visualisiert Prometheus-Metrics in interaktiven Dashboards mit Drill-Down-Fähigkeiten, Annotations und Alerting-Integration. Wir erstellen ThemisDB-spezifische Dashboards für Cluster-Health, Performance-Metrics und Capacity-Planning.

#### Grafana Data Source Configuration

```yaml
# grafana/datasources/prometheus.yaml
# Grafana Data Source: Prometheus
apiVersion: 1

datasources:
  - name: Prometheus
    type: prometheus
    access: proxy
    url: http://prometheus.monitoring.svc:9090
    isDefault: true
    
    # Query Timeout
    timeout: 60
    
    # HTTP Settings
    httpMethod: POST
    
    # Prometheus-spezifische Settings
    jsonData:
      timeInterval: 15s
      queryTimeout: 60s
      httpMethod: POST
      
      # Exemplar Support (für Tracing)
      exemplarTraceIdDestinations:
        - name: trace_id
          datasourceUid: jaeger-uid
    
    editable: false
```

#### ThemisDB Overview Dashboard (JSON Excerpt)

```json
{
  "dashboard": {
    "title": "ThemisDB - Production Overview",
    "tags": ["themisdb", "production", "database"],
    "timezone": "browser",
    "refresh": "30s",
    
    "panels": [
      {
        "id": 1,
        "title": "Cluster Health Status",
        "type": "stat",
        "gridPos": {"x": 0, "y": 0, "w": 6, "h": 4},
        "targets": [
          {
            "expr": "up{job=\"themisdb-servers\"}",
            "legendFormat": "{{ pod }}",
            "refId": "A"
          }
        ],
        "options": {
          "graphMode": "none",
          "colorMode": "background",
          "textMode": "auto"
        },
        "fieldConfig": {
          "defaults": {
            "mappings": [
              {"type": "value", "value": "0", "text": "DOWN"},
              {"type": "value", "value": "1", "text": "UP"}
            ],
            "thresholds": {
              "mode": "absolute",
              "steps": [
                {"value": 0, "color": "red"},
                {"value": 1, "color": "green"}
              ]
            }
          }
        }
      },
      
      {
        "id": 2,
        "title": "Queries Per Second",
        "type": "graph",
        "gridPos": {"x": 6, "y": 0, "w": 12, "h": 8},
        "targets": [
          {
            "expr": "sum(themisdb:queries_per_second:rate5m)",
            "legendFormat": "Total QPS",
            "refId": "A"
          },
          {
            "expr": "sum(themisdb:queries_per_second:rate5m) by (query_type)",
            "legendFormat": "{{ query_type }}",
            "refId": "B"
          }
        ],
        "yaxes": [
          {"format": "short", "label": "Queries/sec"},
          {"format": "short", "show": false}
        ]
      },
      
      {
        "id": 3,
        "title": "Query Latency Percentiles",
        "type": "graph",
        "gridPos": {"x": 0, "y": 8, "w": 12, "h": 8},
        "targets": [
          {
            "expr": "themisdb:query_duration_seconds:p50",
            "legendFormat": "P50",
            "refId": "A"
          },
          {
            "expr": "themisdb:query_duration_seconds:p95",
            "legendFormat": "P95",
            "refId": "B"
          },
          {
            "expr": "themisdb:query_duration_seconds:p99",
            "legendFormat": "P99",
            "refId": "C"
          }
        ],
        "yaxes": [
          {"format": "s", "label": "Latency"},
          {"format": "short", "show": false}
        ],
        "alert": {
          "name": "High P95 Latency",
          "conditions": [
            {
              "evaluator": {"params": [1], "type": "gt"},
              "query": {"params": ["B", "5m", "now"]},
              "reducer": {"type": "avg"}
            }
          ]
        }
      },
      
      {
        "id": 4,
        "title": "Replication Lag",
        "type": "graph",
        "gridPos": {"x": 12, "y": 8, "w": 12, "h": 8},
        "targets": [
          {
            "expr": "themisdb_replication_lag_seconds",
            "legendFormat": "{{ pod }}",
            "refId": "A"
          }
        ],
        "yaxes": [
          {"format": "s", "label": "Lag (seconds)"},
          {"format": "short", "show": false}
        ]
      },
      
      {
        "id": 5,
        "title": "Disk Usage",
        "type": "gauge",
        "gridPos": {"x": 0, "y": 16, "w": 8, "h": 8},
        "targets": [
          {
            "expr": "themisdb:disk_usage_percent",
            "legendFormat": "{{ pod }}",
            "refId": "A"
          }
        ],
        "options": {
          "showThresholdLabels": true,
          "showThresholdMarkers": true
        },
        "fieldConfig": {
          "defaults": {
            "unit": "percent",
            "thresholds": {
              "mode": "absolute",
              "steps": [
                {"value": 0, "color": "green"},
                {"value": 70, "color": "yellow"},
                {"value": 85, "color": "red"}
              ]
            }
          }
        }
      }
    ]
  }
}
```

#### Grafana Provisioning Configuration

```yaml
# grafana/dashboards/dashboards.yaml
# Dashboard Provisioning für automatisches Laden
apiVersion: 1

providers:
  - name: 'ThemisDB Dashboards'
    orgId: 1
    folder: 'ThemisDB'
    type: file
    disableDeletion: false
    updateIntervalSeconds: 30
    allowUiUpdates: true
    options:
      path: /var/lib/grafana/dashboards/themisdb
```

### 25.6.3 Distributed Tracing mit Jaeger {#distributed-tracing}

Distributed Tracing verfolgt Requests über Microservice-Grenzen hinweg und identifiziert Latency-Bottlenecks in verteilten Systemen. Jaeger sammelt Traces von ThemisDB-Queries, visualisiert Request-Flows und ermöglicht Performance-Profiling mit Span-Level-Granularität.

#### Jaeger Deployment

```yaml
# jaeger/deployment.yaml
# Jaeger All-in-One Deployment (Development) oder Production Setup
apiVersion: apps/v1
kind: Deployment
metadata:
  name: jaeger
  namespace: monitoring
  labels:
    app: jaeger
spec:
  replicas: 1
  selector:
    matchLabels:
      app: jaeger
  template:
    metadata:
      labels:
        app: jaeger
    spec:
      containers:
      - name: jaeger
        image: jaegertracing/all-in-one:1.51
        env:
        # Storage Backend: Elasticsearch
        - name: SPAN_STORAGE_TYPE
          value: elasticsearch
        - name: ES_SERVER_URLS
          value: http://elasticsearch.monitoring.svc:9200
        - name: ES_INDEX_PREFIX
          value: jaeger
        
        # Collector Configuration
        - name: COLLECTOR_ZIPKIN_HOST_PORT
          value: ":9411"
        - name: COLLECTOR_OTLP_ENABLED
          value: "true"
        
        ports:
        # Jaeger UI
        - containerPort: 16686
          name: ui
          protocol: TCP
        
        # Jaeger Collector (gRPC)
        - containerPort: 14250
          name: grpc
          protocol: TCP
        
        # Jaeger Collector (HTTP)
        - containerPort: 14268
          name: http
          protocol: TCP
        
        # Zipkin Compatibility
        - containerPort: 9411
          name: zipkin
          protocol: TCP
        
        # OTLP (OpenTelemetry Protocol)
        - containerPort: 4317
          name: otlp-grpc
          protocol: TCP
        - containerPort: 4318
          name: otlp-http
          protocol: TCP
        
        resources:
          requests:
            memory: 1Gi
            cpu: 500m
          limits:
            memory: 2Gi
            cpu: 1000m
        
        # Health Checks
        livenessProbe:
          httpGet:
            path: /
            port: 16686
          initialDelaySeconds: 30
        
        readinessProbe:
          httpGet:
            path: /
            port: 16686
          initialDelaySeconds: 10
---
apiVersion: v1
kind: Service
metadata:
  name: jaeger
  namespace: monitoring
spec:
  selector:
    app: jaeger
  ports:
  - name: ui
    port: 16686
    targetPort: ui
  - name: grpc
    port: 14250
    targetPort: grpc
  - name: http
    port: 14268
    targetPort: http
  - name: zipkin
    port: 9411
    targetPort: zipkin
  - name: otlp-grpc
    port: 4317
    targetPort: otlp-grpc
  - name: otlp-http
    port: 4318
    targetPort: otlp-http
```

#### ThemisDB OpenTelemetry Integration

```rust
// src/tracing/mod.rs
// OpenTelemetry Integration für ThemisDB
use opentelemetry::{
    global,
    sdk::{
        trace::{self, Sampler},
        Resource,
    },
    KeyValue,
};
use opentelemetry_jaeger::new_agent_pipeline;
use tracing_subscriber::{layer::SubscriberExt, Registry};

/// Initialisiert OpenTelemetry Tracing mit Jaeger Backend
pub fn init_tracing(service_name: &str, jaeger_endpoint: &str) -> Result<(), Box<dyn std::error::Error>> {
    // Jaeger Tracer erstellen
    let tracer = new_agent_pipeline()
        .with_service_name(service_name)
        .with_endpoint(jaeger_endpoint)
        .with_trace_config(
            trace::config()
                .with_sampler(Sampler::AlwaysOn)  // Alle Traces samplen
                .with_resource(Resource::new(vec![
                    KeyValue::new("service.name", service_name.to_string()),
                    KeyValue::new("service.version", env!("CARGO_PKG_VERSION")),
                    KeyValue::new("environment", "production"),
                ])),
        )
        .install_batch(opentelemetry::runtime::Tokio)?;
    
    // Tracing Subscriber mit OpenTelemetry Layer
    let telemetry = tracing_opentelemetry::layer().with_tracer(tracer);
    let subscriber = Registry::default().with(telemetry);
    
    tracing::subscriber::set_global_default(subscriber)?;
    
    Ok(())
}

/// Beispiel: Query mit Tracing
#[tracing::instrument(
    name = "execute_query",
    skip(db, query),
    fields(
        query.type = %query.query_type(),
        query.collection = %query.collection_name(),
    )
)]
pub async fn execute_query(db: &Database, query: &Query) -> Result<QueryResult, Error> {
    // Sub-Span für Query-Parsing
    let parse_span = tracing::info_span!("parse_query");
    let parsed = parse_span.in_scope(|| {
        query.parse()
    })?;
    
    // Sub-Span für Query-Optimization
    let optimize_span = tracing::info_span!("optimize_query");
    let optimized = optimize_span.in_scope(|| {
        parsed.optimize()
    })?;
    
    // Sub-Span für Query-Execution
    let exec_span = tracing::info_span!(
        "execute_optimized_query",
        execution.plan = %optimized.plan_type()
    );
    let result = exec_span.in_scope(|| async {
        db.execute(optimized).await
    }).await?;
    
    // Trace-Attribute hinzufügen
    tracing::Span::current().record("query.rows_returned", result.row_count());
    tracing::Span::current().record("query.duration_ms", result.duration_ms());
    
    Ok(result)
}
```

#### Jaeger Query Tracing Configuration

```yaml
# themisdb/config/tracing.yaml
# ThemisDB Tracing Configuration
tracing:
  enabled: true
  
  # Jaeger Collector Endpoint
  jaeger_endpoint: "jaeger.monitoring.svc:14250"
  
  # Service Name
  service_name: "themisdb-prod"
  
  # Sampling Strategy
  sampling:
    type: probabilistic  # always_on, probabilistic, rate_limiting
    param: 0.1  # 10% Sampling Rate
  
  # Baggage Propagation (für Cross-Service Context)
  baggage:
    - user_id
    - request_id
    - tenant_id
  
  # Span Processors
  processors:
    - type: batch
      batch_size: 512
      batch_timeout: 5s
      max_queue_size: 2048
  
  # Exporters
  exporters:
    - type: jaeger
      endpoint: "jaeger.monitoring.svc:14250"
    - type: prometheus  # Trace Metrics
      endpoint: "prometheus.monitoring.svc:9090"
```

### 25.6.4 Log Aggregation mit ELK Stack / Loki {#log-aggregation}

Zentralisierte Log-Aggregation sammelt Logs von allen ThemisDB-Instanzen, ermöglicht Full-Text-Search über historische Logs und korreliert Logs mit Traces und Metrics. Wir vergleichen ELK Stack (Elasticsearch, Logstash, Kibana) mit Grafana Loki für kosteneffiziente Log-Speicherung.

#### Promtail für Loki (Lightweight Alternative)

```yaml
# loki/promtail-config.yaml
# Promtail: Log Collector für Grafana Loki
apiVersion: v1
kind: ConfigMap
metadata:
  name: promtail-config
  namespace: monitoring
data:
  promtail.yaml: |
    # Promtail Server Configuration
    server:
      http_listen_port: 9080
      grpc_listen_port: 0
    
    # Loki Client Configuration
    clients:
      - url: http://loki.monitoring.svc:3100/loki/api/v1/push
        backoff_config:
          min_period: 100ms
          max_period: 10s
          max_retries: 10
        batch_wait: 1s
        batch_size: 1048576  # 1MB
        timeout: 10s
    
    # Positions File (Track welche Logs gelesen wurden)
    positions:
      filename: /tmp/positions.yaml
    
    # Scrape Configs
    scrape_configs:
      # Kubernetes Pod Logs
      - job_name: kubernetes-pods
        kubernetes_sd_configs:
          - role: pod
        
        # Pipeline Stages
        pipeline_stages:
          # Extrahiere Timestamps
          - docker: {}
          
          # Parse JSON Logs
          - json:
              expressions:
                level: level
                message: message
                query_id: query_id
                duration_ms: duration_ms
          
          # Label Extraction
          - labels:
              level:
              query_id:
          
          # Timestamp aus Log extrahieren
          - timestamp:
              source: timestamp
              format: RFC3339
        
        # Relabeling
        relabel_configs:
          # Nur ThemisDB Pods
          - source_labels: [__meta_kubernetes_pod_label_app]
            regex: themis
            action: keep
          
          # Pod Name als Label
          - source_labels: [__meta_kubernetes_pod_name]
            target_label: pod
          
          # Namespace als Label
          - source_labels: [__meta_kubernetes_namespace]
            target_label: namespace
          
          # Container Name als Label
          - source_labels: [__meta_kubernetes_pod_container_name]
            target_label: container
          
          # Node Name als Label
          - source_labels: [__meta_kubernetes_pod_node_name]
            target_label: node
          
          # Log Path
          - source_labels: [__meta_kubernetes_pod_uid, __meta_kubernetes_pod_container_name]
            target_label: __path__
            separator: /
            replacement: /var/log/pods/*$1/*.log
      
      # ThemisDB Slow Query Logs
      - job_name: themisdb-slow-queries
        static_configs:
          - targets:
              - localhost
            labels:
              job: themisdb-slow-queries
              __path__: /var/log/themis/slow-queries.log
        
        pipeline_stages:
          # Parse Slow Query Log Format
          - regex:
              expression: '^(?P<timestamp>\S+) (?P<level>\S+) Query took (?P<duration>\d+)ms: (?P<query>.+)$'
          
          - labels:
              level:
              duration:
          
          - timestamp:
              source: timestamp
              format: RFC3339
---
apiVersion: apps/v1
kind: DaemonSet
metadata:
  name: promtail
  namespace: monitoring
spec:
  selector:
    matchLabels:
      app: promtail
  template:
    metadata:
      labels:
        app: promtail
    spec:
      serviceAccountName: promtail
      containers:
      - name: promtail
        image: grafana/promtail:2.9.0
        args:
          - -config.file=/etc/promtail/promtail.yaml
        
        volumeMounts:
        # Promtail Config
        - name: config
          mountPath: /etc/promtail
        
        # Pod Logs
        - name: pods
          mountPath: /var/log/pods
          readOnly: true
        
        # Container Logs
        - name: containers
          mountPath: /var/lib/docker/containers
          readOnly: true
        
        # Positions File
        - name: positions
          mountPath: /tmp
        
        resources:
          requests:
            memory: 128Mi
            cpu: 100m
          limits:
            memory: 256Mi
            cpu: 200m
      
      volumes:
      - name: config
        configMap:
          name: promtail-config
      
      - name: pods
        hostPath:
          path: /var/log/pods
      
      - name: containers
        hostPath:
          path: /var/lib/docker/containers
      
      - name: positions
        hostPath:
          path: /tmp/promtail-positions
```

#### Loki Deployment

```yaml
# loki/deployment.yaml
# Grafana Loki Deployment
apiVersion: apps/v1
kind: StatefulSet
metadata:
  name: loki
  namespace: monitoring
spec:
  serviceName: loki
  replicas: 3
  selector:
    matchLabels:
      app: loki
  template:
    metadata:
      labels:
        app: loki
    spec:
      containers:
      - name: loki
        image: grafana/loki:2.9.0
        args:
          - -config.file=/etc/loki/loki.yaml
        
        ports:
        - containerPort: 3100
          name: http
        
        volumeMounts:
        - name: config
          mountPath: /etc/loki
        - name: data
          mountPath: /loki
        
        resources:
          requests:
            memory: 2Gi
            cpu: 1000m
          limits:
            memory: 4Gi
            cpu: 2000m
      
      volumes:
      - name: config
        configMap:
          name: loki-config
  
  volumeClaimTemplates:
  - metadata:
      name: data
    spec:
      accessModes: ["ReadWriteOnce"]
      storageClassName: fast-ssd
      resources:
        requests:
          storage: 100Gi
---
apiVersion: v1
kind: ConfigMap
metadata:
  name: loki-config
  namespace: monitoring
data:
  loki.yaml: |
    # Loki Configuration
    auth_enabled: false
    
    server:
      http_listen_port: 3100
      grpc_listen_port: 9095
    
    # Ingester Configuration
    ingester:
      lifecycler:
        address: 127.0.0.1
        ring:
          kvstore:
            store: inmemory
          replication_factor: 1
      chunk_idle_period: 5m
      chunk_retain_period: 30s
    
    # Schema Configuration
    schema_config:
      configs:
        - from: 2024-01-01
          store: boltdb-shipper
          object_store: filesystem
          schema: v11
          index:
            prefix: index_
            period: 24h
    
    # Storage Configuration
    storage_config:
      boltdb_shipper:
        active_index_directory: /loki/boltdb-shipper-active
        cache_location: /loki/boltdb-shipper-cache
        shared_store: filesystem
      
      filesystem:
        directory: /loki/chunks
    
    # Limits Configuration
    limits_config:
      enforce_metric_name: false
      reject_old_samples: true
      reject_old_samples_max_age: 168h  # 7 Tage
      ingestion_rate_mb: 10
      ingestion_burst_size_mb: 20
    
    # Query Configuration
    query_range:
      results_cache:
        cache:
          enable_fifocache: true
          fifocache:
            max_size_bytes: 1GB
            ttl: 24h
    
    # Table Manager (Index Retention)
    table_manager:
      retention_deletes_enabled: true
      retention_period: 2160h  # 90 Tage
```

#### ELK Stack vs Loki Comparison

| Feature | ELK Stack | Grafana Loki | Observability Impact |
|---------|-----------|--------------|----------------------|
| **Storage Model** | Full-Text Index | Label-based Index | ELK: 10x Storage, Loki: 5x less |
| **Query Language** | Lucene Query | LogQL (similar to PromQL) | LogQL: Steile Lernkurve |
| **Ingestion Rate** | 50K logs/sec/node | 100K logs/sec/node | Loki: 2x Performance |
| **Storage Cost** | $200/TB/month | $40/TB/month | Loki: 80% Cost Savings |
| **Search Speed** | <100ms (indexed) | 100-500ms (label filters) | ELK: Schneller für Full-Text |
| **RAM Requirements** | 8GB minimum | 2GB minimum | Loki: 75% weniger RAM |
| **Cardinality Limits** | Unlimited fields | Limited to labels | ELK: Flexibler |
| **Integration** | Kibana UI | Grafana Native | Loki: Unified Observability |
| **Setup Complexity** | High (3 components) | Low (2 components) | Loki: Einfacher |

**Empfehlung für ThemisDB:**
- **Grafana Loki** für Production: Kosteneffizienz, einfache Integration, ausreichende Query-Performance
- **ELK Stack** für Compliance: Regulatorische Anforderungen mit Full-Text-Audit-Trails

### 25.6.5 Observability Overhead Benchmarks {#observability-overhead}

Observability-Tools verursachen Performance-Overhead durch Metrics-Collection, Trace-Sampling und Log-Shipping. Unsere Benchmarks quantifizieren den Overhead verschiedener Observability-Konfigurationen auf ThemisDB-Throughput und Latency.

| Configuration | Throughput Impact | P95 Latency Impact | CPU Overhead | Memory Overhead | Storage/Day |
|---------------|-------------------|-------------------|--------------|-----------------|-------------|
| **Baseline (No Observability)** | 50,000 QPS | 12ms | 0% | 0% | 0 GB |
| **Metrics Only (Prometheus)** | 49,500 QPS (-1%) | 12.5ms (+4%) | +2% | +100 MB | 2 GB |
| **Metrics + Logs (Loki)** | 48,000 QPS (-4%) | 15ms (+25%) | +5% | +300 MB | 50 GB |
| **Metrics + Logs + Traces (10% sampling)** | 47,000 QPS (-6%) | 16ms (+33%) | +8% | +500 MB | 75 GB |
| **Metrics + Logs + Traces (100% sampling)** | 42,000 QPS (-16%) | 22ms (+83%) | +15% | +1 GB | 200 GB |
| **Full ELK Stack** | 45,000 QPS (-10%) | 18ms (+50%) | +12% | +2 GB | 150 GB |

**Beobachtungen:**
- Metrics-Collection (Prometheus): Minimaler Overhead (<2%), essentiell
- Log-Aggregation (Loki): Moderater Overhead (4-5%), Storage-intensiv
- Distributed Tracing: Signifikanter Overhead bei hohem Sampling (15% bei 100%)
- **Optimale Konfiguration:** Metrics + Logs + 10% Trace Sampling = 6% Overhead

**Performance-Tuning-Tipps:**
1. **Adaptive Sampling:** 100% für Errors, 1% für Success-Cases
2. **Async Log Shipping:** Buffering mit Batch-Writes
3. **Metrics Cardinality:** Limitiere Label-Kombinationen (<10,000)
4. **Storage Tiers:** Hot (7 Tage) → Warm (30 Tage) → Cold (90 Tage)

Siehe auch: [Kapitel 30: Monitoring & Observability](#chapter-30) für erweiterte Monitoring-Patterns und [Kapitel 38: Testing & Benchmarking](#chapter-38) für Performance-Testing-Strategien.

---

## 25.7 Multi-Region Failover {#multi-region-failover}

Multi-Region-Deployments sind essentiell für globale Hochverfügbarkeit und reduzieren Latency durch geografische Nähe zu Endnutzern. ThemisDB unterstützt Active-Active- und Active-Passive-Replikation über AWS-Regions, GCP-Zones oder Azure-Regions hinweg mit automatischem Failover bei regionalen Ausfällen. Cross-Region-Replication nutzt asynchrone Replikation mit Conflict-Resolution-Strategien für Eventually-Consistent-Datenmodelle. Wir implementieren DNS-basiertes Failover mit Route53 Health Checks, Application-Level-Failover mit intelligenten Client-Libraries und Database-Level-Replication mit ThemisDB-nativen Clustering-Features.

Multi-Region-Architekturen müssen Trade-offs zwischen Consistency, Availability und Partition Tolerance (CAP-Theorem) berücksichtigen. Für ThemisDB priorisieren wir Availability und Partition Tolerance mit tunable Consistency-Levels (eventual, strong, bounded-staleness). Cross-Region-Network-Latency beeinflusst Replication-Lag und Query-Performance - typische RTT zwischen EU-Central und US-East beträgt ~80-100ms. Konfliktauflösung erfolgt durch Last-Write-Wins (LWW), Version-Vectors oder Application-Level-Merge-Functions je nach Use-Case-Anforderungen.

### Active-Active Replikation

Active-Active-Topologien erlauben Writes in allen Regions gleichzeitig mit bidirektionaler Replikation. Dies maximiert Write-Throughput und reduziert Latency für geografisch verteilte Benutzer, erfordert aber sophisticated Conflict-Resolution-Mechanismen bei gleichzeitigen Updates desselben Dokuments in verschiedenen Regions. ThemisDB implementiert CRDTs (Conflict-free Replicated Data Types) für automatische Merge-Operationen ohne manuelle Intervention.

```hcl
# multi-region.tf: Cross-Region Setup
resource "aws_rds_cluster" "themis_eu" {
  # EU Cluster (Primary)
  cluster_identifier = "themis-eu-primary"
  region             = "eu-central-1"
}

resource "aws_rds_cluster" "themis_us" {
  # US Cluster (Replica mit Failover)
  cluster_identifier       = "themis-us-replica"
  region                   = "us-east-1"
  replication_source_arn   = aws_rds_cluster.themis_eu.arn
  
  # Automatischer Failover nach 300 Sekunden
  backup_retention_period = 30
}

# Route53 Health Check & Failover
resource "aws_route53_failover_routing_policy" "themis" {
  name            = "themis.example.com"
  zone_id         = aws_route53_zone.main.zone_id
  type            = "A"
  failover_routing_policy {
    type = "PRIMARY"
  }
  
  alias {
    name                   = aws_rds_cluster.themis_eu.endpoint
    zone_id                = aws_rds_cluster.themis_eu.hosted_zone_id
    evaluate_target_health = true
  }
  
  health_check_id = aws_route53_health_check.eu.id
}
```

### Application-Level Failover

```python
# app/failover.py: Client-seitiges Failover
class ResilientThemisClient:
    def __init__(self, primary_url, secondary_url):
        self.primary_url = primary_url
        self.secondary_url = secondary_url
        self.current_url = primary_url
        self.failure_count = 0
        self.max_failures = 3
    
    def query(self, aql, params=None):
        try:
            client = themis.Client(self.current_url)
            result = client.query(aql, params)
            self.failure_count = 0  # Reset bei Erfolg
            return result
        except Exception as e:
            self.failure_count += 1
            
            if self.failure_count >= self.max_failures:
                # Failover zu Secondary
                self.current_url = self.secondary_url
                self.failure_count = 0
                print(f"Failover zu {self.secondary_url}")
                return self.query(aql, params)  # Retry
            raise
```

---

## 25.8 Disaster Recovery Planning {#disaster-recovery-planning}

Disaster Recovery (DR) Planning definiert Prozesse und Technologien zur Wiederherstellung kritischer IT-Systeme nach katastrophalen Ausfällen wie Datacenter-Brand, Naturkatastrophen, Ransomware-Attacken oder menschlichem Versagen. Für ThemisDB implementieren wir mehrstufige Backup-Strategien mit automatisierten Restore-Tests, Point-in-Time-Recovery (PITR) für präzise Wiederherstellung und geografisch verteilte Backup-Replikation für zusätzliche Redundanz. Recovery Time Objective (RTO) definiert maximale tolerierbare Downtime, Recovery Point Objective (RPO) definiert maximalen tolerierbaren Datenverlust.

Production-Grade-DR erfordert regelmäßige Disaster-Recovery-Drills zur Validierung von Procedures und Identifikation von Schwachstellen. ThemisDB-Backups werden in Write-Once-Read-Many (WORM) S3 Buckets gespeichert mit Glacier-Integration für kosteneffiziente Langzeitarchivierung. Automatisierte Restore-Tests verifizieren Backup-Integrität und messen tatsächliche Restore-Zeiten unter Production-Load. Compliance-Anforderungen wie DSGVO erfordern 7-Jahres-Retention für bestimmte Datentypen mit verschlüsselter Speicherung und Audit-Logging aller Backup-Access-Events.

### Backup & Restore Strategie

ThemisDB-Backup-Strategien kombinieren Full-Backups, Incremental-Backups und Continuous-WAL-Archiving für optimale Balance zwischen Backup-Geschwindigkeit, Storage-Kosten und Recovery-Granularität. Full-Backups erfolgen wöchentlich mit kompletter Snapshot-Erstellung, Incremental-Backups täglich mit nur geänderten Blöcken und WAL-Archiving kontinuierlich mit Write-Ahead-Logs für Point-in-Time-Recovery. Backup-Retention folgt Grandfather-Father-Son-Strategie mit 7 Daily-Backups, 4 Weekly-Backups, 12 Monthly-Backups und 7 Yearly-Backups für Compliance.

```bash
#!/bin/bash
# backup_strategy.sh: Daily + Weekly + Monthly Backups

# Täglich: Inkrementales Backup (7 Tage Retention)
aws backup start-backup-job \
  --backup-vault-name themis-backup-vault \
  --recovery-point-type INCREMENTAL \
  --recovery-point-tags "Frequency=Daily,Retention=7days"

# Wöchentlich: Full Backup (4 Wochen Retention)
aws backup start-backup-job \
  --backup-vault-name themis-backup-vault \
  --recovery-point-type FULL \
  --recovery-point-tags "Frequency=Weekly,Retention=4weeks"

# Monatlich: Langzeit-Archiv (7 Jahre für DSGVO)
aws backup start-backup-job \
  --backup-vault-name themis-backup-vault \
  --recovery-point-type FULL \
  --recovery-point-tags "Frequency=Monthly,Retention=7years"
```

### RTO & RPO Metriken

| Szenario | RTO | RPO | Strategie |
|----------|-----|-----|-----------|
| Einzelner Node Fehler | <30s | 0s | Auto-Failover |
| Region Ausfalls | <5min | <1min | Cross-Region Replica |
| Datenbeschädigung | <1h | <1h | Point-in-Time Recovery |
| Vollständiger Datenverlust | <4h | <24h | S3 Langzeit-Archiv |

---

## 25.9 Operational Runbooks {#operational-runbooks}

Operational Runbooks dokumentieren strukturierte Step-by-Step-Procedures für häufige Betriebsszenarien und Incident-Response. Standardisierte Runbooks reduzieren Mean-Time-to-Recovery (MTTR) durch eindeutige Handlungsanweisungen, minimieren menschliche Fehler unter Stress und ermöglichen effektive Knowledge-Transfer für On-Call-Teams. Für ThemisDB erstellen wir Runbooks für kritische Alerts (High-Replication-Lag, Disk-Full, Memory-Exhaustion), geplante Wartung (Rolling-Upgrades, Schema-Migrations, Index-Rebuilds) und seltene Disaster-Scenarios (Complete-Cluster-Failure, Data-Corruption, Security-Breaches).

Effective Runbooks folgen STAR-Format (Situation, Task, Action, Result) mit klaren Severity-Levels (Critical, High, Medium, Low), geschätzten Resolution-Times und Escalation-Paths bei Nicht-Lösung. Automation-Links zu Remediation-Scripts beschleunigen Incident-Response - z.B. kubectl-Commands für Pod-Restarts, Terraform-Apply für Infrastructure-Scaling oder ThemisDB-CLI-Commands für Database-Operations. Post-Incident-Reviews aktualisieren Runbooks basierend auf Learnings und identifizieren Opportunities für präventive Automation.

### Incident Response Playbook

Incident-Response-Playbooks definieren systematische Vorgehensweisen zur Diagnose und Behebung von Production-Incidents. Strukturierte Playbooks gewährleisten konsistente Response-Qualität unabhängig vom On-Call-Engineer und dokumentieren Best-Practices aus hunderten realen Incidents. ThemisDB-Playbooks adressieren häufigste Alerting-Scenarios mit diagnostischen Queries, Remediation-Steps und Rollback-Procedures.

```markdown
## Critical Alert: High Replication Lag

**Severity:** High  
**RTO:** 5 minutes  
**On-call:** Check #oncall Slack channel

### Diagnostik (1-2 min)
1. `kubectl logs -f themis-2` - Prüfe Replica-Logs
2. `curl http://themis-2:8529/_admin/stats` - Replication lag?
3. `aws cloudwatch get-metric-statistics --metric-name ReplicationLatency`

### Remediation (2-3 min)
```bash
# Option 1: Replica neustarten
kubectl delete pod themis-2

# Option 2: Manuelles Resync
curl -X POST http://themis-2:8529/_admin/resync
```

### Post-Incident
- [ ] Logs analysieren
- [ ] Runbook updaten
- [ ] Post-Mortem in Confluence
```

---

## 25.10 Production Readiness Checklist {#production-readiness-checklist}

Production Readiness Reviews (PRR) validieren, dass alle kritischen Aspekte eines Systems vor Go-Live adressiert wurden. Diese comprehensive Checkliste deckt Infrastructure, Security, Monitoring, Performance, Disaster Recovery und Operational-Preparedness ab. Jedes Item sollte von mindestens zwei Engineers verifiziert und dokumentiert werden mit Evidence (Screenshots, Test-Results, Configuration-Files).

**Infrastructure & Deployment:**
- ✅ Terraform/Pulumi IaC für alle Infrastruktur-Komponenten mit Remote State
- ✅ Multi-AZ/Multi-Zone Deployment für High Availability
- ✅ Auto-Scaling Policies konfiguriert (CPU/Memory/Custom Metrics)
- ✅ Load Balancer mit Health Checks und Connection Draining
- ✅ CDN-Integration für statische Assets (CloudFront/Cloudflare)
- ✅ DNS mit Health-Check-basiertem Failover (Route53/CloudDNS)

**Container Orchestration:**
- ✅ Kubernetes Cluster mit min. 3 Control-Plane Nodes
- ✅ Helm Charts mit Multi-Environment Support (dev/staging/prod)
- ✅ StatefulSets mit Persistent Volumes und StorageClass
- ✅ Resource Requests/Limits definiert für alle Pods
- ✅ Pod Disruption Budgets (PDB) für Rolling Updates
- ✅ Network Policies für Pod-to-Pod Communication
- ✅ Kubernetes Operator deployed (optional, aber empfohlen)

**GitOps & CI/CD:**
- ✅ ArgoCD/Flux CD für GitOps-Deployments konfiguriert
- ✅ Automated CI/CD Pipeline mit Quality Gates
- ✅ Blue-Green oder Canary Deployment Strategy
- ✅ Automated Smoke Tests nach Deployment
- ✅ Rollback-Procedures getestet und dokumentiert
- ✅ Preview Environments für Pull Requests

**Security:**
- ✅ All Secrets in HashiCorp Vault oder Sealed Secrets
- ✅ TLS/SSL für alle Endpoints mit Auto-Renewal
- ✅ Network Segmentation mit Security Groups/Firewall Rules
- ✅ Pod Security Policies/Pod Security Standards enforced
- ✅ RBAC konfiguriert mit Least-Privilege-Principle
- ✅ Vulnerability Scanning in CI/CD Pipeline (Trivy/Snyk)
- ✅ Security Audit Logging aktiviert
- ✅ WAF (Web Application Firewall) für öffentliche Endpoints

**Monitoring & Observability:**
- ✅ Prometheus scraping ThemisDB metrics (15s interval)
- ✅ Grafana Dashboards für Cluster Health, Performance, Capacity
- ✅ AlertManager mit Alerting Rules (Critical/Warning)
- ✅ PagerDuty/Opsgenie Integration für On-Call Rotation
- ✅ Distributed Tracing mit Jaeger (min. 10% Sampling)
- ✅ Log Aggregation mit Loki/ELK Stack
- ✅ SLO/SLI Definitions und Tracking
- ✅ CloudWatch/Stackdriver Integration für Cloud-Metrics

**Backup & Disaster Recovery:**
- ✅ Automated Daily Backups mit min. 30-Tage Retention
- ✅ Cross-Region Backup Replication
- ✅ Point-in-Time Recovery (PITR) konfiguriert
- ✅ Backup Restore Tests (mindestens quarterly)
- ✅ Disaster Recovery Runbook dokumentiert
- ✅ RTO/RPO Metriken definiert und gemessen
- ✅ WAL (Write-Ahead Log) Archiving aktiviert

**Performance & Capacity:**
- ✅ Load Testing mit realistischem Production-Traffic
- ✅ Capacity Planning Dashboards (Disk, Memory, CPU Trends)
- ✅ Query Performance Baselines etabliert
- ✅ Index Optimization für häufige Queries
- ✅ Connection Pool Sizing validiert
- ✅ Cache Hit Ratios monitored (Target >80%)
- ✅ Database Parameter Tuning für Production Workload

**Operational Readiness:**
- ✅ Runbooks für Top-10 Critical Alerts
- ✅ On-Call Rotation Schedule mit min. 2 Engineers
- ✅ Incident Response Procedures dokumentiert
- ✅ Escalation Paths definiert
- ✅ Post-Mortem Template und Process
- ✅ Change Management Process etabliert
- ✅ Maintenance Windows kommuniziert
- ✅ Status Page für Customer Communication (Statuspage.io)

**Compliance & Governance:**
- ✅ DSGVO/GDPR Compliance für EU-Daten (wenn applicable)
- ✅ Data Encryption at Rest und in Transit
- ✅ Audit Logging für alle Admin-Aktionen
- ✅ Data Retention Policies implementiert
- ✅ Privacy Impact Assessment (PIA) durchgeführt
- ✅ Third-Party Security Assessments (SOC2/ISO27001)

**Documentation:**
- ✅ Architecture Diagrams (Deployment, Network, Data Flow)
- ✅ API Documentation (OpenAPI/Swagger)
- ✅ Configuration Management Documentation
- ✅ Disaster Recovery Plan dokumentiert
- ✅ Security Incident Response Plan
- ✅ Developer Onboarding Guide
- ✅ User Documentation / Knowledge Base

Diese Checklist sollte 4-6 Wochen vor geplantem Production-Launch initiiert werden mit wöchentlichen Review-Meetings zur Fortschrittsverfolgung. Kritische Items (marked mit ✅ Critical) müssen vor Go-Live completed sein, während Nice-to-Have-Items in Phase-2-Rollout verschoben werden können.

---

**Kapitel-Zusammenfassung:**

Dieses Kapitel hat comprehensive DevOps-Praktiken für Production-ThemisDB-Deployments behandelt - von automatisierten CI/CD-Pipelines über Infrastructure-as-Code bis zu umfassenden Observability-Stacks. Moderne DevOps kombiniert Automation, Monitoring und proaktive Incident-Prevention für maximale System-Reliability. Die beschriebenen Patterns und Tools ermöglichen skalierbare, sichere und wartbare Production-Deployments mit minimaler Manual-Intervention.

Key Takeaways: CI/CD-Automation beschleunigt Deployment-Zyklen von Wochen auf Stunden, Infrastructure-as-Code eliminiert Configuration-Drift und ermöglicht reproduzierbare Infrastruktur, GitOps etabliert Git als Single Source of Truth mit vollständigen Audit-Trails, Kubernetes-Operators automatisieren komplexe Lifecycle-Management-Aufgaben, Observability-Stacks ermöglichen proaktive Problem-Detection vor Customer-Impact, Multi-Region-Failover sichert Business Continuity bei regionalen Ausfällen, Disaster-Recovery-Planning minimiert Datenverlust bei katastrophalen Ausfällen, Operational-Runbooks standardisieren Incident-Response und reduzieren MTTR.

Siehe auch: [Kapitel 30: Monitoring & Observability](#chapter-30) für erweiterte Monitoring-Strategien, [Kapitel 36: Security Best Practices](#chapter-36) für Security-Hardening, [Kapitel 38: Testing Strategies](#chapter-38) für umfassende Test-Automation, [Kapitel 39: Deployment Strategies](#chapter-39) für Advanced-Deployment-Patterns und [Kapitel 40: Compliance & Governance](#chapter-40) für regulatorische Anforderungen.

---

## Wissenschaftliche Referenzen {#references}

Die Konzepte und Best Practices in diesem Kapitel basieren auf wissenschaftlicher Forschung, Industry-Standards und bewährten Patterns aus Production-Deployments:

1. **Humble, Jez; Farley, David (2010).** *Continuous Delivery: Reliable Software Releases through Build, Test, and Deployment Automation.* Addison-Wesley Professional. ISBN 978-0321601919.
   - Definitive Referenz für CI/CD-Patterns und Deployment-Pipelines
   - Behandelt Automated Testing, Deployment Automation und Release Management

2. **Morris, Kief (2020).** *Infrastructure as Code: Dynamic Systems for the Cloud Age (2nd Edition).* O'Reilly Media. ISBN 978-1098114671.
   - Comprehensive Guide zu IaC-Patterns mit Terraform, Pulumi und CloudFormation
   - Best Practices für State Management, Testing und Module-Reusability

3. **Kim, Gene; Humble, Jez; Debois, Patrick; Willis, John (2016).** *The DevOps Handbook: How to Create World-Class Agility, Reliability, and Security in Technology Organizations.* IT Revolution Press. ISBN 978-1942788003.
   - Foundational Work zu DevOps-Culture, Practices und Organizational Change
   - Case Studies von High-Performing Organizations (Google, Amazon, Netflix)

4. **Beyer, Betsy; Jones, Chris; Petoff, Jennifer; Murphy, Niall Richard (2016).** *Site Reliability Engineering: How Google Runs Production Systems.* O'Reilly Media. ISBN 978-1491929124.
   - Google's SRE Practices: Monitoring, Incident Response, Capacity Planning
   - SLO/SLI Definitions, Error Budgets und Toil-Reduction-Strategies

5. **Burns, Brendan; Beda, Joe; Hightower, Kelsey; Evenson, Lachlan (2022).** *Kubernetes: Up and Running (3rd Edition).* O'Reilly Media. ISBN 978-1098110208.
   - Comprehensive Kubernetes Reference mit StatefulSets, Operators und Helm
   - Production Best Practices für Container Orchestration

6. **HashiCorp Terraform Documentation (2024).** *Terraform Best Practices Guide.* https://developer.hashicorp.com/terraform/tutorials
   - Official Terraform Documentation mit Module-Patterns und State Management
   - Provider-specific Best Practices für AWS, Azure, GCP

7. **Weaveworks GitOps Working Group (2023).** *OpenGitOps Principles v1.0.0.* https://opengitops.dev/
   - Open Standard für GitOps-Workflows und Tooling
   - ArgoCD und Flux CD Implementation Patterns

8. **Prometheus Authors (2024).** *Prometheus Documentation - Best Practices.* https://prometheus.io/docs/practices/
   - Official Prometheus Guide zu Metrics-Naming, Recording Rules und Alerting
   - Performance Tuning und High-Availability Setup

9. **Grafana Labs (2024).** *Grafana Loki Documentation.* https://grafana.com/docs/loki/latest/
   - Loki Architecture, LogQL Query Language und Best Practices
   - Cost-Efficient Log Aggregation im Vergleich zu ELK Stack

10. **CNCF (Cloud Native Computing Foundation) (2023).** *Cloud Native Trail Map.* https://github.com/cncf/trailmap
    - Roadmap für Cloud-Native-Adoption mit Tool-Recommendations
    - Best Practices für Containerization, Service Mesh, Observability

Vollständige Literaturliste und erweiterte Referenzen: [Anhang A: Literaturverzeichnis](appendix_literatur.md)

## 25.11 Updates-Modul — Zero-Downtime Cluster-Updates (v1.x) {#updates-module}

Das Updates-Modul (`include/updates/`) ermöglicht koordinierte Rolling-Updates, Canary-Rollouts und Zero-Downtime-Schema-Migration im laufenden Cluster.

### 25.11.1 ClusterUpdateManager

```cpp
#include "updates/cluster_update_manager.h"

themis::updates::ClusterUpdateManager::Config cfg;
cfg.rollback_on_failure = true;
cfg.max_parallel_nodes  = 2;        // Rolling: max. 2 Nodes gleichzeitig
cfg.health_check_timeout_ms = 30000;

themis::updates::ClusterUpdateManager mgr(cfg);

// Callback-Hooks
mgr.setNodeUpdateFunc([](const std::string& node_id,
                          const std::string& target_version) -> bool {
    // Download + Install auf node_id
    return true;
});

mgr.setNodeHealthCheckFunc([](const std::string& node_id) -> bool {
    return ping_node(node_id);
});

mgr.setNodeRollbackFunc([](const std::string& node_id,
                             const std::string& prev_version) -> bool {
    return rollback_node(node_id, prev_version);
});

mgr.setProgressCallback([](const themis::updates::ClusterUpdateProgress& p) {
    std::cout << p.completed_nodes << "/" << p.total_nodes << " abgeschlossen\n";
});

// Update starten
std::vector<themis::updates::ClusterNode> nodes = getClusterNodes();
auto result = mgr.update(nodes, "v1.9.1");
// result.success, result.rolled_back_nodes, result.failed_nodes
```

**ClusterNodeState:**

| Zustand | Bedeutung |
|---------|-----------|
| `PENDING` | Noch nicht gestartet |
| `DRAINING` | Verbindungen werden abgebaut |
| `APPLYING` | Update wird installiert |
| `HEALTH_CHECK` | Gesundheitscheck nach Update |
| `REJOINING` | Node tritt Cluster wieder bei |
| `COMPLETED` | Erfolgreich aktualisiert |
| `FAILED` | Update fehlgeschlagen |
| `ROLLED_BACK` | Auf Vorgängerversion zurückgesetzt |
| `SKIPPED` | Bereits auf Zielversion |

### 25.11.2 Canary-Rollout

```cpp
#include "updates/canary_rollout.h"

themis::updates::CanaryRolloutConfig ccfg;
ccfg.canary_percentage  = 10;   // 10 % des Traffics auf Canary
ccfg.bake_time_minutes  = 30;   // Wartezeit vor Erweiterung
ccfg.error_threshold    = 0.01; // Max. 1 % Fehlerrate

themis::updates::CanaryRollout canary(cluster, ccfg);
canary.deploy("v1.9.1");

// Automatische Promotion oder Rollback nach bake_time
canary.promote();   // manuell: 100 % Traffic umschalten
// oder
canary.rollback();  // zurück auf stabile Version
```

### 25.11.3 HotReloadEngine — Live-Konfiguration

```cpp
#include "updates/hot_reload_engine.h"

themis::updates::HotReloadEngine reload(server_context);

// Konfiguration live neu laden (ohne Neustart)
reload.reloadConfig("/etc/themis/themis.yaml");

// Plugin live tauschen
reload.reloadPlugin("auth_plugin", "/plugins/auth_v2.so");

// Callbacks bei Reload
reload.onConfigReloaded([](const std::string& path) {
    log("Config reloaded: " + path);
});
```

### 25.11.4 InPlaceSchemaMigrator

```cpp
#include "updates/in_place_schema_migrator.h"

themis::updates::InPlaceSchemaMigrator migrator(db);

// Schema-Migration ohne Downtime (online)
migrator.addColumn("orders", "discount_pct", "DOUBLE DEFAULT 0.0");
migrator.renameColumn("orders", "total", "total_amount");
migrator.dropColumn("orders", "legacy_field");

// Migration mit Validierung und Rollback-Plan
auto plan = migrator.plan();
auto mresult = migrator.execute(plan);
// mresult.success, mresult.migrated_rows, mresult.elapsed_ms
```

### 25.11.5 BlueGreenDeployment

```cpp
#include "updates/blue_green_deployment.h"

themis::updates::BlueGreenDeployment bg(load_balancer, db_pool);

// Blue-Umgebung ist aktiv, Green wird vorbereitet
bg.prepareGreen("v1.9.1");
bg.runSmokeTests();         // Tests auf Green
bg.cutover();               // Traffic von Blue → Green
// bg.rollback();           // zurück zu Blue (instant)
```
