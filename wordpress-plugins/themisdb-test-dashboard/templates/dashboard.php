<?php
/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            dashboard.php                                      ║
  Version:         0.0.2                                              ║
  Last Modified:   2026-03-09 04:08:22                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     88                                             ║
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
 * Template for ThemisDB Test Dashboard
 *
 * @package ThemisDB_Test_Dashboard
 */

// Exit if accessed directly
if (!defined('ABSPATH')) {
    exit;
}

$view = isset($atts['view']) ? esc_attr($atts['view']) : 'overview';
$period = isset($atts['period']) ? intval($atts['period']) : 30;
$repo = isset($atts['repo']) ? esc_attr($atts['repo']) : get_option('themisdb_test_dashboard_repo', 'makr-code/wordpressPlugins');
$height = isset($atts['height']) ? esc_attr($atts['height']) : '600px';
?>

<div class="themisdb-test-dashboard" style="min-height: <?php echo $height; ?>;">
    <!-- Header -->
    <div class="tdb-header">
        <h2>📊 ThemisDB Test Dashboard</h2>
        <p>Comprehensive testing and quality metrics for ThemisDB</p>
    </div>
    
    <!-- Controls -->
    <div class="tdb-controls">
        <div class="tdb-control-group">
            <label for="tdb-view-select">View</label>
            <select id="tdb-view-select">
                <option value="overview" <?php selected($view, 'overview'); ?>>Overview</option>
                <option value="coverage" <?php selected($view, 'coverage'); ?>>Test Coverage</option>
                <option value="pipeline" <?php selected($view, 'pipeline'); ?>>CI/CD Pipeline</option>
                <option value="quality" <?php selected($view, 'quality'); ?>>Quality Gates</option>
            </select>
        </div>
        
        <div class="tdb-control-group">
            <label for="tdb-period-select">Time Period</label>
            <select id="tdb-period-select">
                <option value="7" <?php selected($period, 7); ?>>Last 7 days</option>
                <option value="14" <?php selected($period, 14); ?>>Last 14 days</option>
                <option value="30" <?php selected($period, 30); ?>>Last 30 days</option>
                <option value="90" <?php selected($period, 90); ?>>Last 90 days</option>
            </select>
        </div>
        
        <div class="tdb-control-group">
            <label for="tdb-repo-input">Repository</label>
            <input type="text" id="tdb-repo-input" value="<?php echo esc_attr($repo); ?>" placeholder="owner/repo">
        </div>
        
        <div class="tdb-control-group">
            <label>&nbsp;</label>
            <button id="tdb-refresh-btn" class="tdb-button">🔄 Refresh</button>
        </div>
    </div>
    
    <!-- Content Area -->
    <div id="tdb-content">
        <div class="tdb-loading">
            <div class="tdb-spinner"></div>
            <p>Loading dashboard data...</p>
        </div>
    </div>
</div>
