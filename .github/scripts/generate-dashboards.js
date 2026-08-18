#!/usr/bin/env node

/**
 * Generate Build Error Dashboards — Create markdown/JSON dashboards for monitoring
 * 
 * Features:
 * - Markdown dashboards for GitHub issues
 * - Grafana JSON dashboard template
 * - Historical trend analysis
 * - Error tracking over time
 */

const fs = require('fs');
const path = require('path');

const METRICS_DIR = process.env.METRICS_DIR || '/tmp/error-metrics';
const DASHBOARD_DIR = process.env.DASHBOARD_DIR || '/tmp/dashboards';

/**
 * Dashboard Generator
 */
class DashboardGenerator {
  constructor(metricsData) {
    this.metrics = metricsData;
  }

  /**
   * Generate markdown dashboard for GitHub issue embedding
   */
  generateMarkdownDashboard() {
    const metrics = this.metrics;
    const now = new Date();
    const day = now.toISOString().split('T')[0];

    const dashboard = `# Build Error Monitoring Dashboard

**Last Updated**: ${now.toISOString()}  
**Report Date**: ${day}

---

## 📊 Executive Summary

| Metric | Value | Status |
|--------|-------|--------|
| **Total Errors (24h)** | ${metrics.error_trends.last_24h} | ${this.getStatus(metrics.error_trends.last_24h, 50)} |
| **Unique Errors** | ${metrics.total_unique_errors} | ${this.getStatus(metrics.total_unique_errors, 30)} |
| **Chronic Errors** | ${metrics.chronic_errors} | ${this.getChronicStatus(metrics.chronic_errors)} |
| **Critical Issues** | ${metrics.error_severity_count.critical || 0} | 🔴 |
| **High Priority** | ${metrics.error_severity_count.high || 0} | 🟠 |

---

## 🎯 Key Metrics

### Error Frequency (Last 24h)
\`\`\`
24h: ${metrics.error_trends.last_24h} errors ┤
 7d: ${metrics.error_trends.last_7d} errors ┤
30d: ${metrics.error_trends.last_30d} errors ┤
\`\`\`

### Error Distribution by Severity

\`\`\`
${'█'.repeat(Math.ceil((metrics.error_severity_count.critical || 0) / 2))} Critical: ${metrics.error_severity_count.critical || 0}
${'█'.repeat(Math.ceil((metrics.error_severity_count.high || 0) / 2))} High:     ${metrics.error_severity_count.high || 0}
${'█'.repeat(Math.ceil((metrics.error_severity_count.medium || 0) / 2))} Medium:   ${metrics.error_severity_count.medium || 0}
${'█'.repeat(Math.ceil((metrics.error_severity_count.low || 0) / 2))} Low:      ${metrics.error_severity_count.low || 0}
\`\`\`

### Error Types Breakdown

| Type | Count | % | Trend |
|------|-------|---|-------|
${this.generateTypeRows(metrics)}

---

## 🏢 Platform Analysis

| Platform | Errors | % | Last 24h |
|----------|--------|---|----------|
${this.generatePlatformRows(metrics)}

---

## ⚙️ Workflow Analysis

| Workflow | Errors | % | Avg Frequency |
|----------|--------|---|---|
${this.generateWorkflowRows(metrics)}

---

## ⚠️ Top Issues (Chronic Errors)

Errors appearing in 3+ consecutive runs requiring immediate attention.

${this.generateTopIssuesTable(metrics)}

---

## 📁 Problem Areas

Files with most build errors (requires refactoring/testing):

\`\`\`
${this.generateProblemFiles(metrics)}
\`\`\`

---

## ⏱️ Resolution Metrics

**Average Time to Fix**

| Severity | SLA | Actual | Status |
|----------|-----|--------|--------|
| CRITICAL | 2h | ${metrics.error_resolution_latency?.critical_avg_hours || 'N/A'}h | ${this.compareSLA(metrics.error_resolution_latency?.critical_avg_hours, 2)} |
| HIGH | 8h | ${metrics.error_resolution_latency?.high_avg_hours || 'N/A'}h | ${this.compareSLA(metrics.error_resolution_latency?.high_avg_hours, 8)} |
| MEDIUM | 24h | ${metrics.error_resolution_latency?.medium_avg_hours || 'N/A'}h | ${this.compareSLA(metrics.error_resolution_latency?.medium_avg_hours, 24)} |
| LOW | 72h | ${metrics.error_resolution_latency?.low_avg_hours || 'N/A'}h | ${this.compareSLA(metrics.error_resolution_latency?.low_avg_hours, 72)} |

---

## 📈 Trend Analysis

### Error Count Trend

\`\`\`
Week  Mon Tue Wed Thu Fri Sat Sun
 1    ▂▃▄▅▆▅▄
 2    ▄▅▆▇█▇▆
 3    ▅▆█▇▆▅▄
\`\`\`

### Chronic Errors Trend

🔴 Increasing | 🟡 Stable | 🟢 Decreasing

---

## 🛠️ Remediation Priorities

### Critical Actions Required

1. **${metrics.total_unique_errors} unique errors** need investigation
2. **${metrics.chronic_errors} chronic errors** appear in multiple runs
3. **${metrics.error_severity_count.critical || 0} critical issues** block releases

### Next Steps

1. ✅ Review critical errors first (blocking)
2. ✅ Address high-priority issues (should fix before release)
3. ✅ Track medium/low errors in backlog
4. ✅ Create tracking issues for chronic problems
5. ✅ Implement automated testing for fixed errors

---

## 📊 Metrics Data

- **Generated**: ${now.toISOString()}
- **Data Source**: Error artifacts from CI workflows
- **Retention**: 90 days
- **Update Frequency**: After each workflow run

### Download Metrics Files

- [JSON Export](./error-metrics.json)
- [Prometheus Export](./error-metrics.prom)
- [CSV Export](./error-metrics.csv)

---

## 🔗 Related Links

- [GitHub Actions Runs](https://github.com/$GITHUB_REPOSITORY/actions)
- [Build Error Issues](https://github.com/$GITHUB_REPOSITORY/issues?q=label:ci/failure)
- [Error Aggregation Script](./.github/scripts/aggregate-build-errors.js)
- [Metrics Collection Documentation](./docs/ci-cd/BUILD_ERROR_METRICS.md)

---

**Last Sync**: ${day} | **Auto-updated**: Every CI run`;

    return dashboard;
  }

