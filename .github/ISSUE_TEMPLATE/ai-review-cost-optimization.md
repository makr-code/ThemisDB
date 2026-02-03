---
name: 💰 AI Review - Cost Optimization & Resource Efficiency
about: Systematische Kosten-Optimierungs- und Ressourcen-Effizienz-Analyse / Systematic cost optimization and resource efficiency review
title: '[COST-OPTIMIZATION-REVIEW] '
labels: ['type:systematic-review', 'area:cost', 'area:optimization', 'needs-triage']
assignees: ''
---

<!-- 
====================================================================================================
📖 AI AGENT GUIDANCE - COST OPTIMIZATION
====================================================================================================

COST OPTIMIZATION REVIEW REQUIREMENTS:

1. **QUANTIFIED SAVINGS REQUIRED**:
   - Calculate exact savings: "Reduce X from $Y to $Z = $W savings/month"
   - Prioritize by ROI: savings per effort hour
   - Document quick wins: high savings, low effort
   - Project annual impact: monthly savings × 12

2. **RESOURCE UTILIZATION METRICS**:
   - Measure actual utilization: CPU%, memory%, storage%
   - Identify idle resources: <25% utilization
   - Find over-provisioned instances: specs vs actual usage
   - Calculate waste: provisioned - used

3. **BENCHMARKING**:
   - Compare costs: AWS vs Azure vs GCP for same workload
   - Instance comparison: M5 vs C5 vs R5 for workload profile
   - Reserved vs on-demand vs spot pricing analysis
   - Storage tiers: hot vs warm vs cold data

4. **OPTIMIZATION OPPORTUNITIES**:
   - Right-sizing: list instances with recommendations
   - Shutdown schedules: dev/staging non-business hours
   - Reserved capacity: predict stable workload for commitments
   - Architecture changes: serverless, containers, etc.

5. **ACTION ITEMS WITH SAVINGS**:
   - Each item: Current cost, New cost, Savings, Effort, Risk
   - Prioritize: (Savings / Effort) ratio
   - Group by: P0 (>$1K/mo), P1 ($500-1K), P2 (<$500)
   - Include implementation steps

📚 **REQUIRED READING**: `.github/ISSUE_TEMPLATE/_guides/AI_AGENT_REVIEW_GUIDE.md`

====================================================================================================
-->

<!-- 
Wiederholbare Template für Cost Optimization & Resource Efficiency Reviews
Repeatable template for cost optimization and resource efficiency reviews
Empfohlene Häufigkeit: Quartalsweise / Recommended frequency: Quarterly
-->

## 🎯 Scope / Umfang

**Review Scope:** <!-- z.B. Production Environment, All Infrastructure, Specific Service -->
**Review Period:** <!-- z.B. Q1 2026, Last 3 Months -->
**Reviewer(s):** <!-- Namen der Reviewer -->
**Previous Review:** <!-- Datum des letzten Reviews -->

---

## 💵 Cost Overview / Kosten-Übersicht

### Total Costs / Gesamtkosten
- **Total Monthly Cost:** <!-- $ amount -->
- **Total Annual Cost:** <!-- $ amount projected -->
- **Cost Trend:** <!-- ↗️ Increasing, ↘️ Decreasing, → Stable -->
- **Budget:** <!-- $ amount -->
- **Budget Utilization:** <!-- % -->

### Cost Breakdown by Category / Kosten-Aufschlüsselung
| Category | Monthly Cost | % of Total | Trend |
|----------|--------------|------------|-------|
| Compute (VMs, Containers) | $ | % | ↗️/↘️/→ |
| Storage (Block, Object) | $ | % | |
| Database | $ | % | |
| Network (Bandwidth, CDN) | $ | % | |
| GPU/Accelerators | $ | % | |
| Monitoring & Logging | $ | % | |
| Backup & DR | $ | % | |
| Licenses & Software | $ | % | |
| Support & Services | $ | % | |
| Other | $ | % | |
| **Total** | **$** | **100%** | |

---

## ☁️ Cloud Infrastructure Costs / Cloud-Infrastruktur-Kosten

### Cloud Provider Breakdown / Cloud-Anbieter-Aufschlüsselung
| Provider | Monthly Cost | Services | Optimization Score |
|----------|--------------|----------|-------------------|
| AWS | $ | | /10 |
| Azure | $ | | /10 |
| GCP | $ | | /10 |
| Other | $ | | /10 |

