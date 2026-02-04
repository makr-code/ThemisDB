# ThemisDB Admin Tools Overview

## Introduction

ThemisDB provides a comprehensive suite of administrative tools designed to help database administrators, security officers, and compliance teams manage their ThemisDB deployments effectively. This document provides an overview of all admin tools with focus on privacy, security, and compliance features.

## Core Admin Tools

### Data Privacy & Compliance

#### 1. PII Manager (Themis.PIIManager)

**Status**: ✅ Available

Detects, manages, and pseudonymizes Personally Identifiable Information (PII) in ThemisDB to ensure GDPR, CCPA, and other privacy regulation compliance.

**Key Features:**
- Pattern-based and ML-based PII detection
- Support for multiple PII types (email, SSN, credit cards, phone numbers, etc.)
- Custom rule configuration for organization-specific patterns
- Pseudonymization with consistent pseudonyms
- Data masking and redaction
- Compliance reporting and audit logs

**Use Cases:**
- Pre-production PII scanning before data ingestion
- Regular compliance audits
- Data subject access requests (DSAR)
- Right to be forgotten (RTBF) implementation

**Documentation**: [pii-manager.md](pii-manager.md)

#### 2. Key Rotation Dashboard (Themis.KeyRotationDashboard)

**Status**: 🚧 Under Development

Manages encryption keys and automates key rotation procedures to maintain security best practices.

**Key Features:**
- Centralized key management interface
- Automated key rotation scheduling
- Key versioning and lifecycle management
- Zero-downtime key rotation
- Compliance with key management standards (NIST, FIPS)
- Audit trail for all key operations
- Integration with HSM (Hardware Security Modules)

**Planned Capabilities:**
- **Encryption Key Types:**
  - Data-at-rest encryption keys (AES-256)
  - Transport encryption keys (TLS certificates)
  - Field-level encryption keys
  - Vector embedding encryption keys
  
- **Rotation Strategies:**
  - Scheduled rotation (daily, weekly, monthly)
  - On-demand rotation
  - Emergency rotation for compromised keys
  - Graceful key deprecation

- **Key Storage Options:**
  - Local secure storage
  - Cloud KMS (AWS KMS, Azure Key Vault, Google Cloud KMS)
  - HashiCorp Vault
  - Hardware Security Modules (HSM)

**Use Cases:**
- Regular key rotation for compliance (PCI-DSS, HIPAA)
- Emergency key rotation after security incidents
- Certificate renewal and management
- Key lifecycle audit and reporting

**Documentation**: [key-rotation-dashboard.md](key-rotation-dashboard.md) *(to be created)*

#### 3. Compliance Reports (Themis.ComplianceReports)

**Status**: 🚧 Under Development

Generates comprehensive compliance reports for various regulatory frameworks and standards.

**Key Features:**
- Pre-built report templates for major regulations
- Customizable report formats and content
- Automated report generation and scheduling
- Evidence collection and documentation
- Export to multiple formats (PDF, CSV, JSON, Excel)
- Digital signing and timestamping

**Supported Frameworks:**
- **GDPR** (General Data Protection Regulation)
  - Data processing activities
  - Consent management
  - Data subject rights (access, deletion, portability)
  - Data breach notifications
  
- **eIDAS** (Electronic Identification and Trust Services)
  - Electronic signatures
  - Timestamp services
  - Qualified trust service providers
  
- **HIPAA** (Health Insurance Portability and Accountability Act)
  - PHI access logs
  - Encryption compliance
  - Audit controls
  
- **PCI-DSS** (Payment Card Industry Data Security Standard)
  - Cardholder data environment
  - Key management
  - Access controls
  
- **SOC 2** (Service Organization Control 2)
  - Security controls
  - Availability monitoring
  - Processing integrity
  
- **ISO 27001** (Information Security Management)
  - Risk assessments
  - Security controls
  - Incident response

**Report Types:**
- **Audit Reports:** Complete audit trails with timestamps
- **Access Reports:** User access patterns and permissions
- **Data Inventory:** Classification and location of sensitive data
- **Retention Reports:** Data retention policy compliance
- **Incident Reports:** Security and privacy incidents
- **Assessment Reports:** Risk assessments and gap analysis

**Use Cases:**
- Regulatory compliance audits
- Internal security assessments
- Customer compliance documentation
- Continuous compliance monitoring

**Documentation**: [compliance-reports.md](compliance-reports.md) *(to be created)*

### Data Management Tools

#### 4. Classification Dashboard (Themis.ClassificationDashboard)

**Status**: ✅ Documented (see TOOLS_INDEX.md)

Configure and manage data classification rules to automatically categorize sensitive data.

**Documentation**: Referenced in [TOOLS_INDEX.md](../../TOOLS_INDEX.md)

#### 5. Retention Manager (Themis.RetentionManager)

**Status**: ✅ Documented (see TOOLS_INDEX.md)

Configure and monitor data retention policies to ensure compliance with legal and business requirements.

**Documentation**: Referenced in [TOOLS_INDEX.md](../../TOOLS_INDEX.md)

### Query & Development Tools

#### 6. AQL Query Builder (Themis.AqlQueryBuilder)

**Status**: ✅ Available

Visual query builder for AQL (Advanced Query Language) with syntax highlighting and query optimization.

**Documentation**: [aql-query-builder.md](aql-query-builder.md)

#### 7. Audit Log Viewer (Themis.AuditLogViewer)

**Status**: ✅ Available

Browse, filter, and export audit logs for security analysis and compliance.

**Documentation**: [audit-log-viewer.md](audit-log-viewer.md)

### Specialized Tools

#### 8. GIS Viewer Control Panel (Themis.GISViewer.ControlPanel)

