# Kapitel 20: Backup & Recovery

## Einführung

Backup und Recovery sind kritische Komponenten für jede Produktionsumgebung. Dieses Kapitel behandelt umfassende Strategien für Datensicherung, Wiederherstellung und Disaster Recovery mit ThemisDB.

## 19.1 Backup-Strategien

### 19.1.1 Vollständige Backups

**Konzept:**
Ein vollständiges Backup erstellt eine komplette Kopie aller Daten zu einem bestimmten Zeitpunkt.

**Durchführung:**
```bash
# Online-Backup erstellen
themisdb-backup --type full --output /backups/full_$(date +%Y%m%d).tar.gz

# Mit Kompression
themisdb-backup --type full --compress gzip --output /backups/full.tar.gz
```

**Vorteile:**
- Einfache Wiederherstellung
- Vollständige Datenkonsistenz
- Einfaches Backup-Management

**Nachteile:**
- Hoher Speicherbedarf
- Längere Backup-Dauer
- Höhere I/O-Last

### 19.1.2 Inkrementelle Backups

**Konzept:**
Nur Änderungen seit dem letzten Backup werden gesichert.

**Implementierung:**
```bash
# Erstes vollständiges Backup
themisdb-backup --type full --output /backups/base.tar.gz

# Inkrementelle Backups
themisdb-backup --type incremental \
  --base /backups/base.tar.gz \
  --output /backups/incr_$(date +%Y%m%d_%H%M).tar.gz
```

**Vorteile:**
- Schnellere Backup-Erstellung
- Geringerer Speicherbedarf
- Weniger I/O-Last

**Nachteile:**
- Komplexere Wiederherstellung
- Abhängigkeit von vorherigen Backups

### 19.1.3 Differenzielle Backups

**Konzept:**
Alle Änderungen seit dem letzten vollständigen Backup.

```bash
# Differenzielles Backup
themisdb-backup --type differential \
  --base /backups/full.tar.gz \
  --output /backups/diff_$(date +%Y%m%d).tar.gz
```

```mermaid
gantt
    title Backup Strategy Timeline
    dateFormat YYYY-MM-DD
    
    section Full Backups
    Full Backup 1           :done, full1, 2024-01-01, 1d
    Full Backup 2           :done, full2, 2024-01-08, 1d
    Full Backup 3           :active, full3, 2024-01-15, 1d
    
    section Incremental
    Incremental Day 2       :done, inc1, 2024-01-02, 1d
    Incremental Day 3       :done, inc2, 2024-01-03, 1d
    Incremental Day 4       :done, inc3, 2024-01-04, 1d
    Incremental Day 5       :done, inc4, 2024-01-05, 1d
    Incremental Day 6       :done, inc5, 2024-01-06, 1d
    Incremental Day 7       :done, inc6, 2024-01-07, 1d
    
    section Differential
    Differential Day 2-7    :done, diff1, 2024-01-09, 6d
```

Abb. 20.1: Backup-Strategy-Overview

```mermaid
flowchart LR
    subgraph "Backup Flow"
        Data[(Production Data)] --> Check{Backup Type?}
        
        Check -->|Full| Full[Full Backup<br/>All Data]
        Check -->|Incremental| Inc[Incremental<br/>Changes since last backup]
        Check -->|Differential| Diff[Differential<br/>Changes since last full]
        
        Full --> Compress[Compress<br/>gzip/zstd]
        Inc --> Compress
        Diff --> Compress
        
        Compress --> Encrypt[Encrypt<br/>AES-256]
        Encrypt --> Storage[(Backup Storage<br/>S3/Local/Network)]
        
        Storage --> Verify[Verify<br/>Checksum]
        Verify --> Retention{Retention Policy}
        
        Retention -->|Keep| Archive[Archive Storage]
        Retention -->|Delete| Remove[Delete Old Backups]
    end
    
    style Data fill:#667eea
    style Full fill:#43e97b
    style Inc fill:#4facfe
    style Diff fill:#f093fb
    style Storage fill:#ffd32a
    style Archive fill:#95e1d3
```

Abb. 20.2: Point-in-Time-Recovery-Timeline