### Compute Costs / Rechen-Kosten
- **Total Compute Cost:** <!-- $ -->
- **Instance Types:**
  - On-Demand: <!-- $ -->
  - Reserved Instances: <!-- $ -->
  - Spot/Preemptible: <!-- $ -->
  - Savings Plans: <!-- $ -->

**Compute Optimization Opportunities:**
- [ ] **Right-sizing** underutilized instances
- [ ] **Reserved instances** for steady workloads
- [ ] **Spot instances** for fault-tolerant workloads
- [ ] **Auto-scaling** optimization
- [ ] **Shutdown** non-production during off-hours

**Specific Recommendations:**
1. 
2. 
3. 

### Storage Costs / Speicher-Kosten
- **Total Storage Cost:** <!-- $ -->
- **Storage Breakdown:**
  - Block Storage (SSD): <!-- $ -->
  - Object Storage (S3, Blob): <!-- $ -->
  - Archival Storage: <!-- $ -->
  - Snapshots/Backups: <!-- $ -->

**Storage Optimization Opportunities:**
- [ ] **Lifecycle policies** for old data
- [ ] **Compression** enabled
- [ ] **Deduplication** implemented
- [ ] **Archival** for cold data
- [ ] **Snapshot retention** optimized
- [ ] **Unused volumes** deleted

**Specific Recommendations:**
1. 
2. 
3. 

### Network Costs / Netzwerk-Kosten
- **Total Network Cost:** <!-- $ -->
- **Network Breakdown:**
  - Data Transfer Out: <!-- $ -->
  - Inter-region Transfer: <!-- $ -->
  - CDN: <!-- $ -->
  - Load Balancers: <!-- $ -->
  - VPN/Direct Connect: <!-- $ -->

**Network Optimization Opportunities:**
- [ ] **CDN** for static content
- [ ] **Caching** to reduce origin requests
- [ ] **Compression** for transfers
- [ ] **Regional optimization**
- [ ] **Peering** arrangements

**Specific Recommendations:**
1. 
2. 
3. 

---

## 🗄️ Database Costs / Datenbank-Kosten

### Database Cost Breakdown / Datenbank-Kosten-Aufschlüsselung
| Database | Type | Monthly Cost | Storage | IOPS | Optimization Score |
|----------|------|--------------|---------|------|--------------------|
| RocksDB | Self-hosted | $ | | | /10 |
| PostgreSQL | Managed | $ | | | /10 |
| Other | | $ | | | /10 |

**Database Optimization Opportunities:**
- [ ] **Right-sizing** database instances
- [ ] **Reserved capacity** for predictable workloads
- [ ] **Read replicas** optimization
- [ ] **Index optimization** (reduce storage)
- [ ] **Data retention** policies
- [ ] **Compression** enabled
- [ ] **Backup retention** optimized

**Specific Recommendations:**
1. 
2. 
3. 

---

## 🎮 GPU & Accelerator Costs / GPU & Beschleuniger-Kosten

### GPU Cost Breakdown / GPU-Kosten-Aufschlüsselung
- **Total GPU Cost:** <!-- $ -->
- **GPU Types:**
  - NVIDIA A100: <!-- $ -->
  - NVIDIA V100: <!-- $ -->
  - NVIDIA T4: <!-- $ -->
  - AMD MI250: <!-- $ -->
  - Other: <!-- $ -->

**GPU Optimization Opportunities:**
- [ ] **Spot instances** for training
- [ ] **Batch processing** to maximize utilization
- [ ] **Model quantization** (reduce GPU requirements)
- [ ] **GPU sharing** between workloads
- [ ] **Shutdown** idle GPUs
- [ ] **Right-sizing** GPU type for workload

**GPU Utilization:** <!-- % average -->

**Specific Recommendations:**
1. 
2. 
3. 

---

## 📊 Resource Utilization / Ressourcen-Auslastung

### CPU Utilization / CPU-Auslastung
- **Average CPU Utilization:** <!-- % -->
- **Peak CPU Utilization:** <!-- % -->
- **Idle Capacity:** <!-- % -->

**CPU Optimization:**
- [ ] Underutilized instances identified (< 25%)
- [ ] Over-provisioned instances right-sized
- [ ] Auto-scaling configured appropriately

