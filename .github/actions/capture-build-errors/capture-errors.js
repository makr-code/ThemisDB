#!/usr/bin/env node

/**
 * Capture Build Errors — Extract errors from build logs and export as JSON
 * 
 * Comprehensive error parser supporting:
 * - GCC/Clang compiler errors/warnings: file:line:col: error/warning: message
 * - MSVC compiler errors/warnings: file(line): error/warning XXX: message
 * - CMake configuration errors: CMake Error at file:line
 * - Linker errors: undefined reference to, multiple definition of
 * - Dependency resolution errors: missing libraries, unresolved symbols
 * - Sanitizer errors: AddressSanitizer, MemorySanitizer, UndefinedBehaviorSanitizer
 * - Test failures: from ctest/pytest output
 * - Platform-specific errors: Windows/Linux/macOS edge cases
 */

const fs = require('fs');
const path = require('path');

// Environment variables
const buildLogPath = process.env.BUILD_LOG_PATH || '';
const outputFile = process.env.OUTPUT_FILE || '';
const errorLimit = parseInt(process.env.ERROR_LIMIT || '50', 10);
const workspace = process.env.WORKSPACE || process.cwd();
const includeSuppressed = (process.env.INCLUDE_WARNINGS || 'true') === 'true';

const runId = process.env.RUN_ID || '';
const runNumber = process.env.RUN_NUMBER || '';
const workflow = process.env.WORKFLOW || '';
const event = process.env.EVENT || '';
const ref = process.env.REF || '';
const sha = process.env.SHA || '';
const actor = process.env.ACTOR || '';
const matrixOs = process.env.MATRIX_OS || '';

/**
 * Error severity levels: critical > high > medium > low > info
 */
const SEVERITY = {
  CRITICAL: 'critical',
  HIGH: 'high',
  MEDIUM: 'medium',
  LOW: 'low',
  INFO: 'info'
};

/**
 * Error matcher registry: plugin-like pattern matcher registration
 */
class ErrorMatcher {
  constructor() {
    this.matchers = [];
  }

  register(name, pattern, handler) {
    this.matchers.push({ name, pattern, handler });
  }

  match(content) {
    const errors = [];
    const seen = new Set();

    for (const matcher of this.matchers) {
      let m;
      const regex = new RegExp(matcher.pattern, 'gm');
      while ((m = regex.exec(content)) !== null && errors.length < errorLimit) {
        const error = matcher.handler(m);
        const key = `${error.type}:${error.fingerprint || JSON.stringify(error)}`;
        if (!seen.has(key)) {
          seen.add(key);
          errors.push(error);
        }
      }
    }

    return errors;
  }
}

const matcher = new ErrorMatcher();

/**
 * Normalize file path: remove workspace prefix and convert to forward slashes
 */
function normalizePath(filePath) {
  if (!filePath) return '';
  let normalized = filePath.replace(/\\/g, '/');
  normalized = normalized.replace(new RegExp(`^${workspace.replace(/\\/g, '/')}/`), '');
  return normalized;
}

/**
 * Extract context lines around an error (for debugging)
 */
function extractContext(lines, lineNum, contextLines = 2) {
  const start = Math.max(0, lineNum - 1 - contextLines);
  const end = Math.min(lines.length, lineNum + contextLines);
  return lines.slice(start, end).join('\n');
}

/**
 * Generate error fingerprint for deduplication
 */
function generateFingerprint(type, data) {
  const parts = [type];
  if (data.file) parts.push(data.file);
  if (data.line) parts.push(data.line);
  if (data.symbol) parts.push(data.symbol);
  if (data.message) parts.push(data.message.substring(0, 50));
  return parts.join(':');
}

// ============================================================================
// ERROR MATCHER REGISTRATION
// ============================================================================

// GCC/Clang: file:line:col: error: message
matcher.register('gcc-error', /^([^:\s][^:]*):(\d+):\d+:\s+(error):\s+(.+)$/gm, (m) => ({
  type: 'compiler_error',
  subtype: 'gcc_error',
  severity: SEVERITY.HIGH,
  file: normalizePath(m[1]),
  line: parseInt(m[2], 10),
  column: 1,
  message: m[4],
  fingerprint: generateFingerprint('compiler_error', { file: m[1], line: m[2], message: m[4] })
}));