## 19.2 Backup-Mechanismen

### 19.2.1 Physische Backups

**RocksDB SST-Dateien:**
```python
import shutil
from pathlib import Path

def physical_backup(data_dir, backup_dir):
    """Erstellt physisches Backup der RocksDB-Dateien"""
    # Checkpoint erstellen (konsistenter Snapshot)
    checkpoint_dir = Path(backup_dir) / "checkpoint"
    
    # ThemisDB Checkpoint API
    client.admin.create_checkpoint(str(checkpoint_dir))
    
    # Backup mit rsync
    import subprocess
    subprocess.run([
        "rsync", "-av", "--delete",
        str(checkpoint_dir) + "/",
        str(backup_dir) + "/"
    ])
    
    print(f"Physical backup completed: {backup_dir}")
```

**Vorteile:**
- Sehr schnelle Backups
- Minimale Auswirkungen auf die Performance
- Byte-genaue Kopien

### 19.2.2 Logische Backups

**Datenexport:**
```python
def logical_backup(client, backup_file):
    """Exportiert Daten als logisches Backup"""
    import json
    
    backup_data = {
        'version': '1.3.4',
        'timestamp': datetime.now().isoformat(),
        'collections': {}
    }
    
    # Alle Collections exportieren
    collections = client.list_collections()
    for coll_name in collections:
        print(f"Backing up collection: {coll_name}")
        
        # Alle Dokumente exportieren
        docs = list(client.collection(coll_name).find({}))
        backup_data['collections'][coll_name] = docs
    
    # Als JSON speichern
    with open(backup_file, 'w') as f:
        json.dump(backup_data, f, indent=2)
    
    print(f"Logical backup completed: {backup_file}")
```

**Vorteile:**
- Plattformunabhängig
- Lesbare Datenformate
- Einfache Teilwiederherstellung

### 19.2.3 Snapshot-basierte Backups

**Mit Dateisystem-Snapshots:**
```bash
# LVM Snapshot
lvcreate --snapshot --name themisdb_snap --size 10G /dev/vg0/themisdb

# Backup vom Snapshot
tar czf /backups/snapshot_$(date +%Y%m%d).tar.gz \
  /mnt/themisdb_snap/

# Snapshot entfernen
lvremove /dev/vg0/themisdb_snap
```

**AWS EBS Snapshots:**
```python
import boto3

def create_ebs_snapshot(volume_id, description):
    """Erstellt EBS Snapshot"""
    ec2 = boto3.client('ec2')
    
    snapshot = ec2.create_snapshot(
        VolumeId=volume_id,
        Description=description,
        TagSpecifications=[{
            'ResourceType': 'snapshot',
            'Tags': [
                {'Key': 'Name', 'Value': f'themisdb-backup-{datetime.now().strftime("%Y%m%d")}'},
                {'Key': 'Type', 'Value': 'automated'}
            ]
        }]
    )
    
    return snapshot['SnapshotId']
```

## 19.3 Point-in-Time Recovery (PITR)

### 19.3.1 Binlog-basierte Recovery

**Konzept:**
Kombination aus vollständigem Backup und Transaction Logs.

**Aktivierung:**
```yaml
# themisdb.yaml
storage:
  enable_binlog: true
  binlog_dir: /var/lib/themisdb/binlog
  binlog_rotation: 100MB
  binlog_retention: 7d
```

**Recovery-Prozess:**
```python
def point_in_time_recovery(backup_file, target_time):
    """
    Wiederherstellung zu einem bestimmten Zeitpunkt
    """
    # 1. Basis-Backup wiederherstellen
    restore_backup(backup_file)
    
    # 2. Binlogs bis zum Ziel-Zeitpunkt anwenden
    binlog_dir = Path("/var/lib/themisdb/binlog")
    
    for binlog in sorted(binlog_dir.glob("binlog.*")):
        # Binlog-Einträge bis target_time anwenden
        apply_binlog(binlog, target_time)
    
    print(f"PITR completed to: {target_time}")
```

### 19.3.2 MVCC-basierte Recovery

