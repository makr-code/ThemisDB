#!/usr/bin/env node

/**
 * Aggregate Build Errors — Cross-run error aggregation and deduplication
 *
 * Features:
 * - Parses structured *-errors.json artifacts
 * - Parses compiler diagnostics from run logs (MSVC, GCC, LLVM/Clang)
 * - Scans src/ for GAP/SIMULATION/MOCKUP marker lines
 * - Groups diagnostics per module and file in src/
 * - Emits markdown + machine-readable grouped output
 */

const fs = require('fs');
const path = require('path');

const MAX_GROUPS_IN_REPORT = 120;
const MAX_ITEMS_PER_GROUP_IN_REPORT = 10;
const CHRONIC_THRESHOLD = 3;

const GAP_MARKER_PATTERN = /\b(GAP|SIMULATION|MOCKUP|STUB)\b/g;

function safeReadJson(filePath) {
  try {
    return JSON.parse(fs.readFileSync(filePath, 'utf8'));
  } catch {
    return null;
  }
}

function walkFiles(dir) {
  if (!fs.existsSync(dir)) return [];
  const out = [];
  const stack = [dir];

  while (stack.length > 0) {
    const current = stack.pop();
    let entries = [];
    try {
      entries = fs.readdirSync(current, { withFileTypes: true });
    } catch {
      continue;
    }

    for (const entry of entries) {
      const full = path.join(current, entry.name);
      if (entry.isDirectory()) {
        stack.push(full);
      } else if (entry.isFile()) {
        out.push(full);
      }
    }
  }

  return out;
}