// GCC/Clang: compiler warnings (conversion, deprecation, unused, etc.)
matcher.register('gcc-warning', /^([^:\s][^:]*):(\d+):\d+:\s+(warning):\s+(.+)$/gm, (m) => {
  const msg = m[4];
  let severity = SEVERITY.LOW;
  if (msg.includes('deprecated') || msg.includes('will be removed')) severity = SEVERITY.MEDIUM;
  if (msg.includes('conversion') || msg.includes('narrowing')) severity = SEVERITY.MEDIUM;
  if (msg.includes('undefined behavior') || msg.includes('always true')) severity = SEVERITY.HIGH;
  
  return {
    type: 'compiler_warning',
    subtype: msg.includes('deprecated') ? 'deprecation' : 
             msg.includes('conversion') ? 'conversion' : 
             msg.includes('unused') ? 'unused' : 'generic',
    severity,
    file: normalizePath(m[1]),
    line: parseInt(m[2], 10),
    column: 1,
    message: msg,
    fingerprint: generateFingerprint('compiler_warning', { file: m[1], line: m[2], message: msg })
  };
});

// MSVC: file(line): error XXX: message
matcher.register('msvc-error', /^([^:()]+)\((\d+)\):\s+error\s+\w+:\s+(.+)$/gm, (m) => ({
  type: 'compiler_error',
  subtype: 'msvc_error',
  severity: SEVERITY.HIGH,
  file: normalizePath(m[1]),
  line: parseInt(m[2], 10),
  column: 1,
  message: m[3],
  fingerprint: generateFingerprint('compiler_error', { file: m[1], line: m[2], message: m[3] })
}));

// MSVC: compiler warnings
matcher.register('msvc-warning', /^([^:()]+)\((\d+)\):\s+warning\s+\w+:\s+(.+)$/gm, (m) => {
  const msg = m[3];
  let severity = SEVERITY.LOW;
  if (msg.includes('deprecated') || msg.includes('obsolete')) severity = SEVERITY.MEDIUM;
  if (msg.includes('conversion') || msg.includes('truncation')) severity = SEVERITY.MEDIUM;
  
  return {
    type: 'compiler_warning',
    subtype: msg.includes('deprecated') ? 'deprecation' : 'generic',
    severity,
    file: normalizePath(m[1]),
    line: parseInt(m[2], 10),
    column: 1,
    message: msg,
    fingerprint: generateFingerprint('compiler_warning', { file: m[1], line: m[2], message: msg })
  };
});

// CMake errors: CMake Error at file:line
matcher.register('cmake-error', /CMake Error at ([^:]+):(\d+)\s+\(([^)]+)\):\s*\n\s+(.+?)(?=\n|$)/gm, (m) => ({
  type: 'cmake_error',
  severity: SEVERITY.HIGH,
  file: normalizePath(m[1]),
  line: parseInt(m[2], 10),
  context: m[3],
  message: m[4],
  fingerprint: generateFingerprint('cmake_error', { file: m[1], line: m[2], context: m[3] })
}));

// CMake warnings
matcher.register('cmake-warning', /CMake Warning at ([^:]+):(\d+)\s+\(([^)]+)\):\s*\n\s+(.+?)(?=\n|$)/gm, (m) => ({
  type: 'cmake_warning',
  severity: SEVERITY.LOW,
  file: normalizePath(m[1]),
  line: parseInt(m[2], 10),
  context: m[3],
  message: m[4],
  fingerprint: generateFingerprint('cmake_warning', { file: m[1], line: m[2], context: m[3] })
}));

// Linker errors: undefined reference to 'symbol'
matcher.register('linker-undefined', /undefined reference to `([^']+)'|undefined reference to "([^"]+)"/gm, (m) => {
  const symbol = m[1] || m[2];
  return {
    type: 'linker_error',
    subtype: 'undefined_reference',
    severity: SEVERITY.HIGH,
    symbol,
    message: `undefined reference to '${symbol}'`,
    fingerprint: generateFingerprint('linker_error', { type: 'undefined', symbol })
  };
});

// Linker errors: multiple definition of 'symbol'
matcher.register('linker-multiple', /multiple definition of `([^']+)'|multiple definition of "([^"]+)"/gm, (m) => {
  const symbol = m[1] || m[2];
  return {
    type: 'linker_error',
    subtype: 'multiple_definition',
    severity: SEVERITY.HIGH,
    symbol,
    message: `multiple definition of '${symbol}'`,
    fingerprint: generateFingerprint('linker_error', { type: 'multiple', symbol })
  };
});

