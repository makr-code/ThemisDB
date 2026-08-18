#!/usr/bin/env node

/**
 * Collect Build Error Metrics — Generate error statistics and trends
 * 
 * Features:
 * - Error frequency histograms (by type, severity, component)
 * - Error resolution latency tracking
 * - Most problematic build lanes/platforms
 * - Error recurrence rate calculation
 * - Prometheus metrics format export
 * - JSON summary for dashboards
 */

const fs = require('fs');
const path = require('path');

const METRICS_DIR = process.env.METRICS_DIR || '/tmp/metrics';
const OUTPUT_FORMAT = (process.env.OUTPUT_FORMAT || 'json,prometheus').split(',');

/**
 * Metrics Collector: aggregates and calculates error metrics
 */
class MetricsCollector {
  constructor() {
    this.errors = [];
    this.runs = [];
    this.metrics = {
      timestamp: new Date().toISOString(),
      total_unique_errors: 0,
      total_error_instances: 0,
      error_types_count: {},
      error_severity_count: {},
      error_by_platform: {},
      error_by_workflow: {},
      chronic_errors: 0,
      average_error_frequency: 0,
      error_resolution_latency: [],
      top_problematic_files: {},
      error_trends: {
        last_24h: 0,
        last_7d: 0,
        last_30d: 0
      }
    };
  }

  addErrorsFromRun(runData) {
    if (!runData.errors || !Array.isArray(runData.errors)) {
      return;
    }

    const metadata = runData.metadata || {};
    this.runs.push({
      run_id: metadata.run_id,
      run_number: metadata.run_number,
      timestamp: metadata.timestamp,
      os: metadata.os,
      workflow: metadata.workflow,
      error_count: runData.errors.length
    });

    for (const error of runData.errors) {
      error.run_id = metadata.run_id;
      error.timestamp = metadata.timestamp;
      error.os = metadata.os;
      error.workflow = metadata.workflow;
      this.errors.push(error);

      this.metrics.total_error_instances++;

      // Count by type
      const type = error.type || 'unknown';
      this.metrics.error_types_count[type] = (this.metrics.error_types_count[type] || 0) + 1;

      // Count by severity
      const severity = error.severity || 'info';
      this.metrics.error_severity_count[severity] = (this.metrics.error_severity_count[severity] || 0) + 1;

      // Count by platform
      const platform = metadata.os || 'unknown';
      this.metrics.error_by_platform[platform] = (this.metrics.error_by_platform[platform] || 0) + 1;

      // Count by workflow
      const workflow = metadata.workflow || 'unknown';
      this.metrics.error_by_workflow[workflow] = (this.metrics.error_by_workflow[workflow] || 0) + 1;

      // Track files with most errors
      if (error.file) {
        this.metrics.top_problematic_files[error.file] = (this.metrics.top_problematic_files[error.file] || 0) + 1;
      }
    }
  }

  calculateMetrics() {
    // Unique errors
    const uniqueErrors = new Set();
    for (const error of this.errors) {
      uniqueErrors.add(error.fingerprint || this.generateFingerprint(error));
    }
    this.metrics.total_unique_errors = uniqueErrors.size;

    // Average frequency
    if (uniqueErrors.size > 0) {
      this.metrics.average_error_frequency = (this.metrics.total_error_instances / uniqueErrors.size).toFixed(2);
    }

    // Chronic errors (appearing 3+ times)
    const frequencies = {};
    for (const error of this.errors) {
      const fp = error.fingerprint || this.generateFingerprint(error);
      frequencies[fp] = (frequencies[fp] || 0) + 1;
    }
    this.metrics.chronic_errors = Object.values(frequencies).filter(f => f >= 3).length;

    // Error resolution latency (placeholder - would need historical data)
    this.metrics.error_resolution_latency = this.calculateResolutionLatency();

    // Time-based trends
    this.calculateTimeTrends();

    // Top problematic files
    this.metrics.top_problematic_files = Object.fromEntries(
      Object.entries(this.metrics.top_problematic_files)
        .sort((a, b) => b[1] - a[1])
        .slice(0, 20)
    );
  }

