# ThemisDB TCO Calculator - User Guide

## Overview

The ThemisDB TCO Calculator is an interactive WordPress plugin for calculating and comparing **Total Cost of Ownership (TCO)** for database solutions over a 3-year period. The plugin enables realistic cost analysis considering:

- 💰 **Infrastructure Costs** - Servers, storage, network, backups
- 👥 **Personnel Costs** - Database administrators and developers
- 📜 **License Costs** - ThemisDB editions (Community/Enterprise)
- 🔧 **Operations Costs** - Training, support, maintenance
- 🤖 **AI/LLM Costs** - Native integration vs. external APIs

## Key Features

### ⚡ Real-time Calculation

The calculator updates **automatically** when you move sliders. No need to click "Calculate" after every change.

**How it works:**
- Change any slider
- After 500ms of inactivity, calculation updates automatically
- Results display immediately
- "Calculate" button remains available for manual calculations

**Benefits:**
- ✅ Immediate visual feedback
- ✅ Intuitive operation
- ✅ Quick experimentation with different scenarios
- ✅ No delay in decision-making

### 📊 Visual Grouping

Input fields are organized into **5 thematic groups** for better clarity:

#### 1. 📊 Workload & Requirements
Define your database load and requirements:
- **Requests per Day**: 1,000 - 10,000,000 requests/day
- **Data Size**: 10 GB - 10 TB
- **Peak Load Factor**: 1x - 10x average load
- **Availability**: 99% - 99.999% (Standard to Mission Critical)

#### 2. 🖥️ Infrastructure & Hardware
Hardware and infrastructure costs:
- **Server Costs**: €100 - €2,000 per server/month
- **Storage Costs**: €0.01 - €1.00 per GB/month
- **Network Costs**: €10 - €200 per TB
- **Backup Costs**: €0.01 - €0.50 per GB/month

#### 3. 👥 Personnel & Team
Personnel resources and salaries:
- **Number of DBAs**: 0 - 10 Full-Time Equivalents (FTE)
- **DBA Salary**: €40,000 - €150,000 per year
- **Number of Developers**: 0 - 20 FTE
- **Developer Salary**: €35,000 - €130,000 per year

*Note: Personnel costs automatically include 30% overhead for benefits and infrastructure.*

#### 4. 🔧 Operations & Support
Ongoing operational costs:
- **Training Costs**: €0 - €100,000 per year
- **Support Costs**: €0 - €200,000 per year

#### 5. 🤖 AI & LLM Features
Costs for AI functionality:
- **Use AI Features**: Yes/No (incl. GPU server)
- **External AI API Costs**: €0 - €20,000 per month

### 🎨 Modular Shortcodes

The plugin offers **6 independent shortcodes** for flexible page layouts:

#### Complete Calculator
```
[themisdb_tco_calculator]
```
Displays all sections and results on one page.

#### Individual Sections

**Workload Section:**
```
[themisdb_tco_workload scale="1" animation="fade-in" delay="0"]
```

**Infrastructure Section:**
```
[themisdb_tco_infrastructure scale="1" animation="slide-up" delay="100"]
```

**Personnel Section:**
```
[themisdb_tco_personnel scale="1" animation="slide-right" delay="200"]
```

**Operations Section:**
```
[themisdb_tco_operations scale="1" animation="zoom-in" delay="300"]
```

**AI Section:**
```
[themisdb_tco_ai scale="1" animation="bounce-in" delay="400"]
```

**Results Section:**
```
[themisdb_tco_results scale="1" animation="fade-in" delay="500"]
```

#### Shortcode Parameters

**scale** - Section scaling
- `scale="0.8"` - 80% of normal size (compact)
- `scale="1.0"` - Normal size (default)
- `scale="1.2"` - 120% of normal size (emphasized)

**animation** - Entry animation
- `fade-in` - Gentle fade in
- `slide-up` - Slide in from bottom
- `slide-down` - Slide in from top
- `slide-left` - Slide in from right
- `slide-right` - Slide in from left
- `zoom-in` - Zoom effect
- `bounce-in` - Bounce effect entrance

**delay** - Delay in milliseconds
- `delay="0"` - Immediate (default)
- `delay="100"` - 0.1 seconds delay
- `delay="500"` - 0.5 seconds delay
- `delay="1000"` - 1 second delay

### 💡 Realistic Cost Model

The calculator uses an **industry-validated cost model** based on studies from Gartner, Forrester, and IDC.

#### Personnel Cost Regression (Learning Curve)

Personnel costs reduce over time through automation and expertise:

**Year 1: 100% of costs**
- Initial onboarding and learning phase
- Frequent manual interventions required
- Problem resolution takes longer
- No established processes yet

