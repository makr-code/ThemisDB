> **Aktueller Build-Flow:** `cmake --preset linux-ninja-release && cmake --build --preset linux-ninja-release`

# DMS/ERP System - Administrator Guide

> **Historischer Stand:** 2026-01-31 — Inhalte nicht gegen aktuelle Quellen geprüft.

## Übersicht

Dieses Handbuch richtet sich an Systemadministratoren, die das DMS/ERP-System installieren, konfigurieren und betreiben.

## System-Anforderungen

### Hardware

**Minimum**:
- CPU: 4 Cores
- RAM: 8 GB
- Disk: 50 GB SSD
- Netzwerk: 100 Mbps

**Empfohlen**:
- CPU: 8+ Cores
- RAM: 16+ GB
- Disk: 200+ GB NVMe SSD
- Netzwerk: 1 Gbps

### Software

**Erforderlich**:
- Python 3.8+
- ThemisDB Server 1.3.0+
- Docker (optional, empfohlen)

**Optional**:
- Nginx (Reverse Proxy)
- PostgreSQL (externes Reporting)
- Redis (Session Cache)

## Installation

### Docker-Deployment (Empfohlen)

```bash
# 1. Repository klonen
git clone https://github.com/makr-code/ThemisDB.git
cd ThemisDB/examples/08_dms_erp_system

# 2. Docker Compose starten
docker-compose up -d

# 3. Status prüfen
docker-compose ps

# 4. Logs anzeigen
docker-compose logs -f
```

**docker-compose.yml**:
```yaml
version: '3.8'

services:
  themisdb:
    image: themisdb/themisdb:latest
    ports:
      - "8080:8080"
      - "18765:18765"
    volumes:
      - themisdb_data:/data
    environment:
      - THEMIS_LOG_LEVEL=INFO
      - THEMIS_MAX_CONNECTIONS=100

  dms_erp:
    build: .
    ports:
      - "5000:5000"
    depends_on:
      - themisdb
    environment:
      - THEMIS_HOST=themisdb
      - THEMIS_PORT=8080
      - APP_SECRET_KEY=${APP_SECRET_KEY}
      - ADMIN_PASSWORD=${ADMIN_PASSWORD}
    volumes:
      - ./uploads:/app/uploads
      - ./logs:/app/logs

volumes:
  themisdb_data:
```

### Manuelle Installation

```bash
# 1. ThemisDB installieren
docker run -d -p 8080:8080 themisdb/themisdb:latest

# 2. Python-Dependencies
pip install -r requirements.txt

# 3. Konfiguration erstellen
cp config.example.yaml config.yaml
vim config.yaml

# 4. Datenbank initialisieren
python scripts/init_database.py

# 5. Admin-User erstellen
python scripts/create_admin.py

# 6. Anwendung starten
python main.py
```

## Konfiguration

### Haupt-Konfiguration

**config.yaml**:
```yaml
# Datenbank
database:
  host: localhost
  port: 8080
  timeout: 30
  max_retries: 3
  pool_size: 10

# Anwendung
application:
  host: 0.0.0.0
  port: 5000
  debug: false
  secret_key: ${APP_SECRET_KEY}  # Aus Environment

# Security
security:
  session_timeout: 3600  # 1 Stunde
  max_login_attempts: 5
  password_min_length: 12
  require_2fa: false
  allowed_file_extensions:
    - pdf
    - docx
    - xlsx
    - png
    - jpg
  max_file_size_mb: 50

# Audit
audit:
  enabled: true
  log_all_reads: false
  log_all_writes: true
  retention_days: 365

# Email (für Benachrichtigungen)
email:
  smtp_host: smtp.example.com
  smtp_port: 587
  smtp_user: ${SMTP_USER}
  smtp_password: ${SMTP_PASSWORD}
  from_address: noreply@example.com

# Storage
storage:
  type: local  # local, s3, azure
  local_path: ./uploads
  # s3_bucket: my-bucket
  # s3_region: eu-central-1
```

### Environment Variables

```bash
# .env Datei
APP_SECRET_KEY=<generate-with-openssl-rand-hex-32>
ADMIN_PASSWORD=<secure-password>
SMTP_USER=noreply@example.com
SMTP_PASSWORD=<smtp-password>

# S3 (optional)
AWS_ACCESS_KEY_ID=<key>
AWS_SECRET_ACCESS_KEY=<secret>
```

**Secret Key generieren**:
```bash
openssl rand -hex 32
```

## Benutzerverwaltung

### Admin-User erstellen

```bash
python scripts/create_admin.py \
  --username admin \
  --email admin@example.com \
  --password <secure-password>
```

### Benutzer-Rollen

