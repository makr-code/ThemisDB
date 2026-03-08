# CI Test Reporting

This document describes the test reporting infrastructure in ThemisDB's CI/CD pipelines.

## Overview

ThemisDB uses automated test reporting across all CI workflows to provide immediate visibility into test results, track trends, and identify flaky tests. Test reports are automatically published as part of pull requests and workflow runs.

## Features

### 📊 Test Report Publishing

All CI workflows automatically generate and publish test reports using the [dorny/test-reporter](https://github.com/dorny/test-reporter) GitHub Action:

- **JUnit XML Format**: All tests output results in JUnit XML format for standardized reporting
- **In-PR Visualization**: Test results appear directly in pull requests
- **Detailed Annotations**: Failed tests are annotated with error details
- **Historical Tracking**: Test results are stored as artifacts for up to 30-90 days

### 🎯 Workflow Coverage

Test reporting is implemented in the following workflows:

#### Core CI Workflows
- `ci.yml` - Main CI workflow (Ubuntu)
- `develop-ci.yml` - Develop branch CI
- `feature-ci.yml` - Feature/bugfix branch CI
- `main-ci.yml` - Main branch verification (with critical issue creation)

#### SDK Test Workflows
- `java-sdk-test.yml` - Java SDK tests (Maven Surefire reports)
- `python-sdk-test.yml` - Python SDK tests (pytest JUnit XML)
- `go-sdk-test.yml` - Go SDK tests (go-junit-report)

### 🔔 Automatic Issue Creation

Test failures on critical branches trigger automatic issue creation:

- **Main Branch**: Critical issues are created when tests fail on release merges
- **Labels**: Automatically tagged with `test-failure`, `ci`, `automated`, and `critical` (for main branch)
- **Details**: Issues include workflow run links, commit information, and failure context

## Usage

### Viewing Test Reports

1. **In Pull Requests**: Navigate to the "Checks" tab in any PR to see test results
2. **Workflow Runs**: Click on any workflow run and view the "Publish Test Report" step
3. **Artifacts**: Download test result XML files from workflow artifacts

### Test Result Artifacts

Test results are uploaded as artifacts with the following naming convention:

- `test-results-ci-ubuntu` - Main CI test results
- `test-results-develop-linux` - Develop branch test results
- `test-results-feature` - Feature/bugfix test results
- `test-results-main` - Main branch test results
- `test-results-java-{version}` - Java SDK test results
- `test-results-python-{version}` - Python SDK test results
- `test-results-go-{version}` - Go SDK test results

### Coverage Reports

Coverage reports are also uploaded as artifacts:
- `coverage-report-{workflow-name}` - Coverage data (when available)
- Retention: 30 days for regular workflows, 90 days for main branch

## Configuration

### JUnit XML Output

#### C++ Tests (CTest)

CTest is configured to output JUnit XML using the `--output-junit` flag:

```bash
ctest -C Release --output-on-failure --output-junit test-results.xml
```

#### Java Tests (Maven)

Maven Surefire automatically generates JUnit XML reports in `target/surefire-reports/`:

```bash
mvn test -B
```

#### Python Tests (pytest)

Pytest is configured to output JUnit XML:

```bash
pytest tests/ -v --tb=short --junit-xml=test-results.xml
```

#### Go Tests

Go tests use `go-junit-report` to convert output to JUnit XML:

```bash
go test -v ./... 2>&1 | go-junit-report -set-exit-code > test-results.xml
```

### Test Reporter Configuration

The test reporter action is configured with:

```yaml
- name: Publish Test Report
  uses: dorny/test-reporter@v1
  if: always()
  with:
    name: 'Test Report Name'
    path: path/to/test-results.xml
    reporter: java-junit
    fail-on-error: true
    max-annotations: 50
```

## Reusable Workflow

A reusable workflow is available for standardized test reporting:

```yaml
# .github/workflows/reusable-test-report.yml
```

To use in other workflows:

```yaml
jobs:
  test:
    # ... run tests ...
  
  report:
    needs: test
    uses: ./.github/workflows/reusable-test-report.yml
    with:
      test-results-path: 'build/test-results.xml'
      report-name: 'My Test Report'
      fail-on-error: true
      create-issue-on-failure: true
```

## Badges

The README includes badges for test status and coverage:

- **Test Report Badge**: Links to latest workflow run with test results
- **Coverage Badge**: Links to coverage report (when available)

```markdown
[![Test Report](https://img.shields.io/badge/tests-view%20report-blue)](https://github.com/makr-code/ThemisDB/actions/workflows/themis-core-ci.yml)
[![Coverage](https://img.shields.io/badge/coverage-view%20report-brightgreen)](https://makr-code.github.io/ThemisDB/coverage/)
```

## Best Practices

1. **Always Use JUnit XML**: Ensure all test frameworks output JUnit XML format
2. **Continue on Error**: Use `continue-on-error: true` for test steps to allow reporting even on failures
3. **Upload Artifacts**: Always upload test results as artifacts for later analysis
4. **Retention Period**: 
   - Regular workflows: 30 days
   - Main/release workflows: 90 days
5. **Issue Creation**: Only enable automatic issue creation for critical branches (main, release)

## Troubleshooting

### Test Reporter Not Working

1. Ensure test results are in JUnit XML format
2. Verify the path to test results XML file is correct
3. Check that the workflow has `checks: write` permission
4. Confirm the `if: always()` condition is present

### Missing Test Results

1. Check that tests actually ran (look at test execution logs)
2. Verify test results file was created in expected location
3. Ensure artifact upload step ran (may be skipped on cancellation)

### Issue Creation Failing

1. Verify workflow has `issues: write` permission
2. Check that the GitHub token has required scopes
3. Review script syntax in issue creation step

## Future Enhancements

Planned improvements:

- [ ] Flaky test detection and tracking
- [ ] Test trend analysis and reporting
- [ ] Integration with external coverage services (Codecov, Coveralls)
- [ ] Custom dashboards for test metrics
- [ ] Automatic PR comments with test summaries

## References

- [dorny/test-reporter](https://github.com/dorny/test-reporter)
- [JUnit XML Format](https://llg.cubic.org/docs/junit/)
- [CTest Documentation](https://cmake.org/cmake/help/latest/manual/ctest.1.html)
- [GitHub Actions Workflow Syntax](https://docs.github.com/en/actions/using-workflows/workflow-syntax-for-github-actions)