**Mit Snapshot Isolation:**
```python
def recover_to_timestamp(timestamp):
    """
    Nutzt MVCC für zeitpunktgenaue Wiederherstellung
    """
    # Query mit AS OF SYSTEM TIME
    result = client.query(f"""
        SELECT * FROM orders
        AS OF SYSTEM TIME '{timestamp}'
    """)
    
    return result
```

## 19.4 Backup-Automatisierung

### 19.4.1 Cron-basierte Backups

**Backup-Script:**
```bash
#!/bin/bash
# /usr/local/bin/themisdb-backup.sh

BACKUP_DIR=/backups/themisdb
DATE=$(date +%Y%m%d_%H%M)
LOG_FILE=/var/log/themisdb-backup.log

# Vollständiges Backup jeden Sonntag
if [ $(date +%u) -eq 7 ]; then
    echo "[$(date)] Starting full backup" >> $LOG_FILE
    themisdb-backup --type full --output $BACKUP_DIR/full_$DATE.tar.gz
else
    echo "[$(date)] Starting incremental backup" >> $LOG_FILE
    themisdb-backup --type incremental --output $BACKUP_DIR/incr_$DATE.tar.gz
fi

# Alte Backups löschen (>30 Tage)
find $BACKUP_DIR -name "*.tar.gz" -mtime +30 -delete

echo "[$(date)] Backup completed" >> $LOG_FILE
```

**Crontab-Eintrag:**
```cron
# Tägliches Backup um 2:00 Uhr
0 2 * * * /usr/local/bin/themisdb-backup.sh

# Wöchentliches vollständiges Backup (Sonntag 3:00)
0 3 * * 0 /usr/local/bin/themisdb-full-backup.sh
```

### 19.4.2 Python Backup-Manager

Das folgende Python-Skript implementiert einen vollautomatischen Backup-Manager mit Schedule-Support. Es unterscheidet zwischen vollständigen und inkrementellen Backups und führt automatisch Retention-Management durch (löscht alte Backups nach 30 Tagen).

📁 **Vollständiger Code:** `examples/20_backup/backup_manager.py` (~95 Zeilen)

```python
import schedule
import time
from datetime import datetime, timedelta
from pathlib import Path

class BackupManager:
    def __init__(self, backup_dir, retention_days=30):
        self.backup_dir = Path(backup_dir)
        self.retention_days = retention_days
        
    def full_backup(self):
        """Vollständiges Backup aller Daten"""
        timestamp = datetime.now().strftime("%Y%m%d_%H%M")
        backup_file = self.backup_dir / f"full_{timestamp}.tar.gz"
        print(f"Starting full backup: {backup_file}")
        # Backup-Logik (tar/gzip Kompression)
        
    def incremental_backup(self):
        """Inkrementelles Backup nur geänderter Daten"""
        timestamp = datetime.now().strftime("%Y%m%d_%H%M")
        backup_file = self.backup_dir / f"incr_{timestamp}.tar.gz"
        print(f"Starting incremental backup: {backup_file}")
        
    def cleanup_old_backups(self):
        """Löscht Backups älter als retention_days"""
        cutoff = datetime.now() - timedelta(days=self.retention_days)
        for backup in self.backup_dir.glob("*.tar.gz"):
            if datetime.fromtimestamp(backup.stat().st_mtime) < cutoff:
                backup.unlink()
                print(f"Deleted old backup: {backup}")
    
    def start_scheduler(self):
        """Startet automatischen Backup-Scheduler"""
        schedule.every().day.at("02:00").do(self.incremental_backup)
        schedule.every().sunday.at("03:00").do(self.full_backup)
        schedule.every().day.at("04:00").do(self.cleanup_old_backups)
        
        while True:
            schedule.run_pending()
            time.sleep(60)

# Usage
manager = BackupManager("/backups/themisdb", retention_days=30)
manager.start_scheduler()
```

**Zusätzliche Features im vollständigen Skript:**
- Cloud-Upload zu S3/Azure Blob Storage
- Verschlüsselung mit GPG
- Email-Benachrichtigungen bei Fehlern
- Prometheus-Metriken für Monitoring

## 19.5 Backup-Verifizierung

### 19.5.1 Backup-Integrität prüfen

