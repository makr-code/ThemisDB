#!/usr/bin/env node

/**
 * Notify Chronic Build Errors — Send alerts to Slack/Discord for chronic failures
 * 
 * Features:
 * - Detects chronic errors (appearing in N consecutive runs)
 * - Formats rich notifications with error summaries
 * - Includes direct links to CI runs and suggested fixes
 * - Supports both Slack and Discord
 */

const fs = require('fs');
const path = require('path');
const https = require('https');
const http = require('http');

const CHRONIC_THRESHOLD = parseInt(process.env.CHRONIC_THRESHOLD || '3', 10);
const WEBHOOK_URL = process.env.WEBHOOK_URL || '';
const ERROR_REPORT_FILE = process.env.ERROR_REPORT_FILE || '';
const PLATFORM = process.env.PLATFORM || 'auto';
const INCLUDE_REMEDIATION = (process.env.INCLUDE_REMEDIATION || 'true') === 'true';

const GITHUB_RUN_URL = process.env.GITHUB_RUN_URL || '';
const GITHUB_REPO = process.env.GITHUB_REPO || '';

/**
 * Detect platform from webhook URL or explicit input.
 * Uses hostname comparison to prevent substring-in-URL bypass.
 */
function detectPlatform(url) {
  if (PLATFORM !== 'auto') return PLATFORM.toLowerCase();

  try {
    const hostname = new URL(url).hostname;
    if (hostname === 'hooks.slack.com' || hostname.endsWith('.hooks.slack.com')) return 'slack';
    if (hostname === 'discord.com' || hostname.endsWith('.discord.com')) return 'discord';
  } catch (_) {
    // malformed URL — fall through to default
  }
  return 'slack'; // default
}

/**
 * Load error report (JSON or markdown)
 */
function loadErrorReport(filePath) {
  if (!fs.existsSync(filePath)) {
    return null;
  }

  try {
    const content = fs.readFileSync(filePath, 'utf8');
    
    // Try to parse as JSON first
    if (filePath.endsWith('.json')) {
      return JSON.parse(content);
    }
    
    // Otherwise treat as markdown or text
    return { markdown: content, isMarkdown: true };
  } catch (e) {
    console.warn(`Failed to parse report: ${e.message}`);
    return null;
  }
}

/**
 * Extract chronic errors from report
 */
function extractChronicErrors(report) {
  if (!report) return [];
  
  if (report.isMarkdown) {
    // Parse markdown report for chronic errors section
    const lines = report.markdown.split('\n');
    const chronicSection = lines.findIndex(l => l.includes('Chronic'));
    if (chronicSection === -1) return [];
    
    const errors = [];
    for (let i = chronicSection + 2; i < lines.length; i++) {
      const line = lines[i];
      if (line.startsWith('##')) break; // next section
      const numbered = line.match(/^(\d+)\.\s*(.*)/);
      if (numbered) {
        // Normalise to the object shape expected by formatSlackMessage
        errors.push({ message: numbered[2], frequency: CHRONIC_THRESHOLD });
      }
    }
    return errors;
  }
  
  if (report.errors && Array.isArray(report.errors)) {
    return report.errors
      .filter(e => e.frequency >= CHRONIC_THRESHOLD)
      .sort((a, b) => (b.frequency || 0) - (a.frequency || 0));
  }
  
  return [];
}

/**
 * Format Slack message for chronic errors
 */
function formatSlackMessage(chronicErrors, stats) {
  const errorSummary = chronicErrors
    .slice(0, 5)
    .map((e, i) => {
      const message = e.message || e.error?.message || 'Unknown error';
      const freq = e.frequency || 1;
      return {
        type: 'section',
        text: {
          type: 'mrkdwn',
          text: `*${i + 1}. [${freq}x]* ${message.substring(0, 80)}`
        }
      };
    });

  const blocks = [
    {
      type: 'header',
      text: {
        type: 'plain_text',
        text: '🚨 Chronic Build Failures Detected',
        emoji: true
      }
    },
    {
      type: 'section',
      text: {
        type: 'mrkdwn',
        text: `Repository: *${GITHUB_REPO}*\nRun: <${GITHUB_RUN_URL}|#${process.env.GITHUB_RUN_ID}>`
      }
    },
    {
      type: 'divider'
    },
    {
      type: 'section',
      text: {
        type: 'mrkdwn',
        text: `*Error Summary*\n• Chronic Errors: ${chronicErrors.length}\n• Threshold: ≥${CHRONIC_THRESHOLD} runs`
      }
    },
    {
      type: 'divider'
    },
    ...errorSummary,
    {
      type: 'divider'
    }
  ];

  // Add remediation hints
  if (INCLUDE_REMEDIATION) {
    blocks.push({
      type: 'section',
      text: {
        type: 'mrkdwn',
        text: `*Remediation Steps*\n` +
              `1. Open <${GITHUB_RUN_URL}|this workflow run>\n` +
              `2. Download error artifacts (build-errors-*.json)\n` +
              `3. Review error details and affected files\n` +
              `4. Create PR with fix and reference this issue\n` +
              `5. Verify fix on next CI run`
      }
    });
  }

  // Add action button
  blocks.push({
    type: 'actions',
    elements: [
      {
        type: 'button',
        text: {
          type: 'plain_text',
          text: 'View CI Run'
        },
        url: GITHUB_RUN_URL,
        style: 'primary'
      },
      {
        type: 'button',
        text: {
          type: 'plain_text',
          text: 'View Issues'
        },
        url: `https://github.com/${GITHUB_REPO}/issues?q=label:ci/failure`
      }
    ]
  });

  return {
    text: '🚨 Chronic Build Failures Detected',
    blocks,
    attachments: [{
      color: '#ff0000',
      footer: 'ThemisDB CI System',
      ts: Math.floor(Date.now() / 1000)
    }]
  };
}

