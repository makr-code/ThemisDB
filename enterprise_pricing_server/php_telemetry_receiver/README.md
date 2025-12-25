# ThemisDB Telemetry Receiver - PHP Implementation

Standalone PHP script for receiving telemetry heartbeats from ThemisDB instances. This is a lightweight alternative to the Python/FastAPI telemetry server.

## Features

- ✅ **Simple Deployment** - Single PHP file, works on any PHP-enabled web server
- ✅ **MySQL/MariaDB Backend** - Standard SQL database
- ✅ **RESTful API** - 5 endpoints for telemetry operations
- ✅ **Rate Limiting** - Prevents spam (5-minute minimum between heartbeats)
- ✅ **Security** - HTTPS enforcement, SQL injection protection, input validation
- ✅ **Statistics** - Global analytics and per-license tracking
- ✅ **Auto Cleanup** - Automatic removal of old data (90 days default)
- ✅ **Easy Integration** - Compatible with existing ThemisDB C++/Python clients

## Requirements

- PHP 7.4 or higher (PHP 8.x recommended)
- MySQL 5.7+ or MariaDB 10.3+
- PDO extension with MySQL driver
- Apache/Nginx web server
- SSL certificate for HTTPS

## Quick Start

### 1. Database Setup

```bash
# Import database schema
mysql -u root -p < setup.sql

# Create database user (edit credentials first!)
mysql -u root -p themisdb_pricing
```

Edit `setup.sql` and uncomment the user creation lines:
```sql
CREATE USER 'themisdb_user'@'localhost' IDENTIFIED BY 'YOUR_SECURE_PASSWORD';
GRANT SELECT, INSERT, UPDATE, DELETE ON themisdb_pricing.* TO 'themisdb_user'@'localhost';
FLUSH PRIVILEGES;
```

### 2. Configuration

```bash
# Copy and edit configuration
cp .env.example .env

# Edit database credentials
nano .env
```

Update `.env`:
```
DB_HOST=localhost
DB_NAME=themisdb_pricing
DB_USER=themisdb_user
DB_PASS=your_secure_password_here
REQUIRE_HTTPS=true
```

### 3. Web Server Setup

#### Apache

```apache
<VirtualHost *:443>
    ServerName service.themisdb.org
    DocumentRoot /var/www/themisdb-telemetry
    
    SSLEngine on
    SSLCertificateFile /path/to/certificate.crt
    SSLCertificateKeyFile /path/to/private.key
    
    <Directory /var/www/themisdb-telemetry>
        Options -Indexes +FollowSymLinks
        AllowOverride All
        Require all granted
        
        # Enable .htaccess for URL rewriting
        RewriteEngine On
        RewriteBase /
        
        # Security headers
        Header set X-Content-Type-Options "nosniff"
        Header set X-Frame-Options "DENY"
        Header set X-XSS-Protection "1; mode=block"
    </Directory>
    
    ErrorLog ${APACHE_LOG_DIR}/themisdb-telemetry-error.log
    CustomLog ${APACHE_LOG_DIR}/themisdb-telemetry-access.log combined
</VirtualHost>
```

#### Nginx

```nginx
server {
    listen 6734 ssl http2;
    server_name service.themisdb.org;
    
    root /var/www/themisdb-telemetry;
    index telemetry.php;
    
    ssl_certificate /path/to/certificate.crt;
    ssl_certificate_key /path/to/private.key;
    
    # Security headers
    add_header X-Content-Type-Options "nosniff" always;
    add_header X-Frame-Options "DENY" always;
    add_header X-XSS-Protection "1; mode=block" always;
    
    location / {
        try_files $uri $uri/ /telemetry.php?$query_string;
    }
    
    location ~ \.php$ {
        fastcgi_pass unix:/var/run/php/php8.2-fpm.sock;
        fastcgi_index telemetry.php;
        include fastcgi_params;
        fastcgi_param SCRIPT_FILENAME $document_root$fastcgi_script_name;
    }
    
    access_log /var/log/nginx/themisdb-telemetry-access.log;
    error_log /var/log/nginx/themisdb-telemetry-error.log;
}
```

### 4. Deploy Files

```bash
# Copy files to web server
sudo mkdir -p /var/www/themisdb-telemetry
sudo cp telemetry.php /var/www/themisdb-telemetry/
sudo cp .env /var/www/themisdb-telemetry/

# Set permissions
sudo chown -R www-data:www-data /var/www/themisdb-telemetry
sudo chmod 640 /var/www/themisdb-telemetry/.env
sudo chmod 644 /var/www/themisdb-telemetry/telemetry.php

# Restart web server
sudo systemctl restart apache2  # or nginx
```

## API Endpoints

### 1. Receive Heartbeat

```bash
POST https://service.themisdb.org:6734/telemetry.php?action=heartbeat

Content-Type: application/json
{
    "instance_id": "unique-instance-id",
    "license_key": "THEMIS-ENT-1234-ABCD",
    "metrics": {
        "nodes": 10,
        "cores": 320,
        "storage_tb": 50.5,
        "uptime_seconds": 86400,
        "query_count_24h": 1500000
    },
    "server_info": {
        "hostname": "themis-prod-01",
        "version": "2.1.0",
        "location": "eu-west-1"
    }
}
```

**Response:**
```json
{
    "status": "success",
    "message": "Heartbeat received",
    "telemetry_id": 42,
    "timestamp": "2025-12-25 08:00:00"
}
```

### 2. Get Global Statistics

```bash
GET https://service.themisdb.org:6734/telemetry.php?action=statistics
```