**SUPER_ADMIN**:
- Volle System-Kontrolle
- User-Management
- System-Konfiguration

**ADMIN**:
- DMS/ERP-Management
- Workflow-Konfiguration
- Reporting

**USER**:
- Dokumente erstellen/bearbeiten
- Workflows ausführen

**VIEWER**:
- Nur Lesezugriff

### Rollen zuweisen

```python
# Via Script
python scripts/assign_role.py \
  --user user@example.com \
  --role ADMIN

# Via API
POST /api/admin/users/{user_id}/roles
{
  "role": "ADMIN"
}
```

## Backup und Recovery

### Backup-Strategie

**Vollbackup** (täglich, 3 AM):
```bash
#!/bin/bash
# backup_full.sh

DATE=$(date +%Y%m%d_%H%M%S)
BACKUP_DIR=/backups/full
THEMIS_DIR=/data/themisdb
UPLOADS_DIR=/app/uploads

# ThemisDB Backup
docker exec themisdb themisdb-backup \
  --output $BACKUP_DIR/themisdb_$DATE.bak

# Uploads Backup
tar -czf $BACKUP_DIR/uploads_$DATE.tar.gz $UPLOADS_DIR

# Retention: 30 Tage
find $BACKUP_DIR -name "*.bak" -mtime +30 -delete
find $BACKUP_DIR -name "*.tar.gz" -mtime +30 -delete
```

**Cron-Job**:
```bash
0 3 * * * /opt/dms_erp/scripts/backup_full.sh
```

### Inkrementelles Backup

```bash
#!/bin/bash
# backup_incremental.sh

DATE=$(date +%Y%m%d_%H%M%S)
BACKUP_DIR=/backups/incremental

docker exec themisdb themisdb-backup \
  --output $BACKUP_DIR/themisdb_incr_$DATE.bak \
  --incremental

# Retention: 7 Tage
find $BACKUP_DIR -name "*.bak" -mtime +7 -delete
```

**Cron-Job** (jede Stunde):
```bash
0 * * * * /opt/dms_erp/scripts/backup_incremental.sh
```

### Recovery

**Vollständige Wiederherstellung**:
```bash
# 1. System stoppen
docker-compose down

# 2. ThemisDB wiederherstellen
docker run --rm -v themisdb_data:/data \
  themisdb/themisdb:latest \
  themisdb-restore --input /backups/full/themisdb_20250122_030000.bak

# 3. Uploads wiederherstellen
tar -xzf /backups/full/uploads_20250122_030000.tar.gz -C /

# 4. System starten
docker-compose up -d

# 5. Integrität prüfen
python scripts/verify_database.py
```

**Point-in-Time Recovery**:
```bash
# 1. Restore Vollbackup
themisdb-restore --input full_backup.bak

# 2. Apply Incremental Backups
for backup in incr_*.bak; do
  themisdb-restore --input $backup --incremental
done
```

## Monitoring

### Health Checks

**Application Health**:
```bash
curl http://localhost:5000/health
```

**Response**:
```json
{
  "status": "healthy",
  "database": "connected",
  "uptime": 86400,
  "version": "1.0.0"
}
```

**ThemisDB Health**:
```bash
curl http://localhost:8080/health
```

### Metriken

**Prometheus Endpoint**:
```yaml
# prometheus.yml
scrape_configs:
  - job_name: 'dms_erp'
    static_configs:
      - targets: ['localhost:5000']
    metrics_path: '/metrics'
```

**Verfügbare Metriken**:
- `dms_requests_total` - Gesamt-Requests
- `dms_request_duration_seconds` - Request-Dauer
- `dms_documents_total` - Anzahl Dokumente
- `dms_workflows_active` - Aktive Workflows
- `dms_storage_bytes` - Speichernutzung

### Log-Monitoring

**Logs aggregieren**:
```bash
# Zentrales Logging mit Loki
docker-compose logs -f | promtail --config=promtail.yaml
```

**Log-Levels**:
- ERROR: Fehler die Aktion erfordern
- WARN: Potentielle Probleme
- INFO: Wichtige Events
- DEBUG: Detaillierte Informationen

### Alerting

**Beispiel-Alerts** (AlertManager):
```yaml
groups:
  - name: dms_erp
    rules:
      - alert: HighErrorRate
        expr: rate(dms_errors_total[5m]) > 0.1
        for: 5m
        annotations:
          summary: "Hohe Fehlerrate in DMS/ERP"
      
      - alert: DatabaseDown
        expr: dms_database_status != 1
        for: 1m
        annotations:
          summary: "ThemisDB nicht erreichbar"
      
      - alert: DiskSpaceLow
        expr: dms_storage_bytes / dms_storage_capacity > 0.9
        for: 10m
        annotations:
          summary: "Speicherplatz unter 10%"
```

