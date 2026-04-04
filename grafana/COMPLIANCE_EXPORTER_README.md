# Compliance Report Exporter

Automated compliance reporting tool for ThemisDB SIEM metrics.

## Features

- **SOC2 Compliance Reports**: Trust Services Criteria (CC6.1, CC6.2, CC6.7, CC7.1, CC7.2)
- **GDPR Compliance Reports**: Articles 5, 32, 33 (Coming soon)
- **HIPAA Compliance Reports**: Security Rule standards (Coming soon)
- **Multiple Export Formats**: PDF, JSON, CSV
- **Automated Threshold Checking**: Pass/Fail status based on compliance requirements
- **Customizable Periods**: 7, 30, 90 days or custom

## Installation

```bash
# Install dependencies
pip install requests pandas matplotlib reportlab

# Or using requirements file
pip install -r requirements.txt
```

### requirements.txt

```
requests>=2.28.0
pandas>=1.5.0
matplotlib>=3.6.0
reportlab>=3.6.0
```

## Usage

### Basic Usage

```bash
# Generate SOC2 report for last 30 days (PDF)
python compliance_exporter.py --framework soc2 --period 30d

# Generate SOC2 report for last 7 days (JSON)
python compliance_exporter.py --framework soc2 --period 7d --format json

# Generate SOC2 report for last 90 days (CSV)
python compliance_exporter.py --framework soc2 --period 90d --format csv --output my_report.csv
```

### Advanced Usage

```bash
# Custom Prometheus URL
python compliance_exporter.py \
  --framework soc2 \
  --period 30d \
  --prometheus http://prometheus.example.com:9090

# Automated daily reports
0 2 * * * /usr/bin/python3 /path/to/compliance_exporter.py --framework soc2 --period 1d --output /reports/daily/soc2_$(date +\%Y\%m\%d).pdf
```

## Output Formats

### PDF Report

Professional PDF report with:
- Executive summary with compliance rate
- Control-by-control breakdown
- Pass/Fail status with visual indicators
- Detailed findings for failed controls
- Compliance thresholds and actual values

**Example**: `soc2_compliance_report_20260127_143000.pdf`

### JSON Report

Machine-readable format for:
- Integration with other tools
- Programmatic analysis
- API consumption
- Historical tracking

**Example**:
```json
{
  "framework": "SOC2",
  "generated_at": "2026-01-27T14:30:00",
  "period_days": 30,
  "summary": {
    "total_controls": 5,
    "passed": 4,
    "failed": 1
  },
  "controls": [
    {
      "control_id": "CC6.1",
      "control_name": "Logical Access",
      "status": "PASS",
      "description": "Monitor and control logical access to systems",
      "findings": []
    }
  ]
}
```

### CSV Report

Spreadsheet-compatible format for:
- Excel/Google Sheets import
- Database loading
- Custom analysis
- Management reporting

**Example**:
```csv
Control ID,Control Name,Status,Description,Findings
CC6.1,Logical Access,PASS,Monitor and control logical access to systems,
CC6.2,Privileged Access Management,FAIL,Control and monitor privileged access,auth_failures: 150
```

## Compliance Frameworks

### SOC2 Trust Services Criteria

| Control | Name | Metrics Monitored | Threshold |
|---------|------|-------------------|-----------|
| **CC6.1** | Logical Access | Authentication attempts, failures, active sessions | <100 failures/24h |
| **CC6.2** | Privileged Access | Privilege escalations, admin actions | 0 unauthorized escalations |
| **CC6.7** | Encryption Keys | Key rotation age | <90 days |
| **CC7.1** | System Operations | Uptime, CPU usage | >99.9% uptime |
| **CC7.2** | System Monitoring | Backup age, replication lag | <24h since backup |

### GDPR (Coming Soon)

- **Article 5**: Purpose & Storage Limitation
- **Article 32**: Security of Processing
- **Article 33**: Breach Notification

### HIPAA (Coming Soon)

- **164.308(a)(1)**: Security Management
- **164.308(a)(3)**: Workforce Security
- **164.312(a)(1)**: Access Control
- **164.312(b)**: Audit Controls

## Configuration

### Custom Thresholds

Edit `compliance_exporter.py` to customize thresholds:

```python
"thresholds": {
    "auth_failures_24h": 100,  # Change to 50 for stricter control
    "backup_age_hours": 24     # Change to 12 for more frequent backups
}
```

### Custom Metrics

Add new metrics to existing controls:

```python
"metrics": [
    "sum(increase(themis_auth_failures_total[24h]))",
    "sum(increase(your_custom_metric[24h]))"  # Add your metric
]
```

### Add New Controls

```python
"CC6.8": {
    "name": "Your Custom Control",
    "metrics": ["your_prometheus_query"],
    "thresholds": {"your_metric": 100},
    "description": "Description of your control"
}
```

## Scheduling Automated Reports

### Using Cron

