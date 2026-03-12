# ThemisDB Test Dashboard - WordPress Plugin

Real-time test and quality metrics dashboard for ThemisDB.

## 📋 Overview

Displays test coverage, CI/CD status, and quality metrics for ThemisDB project.

- **Shortcode**: `[themisdb_test_dashboard]`
- **Data Source**: GitHub Actions, test reports
- **Real-time**: CI/CD pipeline status

## ✨ Features

### Test Metrics
- 📊 **Coverage Stats**: Line, branch, function coverage
- ✅ **Test Results**: Pass/fail rates and trends
- ⚡ **Performance Tests**: Benchmark trends over time
- 🎯 **Quality Gates**: Pass/fail status

### CI/CD Integration
- GitHub Actions workflow status
- Build success rates
- Test execution times
- Deployment status

### Visualizations
- Chart.js for metrics trends
- Mermaid.js for pipeline diagrams
- Real-time status indicators

## 🚀 Installation

1. Copy plugin to `/wp-content/plugins/themisdb-test-dashboard/`
2. Activate in WordPress Admin → Plugins
3. Configure GitHub token in Settings → Test Dashboard

## 📖 Usage

### Basic Dashboard
```php
[themisdb_test_dashboard]
```

### Specific Metrics
```php
[themisdb_test_dashboard metric="coverage"]
[themisdb_test_dashboard metric="ci_status"]
```

### Custom Period
```php
[themisdb_test_dashboard period="30d" show_trends="true"]
```

## ⚙️ Settings

- **GitHub Token**: For API access (Settings → Test Dashboard)
- **Repository**: makr-code/wordpressPlugins
- **Refresh Rate**: Configurable cache duration
- **Metrics Display**: Customize visible metrics

## 📄 License

MIT License

---

**Phase 3.2** - ThemisDB WordPress Plugins Suite
