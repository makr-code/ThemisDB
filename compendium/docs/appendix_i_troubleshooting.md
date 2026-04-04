# Appendix I: Troubleshooting Guide & Common Issues

> "The best troubleshooting guide prevents issues before they start."

---

## I.1 Installation Issues

### Issue: Failed to Install Dependencies

**Error:**
```
ERROR: Package xyz not found in vcpkg
```

**Diagnosis:**
```bash
# Check vcpkg status
./vcpkg list | grep xyz

# Verify internet connection
ping github.com
```

**Solutions:**
1. **Update vcpkg registry:**
   ```bash
   ./vcpkg update
   ./vcpkg upgrade  # Rebuild all packages
   ```

2. **Clear cache:**
   ```bash
   rm -rf vcpkg_installed/
   ./vcpkg install  # Reinstall from scratch
   ```

3. **Use specific version:**
   ```json
   // vcpkg.json
   {
     "overrides": [
       {
         "name": "xyz",
         "version": "1.2.3"
       }
     ]
   }
   ```

---

## I.2 Connection Issues

### Issue: Cannot Connect to Database

**Error:**
```
Connection refused at localhost:8529
```

**Diagnosis:**
```bash
# Check if service running
systemctl status themis

# Check port in use
lsof -i :8529

# Check firewall
sudo ufw status
```

**Solutions:**

1. **Service not running:**
   ```bash
   sudo systemctl start themis
   sudo systemctl enable themis  # Auto-start on reboot
   ```

2. **Port blocked by firewall:**
   ```bash
   sudo ufw allow 8529
   sudo ufw allow in on docker0 to any port 8529  # Docker
   ```

3. **Process stuck:**
   ```bash
   # Force kill (last resort)
   pkill -9 themisdb
   sudo systemctl restart themis
   ```

### Issue: SSL/TLS Certificate Error

**Error:**
```
ERROR: x509: certificate signed by unknown authority
```

**Solutions:**

1. **Invalid certificate:**
   ```bash
   # Check certificate
   openssl x509 -in /etc/themis/certs/server.crt -text -noout
   
   # Verify against key
   openssl x509 -in server.crt -noout -modulus | openssl md5
   openssl rsa -in server.key -noout -modulus | openssl md5
   # Should match
   ```

2. **Self-signed certificate (dev only):**
   ```bash
   # Disable SSL verification (DANGER - dev only)
   THEMIS_SKIP_VERIFY=true themis-cli ...
   ```

3. **Certificate expired:**
   ```bash
   # Generate new certificate
   openssl req -x509 -nodes -days 365 -newkey rsa:4096 \
     -keyout server.key -out server.crt
   
   # Restart
   sudo systemctl restart themis
   ```

---

## I.3 Performance Issues

### Issue: Queries Running Slow (>1s)

**Diagnosis:**
```aql
-- Use EXPLAIN to see query plan
EXPLAIN
FOR user IN users
  FILTER user.status == 'active'
  RETURN user

-- Check which collection scan
-- If CollectionScan on large table → use index!
```

**Solutions:**

1. **Missing Index:**
   ```aql
   -- Create index on filtered field
   CREATE INDEX idx_status ON users(status)
   
   -- Verify index is used
   EXPLAIN <query>  -- Should show IndexSeek
   ```

2. **Wrong Index:**
   ```aql
   -- Problem: Composite index (status, created_at)
   -- But querying only created_at
   
   -- Solution: Reorder to match query pattern
   DROP INDEX idx_status_created
   CREATE INDEX idx_created_status ON users(created_at, status)
   ```

3. **Expression in FILTER:**
   ```aql
   -- ❌ SLOW: Expression prevents index use
   FILTER YEAR(user.created_at) == 2025
   
   -- ✅ FAST: Range query on indexed field
   FILTER user.created_at >= '2025-01-01' 
     AND user.created_at < '2026-01-01'
   ```

### Issue: High Memory Usage (>80%)

**Diagnosis:**
```bash
# Check memory
free -h
top -p $(pgrep themis)

# Check cache settings
curl http://localhost:8529/_admin/cache | jq .

# Check for cursor leaks
curl http://localhost:8529/_admin/cursors | jq 'length'
```

**Solutions:**

1. **Cache too large:**
   ```yaml
   # themis.conf
   cache:
     size_mb: 2048  # Reduce from 8192
   ```
   Then: `sudo systemctl reload themis`

2. **Cursor leak (unclosed connections):**
   ```python
   # ❌ Leak
   cursor = client.query("SELECT ...")
   # Never closed
   
   # ✅ Safe
   with client.query("SELECT ...") as cursor:
       for doc in cursor:
           process(doc)
   ```