**Status**: ✅ Documented (see TOOLS_INDEX.md)

Visualize and query geospatial data stored in ThemisDB.

**Documentation**: Referenced in [TOOLS_INDEX.md](../../TOOLS_INDEX.md)

#### 9. Impact Analysis Viewer (Themis.ImpactAnalysisViewer)

**Status**: ✅ Documented (see TOOLS_INDEX.md)

Analyze query performance and resource impact.

**Documentation**: Referenced in [TOOLS_INDEX.md](../../TOOLS_INDEX.md)

#### 10. SAGA Verifier (Themis.SAGAVerifier)

**Status**: ✅ Documented (see TOOLS_INDEX.md)

Verify and debug SAGA distributed transactions.

**Documentation**: Referenced in [TOOLS_INDEX.md](../../TOOLS_INDEX.md)

## Common Architecture

All admin tools share a common architecture:

### Technology Stack
- **Framework**: .NET 8.0+
- **UI**: Avalonia (cross-platform) or WPF (Windows)
- **API Client**: Shared library (Themis.AdminTools.Shared)
- **Authentication**: JWT Bearer tokens
- **Data Format**: JSON over REST API

### Shared Components

```
Themis.AdminTools.Shared/
├── ApiClient/              # REST API client
├── Models/                 # Data models
├── Services/               # Shared business logic
├── Extensions/             # Utility extensions
└── Configuration/          # Configuration management
```

**Documentation**: [admin-tools-shared.md](admin-tools-shared.md)

## Installation & Setup

### Prerequisites

- .NET 8.0 SDK or later
- Access to running ThemisDB server
- Valid authentication credentials
- Appropriate permissions for admin operations

### Installation Steps

```bash
# Clone repository
git clone https://github.com/makr-code/ThemisDB.git
cd ThemisDB/tools

# Restore dependencies
dotnet restore

# Build all admin tools
dotnet build

# Run a specific tool (example: PII Manager)
cd Themis.PIIManager
dotnet run
```

### Configuration

Each tool uses an `appsettings.json` configuration file:

```json
{
  "ThemisDB": {
    "BaseUrl": "http://localhost:8080",
    "BearerToken": "your-jwt-token-here",
    "Timeout": 30
  },
  "Logging": {
    "LogLevel": {
      "Default": "Information"
    }
  }
}
```

## Security Best Practices

When using admin tools:

1. **Authentication**
   - Always use strong authentication tokens
   - Rotate tokens regularly
   - Never commit tokens to version control

2. **Network Security**
   - Use TLS/HTTPS for all connections
   - Consider VPN for remote access
   - Implement IP whitelisting where possible

3. **Access Control**
   - Apply principle of least privilege
   - Use role-based access control (RBAC)
   - Audit all admin actions

4. **Data Protection**
   - Encrypt sensitive data at rest and in transit
   - Use secure key storage (HSM, cloud KMS)
   - Implement data masking for display

5. **Audit & Compliance**
   - Enable comprehensive audit logging
   - Regular compliance reviews
   - Document all administrative actions

## Compliance Workflows

### GDPR Compliance Workflow

1. **Data Discovery**: Use PII Manager to scan for personal data
2. **Data Classification**: Apply classification rules with Classification Dashboard
3. **Retention Policies**: Configure with Retention Manager
4. **Access Auditing**: Review with Audit Log Viewer
5. **Compliance Reporting**: Generate reports with Compliance Reports tool
6. **Key Management**: Ensure encryption with Key Rotation Dashboard

### HIPAA Compliance Workflow

1. **PHI Identification**: Scan with PII Manager for health information
2. **Encryption**: Manage keys with Key Rotation Dashboard
3. **Access Controls**: Monitor with Audit Log Viewer
4. **Audit Trails**: Generate with Compliance Reports
5. **Risk Assessments**: Perform with Impact Analysis Viewer

### PCI-DSS Compliance Workflow

1. **Cardholder Data**: Detect with PII Manager
2. **Encryption Keys**: Rotate with Key Rotation Dashboard
3. **Access Logs**: Review with Audit Log Viewer
4. **Compliance Reports**: Generate quarterly reports
5. **Network Security**: Monitor with administrative tools

## Development Roadmap

### Planned Features (Q1-Q2 2026)

- **Key Rotation Dashboard**: Complete implementation with HSM support
- **Compliance Reports**: Add support for additional frameworks
- **Enhanced PII Detection**: ML model improvements
- **Integration APIs**: Programmatic access to admin functions
- **Mobile Apps**: iOS and Android admin tools
- **Cloud Integrations**: Native support for AWS, Azure, GCP

### Future Considerations

- **Blockchain Integration**: Immutable audit trails
- **AI-Powered Anomaly Detection**: Automated threat detection
- **Real-time Compliance Monitoring**: Continuous compliance dashboards
- **Multi-tenancy Support**: Isolated admin environments

## Support & Resources

- **Documentation**: [ThemisDB Docs](https://docs.themisdb.org)
- **Tool Index**: [TOOLS_INDEX.md](../../TOOLS_INDEX.md)
- **GitHub Issues**: [Report Problems](https://github.com/makr-code/ThemisDB/issues)
- **Community**: [Discussions](https://github.com/makr-code/ThemisDB/discussions)

## Contributing

We welcome contributions to admin tools:

1. Follow the existing architecture and patterns
2. Ensure cross-platform compatibility
3. Include comprehensive tests
4. Document new features
5. Follow security best practices

See [CONTRIBUTING.md](../../../CONTRIBUTING.md) for details.

## License

Apache 2.0 - See [LICENSE](../../../LICENSE) for details.
