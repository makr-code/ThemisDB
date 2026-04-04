# Themis.ComplianceReports - Compliance Report Generation

## Overview

Desktop application for generating comprehensive compliance reports for various regulatory frameworks and standards. Automates evidence collection and documentation for audits.

## Status

🚧 **Under Development** - Structure and specification complete, implementation in progress.

## Use Cases

- Generate regulatory compliance reports
- Automate audit preparation
- Document security controls
- Track compliance metrics over time
- Export evidence for external auditors
- Continuous compliance monitoring

## Requirements

- .NET 8.0+
- Windows, Linux, or macOS
- Access to ThemisDB server with admin privileges
- Read access to audit logs and system metrics

## Supported Frameworks

### Data Privacy Regulations

#### GDPR (General Data Protection Regulation)

**Report Sections:**
- Data processing activities (Article 30)
- Lawful basis for processing
- Consent management records
- Data subject rights requests (access, erasure, portability)
- Data breach notifications (Article 33)
- Data Protection Impact Assessments (DPIA)
- Cross-border data transfers
- Third-party processor agreements

**Evidence Collected:**
- Personal data inventory
- Consent records and timestamps
- DSAR fulfillment logs
- Breach notification timelines
- Encryption status

#### CCPA (California Consumer Privacy Act)

**Report Sections:**
- Categories of personal information collected
- Business purposes for collection
- Consumer rights requests
- Sales and sharing disclosures
- Opt-out mechanisms
- Data retention policies

#### PIPEDA (Canada's Privacy Law)

**Report Sections:**
- Purpose specification
- Consent documentation
- Safeguards and security measures
- Access request handling
- Third-party disclosure

### Healthcare Regulations

#### HIPAA (Health Insurance Portability and Accountability Act)

**Report Sections:**
- PHI access logs and audit trails
- Administrative safeguards
- Physical safeguards
- Technical safeguards
  - Access controls
  - Encryption and decryption
  - Audit controls
  - Integrity controls
  - Transmission security
- Business Associate Agreements (BAAs)
- Breach notification compliance
- Security risk assessments

**Evidence Collected:**
- PHI access logs (who, what, when, where)
- Encryption status for PHI at rest and in transit
- User authentication records
- Automatic logoff configurations
- Emergency access procedures

### Financial Regulations

#### PCI-DSS (Payment Card Industry Data Security Standard)

**Report Sections:**
- Requirement 1: Network security controls
- Requirement 2: Secure configurations
- Requirement 3: Cardholder data protection
  - Data retention and disposal
  - Encryption of stored data
  - Key management procedures
- Requirement 4: Encryption in transit
- Requirement 6: Secure systems
- Requirement 7: Access controls
- Requirement 8: Authentication
- Requirement 9: Physical access
- Requirement 10: Monitoring and logging
- Requirement 11: Security testing
- Requirement 12: Information security policy

**Evidence Collected:**
- Cardholder data environment (CDE) inventory
- Encryption key rotation logs
- Access control matrices
- Vulnerability scan results
- Penetration test summaries

### Security & IT Governance

#### SOC 2 Type II

**Report Sections:**
- Security: System protection controls
- Availability: System uptime and recovery
- Processing Integrity: Data processing accuracy
- Confidentiality: Protection of confidential data
- Privacy: Personal information handling

**Trust Service Criteria:**
- Control environment
- Communication and information
- Risk assessment
- Monitoring activities
- Control activities

**Evidence Collected:**
- System uptime metrics
- Incident response logs
- Change management records
- Access reviews
- Backup and recovery tests

#### ISO 27001 (Information Security Management)

**Report Sections:**
- A.5: Information security policies
- A.6: Organization of information security
- A.7: Human resource security
- A.8: Asset management
- A.9: Access control
- A.10: Cryptography
- A.12: Operations security
- A.13: Communications security
- A.14: System acquisition, development, and maintenance
- A.16: Information security incident management
- A.17: Business continuity management
- A.18: Compliance

**Evidence Collected:**
- Security policies and procedures
- Risk treatment plans
- Asset registers
- Access control lists
- Incident logs
- Business continuity plans