```bash
# Daily SOC2 report at 2 AM
0 2 * * * python3 /opt/themisdb/grafana/compliance_exporter.py --framework soc2 --period 1d --output /reports/daily/$(date +\%Y\%m\%d).pdf

# Weekly comprehensive report on Mondays at 6 AM
0 6 * * 1 python3 /opt/themisdb/grafana/compliance_exporter.py --framework soc2 --period 7d --output /reports/weekly/soc2_week_$(date +\%V).pdf

# Monthly report on 1st of each month
0 3 1 * * python3 /opt/themisdb/grafana/compliance_exporter.py --framework soc2 --period 30d --output /reports/monthly/soc2_$(date +\%Y\%m).pdf
```

### Using systemd Timer

Create `/etc/systemd/system/themisdb-compliance-report.service`:

```ini
[Unit]
Description=ThemisDB SOC2 Compliance Report
After=network.target

[Service]
Type=oneshot
User=themisdb
WorkingDirectory=/opt/themisdb/grafana
ExecStart=/usr/bin/python3 compliance_exporter.py --framework soc2 --period 30d --output /reports/soc2_latest.pdf
StandardOutput=journal
StandardError=journal
```

Create `/etc/systemd/system/themisdb-compliance-report.timer`:

```ini
[Unit]
Description=Daily ThemisDB Compliance Report
Requires=themisdb-compliance-report.service

[Timer]
OnCalendar=daily
OnCalendar=02:00
Persistent=true

[Install]
WantedBy=timers.target
```

Enable and start:

```bash
sudo systemctl enable themisdb-compliance-report.timer
sudo systemctl start themisdb-compliance-report.timer
sudo systemctl status themisdb-compliance-report.timer
```

## Troubleshooting

### Connection Refused

```
Error: Connection refused to http://localhost:9090
```

**Solution**: Verify Prometheus is running and accessible:
```bash
curl http://localhost:9090/api/v1/status/config
```

### No Data for Metrics

```
Warning: Could not query themis_auth_failures_total: no data
```

**Solution**: 
1. Check ThemisDB is exporting metrics:
   ```bash
   curl http://localhost:9091/metrics | grep themis_
   ```
2. Verify Prometheus is scraping ThemisDB:
   ```bash
   curl http://localhost:9090/api/v1/targets
   ```

### PDF Generation Fails

```
ImportError: No module named 'reportlab'
```

**Solution**: Install required dependencies:
```bash
pip install reportlab matplotlib
```

### Permission Denied

```
PermissionError: [Errno 13] Permission denied: 'soc2_report.pdf'
```

**Solution**: Ensure write permissions for output directory:
```bash
chmod 755 /reports
chown themisdb:themisdb /reports
```

## Integration Examples

### Email Reports

```bash
#!/bin/bash
# Generate and email report

REPORT_FILE="/tmp/soc2_report_$(date +%Y%m%d).pdf"

python3 compliance_exporter.py \
  --framework soc2 \
  --period 30d \
  --output "$REPORT_FILE"

if [ $? -eq 0 ]; then
  echo "SOC2 Compliance Report attached" | \
    mail -s "ThemisDB SOC2 Compliance Report - $(date +%Y-%m-%d)" \
         -a "$REPORT_FILE" \
         compliance@example.com
  rm "$REPORT_FILE"
fi
```

### Slack Notifications

```python
import requests
import json

# Generate report
exporter = ComplianceExporter()
report_data = exporter.generate_soc2_report(30, "report.json", format="json")

# Load JSON
with open("report.json") as f:
    report = json.load(f)

# Send to Slack
slack_webhook = "https://hooks.slack.com/services/YOUR/WEBHOOK/URL"
message = {
    "text": f"SOC2 Compliance Report",
    "attachments": [
        {
            "color": "good" if report["summary"]["failed"] == 0 else "danger",
            "fields": [
                {"title": "Total Controls", "value": str(report["summary"]["total_controls"]), "short": True},
                {"title": "Passed", "value": str(report["summary"]["passed"]), "short": True},
                {"title": "Failed", "value": str(report["summary"]["failed"]), "short": True}
            ]
        }
    ]
}
requests.post(slack_webhook, json=message)
```

### API Integration

```python
# Upload report to compliance management system
import requests

with open("soc2_report.pdf", "rb") as f:
    files = {"file": f}
    response = requests.post(
        "https://compliance-api.example.com/reports",
        files=files,
        headers={"Authorization": "Bearer YOUR_API_TOKEN"},
        data={
            "framework": "SOC2",
            "period": "30d",
            "generated_at": datetime.now().isoformat()
        }
    )
    print(f"Upload status: {response.status_code}")
```

## Best Practices

### Report Retention

- **Daily Reports**: Keep for 30 days
- **Weekly Reports**: Keep for 6 months
- **Monthly Reports**: Keep for 7 years (SOC2 requirement)
- **Audit Reports**: Keep indefinitely

### Automation

- Schedule reports during low-traffic hours (2-4 AM)
- Always generate multiple formats (PDF for human review, JSON for archival)
- Set up monitoring for report generation failures
- Rotate old reports automatically

### Security

- Restrict access to report output directory
- Encrypt reports at rest (especially for HIPAA)
- Use secure channels for report distribution
- Audit access to compliance reports
- Never commit reports to version control

## Support

- **Documentation**: See `docs/en/observability/siem_integration.md`
- **Issues**: https://github.com/makr-code/ThemisDB/issues
- **Security**: security@themisdb.io

## License

Same as ThemisDB project.
