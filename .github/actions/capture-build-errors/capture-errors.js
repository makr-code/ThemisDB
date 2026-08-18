#!/usr/bin/env node

/**
 * Capture Build Errors — Extract errors from build logs and export as JSON
 * 
 * Supports:
 * - GCC/Clang compiler errors: file:line:col: error: message
 * - MSVC compiler errors: file(line): error XXX: message
 * - CMake configuration errors: CMake Error at file:line
 * - Linker errors: undefined reference to, multiple definition of
 * - Test failures: from ctest output
 */

const fs = require('fs');
const path = require('path');

// Environment variables
const buildLogPath = process.env.BUILD_LOG_PATH || '';
const outputFile = process.env.OUTPUT_FILE || '';
const errorLimit = parseInt(process.env.ERROR_LIMIT || '20', 10);
const workspace = process.env.WORKSPACE || process.cwd();

const runId = process.env.RUN_ID || '';
const runNumber = process.env.RUN_NUMBER || '';
const workflow = process.env.WORKFLOW || '';
const event = process.env.EVENT || '';
const ref = process.env.REF || '';
const sha = process.env.SHA || '';
const actor = process.env.ACTOR || '';
const matrixOs = process.env.MATRIX_OS || '';

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
 * Parse errors from build log content
 */
function parseErrors(logContent) {
  const lines = logContent.split('\n');
  const errors = [];
  const errorTypes = new Set();

  // Track seen errors to avoid duplicates
  const seen = new Set();

  // GCC/Clang: file:line:col: error: message
  const gccErrorRe = /^([^:\s][^:]*):(\d+):\d+:\s+(error):\s+(.+)$/gm;
  let m;
  while ((m = gccErrorRe.exec(logContent)) !== null && errors.length < errorLimit) {
    const file = normalizePath(m[1]);
    const line = parseInt(m[2], 10);
    const message = m[4];
    const key = `compiler_error:${file}:${line}`;
    
    if (!seen.has(key)) {
      seen.add(key);
      errors.push({
        type: 'compiler_error',
        file,
        line,
        column: 1,
        message,
        context: extractContext(lines, line)
      });
      errorTypes.add('compiler_error');
    }
  }

  // MSVC: file(line): error XXX: message
  const msvcErrorRe = /^([^:()]+)\((\d+)\):\s+error\s+\w+:\s+(.+)$/gm;
  while ((m = msvcErrorRe.exec(logContent)) !== null && errors.length < errorLimit) {
    const file = normalizePath(m[1]);
    const line = parseInt(m[2], 10);
    const message = m[3];
    const key = `compiler_error:${file}:${line}`;
    
    if (!seen.has(key)) {
      seen.add(key);
      errors.push({
        type: 'compiler_error',
        file,
        line,
        column: 1,
        message,
        context: extractContext(lines, line)
      });
      errorTypes.add('compiler_error');
    }
  }

  // CMake errors: CMake Error at file:line
  const cmakeErrorRe = /CMake Error at ([^:]+):(\d+)\s+\(([^)]+)\):\s*\n\s+(.+?)(?=\n|$)/gm;
  while ((m = cmakeErrorRe.exec(logContent)) !== null && errors.length < errorLimit) {
    const file = normalizePath(m[1]);
    const line = parseInt(m[2], 10);
    const context = m[3];
    const message = m[4];
    const key = `cmake_error:${file}:${line}:${context}`;
    
    if (!seen.has(key)) {
      seen.add(key);
      errors.push({
        type: 'cmake_error',
        file,
        line,
        context,
        message,
        full_context: extractContext(lines, line)
      });
      errorTypes.add('cmake_error');
    }
  }

  // Linker errors: undefined reference to 'symbol'
  const linkerRe = /undefined reference to `([^']+)'|undefined reference to "([^"]+)"/gm;
  while ((m = linkerRe.exec(logContent)) !== null && errors.length < errorLimit) {
    const symbol = m[1] || m[2];
    const key = `linker_error:undefined:${symbol}`;
    
    if (!seen.has(key)) {
      seen.add(key);
      errors.push({
        type: 'linker_error',
        subtype: 'undefined_reference',
        symbol,
        message: `undefined reference to '${symbol}'`
      });
      errorTypes.add('linker_error');
    }
  }

  // Linker errors: multiple definition of 'symbol'
  const multidefRe = /multiple definition of `([^']+)'|multiple definition of "([^"]+)"/gm;
  while ((m = multidefRe.exec(logContent)) !== null && errors.length < errorLimit) {
    const symbol = m[1] || m[2];
    const key = `linker_error:multiple:${symbol}`;
    
    if (!seen.has(key)) {
      seen.add(key);
      errors.push({
        type: 'linker_error',
        subtype: 'multiple_definition',
        symbol,
        message: `multiple definition of '${symbol}'`
      });
      errorTypes.add('linker_error');
    }
  }

  // Test failures: ctest failures in output
  const testFailRe = /\*\*\*Exception:\s+(.+?)$/gm;
  while ((m = testFailRe.exec(logContent)) !== null && errors.length < errorLimit) {
    const message = m[1];
    const key = `test_failure:${message}`;
    
    if (!seen.has(key)) {
      seen.add(key);
      errors.push({
        type: 'test_failure',
        message
      });
      errorTypes.add('test_failure');
    }
  }

  return { errors: errors.slice(0, errorLimit), errorTypes: Array.from(errorTypes) };
}

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

    // Parse errors
    const { errors, errorTypes } = parseErrors(logContent);

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

    // Build error summary
    const summary = {
      total_errors: errors.length,
      error_types: errorTypes.reduce((acc, type) => {
        acc[type] = errors.filter(e => e.type === type).length;
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
    console.log(`📊 Error types: ${errorTypes.join(', ') || 'none'}`);

    // Write GitHub Actions outputs
    const outputsText = `error_count=${errors.length}
error_types=${errorTypes.join(',')}
has_errors=${errors.length > 0 ? 'true' : 'false'}`;
    
    fs.appendFileSync(process.env.GITHUB_OUTPUT, outputsText + '\n');

  } catch (error) {
    console.error(`❌ Error processing build logs: ${error.message}`);
    console.error(error.stack);
    process.exitCode = 1;
  }
}

main();
