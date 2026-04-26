# Operational Runbooks

**Version:** 1.8.0-rc1  
**Last Updated:** April 2026  
**Target Audience:** Operations Teams, SREs

## Table of Contents

1. [Training Job Submission](#training-job-submission)
2. [Model Checkpoint Management](#model-checkpoint-management)
3. [LoRA Adapter Deployment](#lora-adapter-deployment)
4. [Multi-Shard Inference Setup](#multi-shard-inference-setup)
5. [Emergency Procedures](#emergency-procedures)
6. [Maintenance Windows](#maintenance-windows)
7. [Backup and Recovery](#backup-and-recovery)
8. [Scaling Operations](#scaling-operations)
9. [Security Incident Response](#security-incident-response)
10. [Performance Tuning Workflow](#performance-tuning-workflow)

---

## Training Job Submission

### Standard Training Job

**Prerequisites:**
- GPU resources available
- Dataset prepared and validated
- Model configuration reviewed

**Procedure:**

```bash
# Step 1: Validate dataset
themisdb-cli validate dataset \
  --path /data/datasets/my-training-data \
  --format jsonl \
  --check-duplicates \
  --check-nan

# Expected output:
# ✓ Dataset valid: 10,000 samples
# ✓ No duplicates found
# ✓ No NaN values detected
# ✓ Average sequence length: 512 tokens

# Step 2: Review training configuration
cat > /etc/themisdb/jobs/train-job-001.yaml << 'EOF'
job:
  name: llama2-7b-finetune-001
  type: training
  priority: normal
  
model:
  base: llama-2-7b-chat
  output: llama-2-7b-custom-001
  
training:
  dataset: /data/datasets/my-training-data
  epochs: 3
  batch_size: 32
  learning_rate: 3e-4
  
  precision: fp16
  gradient_checkpointing: true
  
  checkpoint:
    save_interval: 500
    max_keep: 5
    output_dir: /data/checkpoints/train-job-001

gpu:
  devices: [0, 1]
  strategy: data_parallel
  
notifications:
  email: ops@example.com
  slack_webhook: https://hooks.slack.com/services/XXX
EOF

# Step 3: Dry run (validation only)
themisdb-cli job submit \
  --config /etc/themisdb/jobs/train-job-001.yaml \
  --dry-run

# Expected output:
# ✓ Configuration valid
# ✓ GPU resources available
# ✓ Dataset accessible
# Estimated duration: 4h 30m
# Estimated cost: $12.50 (compute time)

# Step 4: Submit job
themisdb-cli job submit \
  --config /etc/themisdb/jobs/train-job-001.yaml

# Expected output:
# Job submitted: train-job-001
# Job ID: job_abc123def456
# Status: QUEUED
# Position in queue: 1
# Estimated start time: 2026-01-17 16:00:00

# Step 5: Monitor job
themisdb-cli job status job_abc123def456

# Step 6: Stream logs
themisdb-cli job logs job_abc123def456 --follow
```

**Monitoring Checklist:**
- [ ] GPU utilization >85%
- [ ] Training loss decreasing
- [ ] No OOM errors
- [ ] Checkpoint saves completing
- [ ] Disk space sufficient

**Rollback Procedure:**
```bash
# Stop job
themisdb-cli job stop job_abc123def456

# Investigate
themisdb-cli job logs job_abc123def456 --last 100

# Restart with adjusted config
themisdb-cli job resubmit job_abc123def456 --config new-config.yaml
```

---

## Model Checkpoint Management

### Checkpoint Lifecycle

**Best Practices:**
- Keep 3-5 recent checkpoints
- Archive milestone checkpoints
- Validate before deleting
- Monitor disk usage

### Save Checkpoint Manually

```bash
# Save current training state
themisdb-cli checkpoint save \
  --job-id job_abc123def456 \
  --name manual-checkpoint-001 \
  --description "Before switching to BF16"

# Verify checkpoint
themisdb-cli checkpoint verify manual-checkpoint-001

# Expected output:
# ✓ Model weights: 7.1 GB
# ✓ Optimizer state: 14.2 GB
# ✓ Training metadata: present
# ✓ Checksum: valid
```

### Restore from Checkpoint

```bash
# List available checkpoints
themisdb-cli checkpoint list --job-id job_abc123def456

# Output:
# ID                    Name                    Step    Loss    Date
# ckpt_001             checkpoint-500          500     2.45    2026-01-17 10:00
# ckpt_002             checkpoint-1000         1000    1.89    2026-01-17 11:00
# ckpt_003             manual-checkpoint-001   1200    1.75    2026-01-17 11:30

# Restore from specific checkpoint
themisdb-cli job restore \
  --job-id job_abc123def456 \
  --checkpoint ckpt_003 \
  --resume

# Verify restoration
themisdb-cli job status job_abc123def456
```

### Checkpoint Cleanup

```bash
# Automatic cleanup (configured in job)
checkpoint:
  max_keep: 5
  retention_policy: keep_best
  cleanup_interval: 1000

# Manual cleanup
themisdb-cli checkpoint cleanup \
  --job-id job_abc123def456 \
  --keep 3 \
  --dry-run  # Preview what will be deleted

# Archive old checkpoints
themisdb-cli checkpoint archive \
  --checkpoint ckpt_001 \
  --destination /archive/checkpoints/ \
  --compress

# Delete archived checkpoint from active storage
themisdb-cli checkpoint delete ckpt_001 --force
```

### Checkpoint Export/Import

```bash
# Export checkpoint for external use
themisdb-cli checkpoint export \
  --checkpoint ckpt_003 \
  --format huggingface \
  --output /export/my-model/

# Import external checkpoint
themisdb-cli checkpoint import \
  --path /import/external-model/ \
  --format pytorch \
  --name imported-model-001
```

---

## LoRA Adapter Deployment

### Deploy New LoRA Adapter

**Prerequisites:**
- Base model loaded
- LoRA adapter trained and validated
- Resources allocated

**Procedure:**

```bash
# Step 1: Validate LoRA adapter
themisdb-cli lora validate \
  --adapter /models/lora/adapter-001.safetensors \
  --base-model llama-2-7b

# Expected output:
# ✓ Adapter format: valid
# ✓ Rank: 16
# ✓ Alpha: 32
# ✓ Target modules: q_proj, v_proj
# ✓ Compatible with base model

# Step 2: Stage adapter
themisdb-cli lora stage \
  --adapter /models/lora/adapter-001.safetensors \
  --name custom-domain-adapter \
  --version v1.0.0

# Step 3: Test adapter (dry run)
themisdb-cli lora test \
  --adapter custom-domain-adapter \
  --test-prompts /tests/lora-test-prompts.json \
  --output /tmp/lora-test-results.json

# Review test results
cat /tmp/lora-test-results.json

# Step 4: Deploy adapter
themisdb-cli lora deploy \
  --adapter custom-domain-adapter \
  --endpoint /v1/lora/custom-domain \
  --replicas 2

# Expected output:
# ✓ Adapter loaded on GPU 0
# ✓ Adapter loaded on GPU 1
# ✓ Endpoint active: http://localhost:8080/v1/lora/custom-domain
# ✓ Health check: PASSED

# Step 5: Verify deployment
curl -X POST http://localhost:8080/v1/lora/custom-domain/inference \
  -H "Content-Type: application/json" \
  -d '{
    "prompt": "Test prompt",
    "max_tokens": 50
  }'
```

### Hot-Swap LoRA Adapters

```bash
# Zero-downtime adapter swap
themisdb-cli lora swap \
  --current custom-domain-adapter:v1.0.0 \
  --new custom-domain-adapter:v1.1.0 \
  --strategy blue-green

# Monitor swap progress
themisdb-cli lora swap-status

# Rollback if needed
themisdb-cli lora swap-rollback --swap-id swap_001
```

### Multi-Adapter Inference

```bash
# Load multiple adapters
themisdb-cli lora multi-load \
  --adapters adapter-1,adapter-2,adapter-3 \
  --mode dynamic

# Route requests to specific adapter
curl -X POST http://localhost:8080/v1/inference \
  -H "X-LoRA-Adapter: adapter-2" \
  -d '{"prompt": "..."}'
```

---

## Multi-Shard Inference Setup

### Deploy Multi-Shard System

**Architecture:**
```
Load Balancer
      ↓
  Coordinator
    ↙  ↓  ↘
Shard1 Shard2 Shard3
(GPU0) (GPU1) (GPU2)
```

**Setup Procedure:**

```bash
# Step 1: Configure sharding
cat > /etc/themisdb/sharding.yaml << 'EOF'
sharding:
  enabled: true
  num_shards: 3
  
  coordinator:
    host: 0.0.0.0
    port: 9000
  
  shards:
    - id: shard-0
      gpu_device: 0
      model_layers: [0, 15]
      port: 9001
    
    - id: shard-1
      gpu_device: 1
      model_layers: [16, 31]
      port: 9002
    
    - id: shard-2
      gpu_device: 2
      model_layers: [32, 47]
      port: 9003
  
  load_balancing:
    strategy: least_connections
    health_check_interval: 10s
EOF

# Step 2: Start coordinator
themisdb-cli shard coordinator start \
  --config /etc/themisdb/sharding.yaml

# Step 3: Start shards
for shard in shard-0 shard-1 shard-2; do
  themisdb-cli shard worker start \
    --id $shard \
    --config /etc/themisdb/sharding.yaml &
done

# Step 4: Verify sharding
themisdb-cli shard status

# Expected output:
# Coordinator: RUNNING (port 9000)
# Shard-0: RUNNING (GPU 0, layers 0-15)
# Shard-1: RUNNING (GPU 1, layers 16-31)
# Shard-2: RUNNING (GPU 2, layers 32-47)
# Health: ALL_HEALTHY

# Step 5: Test inference
curl -X POST http://localhost:9000/v1/inference \
  -d '{
    "prompt": "Test multi-shard inference",
    "max_tokens": 100
  }'
```

### Shard Failover

```bash
# Configure hot spare
cat >> /etc/themisdb/sharding.yaml << 'EOF'
  failover:
    enabled: true
    hot_spare_shard:
      id: shard-spare
      gpu_device: 3
    
    detection:
      health_check_timeout: 30s
      max_failures: 3
    
    recovery:
      automatic: true
      takeover_timeout: 60s
EOF

# Test failover
themisdb-cli shard simulate-failure --shard-id shard-1

# Monitor failover
themisdb-cli shard events --follow
```

---

## Emergency Procedures

### GPU Failure Response

**Immediate Actions (5 minutes):**

```bash
# 1. Identify failed GPU
nvidia-smi

# 2. Stop affected services
themisdb-cli gpu disable --device 2

# 3. Redistribute load
themisdb-cli redistribute --exclude-gpu 2

# 4. Notify team
themisdb-cli alert send \
  --severity critical \
  --message "GPU 2 failed, redistributed to remaining GPUs"
```

**Recovery Actions (30 minutes):**

```bash
# 1. Attempt GPU reset
sudo nvidia-smi --gpu-reset --id 2

# 2. If reset fails, mark GPU as maintenance
themisdb-cli gpu maintenance --device 2

# 3. Schedule replacement
# - File hardware ticket
# - Schedule maintenance window

# 4. Update monitoring
themisdb-cli monitoring suppress-alerts --gpu 2 --duration 24h
```

### Training Job Crash

```bash
# 1. Capture crash logs
themisdb-cli job logs job_abc123 --last 1000 > /tmp/crash-$(date +%s).log

# 2. Check for OOM
grep -i "out of memory" /tmp/crash-*.log

# 3. Check checkpoint availability
themisdb-cli checkpoint list --job-id job_abc123 | tail -5

# 4. Restart from last checkpoint
themisdb-cli job restart \
  --job-id job_abc123 \
  --from-checkpoint latest \
  --config-override training.micro_batch_size=16
```

### Service Unresponsive

```bash
# 1. Check service status
sudo systemctl status themisdb

# 2. Check resource exhaustion
nvidia-smi
free -h
df -h

# 3. Capture state
themisdb-cli debug dump --output /tmp/debug-dump.tar.gz

# 4. Restart service
sudo systemctl restart themisdb

# 5. Monitor recovery
themisdb-cli health --watch

# 6. If not recovering, escalate
# - Page on-call engineer
# - Prepare for cold restart
```

### Data Corruption Detection

```bash
# 1. Stop writes immediately
themisdb-cli read-only enable

# 2. Run integrity check
themisdb-cli integrity check --full

# 3. Identify scope of corruption
themisdb-cli integrity report

# 4. Restore from backup
themisdb-cli restore \
  --backup /backup/latest \
  --verify-before-restore

# 5. Resume operations
themisdb-cli read-only disable
```

---

## Maintenance Windows

### Planned Maintenance Procedure

**Pre-Maintenance (24 hours before):**

```bash
# 1. Notify users
themisdb-cli maintenance announce \
  --start "2026-01-20 02:00:00 UTC" \
  --duration 4h \
  --reason "GPU driver upgrade"

# 2. Pause new job submissions
themisdb-cli submissions pause

# 3. Let running jobs complete or checkpoint
themisdb-cli jobs checkpoint-all

# 4. Full backup
themisdb-cli backup full \
  --destination /backup/pre-maintenance-$(date +%Y%m%d)
```

**During Maintenance:**

```bash
# 1. Stop service gracefully
sudo systemctl stop themisdb

# 2. Perform maintenance
# - Update GPU drivers
# - Apply OS patches
# - Hardware changes

# 3. Verify system
nvidia-smi
themisdb-cli test gpu --all

# 4. Start service
sudo systemctl start themisdb

# 5. Smoke tests
themisdb-cli test inference --quick
themisdb-cli test training --quick
```

**Post-Maintenance:**

```bash
# 1. Verify all systems operational
themisdb-cli health --full

# 2. Resume job submissions
themisdb-cli submissions resume

# 3. Monitor for issues
themisdb-cli monitor --duration 1h

# 4. Close maintenance window
themisdb-cli maintenance complete

# 5. Send completion notice
themisdb-cli maintenance report \
  --send-email \
  --send-slack
```

### Rolling Updates (Zero Downtime)

```bash
# 1. Prepare new version
docker pull themisdb/themisdb:v1.4.1

# 2. Update nodes one by one
for node in node-1 node-2 node-3; do
  # Drain node
  themisdb-cli node drain $node --wait
  
  # Update
  ssh $node "docker stop themisdb && docker rm themisdb"
  ssh $node "docker run -d --name themisdb themisdb/themisdb:v1.4.1"
  
  # Verify
  themisdb-cli node health $node --wait
  
  # Re-enable
  themisdb-cli node enable $node
  
  # Wait for stability
  sleep 60
done
```

---

## Backup and Recovery

### Automated Backup

```bash
# Configure automated backups
cat > /etc/themisdb/backup.yaml << 'EOF'
backup:
  enabled: true
  
  schedule:
    full_backup: "0 2 * * 0"    # Weekly, Sunday 2 AM
    incremental: "0 2 * * 1-6"  # Daily, 2 AM
    checkpoint: "0 */4 * * *"   # Every 4 hours
  
  retention:
    full: 4         # Keep 4 weekly backups
    incremental: 7  # Keep 7 daily backups
    checkpoint: 24  # Keep 24 checkpoint backups
  
  destinations:
    - type: local
      path: /backup/themisdb
    
    - type: s3
      bucket: themisdb-backups
      region: us-west-2
    
    - type: remote
      host: backup-server.example.com
      path: /backups/themisdb
EOF

# Enable backup service
themisdb-cli backup enable --config /etc/themisdb/backup.yaml

# Test backup
themisdb-cli backup test --dry-run
```

### Manual Backup

```bash
# Full backup
themisdb-cli backup create \
  --type full \
  --output /backup/manual-backup-$(date +%Y%m%d-%H%M%S) \
  --compress \
  --verify

# Incremental backup
themisdb-cli backup create \
  --type incremental \
  --base-backup /backup/last-full-backup \
  --output /backup/incremental-$(date +%Y%m%d-%H%M%S)
```

### Recovery Procedure

```bash
# List available backups
themisdb-cli backup list

# Verify backup integrity
themisdb-cli backup verify /backup/backup-20260117

# Restore (dry run)
themisdb-cli restore \
  --backup /backup/backup-20260117 \
  --dry-run

# Actual restore
themisdb-cli restore \
  --backup /backup/backup-20260117 \
  --target /data/themisdb-restored \
  --verify-after-restore

# Switch to restored data
sudo systemctl stop themisdb
sudo mv /data/themisdb /data/themisdb-old
sudo mv /data/themisdb-restored /data/themisdb
sudo systemctl start themisdb
```

---

## Upgrade Procedures

### Zero-Downtime Rolling Upgrade

**Prerequisites:**
- New version tested in staging
- Backup completed
- Rollback plan prepared

**Procedure:**

```bash
# 1. Pre-upgrade checks
themisdb-cli version check --target $TARGET_VERSION
themisdb-cli health --full
themisdb-cli backup create --type full --label "pre-upgrade-$TARGET_VERSION"

# 2. Enable maintenance mode (optional, for major upgrades)
themisdb-cli maintenance enable --mode soft  # Allows existing jobs to complete

# 3. Upgrade coordinator first
themisdb-cli upgrade coordinator \
  --version $TARGET_VERSION \
  --strategy rolling \
  --wait-for-health

# Expected output:
# ✓ Downloaded version $TARGET_VERSION
# ✓ Backup created
# ✓ Stopping coordinator (graceful)
# ✓ Installing $TARGET_VERSION
# ✓ Starting coordinator
# ✓ Health check passed
# Coordinator upgraded successfully

# 4. Upgrade workers (one at a time)
for worker in worker-1 worker-2 worker-3; do
  echo "Upgrading $worker..."
  
  # Drain worker
  themisdb-cli node drain $worker --timeout 10m
  
  # Upgrade
  themisdb-cli upgrade node $worker \
    --version $TARGET_VERSION \
    --wait-for-health
  
  # Verify
  themisdb-cli node health $worker
  
  # Re-enable
  themisdb-cli node enable $worker
  
  # Wait for stability before next node
  sleep 60
done

# 5. Verify cluster after upgrade
themisdb-cli cluster status
themisdb-cli version --all-nodes

# Expected output (example with v1.4.1):
# Coordinator: $TARGET_VERSION
# Worker-1: $TARGET_VERSION
# Worker-2: $TARGET_VERSION
# Worker-3: $TARGET_VERSION
# Cluster Status: HEALTHY

# 6. Run post-upgrade tests
themisdb-cli test upgrade-validation

# 7. Disable maintenance mode
themisdb-cli maintenance disable

# 8. Monitor for issues
themisdb-cli monitor --duration 2h --alert-on-anomaly
```

**Rollback Procedure:**

```bash
# If upgrade fails, rollback immediately

# 1. Stop upgraded nodes
themisdb-cli cluster pause

# 2. Restore from backup
themisdb-cli restore \
  --backup pre-upgrade-$TARGET_VERSION \
  --verify

# 3. Restart cluster with previous version
themisdb-cli cluster restart --force-version $PREVIOUS_VERSION

# 4. Verify rollback
themisdb-cli health --full
themisdb-cli version --all-nodes

# 5. Investigate upgrade failure
themisdb-cli logs --component upgrade --last 1000
```

### In-Place Upgrade (Single Node)

```bash
# For single-node deployments with planned downtime

# 1. Announce downtime
themisdb-cli maintenance announce \
  --start "2026-01-25 02:00:00 UTC" \
  --duration 1h

# 2. Stop all jobs gracefully
themisdb-cli jobs stop-all --graceful --timeout 10m

# 3. Backup
themisdb-cli backup create --type full

# 4. Stop service
sudo systemctl stop themisdb

# 5. Upgrade (example for specific version)
# Choose appropriate method for your environment:
# - Package manager: sudo apt update && sudo apt install themisdb=$TARGET_VERSION
# - Docker: docker pull themisdb/themisdb:$TARGET_VERSION
sudo apt update && sudo apt install themisdb  # Updates to latest
# OR for specific version:
# sudo apt install themisdb=1.4.1

# 6. Run database migrations (if needed)
themisdb-cli db migrate --auto

# 7. Start service
sudo systemctl start themisdb

# 8. Verify
themisdb-cli health --full
themisdb-cli version

# 9. Resume operations
themisdb-cli maintenance complete
```

### GPU Driver Upgrade

```bash
# Critical: GPU driver upgrades require careful planning

# 1. Check compatibility
themisdb-cli gpu driver-compatibility --target-driver 535.129.03

# 2. Test on one GPU first
themisdb-cli gpu maintenance --device 0

# 3. Upgrade driver
sudo apt install nvidia-driver-535
# OR
sudo nvidia-installer --update

# 4. Reboot (may be required)
sudo reboot

# 5. Verify GPU
nvidia-smi
themisdb-cli gpu test --device 0

# 6. Bring GPU back online
themisdb-cli gpu enable --device 0

# 7. Repeat for remaining GPUs
```

---

## Failover Procedures

### Automatic Failover (Hot Spare)

**Configuration:**

```yaml
# /etc/themisdb/failover.yaml
failover:
  enabled: true
  mode: hot_spare
  
  primary:
    id: primary-node
    host: 192.168.1.100
    priority: 100
  
  hot_spare:
    id: spare-node
    host: 192.168.1.101
    priority: 90
  
  detection:
    heartbeat_interval: 5s
    failure_threshold: 3
    health_check_timeout: 30s
  
  takeover:
    automatic: true
    delay: 10s  # Prevent flapping
    sync_timeout: 60s
  
  recovery:
    automatic_fallback: true
    fallback_delay: 300s  # Wait 5 min after primary recovers
```

**Enable Failover:**

```bash
# 1. Configure hot spare
themisdb-cli failover configure \
  --config /etc/themisdb/failover.yaml

# 2. Start hot spare in standby mode
themisdb-cli failover enable-spare \
  --node spare-node \
  --sync-from primary-node

# Expected output:
# ✓ Spare node initialized
# ✓ Data sync started (0/100GB)
# ✓ Sync progress: 100% (100/100GB)
# ✓ Spare node ready
# ✓ Failover armed

# 3. Verify failover readiness
themisdb-cli failover status

# Expected output:
# Primary: HEALTHY (192.168.1.100)
# Spare: READY (192.168.1.101)
# Failover: ARMED
# Last Sync: 2026-01-24 06:15:00 UTC
# Sync Lag: 2.3 seconds
```

**Manual Failover:**

```bash
# 1. Initiate failover
themisdb-cli failover initiate \
  --from primary-node \
  --to spare-node \
  --reason "Planned maintenance"

# 2. Monitor failover progress
themisdb-cli failover status --follow

# Expected output:
# [06:15:01] Draining primary node
# [06:15:15] Final sync to spare
# [06:15:20] Promoting spare to primary
# [06:15:25] Spare promoted
# [06:15:30] Redirecting traffic
# [06:15:35] Failover complete
# 
# New Primary: 192.168.1.101 (formerly spare-node)
# Old Primary: 192.168.1.100 (now standby)
# Failover Duration: 34 seconds

# 3. Verify new primary
themisdb-cli health --node 192.168.1.101

# 4. Update DNS/load balancer (if manual)
# Update DNS: themisdb.example.com -> 192.168.1.101
```

**Fallback After Recovery:**

```bash
# After original primary is repaired

# 1. Verify original primary is healthy
themisdb-cli health --node 192.168.1.100

# 2. Sync data from current primary to original
themisdb-cli failover sync \
  --from 192.168.1.101 \
  --to 192.168.1.100

# 3. Fallback to original primary
themisdb-cli failover fallback \
  --from 192.168.1.101 \
  --to 192.168.1.100 \
  --wait-for-sync

# 4. Verify
themisdb-cli failover status
```

### Multi-Region Disaster Recovery Failover

**Scenario:** Primary region fails, failover to DR region

```bash
# Pre-configured DR setup required (see DISASTER_RECOVERY.md)

# 1. Declare disaster
themisdb-cli dr declare-disaster \
  --region us-west-2 \
  --reason "Region outage"

# 2. Activate DR site
themisdb-cli dr activate \
  --region us-east-1 \
  --mode emergency

# Expected output:
# ✓ Validating DR site readiness
# ✓ Promoting read replicas to primary
# ✓ Redirecting traffic to us-east-1
# ✓ Updating DNS (propagation may take 60s)
# ✓ DR site active
# 
# New Primary Region: us-east-1
# RPO: 15 seconds (data loss window)
# RTO: 3 minutes (recovery time)

# 3. Verify DR site operations
themisdb-cli health --region us-east-1

# 4. Monitor recovery
themisdb-cli dr status --follow

# 5. When primary region recovers, sync back
themisdb-cli dr failback \
  --from us-east-1 \
  --to us-west-2 \
  --sync-mode full
```

### Shard Failover

```bash
# When a shard fails in a multi-shard setup

# 1. Detect failed shard (automatic)
# System automatically detects shard-2 failure

# 2. Verify failover occurred
themisdb-cli shard status

# Expected output:
# Shard-0: HEALTHY (GPU 0)
# Shard-1: HEALTHY (GPU 1)
# Shard-2: FAILED (GPU 2) -> FAILOVER to spare-0
# Spare-0: ACTIVE (GPU 3, replacing shard-2)

# 3. Manual shard failover (if needed)
themisdb-cli shard failover \
  --from shard-2 \
  --to spare-0 \
  --sync-mode fast

# 4. Replace failed hardware
# - Physical GPU replacement
# - Driver installation

# 5. Restore original shard
themisdb-cli shard restore \
  --shard-id shard-2 \
  --gpu-device 2 \
  --sync-from spare-0

# 6. Return spare to standby
themisdb-cli shard demote \
  --shard-id spare-0 \
  --mode standby
```

---

## Scaling Operations

### Scale Up (Add GPUs)

```bash
# 1. Install new GPU
# - Physical installation
# - Driver installation

# 2. Verify GPU
nvidia-smi

# 3. Register GPU with ThemisDB
themisdb-cli gpu register \
  --device 4 \
  --name "GPU-4-RTX4090"

# 4. Add to resource pool
themisdb-cli gpu enable --device 4

# 5. Rebalance workload
themisdb-cli rebalance --include-gpu 4

# 6. Verify
themisdb-cli gpu list
```

### Scale Out (Add Nodes)

```bash
# On new node:
# 1. Install ThemisDB
curl -fsSL https://get.themisdb.io | sh

# 2. Configure as worker
themisdb-cli cluster join \
  --coordinator 192.168.1.100:9000 \
  --role worker \
  --gpus 4

# On coordinator:
# 3. Accept new node
themisdb-cli cluster accept-node node-4

# 4. Verify cluster
themisdb-cli cluster status
```

---

## Security Incident Response

### Unauthorized Access Detected

```bash
# 1. Identify affected accounts
themisdb-cli audit query \
  --event-type "failed_login" \
  --last 24h

# 2. Lock compromised accounts
themisdb-cli user lock --username suspicious_user

# 3. Revoke active sessions
themisdb-cli session revoke --user suspicious_user

# 4. Enable enhanced monitoring
themisdb-cli security alert-level high

# 5. Rotate credentials
themisdb-cli security rotate-keys --all

# 6. Review audit logs
themisdb-cli audit export \
  --start "24 hours ago" \
  --output /tmp/security-audit-$(date +%s).json
```

### Suspected Data Breach

```bash
# 1. Immediate containment
themisdb-cli network isolate

# 2. Capture forensics
themisdb-cli forensics capture \
  --output /secure/forensics-$(date +%s)

# 3. Notify security team
# - Follow incident response plan
# - Contact legal/compliance

# 4. Investigate
themisdb-cli audit analyze \
  --anomaly-detection \
  --output /secure/analysis.pdf

# 5. Recovery
# - Follow recovery playbook
# - Update security policies
```

---

## Performance Tuning Workflow

### Iterative Optimization Process

```bash
# 1. Establish baseline
themisdb-cli benchmark \
  --workload production \
  --duration 1h \
  --output /tmp/baseline.json

# 2. Identify bottleneck
themisdb-cli profile \
  --duration 300s \
  --output /tmp/profile.json

themisdb-cli profile analyze /tmp/profile.json

# 3. Apply optimization
# Edit config.yaml based on analysis

# 4. Test optimization
themisdb-cli benchmark \
  --workload production \
  --duration 1h \
  --output /tmp/optimized.json

# 5. Compare results
themisdb-cli benchmark compare \
  --baseline /tmp/baseline.json \
  --optimized /tmp/optimized.json

# 6. If improvement > 10%, deploy
# Otherwise, rollback and try different optimization
```

---

## Runbook Templates

### Job Submission Template

```yaml
# Copy and customize for your use case
job:
  name: "CHANGE-ME"
  type: training  # or: inference, fine-tuning
  priority: normal  # or: high, low

model:
  base: "llama-2-7b-chat"
  output: "CHANGE-ME"

training:
  dataset: "/path/to/dataset"
  epochs: 3
  batch_size: 32
  
gpu:
  devices: [0]
  
notifications:
  email: "ops@example.com"
```

### Emergency Contact List

```
Primary On-Call: +1-555-0001
Secondary On-Call: +1-555-0002
Infrastructure Team: infra@example.com
Security Team: security@example.com
Slack Channel: #themisdb-ops
PagerDuty: themisdb-production
```

---

## Next Steps

- **Monitoring**: Set up alerting for critical operations ([MONITORING.md](MONITORING.md))
- **Security**: Review security procedures ([SECURITY.md](SECURITY.md))
- **Checklists**: Use pre-flight checklists ([CHECKLISTS/](CHECKLISTS/))

---

**Document Version:** 1.0  
**Last Updated:** April 2026  
**Next Review:** April 2026