**Response:**
```json
{
    "total_instances": 150,
    "active_instances": 142,
    "total_nodes": 1500,
    "total_cores": 48000,
    "total_storage_tb": 7500.5,
    "total_queries_24h": 225000000,
    "version_distribution": [
        {"version": "2.1.0", "count": 85},
        {"version": "2.0.5", "count": 57}
    ],
    "tier_distribution": [
        {"tier": "enterprise", "instance_count": 92},
        {"tier": "hyperscaler", "instance_count": 50}
    ],
    "timestamp": "2025-12-25 08:00:00"
}
```

### 3. Get Instances for License

```bash
GET https://service.themisdb.org:6734/telemetry.php?action=license_instances&key=THEMIS-ENT-1234-ABCD
```

**Response:**
```json
{
    "license_key": "THEMIS-ENT-1234-ABCD",
    "instance_count": 3,
    "instances": [
        {
            "instance_id": "instance-001",
            "hostname": "themis-prod-01",
            "version": "2.1.0",
            "nodes": 10,
            "cores": 320,
            "storage_tb": 50.5,
            "is_active": true,
            "last_seen": "2025-12-25 07:55:00"
        }
    ]
}
```

### 4. Get Instance Details

```bash
GET https://service.themisdb.org:6734/telemetry.php?action=instance&id=instance-001
```

### 5. Clean Old Data

```bash
POST https://service.themisdb.org:6734/telemetry.php?action=cleanup

Content-Type: application/json
{
    "days": 90
}
```

## Security

### HTTPS Enforcement

The script requires HTTPS by default. For development only:
```bash
export REQUIRE_HTTPS=false
```

### Rate Limiting

Minimum 5 minutes between heartbeats from the same instance. Prevents spam and excessive database writes.

### SQL Injection Protection

All queries use prepared statements with PDO. User input is validated and sanitized.

### CORS Configuration

Default allows all origins (`*`). **Restrict in production:**

```php
// In telemetry.php, line ~38
header('Access-Control-Allow-Origin: https://your-admin-dashboard.com');
```

## Monitoring

### Enable PHP Error Logging

```ini
; php.ini
error_reporting = E_ALL
log_errors = On
error_log = /var/log/php/error.log
```

### Database Health Check

```sql
-- Check active instances
SELECT COUNT(*) FROM active_instances;

-- View statistics
SELECT * FROM telemetry_statistics;

-- Recent heartbeats
SELECT instance_id, hostname, last_seen 
FROM instance_telemetry 
ORDER BY last_seen DESC 
LIMIT 10;
```

### Automatic Cleanup

MySQL Event Scheduler automatically cleans old data daily at 2 AM:

```sql
-- Enable event scheduler
SET GLOBAL event_scheduler = ON;

-- Check events
SHOW EVENTS;

-- Manual cleanup
CALL cleanup_old_telemetry(90);
```

## Client Integration

Use the same C++ clients from `TELEMETRY_INTEGRATION.md`. Simply point to PHP endpoint:

```cpp
// C++ example (libcurl)
std::string url = "https://service.themisdb.org:6734/telemetry.php?action=heartbeat";
```

```python
# Python example
url = "https://service.themisdb.org:6734/telemetry.php?action=heartbeat"
response = requests.post(url, json=payload)
```

## Testing

```bash
# Test heartbeat
curl -X POST https://service.themisdb.org:6734/telemetry.php?action=heartbeat \
  -H "Content-Type: application/json" \
  -d '{
    "instance_id": "test-001",
    "license_key": "THEMIS-ENT-1234-TEST2",
    "metrics": {
        "nodes": 5,
        "cores": 160,
        "storage_tb": 10.0,
        "uptime_seconds": 3600,
        "query_count_24h": 100000
    }
  }'

# Get statistics
curl https://service.themisdb.org:6734/telemetry.php?action=statistics

# Get instances
curl https://service.themisdb.org:6734/telemetry.php?action=license_instances&key=THEMIS-ENT-1234-TEST2
```

## Performance

- **Upsert Pattern**: Uses SELECT + UPDATE/INSERT for reliable operation
- **Indexes**: Optimized for quick lookups by instance_id, license_key, last_seen
- **Connection Pooling**: PDO persistent connections recommended for high traffic
- **Caching**: Consider adding Redis/Memcached for statistics endpoint

### Optimization for High Traffic

```ini
; php.ini
opcache.enable=1
opcache.memory_consumption=128
opcache.max_accelerated_files=10000
```

```ini
; PHP-FPM pool config
pm = dynamic
pm.max_children = 50
pm.start_servers = 5
pm.min_spare_servers = 5
pm.max_spare_servers = 35
```

## Troubleshooting

### Database Connection Failed

Check `.env` credentials and MySQL service:
```bash
systemctl status mysql
mysql -u themisdb_user -p themisdb_pricing
```

### HTTPS Required Error

Development only:
```bash
export REQUIRE_HTTPS=false
```

### Rate Limit Exceeded

Wait 5 minutes or adjust `RATE_LIMIT_SECONDS` in `telemetry.php`.

### 500 Internal Server Error

Check PHP error log:
```bash
tail -f /var/log/php/error.log
# or
tail -f /var/log/apache2/themisdb-telemetry-error.log
```

## Comparison: PHP vs Python/FastAPI

| Feature | PHP | Python/FastAPI |
|---------|-----|----------------|
| Deployment | Simple (single file) | Complex (WSGI/ASGI) |
| Performance | Good (PHP 8.x) | Excellent (async) |
| Hosting | Any PHP host | Requires Python env |
| Database | MySQL native | SQLAlchemy ORM |
| Scalability | Horizontal (easy) | Horizontal + async |
| Maintenance | Low | Medium |

**Recommendation:** Use PHP for simple deployments on shared hosting. Use Python/FastAPI for high-traffic production with advanced features.

## License

Part of ThemisDB Enterprise Pricing Server