### Memory Utilization / Speicher-Auslastung
- **Average Memory Utilization:** <!-- % -->
- **Peak Memory Utilization:** <!-- % -->
- **Idle Capacity:** <!-- % -->

**Memory Optimization:**
- [ ] Underutilized memory identified
- [ ] Memory-optimized instances considered
- [ ] Application memory leaks addressed

### Storage Utilization / Speicher-Auslastung
- **Total Provisioned Storage:** <!-- GB/TB -->
- **Total Used Storage:** <!-- GB/TB -->
- **Utilization:** <!-- % -->
- **Idle Storage:** <!-- GB/TB -->

**Storage Optimization:**
- [ ] Unused volumes deleted
- [ ] Over-provisioned volumes downsized
- [ ] Old snapshots cleaned up

---

## 🔍 Cost by Service / Kosten nach Service

### Top 10 Costly Services / Top 10 kostenintensive Services
| Service | Monthly Cost | % of Total | Optimization Potential |
|---------|--------------|------------|----------------------|
| 1. | $ | % | High/Medium/Low |
| 2. | $ | % | |
| 3. | $ | % | |
| 4. | $ | % | |
| 5. | $ | % | |
| 6. | $ | % | |
| 7. | $ | % | |
| 8. | $ | % | |
| 9. | $ | % | |
| 10. | $ | % | |

---

## 💡 Optimization Opportunities / Optimierungs-Möglichkeiten

### Quick Wins / Schnelle Erfolge
High-impact, low-effort optimizations:
1. **Opportunity:**
   - Current Cost: $ /month
   - Potential Savings: $ /month
   - Effort: <!-- Hours/Days -->
   - Risk: <!-- High/Medium/Low -->

2. **Opportunity:**
   - Current Cost: $ /month
   - Potential Savings: $ /month
   - Effort: 
   - Risk: 

3. **Opportunity:**
   - Current Cost: $ /month
   - Potential Savings: $ /month
   - Effort: 
   - Risk: 

### Long-Term Optimizations / Langfristige Optimierungen
1. **Opportunity:**
   - Current Cost: $ /month
   - Potential Savings: $ /month
   - Effort: 
   - Risk: 
   - Timeline: 

2. **Opportunity:**
   - Current Cost: $ /month
   - Potential Savings: $ /month
   - Effort: 
   - Risk: 
   - Timeline: 

**Total Potential Monthly Savings:** <!-- $ -->
**Total Potential Annual Savings:** <!-- $ -->

---

## 🏷️ Tagging & Cost Allocation / Tagging & Kosten-Zuordnung

### Tagging Strategy / Tagging-Strategie
- [ ] **Environment** tags (prod, staging, dev)
- [ ] **Team/Owner** tags
- [ ] **Project** tags
- [ ] **Cost center** tags
- [ ] **Application** tags

**Tagging Compliance:** <!-- % of resources tagged -->

**Untagged Resources:** <!-- Count -->

### Cost Allocation / Kosten-Zuordnung
| Team/Project | Monthly Cost | % of Total |
|--------------|--------------|------------|
| | $ | % |
| | $ | % |
| | $ | % |

---

## 📈 Cost Trends & Forecasting / Kosten-Trends & Prognose

### Historical Trends / Historische Trends
- **Month-over-Month Change:** <!-- +/- % -->
- **Year-over-Year Change:** <!-- +/- % -->
- **Cost Growth Rate:** <!-- % per month -->

### Cost Drivers / Kostentreiber
1. **Driver:**
   - Impact: <!-- $ increase -->
   - Reason: 

2. **Driver:**
   - Impact: 
   - Reason: 

### Cost Forecast / Kosten-Prognose
- **Next Month:** <!-- $ -->
- **Next Quarter:** <!-- $ -->
- **Next Year:** <!-- $ -->

**Forecast Assumptions:**


---

## 🛠️ Optimization Tools & Processes / Optimierungs-Tools & Prozesse

### Cost Management Tools / Kosten-Management-Tools
- [ ] **Cloud provider cost explorer** (AWS Cost Explorer, Azure Cost Management)
- [ ] **Third-party cost tools** (CloudHealth, Cloudability, Kubecost)
- [ ] **Budget alerts** configured
- [ ] **Anomaly detection** enabled
- [ ] **Cost allocation** reports

