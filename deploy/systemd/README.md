# ThemisDB systemd Deployment Files

This directory contains the systemd unit file and production drop-in for running
ThemisDB as a managed system service.

## Files

| File | Purpose |
|------|---------|
| `themisdb.service` | Base systemd unit — hardened with `ProtectSystem=strict`, `NoNewPrivileges`, `CapabilityBoundingSet=` (drop-all), `PrivateTmp`, `SystemCallFilter` |
| `themisdb.service.d/production.conf` | Drop-in that sets all required production environment variables and removes unsafe debug vars via `UnsetEnvironment=` |
| `themisdb.env.example` | Template for `/etc/themisdb/themisdb.env` — the secrets file loaded by `EnvironmentFile=` |

## Installation

### 1. Install the service unit

```bash
sudo cp themisdb.service /etc/systemd/system/themisdb.service
sudo systemctl daemon-reload
```

### 2. Create the production user and directories

```bash
sudo useradd --system --no-create-home --shell /sbin/nologin themisdb
sudo mkdir -p /etc/themisdb/tls /var/lib/themisdb /var/log/themisdb
sudo chown -R themisdb:themisdb /var/lib/themisdb /var/log/themisdb
```

### 3. Install TLS certificates

```bash
# Place your server cert and key here (mode 640, owned by root:themisdb):
sudo cp server.crt /etc/themisdb/tls/server.crt
sudo cp server.key /etc/themisdb/tls/server.key
sudo chmod 640 /etc/themisdb/tls/server.crt /etc/themisdb/tls/server.key
sudo chown root:themisdb /etc/themisdb/tls/server.crt /etc/themisdb/tls/server.key
```

### 4. Create the secrets environment file

```bash
sudo cp themisdb.env.example /etc/themisdb/themisdb.env
sudo chmod 600 /etc/themisdb/themisdb.env
sudo chown root:themisdb /etc/themisdb/themisdb.env
# Edit the file and fill in: THEMIS_TOKEN_ADMIN, THEMIS_VAULT_TOKEN (or THEMIS_HSM_PIN), etc.
sudo nano /etc/themisdb/themisdb.env
```

### 5. Install the production drop-in

```bash
sudo mkdir -p /etc/systemd/system/themisdb.service.d/
sudo cp themisdb.service.d/production.conf /etc/systemd/system/themisdb.service.d/production.conf
# Edit the drop-in to match your Vault address and certificate paths:
sudo nano /etc/systemd/system/themisdb.service.d/production.conf
sudo systemctl daemon-reload
```

### 6. Enable and start

```bash
sudo systemctl enable themisdb
sudo systemctl start themisdb
sudo systemctl status themisdb
```

### 7. Verify

```bash
# Check the health endpoint (adjust address if THEMIS_HEALTH_BIND_ADDRESS differs):
curl -s http://127.0.0.1:8081/health

# Check audit log output:
sudo journalctl -u themisdb -n 50
```

## Security Notes

- The base unit drops **all** Linux capabilities (`CapabilityBoundingSet=`). If ThemisDB requires any capability (e.g., to bind to ports < 1024), add it explicitly in the drop-in: `AmbientCapabilities=CAP_NET_BIND_SERVICE`.
- The production drop-in uses `UnsetEnvironment=` (systemd ≥ 235) to guarantee that `THEMIS_ALLOW_HSM_STUB`, `THEMIS_DEBUG_ENC_DIR`, and `THEMIS_DEBUG_PKI` are absent even if inherited from the environment.
- Secrets (`THEMIS_TOKEN_ADMIN`, `THEMIS_VAULT_TOKEN`, `THEMIS_HSM_PIN`) are intentionally loaded from `/etc/themisdb/themisdb.env` (mode 600) rather than being set inline in the unit file to prevent exposure in `systemctl show` output.

## Further Reading

- [Production Runbook](../../docs/production/RUNBOOKS/CORE_MODULE_RUNBOOK.md) — env vars, failure modes, mitigations
- [Security Posture Guide](../../docs/production/SECURITY_POSTURE.md) — defaults, hardening checklist
- [Kubernetes production Helm values](../../docs/production/examples/k8s_production_values.yaml)
