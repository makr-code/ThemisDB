#!/usr/bin/env node

/**
 * Aggregate Build Errors — Cross-run error aggregation and deduplication
 * 
 * Features:
 * - Deduplicates errors across multiple workflow runs
 * - Tracks error frequency (how many runs have each error)
 * - Groups errors by severity and type
 * - Generates issue-ready markdown summaries
 * - Suggests related issues for linking
 */

const fs = require('fs');
const path = require('path');

const MAX_ERRORS_PER_CATEGORY = 10;
const CHRONIC_THRESHOLD = 3; // N consecutive runs = chronic

/**
 * Error Aggregator: combines errors from multiple sources
 */
class ErrorAggregator {
  constructor() {
    this.errors = []; // all errors from all runs
    this.fingerprints = new Map(); // fingerprint -> { error, frequency, runs, firstSeen }
    this.byType = new Map(); // type -> errors
    this.bySeverity = new Map(); // severity -> errors
  }

  addErrorsFromRun(runData) {
    if (!runData.errors || !Array.isArray(runData.errors)) {
      return;
    }

    for (const error of runData.errors) {
      const fp = error.fingerprint || this.generateFingerprint(error);
      
      if (this.fingerprints.has(fp)) {
        const existing = this.fingerprints.get(fp);
        existing.frequency++;
        existing.runs.push(runData.metadata.run_number);
        if (!existing.runs.includes(runData.metadata.run_number)) {
          existing.runs.push(runData.metadata.run_number);
        }
      } else {
        this.fingerprints.set(fp, {
          error,
          frequency: 1,
          runs: [runData.metadata.run_number],
          firstSeen: runData.metadata.timestamp
        });
      }
    }
  }

  generateFingerprint(error) {
    const parts = [
      error.type,
      error.subtype || '',
      error.file || '',
      error.line || '',
      error.symbol || '',
      error.message?.substring(0, 50) || ''
    ];
    return parts.filter(p => p).join(':');
  }

  aggregate() {
    // Group by type and severity
    for (const [fp, data] of this.fingerprints) {
      const error = { ...data.error, frequency: data.frequency, runs: data.runs };
      
      const type = error.type || 'unknown';
      if (!this.byType.has(type)) this.byType.set(type, []);
      this.byType.get(type).push(error);

      const severity = error.severity || 'info';
      if (!this.bySeverity.has(severity)) this.bySeverity.set(severity, []);
      this.bySeverity.get(severity).push(error);
    }

    // Sort each group by frequency (descending)
    for (const [, errors] of this.byType) {
      errors.sort((a, b) => (b.frequency || 0) - (a.frequency || 0));
    }
    for (const [, errors] of this.bySeverity) {
      errors.sort((a, b) => (b.frequency || 0) - (a.frequency || 0));
    }
  }

  getStats() {
    return {
      total_unique_errors: this.fingerprints.size,
      by_type: Object.fromEntries(
        Array.from(this.byType.entries()).map(([t, e]) => [t, e.length])
      ),
      by_severity: Object.fromEntries(
        Array.from(this.bySeverity.entries()).map(([s, e]) => [s, e.length])
      ),
      chronic_errors: Array.from(this.fingerprints.values())
        .filter(d => d.frequency >= CHRONIC_THRESHOLD)
        .length
    };
  }

