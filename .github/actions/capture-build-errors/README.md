# Capture Build Errors

A composite GitHub Action that captures compiler, linker, CMake, and test errors from build logs and exports them as structured JSON artifacts.

## Features

- **Multi-format error parsing**: GCC/Clang, MSVC, CMake, Linker errors
- **Structured JSON output**: Unified error schema for all error types
- **Deduplication**: Automatically removes duplicate errors
- **No permissions required**: Only file I/O, no GitHub API access
- **Portable**: Works in any CI/CD workflow

## Supported Error Types

- `compiler_error`: GCC/Clang/MSVC syntax and compilation errors
- `linker_error`: Undefined references, multiple definitions
- `cmake_error`: CMake configuration failures
- `test_failure`: ctest/gtest runtime failures
- `unknown_error`: Other critical errors

## Usage

### Basic Example (ci-build.yml)

```yaml
- name: Capture build errors
  if: failure()
  uses: ./.github/actions/capture-build-errors
  with:
    build-log-path: ${{ steps.strings.outputs.build-output-dir }}/ctest-output.txt
    output-file: ${{ steps.strings.outputs.build-output-dir }}/build-errors.json
    error-limit: '20'

- name: Upload build errors
  if: always()
  uses: actions/upload-artifact@v4
  with:
    name: build-errors-${{ matrix.os }}-${{ matrix.c_compiler }}
    path: ${{ steps.strings.outputs.build-output-dir }}/build-errors.json
    retention-days: 7
```

### Docker Build Example (docker-image.yml)

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

- name: Upload docker errors
  if: always() && hashFiles('docker-errors.json') != ''
  uses: actions/upload-artifact@v4
  with:
    name: docker-errors
    path: docker-errors.json
```

## Inputs

| Input | Required | Default | Description |
|-------|----------|---------|-------------|
| `build-log-path` | Yes | — | Path to build/test log file |
| `output-file` | Yes | — | Path to write JSON error report |
| `error-limit` | No | `20` | Maximum errors to capture |
| `workspace` | No | `${{ github.workspace }}` | Root directory for path normalization |

## Outputs

| Output | Description |
|--------|-------------|
| `error-count` | Number of errors captured (e.g., `0`, `5`, `20+`) |
| `error-types` | Comma-separated error types (e.g., `compiler_error,linker_error`) |
| `has-errors` | `true` if `error-count > 0`, else `false` |

## Output Format

The generated JSON follows this schema:

```json
{
  "metadata": {
    "run_id": 12345,
    "run_number": 456,
    "workflow": "CI — Build",
    "event": "push",
    "ref": "refs/heads/develop",
    "sha": "abc123f",
    "actor": "octocat",
    "os": "Linux",
    "timestamp": "2026-08-18T11:00:00Z"
  },
  "summary": {
    "total_errors": 2,
    "error_types": {
      "compiler_error": 2,
      "linker_error": 0
    }
  },
  "errors": [
    {
      "type": "compiler_error",
      "file": "src/foo.cpp",
      "line": 42,
      "column": 1,
      "message": "undefined reference to 'bar'",
      "context": "src/foo.cpp:40-45 context lines"
    }
  ]
}
```

## Integration with maintenance-build-issues.yml

This action is designed to work with a downstream maintenance workflow that:

1. Downloads error JSON artifacts from failed builds
2. Deduplicates and aggregates errors
3. Creates/updates GitHub issues with unified error reports
4. Tracks chronic build failures

Example downstream workflow trigger:

```yaml
on:
  workflow_run:
    workflows: ["CI — Build", "Docker Image CI/CD"]
    types: [completed]
```

## Development

### Testing Locally

```bash
# Mock environment setup
export BUILD_LOG_PATH="sample-build.log"
export OUTPUT_FILE="build-errors.json"
export ERROR_LIMIT="20"
export GITHUB_OUTPUT="/tmp/github-output.txt"

# Run the parser
node .github/actions/capture-build-errors/capture-errors.js

# View output
cat build-errors.json
cat /tmp/github-output.txt
```

### Error Pattern Examples

The parser recognizes patterns like:

```
src/foo.cpp:42:10: error: undefined reference to 'bar'
src/bar.cpp:100:5: error: expected ';' before '}'
CMake Error at CMakeLists.txt:50 (find_package):
  Could not find a package configuration file
/usr/bin/ld: undefined reference to `symbol'
collect2: error: ld returned 1 exit status
```

## License

Same as repository.