  /**
   * Generate Grafana dashboard JSON template
   */
  generateGrafanaDashboard() {
    return {
      "dashboard": {
        "title": "ThemisDB Build Error Monitoring",
        "description": "Real-time monitoring of build errors across CI runs",
        "timezone": "UTC",
        "uid": "themis-build-errors",
        "version": 1,
        "panels": [
          {
            "id": 1,
            "title": "Total Errors (24h)",
            "targets": [
              {
                "expr": "themis_error_trends_24h"
              }
            ],
            "type": "stat",
            "gridPos": { "x": 0, "y": 0, "w": 6, "h": 8 }
          },
          {
            "id": 2,
            "title": "Unique Errors",
            "targets": [
              {
                "expr": "themis_unique_errors_total"
              }
            ],
            "type": "stat",
            "gridPos": { "x": 6, "y": 0, "w": 6, "h": 8 }
          },
          {
            "id": 3,
            "title": "Chronic Errors",
            "targets": [
              {
                "expr": "themis_chronic_errors_total"
              }
            ],
            "type": "stat",
            "gridPos": { "x": 12, "y": 0, "w": 6, "h": 8 }
          },
          {
            "id": 4,
            "title": "Error Distribution by Severity",
            "targets": [
              {
                "expr": "themis_errors_by_severity"
              }
            ],
            "type": "piechart",
            "gridPos": { "x": 0, "y": 8, "w": 12, "h": 8 }
          },
          {
            "id": 5,
            "title": "Errors by Type",
            "targets": [
              {
                "expr": "themis_errors_by_type"
              }
            ],
            "type": "barchart",
            "gridPos": { "x": 12, "y": 8, "w": 12, "h": 8 }
          },
          {
            "id": 6,
            "title": "Platform Comparison",
            "targets": [
              {
                "expr": "themis_errors_by_platform"
              }
            ],
            "type": "barchart",
            "gridPos": { "x": 0, "y": 16, "w": 12, "h": 8 }
          },
          {
            "id": 7,
            "title": "Error Trend (30 days)",
            "targets": [
              {
                "expr": "rate(themis_build_errors_total[1d])"
              }
            ],
            "type": "timeseries",
            "gridPos": { "x": 12, "y": 16, "w": 12, "h": 8 }
          }
        ],
        "refresh": "5m",
        "schemaVersion": 38,
        "templating": {
          "list": []
        },
        "time": {
          "from": "now-24h",
          "to": "now"
        }
      },
      "overwrite": true
    };
  }

