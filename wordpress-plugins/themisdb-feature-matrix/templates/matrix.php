<?php
/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            matrix.php                                         ║
  Version:         0.0.2                                              ║
  Last Modified:   2026-03-09 04:08:18                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     168                                            ║
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
 * Feature Matrix Template
 * 
 * This template displays the feature comparison matrix with interactive features
 */

if (!defined('ABSPATH')) {
    exit;
}

// Get style class
$style_class = 'themisdb-feature-wrapper';
if (isset($atts['style'])) {
    $style_class .= ' style-' . esc_attr($atts['style']);
}

// Determine if features should be shown
$show_filters = isset($atts['filterable']) && $atts['filterable'] === 'yes';
$enable_csv = get_option('themisdb_fm_enable_csv_export', 'yes') === 'yes';
?>

<div class="<?php echo esc_attr($style_class); ?>" data-category="<?php echo esc_attr($atts['category']); ?>" data-style="<?php echo esc_attr($atts['style']); ?>">
    
    <!-- Header Section -->
    <div class="themisdb-section themisdb-feature-header">
        <h2><?php _e('ThemisDB Feature Comparison', 'themisdb-feature-matrix'); ?></h2>
        <p class="themisdb-description">
            <?php _e('Compare ThemisDB features and capabilities with leading databases. Discover what makes ThemisDB unique with AI/ML integration, multi-model support, and more.', 'themisdb-feature-matrix'); ?>
        </p>
    </div>

    <?php if ($show_filters): ?>
    <!-- Category Filter Buttons -->
    <div class="themisdb-section themisdb-category-filters" role="toolbar" aria-label="<?php esc_attr_e('Category Filters', 'themisdb-feature-matrix'); ?>">
        <button class="category-filter-btn <?php echo ($atts['category'] === 'all') ? 'active' : ''; ?>" data-category="all" aria-pressed="<?php echo ($atts['category'] === 'all') ? 'true' : 'false'; ?>">
            <?php _e('All Features', 'themisdb-feature-matrix'); ?>
        </button>
        <button class="category-filter-btn <?php echo ($atts['category'] === 'data_models') ? 'active' : ''; ?>" data-category="data_models" aria-pressed="<?php echo ($atts['category'] === 'data_models') ? 'true' : 'false'; ?>">
            <?php _e('Data Models', 'themisdb-feature-matrix'); ?>
        </button>
        <button class="category-filter-btn <?php echo ($atts['category'] === 'ai_ml') ? 'active' : ''; ?>" data-category="ai_ml" aria-pressed="<?php echo ($atts['category'] === 'ai_ml') ? 'true' : 'false'; ?>">
            <?php _e('AI/ML ⭐', 'themisdb-feature-matrix'); ?>
        </button>
        <button class="category-filter-btn <?php echo ($atts['category'] === 'performance') ? 'active' : ''; ?>" data-category="performance" aria-pressed="<?php echo ($atts['category'] === 'performance') ? 'true' : 'false'; ?>">
            <?php _e('Performance', 'themisdb-feature-matrix'); ?>
        </button>
        <button class="category-filter-btn <?php echo ($atts['category'] === 'compatibility') ? 'active' : ''; ?>" data-category="compatibility" aria-pressed="<?php echo ($atts['category'] === 'compatibility') ? 'true' : 'false'; ?>">
            <?php _e('Compatibility', 'themisdb-feature-matrix'); ?>
        </button>
        <button class="category-filter-btn <?php echo ($atts['category'] === 'licensing') ? 'active' : ''; ?>" data-category="licensing" aria-pressed="<?php echo ($atts['category'] === 'licensing') ? 'true' : 'false'; ?>">
            <?php _e('Licensing', 'themisdb-feature-matrix'); ?>
        </button>
    </div>
    <?php endif; ?>

    <!-- Dropdown Filters (fallback for mobile) -->
    <div class="themisdb-section themisdb-filters">
        <div class="themisdb-filter-group">
            <label for="fm-category-filter">
                <strong><?php _e('Category:', 'themisdb-feature-matrix'); ?></strong>
            </label>
            <select id="fm-category-filter" class="themisdb-select" aria-label="<?php esc_attr_e('Select Category', 'themisdb-feature-matrix'); ?>">
                <option value="all" <?php selected($atts['category'], 'all'); ?>><?php _e('All Features', 'themisdb-feature-matrix'); ?></option>
                <option value="data_models" <?php selected($atts['category'], 'data_models'); ?>><?php _e('Data Models', 'themisdb-feature-matrix'); ?></option>
                <option value="ai_ml" <?php selected($atts['category'], 'ai_ml'); ?>><?php _e('AI/ML ⭐', 'themisdb-feature-matrix'); ?></option>
                <option value="performance" <?php selected($atts['category'], 'performance'); ?>><?php _e('Performance', 'themisdb-feature-matrix'); ?></option>
                <option value="compatibility" <?php selected($atts['category'], 'compatibility'); ?>><?php _e('Compatibility', 'themisdb-feature-matrix'); ?></option>
                <option value="licensing" <?php selected($atts['category'], 'licensing'); ?>><?php _e('Licensing', 'themisdb-feature-matrix'); ?></option>
            </select>
        </div>

        <button id="fm-refresh-data" class="themisdb-btn-secondary" aria-label="<?php esc_attr_e('Refresh Data', 'themisdb-feature-matrix'); ?>">
            <span class="dashicons dashicons-update" aria-hidden="true"></span>
            <?php _e('Refresh', 'themisdb-feature-matrix'); ?>
        </button>
    </div>

    <!-- Feature Matrix Table -->
    <div class="themisdb-section themisdb-matrix-section">
        <div id="fm-loading" class="themisdb-loading" style="display: none;" role="status" aria-live="polite">
            <div class="themisdb-spinner" aria-hidden="true"></div>
            <p class="loading-text"><?php _e('Loading feature data...', 'themisdb-feature-matrix'); ?></p>
        </div>
        
        <div id="fm-matrix-table" class="themisdb-matrix-table" role="region" aria-label="<?php esc_attr_e('Feature Comparison Matrix', 'themisdb-feature-matrix'); ?>">
            <!-- Table will be populated by JavaScript -->
        </div>
    </div>

    <!-- Feature Legend -->
    <div class="themisdb-section themisdb-legend">
        <h3><?php _e('Feature Status Legend', 'themisdb-feature-matrix'); ?></h3>
        <div class="themisdb-legend-items">
            <div class="themisdb-legend-item">
                <span class="themisdb-status-badge status-full" aria-label="<?php esc_attr_e('Full Support', 'themisdb-feature-matrix'); ?>">
                    <span class="status-icon status-full">✓</span>
                </span>
                <span><?php _e('Full Support - Natively supported with complete functionality', 'themisdb-feature-matrix'); ?></span>
            </div>
            <div class="themisdb-legend-item">
                <span class="themisdb-status-badge status-limited" aria-label="<?php esc_attr_e('Limited Support', 'themisdb-feature-matrix'); ?>">
                    <span class="status-icon status-limited">◐</span>
                </span>
                <span><?php _e('Limited Support - Partial support or requires extensions', 'themisdb-feature-matrix'); ?></span>
            </div>
            <div class="themisdb-legend-item">
                <span class="themisdb-status-badge status-no" aria-label="<?php esc_attr_e('Not Available', 'themisdb-feature-matrix'); ?>">
                    <span class="status-icon status-no">✗</span>
                </span>
                <span><?php _e('Not Available - Feature not supported', 'themisdb-feature-matrix'); ?></span>
            </div>
        </div>
    </div>

    <?php if ($enable_csv): ?>
    <!-- Export Section -->
    <div class="themisdb-section themisdb-export">
        <button id="fm-export-csv" class="themisdb-btn-secondary fm-export-csv" aria-label="<?php esc_attr_e('Export as CSV', 'themisdb-feature-matrix'); ?>">
            <span class="dashicons dashicons-download" aria-hidden="true"></span>
            <?php _e('Export CSV', 'themisdb-feature-matrix'); ?>
        </button>
        <button id="fm-print" class="themisdb-btn-secondary" aria-label="<?php esc_attr_e('Print', 'themisdb-feature-matrix'); ?>">
            <span class="dashicons dashicons-printer" aria-hidden="true"></span>
            <?php _e('Print', 'themisdb-feature-matrix'); ?>
        </button>
    </div>
    <?php endif; ?>

    <!-- Footer -->
    <div class="themisdb-section themisdb-footer">
        <p class="themisdb-disclaimer">
            <small>
                <?php _e('Feature availability and support levels are based on the latest versions. Some features may require specific configurations. ThemisDB offers unique AI/ML capabilities not available in other databases.', 'themisdb-feature-matrix'); ?>
            </small>
        </p>
        <p class="themisdb-branding">
            <small>
                <?php printf(
                    __('Powered by %s - The Multi-Model Database with AI/ML Integration', 'themisdb-feature-matrix'),
                    '<a href="https://github.com/makr-code/wordpressPlugins" target="_blank" rel="noopener">ThemisDB</a>'
                ); ?>
            </small>
        </p>
    </div>
</div>
