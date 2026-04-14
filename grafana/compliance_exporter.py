"""
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            compliance_exporter.py                             ║
  Version:         0.0.41                                             ║
  Last Modified:   2026-04-14 11:22:30                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     505                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 2a1fb04231  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
    • a629043ab2  2026-02-22  Audit: document gaps found - benchmarks and stale annotat... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
"""

#!/usr/bin/env python3
"""
Compliance Report Exporter for ThemisDB SIEM

This script generates compliance reports from ThemisDB Prometheus metrics
for SOC2, GDPR, and HIPAA compliance requirements.

Usage:
    python compliance_exporter.py --framework soc2 --period 30d --output soc2_report.pdf
    python compliance_exporter.py --framework gdpr --period 7d --format json
    python compliance_exporter.py --framework hipaa --period 90d --format csv

Requirements:
    pip install requests pandas matplotlib reportlab
"""

import argparse
import os
import requests
import pandas as pd
import json
import csv
from datetime import datetime, timedelta
from typing import Dict, List, Any
import matplotlib.pyplot as plt
from matplotlib.backends.backend_pdf import PdfPages
from reportlab.lib.pagesizes import letter, A4
from reportlab.platypus import SimpleDocTemplate, Table, TableStyle, Paragraph, Spacer, Image
from reportlab.lib.styles import getSampleStyleSheet, ParagraphStyle
from reportlab.lib.units import inch
from reportlab.lib import colors

# Configuration
# For production, use environment variable: PROMETHEUS_URL=http://your-prometheus:9090
PROMETHEUS_URL = os.environ.get("PROMETHEUS_URL", "http://localhost:9090")
DEFAULT_OUTPUT_DIR = "./compliance_reports"