## Installation

```bash
cd tools/Themis.ComplianceReports
dotnet restore
dotnet build
dotnet run
```

## Configuration

Edit `appsettings.json`:

```json
{
  "ThemisDB": {
    "BaseUrl": "http://localhost:8080",
    "BearerToken": "your-admin-token"
  },
  "ComplianceReports": {
    "OutputDirectory": "./reports",
    "DefaultFormat": "PDF",
    "IncludeRawData": false,
    "DigitalSignature": true,
    "Timestamp": true
  },
  "Frameworks": {
    "GDPR": {
      "Enabled": true,
      "DataController": "Your Organization Name",
      "DPOContact": "dpo@example.com"
    },
    "HIPAA": {
      "Enabled": true,
      "CoveredEntity": "Your Healthcare Org",
      "SecurityOfficer": "security@example.com"
    },
    "PCI-DSS": {
      "Enabled": true,
      "MerchantLevel": "1",
      "QSACompany": "Your QSA"
    }
  },
  "Schedule": {
    "AutoGenerate": true,
    "Frequency": "Monthly",
    "Recipients": ["compliance@example.com", "audit@example.com"]
  }
}
```

## Basic Usage

### Generate Report via UI

1. **Select Framework**
   - Choose regulatory framework (GDPR, HIPAA, PCI-DSS, etc.)
   - Select report template
   - Configure date range

2. **Configure Report**
   - Select sections to include
   - Choose evidence level (summary, detailed, full)
   - Set format (PDF, CSV, JSON, Excel)

3. **Generate Report**
   - Click "Generate Report"
   - Monitor progress
   - Review generated report

4. **Export & Share**
   - Download report
   - Digitally sign (optional)
   - Share with auditors

### Generate Report via CLI

```bash
# Generate GDPR compliance report
dotnet run -- --framework gdpr --start-date 2026-01-01 --end-date 2026-01-31 --output gdpr-report-jan-2026.pdf

# Generate HIPAA report with detailed evidence
dotnet run -- --framework hipaa --detail-level full --output hipaa-audit-2026-q1.pdf

# Generate multiple reports
dotnet run -- --frameworks gdpr,hipaa,pci --output quarterly-compliance-2026-q1.zip
```

## Report Formats

### PDF Reports

- Professional layout with table of contents
- Executive summary
- Detailed sections with evidence
- Charts and visualizations
- Digital signature and timestamp
- Compliance seal/certification marks

### CSV/Excel Reports

- Tabular data export
- Multiple worksheets by section
- Pivot tables and summaries
- Raw data for further analysis

### JSON Reports

- Machine-readable format
- API integration friendly
- Structured evidence data
- Suitable for automated processing

## Report Templates

### Executive Summary Template

```markdown
# Compliance Report: [Framework]
**Period:** [Start Date] to [End Date]
**Organization:** [Organization Name]
**Generated:** [Timestamp]

## Overall Compliance Status
- Status: [Compliant / Non-Compliant / Partial]
- Compliance Score: [X/100]
- Critical Issues: [Count]
- Recommendations: [Count]

## Key Findings
1. [Finding 1]
2. [Finding 2]
...

## Recommendations
1. [Recommendation 1]
2. [Recommendation 2]
...
```

### Detailed Section Template

```markdown
## Section: [Section Name]

### Control Description
[Description of control requirement]

### Implementation Status
- Status: [Implemented / Partially Implemented / Not Implemented]
- Effectiveness: [Effective / Needs Improvement / Ineffective]

### Evidence
- [Evidence item 1]
- [Evidence item 2]

### Findings
[Detailed findings]

### Recommendations
[Specific recommendations for this section]
```

## Automated Scheduling

Configure automated report generation:

```json
{
  "Schedule": {
    "Enabled": true,
    "Reports": [
      {
        "Framework": "GDPR",
        "Frequency": "Monthly",
        "DayOfMonth": 1,
        "Recipients": ["dpo@example.com"],
        "Format": "PDF"
      },
      {
        "Framework": "HIPAA",
        "Frequency": "Quarterly",
        "Recipients": ["security@example.com", "compliance@example.com"],
        "Format": "PDF",
        "IncludeRawData": true
      }
    ]
  }
}
```