**Checksummen-Validierung:**
```python
import hashlib

def verify_backup(backup_file):
    """Überprüft Backup-Integrität"""
    # Checksumme berechnen
    sha256 = hashlib.sha256()
    
    with open(backup_file, 'rb') as f:
        while chunk := f.read(8192):
            sha256.update(chunk)
    
    checksum = sha256.hexdigest()
    
    # Mit gespeicherter Checksumme vergleichen
    checksum_file = f"{backup_file}.sha256"
    if Path(checksum_file).exists():
        with open(checksum_file) as f:
            expected = f.read().strip()
            
        if checksum == expected:
            print(f"✓ Backup integrity verified: {backup_file}")
            return True
        else:
            print(f"✗ Backup corrupted: {backup_file}")
            return False
    else:
        # Checksumme speichern
        with open(checksum_file, 'w') as f:
            f.write(checksum)
        return True
```

### 19.5.2 Wiederherstellungs-Tests

**Automatisierte Test-Recovery:**
```python
def test_backup_restore(backup_file, test_dir="/tmp/restore_test"):
    """Testet Backup-Wiederherstellung"""
    import shutil
    from pathlib import Path
    
    test_path = Path(test_dir)
    test_path.mkdir(exist_ok=True)
    
    try:
        # Backup wiederherstellen
        restore_backup(backup_file, test_path)
        
        # ThemisDB mit Test-Daten starten
        test_client = ThemisDBClient(data_dir=test_path)
        
        # Basis-Tests durchführen
        collections = test_client.list_collections()
        assert len(collections) > 0, "No collections found"
        
        # Sample-Queries testen
        result = test_client.query("SELECT COUNT(*) FROM orders")
        assert result[0]['count'] > 0, "No data found"
        
        print(f"✓ Restore test passed: {backup_file}")
        return True
        
    except Exception as e:
        print(f"✗ Restore test failed: {e}")
        return False
        
    finally:
        # Cleanup
        shutil.rmtree(test_path, ignore_errors=True)
```

## 19.6 Wiederherstellung

### 19.6.1 Vollständige Wiederherstellung

**Restore-Prozess:**
```bash
#!/bin/bash
# Komplette Datenbank-Wiederherstellung

# 1. ThemisDB stoppen
systemctl stop themisdb

# 2. Datenverzeichnis sichern (falls vorhanden)
mv /var/lib/themisdb /var/lib/themisdb.old

# 3. Backup wiederherstellen
tar xzf /backups/full_20231215.tar.gz -C /var/lib/themisdb

# 4. Berechtigungen setzen
chown -R themisdb:themisdb /var/lib/themisdb

# 5. ThemisDB starten
systemctl start themisdb

# 6. Validierung
themisdb-admin validate
```

### 19.6.2 Selektive Wiederherstellung

**Einzelne Collection wiederherstellen:**
```python
def restore_collection(backup_file, collection_name, target_db):
    """Stellt einzelne Collection wieder her"""
    import json
    
    # Backup laden
    with open(backup_file) as f:
        backup_data = json.load(f)
    
    # Collection-Daten extrahieren
    if collection_name not in backup_data['collections']:
        raise ValueError(f"Collection {collection_name} not found in backup")
    
    docs = backup_data['collections'][collection_name]
    
    # In Ziel-DB einfügen
    target_client = ThemisDBClient(target_db)
    coll = target_client.collection(collection_name)
    
    # Batch-Insert
    batch_size = 1000
    for i in range(0, len(docs), batch_size):
        batch = docs[i:i+batch_size]
        coll.insert_many(batch)
    
    print(f"Restored {len(docs)} documents to {collection_name}")
```

## 19.7 Disaster Recovery

### 19.7.1 DR-Strategie

**Recovery Time Objective (RTO):**
- Maximale akzeptable Ausfallzeit
- Ziel: < 4 Stunden

**Recovery Point Objective (RPO):**
- Maximaler Datenverlust
- Ziel: < 15 Minuten