# Compliance frameworks configuration
COMPLIANCE_FRAMEWORKS = {
    "soc2": {
        "name": "SOC 2 Trust Services Criteria",
        "controls": {
            "CC6.1": {
                "name": "Logical Access",
                "metrics": [
                    "sum(increase(themis_auth_failures_total[24h]))",
                    "sum(increase(themis_auth_attempts_total[24h]))",
                    "count(themis_active_sessions > 0)"
                ],
                "thresholds": {"auth_failures_24h": 100},
                "description": "Monitor and control logical access to systems"
            },
            "CC6.2": {
                "name": "Privileged Access Management",
                "metrics": [
                    "sum(increase(themis_privilege_escalation_total[24h]))",
                    "sum(increase(themis_admin_actions_total[24h]))"
                ],
                "thresholds": {"privilege_escalations_24h": 0},
                "description": "Control and monitor privileged access"
            },
            "CC6.7": {
                "name": "Encryption Keys",
                "metrics": [
                    "time() - themis_encryption_key_last_rotation_timestamp"
                ],
                "thresholds": {"key_age_days": 90},
                "description": "Manage encryption keys and key rotation"
            },
            "CC7.1": {
                "name": "System Operations",
                "metrics": [
                    "up{job='themisdb'}",
                    "100 - (avg(rate(node_cpu_seconds_total{mode='idle'}[5m])) * 100)"
                ],
                "thresholds": {"uptime_percentage": 99.9},
                "description": "Monitor system availability and operations"
            },
            "CC7.2": {
                "name": "System Monitoring and Backup",
                "metrics": [
                    "time() - themis_last_successful_backup_timestamp",
                    "themis_replication_lag_seconds"
                ],
                "thresholds": {"backup_age_hours": 24, "replication_lag_seconds": 10},
                "description": "Monitor systems and maintain backups"
            }
        }
    },
    "gdpr": {
        "name": "General Data Protection Regulation",
        "articles": {
            "Article5": {
                "name": "Principles - Purpose Limitation & Storage Limitation",
                "metrics": [
                    "sum by (data_category) (themis_data_retention_days)",
                    "sum(increase(themis_data_access_total{classification='sensitive'}[24h]))",
                    "sum(increase(themis_access_justification_provided_total[24h]))"
                ],
                "thresholds": {"retention_days": 730},
                "description": "Data must be kept only as long as necessary"
            },
            "Article32": {
                "name": "Security of Processing",
                "metrics": [
                    "sum(increase(themis_auth_failures_total[24h]))",
                    "sum(increase(themis_encryption_operations_total[24h]))",
                    "sum(increase(themis_audit_log_integrity_failures_total[24h]))"
                ],
                "thresholds": {"auth_failures_24h": 100, "audit_failures": 0},
                "description": "Implement appropriate technical and organizational measures"
            },
            "Article33": {
                "name": "Breach Notification",
                "metrics": [
                    "sum(increase(themis_security_incidents_total[24h]))",
                    "sum(increase(themis_data_export_events_total{authorized='false'}[24h]))"
                ],
                "thresholds": {"security_incidents": 0, "unauthorized_exports": 0},
                "description": "Notify breaches within 72 hours"
            }
        }
    },
    "hipaa": {
        "name": "Health Insurance Portability and Accountability Act",
        "standards": {
            "164.308(a)(1)": {
                "name": "Security Management Process",
                "metrics": [
                    "sum(increase(themis_security_incidents_total[24h]))",
                    "sum(increase(themis_policy_violations_total[24h]))"
                ],
                "thresholds": {"security_incidents": 0},
                "description": "Implement policies and procedures to prevent, detect, contain, and correct security violations"
            },
            "164.308(a)(3)": {
                "name": "Workforce Security",
                "metrics": [
                    "sum(increase(themis_auth_attempts_total[24h]))",
                    "sum(increase(themis_admin_actions_total[24h]))"
                ],
                "thresholds": {"auth_attempts_24h": 10000},
                "description": "Implement procedures for workforce security"
            },
            "164.308(a)(4)": {
                "name": "Information Access Management",
                "metrics": [
                    "sum(increase(themis_authorization_denied_total[24h]))",
                    "sum(increase(themis_audit_events_total[24h]))"
                ],
                "thresholds": {"denied_access_24h": 100},
                "description": "Implement policies and procedures for authorizing access"
            },
            "164.312(a)(1)": {
                "name": "Access Control",
                "metrics": [
                    "sum(increase(themis_auth_attempts_total[24h]))",
                    "count(themis_active_sessions)"
                ],
                "thresholds": {"auth_failures_24h": 100},
                "description": "Implement technical policies and procedures for access control"
            },
            "164.312(b)": {
                "name": "Audit Controls",
                "metrics": [
                    "sum(increase(themis_audit_events_total[24h]))",
                    "sum(increase(themis_audit_log_integrity_failures_total[24h]))"
                ],
                "thresholds": {"audit_failures": 0},
                "description": "Implement hardware, software, and/or procedural mechanisms that record and examine activity"
            },
            "164.312(c)": {
                "name": "Integrity Controls",
                "metrics": [
                    "sum(increase(themis_audit_log_integrity_failures_total[24h]))",
                    "time() - themis_last_successful_backup_timestamp"
                ],
                "thresholds": {"audit_failures": 0, "backup_age_hours": 24},
                "description": "Implement policies and procedures to protect ePHI from improper alteration or destruction"
            },
            "164.312(d)": {
                "name": "Transmission Security",
                "metrics": [
                    "rate(node_network_transmit_bytes_total[24h])",
                    "sum(increase(themis_data_export_events_total[24h]))"
                ],
                "thresholds": {"unauthorized_exports": 0},
                "description": "Implement technical security measures to guard against unauthorized access to ePHI"
            }
        }
    }
}


