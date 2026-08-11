# GitHub Template: AI Model Card

Used to document AI/ML model characteristics for EU AI Act compliance (Article 13/22).

---

# Model Card: [Model Name]

**Model:** `<module_name>.model`  
**Version:** `<version>`  
**Date:** `<YYYY-MM-DD>`  
**Owner:** `<team/person>`  

## 1. Purpose & Application

**Primary Use:** [What does the model do?]

**Context:** [Where is it deployed?]

**Regulatory Scope:**
- [ ] High-Risk AI (Article 13)
- [ ] Bias & Fairness (Article 22)

---

## 2. Training Data

**Data Origin:** [Dataset names and sources]  
**Volume:** [Size]  
**Date Range:** [YYYY-MM to YYYY-MM]  
**Languages:** [list]  
**Domains:** [list]  

**Data Limitations:**
- [ ] Underrepresented demographic groups
- [ ] Temporal distribution skew
- [ ] Domain shift risks

---

## 3. Model Architecture

**Type:** [e.g., Transformer, LSTM, etc.]  
**Framework:** [PyTorch/TensorFlow/other]  
**Training Hardware:** [GPU/CPU specs]  
**Training Time:** [duration]  

**Reproducibility:**
- Random seed: [seed]
- Repository: [GitHub URL]

---

## 4. Performance Metrics

| Metric | Value | Baseline |
|--------|-------|----------|
| Accuracy | [X]% | [X]% |
| Precision | [X]% | [X]% |
| Recall | [X]% | [X]% |
| Latency (p99) | [Xms] | [Xms] |

---

## 5. Bias & Fairness

**Demographic Parity Testing:**

| Group | Metric | Threshold | Measured | Status |
|-------|--------|-----------|----------|--------|
| Female | Accuracy | ≥ [X]% | [X]% | [ ] PASS |
| Male | Accuracy | ≥ [X]% | [X]% | [ ] PASS |

**Known Biases:** [List with remediation plan]

---

## 6. Limitations

**Performs Well On:** [domain/characteristics]  
**Performs Poorly On:** [domain/characteristics]  

**Known Issues:**
- [ ] Hallucinations
- [ ] Adversarial vulnerability
- [ ] Temporal drift
- [ ] Long-tail category failures

---

## 7. Testing & Validation

**Test Coverage:**
- Unit tests: [count]
- Integration tests: [count]
- Adversarial tests: [describe]
- Bias tests: [describe]

**All Tests Passing:** [ ] Yes / [ ] No

---

## 8. EU AI Act Compliance

- [ ] Article 5 (Risk classification)
- [ ] Article 10 (Data quality)
- [ ] Article 13 (Transparency)
- [ ] Article 14 (Robustness)
- [ ] Article 22 (Bias & discrimination)

**Outstanding Gaps:** [List any gaps]

---

## 9. Governance & Approvals

| Role | Name | Signature | Date |
|------|------|-----------|------|
| Model Owner | [Name] | [Signature] | [Date] |
| Ethics Lead | [Name] | [Signature] | [Date] |
| Security Lead | [Name] | [Signature] | [Date] |

---

## 10. Monitoring & Updates

**Metrics Monitored:** [List KPIs]  
**Monitoring Frequency:** [daily/weekly/monthly]  
**Retraining Schedule:** [Trigger & frequency]  
**Deprecation Plan:** [EOL date & replacement]  

---

**Last Reviewed:** [Date] | **Next Review:** [Date]