**DR-Plan:**
```yaml
disaster_recovery:
  primary_site:
    location: "datacenter-1"
    backup_frequency: "hourly"
    
  secondary_site:
    location: "datacenter-2"
    replication: "synchronous"
    standby_mode: "hot"
    
  backup_locations:
    - "s3://backups-bucket/themisdb/"
    - "azure://backups/themisdb/"
    
  procedures:
    - name: "Site Failover"
      steps:
        - "Verify primary site failure"
        - "Promote secondary to primary"
        - "Update DNS/Load Balancer"
        - "Verify application connectivity"
```

### 19.7.2 Geo-Replikation

**Multi-Region Setup:**
```python
# Master in Region 1
master_config = {
    'replication': {
        'role': 'master',
        'replicas': [
            'themisdb-replica-eu:8529',
            'themisdb-replica-us:8529'
        ]
    }
}

# Replica in Region 2
replica_config = {
    'replication': {
        'role': 'replica',
        'master': 'themisdb-master-eu:8529',
        'replication_lag_alert': '30s'
    }
}
```

## 19.8 Backup zu Cloud-Storage

### 19.8.1 AWS S3

**Upload zu S3:**
```python
import boto3
from pathlib import Path

def upload_to_s3(backup_file, bucket, prefix="backups/"):
    """Lädt Backup zu AWS S3 hoch"""
    s3 = boto3.client('s3')
    
    key = f"{prefix}{Path(backup_file).name}"
    
    # Upload mit Progress
    s3.upload_file(
        backup_file,
        bucket,
        key,
        Callback=ProgressPercentage(backup_file)
    )
    
    print(f"Uploaded to s3://{bucket}/{key}")
    
    # Lifecycle-Policy für automatisches Löschen
    s3.put_bucket_lifecycle_configuration(
        Bucket=bucket,
        LifecycleConfiguration={
            'Rules': [{
                'Id': 'DeleteOldBackups',
                'Status': 'Enabled',
                'Prefix': prefix,
                'Expiration': {'Days': 90}
            }]
        }
    )
```

### 19.8.2 Azure Blob Storage

**Upload zu Azure:**
```python
from azure.storage.blob import BlobServiceClient

def upload_to_azure(backup_file, connection_string, container):
    """Lädt Backup zu Azure Blob Storage"""
    blob_service = BlobServiceClient.from_connection_string(connection_string)
    
    blob_name = Path(backup_file).name
    blob_client = blob_service.get_blob_client(container, blob_name)
    
    with open(backup_file, 'rb') as data:
        blob_client.upload_blob(data, overwrite=True)
    
    print(f"Uploaded to Azure: {container}/{blob_name}")
```

### 19.8.3 Google Cloud Storage

**Upload zu GCS:**
```python
from google.cloud import storage

def upload_to_gcs(backup_file, bucket_name, destination_blob):
    """Lädt Backup zu Google Cloud Storage"""
    client = storage.Client()
    bucket = client.bucket(bucket_name)
    blob = bucket.blob(destination_blob)
    
    blob.upload_from_filename(backup_file)
    
    print(f"Uploaded to gs://{bucket_name}/{destination_blob}")
```

## 19.9 Best Practices

### 19.9.1 3-2-1 Backup-Regel

**Prinzip:**
- **3** Kopien der Daten
- **2** verschiedene Medientypen
- **1** Kopie off-site

**Implementierung:**
```python
backup_strategy = {
    'copies': [
        {'type': 'local', 'location': '/backups/local'},
        {'type': 'nas', 'location': '/mnt/nas/backups'},
        {'type': 'cloud', 'location': 's3://backups/themisdb'}
    ],
    'media_types': ['disk', 'cloud'],
    'offsite': True
}
```

### 19.9.2 Backup-Verschlüsselung

**Verschlüsselte Backups:**
```python
from cryptography.fernet import Fernet

def encrypted_backup(source_file, output_file, key_file):
    """Erstellt verschlüsseltes Backup"""
    # Key laden oder generieren
    if Path(key_file).exists():
        with open(key_file, 'rb') as f:
            key = f.read()
    else:
        key = Fernet.generate_key()
        with open(key_file, 'wb') as f:
            f.write(key)
    
    fernet = Fernet(key)
    
    # Daten verschlüsseln
    with open(source_file, 'rb') as f:
        data = f.read()
    
    encrypted = fernet.encrypt(data)
    
    with open(output_file, 'wb') as f:
        f.write(encrypted)
    
    print(f"Encrypted backup created: {output_file}")
```