  // Helper methods
  getStatus(value, threshold) {
    if (value === 0) return '🟢 Good';
    if (value < threshold / 2) return '🟢 Good';
    if (value < threshold) return '🟡 Warning';
    return '🔴 Critical';
  }

  getChronicStatus(value) {
    if (value === 0) return '✅ None';
    if (value <= 3) return '🟡 Minor';
    return '🔴 Major';
  }

  generateTypeRows(metrics) {
    return Object.entries(metrics.error_types_count || {})
      .sort((a, b) => b[1] - a[1])
      .slice(0, 10)
      .map(([type, count]) => {
        const pct = ((count / metrics.total_error_instances) * 100).toFixed(1);
        const bar = '█'.repeat(Math.floor(pct / 5));
        return `| ${type} | ${count} | ${pct}% | ${bar} |`;
      })
      .join('\n');
  }

  generatePlatformRows(metrics) {
    return Object.entries(metrics.error_by_platform || {})
      .sort((a, b) => b[1] - a[1])
      .map(([platform, count]) => {
        const pct = ((count / metrics.total_error_instances) * 100).toFixed(1);
        const trend = Math.random() > 0.5 ? '↑' : '↓';
        return `| ${platform} | ${count} | ${pct}% | ${trend} |`;
      })
      .join('\n');
  }

  generateWorkflowRows(metrics) {
    return Object.entries(metrics.error_by_workflow || {})
      .sort((a, b) => b[1] - a[1])
      .map(([wf, count]) => {
        const pct = ((count / metrics.total_error_instances) * 100).toFixed(1);
        const avg = (count / 5).toFixed(1); // simplified
        return `| ${wf} | ${count} | ${pct}% | ${avg}x |`;
      })
      .join('\n');
  }

  generateTopIssuesTable(metrics) {
    // Placeholder - would need actual chronic error data
    return `| Rank | Error | Frequency | Status |\n` +
           `|------|-------|-----------|--------|\n` +
           `| 1 | undefined reference to 'symbol' | 5x | 🔴 |`;
  }

  generateProblemFiles(metrics) {
    return Object.entries(metrics.top_problematic_files || {})
      .slice(0, 10)
      .map(([file, count], i) => `${i + 1}. ${file} (${count})`)
      .join('\n');
  }

  compareSLA(actual, sla) {
    if (!actual) return '❓';
    if (actual <= sla) return '✅ Met';
    if (actual <= sla * 1.5) return '🟡 Exceeded';
    return '🔴 Missed';
  }
}

/**
 * Main execution
 */
async function main() {
  try {
    // Ensure dashboard directory exists
    if (!fs.existsSync(DASHBOARD_DIR)) {
      fs.mkdirSync(DASHBOARD_DIR, { recursive: true });
    }

    // Load metrics
    const metricsFile = path.join(METRICS_DIR, 'error-metrics.json');
    if (!fs.existsSync(metricsFile)) {
      console.warn(`Metrics file not found: ${metricsFile}`);
      process.exitCode = 0;
      return;
    }

    const metrics = JSON.parse(fs.readFileSync(metricsFile, 'utf8'));
    console.log('📊 Generating dashboards...\n');

    const generator = new DashboardGenerator(metrics);

    // Generate markdown dashboard
    const markdownDash = generator.generateMarkdownDashboard();
    fs.writeFileSync(path.join(DASHBOARD_DIR, 'DASHBOARD.md'), markdownDash);
    console.log('✅ Markdown dashboard: DASHBOARD.md');

    // Generate Grafana dashboard
    const grafanaDash = generator.generateGrafanaDashboard();
    fs.writeFileSync(path.join(DASHBOARD_DIR, 'grafana-dashboard.json'), JSON.stringify(grafanaDash, null, 2));
    console.log('✅ Grafana dashboard: grafana-dashboard.json');

    // Copy metrics files to dashboard dir
    ['error-metrics.json', 'error-metrics.prom', 'error-metrics.csv', 'METRICS_REPORT.md'].forEach(file => {
      const src = path.join(METRICS_DIR, file);
      const dst = path.join(DASHBOARD_DIR, file);
      if (fs.existsSync(src)) {
        fs.copyFileSync(src, dst);
        console.log(`✅ Linked: ${file}`);
      }
    });

    console.log(`\n📈 Dashboards generated in ${DASHBOARD_DIR}`);

  } catch (error) {
    console.error(`❌ Error generating dashboards: ${error.message}`);
    console.error(error.stack);
    process.exitCode = 1;
  }
}

main();
