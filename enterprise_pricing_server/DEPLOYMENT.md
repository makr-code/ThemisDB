# Production Deployment Guide

## Deployment to https://service.themisdb.org:6734

This guide covers deploying the ThemisDB Enterprise Pricing Server to production.

---

## Prerequisites

- Ubuntu 22.04 LTS or later
- PostgreSQL 14+ installed
- Python 3.10+ installed
- nginx or Caddy for reverse proxy
- SSL certificate for service.themisdb.org
- Firewall configured to allow port 6734

---

## 1. Database Setup

### Install PostgreSQL

```bash
sudo apt update
sudo apt install postgresql postgresql-contrib
```

### Create Database

```bash
# Switch to postgres user
sudo -u postgres psql

# Execute the setup script
sudo -u postgres psql < /path/to/setup_database.sql

# Or manually:
sudo -u postgres createuser themis_pricing
sudo -u postgres createdb -O themis_pricing themis_pricing
```

### Run Database Schema

```bash
cd /opt/themisdb/enterprise_pricing_server
sudo -u postgres psql -d themis_pricing -f setup_database.sql
```

### Set Strong Password

```bash
sudo -u postgres psql
ALTER USER themis_pricing WITH PASSWORD 'YOUR_STRONG_PASSWORD_HERE';
\q
```

---

## 2. Application Setup

### Create System User

```bash
sudo useradd -r -s /bin/bash -d /opt/themisdb themisdb
sudo mkdir -p /opt/themisdb
sudo chown themisdb:themisdb /opt/themisdb
```

### Install Application

```bash
# Clone or copy application files
sudo -u themisdb mkdir -p /opt/themisdb/enterprise_pricing_server
sudo cp -r enterprise_pricing_server/* /opt/themisdb/enterprise_pricing_server/
cd /opt/themisdb/enterprise_pricing_server
```

### Install Python Dependencies

```bash
# Create virtual environment
sudo -u themisdb python3 -m venv /opt/themisdb/venv

# Activate and install
sudo -u themisdb /opt/themisdb/venv/bin/pip install -r requirements.txt
```

---

## 3. Configuration

### Create Production .env File

```bash
sudo -u themisdb nano /opt/themisdb/enterprise_pricing_server/.env
```

```env
# Application
APP_NAME=ThemisDB Enterprise Pricing Server
VERSION=1.0.0
DEBUG=false

# Server
HOST=0.0.0.0
PORT=6734

# Database (PostgreSQL)
DATABASE_URL=postgresql+asyncpg://themis_pricing:YOUR_PASSWORD@localhost:5432/themis_pricing

# Security - IMPORTANT: Generate new secret key!
SECRET_KEY=<output from: openssl rand -hex 32>
ALGORITHM=HS256
ACCESS_TOKEN_EXPIRE_MINUTES=1440

# Payment Provider
STRIPE_API_KEY=sk_live_...
STRIPE_WEBHOOK_SECRET=whsec_...

# Banking Interface (if using)
BANKING_API_URL=https://api.your-bank.com
BANKING_API_KEY=your_api_key

# Pricing (EUR per month)
COMMUNITY_PRICE=0.0
ENTERPRISE_PRICE=5000.0
HYPERSCALER_PRICE=25000.0
RESELLER_PRICE=15000.0
```

### Secure Configuration Files

```bash
sudo chmod 600 /opt/themisdb/enterprise_pricing_server/.env
sudo chown themisdb:themisdb /opt/themisdb/enterprise_pricing_server/.env
```

---

## 4. Systemd Service

### Create Service File

```bash
sudo nano /etc/systemd/system/themisdb-pricing.service
```

```ini
[Unit]
Description=ThemisDB Enterprise Pricing Server
After=network.target postgresql.service
Requires=postgresql.service

[Service]
Type=simple
User=themisdb
Group=themisdb
WorkingDirectory=/opt/themisdb/enterprise_pricing_server
Environment="PATH=/opt/themisdb/venv/bin"
ExecStart=/opt/themisdb/venv/bin/python run_server.py
Restart=always
RestartSec=10

# Security
NoNewPrivileges=true
PrivateTmp=true
ProtectSystem=strict
ProtectHome=true
ReadWritePaths=/opt/themisdb/enterprise_pricing_server

# Logging
StandardOutput=journal
StandardError=journal
SyslogIdentifier=themisdb-pricing

[Install]
WantedBy=multi-user.target
```

### Enable and Start Service

```bash
# Reload systemd
sudo systemctl daemon-reload

# Enable service to start on boot
sudo systemctl enable themisdb-pricing

# Start service
sudo systemctl start themisdb-pricing

# Check status
sudo systemctl status themisdb-pricing

# View logs
sudo journalctl -u themisdb-pricing -f
```

