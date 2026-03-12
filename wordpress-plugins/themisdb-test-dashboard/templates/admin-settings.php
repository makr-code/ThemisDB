<?php
/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            admin-settings.php                                 ║
  Version:         0.0.2                                              ║
  Last Modified:   2026-03-09 04:08:22                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     278                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 2a1fb0423  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
    • 9d3ecaa0e  2026-02-28  Add ThemisDB Wiki Integration plugin with documentation i... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */


/**
 * Admin Settings Page for ThemisDB Test Dashboard
 *
 * @package ThemisDB_Test_Dashboard
 */

// Exit if accessed directly
if (!defined('ABSPATH')) {
    exit;
}

$repo = get_option('themisdb_test_dashboard_repo', 'makr-code/wordpressPlugins');
$github_token = get_option('themisdb_test_dashboard_github_token', '');
$default_view = get_option('themisdb_test_dashboard_default_view', 'overview');
$default_period = get_option('themisdb_test_dashboard_default_period', 30);
?>

<div class="wrap">
    <h1>⚙️ Test Dashboard Settings</h1>
    <p>Configure ThemisDB Test Dashboard settings. All data is cached for 1 hour to improve performance.</p>
    
    <form method="post" action="">
        <?php wp_nonce_field('themisdb_test_dashboard_settings'); ?>
        
        <table class="form-table">
            <tr>
                <th scope="row">
                    <label for="repo">Repository</label>
                </th>
                <td>
                    <input type="text" 
                           id="repo" 
                           name="repo" 
                           value="<?php echo esc_attr($repo); ?>" 
                           class="regular-text" 
                           placeholder="owner/repository">
                    <p class="description">GitHub repository in format "owner/repo" (e.g., "makr-code/wordpressPlugins")</p>
                </td>
            </tr>
            
            <tr>
                <th scope="row">
                    <label for="github_token">GitHub Token (Optional)</label>
                </th>
                <td>
                    <input type="password" 
                           id="github_token" 
                           name="github_token" 
                           value="<?php echo esc_attr($github_token); ?>" 
                           class="regular-text" 
                           placeholder="ghp_xxxxxxxxxxxx">
                    <p class="description">
                        GitHub Personal Access Token for accessing private repositories and increased API rate limits.<br>
                        <strong>Permissions required:</strong> <code>repo</code> (for private repos) or <code>public_repo</code> (for public repos)<br>
                        <a href="https://github.com/settings/tokens/new" target="_blank">Create a token →</a>
                    </p>
                </td>
            </tr>
            
            <tr>
                <th scope="row">
                    <label for="default_view">Default View</label>
                </th>
                <td>
                    <select id="default_view" name="default_view">
                        <option value="overview" <?php selected($default_view, 'overview'); ?>>Overview</option>
                        <option value="coverage" <?php selected($default_view, 'coverage'); ?>>Test Coverage</option>
                        <option value="pipeline" <?php selected($default_view, 'pipeline'); ?>>CI/CD Pipeline</option>
                        <option value="quality" <?php selected($default_view, 'quality'); ?>>Quality Gates</option>
                    </select>
                    <p class="description">Default view when dashboard loads</p>
                </td>
            </tr>
            
            <tr>
                <th scope="row">
                    <label for="default_period">Default Time Period</label>
                </th>
                <td>
                    <select id="default_period" name="default_period">
                        <option value="7" <?php selected($default_period, 7); ?>>Last 7 days</option>
                        <option value="14" <?php selected($default_period, 14); ?>>Last 14 days</option>
                        <option value="30" <?php selected($default_period, 30); ?>>Last 30 days</option>
                        <option value="90" <?php selected($default_period, 90); ?>>Last 90 days</option>
                    </select>
                    <p class="description">Default time period for metrics</p>
                </td>
            </tr>
        </table>
        
        <p class="submit">
            <input type="submit" 
                   name="themisdb_test_dashboard_save" 
                   class="button button-primary" 
                   value="Save Settings">
        </p>
    </form>
    
    <hr>
    
    <h2>🔄 Cache Management</h2>
    <p>Dashboard data is cached for 1 hour to reduce API calls and improve performance.</p>
    
    <form method="post" action="">
        <?php wp_nonce_field('themisdb_test_dashboard_clear_cache'); ?>
        <p class="submit">
            <input type="submit" 
                   name="themisdb_test_dashboard_clear_cache" 
                   class="button button-secondary" 
                   value="Clear Cache" 
                   onclick="return confirm('Are you sure you want to clear all cached dashboard data?');">
        </p>
    </form>
    
    <hr>
    
    <h2>📖 Shortcode Usage</h2>
    <p>Use the following shortcode to display the Test Dashboard on any page or post:</p>
    
    <h3>Basic Usage</h3>
    <pre><code>[themisdb_test_dashboard]</code></pre>
    
    <h3>With Parameters</h3>
    <pre><code>[themisdb_test_dashboard view="coverage" period="30"]</code></pre>
    <pre><code>[themisdb_test_dashboard view="pipeline" repo="makr-code/wordpressPlugins"]</code></pre>
    <pre><code>[themisdb_test_dashboard height="800px"]</code></pre>
    
    <h3>Available Parameters</h3>
    <table class="widefat">
        <thead>
            <tr>
                <th>Parameter</th>
                <th>Description</th>
                <th>Options</th>
                <th>Default</th>
            </tr>
        </thead>
        <tbody>
            <tr>
                <td><code>view</code></td>
                <td>Initial dashboard view</td>
                <td>overview, coverage, pipeline, quality</td>
                <td>overview</td>
            </tr>
            <tr>
                <td><code>period</code></td>
                <td>Time period in days</td>
                <td>7, 14, 30, 90</td>
                <td>30</td>
            </tr>
            <tr>
                <td><code>repo</code></td>
                <td>GitHub repository</td>
                <td>owner/repo format</td>
                <td><?php echo esc_html($repo); ?></td>
            </tr>
            <tr>
                <td><code>height</code></td>
                <td>Minimum dashboard height</td>
                <td>Any CSS height value</td>
                <td>600px</td>
            </tr>
        </tbody>
    </table>
    
    <hr>
    
    <h2>ℹ️ Dashboard Views</h2>
    
    <h3>📊 Overview</h3>
    <p>Displays a comprehensive summary of test coverage, pipeline status, and quality gates with trend charts.</p>
    
    <h3>📈 Test Coverage</h3>
    <p>Detailed test coverage metrics including overall coverage, line coverage, branch coverage, and function coverage with historical trends.</p>
    
    <h3>🔄 CI/CD Pipeline</h3>
    <p>Monitor your GitHub Actions workflows with success rates, recent runs, and execution time trends.</p>
    
    <h3>🎯 Quality Gates</h3>
    <p>Track quality metrics such as code coverage thresholds, technical debt, code smells, duplicated code, security hotspots, and bugs.</p>
    
    <hr>
    
    <h2>🚀 Features</h2>
    <ul>
        <li>✅ <strong>Real-time Monitoring:</strong> Track CI/CD pipelines and test results</li>
        <li>✅ <strong>Test Coverage Trends:</strong> Visualize coverage over time with Chart.js</li>
        <li>✅ <strong>Quality Gates:</strong> Monitor code quality thresholds</li>
        <li>✅ <strong>GitHub Integration:</strong> Fetch data from GitHub Actions API</li>
        <li>✅ <strong>Performance Metrics:</strong> Track test execution times</li>
        <li>✅ <strong>Responsive Design:</strong> Works on mobile, tablet, and desktop</li>
        <li>✅ <strong>Export Functionality:</strong> Export data to CSV and PNG</li>
        <li>✅ <strong>Smart Caching:</strong> 1-hour cache for optimal performance</li>
    </ul>
    
    <hr>
    
    <h2>🔧 Troubleshooting</h2>
    
    <h3>Dashboard not loading?</h3>
    <ul>
        <li>Verify the repository name is correct (owner/repo format)</li>
        <li>Check if the repository is public or if you've provided a valid GitHub token</li>
        <li>Clear the cache and try again</li>
        <li>Check browser console for JavaScript errors</li>
    </ul>
    
    <h3>Data not updating?</h3>
    <ul>
        <li>Data is cached for 1 hour - clear the cache to force a refresh</li>
        <li>Check GitHub API rate limits (60 requests/hour without token, 5000 with token)</li>
    </ul>
    
    <h3>Charts not displaying?</h3>
    <ul>
        <li>Ensure Chart.js is loading correctly (check browser console)</li>
        <li>Verify there's sufficient data for the selected time period</li>
        <li>Try a different browser or clear browser cache</li>
    </ul>
    
    <hr>
    
    <p>
        <strong>Plugin Version:</strong> <?php echo THEMISDB_TEST_DASHBOARD_VERSION; ?><br>
        <strong>Documentation:</strong> <a href="https://github.com/makr-code/wordpressPlugins/tree/main/tools/test-dashboard-wordpress" target="_blank">View on GitHub →</a>
    </p>
</div>

<style>
.widefat code {
    background: #f5f5f5;
    padding: 2px 6px;
    border-radius: 3px;
    font-family: monospace;
}

pre code {
    display: block;
    background: #f5f5f5;
    padding: 12px;
    border-radius: 4px;
    border-left: 4px solid #0073aa;
    overflow-x: auto;
}

.wrap h2 {
    margin-top: 2rem;
}

.wrap hr {
    margin: 2rem 0;
    border: none;
    border-top: 1px solid #ddd;
}
</style>
