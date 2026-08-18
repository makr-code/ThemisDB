# Publish Error Dashboard Action

Generates and publishes build error monitoring dashboards in multiple formats:
- **Markdown**: GitHub issue embedding
- **Grafana JSON**: For Prometheus integration
- **Artifacts**: JSON, Prometheus, CSV exports

## Usage

```yaml
- uses: ./.github/actions/publish-error-dashboard@main
  with:
    metrics-artifact: build-error-metrics
    create-issue: 'true'
    dashboard-title: 'Build Error Dashboard'
    send-slack: 'false'
```

## Inputs

| Input | Description | Default |
|-------|-------------|---------|
| `metrics-artifact` | Name of artifact with error-metrics.json | `build-error-metrics` |
| `metrics-dir` | Directory containing metrics files | `./metrics` |
| `issue-number` | GitHub issue to update (creates new if empty) | — |
| `create-issue` | Auto-create issue if not specified | `true` |
| `dashboard-title` | Title for dashboard issue | `Build Error Monitoring Dashboard` |
| `send-slack` | Send Slack notification | `false` |
| `slack-webhook` | Slack webhook URL (via secret) | — |

## Outputs

| Output | Description |
|--------|-------------|
| `dashboard-url` | URL to GitHub issue with dashboard |
| `dashboard-issue` | Issue number |
| `grafana-json-url` | URL to Grafana dashboard JSON |

## Examples

### Basic: Create Dashboard Issue

```yaml
- uses: ./.github/actions/publish-error-dashboard@main
  with:
    metrics-artifact: build-error-metrics
```

### Advanced: Create + Notify

```yaml
- id: dashboard
  uses: ./.github/actions/publish-error-dashboard@main
  with:
    metrics-artifact: build-error-metrics
    create-issue: 'true'
    send-slack: ${{ secrets.SLACK_BUILD_ALERTS != '' }}
    slack-webhook: ${{ secrets.SLACK_BUILD_ALERTS }}

- name: Comment on PR
  if: github.event.number
  run: |
    gh pr comment ${{ github.event.number }} -b \
      "📊 Dashboard available: ${{ steps.dashboard.outputs.dashboard-url }}"
```

### Full Integration

```yaml
- name: Aggregate Errors
  id: aggregate
  uses: ./.github/actions/capture-build-errors@main
  # ... error parsing steps

- name: Publish Dashboard
  if: steps.aggregate.outputs.has-errors == 'true'
  uses: ./.github/actions/publish-error-dashboard@main
  with:
    metrics-artifact: build-error-metrics
    dashboard-title: 'Build #${{ github.run_number }} - Error Report'
    create-issue: ${{ github.event_name == 'workflow_run' }}
    send-slack: 'true'
    slack-webhook: ${{ secrets.SLACK_ALERTS }}
```

## Features

- ✅ Markdown dashboard with metrics summary
- ✅ Grafana JSON template for advanced monitoring
- ✅ GitHub issue auto-creation/updating
- ✅ Slack notifications with dashboard link
- ✅ 90-day artifact retention
- ✅ Error trend visualization
- ✅ Severity-based prioritization
- ✅ Resolution SLA tracking

## Files Generated

```
dashboards/
├── DASHBOARD.md              # Markdown dashboard for GitHub
├── grafana-dashboard.json    # Grafana JSON template
├── error-metrics.json        # Raw metrics data
├── error-metrics.prom        # Prometheus format
├── error-metrics.csv         # CSV export
└── METRICS_REPORT.md         # Detailed metrics report
```

## Setup (Grafana Integration)

1. **Import Dashboard JSON**:
   ```bash
   curl -L https://github.com/$OWNER/$REPO/raw/branch/dashboards/grafana-dashboard.json \
     -o dashboard.json
   ```

2. **Import in Grafana UI**:
   - Dashboards → Import → Upload JSON
   - Select Prometheus data source
   - Click Import

3. **Configure Alerts**:
   - Set alert thresholds based on your SLAs
   - Route notifications to Slack/PagerDuty

## Slack Webhook Setup

1. Create incoming webhook in Slack workspace
2. Store in repository secret: `SLACK_BUILD_ALERTS`
3. Enable in action: `send-slack: 'true'`

```bash
# Test webhook
curl -X POST "$SLACK_WEBHOOK" \
  -H 'Content-Type: application/json' \
  -d '{"text":"Test message"}'
```

## Troubleshooting

**Dashboard not creating issue**:
- Check artifact exists: `gh run download <run-id> -n build-error-metrics`
- Verify permissions: `gh auth status`
- Check logs: `gh run view <run-id> --log`

**Slack notification not sending**:
- Validate webhook: Test with curl
- Check secrets: `gh secret list`
- Review action logs for errors

**Grafana dashboard empty**:
- Verify Prometheus data source
- Check metrics format: `promtool check metrics error-metrics.prom`
- Confirm scrape interval configuration

## Performance

- Dashboard generation: < 1 second
- Artifact upload: < 5 seconds
- Total action runtime: < 30 seconds (including metrics processing)

## Related

- [Build Error Metrics Documentation](./BUILD_ERROR_METRICS.md)
- [Build Error Dashboard Guide](./BUILD_ERROR_DASHBOARD.md)
- [Error Aggregation Script](./../scripts/aggregate-build-errors.js)
- [Notification Action](./notify-chronic-errors/README.md)