### Optimization Processes / Optimierungs-Prozesse
- [ ] **Regular cost reviews** (weekly, monthly)
- [ ] **Chargeback/Showback** model
- [ ] **Cost optimization** KPIs
- [ ] **FinOps practices** adopted
- [ ] **Cost awareness** in development

**Process Gaps:**


---

## 📊 FinOps Maturity / FinOps-Reife

### FinOps Maturity Level / FinOps-Reifegrad
- **Current Maturity:** <!-- Crawl, Walk, Run -->
- **Target Maturity:** <!-- Crawl, Walk, Run -->

### FinOps Capabilities / FinOps-Fähigkeiten
- [ ] **Cost visibility** and reporting
- [ ] **Cost allocation** and accountability
- [ ] **Cost optimization** practices
- [ ] **Rate optimization** (RIs, savings plans)
- [ ] **Usage optimization** (right-sizing, auto-scaling)
- [ ] **Architecture optimization**
- [ ] **Cloud provider negotiations**

**FinOps Gaps:**


---

## 🗺️ Roadmap / Roadmap

### Short-Term (Next 3 Months)
- [ ] Implement quick-win optimizations ($ savings)
- [ ] Address underutilized resources
- [ ] Improve tagging compliance
- [ ] 

### Medium-Term (3-6 Months)
- [ ] Reserved instance/savings plan strategy
- [ ] Architecture optimizations
- [ ] FinOps process improvements
- [ ] 

### Long-Term (6-12 Months)
- [ ] Major cost reduction initiatives
- [ ] FinOps maturity advancement
- [ ] Cost-aware culture establishment
- [ ] 

---

## ✅ Action Items / Aktionspunkte

### Critical (P0) - High Cost/High Impact
1. [ ] **Action 1:**
   - Owner: 
   - Due Date: 
   - Savings: $ /month
   - Description: 

### High Priority (P1)
1. [ ] **Action 1:**
   - Owner: 
   - Due Date: 
   - Savings: $ /month
   - Description: 

2. [ ] **Action 2:**
   - Owner: 
   - Due Date: 
   - Savings: $ /month
   - Description: 

### Medium Priority (P2)
1. [ ] **Action 1:**
   - Owner: 
   - Due Date: 
   - Savings: $ /month
   - Description: 

---

## 📚 References / Referenzen

### Internal Documentation
- [Cost Management Policy](docs/policies/cost-management.md)
- [Infrastructure Costs](docs/infrastructure/costs.md)
- [Budgets & Forecasts](docs/finance/budgets.md)

### External Resources
- [FinOps Foundation](https://www.finops.org/)
- [AWS Cost Optimization](https://aws.amazon.com/pricing/cost-optimization/)
- [Azure Cost Management](https://azure.microsoft.com/en-us/services/cost-management/)
- [GCP Cost Optimization](https://cloud.google.com/cost-management)
- [Cloud Cost Optimization Best Practices](https://www.cloudzero.com/blog/cloud-cost-optimization)

### Tools
- [AWS Cost Explorer](https://aws.amazon.com/aws-cost-management/aws-cost-explorer/)
- [Kubecost](https://www.kubecost.com/)
- [CloudHealth](https://www.cloudhealthtech.com/)
- [Cloudability](https://www.cloudability.com/)

---

## 📋 Review Checklist / Review-Checkliste

- [ ] Total costs calculated and compared to budget
- [ ] Cost breakdown by category analyzed
- [ ] Cloud infrastructure costs reviewed
- [ ] Database costs optimized
- [ ] GPU/accelerator costs assessed
- [ ] Resource utilization evaluated
- [ ] Top costly services identified
- [ ] Optimization opportunities documented
- [ ] Tagging and cost allocation verified
- [ ] Cost trends and forecast analyzed
- [ ] FinOps maturity assessed
- [ ] Action items created and assigned with savings targets
- [ ] Sign-offs obtained from finance and engineering teams

---

**Review Date:** <!-- YYYY-MM-DD -->
**Next Review:** <!-- YYYY-MM-DD (empfohlen: +3 Monate) -->
**Sign-Off:** <!-- FinOps Lead, Engineering Manager, CFO/Finance -->

---

**Template Version:** 1.0.0  
**Created:** 2026-02-02  
**Maintained by:** ThemisDB FinOps Team