---

## 5. Reverse Proxy Setup

### Option A: Nginx

```bash
sudo apt install nginx
```

Create nginx configuration:

```bash
sudo nano /etc/nginx/sites-available/themisdb-pricing
```

```nginx
upstream themisdb_pricing {
    server 127.0.0.1:6734;
    keepalive 64;
}

server {
    listen 80;
    listen [::]:80;
    server_name service.themisdb.org;
    
    # Redirect to HTTPS
    return 301 https://$server_name$request_uri;
}

server {
    listen 443 ssl http2;
    listen [::]:443 ssl http2;
    server_name service.themisdb.org;
    
    # SSL Configuration
    ssl_certificate /etc/letsencrypt/live/service.themisdb.org/fullchain.pem;
    ssl_certificate_key /etc/letsencrypt/live/service.themisdb.org/privkey.pem;
    ssl_protocols TLSv1.2 TLSv1.3;
    ssl_ciphers HIGH:!aNULL:!MD5;
    ssl_prefer_server_ciphers on;
    
    # Security Headers
    add_header X-Frame-Options "SAMEORIGIN" always;
    add_header X-Content-Type-Options "nosniff" always;
    add_header X-XSS-Protection "1; mode=block" always;
    add_header Strict-Transport-Security "max-age=31536000; includeSubDomains" always;
    
    # Logging
    access_log /var/log/nginx/themisdb-pricing.access.log;
    error_log /var/log/nginx/themisdb-pricing.error.log;
    
    # Rate Limiting
    limit_req_zone $binary_remote_addr zone=pricing_limit:10m rate=60r/m;
    limit_req zone=pricing_limit burst=20 nodelay;
    
    # Proxy Settings
    location / {
        proxy_pass http://themisdb_pricing;
        proxy_http_version 1.1;
        proxy_set_header Upgrade $http_upgrade;
        proxy_set_header Connection "upgrade";
        proxy_set_header Host $host;
        proxy_set_header X-Real-IP $remote_addr;
        proxy_set_header X-Forwarded-For $proxy_add_x_forwarded_for;
        proxy_set_header X-Forwarded-Proto $scheme;
        proxy_redirect off;
        
        # Timeouts
        proxy_connect_timeout 60s;
        proxy_send_timeout 60s;
        proxy_read_timeout 60s;
    }
    
    # Health check endpoint (bypass rate limit)
    location = /health {
        proxy_pass http://themisdb_pricing;
        access_log off;
    }
}
```

Enable site:

```bash
sudo ln -s /etc/nginx/sites-available/themisdb-pricing /etc/nginx/sites-enabled/
sudo nginx -t
sudo systemctl reload nginx
```

### Option B: Caddy (Simpler)

```bash
sudo apt install caddy
```

```bash
sudo nano /etc/caddy/Caddyfile
```

```caddy
service.themisdb.org {
    reverse_proxy localhost:6734 {
        header_up X-Real-IP {remote}
        header_up X-Forwarded-Proto {scheme}
    }
    
    rate_limit {
        zone pricing {
            key {remote_host}
            events 60
            window 1m
        }
    }
    
    encode gzip
    
    log {
        output file /var/log/caddy/themisdb-pricing.log
    }
}
```

Restart Caddy:

```bash
sudo systemctl restart caddy
```

---

## 6. SSL Certificate (Let's Encrypt)

```bash
# Install certbot
sudo apt install certbot python3-certbot-nginx

# Obtain certificate (for nginx)
sudo certbot --nginx -d service.themisdb.org

# Or for Caddy (automatic)
# Caddy automatically obtains and renews certificates
```

---

## 7. Firewall Configuration

```bash
# UFW
sudo ufw allow 80/tcp
sudo ufw allow 443/tcp
sudo ufw allow 6734/tcp  # Direct access if needed
sudo ufw enable

# Or iptables
sudo iptables -A INPUT -p tcp --dport 80 -j ACCEPT
sudo iptables -A INPUT -p tcp --dport 443 -j ACCEPT
sudo iptables -A INPUT -p tcp --dport 6734 -j ACCEPT
```

---

## 8. Monitoring & Logging

### Setup Log Rotation

```bash
sudo nano /etc/logrotate.d/themisdb-pricing
```

```
/var/log/themisdb/*.log {
    daily
    missingok
    rotate 14
    compress
    delaycompress
    notifempty
    create 0640 themisdb themisdb
    sharedscripts
    postrotate
        systemctl reload themisdb-pricing > /dev/null
    endscript
}
```

