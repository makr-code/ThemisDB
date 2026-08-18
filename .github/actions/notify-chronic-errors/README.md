# Notify Chronic Build Errors

A GitHub Actions composite action that sends rich alerts to Slack or Discord when build failures become chronic (appearing in N consecutive CI runs).

## Features

- **Chronic error detection**: Identifies errors appearing in ≥N consecutive runs
- **Multi-platform support**: Works with both Slack and Discord webhooks
- **Rich formatting**: Includes severity colors, error counts, and actionable links
- **Auto-platform detection**: Automatically detects Slack vs Discord from webhook URL
- **Remediation hints**: Suggests specific fix steps for each error category
- **Direct CI links**: Includes links to failed runs and GitHub issues

## Usage

### Slack Example

```yaml
- name: Notify chronic build errors to Slack
  if: failure()
  uses: ./.github/actions/notify-chronic-errors
  with:
    error-report-file: /tmp/aggregated-errors.md
    webhook-url: ${{ secrets.SLACK_WEBHOOK_URL }}
    chronic-threshold: '3'
    platform: slack
    include-remediation-hints: 'true'
```

### Discord Example

```yaml
- name: Notify chronic build errors to Discord
  if: failure()
  uses: ./.github/actions/notify-chronic-errors
  with:
    error-report-file: /tmp/aggregated-errors.json
    webhook-url: ${{ secrets.DISCORD_WEBHOOK_URL }}
    chronic-threshold: '3'
    platform: discord
```

### In maintenance-build-issues.yml

```yaml
- name: Send notifications for chronic errors
  if: steps.aggregate.outputs.chronic_errors > 0
  uses: ./.github/actions/notify-chronic-errors
  with:
    error-report-file: /tmp/aggregated-errors.md
    webhook-url: ${{ secrets.SLACK_WEBHOOK_URL }}
    chronic-threshold: '3'
```

## Inputs

| Input | Required | Default | Description |
|-------|----------|---------|-------------|
| `error-report-file` | Yes | — | Path to aggregated error report (JSON or markdown) |
| `webhook-url` | No | — | Slack or Discord webhook URL (from secrets) |
| `chronic-threshold` | No | `3` | Number of consecutive runs to trigger notification |
| `platform` | No | `auto` | Platform: `slack`, `discord`, or `auto` (auto-detect) |
| `include-remediation-hints` | No | `true` | Include suggested fix steps in notification |

## Outputs

| Output | Description |
|--------|-------------|
| `notification-sent` | `true` if notification was successfully sent |
| `notification-url` | URL to the notification message (if available) |
| `chronic-count` | Number of chronic errors detected |

## Setup Instructions

### Slack Webhook Setup

1. Go to your Slack workspace → **Settings & Administration**
2. Select **Manage Apps** → **Custom Integrations**
3. Create **Incoming Webhooks**
4. Add New Webhook → Select channel (e.g., #ci-notifications)
5. Copy the webhook URL
6. In GitHub repository settings → **Secrets and variables** → **Repository secrets**
7. Add secret: `SLACK_WEBHOOK_URL` = `<your-webhook-url>`

### Discord Webhook Setup

1. Open Discord server → Right-click channel → **Edit Channel**
2. Select **Integrations** → **Webhooks** → **Create Webhook**
3. Copy the webhook URL
4. In GitHub repository settings → **Secrets and variables** → **Repository secrets**
5. Add secret: `DISCORD_WEBHOOK_URL` = `<your-webhook-url>`

## Notification Examples

### Slack Notification

```
🚨 Chronic Build Failures Detected

Repository: makr-code/ThemisDB
Run: https://github.com/.../actions/runs/12345

Error Summary
• Chronic Errors: 5
• Threshold: ≥3 runs

1. [5x] undefined reference to 'symbol_name'
2. [4x] CMake Error: Could not find package 'foo'
3. [3x] AddressSanitizer: use-after-free
...

Remediation Steps
1. Open this workflow run
2. Download error artifacts (build-errors-*.json)
3. Review error details and affected files
4. Create PR with fix and reference this issue
5. Verify fix on next CI run

[View CI Run] [View Issues]
```

### Discord Notification

```
🚨 Chronic Build Failures in makr-code/ThemisDB 🚨
Run: https://github.com/.../actions/runs/12345

🚨 Chronic Build Failures Detected
Repository: makr-code/ThemisDB
Threshold: ≥3 consecutive runs

Chronic Errors
5 error(s) appearing repeatedly

Top Issues
1. [5x] undefined reference to 'symbol_name'
2. [4x] CMake Error: Could not find package 'foo'
3. [3x] AddressSanitizer: use-after-free
...

🛠️ Remediation Steps
1. Open the workflow run
2. Download error artifacts
3. Review error details
4. Create fix PR
5. Verify on next run
```

## Error Report Formats

### JSON Format

```json
{
  "metadata": { "run_id": 12345, "timestamp": "2026-08-18T11:00:00Z" },
  "errors": [
    {
      "type": "linker_error",
      "message": "undefined reference to 'symbol_name'",
      "frequency": 5,
      "runs": [100, 101, 102, 103, 104]
    }
  ]
}
```

### Markdown Format

```markdown
## ⚠️ Chronic Errors (Repeat Offenders)

1. [5x] undefined reference to 'symbol_name'
   - Runs: 100, 101, 102, 103, 104
   - First seen: 2026-08-18T10:00:00Z
```

## Platform Auto-Detection

The action automatically detects the platform from the webhook URL:

- URLs containing `hooks.slack.com` → Slack format
- URLs containing `discord.com` → Discord format
- Otherwise → Defaults to Slack format

You can override this with the `platform` input parameter.

## Integration with CI/CD

### Full Workflow Example

```yaml
- name: Aggregate build errors
  id: aggregate
  shell: bash
  run: node .github/scripts/aggregate-build-errors.js

- name: Create issue for chronic errors
  if: steps.aggregate.outputs.chronic_errors > 0
  uses: ./.github/actions/manage-governance-issue
  with:
    title: '🔴 Chronic Build Failures'
    body: ${{ steps.aggregate.outputs.report_markdown }}

- name: Notify team via Slack
  if: steps.aggregate.outputs.chronic_errors > 0
  uses: ./.github/actions/notify-chronic-errors
  with:
    error-report-file: /tmp/aggregated-errors.md
    webhook-url: ${{ secrets.SLACK_WEBHOOK_URL }}
    chronic-threshold: '3'
```

## Development

### Testing Locally

```bash
# Mock webhook (using webhook.site or similar)
export WEBHOOK_URL="https://webhook.site/your-unique-url"
export ERROR_REPORT_FILE="/tmp/sample-errors.json"
export CHRONIC_THRESHOLD="2"
export PLATFORM="slack"
export GITHUB_OUTPUT="/tmp/github-output.txt"

# Run the notification script
node .github/actions/notify-chronic-errors/notify-errors.js

# Check outputs
cat /tmp/github-output.txt
```

## Troubleshooting

### Notification Not Received

1. Check webhook URL is correct and active
2. Verify secrets are set in repository settings
3. Check workflow run logs for error messages
4. Test webhook URL with curl:
   ```bash
   curl -X POST -H 'Content-type: application/json' \
     --data '{"text":"Test message"}' \
     <your-webhook-url>
   ```

### Platform Not Detected

- Explicitly set `platform` input to `slack` or `discord`
- Webhook URL must contain `hooks.slack.com` or `discord.com`

## License

Same as repository.