**Year 2: 75% of costs (-25%)**
- Team familiar with the system
- Automated monitoring tools implemented
- Standardized maintenance processes
- Faster problem resolution
- Fewer errors through experience

**Year 3: 60% of costs (-40%)**
- Highly optimized workflows
- Comprehensive automation
- Proactive instead of reactive management
- Deep system expertise available
- Minimal manual intervention needed

**Reasons for reduction:**
- ✅ Automation of routine tasks (20-40% time savings)
- ✅ Improved monitoring tools (15-25% faster response)
- ✅ Better documentation (10-20% onboarding time)
- ✅ Error reduction through expertise (30-50% fewer incidents)

#### Investment Cost Distribution

Infrastructure costs follow a front-loading pattern:

**Year 1: 130% of baseline (+30%)**
- Migration costs
- Initial hardware procurement
- Setup and configuration
- Test and development environment
- Redundant systems for proof-of-concept

**Year 2: 90% of baseline (-10%)**
- Optimization after initial experience
- Right-sizing of resources
- Minor upgrades and adjustments
- Efficiency improvements

**Year 3: 80% of baseline (-20%)**
- Stable, mature environment
- Only maintenance and small adjustments
- Optimal resource utilization
- Reduced overhead costs

## Usage Scenarios

### Scenario 1: Startup with Growing Load

**Starting Situation:**
- 50,000 requests/day
- 100 GB data
- 1 DBA, 3 developers
- Community Edition sufficient

**TCO Result (typical):**
- Year 1: ~€80,000
- Year 2: ~€55,000
- Year 3: ~€45,000
- **Total: ~€180,000**

### Scenario 2: Mid-Market with High Availability

**Starting Situation:**
- 1 million requests/day
- 500 GB data
- 99.99% availability
- 2 DBAs, 5 developers
- Enterprise Edition required

**TCO Result (typical):**
- Year 1: ~€320,000
- Year 2: ~€240,000
- Year 3: ~€195,000
- **Total: ~€755,000**

### Scenario 3: Enterprise with AI Features

**Starting Situation:**
- 5 million requests/day
- 2 TB data
- 99.999% availability
- AI/LLM features active
- 3 DBAs, 10 developers

**TCO Result (typical):**
- Year 1: ~€650,000
- Year 2: ~€490,000
- Year 3: ~€400,000
- **Total: ~€1,540,000**

## Comparison: ThemisDB vs. Hyperscaler

### ThemisDB Advantages

✅ **Predictable Costs**
- No surprises from peak loads
- Plannable budgets over 3 years
- No hidden egress costs

✅ **Data Sovereignty**
- Full control over your data
- No dependency on cloud providers
- GDPR-compliant on-premise

✅ **Multi-Model in One Database**
- Graph, Relational, Document, Vector in one system
- No need for 8+ separate services
- Unified administration

✅ **Native AI Integration**
- Built-in llama.cpp integration
- No external API calls
- 4x faster inference latency (50ms vs. 200ms)

✅ **Cost Reduction Over Time**
- Learning curve leads to 40% personnel cost savings
- Optimization reduces infrastructure costs by 20%
- No rising license costs

### Hyperscaler Characteristics

⚠️ **Variable Costs**
- Pay-per-request can be expensive during peaks
- Hard to predict monthly bills

⚠️ **Vendor Lock-in**
- Proprietary APIs and services
- Migration complex and expensive

⚠️ **Polyglot Complexity**
- 8+ separate services for all data models
- Estimated additional costs: €1,450-4,400/month
- Complex integration and maintenance

⚠️ **External API Dependency**
- AI features through external providers
- Higher latency (200ms+)
- Additional costs: €5,000-20,000/month

## Tips for Accurate TCO Calculation

### 1. Realistic Workload Estimation

**Requests per Day:**
- Analyze your current logs
- Consider seasonal fluctuations
- Plan for 20-30% growth per year

**Data Volume:**
- Start with current database size
- Add log and backup data
- Calculate data growth (default: 20% p.a.)

### 2. Personnel Estimation

**DBAs:**
- Small deployments (< 500 GB): 0.5-1 FTE
- Medium deployments (500 GB - 2 TB): 1-2 FTE
- Large deployments (> 2 TB): 2-5 FTE

**Developers:**
- API integration: 2-3 FTE
- Complex queries: 3-5 FTE
- Full-stack with DB logic: 5-10 FTE

### 3. Infrastructure Costs

**On-Premise:**
- Hardware amortization over 3-5 years
- Power costs: ~€100-300/month per server
- Datacenter: ~€500-2,000/month

**Cloud:**
- Compute: ~€200-1,000/month per server
- Storage: ~€0.05-0.20/GB/month
- Network: ~€50-150/TB egress