### Setup Monitoring (Optional)

```bash
# Install Prometheus node exporter
sudo apt install prometheus-node-exporter

# Or use systemd integration for metrics
systemctl status themisdb-pricing
journalctl -u themisdb-pricing --since "1 hour ago"
```

---

## 9. Backup Strategy

### Database Backups

```bash
# Create backup script
sudo nano /opt/themisdb/backup.sh
```

```bash
#!/bin/bash
BACKUP_DIR="/opt/themisdb/backups"
DATE=$(date +%Y%m%d_%H%M%S)
FILENAME="themis_pricing_$DATE.sql"

mkdir -p $BACKUP_DIR

# Backup database
pg_dump -U themis_pricing -h localhost themis_pricing > $BACKUP_DIR/$FILENAME

# Compress
gzip $BACKUP_DIR/$FILENAME

# Keep only last 30 days
find $BACKUP_DIR -name "*.sql.gz" -mtime +30 -delete

echo "Backup completed: $BACKUP_DIR/$FILENAME.gz"
```

```bash
sudo chmod +x /opt/themisdb/backup.sh
sudo chown themisdb:themisdb /opt/themisdb/backup.sh
```

### Schedule Daily Backups

```bash
sudo crontab -e -u themisdb
```

```cron
# Daily backup at 2 AM
0 2 * * * /opt/themisdb/backup.sh >> /var/log/themisdb/backup.log 2>&1
```

---

## 10. Testing Deployment

### Test Health Endpoint

```bash
curl https://service.themisdb.org/health
```

Expected:
```json
{"status":"healthy","version":"1.0.0"}
```

### Test License Validation

```bash
curl -X POST https://service.themisdb.org/license/validate \
  -H "Content-Type: application/json" \
  -d '{
    "license_key": "THEMIS-ENT-A1B2C3D4-E5F6G7H8",
    "server_hostname": "test",
    "server_version": "1.3.0"
  }'
```

### Test API Documentation

Visit: https://service.themisdb.org/docs

---

## 11. Security Checklist

- [ ] Strong database password set
- [ ] SECRET_KEY changed from default
- [ ] SSL certificate installed and valid
- [ ] Firewall configured properly
- [ ] Rate limiting enabled
- [ ] File permissions secured (chmod 600 .env)
- [ ] Service runs as non-root user
- [ ] Backups automated and tested
- [ ] Logs rotated properly
- [ ] CORS origins restricted to specific domains

---

## 12. Maintenance

### Update Application

```bash
# Stop service
sudo systemctl stop themisdb-pricing

# Backup current version
sudo -u themisdb cp -r /opt/themisdb/enterprise_pricing_server /opt/themisdb/enterprise_pricing_server.backup

# Deploy new version
sudo -u themisdb cp -r new_version/* /opt/themisdb/enterprise_pricing_server/

# Install any new dependencies
sudo -u themisdb /opt/themisdb/venv/bin/pip install -r requirements.txt

# Run migrations if any
# sudo -u themisdb /opt/themisdb/venv/bin/alembic upgrade head

# Start service
sudo systemctl start themisdb-pricing

# Check status
sudo systemctl status themisdb-pricing
```

### Database Maintenance

```bash
# Vacuum database
sudo -u postgres psql -d themis_pricing -c "VACUUM ANALYZE;"

# Check database size
sudo -u postgres psql -d themis_pricing -c "SELECT pg_size_pretty(pg_database_size('themis_pricing'));"

# List active connections
sudo -u postgres psql -d themis_pricing -c "SELECT * FROM pg_stat_activity;"
```

---

## Troubleshooting

### Service Won't Start

```bash
# Check logs
sudo journalctl -u themisdb-pricing -n 100

# Check file permissions
ls -la /opt/themisdb/enterprise_pricing_server/.env

# Test manually
sudo -u themisdb /opt/themisdb/venv/bin/python /opt/themisdb/enterprise_pricing_server/run_server.py
```

### Database Connection Issues

```bash
# Test connection
sudo -u postgres psql -d themis_pricing -c "SELECT version();"

# Check PostgreSQL status
sudo systemctl status postgresql

# Check PostgreSQL logs
sudo tail -f /var/log/postgresql/postgresql-14-main.log
```

### Port Already in Use

```bash
# Check what's using port 6734
sudo lsof -i :6734

# Kill process if needed
sudo kill <PID>
```

---

## Support

For deployment issues:
- **Email**: support@themisdb.com
- **Documentation**: https://docs.themisdb.org
- **Community**: https://github.com/makr-code/ThemisDB/discussions