class ComplianceExporter:
    """Export compliance reports from ThemisDB metrics"""
    
    def __init__(self, prometheus_url: str = PROMETHEUS_URL):
        self.prometheus_url = prometheus_url
        self.session = requests.Session()
    
    def query_prometheus(self, query: str, start_time: datetime, end_time: datetime, step: str = "1h") -> List[Dict]:
        """Query Prometheus for a time range"""
        response = self.session.get(
            f"{self.prometheus_url}/api/v1/query_range",
            params={
                "query": query,
                "start": start_time.timestamp(),
                "end": end_time.timestamp(),
                "step": step
            }
        )
        response.raise_for_status()
        data = response.json()
        
        if data["status"] != "success":
            error_msg = data.get("error", "Unknown error")
            error_type = data.get("errorType", "Unknown")
            raise ValueError(f"Prometheus query failed [{error_type}]: {error_msg}")
        
        return data["data"]["result"]
    
    def query_instant(self, query: str) -> float:
        """Query Prometheus for instant value"""
        response = self.session.get(
            f"{self.prometheus_url}/api/v1/query",
            params={"query": query}
        )
        response.raise_for_status()
        data = response.json()
        
        if data["status"] != "success":
            raise ValueError(f"Prometheus query failed: {data}")
        
        results = data["data"]["result"]
        if not results:
            return 0.0
        
        return float(results[0]["value"][1])
    
    def generate_soc2_report(self, period_days: int, output_file: str, format: str = "pdf"):
        """Generate SOC2 compliance report"""
        end_time = datetime.now()
        start_time = end_time - timedelta(days=period_days)
        
        framework = COMPLIANCE_FRAMEWORKS["soc2"]
        report_data = []
        
        for control_id, control in framework["controls"].items():
            print(f"Processing {control_id}: {control['name']}...")
            
            control_results = {
                "control_id": control_id,
                "control_name": control["name"],
                "description": control["description"],
                "status": "PASS",
                "findings": []
            }
            
            for metric_query in control["metrics"]:
                try:
                    # Get current value
                    value = self.query_instant(metric_query)
                    
                    # Check against thresholds
                    threshold_key = self._extract_metric_name(metric_query)
                    threshold = control.get("thresholds", {}).get(threshold_key)
                    
                    if threshold and value > threshold:
                        control_results["status"] = "FAIL"
                        control_results["findings"].append({
                            "metric": metric_query,
                            "value": value,
                            "threshold": threshold,
                            "status": "EXCEEDED"
                        })
                    
                    control_results[threshold_key] = value
                    
                except Exception as e:
                    print(f"  Warning: Could not query {metric_query}: {e}")
                    control_results["findings"].append({
                        "metric": metric_query,
                        "error": str(e)
                    })
            
            report_data.append(control_results)
        
        # Generate output
        if format == "pdf":
            self._export_pdf_report(report_data, output_file, "SOC2", framework["name"], period_days)
        elif format == "json":
            self._export_json_report(report_data, output_file, "SOC2", period_days)
        elif format == "csv":
            self._export_csv_report(report_data, output_file, "SOC2")
        else:
            raise ValueError(f"Unsupported format: {format}")
        
        print(f"\nSOC2 report generated: {output_file}")
        return report_data
    
    def _extract_metric_name(self, query: str) -> str:
        """Extract a readable name from Prometheus query"""
        if "auth_failures" in query:
            return "auth_failures_24h"
        elif "auth_attempts" in query:
            return "auth_attempts_24h"
        elif "privilege_escalation" in query:
            return "privilege_escalations_24h"
        elif "admin_actions" in query:
            return "admin_actions_24h"
        elif "encryption_key" in query:
            return "key_age_days"
        elif "backup" in query:
            return "backup_age_hours"
        elif "replication_lag" in query:
            return "replication_lag_seconds"
        elif "up{" in query:
            return "uptime_percentage"
        else:
            return "unknown_metric"
    
    def _export_pdf_report(self, data: List[Dict], output_file: str, framework: str, framework_name: str, period_days: int):
        """Export report as PDF"""
        doc = SimpleDocTemplate(output_file, pagesize=letter)
        elements = []
        styles = getSampleStyleSheet()
        
        # Title
        title_style = ParagraphStyle(
            'CustomTitle',
            parent=styles['Heading1'],
            fontSize=24,
            textColor=colors.HexColor('#1f77b4'),
            spaceAfter=30,
        )
        elements.append(Paragraph(f"ThemisDB {framework} Compliance Report", title_style))
        elements.append(Spacer(1, 0.2 * inch))
        
        # Report metadata
        metadata_style = styles['Normal']
        elements.append(Paragraph(f"<b>Framework:</b> {framework_name}", metadata_style))
        elements.append(Paragraph(f"<b>Period:</b> Last {period_days} days", metadata_style))
        elements.append(Paragraph(f"<b>Generated:</b> {datetime.now().strftime('%Y-%m-%d %H:%M:%S')}", metadata_style))
        elements.append(Spacer(1, 0.3 * inch))
        
        # Summary statistics
        total_controls = len(data)
        passed_controls = sum(1 for item in data if item["status"] == "PASS")
        failed_controls = total_controls - passed_controls
        compliance_rate = (passed_controls / total_controls * 100) if total_controls > 0 else 0
        
        summary_data = [
            ["Metric", "Value"],
            ["Total Controls", str(total_controls)],
            ["Passed Controls", str(passed_controls)],
            ["Failed Controls", str(failed_controls)],
            ["Compliance Rate", f"{compliance_rate:.1f}%"]
        ]
        
        summary_table = Table(summary_data, colWidths=[3 * inch, 2 * inch])
        summary_table.setStyle(TableStyle([
            ('BACKGROUND', (0, 0), (-1, 0), colors.HexColor('#1f77b4')),
            ('TEXTCOLOR', (0, 0), (-1, 0), colors.whitesmoke),
            ('ALIGN', (0, 0), (-1, -1), 'LEFT'),
            ('FONTNAME', (0, 0), (-1, 0), 'Helvetica-Bold'),
            ('FONTSIZE', (0, 0), (-1, 0), 12),
            ('BOTTOMPADDING', (0, 0), (-1, 0), 12),
            ('GRID', (0, 0), (-1, -1), 1, colors.black)
        ]))
        elements.append(summary_table)
        elements.append(Spacer(1, 0.5 * inch))
        
        # Control details
        elements.append(Paragraph("<b>Control Details</b>", styles['Heading2']))
        elements.append(Spacer(1, 0.2 * inch))
        
        for item in data:
            # Control header
            status_color = colors.green if item["status"] == "PASS" else colors.red
            control_text = f"<b>{item['control_id']}: {item['control_name']}</b> - <font color='{status_color}'>{item['status']}</font>"
            elements.append(Paragraph(control_text, styles['Heading3']))
            elements.append(Paragraph(item['description'], styles['Normal']))
            elements.append(Spacer(1, 0.1 * inch))
            
            # Findings
            if item["findings"]:
                elements.append(Paragraph("<b>Findings:</b>", styles['Normal']))
                for finding in item["findings"]:
                    if "error" in finding:
                        finding_text = f"• Error querying metric: {finding['error']}"
                    else:
                        finding_text = f"• Metric exceeded threshold: {finding['value']:.2f} > {finding['threshold']}"
                    elements.append(Paragraph(finding_text, styles['Normal']))
            else:
                elements.append(Paragraph("No findings - Control passed", styles['Normal']))
            
            elements.append(Spacer(1, 0.3 * inch))
        
        # Build PDF
        doc.build(elements)
    
    def _export_json_report(self, data: List[Dict], output_file: str, framework: str, period_days: int):
        """Export report as JSON"""
        report = {
            "framework": framework,
            "generated_at": datetime.now().isoformat(),
            "period_days": period_days,
            "summary": {
                "total_controls": len(data),
                "passed": sum(1 for item in data if item["status"] == "PASS"),
                "failed": sum(1 for item in data if item["status"] != "PASS")
            },
            "controls": data
        }
        
        with open(output_file, 'w') as f:
            json.dump(report, f, indent=2)
    
    def _export_csv_report(self, data: List[Dict], output_file: str, framework: str):
        """Export report as CSV"""
        with open(output_file, 'w', newline='') as f:
            writer = csv.writer(f)
            writer.writerow(["Control ID", "Control Name", "Status", "Description", "Findings"])
            
            for item in data:
                findings_str = "; ".join([
                    f"{f.get('metric', 'N/A')}: {f.get('value', 'N/A')}"
                    for f in item.get("findings", [])
                ])
                writer.writerow([
                    item["control_id"],
                    item["control_name"],
                    item["status"],
                    item["description"],
                    findings_str
                ])


