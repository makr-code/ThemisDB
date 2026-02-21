/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            dashboard.php                                      ║
  Version:         0.0.15                                             ║
  Last Modified:   2026-02-21 17:08:00                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   96.0/100                                       ║
    • Total Lines:     92                                             ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • e178371a5  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 234245ceb  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • b8b369411  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 8efb1d2fe  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 31ccce9fb  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

<?php
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
$repo = isset($atts['repo']) ? esc_attr($atts['repo']) : get_option('themisdb_test_dashboard_repo', 'makr-code/ThemisDB');
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