## Performance-Tuning

### Datenbank-Optimierung

**ThemisDB Indizes**:
```sql
-- Dokumente
CREATE INDEX idx_documents_status ON documents(status);
CREATE INDEX idx_documents_created ON documents(created_at DESC);
CREATE INDEX idx_documents_type ON documents(document_type);

-- Workflows
CREATE INDEX idx_workflows_status ON workflows(status);
CREATE INDEX idx_workflows_assigned ON workflows(assigned_to);

-- Audit-Logs
CREATE INDEX idx_audit_timestamp ON audit_logs(timestamp DESC);
CREATE INDEX idx_audit_entity ON audit_logs(entity_type, entity_id);
```

**Connection Pooling**:
```yaml
database:
  pool_size: 20
  pool_overflow: 10
  pool_timeout: 30
  pool_recycle: 3600
```

### Application-Tuning

**Caching**:
```yaml
cache:
  type: redis
  host: localhost
  port: 6379
  ttl: 3600
  
  # Cache-Strategien
  cache_documents: true
  cache_users: true
  cache_workflows: false  # Zu dynamisch
```

**Worker Threads**:
```yaml
application:
  workers: 4  # = CPU Cores
  threads_per_worker: 2
  worker_class: "gthread"
```

## Sicherheit

### SSL/TLS

**Nginx als Reverse Proxy**:
```nginx
server {
    listen 443 ssl http2;
    server_name dms.example.com;
    
    ssl_certificate /etc/ssl/certs/dms.crt;
    ssl_certificate_key /etc/ssl/private/dms.key;
    ssl_protocols TLSv1.2 TLSv1.3;
    ssl_ciphers HIGH:!aNULL:!MD5;
    
    location / {
        proxy_pass http://localhost:5000;
        proxy_set_header Host $host;
        proxy_set_header X-Real-IP $remote_addr;
        proxy_set_header X-Forwarded-For $proxy_add_x_forwarded_for;
        proxy_set_header X-Forwarded-Proto $scheme;
    }
}
```

### Firewall

```bash
# UFW Regeln
ufw allow 22/tcp    # SSH
ufw allow 443/tcp   # HTTPS
ufw deny 5000/tcp   # Block direct access
ufw enable
```

### Audit-Compliance

**GDPR-konforme Einstellungen**:
```yaml
audit:
  enabled: true
  log_personal_data: false
  anonymize_after_days: 90
  retention_days: 2555  # 7 Jahre
```

## Wartung

### Updates

**Anwendung aktualisieren**:
```bash
# 1. Backup erstellen
./scripts/backup_full.sh

# 2. Neue Version pullen
git pull origin main

# 3. Dependencies aktualisieren
pip install -r requirements.txt --upgrade

# 4. Migrations ausführen
python scripts/migrate.py

# 5. Neu starten
docker-compose restart dms_erp
```

**ThemisDB aktualisieren**:
```bash
# 1. Backup
docker exec themisdb themisdb-backup --output /backups/pre_upgrade.bak

# 2. Stop
docker-compose stop themisdb

# 3. Update
docker-compose pull themisdb

# 4. Start
docker-compose up -d themisdb

# 5. Verify
curl http://localhost:8080/health
```

### Datenpflege

**Alte Dokumente archivieren**:
```bash
python scripts/archive_old_documents.py --older-than-days 365
```

**Audit-Logs bereinigen**:
```bash
python scripts/cleanup_audit_logs.py --retention-days 365
```

## Troubleshooting

### Häufige Probleme

**Problem**: Anwendung startet nicht
```bash
# Logs prüfen
docker-compose logs dms_erp

# Config validieren
python scripts/validate_config.py

# Datenbank-Verbindung testen
python scripts/test_db_connection.py
```

**Problem**: Langsame Performance
```bash
# DB-Statistiken
python scripts/db_stats.py

# Slow Query Log
docker exec themisdb cat /var/log/themisdb/slow_queries.log

# Cache Status
redis-cli INFO stats
```

**Problem**: Speicher läuft voll
```bash
# Disk Usage
df -h

# Große Dateien finden
du -sh /app/uploads/* | sort -rh | head -20

# Bereinigung
python scripts/cleanup_old_files.py --dry-run
```

## Support

**Logs sammeln**:
```bash
./scripts/collect_support_logs.sh
# Erstellt: support_logs_20250122_150000.tar.gz
```

**System-Info**:
```bash
python scripts/system_info.py > system_info.txt
```

---

**Letzte Aktualisierung**: 2025-12-22