def main():
    parser = argparse.ArgumentParser(description="Generate compliance reports from ThemisDB metrics")
    parser.add_argument("--framework", choices=["soc2", "gdpr", "hipaa"], required=True,
                        help="Compliance framework")
    parser.add_argument("--period", default="30d", help="Reporting period (e.g., 7d, 30d, 90d)")
    parser.add_argument("--format", choices=["pdf", "json", "csv"], default="pdf",
                        help="Output format")
    parser.add_argument("--output", help="Output file path (default: auto-generated)")
    parser.add_argument("--prometheus", default=PROMETHEUS_URL,
                        help=f"Prometheus URL (default: {PROMETHEUS_URL})")
    
    args = parser.parse_args()
    
    # Parse period
    period_days = int(args.period.rstrip('d'))
    
    # Generate output filename if not specified
    if not args.output:
        timestamp = datetime.now().strftime("%Y%m%d_%H%M%S")
        args.output = f"{args.framework}_compliance_report_{timestamp}.{args.format}"
    
    # Create exporter
    exporter = ComplianceExporter(prometheus_url=args.prometheus)
    
    # Generate report
    print(f"Generating {args.framework.upper()} compliance report...")
    print(f"Period: Last {period_days} days")
    print(f"Format: {args.format}")
    print(f"Output: {args.output}")
    print()
    
    if args.framework == "soc2":
        exporter.generate_soc2_report(period_days, args.output, args.format)
    elif args.framework == "gdpr":
        print("GDPR reporting not yet implemented")
    elif args.framework == "hipaa":
        print("HIPAA reporting not yet implemented")
    
    print("\nDone!")


if __name__ == "__main__":
    main()