## Evidence Collection

### Automatic Evidence Collection

The tool automatically collects:
- Audit logs (access, modifications, deletions)
- Authentication records
- Encryption status and key rotations
- Data retention policy compliance
- User access reviews
- System configurations
- Backup and recovery logs
- Incident response records

### Manual Evidence Upload

Upload additional evidence:
- Policies and procedures
- Training records
- Third-party assessments
- Penetration test results
- Vulnerability scan reports
- Business continuity plans

## Compliance Metrics

Track compliance metrics over time:
- Compliance score trends
- Control effectiveness ratings
- Issue resolution times
- Audit findings
- Remediation progress

### Dashboard Metrics

- **Overall Compliance Score**: Percentage compliance across all frameworks
- **Critical Issues**: Count of critical compliance gaps
- **Open Recommendations**: Pending remediation items
- **Audit Readiness**: Percentage ready for external audit
- **Trend Analysis**: Compliance score changes over time

## Integration Examples

### Automated Compliance Monitoring

```csharp
// Programmatic example (API will be implemented)
var monitor = new ComplianceMonitor();

// Set up continuous monitoring
await monitor.ConfigureAsync(new MonitoringConfig
{
    Frameworks = new[] { "GDPR", "HIPAA" },
    CheckInterval = TimeSpan.FromHours(1),
    AlertThresholds = new AlertThresholds
    {
        ComplianceScoreBelow = 95,
        CriticalIssues = 1,
        UnresolvedDaysOver = 7
    }
});

// Start monitoring
await monitor.StartAsync();
```

### Export to SIEM

```csharp
var exporter = new ComplianceExporter();

// Export compliance events to SIEM
await exporter.ExportToSIEMAsync(new SIEMExportConfig
{
    Destination = "https://siem.example.com/api/events",
    Format = "CEF", // Common Event Format
    Frameworks = new[] { "GDPR", "HIPAA", "PCI-DSS" }
});
```

## Troubleshooting

### Common Issues

**Issue**: Report generation fails with "insufficient permissions"
```
Solution: Ensure the user has admin or audit role.
Verify bearer token has correct permissions.
```

**Issue**: Missing audit data in report
```
Solution: Check audit log retention settings.
Ensure audit logging is enabled for all operations.
Verify date range includes the desired period.
```

**Issue**: PDF generation timeout
```
Solution: Reduce evidence detail level.
Split large reports into multiple smaller reports.
Increase timeout in configuration.
```

## Best Practices

1. **Regular Generation**
   - Generate reports monthly minimum
   - Keep historical reports for audit trail
   - Review trends and patterns

2. **Evidence Management**
   - Maintain complete audit logs
   - Document all security controls
   - Keep supporting documentation organized

3. **Review Process**
   - Have compliance officer review reports
   - Address findings promptly
   - Track remediation progress

4. **Audit Preparation**
   - Generate reports 2 weeks before audits
   - Review with legal/compliance team
   - Prepare additional evidence as needed

## API Reference

*(To be implemented)*

```csharp
// Compliance report API endpoints
POST /api/v1/compliance/reports/generate
GET  /api/v1/compliance/reports/{reportId}
GET  /api/v1/compliance/reports/scheduled
GET  /api/v1/compliance/metrics
POST /api/v1/compliance/evidence/upload
```

## See Also

- [PII Manager](pii-manager.md) - Data privacy management
- [Key Rotation Dashboard](key-rotation-dashboard.md) - Key management
- [Audit Log Viewer](audit-log-viewer.md) - Audit log analysis
- [Admin Tools Overview](ADMIN_TOOLS_OVERVIEW.md) - Complete admin tools guide
- [TOOLS_INDEX.md](../../TOOLS_INDEX.md) - All ThemisDB tools

## Contributing

Contributions welcome! Please ensure:
- Follow .NET coding standards
- Add report templates for new frameworks
- Document evidence requirements
- Include sample reports

## License

Apache 2.0 - See [LICENSE](../../../LICENSE) for details.