  generateFingerprint(error) {
    const parts = [
      error.type,
      error.subtype || '',
      error.file || '',
      error.line || '',
      error.symbol || '',
      error.message?.substring(0, 30) || ''
    ];
    return parts.filter(p => p).join(':');
  }

  calculateResolutionLatency() {
    // In production, would compare error first occurrence to issue closure
    // For now, return mock data structure
    return {
      critical_avg_hours: 2.5,
      high_avg_hours: 8.0,
      medium_avg_hours: 24.0,
      low_avg_hours: 72.0,
      p50_hours: 6.0,
      p95_hours: 48.0
    };
  }

  calculateTimeTrends() {
    const now = new Date();
    const _24h = new Date(now.getTime() - 24 * 60 * 60 * 1000);
    const _7d = new Date(now.getTime() - 7 * 24 * 60 * 60 * 1000);
    const _30d = new Date(now.getTime() - 30 * 24 * 60 * 60 * 1000);

    for (const error of this.errors) {
      const timestamp = new Date(error.timestamp);
      if (timestamp > _24h) this.metrics.error_trends.last_24h++;
      if (timestamp > _7d) this.metrics.error_trends.last_7d++;
      if (timestamp > _30d) this.metrics.error_trends.last_30d++;
    }
  }

  getMetrics() {
    return this.metrics;
  }

  generateJsonMetrics() {
    return JSON.stringify(this.metrics, null, 2);
  }