function normalizePathForGrouping(filePath) {
  if (!filePath || typeof filePath !== 'string') return '';
  return filePath.replace(/\\/g, '/').replace(/^\.\//, '').trim();
}

function resolveModuleAndFile(filePath) {
  const normalized = normalizePathForGrouping(filePath);
  const marker = '/src/';
  const srcIdx = normalized.indexOf(marker);
  const srcRel = srcIdx >= 0 ? normalized.slice(srcIdx + marker.length) :
    (normalized.startsWith('src/') ? normalized.slice(4) : '');

  if (!srcRel) {
    return {
      module: 'external',
      file: normalized || '(unknown)',
      inSrc: false,
    };
  }

  const parts = srcRel.split('/').filter(Boolean);
  const module = parts.length > 0 ? parts[0] : 'unknown';
  return {
    module,
    file: `src/${srcRel}`,
    inSrc: true,
  };
}

function detectCompilerFromContext(line, fallback = 'unknown') {
  const l = String(line || '').toLowerCase();
  if (l.includes('cl.exe') || /\berror\s+C\d+\b/.test(line) || /\bwarning\s+C\d+\b/.test(line)) {
    return 'msvc';
  }
  if (l.includes('clang++') || l.includes('clang ') || l.includes('llvm')) {
    return 'clang';
  }
  if (l.includes('g++') || l.includes('gcc ')) {
    return 'gcc';
  }
  return fallback;
}

class ErrorAggregator {
  constructor() {
    this.fingerprints = new Map();
    this.byType = new Map();
    this.bySeverity = new Map();
    this.byModuleFile = new Map();
    this.totalInputFindings = 0;
  }

  addFinding(finding, metadata = {}) {
    if (!finding || typeof finding !== 'object') return;

    const type = finding.type || 'unknown';
    const severity = finding.severity || (type === 'compiler_warning' ? 'medium' : 'high');
    const compiler = finding.compiler || detectCompilerFromContext(finding.message || '', 'unknown');

    const loc = resolveModuleAndFile(finding.file || '');
    const normalized = {
      ...finding,
      type,
      severity,
      compiler,
      module: loc.module,
      file: loc.file,
      in_src: loc.inSrc,
      line: finding.line || null,
      column: finding.column || null,
      workflow: metadata.workflow || finding.workflow || 'unknown',
      run_id: metadata.run_id || finding.run_id || null,
      run_number: metadata.run_number || finding.run_number || null,
      timestamp: metadata.timestamp || finding.timestamp || null,
    };

    const fp = normalized.fingerprint || this.generateFingerprint(normalized);
    this.totalInputFindings += 1;

    if (this.fingerprints.has(fp)) {
      const existing = this.fingerprints.get(fp);
      existing.frequency += 1;
      if (normalized.run_number && !existing.runs.includes(normalized.run_number)) {
        existing.runs.push(normalized.run_number);
      }
      if (existing.sample_messages.length < 3 && normalized.message) {
        existing.sample_messages.push(normalized.message);
      }
    } else {
      this.fingerprints.set(fp, {
        finding: normalized,
        frequency: 1,
        runs: normalized.run_number ? [normalized.run_number] : [],
        firstSeen: normalized.timestamp,
        sample_messages: normalized.message ? [normalized.message] : [],
      });
    }
  }

  addErrorsFromRun(runData) {
    if (!runData || !Array.isArray(runData.errors)) return;
    const metadata = runData.metadata || {};
    for (const error of runData.errors) {
      this.addFinding(error, {
        run_id: metadata.run_id,
        run_number: metadata.run_number,
        workflow: metadata.workflow,
        timestamp: metadata.timestamp || metadata.created_at,
      });
    }
  }

  parseCompilerDiagnosticsFromLog(logContent, metadata = {}) {
    if (!logContent) return;

    const lines = String(logContent).split(/\r?\n/);
    let currentCompiler = 'unknown';

    const msvcRegex = /^.*?([A-Za-z]:\\[^:(]+|[^:(\s][^:(]*?)\((\d+)(?:,(\d+))?\):\s*(fatal error|error|warning)\s*(C\d+)\s*:\s*(.+)$/;
    const gccClangRegex = /^.*?([^:\n]+):(\d+):(\d+):\s*(fatal error|error|warning):\s*(.+)$/;
    const gccClangNoColRegex = /^.*?([^:\n]+):(\d+):\s*(fatal error|error|warning):\s*(.+)$/;

    for (const rawLine of lines) {
      const line = rawLine.replace(/^\d{4}-\d{2}-\d{2}T[^ ]+Z\s+/, '');
      currentCompiler = detectCompilerFromContext(line, currentCompiler);

      let m = line.match(msvcRegex);
      if (m) {
        const [, file, lineNo, colNo, level, code, message] = m;
        this.addFinding({
          type: level === 'warning' ? 'compiler_warning' : 'compiler_error',
          subtype: 'msvc',
          compiler: 'msvc',
          message: `${code}: ${message}`.trim(),
          file,
          line: Number(lineNo),
          column: colNo ? Number(colNo) : null,
          code,
          severity: level === 'warning' ? 'medium' : 'high',
        }, metadata);
        continue;
      }

      m = line.match(gccClangRegex);
      if (m) {
        const [, file, lineNo, colNo, level, message] = m;
        const compiler = currentCompiler === 'unknown' ? 'gcc_or_clang' : currentCompiler;
        this.addFinding({
          type: level === 'warning' ? 'compiler_warning' : 'compiler_error',
          subtype: compiler,
          compiler,
          message: message.trim(),
          file,
          line: Number(lineNo),
          column: Number(colNo),
          severity: level === 'warning' ? 'medium' : 'high',
        }, metadata);
        continue;
      }

      m = line.match(gccClangNoColRegex);
      if (m) {
        const [, file, lineNo, level, message] = m;
        const compiler = currentCompiler === 'unknown' ? 'gcc_or_clang' : currentCompiler;
        this.addFinding({
          type: level === 'warning' ? 'compiler_warning' : 'compiler_error',
          subtype: compiler,
          compiler,
          message: message.trim(),
          file,
          line: Number(lineNo),
          column: null,
          severity: level === 'warning' ? 'medium' : 'high',
        }, metadata);
      }
    }
  }

  scanGapMarkersInSrc(repoRoot) {
    const srcRoot = path.join(repoRoot, 'src');
    if (!fs.existsSync(srcRoot)) return;

    const files = walkFiles(srcRoot).filter((filePath) => {
      const ext = path.extname(filePath).toLowerCase();
      return [
        '.c', '.cc', '.cpp', '.cxx', '.h', '.hh', '.hpp', '.hxx',
        '.ipp', '.tpp', '.inl', '.ixx',
      ].includes(ext);
    });

    for (const filePath of files) {
      let content;
      try {
        content = fs.readFileSync(filePath, 'utf8');
      } catch {
        continue;
      }

      const lines = content.split(/\r?\n/);
      lines.forEach((line, idx) => {
        GAP_MARKER_PATTERN.lastIndex = 0;
        const markers = [];
        let match;
        while ((match = GAP_MARKER_PATTERN.exec(line)) !== null) {
          markers.push(match[1].toUpperCase());
        }

        if (markers.length === 0) return;

        const uniqueMarkers = Array.from(new Set(markers));
        for (const marker of uniqueMarkers) {
          this.addFinding({
            type: 'gap_marker',
            subtype: marker.toLowerCase(),
            compiler: 'n/a',
            message: `${marker} marker found`,
            file: normalizePathForGrouping(path.relative(repoRoot, filePath)),
            line: idx + 1,
            column: null,
            severity: marker === 'STUB' ? 'medium' : 'low',
            marker,
            source_line: line.trim().slice(0, 300),
          }, {
            workflow: 'source-scan',
            run_id: null,
            run_number: null,
            timestamp: new Date().toISOString(),
          });
        }
      });
    }
  }

  generateFingerprint(finding) {
    const parts = [
      finding.type,
      finding.subtype || '',
      finding.compiler || '',
      finding.file || '',
      finding.line || '',
      finding.code || '',
      (finding.message || '').substring(0, 120),
    ];
    return parts.filter(Boolean).join(':');
  }

  aggregate() {
    for (const [, data] of this.fingerprints) {
      const f = {
        ...data.finding,
        frequency: data.frequency,
        runs: data.runs,
        firstSeen: data.firstSeen,
        sample_messages: data.sample_messages,
      };

      if (!this.byType.has(f.type)) this.byType.set(f.type, []);
      this.byType.get(f.type).push(f);

      if (!this.bySeverity.has(f.severity)) this.bySeverity.set(f.severity, []);
      this.bySeverity.get(f.severity).push(f);

      const groupKey = `${f.module}::${f.file}`;
      if (!this.byModuleFile.has(groupKey)) {
        this.byModuleFile.set(groupKey, {
          module: f.module,
          file: f.file,
          diagnostics: [],
          counts: {
            total: 0,
            compiler_error: 0,
            compiler_warning: 0,
            gap_marker: 0,
          },
        });
      }

      const group = this.byModuleFile.get(groupKey);
      group.diagnostics.push(f);
      group.counts.total += f.frequency || 1;
      if (f.type in group.counts) {
        group.counts[f.type] += f.frequency || 1;
      }
    }

    for (const [, arr] of this.byType) {
      arr.sort((a, b) => (b.frequency || 0) - (a.frequency || 0));
    }
    for (const [, arr] of this.bySeverity) {
      arr.sort((a, b) => (b.frequency || 0) - (a.frequency || 0));
    }
    for (const [, group] of this.byModuleFile) {
      group.diagnostics.sort((a, b) => (b.frequency || 0) - (a.frequency || 0));
    }
  }

  getStats() {
    const compilerCounts = {};
    for (const [, data] of this.fingerprints) {
      const compiler = data.finding.compiler || 'unknown';
      compilerCounts[compiler] = (compilerCounts[compiler] || 0) + 1;
    }

    return {
      total_input_findings: this.totalInputFindings,
      total_unique_errors: this.fingerprints.size,
      by_type: Object.fromEntries(Array.from(this.byType.entries()).map(([k, v]) => [k, v.length])),
      by_severity: Object.fromEntries(Array.from(this.bySeverity.entries()).map(([k, v]) => [k, v.length])),
      by_compiler: compilerCounts,
      module_file_groups: this.byModuleFile.size,
      chronic_errors: Array.from(this.fingerprints.values()).filter((v) => v.frequency >= CHRONIC_THRESHOLD).length,
    };
  }

  getGroupedEntries() {
    return Array.from(this.byModuleFile.values())
      .filter((g) => g.module !== 'external')
      .sort((a, b) => b.counts.total - a.counts.total);
  }

  generateMarkdown() {
    const stats = this.getStats();
    let md = '';

    md += '## 🔍 Error Summary\n\n';
    md += `- **Total Input Findings**: ${stats.total_input_findings}\n`;
    md += `- **Total Unique Diagnostics**: ${stats.total_unique_errors}\n`;
    md += `- **Chronic Diagnostics** (appearing ≥${CHRONIC_THRESHOLD}x): ${stats.chronic_errors}\n`;
    md += `- **Module/File Groups**: ${stats.module_file_groups}\n\n`;

    md += '### By Compiler\n\n';
    Object.entries(stats.by_compiler)
      .sort((a, b) => b[1] - a[1])
      .forEach(([compiler, count]) => {
        md += `- **${compiler}**: ${count}\n`;
      });
    md += '\n';

    md += '### By Error Type\n\n';
    Object.entries(stats.by_type)
      .sort((a, b) => b[1] - a[1])
      .forEach(([type, count]) => {
        md += `- **${type}**: ${count}\n`;
      });
    md += '\n';

    const groups = this.getGroupedEntries();
    md += '## 📁 Diagnostics by Module/File (src/)\n\n';

    groups.slice(0, MAX_GROUPS_IN_REPORT).forEach((group) => {
      md += `### ${group.module} — ${group.file}\n\n`;
      md += `- Total: ${group.counts.total}\n`;
      md += `- Compiler Errors: ${group.counts.compiler_error}\n`;
      md += `- Compiler Warnings: ${group.counts.compiler_warning}\n`;
      md += `- GAP/SIMULATION/MOCKUP Markers: ${group.counts.gap_marker}\n\n`;

      group.diagnostics.slice(0, MAX_ITEMS_PER_GROUP_IN_REPORT).forEach((d, idx) => {
        const loc = d.line ? `${d.file}:${d.line}${d.column ? `:${d.column}` : ''}` : d.file;
        const freq = d.frequency ? `[${d.frequency}x] ` : '';
        const compiler = d.compiler && d.compiler !== 'n/a' ? ` (${d.compiler})` : '';
        md += `${idx + 1}. ${freq}**${d.type}**${compiler} — ${d.message || 'n/a'}\n`;
        md += `   - Location: ${loc}\n`;
        if (d.marker) md += `   - Marker: ${d.marker}\n`;
      });

      if (group.diagnostics.length > MAX_ITEMS_PER_GROUP_IN_REPORT) {
        md += `\n_... and ${group.diagnostics.length - MAX_ITEMS_PER_GROUP_IN_REPORT} more diagnostics_\n`;
      }
      md += '\n';
    });

    if (groups.length > MAX_GROUPS_IN_REPORT) {
      md += `\n_Only first ${MAX_GROUPS_IN_REPORT} module/file groups shown in markdown. Full data in JSON artifact._\n`;
    }

    return md;
  }
}

function findMetadataForInput(filePath) {
  const dir = path.dirname(filePath);
  const metadataPath = path.join(dir, '_metadata.json');
  const metadata = safeReadJson(metadataPath) || {};
  return {
    run_id: metadata.run_id || null,
    run_number: metadata.run_number || null,
    workflow: metadata.workflow || metadata.name || 'unknown',
    timestamp: metadata.created_at || metadata.timestamp || null,
  };
}

async function main() {
  const aggregator = new ErrorAggregator();

  const errorDir = process.env.ERROR_ARTIFACTS_DIR || '.';
  const outputFile = process.env.OUTPUT_FILE || '/tmp/aggregated-errors.md';
  const groupedOutputFile = process.env.GROUPED_OUTPUT_FILE || '/tmp/aggregated-errors-by-module-file.json';
  const repoRoot = process.env.GITHUB_WORKSPACE || process.cwd();

  const files = walkFiles(errorDir);
  const errorJsonFiles = files.filter((f) => f.endsWith('-errors.json'));
  const logFiles = files.filter((f) => f.endsWith('.log') || f.endsWith('.txt'));

  console.log(`📂 Found ${errorJsonFiles.length} structured error file(s)`);
  console.log(`📂 Found ${logFiles.length} log file(s)`);

  for (const filePath of errorJsonFiles) {
    const parsed = safeReadJson(filePath);
    if (!parsed) {
      console.warn(`⚠️ Failed to parse JSON: ${filePath}`);
      continue;
    }
    aggregator.addErrorsFromRun(parsed);
    console.log(`✅ Loaded ${path.relative(errorDir, filePath)}`);
  }

  for (const filePath of logFiles) {
    let content;
    try {
      content = fs.readFileSync(filePath, 'utf8');
    } catch {
      console.warn(`⚠️ Failed to read log: ${filePath}`);
      continue;
    }
    aggregator.parseCompilerDiagnosticsFromLog(content, findMetadataForInput(filePath));
  }

  aggregator.scanGapMarkersInSrc(repoRoot);
  aggregator.aggregate();

  const stats = aggregator.getStats();
  const markdown = aggregator.generateMarkdown();
  const grouped = {
    generated_at: new Date().toISOString(),
    stats,
    entries: aggregator.getGroupedEntries(),
  };

  fs.writeFileSync(outputFile, markdown);
  fs.writeFileSync(groupedOutputFile, JSON.stringify(grouped, null, 2));

  console.log(`\n📊 Aggregation Results:`);
  console.log(`  - Total input findings: ${stats.total_input_findings}`);
  console.log(`  - Unique diagnostics: ${stats.total_unique_errors}`);
  console.log(`  - Module/file groups: ${stats.module_file_groups}`);
  console.log(`  - Chronic diagnostics: ${stats.chronic_errors}`);
  console.log(`\n📝 Report written to: ${outputFile}`);
  console.log(`🧾 Grouped JSON written to: ${groupedOutputFile}`);

  const outputsText = [
    `total_errors=${stats.total_unique_errors}`,
    `chronic_errors=${stats.chronic_errors}`,
    `module_file_groups=${stats.module_file_groups}`,
    `has_chronic_errors=${stats.chronic_errors > 0 ? 'true' : 'false'}`,
  ].join('\n');

  if (process.env.GITHUB_OUTPUT) {
    fs.appendFileSync(process.env.GITHUB_OUTPUT, outputsText + '\n');
  }
}

main().catch((e) => {
  console.error(`❌ Error in aggregation: ${e.message}`);
  console.error(e.stack);
  process.exitCode = 1;
});