// Linker errors: undefined reference (alternative format)
matcher.register('linker-ld-undefined', /\/usr\/bin\/ld.*:.*undefined reference to ['\`]([^'`]+)['\`]/gm, (m) => {
  const symbol = m[1];
  return {
    type: 'linker_error',
    subtype: 'undefined_reference',
    severity: SEVERITY.HIGH,
    symbol,
    message: `undefined reference to '${symbol}'`,
    fingerprint: generateFingerprint('linker_error', { type: 'undefined', symbol })
  };
});

// Linker errors: missing library
matcher.register('linker-missing-lib', /cannot find -l(\w+)|(-l\w+).*not found/gm, (m) => {
  const lib = m[1] || m[2];
  return {
    type: 'dependency_error',
    subtype: 'missing_library',
    severity: SEVERITY.HIGH,
    library: lib,
    message: `cannot find library '${lib}'`,
    fingerprint: generateFingerprint('dependency_error', { type: 'missing_lib', lib })
  };
});

// Dependency errors: missing header/include
matcher.register('dependency-missing-header', /fatal error:\s+([^\s:]+):\s+No such file or directory/gm, (m) => {
  const header = m[1];
  return {
    type: 'dependency_error',
    subtype: 'missing_header',
    severity: SEVERITY.HIGH,
    header,
    message: `missing header file '${header}'`,
    fingerprint: generateFingerprint('dependency_error', { type: 'missing_header', header })
  };
});

// Sanitizer errors: AddressSanitizer
matcher.register('asan-error', /==\d+==ERROR: AddressSanitizer:\s+([^\s]+)\s+(.+?)(?==\d+==ABORTING|$)/gm, (m) => {
  const errorType = m[1];
  const details = m[2];
  return {
    type: 'sanitizer_error',
    subtype: 'asan',
    severity: SEVERITY.CRITICAL,
    error_type: errorType,
    message: `AddressSanitizer: ${errorType} - ${details.substring(0, 100)}`,
    fingerprint: generateFingerprint('sanitizer_error', { subtype: 'asan', error_type: errorType })
  };
});

// Sanitizer errors: MemorySanitizer
matcher.register('msan-error', /WARNING: MemorySanitizer:\s+([^\n]+)/gm, (m) => ({
  type: 'sanitizer_error',
  subtype: 'msan',
  severity: SEVERITY.CRITICAL,
  message: `MemorySanitizer: ${m[1]}`,
  fingerprint: generateFingerprint('sanitizer_error', { subtype: 'msan', message: m[1] })
}));

// Sanitizer errors: UndefinedBehaviorSanitizer
matcher.register('ubsan-error', /runtime error:\s+(.+?)(?=\n|$)/gm, (m) => {
  const msg = m[1];
  let severity = SEVERITY.HIGH;
  if (msg.includes('division by zero') || msg.includes('out of bounds')) severity = SEVERITY.CRITICAL;
  
  return {
    type: 'sanitizer_error',
    subtype: 'ubsan',
    severity,
    message: `UBSanitizer: ${msg}`,
    fingerprint: generateFingerprint('sanitizer_error', { subtype: 'ubsan', message: msg })
  };
});

// Test failures: pytest/unittest
matcher.register('python-test-failure', /FAILED\s+(\S+)\s+-\s+(.+?)(?=\n|$)/gm, (m) => ({
  type: 'test_failure',
  subtype: 'python',
  severity: SEVERITY.MEDIUM,
  test: m[1],
  message: m[2],
  fingerprint: generateFingerprint('test_failure', { subtype: 'python', test: m[1] })
}));

// Test failures: ctest
matcher.register('ctest-failure', /Test project.*\n.*\*\*\*Exception:\s+(.+?)$/gm, (m) => ({
  type: 'test_failure',
  subtype: 'ctest',
  severity: SEVERITY.MEDIUM,
  message: m[1],
  fingerprint: generateFingerprint('test_failure', { subtype: 'ctest', message: m[1] })
}));

// Test failures: gtest
matcher.register('gtest-failure', /\[  FAILED  \]\s+(\S+)\s+\((.+?)\)/gm, (m) => ({
  type: 'test_failure',
  subtype: 'gtest',
  severity: SEVERITY.MEDIUM,
  test: m[1],
  message: m[2],
  fingerprint: generateFingerprint('test_failure', { subtype: 'gtest', test: m[1] })
}));

// Docker build errors: RUN command failure
matcher.register('docker-run-error', /Step \d+\/\d+ : RUN\s+(.+?)\n.*error:?\s+(.+?)(?=\n|$)/gm, (m) => ({
  type: 'docker_error',
  subtype: 'run_command',
  severity: SEVERITY.HIGH,
  command: m[1],
  message: m[2],
  fingerprint: generateFingerprint('docker_error', { subtype: 'run', command: m[1] })
}));

// Docker build errors: COPY/ADD failure
matcher.register('docker-copy-error', /COPY\s+(.+?)\s+(.+?)\n.*error:?\s+(.+?)(?=\n|$)/gm, (m) => ({
  type: 'docker_error',
  subtype: 'copy_error',
  severity: SEVERITY.HIGH,
  source: m[1],
  dest: m[2],
  message: m[3],
  fingerprint: generateFingerprint('docker_error', { subtype: 'copy', source: m[1] })
}));

// Platform-specific: Windows path resolution
matcher.register('windows-path-error', /Cannot access path.*The path '(.+?)' does not exist/gm, (m) => ({
  type: 'platform_error',
  subtype: 'windows_path',
  severity: SEVERITY.MEDIUM,
  path: m[1],
  message: `Windows path resolution error: ${m[1]}`,
  fingerprint: generateFingerprint('platform_error', { platform: 'windows', path: m[1] })
}));

// Platform-specific: Permission denied
matcher.register('permission-denied', /permission denied.*(.+?)(?=\n|$)/gm, (m) => ({
  type: 'platform_error',
  subtype: 'permission_denied',
  severity: SEVERITY.HIGH,
  path: m[1],
  message: `permission denied: ${m[1]}`,
  fingerprint: generateFingerprint('platform_error', { platform: 'generic', error: 'permission' })
}));

/**
 * Main execution
 */
async function main() {
  try {
    // Validate inputs
    if (!buildLogPath) {
      console.error('❌ BUILD_LOG_PATH not provided');
      process.exitCode = 1;
      return;
    }

    if (!outputFile) {
      console.error('❌ OUTPUT_FILE not provided');
      process.exitCode = 1;
      return;
    }

    // Read log file
    let logContent = '';
    if (fs.existsSync(buildLogPath)) {
      logContent = fs.readFileSync(buildLogPath, 'utf8');
    } else {
      console.warn(`⚠️  Build log not found: ${buildLogPath}`);
    }

    // Parse errors using matcher registry
    const errors = matcher.match(logContent);

    // Build metadata
    const metadata = {
      run_id: runId,
      run_number: runNumber,
      workflow,
      event,
      ref,
      sha: sha.substring(0, 7),
      actor,
      os: matrixOs,
      timestamp: new Date().toISOString()
    };

    // Build error summary by type
    const summary = {
      total_errors: errors.length,
      by_type: errors.reduce((acc, error) => {
        if (!acc[error.type]) acc[error.type] = 0;
        acc[error.type]++;
        return acc;
      }, {}),
      by_severity: errors.reduce((acc, error) => {
        const sev = error.severity || SEVERITY.INFO;
        if (!acc[sev]) acc[sev] = 0;
        acc[sev]++;
        return acc;
      }, {})
    };

    // Build full report
    const report = {
      metadata,
      summary,
      errors
    };

    // Ensure output directory exists
    const outputDir = path.dirname(outputFile);
    if (!fs.existsSync(outputDir)) {
      fs.mkdirSync(outputDir, { recursive: true });
    }

    // Write JSON report
    fs.writeFileSync(outputFile, JSON.stringify(report, null, 2));

    // Set outputs
    console.log(`✅ Captured ${errors.length} error(s) from ${buildLogPath}`);
    console.log(`📝 Report written to: ${outputFile}`);
    
    const errorTypes = Array.from(new Set(errors.map(e => e.type)));
    console.log(`📊 Error types: ${errorTypes.join(', ') || 'none'}`);
    console.log(`🔴 Critical: ${summary.by_severity[SEVERITY.CRITICAL] || 0}`);
    console.log(`🟠 High: ${summary.by_severity[SEVERITY.HIGH] || 0}`);
    console.log(`🟡 Medium: ${summary.by_severity[SEVERITY.MEDIUM] || 0}`);

    // Write GitHub Actions outputs
    const outputsText = `error_count=${errors.length}
error_types=${errorTypes.join(',')}
has_errors=${errors.length > 0 ? 'true' : 'false'}
critical_count=${summary.by_severity[SEVERITY.CRITICAL] || 0}`;
    
    fs.appendFileSync(process.env.GITHUB_OUTPUT, outputsText + '\n');

  } catch (error) {
    console.error(`❌ Error processing build logs: ${error.message}`);
    console.error(error.stack);
    process.exitCode = 1;
  }
}

main();