3. **Large result set:**
   ```aql
   -- ❌ Materializes 1M rows
   FOR doc IN collection
     RETURN doc
   
   -- ✅ Paginate
   FOR doc IN collection
     LIMIT @offset, 100
     RETURN doc
   ```

---

## I.4 Data Issues

### Issue: Data Corruption (Checksum Mismatch)

**Error:**
```
ERROR: Block checksum mismatch at offset 123456
```

**Recovery:**
```bash
# 1. Stop database
sudo systemctl stop themis

# 2. Run recovery
themisdb recover /var/lib/themis

# 3. If recovery fails, restore backup
tar -xzf /backup/themis-2025-01-01.tar.gz -C /var/lib/
sudo chown -R themis:themis /var/lib/themis

# 4. Start
sudo systemctl start themis

# 5. Verify
aql "RETURN COUNT(*) FROM collection"
```

### Issue: Missing Data (Unintended Deletion)

**Diagnosis:**
```bash
# Check logs for DELETE operations
grep DELETE /var/log/themis/audit.log | tail -20

# Find deletion timestamp
# Look at backup from before deletion
```

**Recovery:**
```bash
# Option 1: Point-in-time restore
sudo systemctl stop themis
tar -xzf /backup/themis-2025-01-01-14-00.tar.gz -C /var/lib/  # Before deletion
sudo systemctl start themis

# Option 2: Selective restore (if using AWS)
aws s3 cp s3://backups/themis-before-deletion/ /tmp/
# Manually restore needed collections
```

### Issue: Replication Lag Too High (>10s)

**Diagnosis:**
```bash
# Check lag
curl http://primary:8529/_admin/replication/lag | jq .lag_ms

# Check follower CPU/IO
ssh follower "top -bn1 | head -15"
ssh follower "iostat -x 1 5 | grep sda"

# Check network
mtr primary-ip --report --report-wide
```

**Solutions:**

1. **Follower slow (CPU/IO):**
   ```bash
   # Add more resources
   # Edit VM/K8s resource limits
   # Restart follower to pick up new limits
   ```

2. **Network congestion:**
   ```bash
   # Check MTU
   ip link show | grep mtu
   
   # If < 9000, enable Jumbo frames (if supported)
   ip link set dev eth0 mtu 9000
   ```

3. **Primary write rate too high:**
   ```yaml
   # Throttle writes (temporary)
   # themis.conf
   database:
     max_write_rate_per_sec: 1000  # Limit to 1k/s
   ```

---

## I.5 Replication Issues

### Issue: Follower Won't Start (Replication Sync Failed)

**Error:**
```
ERROR: Cannot connect to primary at primary.example.com:8529
```

**Solutions:**

1. **Primary unreachable:**
   ```bash
   # Test connectivity
   telnet primary.example.com 8529
   ping primary.example.com
   
   # Check network policies
   kubectl get networkpolicies -A
   ```

2. **Follower config wrong:**
   ```yaml
   # Check follower primary address
   cat /etc/themis/themis.conf | grep follower_primary
   
   # Fix if needed
   # Edit config, then: systemctl restart themis
   ```

3. **Data mismatch (corruption):**
   ```bash
   # Full resync (rebuilds follower from scratch)
   themisdb replication reset
   sudo systemctl start themis
   # This will take time (downloads full dataset from primary)
   ```

### Issue: Primary-Follower Divergence

**Symptoms:**
```
Primary: 10000 documents
Follower: 9999 documents
```

**Investigation:**
```bash
# Count on both
curl http://primary:8529/_admin/collection/mycoll/count | jq .count
curl http://follower:8529/_admin/collection/mycoll/count | jq .count

# Find differences
# Get document IDs from both, compare
```

**Fix:**
```bash
# Resync follower
sudo systemctl stop themis-follower

# Reset and rebuild from primary
themisdb replication reset --primary primary:8529

# Verify after sync complete
curl http://follower:8529/_admin/replication/status | jq .lag_ms
# Should be < 100ms
```

---

## I.6 Backup & Recovery Issues

### Issue: Backup Failed

**Error:**
```
ERROR: Failed to backup - insufficient space
```

**Solutions:**

1. **Disk full:**
   ```bash
   # Check disk
   df -h /backup
   
   # Clean old backups
   ls -lt /backup/*.tar.gz | tail -10  # Identify oldest
   rm /backup/themis-2024-*.tar.gz     # Delete old
   ```

2. **Permissions:**
   ```bash
   # Check backup directory ownership
   ls -ld /backup
   
   # Fix if needed
   sudo chown themis:themis /backup
   sudo chmod 750 /backup
   ```

3. **Database locked:**
   ```bash
   # Check if backup process stuck
   ps aux | grep themis-backup
   
   # Kill if necessary
   pkill -f themis-backup
   ```

### Issue: Restore Failed (Data Corrupted)

