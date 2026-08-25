# Capture Build Errors

A composite GitHub Action that captures compiler, linker, CMake, dependency, sanitizer, and test errors from build logs and exports them as structured JSON artifacts with severity levels.

## Features

- **20+ error pattern types**: Compiler (GCC/Clang/MSVC), linker, CMake, dependency, sanitizer, test, Docker, platform-specific
- **Severity levels**: CRITICAL, HIGH, MEDIUM, LOW for prioritization
- **Structured JSON output**: Unified error schema with fingerprinting for all error types
- **Automatic deduplication**: Removes duplicate errors within a log via fingerprinting
- **Plugin-like matcher registration**: Easily extensible error pattern system
- **No permissions required**: Only file I/O, no GitHub API access
- **Portable**: Works in any CI/CD workflow

## Supported Error Types

| Category | Subtypes | Severity |
|----------|----------|----------|
| **compiler_error** | gcc_error, msvc_error | HIGH |
| **compiler_warning** | deprecation, conversion, unused, generic | LOW/MEDIUM |
| **linker_error** | undefined_reference, multiple_definition | HIGH |
| **dependency_error** | missing_library, missing_header | HIGH |
| **cmake_error** | configuration failure | HIGH |
| **cmake_warning** | configuration warning | LOW |
| **sanitizer_error** | asan, msan, ubsan | **CRITICAL** |
| **test_failure** | python, ctest, gtest | MEDIUM |
| **docker_error** | run_command, copy_error | HIGH |
| **platform_error** | windows_path, permission_denied | MEDIUM/HIGH |

## Usage

### Basic Example (build-mainline.yml)

```yaml
- name: Capture build errors
  if: failure()
  uses: ./.github/actions/capture-build-errors
  with:
    build-log-path: ${{ steps.strings.outputs.build-output-dir }}/ctest-output.txt
    output-file: ${{ steps.strings.outputs.build-output-dir }}/build-errors.json
    error-limit: '50'
    include-warnings: 'true'

- name: Upload build errors
  if: always()
  uses: actions/upload-artifact@v4
  with:
    name: build-errors-${{ matrix.os }}-${{ matrix.c_compiler }}
    path: ${{ steps.strings.outputs.build-output-dir }}/build-errors.json
    retention-days: 7
```

### Docker Build Example (release-docker-image.yml)

```yaml
- name: Build Docker image
  id: docker_build
  run: |
    docker build -t themisdb:latest . 2>&1 | tee docker-build.log || true

- name: Capture docker build errors
  if: failure()
  uses: ./.github/actions/capture-build-errors
  with:
    build-log-path: docker-build.log
    output-file: docker-errors.json
    error-limit: '30'

- name: Upload docker errors
  if: always() && hashFiles('docker-errors.json') != ''
  uses: actions/upload-artifact@v4
  with:
    name: docker-errors-${{ github.run_number }}
    path: docker-errors.json
```

## Inputs

| Input | Required | Default | Description |
|-------|----------|---------|-------------|
| `build-log-path` | Yes | — | Path to build/test log file |
| `output-file` | Yes | — | Path to write JSON error report |
| `error-limit` | No | `50` | Maximum errors to capture |
| `workspace` | No | `${{ github.workspace }}` | Root directory for path normalization |
| `include-warnings` | No | `true` | Include compiler warnings and non-critical messages |

## Outputs

| Output | Description |
|--------|-------------|
| `error-count` | Total number of errors and warnings captured |
| `error-types` | Comma-separated error type categories (e.g., `compiler_error,linker_error`) |
| `has-errors` | `true` if `error-count > 0`, else `false` |
| `critical-count` | Number of CRITICAL severity errors (sanitizer, undefined refs, etc.) |

## Output Format

The generated JSON follows this schema with severity levels:

```json
{
  "metadata": {
    "run_id": 12345,
    "run_number": 456,
    "workflow": "Build: Mainline",
    "event": "push",
    "ref": "refs/heads/develop",
    "sha": "abc123f",
    "actor": "octocat",
    "os": "Linux",
    "timestamp": "2026-08-18T11:00:00Z"
  },
  "summary": {
    "total_errors": 5,
    "by_type": {
      "compiler_error": 2,
      "sanitizer_error": 1,
      "linker_error": 2
    },
    "by_severity": {
      "critical": 1,
      "high": 4
    }
  },
  "errors": [
    {
      "type": "sanitizer_error",
      "subtype": "asan",
      "severity": "critical",
      "error_type": "use-after-free",
      "message": "AddressSanitizer: use-after-free on unknown address",
      "fingerprint": "sanitizer_error:asan:use-after-free"
    },
    {
      "type": "compiler_error",
      "subtype": "gcc_error",
      "severity": "high",
      "file": "src/foo.cpp",
      "line": 42,
      "column": 1,
      "message": "undefined reference to 'bar'",
      "fingerprint": "compiler_error:src/foo.cpp:42:undefined"
    }
  ]
}
```

## Integration with maintenance-build-issues.yml

This action is designed to work with downstream maintenance workflows that:

1. Download error JSON artifacts from failed builds
2. Deduplicate and aggregate errors across multiple runs
3. Group errors by type and severity
4. Create/update GitHub issues with unified error reports
5. Track chronic build failures and error trends

## Development

### Testing Locally

```bash
# Mock environment setup
export BUILD_LOG_PATH="sample-build.log"
export OUTPUT_FILE="build-errors.json"
export ERROR_LIMIT="50"
export INCLUDE_WARNINGS="true"
export GITHUB_OUTPUT="/tmp/github-output.txt"

# Run the parser
node .github/actions/capture-build-errors/capture-errors.js

# View output
cat build-errors.json
cat /tmp/github-output.txt
```

### Adding New Error Patterns

The action uses a plugin-like error matcher registry. Add new patterns in `capture-errors.js`:

```javascript
matcher.register('pattern-name', /regex/gm, (m) => ({
  type: 'category',
  subtype: 'specific_type',
  severity: SEVERITY.HIGH,
  message: m[1],
  fingerprint: generateFingerprint('category', { message: m[1] })
}));
```

### Error Pattern Examples

The parser recognizes patterns like:

```
src/foo.cpp:42:10: error: undefined reference to 'bar'
src/bar.cpp:100:5: warning: unused variable 'x'
src/main.cpp(42): error C2065: 'undefined_symbol' : undeclared identifier

CMake Error at CMakeLists.txt:50 (find_package):
  Could not find a package configuration file

==12345==ERROR: AddressSanitizer: use-after-free
runtime error: division by zero

docker: /usr/bin/ld: undefined reference to `symbol'
```

## License

Same as repository.