  generateMarkdown() {
    let md = '';
    const stats = this.getStats();

    // Critical errors first
    const severityOrder = ['critical', 'high', 'medium', 'low', 'info'];
    
    md += '## 🔍 Error Summary\n\n';
    md += `- **Total Unique Errors**: ${stats.total_unique_errors}\n`;
    md += `- **Chronic Errors** (appearing ≥${CHRONIC_THRESHOLD}x): ${stats.chronic_errors}\n\n`;

    md += '### By Severity\n\n';
    for (const severity of severityOrder) {
      if (this.bySeverity.has(severity)) {
        const errors = this.bySeverity.get(severity);
        const icon = {
          critical: '🔴',
          high: '🟠',
          medium: '🟡',
          low: '🟢',
          info: '⚪'
        }[severity] || '❓';
        
        md += `${icon} **${severity.toUpperCase()}** (${errors.length})\n\n`;
        
        errors.slice(0, MAX_ERRORS_PER_CATEGORY).forEach((error, i) => {
          md += this.formatErrorMarkdown(error, i + 1);
        });

        if (errors.length > MAX_ERRORS_PER_CATEGORY) {
          md += `_... and ${errors.length - MAX_ERRORS_PER_CATEGORY} more ${severity} errors_\n\n`;
        }
      }
    }

    // By type
    md += '### By Error Type\n\n';
    
    const typeOrder = [
      'sanitizer_error',
      'linker_error',
      'compiler_error',
      'cmake_error',
      'dependency_error',
      'test_failure',
      'docker_error',
      'compiler_warning',
      'platform_error'
    ];

    for (const type of typeOrder) {
      if (this.byType.has(type)) {
        const errors = this.byType.get(type);
        md += `#### ${this.formatTypeName(type)} (${errors.length})\n\n`;
        
        errors.slice(0, MAX_ERRORS_PER_CATEGORY).forEach((error, i) => {
          const chronic = error.frequency >= CHRONIC_THRESHOLD ? ' 🔁' : '';
          md += `${i + 1}. [${error.frequency}x]${chronic} ${this.formatErrorSummary(error)}\n`;
        });

        if (errors.length > MAX_ERRORS_PER_CATEGORY) {
          md += `_... and ${errors.length - MAX_ERRORS_PER_CATEGORY} more_\n`;
        }
        md += '\n';
      }
    }

    // Chronic errors highlight
    const chronicErrors = Array.from(this.fingerprints.values())
      .filter(d => d.frequency >= CHRONIC_THRESHOLD)
      .sort((a, b) => b.frequency - a.frequency);

    if (chronicErrors.length > 0) {
      md += '## ⚠️ Chronic Errors (Repeat Offenders)\n\n';
      md += 'These errors appear in multiple consecutive builds and need immediate attention.\n\n';
      
      chronicErrors.slice(0, 15).forEach((data, i) => {
        const error = data.error;
        const runs = data.runs.length <= 3 
          ? data.runs.join(', ') 
          : `${data.runs.slice(0, 3).join(', ')} ... (${data.runs.length} runs)`;
        
        md += `${i + 1}. **[${data.frequency}x]** ${this.formatErrorSummary(error)}\n`;
        md += `   - Runs: ${runs}\n`;
        md += `   - First seen: ${data.firstSeen}\n`;
        md += `   - **Suggested action**: Create issue or review related PRs\n\n`;
      });
    }

    // Remediation section
    md += '## 🛠️ Remediation Steps\n\n';
    md += '### For Each Category:\n\n';
    md += '1. **Compiler Errors**: Check syntax, includes, and type definitions\n';
    md += '2. **Linker Errors**: Verify library linking, symbol exports, and dependencies\n';
    md += '3. **CMake Errors**: Check CMakeLists.txt dependencies and configuration\n';
    md += '4. **Sanitizer Errors**: Address memory safety issues immediately (CRITICAL)\n';
    md += '5. **Test Failures**: Review test expectations and environment setup\n';
    md += '6. **Docker Errors**: Check Dockerfile RUN commands and COPY paths\n\n';

    md += '### Next Steps:\n\n';
    md += '1. Click the error type links above to view details\n';
    md += '2. Review the CI build logs from the most frequent failing runs\n';
    md += '3. If chronic (🔁), create a GitHub issue to track the fix\n';
    md += '4. Reference this issue in pull requests that fix the errors\n\n';

    return md;
  }

  formatErrorMarkdown(error, index) {
    const chronic = (error.frequency || 0) >= CHRONIC_THRESHOLD ? ' 🔁' : '';
    const freq = error.frequency ? `[${error.frequency}x]${chronic} ` : '';
    
    let md = `${index}. ${freq}**${this.formatErrorSummary(error)}**\n`;
    
    if (error.file && error.line) {
      md += `   - Location: ${error.file}:${error.line}\n`;
    }
    if (error.symbol) {
      md += `   - Symbol: ${error.symbol}\n`;
    }
    if (error.runs && error.runs.length > 0) {
      const runs = error.runs.length <= 3
        ? error.runs.join(', ')
        : `${error.runs.slice(0, 3).join(', ')} + ${error.runs.length - 3} more`;
      md += `   - Failed runs: ${runs}\n`;
    }
    md += `\n`;
    
    return md;
  }