### 4. Don't Forget Hidden Costs

- ✅ Backup storage (often 2x primary data)
- ✅ Test/development environments
- ✅ Monitoring tools and licenses
- ✅ Training and certifications
- ✅ Emergency support (24/7)

## Interpreting Results

### ThemisDB Edition

The calculator automatically selects the appropriate edition:

**Minimal (Free)**
- Up to 100,000 requests/day
- Up to 50 GB data
- 99% availability
- Perfect for: Development, small projects

**Community (Free)**
- Up to 1 million requests/day
- Unlimited data
- Up to 99.9% availability
- Perfect for: Startups, SMEs

**Enterprise (Commercial)**
- > 1 million requests/day
- Unlimited data
- Up to 99.999% availability
- Perfect for: Large enterprises, critical systems
- License costs: ~€50,000/year

### Understanding Cost Categories

**Infrastructure (20-40% of TCO)**
- Hardware/servers
- Storage and backups
- Network
- Varies greatly with deployment size

**Personnel (40-70% of TCO)**
- Largest cost factor
- Reduces over time (learning curve)
- Most important optimization lever

**Licenses (0-25% of TCO)**
- Only for Enterprise Edition
- Plannable and predictable
- No surprises

**Operations (5-15% of TCO)**
- Training and support
- Maintenance contracts
- External consultants

### ROI Time

The calculator shows when ThemisDB pays off compared to hyperscaler solutions:

- **< 6 months**: Very economical, immediate savings
- **6-12 months**: Good, fast ROI
- **12-24 months**: Acceptable for enterprise
- **> 24 months**: Hyperscaler might be cheaper

## Export Functions

### PDF Export
Creates a print-ready report with:
- Complete input parameters
- TCO comparison over 3 years
- Visualizations and diagrams
- Insights and recommendations

**Usage:** Click "Export PDF" after calculation.

### CSV Export
Exports data for further analysis:
- Year-by-year breakdown
- All cost categories
- ThemisDB vs. Hyperscaler
- Import into Excel/Google Sheets possible

**Usage:** Click "Export CSV" after calculation.

## Frequently Asked Questions (FAQ)

### Why do personnel costs reduce over time?

Personnel costs decrease through:
1. **Automation** - Routine tasks are automated
2. **Expertise** - Team becomes more efficient
3. **Standardization** - Established processes save time
4. **Tools** - Better monitoring and management tools

This reflects reality in IT projects (Learning Curve Effect).

### Why are Year 1 costs higher?

Year 1 includes:
- Migration from existing systems
- Initial hardware procurement
- Setup and configuration
- Test environments
- Training and onboarding

These one-time costs don't recur in following years.

### How accurate are the calculations?

Calculations are based on:
- ✅ Gartner TCO analyses
- ✅ Forrester Economic Impact studies
- ✅ IDC market research
- ✅ Real customer migration costs

**Accuracy:** ±15-25% for typical scenarios.

### Can I adjust the factors?

Yes! Advanced users can modify constants in `assets/js/tco-calculator.js`:

```javascript
PERSONNEL_EFFICIENCY_YEAR_1: 1.0,   // Year 1: 100%
PERSONNEL_EFFICIENCY_YEAR_2: 0.75,  // Year 2: 75%
PERSONNEL_EFFICIENCY_YEAR_3: 0.60,  // Year 3: 60%
```

### Does the model work for > 3 years?

Yes! After Year 3, costs typically stabilize:
- Personnel: ~60% of Year 1 level
- Infrastructure: ~80% of Year 1 level

You can divide total costs by 3 and multiply by the number of years.

## Support and Feedback

### Community Support
- **GitHub Issues**: [ThemisDB Issues](https://github.com/makr-code/wordpressPlugins/issues)
- **Discussions**: GitHub Discussions
- **Documentation**: [Online Docs](https://github.com/makr-code/wordpressPlugins)

### Enterprise Support
- **Email**: enterprise@themisdb.org
- **SLA**: 24/7 support available
- **Phone**: +49 (0) XXX XXXXXXX

### Suggestions for Improvement
We welcome your feedback:
1. Open an issue on GitHub
2. Describe your use case
3. Suggest improvements

## Additional Resources

- 📖 **README.md** - Technical overview
- 🚀 **QUICKSTART.md** - Quick start guide
- 🔧 **INSTALLATION.md** - Detailed installation
- 💻 **SHORTCODES.md** - Modular shortcodes
- 📊 **COMPARISON.md** - Detailed comparison
- 🏗️ **IMPLEMENTATION.md** - Technical implementation

---

**Version:** 1.0.0  
**Last Updated:** January 2026  
**License:** MIT
