# Audit Framework Templates

This directory contains reusable templates for conducting audits and documenting findings.

## Available Templates

### 1. Findings Register Template
**File:** [FINDINGS_REGISTER_TEMPLATE.md](FINDINGS_REGISTER_TEMPLATE.md)

**Purpose:** Track and manage audit findings throughout the audit lifecycle

**Use When:**
- Starting a new audit
- Documenting findings during assessment
- Tracking remediation progress
- Reporting to stakeholders

**Key Sections:**
- Executive summary with finding counts
- Detailed finding descriptions with RCA
- Remediation plans and verification
- Risk acceptance register
- Metrics and trends

### 2. Executive Audit Report Template
**File:** [AUDIT_REPORT_EXECUTIVE_TEMPLATE.md](AUDIT_REPORT_EXECUTIVE_TEMPLATE.md)

**Purpose:** Create executive-level audit reports for management and stakeholders

**Use When:**
- Completing an audit
- Presenting to executive leadership
- Documenting audit opinions
- Supporting release decisions

**Key Sections:**
- Executive summary and audit opinion
- Top findings and risk assessment
- Compliance status by standard
- Trend analysis
- Recommendations and management response
- Sign-off and approval

## How to Use These Templates

### For a New Audit

1. **Copy the template:**
   ```bash
   VERSION="v1.4.1"
   mkdir -p docs/audit-framework/evidence/${VERSION}/findings/
   mkdir -p docs/audit-framework/evidence/${VERSION}/reports/
   
   # Copy findings register
   cp docs/audit-framework/templates/FINDINGS_REGISTER_TEMPLATE.md \
      docs/audit-framework/evidence/${VERSION}/findings/findings-register.md
   
   # Copy audit report
   cp docs/audit-framework/templates/AUDIT_REPORT_EXECUTIVE_TEMPLATE.md \
      docs/audit-framework/evidence/${VERSION}/reports/audit-report-executive.md
   ```

2. **Fill in version-specific information:**
   - Replace `vX.X.X` with actual version number
   - Update dates (audit period, target dates)
   - Update auditor names and contact information

3. **Document findings as you discover them:**
   - Add each finding to the findings register
   - Include complete details: description, evidence, RCA
   - Track remediation progress

4. **Complete the audit report:**
   - Summarize key findings
   - Document compliance status
   - Include recommendations
   - Obtain required sign-offs

### Customization Guidelines

#### Required Fields
These fields MUST be completed:
- Version number and dates
- Auditor names and roles
- Finding descriptions and severity
- Evidence and recommendations
- Sign-off signatures

#### Optional Sections
These sections can be adapted:
- Additional finding categories
- Custom metrics
- Organization-specific compliance requirements
- Additional appendices

#### Branding
Update these elements for your organization:
- Contact email addresses
- Organizational roles and titles
- Internal classification levels
- Document ID format

## Template Maintenance

### Version Control
- Templates are version controlled in Git
- Changes should be reviewed by audit team
- Breaking changes require major version bump

### Updates
Templates should be updated when:
- New compliance standards are adopted
- Audit methodology changes
- Feedback indicates improvements needed
- Regulatory requirements change

### Feedback
Submit feedback and improvement suggestions:
- Open GitHub issue with label `audit-framework`
- Email: audit-team@themisdb.org
- During post-audit retrospectives

## Related Templates

### GitHub Issue Template
**Location:** `.github/ISSUE_TEMPLATE/audit_review.md`

**Purpose:** Create audit tracking issues automatically

**Used By:** `audit-gate-issue.yml` workflow

### Master Audit Gate Template
**Location:** `docs/audit-framework/AUDIT_GATE_TEMPLATE.md`

**Purpose:** Comprehensive 113-point audit checklist

**Use:** Reference for complete audit dimensions

## Best Practices

### DO:
- ✅ Use templates consistently for all audits
- ✅ Keep templates up-to-date with current practices
- ✅ Document all required sections completely
- ✅ Archive completed reports properly
- ✅ Review and improve templates after each audit

### DON'T:
- ❌ Skip required sections
- ❌ Modify templates mid-audit (complete current audit first)
- ❌ Include sensitive data in templates (use specific instances)
- ❌ Delete historical evidence or reports
- ❌ Bypass sign-off requirements

## Examples

### Example Directory Structure
```
docs/audit-framework/evidence/
├── v1.4.0/
│   ├── findings/
│   │   └── findings-register.md
│   ├── reports/
│   │   ├── audit-report-executive.md
│   │   └── audit-report-technical.md
│   ├── scans/
│   │   ├── cppcheck-report.xml
│   │   └── trivy-scan.json
│   └── compliance/
│       └── compliance-verification.md
└── v1.4.1/
    ├── findings/
    │   └── findings-register.md [from template]
    └── reports/
        └── audit-report-executive.md [from template]
```

### Example Workflow
1. Release tag pushed: `v1.4.1`
2. Workflow creates audit issue from template
3. Auditor copies templates to evidence directory
4. Audit execution follows runbook
5. Findings documented in register
6. Reports generated from templates
7. Sign-offs obtained
8. Evidence archived

## Support

### Questions?
- **Documentation:** [Audit Framework README](../README.md)
- **Runbook:** [AUDIT_RUNBOOK.md](../AUDIT_RUNBOOK.md)
- **GitHub Issues:** Label `audit-framework`
- **Email:** audit-team@themisdb.org

### Contributing
Improvements to templates are welcome:
1. Open issue to discuss proposed changes
2. Submit PR with template updates
3. Update this README if needed
4. Ensure changes align with audit standards

---

**Last Updated:** April 2026  
**Version:** 1.0  
**Maintainer:** Audit & Security Team