### 19.9.3 Backup-Monitoring

**Monitoring-Integration:**
```python
from prometheus_client import Counter, Histogram, Gauge

backup_counter = Counter('themisdb_backups_total', 'Total backups', ['type', 'status'])
backup_duration = Histogram('themisdb_backup_duration_seconds', 'Backup duration')
backup_size = Gauge('themisdb_backup_size_bytes', 'Backup size')
last_backup = Gauge('themisdb_last_backup_timestamp', 'Last backup timestamp')

@backup_duration.time()
def monitored_backup(backup_type):
    """Backup mit Monitoring"""
    try:
        # Backup durchführen
        result = create_backup(backup_type)
        
        # Metriken aktualisieren
        backup_counter.labels(type=backup_type, status='success').inc()
        backup_size.set(result['size'])
        last_backup.set(time.time())
        
        return result
        
    except Exception as e:
        backup_counter.labels(type=backup_type, status='failure').inc()
        raise
```

## 19.10 Zusammenfassung

**Kritische Punkte:**
- Regelmäßige Backups sind essenziell
- PITR ermöglicht zeitpunktgenaue Recovery
- Automatisierung reduziert Fehler
- Cloud-Storage bietet Off-site-Schutz
- Regelmäßige Wiederherstellungs-Tests
- Verschlüsselung schützt sensible Daten
- Monitoring gewährleistet Backup-Erfolg

**Checkliste:**
- [ ] Backup-Strategie definiert
- [ ] Automatisierte Backups konfiguriert
- [ ] PITR aktiviert
- [ ] Off-site Backups eingerichtet
- [ ] Wiederherstellungs-Tests durchgeführt
- [ ] DR-Plan dokumentiert
- [ ] Monitoring aktiviert
- [ ] Team geschult

---

## 20.11 Erweiterte Storage & Recovery Services (v1.8.0)

<!-- Source: include/storage/ — backup_manager.h, pitr_manager.h, tiered_storage.h -->

> **Neu in v1.8.0** – Das Storage-Modul erweitert das klassische Backup-Konzept um drei hochintegrierte Komponenten: `BackupManager` mit RAID-Awareness, `PITRManager` für Git-artige Point-in-Time Recovery und `TieredStorageManager` für automatisches Hot→Warm→Cold-Lifecycle-Management.

### 20.11.1 BackupManager — RAID-aware Backup & Restore

`BackupManager` erkennt die RAID-Konfiguration der Cluster-Umgebung automatisch aus Umgebungsvariablen und koordiniert RAID-übergreifende Backup-Operationen.

**Unterstützte RAID-Modi:**

| RAID-Modus | Beschreibung | Redundanz |
|------------|-------------|-----------|
| `NONE` | Single-Node, keine Redundanz | 0 |
| `RAID0` | Striping (Performance) | 0 Parität |
| `RAID1` | Mirroring (volle Kopie) | N-1 Nodes |
| `RAID5` | Striping + verteilte Parität | 1 Node |
| `RAID6` | Striping + doppelte Parität | 2 Nodes |
| `RAID10` | Striping + Mirroring | N/2 Nodes |

**API-Nutzung:**

```cpp
#include "storage/backup_manager.h"

// BackupManager mit RocksDB-Instanz erstellen
auto manager = std::make_shared<BackupManager>(rocksdb_wrapper);

// RAID-Konfiguration erkennen (aus ENV: THEMIS_RAID_MODE, THEMIS_SHARD_*)
auto raid_cfg = manager->detectRAIDConfiguration();
// raid_cfg.mode, raid_cfg.shards, raid_cfg.data_shards, raid_cfg.parity_shards

// Vollständiges Backup
BackupOptions opts;
opts.destination    = "/backups/themisdb_2026-04-13";
opts.compress       = CompressionType::ZSTD;
opts.encrypt        = true;
opts.encryption_key = vault.getKey("backup-master");
opts.verify_after   = true;
opts.parallel_shards = 4;

auto result = manager->createBackup(opts);
// result.backup_id, result.size_bytes, result.duration_ms, result.checksum

// Scheduling: Backup alle 6 Stunden
BackupSchedule schedule;
schedule.interval_hours = 6;
schedule.retention_days = 30;
manager->setSchedule(schedule);
```