/**
 * Format Discord message for chronic errors
 */
function formatDiscordMessage(chronicErrors, stats) {
  const errorList = chronicErrors
    .slice(0, 8)
    .map((e, i) => {
      const message = e.message || e.error?.message || 'Unknown error';
      const freq = e.frequency || 1;
      return `${i + 1}. **[${freq}x]** ${message.substring(0, 100)}`;
    })
    .join('\n');

  const embeds = [{
    title: '🚨 Chronic Build Failures Detected',
    description: `Repository: **${GITHUB_REPO}**\nThreshold: ≥${CHRONIC_THRESHOLD} consecutive runs`,
    color: 16711680, // Red
    fields: [
      {
        name: 'Chronic Errors',
        value: `${chronicErrors.length} error(s) appearing repeatedly`,
        inline: true
      },
      {
        name: 'Top Issues',
        value: errorList || 'No chronic errors',
        inline: false
      }
    ],
    footer: {
      text: 'ThemisDB CI System',
      icon_url: 'https://github.com/images/icons/emoji/unicode/26a0.png'
    },
    timestamp: new Date().toISOString()
  }];

  // Add remediation
  if (INCLUDE_REMEDIATION) {
    embeds.push({
      title: '🛠️ Remediation Steps',
      description: '1. Open the workflow run\n2. Download error artifacts\n3. Review error details\n4. Create fix PR\n5. Verify on next run',
      color: 16776960 // Yellow
    });
  }

  const content = `🚨 **Chronic Build Failures in ${GITHUB_REPO}** 🚨\nRun: ${GITHUB_RUN_URL}`;

  return {
    content,
    embeds
  };
}

/**
 * Send webhook notification
 */
async function sendNotification(payload, url, platform) {
  return new Promise((resolve, reject) => {
    const isHttps = url.startsWith('https://');
    const client = isHttps ? https : http;
    
    const urlObj = new URL(url);
    const options = {
      hostname: urlObj.hostname,
      port: urlObj.port,
      path: urlObj.pathname + urlObj.search,
      method: 'POST',
      headers: {
        'Content-Type': 'application/json',
        'Content-Length': Buffer.byteLength(JSON.stringify(payload))
      }
    };

    const req = client.request(options, (res) => {
      let data = '';
      res.on('data', (chunk) => { data += chunk; });
      res.on('end', () => {
        if (res.statusCode >= 200 && res.statusCode < 300) {
          console.log(`✅ Notification sent via ${platform} (${res.statusCode})`);
          resolve({
            success: true,
            status: res.statusCode,
            url: url
          });
        } else {
          console.warn(`⚠️  Notification returned status ${res.statusCode}`);
          resolve({
            success: false,
            status: res.statusCode,
            response: data
          });
        }
      });
    });

    req.on('error', (e) => {
      console.error(`❌ Notification error: ${e.message}`);
      reject(e);
    });

    req.write(JSON.stringify(payload));
    req.end();
  });
}

/**
 * Main execution
 */
async function main() {
  try {
    // Validate inputs
    if (!WEBHOOK_URL) {
      console.log('⏭️  No webhook URL provided - skipping notifications');
      process.exitCode = 0;
      return;
    }

    if (!ERROR_REPORT_FILE || !fs.existsSync(ERROR_REPORT_FILE)) {
      console.warn(`⚠️  Error report file not found: ${ERROR_REPORT_FILE}`);
      process.exitCode = 0;
      return;
    }

    console.log('📨 Processing error report for chronic failures...');
    
    // Load report
    const report = loadErrorReport(ERROR_REPORT_FILE);
    if (!report) {
      console.log('⏭️  Could not parse error report');
      process.exitCode = 0;
      return;
    }

    // Extract chronic errors
    const chronicErrors = extractChronicErrors(report);
    console.log(`Found ${chronicErrors.length} chronic error(s)`);

    if (chronicErrors.length === 0) {
      console.log('✅ No chronic errors detected - no notification needed');
      
      // Set outputs
      if (process.env.GITHUB_OUTPUT) {
        fs.appendFileSync(process.env.GITHUB_OUTPUT, 
          `notification_sent=false\nchronic_count=0\n`);
      }
      process.exitCode = 0;
      return;
    }

    // Detect platform
    const platform = detectPlatform(WEBHOOK_URL);
    console.log(`📱 Platform: ${platform}`);

    // Format message
    const payload = platform === 'discord'
      ? formatDiscordMessage(chronicErrors, {})
      : formatSlackMessage(chronicErrors, {});

    // Send notification
    const result = await sendNotification(payload, WEBHOOK_URL, platform);

    if (result.success) {
      console.log(`\n✅ Notification sent successfully to ${platform}`);
      console.log(`   URL: ${WEBHOOK_URL.substring(0, 50)}...`);
      console.log(`   Chronic errors reported: ${chronicErrors.length}`);

      // Set outputs
      if (process.env.GITHUB_OUTPUT) {
        fs.appendFileSync(process.env.GITHUB_OUTPUT, 
          `notification_sent=true\nchronic_count=${chronicErrors.length}\nmessage_url=${result.url}\n`);
      }
    } else {
      console.warn(`⚠️  Notification may not have been delivered`);
      if (process.env.GITHUB_OUTPUT) {
        fs.appendFileSync(process.env.GITHUB_OUTPUT, 
          `notification_sent=false\nchronic_count=${chronicErrors.length}\n`);
      }
    }

  } catch (error) {
    console.error(`❌ Error sending notification: ${error.message}`);
    console.error(error.stack);
    process.exitCode = 1;
  }
}

main();