**Error:**
```
ERROR: Restore failed - backup corrupted
```

**Verification:**
```bash
# Test backup before restoring
tar -tzf /backup/themis-2025-01-01.tar.gz > /dev/null && \
  echo "Backup OK" || echo "Backup CORRUPTED"

# If corrupted, try another backup
ls -lt /backup/themis-*.tar.gz | head -5
```

**Recovery:**
```bash
# Use oldest good backup
BACKUP=/backup/themis-2024-12-25.tar.gz

# Verify it's good
tar -tzf $BACKUP > /dev/null || echo "Also corrupted!"

# If all backups corrupted
# Use WAL recovery (if available)
themisdb recover --wal-only /var/lib/themis
```

---

## I.7 Security Issues

### Issue: Unauthorized Access (Auth Failing)

**Error:**
```
ERROR: Authentication failed: Invalid credentials
```

**Diagnosis:**
```bash
# Check auth logs
tail -50 /var/log/themis/auth.log

# Verify user exists
aql "SELECT * FROM users WHERE name == 'alice'"

# Check password hash (if applicable)
# Never log actual password
```

**Solutions:**

1. **Wrong password:**
   ```bash
   # Reset user password
   themis-cli admin reset-password alice
   # New temporary password will be printed
   ```

2. **User locked out:**
   ```aql
   -- Unlock user
   UPDATE {name: 'alice'} WITH {locked: false} IN users
   ```

3. **JWT token expired:**
   ```python
   # Get new token
   token = client.authenticate(username='alice', password='xxx')
   # Use new token in subsequent requests
   ```

### Issue: Data Breach Detected

**Immediate Actions:**
```
1. Isolate database from network
   → Drop external connections
   → Kill active sessions
   
2. Preserve evidence
   → Copy audit logs
   → Backup database
   → Keep system for forensics
   
3. Notify stakeholders
   → Security team
   → Management
   → Legal (GDPR notification)
   
4. Rotate credentials
   → Generate new passwords
   → Rotate encryption keys
   → Revoke old tokens
```

---

## I.8 Support Matrix: By Issue Type

| Issue | Severity | MTTR | Self-Fix % |
|-------|----------|------|-----------|
| Connection timeout | High | 5 min | 95% |
| Query slow (no index) | Medium | 10 min | 90% |
| High memory usage | High | 15 min | 80% |
| Data corruption | Critical | 30 min | 60% |
| Replication lag | Medium | 20 min | 70% |
| Backup failed | Medium | 15 min | 85% |
| Auth issues | High | 10 min | 75% |
| Security breach | Critical | 60 min | 20% |

---

## I.9 Escalation Path

```
Level 1: Try self-help
  ├─ Check documentation
  ├─ Search existing issues
  └─ Try basic troubleshooting

Level 2: Community support
  ├─ GitHub Discussions
  ├─ Stack Overflow
  └─ Slack community

Level 3: Vendor support
  ├─ File bug report
  ├─ Provide diagnostic data
  └─ Wait for response

Level 4: Enterprise support
  ├─ Dedicated support engineer
  ├─ SLA response times
  └─ Priority queue
```

---

## I.10 Data Collection for Support

**When reporting issues, include:**

```bash
#!/bin/bash
# Diagnostic data collection script

echo "=== System Info ==="
uname -a
free -h
lsb_release -a

echo "=== ThemisDB Version ==="
themisdb --version

echo "=== Service Status ==="
systemctl status themis
journalctl -u themis -n 50

echo "=== Memory Usage ==="
curl http://localhost:8529/_admin/memory | jq .

echo "=== Disk Usage ==="
df -h /var/lib/themis

echo "=== Active Queries (top 5) ==="
curl http://localhost:8529/_admin/slow-queries | jq '.queries[0:5]'

echo "=== Configuration ==="
grep -v '^#' /etc/themis/themis.conf | grep -v '^$'

# Save to file
```

---

## I.11 Contacting Support

### Community Channels
- **GitHub Issues:** github.com/makr-code/ThemisDB/issues
- **Discussions:** github.com/makr-code/ThemisDB/discussions
- **Stack Overflow:** Tag: `themisdb`

### Enterprise Support (SLA)
- **Response time:** < 4 hours for Critical
- **On-call escalation:** Yes
- **Dedicated engineer:** Yes

### When to Contact
- **Development questions:** Community first
- **Bug reports:** GitHub Issues
- **Security issues:** Responsible disclosure (see SECURITY.md)
- **Critical production outage:** Vendor support hotline

---

## Summary

Most issues fall into categories above. With this guide, you should be able to:
1. **Diagnose** problems quickly
2. **Fix** common issues yourself
3. **Know when to escalate** to support
4. **Provide** detailed diagnostic data

Keep this guide bookmarked for quick reference during incidents.