**RAID5-Koordination:**

```
Node 0 (data)   ──┐
Node 1 (data)   ──┤──► BackupManager::coordinatedBackup()
Node 2 (data)   ──┤         │
Node 3 (parity) ──┘         ▼
                    RAID5-übergreifendes Backup
                    (alle Shards synchron gesichert)
```

**Backup-Statusüberwachung:**

```aql
// Backup-Status in AQL abfragen
FOR backup IN _backups
  FILTER backup.created_at > DATE_SUBTRACT(DATE_NOW(), 1, "day")
  SORT backup.created_at DESC
  RETURN {
    id:       backup._id,
    size_mb:  ROUND(backup.size_bytes / 1048576, 2),
    compress: backup.compression,
    status:   backup.verify_status,
    duration: backup.duration_ms
  }
```

---

### 20.11.2 PITRManager — Point-in-Time Recovery (Git-artig)

`PITRManager` ermöglicht die Wiederherstellung der Datenbank zu einem beliebigen vergangenen Zustand über Sequence-Number, Named-Snapshot oder Timestamp. Die Implementierung basiert auf dem `Changefeed`-System: Änderungen werden rückwärts replayed (PUT→DELETE, DELETE→PUT).

**Recovery-Optionen:**

```cpp
#include "storage/pitr_manager.h"

PITRManager pitr(rocksdb_wrapper, snapshot_manager, changefeed);

// --- Option 1: Recovery zu einer Sequence-Nummer ---
PITRManager::RestoreOptions opts;
opts.target_sequence = 1_250_000;  // Gewünschte LSN
opts.dry_run         = true;        // Vorschau ohne Anwendung
opts.create_backup   = true;        // Auto-Backup vor Recovery
opts.progress_callback = [](size_t applied, size_t total) {
    printf("PITR: %zu/%zu Änderungen zurückgespielt\n", applied, total);
};

auto preview = pitr.restoreToSequence(opts);
// preview.events_to_replay, preview.estimated_duration_ms, preview.affected_tables

// Ohne dry_run: tatsächliche Wiederherstellung
opts.dry_run = false;
auto result = pitr.restoreToSequence(opts);

// --- Option 2: Recovery zu einem Named-Snapshot ---
auto snap_opts = PITRManager::RestoreOptions{};
snap_opts.target_snapshot = "before-migration-2026-04-01";
pitr.restoreToSnapshot(snap_opts);

// --- Option 3: Recovery zu einem Zeitstempel ---
auto ts_opts = PITRManager::RestoreOptions{};
ts_opts.target_timestamp = "2026-04-13T03:00:00Z";
pitr.restoreToTimestamp(ts_opts);

// --- Option 4: Selektive Wiederherstellung (nur bestimmte Tabellen) ---
auto sel_opts = PITRManager::RestoreOptions{};
sel_opts.target_sequence = 1_200_000;
sel_opts.tables_filter   = {"users", "orders"};  // Nur diese Tabellen
pitr.restoreToSequence(sel_opts);
```

**Sicherheitsmechanismen:**

| Feature | Beschreibung |
|---------|-------------|
| Auto-Backup vor Recovery | Vollständiges Backup vor jeder Wiederherstellung |
| Dry-Run-Modus | Detaillierte Vorschau ohne Datenmutation |
| Auto-Rollback | Automatischer Rückfall bei Fehlern |
| Progress-Tracking | Callback für lange Recovery-Operationen |
| Table-Filter | Selektive Recovery einzelner Collections |

**Anwendungsfälle:**

- **Disaster Recovery**: Wiederherstellung nach Datenbeschädigung
- **Schema-Migration-Rollback**: Rückgängigmachen fehlgeschlagener Migrationen
- **Compliance**: Wiederherstellung zu historischen Audit-Punkten
- **Testing**: Reset auf bekannt-guten Zustand

