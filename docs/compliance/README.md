# ThemisDB Compliance Documentation

This directory contains compliance-related documentation for ThemisDB, supporting GDPR, ISO 27001, and other regulatory requirements.

## Contents

### Data Protection Impact Assessment (DPIA)
- **File:** [DPIA.md](DPIA.md)
- **Purpose:** GDPR Article 35 compliance
- **Status:** Active for v1.4.1+
- **Review:** Annual or upon significant changes

### LoRA Adapter Provenance and Audit
- **File:** [LORA_PROVENANCE_AUDIT.md](LORA_PROVENANCE_AUDIT.md)
- **Purpose:** Cryptographic auditability for LoRA adapters (eIDAS, GDPR, ISO 27001, SOC 2)
- **Status:** Active for v1.5.0+
- **Covers:** Provenance records, Merkle-chained inference audit log, MVCC snapshots, external adapter validation, compliance matrix, threat model, operational runbook

### Breach Notification Plan
- **File:** See [BREACH_NOTIFICATION_PLAN.md](../security/BREACH_NOTIFICATION_PLAN.md) in security docs
- **Purpose:** GDPR Articles 33-34 compliance
- **Covers:** Detection, assessment, notification procedures

### Privacy Documentation Templates
Organizations deploying ThemisDB should create:
- Privacy Policy (based on data controller's legal basis)
- Data Processing Agreement (DPA) for third-party processing
- Records of Processing Activities (ROPA) per GDPR Art. 30
- Data Subject Rights Request procedures

## Regulatory Framework

### GDPR (General Data Protection Regulation)
ThemisDB supports GDPR compliance through:
- **Privacy by Design** (Art. 25) - Security features built-in
- **Security of Processing** (Art. 32) - Encryption, access controls
- **DPIA** (Art. 35) - Risk assessment documented
- **Data Subject Rights** (Art. 15-22) - Technical support provided

### ISO 27001:2022
Information Security Management System:
- See [COMPLIANCE_MAPPING.md](../audit-framework/COMPLIANCE_MAPPING.md) for control mappings
- 93 Annex A controls assessed
- 95.4% compliance rate

### SOC 2 Type II
Trust Services Criteria:
- Security (CC6.x)
- Availability (CC7.x)
- Processing Integrity (CC8.x)
- Confidentiality (CC9.x)
- Privacy (P criteria)

### NIST Cybersecurity Framework
- Identify, Protect, Detect, Respond, Recover
- Tier 3 (Repeatable) maturity level
- See audit framework for control mapping

## For Deploying Organizations

### Responsibility Matrix

| Responsibility | ThemisDB Project | Deploying Organization |
|----------------|------------------|------------------------|
| Software Security Features | ✅ Provided | - |
| Lawful Basis for Processing | - | ✅ Required |
| Data Controller Obligations | - | ✅ Required |
| Data Processor Agreement | - | ✅ If applicable |
| Privacy Policy | - | ✅ Required |
| GDPR Compliance Implementation | ⚠️ Supported | ✅ Required |
| Breach Notification | ⚠️ Technical support | ✅ Required |
| Data Subject Rights Implementation | ⚠️ Technical support | ✅ Required |

### Implementation Checklist for Organizations

- [ ] Determine lawful basis for processing (GDPR Art. 6)
- [ ] Create Privacy Policy aligned with GDPR Art. 13-14
- [ ] Implement consent management (if applicable)
- [ ] Configure data retention policies
- [ ] Enable encryption at-rest and in-transit
- [ ] Set up audit logging with appropriate retention
- [ ] Implement data subject rights request procedures
- [ ] Conduct organization-specific DPIA if needed
- [ ] Establish breach notification procedures
- [ ] Train staff on GDPR obligations
- [ ] Document Records of Processing Activities (ROPA)
- [ ] Review and sign Data Processing Agreements (DPAs)

## Related Documentation

- [Audit Framework](../audit-framework/README.md) - Comprehensive audit procedures
- [Security Documentation](../security/) - Security architecture and controls
- [Data Flow Diagrams](../architecture/DATA_FLOW_DIAGRAMS.md) - System architecture
- [Threat Model](../security/THREAT_MODEL.md) - Risk assessment

## Updates and Maintenance

- **Review Frequency:** Annual or upon regulatory changes
- **Owner:** Security & Compliance Team
- **Last Updated:** April 2026
- **Next Review:** February 2027

## Support

For compliance questions or assistance:
- **Email:** compliance@themisdb.org
- **Security:** security@themisdb.org
- **GitHub Issues:** Use label `compliance`

---

**Note:** This documentation is provided for informational purposes. Organizations deploying ThemisDB are responsible for ensuring their own compliance with applicable laws and regulations.