  formatErrorSummary(error) {
    const type = error.type || 'unknown';
    let summary = '';

    switch (type) {
      case 'compiler_error':
        summary = `Compiler Error: ${error.message || 'Unknown error'}`;
        if (error.file) summary += ` (${error.file}:${error.line})`;
        break;
      case 'compiler_warning':
        summary = `Warning: ${error.message || 'Unknown warning'}`;
        if (error.file) summary += ` (${error.file})`;
        break;
      case 'linker_error':
        if (error.subtype === 'undefined_reference') {
          summary = `Undefined reference: '${error.symbol}'`;
        } else if (error.subtype === 'multiple_definition') {
          summary = `Multiple definition: '${error.symbol}'`;
        } else {
          summary = `Linker error: ${error.message}`;
        }
        break;
      case 'dependency_error':
        if (error.subtype === 'missing_library') {
          summary = `Missing library: ${error.library}`;
        } else if (error.subtype === 'missing_header') {
          summary = `Missing header: ${error.header}`;
        } else {
          summary = `Dependency error: ${error.message}`;
        }
        break;
      case 'cmake_error':
        summary = `CMake Error: ${error.message || 'Configuration failure'}`;
        break;
      case 'sanitizer_error':
        summary = `${error.subtype?.toUpperCase() || 'Sanitizer'}: ${error.message || error.error_type}`;
        break;
      case 'test_failure':
        summary = `Test failure: ${error.test || error.message}`;
        break;
      case 'docker_error':
        summary = `Docker error: ${error.message || error.command}`;
        break;
      default:
        summary = error.message || `${type}: unknown error`;
    }

    return summary;
  }

  formatTypeName(type) {
    return type
      .split('_')
      .map(w => w.charAt(0).toUpperCase() + w.slice(1))
      .join(' ');
  }
}

/**
 * GitHub Issue Search: Find related issues for error linking
 */
async function findRelatedIssues(github, owner, repo, error) {
  try {
    // Build search query from error message
    let query = '';
    
    if (error.symbol) {
      query = `repo:${owner}/${repo} "${error.symbol}"`;
    } else if (error.message) {
      const msg = error.message.split(':')[0].trim().substring(0, 50);
      query = `repo:${owner}/${repo} "${msg}"`;
    } else {
      return [];
    }

    const { data } = await github.rest.search.issuesAndPullRequests({
      q: query,
      sort: 'updated',
      order: 'desc',
      per_page: 3
    });

    return data.items || [];
  } catch (e) {
    console.warn(`Failed to search issues: ${e.message}`);
    return [];
  }
}

/**
 * Main execution
 */
async function main() {
  const aggregator = new ErrorAggregator();

  // Read all error JSON files from stdin or directory
  const errorDir = process.env.ERROR_ARTIFACTS_DIR || '.';
  const errorFiles = process.env.ERROR_FILES ? 
    process.env.ERROR_FILES.split(',') : 
    fs.readdirSync(errorDir).filter(f => f.endsWith('-errors.json'));

  console.log(`📂 Found ${errorFiles.length} error file(s)\n`);

  let totalErrors = 0;
  for (const file of errorFiles) {
    const filePath = path.join(errorDir, file);
    if (fs.existsSync(filePath)) {
      try {
        const content = JSON.parse(fs.readFileSync(filePath, 'utf8'));
        aggregator.addErrorsFromRun(content);
        totalErrors += content.errors?.length || 0;
        console.log(`✅ Loaded ${file}: ${content.errors?.length || 0} errors`);
      } catch (e) {
        console.warn(`⚠️  Failed to parse ${file}: ${e.message}`);
      }
    }
  }

  // Aggregate errors
  aggregator.aggregate();
  const stats = aggregator.getStats();

  console.log(`\n📊 Aggregation Results:`);
  console.log(`  - Total input errors: ${totalErrors}`);
  console.log(`  - Unique errors: ${stats.total_unique_errors}`);
  console.log(`  - Chronic errors: ${stats.chronic_errors}`);

  // Generate markdown
  const markdown = aggregator.generateMarkdown();

  // Write output
  const outputFile = process.env.OUTPUT_FILE || '/tmp/aggregated-errors.md';
  fs.writeFileSync(outputFile, markdown);
  console.log(`\n📝 Report written to: ${outputFile}`);

  // Write GitHub Actions outputs
  const outputsText = `total_errors=${stats.total_unique_errors}
chronic_errors=${stats.chronic_errors}
has_chronic_errors=${stats.chronic_errors > 0 ? 'true' : 'false'}`;

  if (process.env.GITHUB_OUTPUT) {
    fs.appendFileSync(process.env.GITHUB_OUTPUT, outputsText + '\n');
  }

  return {
    stats,
    markdown,
    chronic_threshold: CHRONIC_THRESHOLD
  };
}

main().catch(e => {
  console.error(`❌ Error in aggregation: ${e.message}`);
  console.error(e.stack);
  process.exitCode = 1;
});