---

### 20.11.3 TieredStorageManager — Automatisches Hot→Warm→Cold-Lifecycle

`TieredStorageManager` migriert Daten automatisch zwischen drei Storage-Tiers basierend auf Alter und Zugriffsfrequenz.

**Tier-Architektur:**

```
Tier 1: HOT  (NVMe SSD)    ← Aktive Daten, < 30 Tage
    │    │
    │    └─ Migration: age > hot_to_warm_days ODER
    │                  zero reads > hot_zero_access_days
    ▼
Tier 2: WARM (SATA SSD)    ← Warme Daten, 30–90 Tage
    │    │
    │    └─ Migration: age > warm_to_cold_days ODER
    │                  zero reads > warm_zero_access_days
    ▼
Tier 3: COLD (S3/GCS/Azure) ← Archivdaten, > 90 Tage
```

**Konfiguration:**

```cpp
#include "storage/tiered_storage.h"

namespace ts = themis::storage;

ts::TieredStorageConfig cfg;
// Tier-Pfade
cfg.hot_tier_path  = "/nvme/data/hot";
cfg.warm_tier_path = "/sata/data/warm";
cfg.cold_tier_path = "s3://company-archive/themisdb/";

// Altersbasierte Migration
cfg.hot_to_warm_days  = 30;   // Hot→Warm nach 30 Tagen ohne Schreibzugriff
cfg.warm_to_cold_days = 90;   // Warm→Cold nach 90 Tagen

// Zugriffsfrequenz-basierte Migration
cfg.hot_zero_access_days  = 14;  // Hot→Warm ohne Lesezugriff für 14 Tage
cfg.warm_zero_access_days = 30;  // Warm→Cold ohne Lesezugriff für 30 Tage

// Migrations-Worker
cfg.migration_check_interval = std::chrono::hours(6);
cfg.migration_batch_size     = 1000;  // Keys pro Migration-Batch

ts::TieredStorageManager tiered(cfg);

// Daten lesen (transparente Tier-Auflösung)
auto value = tiered.get("user:42");

// Daten schreiben (immer in Hot-Tier)
tiered.put("user:42", serialized_user);

// Manuelle Migration erzwingen
tiered.migrateToTier("sensor-data:old-prefix", ts::StorageTierLevel::COLD);

// Status abfragen
auto stats = tiered.getStats();
// stats.hot_size_bytes, stats.warm_size_bytes, stats.cold_size_bytes
// stats.migrations_last_24h, stats.hot_access_rate
```

**Monitoring-Integration:**

```cpp
// Prometheus-Metriken werden automatisch gemeldet:
// themis_storage_tier_hot_bytes
// themis_storage_tier_warm_bytes
// themis_storage_tier_cold_bytes
// themis_storage_migrations_total{direction="hot_to_warm"}
// themis_storage_migrations_total{direction="warm_to_cold"}
```

### 20.11.4 Kosten-Nutzen-Analyse: Tiered Storage

| Szenario | Ohne Tiering | Mit Tiering | Kosteneinsparung |
|----------|-------------|------------|-----------------|
| 10 TB Gesamtdaten | 10 TB NVMe | 2 TB NVMe + 3 TB SATA + 5 TB S3 | ~70 % |
| 1 TB / Monat Wachstum | +1 TB NVMe/Monat | +50 GB NVMe + Rest archiviert | ~85 % |
| Compliance-Archivierung | Teuer on-premise | Günstig in Object-Storage | ~90 % |

### 20.11.5 Gesamtübersicht: Storage & Recovery (v1.8.0)

| Komponente | Funktion | Status | Performance |
|------------|---------|--------|-------------|
| `BackupManager` | RAID-aware Backup (RAID0/1/5/6/10) | ✅ Production-Ready | 500 MB/s (ZSTD) |
| `PITRManager` | Git-artige Point-in-Time Recovery | ✅ Production-Ready | < 1 min / 1M Events |
| `TieredStorageManager` | Automatisches Hot→Warm→Cold | ✅ Production-Ready | Transparent < 1 ms overhead |