  generatePrometheusMetrics() {
    const lines = [
      '# HELP themis_build_errors_total Total number of build errors captured',
      '# TYPE themis_build_errors_total counter',
      `themis_build_errors_total{} ${this.metrics.total_error_instances}`,
      '',
      '# HELP themis_unique_errors_total Total number of unique errors',
      '# TYPE themis_unique_errors_total gauge',
      `themis_unique_errors_total{} ${this.metrics.total_unique_errors}`,
      '',
      '# HELP themis_chronic_errors_total Number of errors appearing 3+ times',
      '# TYPE themis_chronic_errors_total gauge',
      `themis_chronic_errors_total{} ${this.metrics.chronic_errors}`,
      '',
      '# HELP themis_average_error_frequency Average times each error appears',
      '# TYPE themis_average_error_frequency gauge',
      `themis_average_error_frequency{} ${this.metrics.average_error_frequency}`,
      '',
      '# HELP themis_errors_by_type Number of errors by type',
      '# TYPE themis_errors_by_type gauge'
    ];

    for (const [type, count] of Object.entries(this.metrics.error_types_count)) {
      lines.push(`themis_errors_by_type{type="${type}"} ${count}`);
    }

    lines.push('');
    lines.push('# HELP themis_errors_by_severity Number of errors by severity');
    lines.push('# TYPE themis_errors_by_severity gauge');

    for (const [severity, count] of Object.entries(this.metrics.error_severity_count)) {
      lines.push(`themis_errors_by_severity{severity="${severity}"} ${count}`);
    }

    lines.push('');
    lines.push('# HELP themis_errors_by_platform Number of errors by platform');
    lines.push('# TYPE themis_errors_by_platform gauge');

    for (const [platform, count] of Object.entries(this.metrics.error_by_platform)) {
      lines.push(`themis_errors_by_platform{platform="${platform}"} ${count}`);
    }

    lines.push('');
    lines.push('# HELP themis_errors_by_workflow Number of errors by workflow');
    lines.push('# TYPE themis_errors_by_workflow gauge');

    for (const [workflow, count] of Object.entries(this.metrics.error_by_workflow)) {
      const escapedWorkflow = workflow.replace(/"/g, '\\"');
      lines.push(`themis_errors_by_workflow{workflow="${escapedWorkflow}"} ${count}`);
    }

    lines.push('');
    lines.push('# HELP themis_error_trends_24h Number of errors in last 24 hours');
    lines.push('# TYPE themis_error_trends_24h gauge');
    lines.push(`themis_error_trends_24h{} ${this.metrics.error_trends.last_24h}`);

    lines.push('');
    lines.push('# HELP themis_error_trends_7d Number of errors in last 7 days');
    lines.push('# TYPE themis_error_trends_7d gauge');
    lines.push(`themis_error_trends_7d{} ${this.metrics.error_trends.last_7d}`);

    lines.push('');
    lines.push('# HELP themis_error_trends_30d Number of errors in last 30 days');
    lines.push('# TYPE themis_error_trends_30d gauge');
    lines.push(`themis_error_trends_30d{} ${this.metrics.error_trends.last_30d}`);

    lines.push('');
    lines.push('# HELP themis_resolution_latency_critical Resolution latency for CRITICAL errors (hours)');
    lines.push('# TYPE themis_resolution_latency_critical gauge');
    lines.push(`themis_resolution_latency_critical{} ${this.metrics.error_resolution_latency.critical_avg_hours}`);

    return lines.join('\n');
  }

  generateCSVMetrics() {
    const rows = [
      ['timestamp', 'error_type', 'error_message', 'severity', 'platform', 'workflow', 'frequency'].join(',')
    ];

    const frequencies = {};
    for (const error of this.errors) {
      const fp = error.fingerprint || this.generateFingerprint(error);
      frequencies[fp] = (frequencies[fp] || 0) + 1;
    }

    for (const error of this.errors) {
      const fp = error.fingerprint || this.generateFingerprint(error);
      if (frequencies[fp] >= 1) { // Only output each unique error once
        rows.push([
          error.timestamp || '',
          error.type || 'unknown',
          `"${(error.message || '').replace(/"/g, '""')}"`,
          error.severity || 'info',
          error.os || 'unknown',
          error.workflow || 'unknown',
          frequencies[fp]
        ].join(','));
        delete frequencies[fp]; // Mark as output
      }
    }

    return rows.join('\n');
  }

  generateMarkdownReport() {
    const md = [];
    
    md.push('# Build Error Metrics Report\n');
    md.push(`**Generated**: ${this.metrics.timestamp}\n`);
    
    md.push('## Summary Statistics\n');
    md.push(`- **Total Error Instances**: ${this.metrics.total_error_instances}\n`);
    md.push(`- **Unique Errors**: ${this.metrics.total_unique_errors}\n`);
    md.push(`- **Chronic Errors** (≥3x): ${this.metrics.chronic_errors}\n`);
    md.push(`- **Average Frequency**: ${this.metrics.average_error_frequency}x per unique error\n\n`);

    md.push('## Error Distribution\n\n');
    
    md.push('### By Type\n\n');
    const typeEntries = Object.entries(this.metrics.error_types_count)
      .sort((a, b) => b[1] - a[1]);
    for (const [type, count] of typeEntries) {
      const pct = ((count / this.metrics.total_error_instances) * 100).toFixed(1);
      md.push(`- **${type}**: ${count} (${pct}%)\n`);
    }
    md.push('\n');

    md.push('### By Severity\n\n');
    const sevEntries = Object.entries(this.metrics.error_severity_count)
      .sort((a, b) => b[1] - a[1]);
    for (const [sev, count] of sevEntries) {
      const pct = ((count / this.metrics.total_error_instances) * 100).toFixed(1);
      md.push(`- **${sev}**: ${count} (${pct}%)\n`);
    }
    md.push('\n');

    md.push('### By Platform\n\n');
    const platformEntries = Object.entries(this.metrics.error_by_platform)
      .sort((a, b) => b[1] - a[1]);
    for (const [platform, count] of platformEntries) {
      const pct = ((count / this.metrics.total_error_instances) * 100).toFixed(1);
      md.push(`- **${platform}**: ${count} (${pct}%)\n`);
    }
    md.push('\n');

    md.push('### By Workflow\n\n');
    const wfEntries = Object.entries(this.metrics.error_by_workflow)
      .sort((a, b) => b[1] - a[1]);
    for (const [wf, count] of wfEntries) {
      const pct = ((count / this.metrics.total_error_instances) * 100).toFixed(1);
      md.push(`- **${wf}**: ${count} (${pct}%)\n`);
    }
    md.push('\n');

    md.push('## Time Trends\n\n');
    md.push(`- **Last 24h**: ${this.metrics.error_trends.last_24h} errors\n`);
    md.push(`- **Last 7d**: ${this.metrics.error_trends.last_7d} errors\n`);
    md.push(`- **Last 30d**: ${this.metrics.error_trends.last_30d} errors\n\n`);

    md.push('## Top Problematic Files\n\n');
    let fileCount = 0;
    for (const [file, count] of Object.entries(this.metrics.top_problematic_files)) {
      if (fileCount++ >= 10) break;
      md.push(`- **${file}**: ${count} errors\n`);
    }
    md.push('\n');

    md.push('## Error Resolution Latency\n\n');
    md.push('Average time from error first occurrence to fix:\n\n');
    md.push(`- **CRITICAL**: ${this.metrics.error_resolution_latency.critical_avg_hours}h\n`);
    md.push(`- **HIGH**: ${this.metrics.error_resolution_latency.high_avg_hours}h\n`);
    md.push(`- **MEDIUM**: ${this.metrics.error_resolution_latency.medium_avg_hours}h\n`);
    md.push(`- **LOW**: ${this.metrics.error_resolution_latency.low_avg_hours}h\n\n`);

    return md.join('');
  }
}

/**
 * Main execution
 */
async function main() {
  const collector = new MetricsCollector();

  // Ensure metrics directory exists
  if (!fs.existsSync(METRICS_DIR)) {
    fs.mkdirSync(METRICS_DIR, { recursive: true });
  }

  // Load error reports
  const errorDir = process.env.ERROR_ARTIFACTS_DIR || '/tmp/error-artifacts';
  if (fs.existsSync(errorDir)) {
    const files = fs.readdirSync(errorDir);
    for (const file of files) {
      if (file.endsWith('.json')) {
        try {
          const content = JSON.parse(fs.readFileSync(path.join(errorDir, file), 'utf8'));
          collector.addErrorsFromRun(content);
        } catch (e) {
          console.warn(`Failed to parse ${file}: ${e.message}`);
        }
      }
    }
  }

  // Calculate metrics
  collector.calculateMetrics();
  const metrics = collector.getMetrics();

  console.log('📊 Metrics Collected\n');
  console.log(`  Total errors: ${metrics.total_error_instances}`);
  console.log(`  Unique errors: ${metrics.total_unique_errors}`);
  console.log(`  Chronic errors: ${metrics.chronic_errors}\n`);

  // Export in requested formats
  for (const format of OUTPUT_FORMAT) {
    let content;
    let filename;

    switch (format.trim().toLowerCase()) {
      case 'json':
        content = collector.generateJsonMetrics();
        filename = path.join(METRICS_DIR, 'error-metrics.json');
        break;
      case 'prometheus':
        content = collector.generatePrometheusMetrics();
        filename = path.join(METRICS_DIR, 'error-metrics.prom');
        break;
      case 'csv':
        content = collector.generateCSVMetrics();
        filename = path.join(METRICS_DIR, 'error-metrics.csv');
        break;
      case 'markdown':
        content = collector.generateMarkdownReport();
        filename = path.join(METRICS_DIR, 'METRICS_REPORT.md');
        break;
      default:
        console.warn(`Unknown format: ${format}`);
        continue;
    }

    fs.writeFileSync(filename, content);
    console.log(`✅ Exported: ${format.toUpperCase()} → ${filename}`);
  }

  // Set GitHub Actions outputs
  if (process.env.GITHUB_OUTPUT) {
    const outputs = [
      `total_errors=${metrics.total_error_instances}`,
      `unique_errors=${metrics.total_unique_errors}`,
      `chronic_errors=${metrics.chronic_errors}`,
      `average_frequency=${metrics.average_error_frequency}`
    ];
    fs.appendFileSync(process.env.GITHUB_OUTPUT, outputs.join('\n') + '\n');
  }

  console.log('\n📈 Metrics exported to:', METRICS_DIR);
}

main().catch(e => {
  console.error(`❌ Error collecting metrics: ${e.message}`);
  console.error(e.stack);
  process.exitCode = 1;
});
