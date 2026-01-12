# Themis.PIIManager - PII Detection and Management

## Overview

Desktop application for detecting, managing, and pseudonymizing Personally Identifiable Information (PII) in ThemisDB. Helps ensure GDPR, CCPA, and other privacy regulation compliance.

## Use Cases

- Detect PII in documents and databases
- Configure PII detection rules and patterns
- Apply pseudonymization or anonymization
- Generate PII compliance reports
- Audit data for sensitive information

## Requirements

- .NET 8.0+
- Windows, Linux, or macOS
- Access to ThemisDB server with PII API
- Appropriate permissions for PII operations

## Features

- **Pattern-Based Detection:** Email, SSN, credit cards, phone numbers
- **ML-Based Detection:** Named entity recognition for names, addresses
- **Custom Rules:** Define organization-specific PII patterns
- **Pseudonymization:** Replace PII with consistent pseudonyms
- **Masking:** Partial masking of sensitive data
- **Reporting:** Generate compliance reports and audit logs

## Installation

```bash
cd tools/Themis.PIIManager
dotnet restore
dotnet build
dotnet run
```

## Basic Usage

1. **Connect to Server:** Configure server URL in settings
2. **Scan Data:** Select collections or documents to scan
3. **Review Findings:** Examine detected PII
4. **Apply Actions:** Pseudonymize, mask, or redact PII
5. **Generate Report:** Export compliance report

## Configuration

Edit `appsettings.json`:

```json
{
  "ThemisServer": {
    "BaseUrl": "http://localhost:8080",
    "ApiKey": ""
  },
  "PIIDetection": {
    "Patterns": ["email", "ssn", "credit_card", "phone"],
    "EnableMLDetection": true,
    "ConfidenceThreshold": 0.85
  }
}
```

## See Also

- [PII API Documentation](../../docs/pii_api.md)
- [PII Detection Guide](../../docs/security/pii_detection.md)
- [GDPR Compliance](../../docs/security/gdpr_compliance.md)
