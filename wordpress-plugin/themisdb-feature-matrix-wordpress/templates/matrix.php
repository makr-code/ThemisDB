<?php
/**
 * Feature Matrix Template
 * 
 * This template displays the feature comparison matrix
 */

if (!defined('ABSPATH')) {
    exit;
}
?>

<div class="themisdb-feature-wrapper">
    <div class="themisdb-section themisdb-feature-header">
        <h2><?php _e('ThemisDB Feature Comparison', 'themisdb-feature-matrix'); ?></h2>
        <p class="themisdb-description">
            <?php _e('Compare ThemisDB features and capabilities with leading databases. Discover what makes ThemisDB unique.', 'themisdb-feature-matrix'); ?>
        </p>
    </div>

    <!-- Category Filters -->
    <div class="themisdb-section themisdb-filters">
        <div class="themisdb-filter-group">
            <label for="fm-category-filter">
                <strong><?php _e('Category:', 'themisdb-feature-matrix'); ?></strong>
            </label>
            <select id="fm-category-filter" class="themisdb-select">
                <option value="all" <?php selected($atts['category'], 'all'); ?>><?php _e('All Features', 'themisdb-feature-matrix'); ?></option>
                <option value="architecture" <?php selected($atts['category'], 'architecture'); ?>><?php _e('Architecture', 'themisdb-feature-matrix'); ?></option>
                <option value="ai_ml" <?php selected($atts['category'], 'ai_ml'); ?>><?php _e('AI/ML', 'themisdb-feature-matrix'); ?></option>
                <option value="scalability" <?php selected($atts['category'], 'scalability'); ?>><?php _e('Scalability', 'themisdb-feature-matrix'); ?></option>
                <option value="security" <?php selected($atts['category'], 'security'); ?>><?php _e('Security', 'themisdb-feature-matrix'); ?></option>
                <option value="reliability" <?php selected($atts['category'], 'reliability'); ?>><?php _e('Reliability', 'themisdb-feature-matrix'); ?></option>
                <option value="usability" <?php selected($atts['category'], 'usability'); ?>><?php _e('Usability', 'themisdb-feature-matrix'); ?></option>
            </select>
        </div>

        <div class="themisdb-filter-group">
            <label for="fm-view-type">
                <strong><?php _e('View:', 'themisdb-feature-matrix'); ?></strong>
            </label>
            <select id="fm-view-type" class="themisdb-select">
                <option value="detailed" <?php selected($atts['view'], 'detailed'); ?>><?php _e('Detailed', 'themisdb-feature-matrix'); ?></option>
                <option value="compact" <?php selected($atts['view'], 'compact'); ?>><?php _e('Compact', 'themisdb-feature-matrix'); ?></option>
            </select>
        </div>

        <button id="fm-refresh-data" class="themisdb-btn-secondary">
            <span class="dashicons dashicons-update"></span>
            <?php _e('Refresh', 'themisdb-feature-matrix'); ?>
        </button>
    </div>

    <!-- Feature Matrix Table -->
    <div class="themisdb-section themisdb-matrix-section">
        <div id="fm-loading" class="themisdb-loading" style="display: none;">
            <div class="themisdb-spinner"></div>
            <p><?php _e('Loading feature data...', 'themisdb-feature-matrix'); ?></p>
        </div>
        
        <div id="fm-matrix-table" class="themisdb-matrix-table">
            <!-- Table will be populated by JavaScript -->
        </div>
    </div>

    <!-- Mermaid Diagram Section -->
    <?php if ($atts['show_diagram'] === 'true' || $atts['show_diagram'] === true): ?>
    <div class="themisdb-section themisdb-diagram-section">
        <h3>
            <span class="dashicons dashicons-networking"></span>
            <?php _e('Feature Hierarchy', 'themisdb-feature-matrix'); ?>
        </h3>
        <div class="themisdb-mermaid-container">
            <div class="mermaid" id="fm-feature-diagram">
                <!-- Mermaid diagram will be populated by JavaScript -->
            </div>
        </div>
    </div>
    <?php endif; ?>

    <!-- Feature Legend -->
    <div class="themisdb-section themisdb-legend">
        <h3><?php _e('Feature Status Legend', 'themisdb-feature-matrix'); ?></h3>
        <div class="themisdb-legend-items">
            <div class="themisdb-legend-item">
                <span class="themisdb-status-badge status-available">✅</span>
                <span><?php _e('Available - Fully supported natively', 'themisdb-feature-matrix'); ?></span>
            </div>
            <div class="themisdb-legend-item">
                <span class="themisdb-status-badge status-partial">⚠️</span>
                <span><?php _e('Partial - Available with limitations or via extensions', 'themisdb-feature-matrix'); ?></span>
            </div>
            <div class="themisdb-legend-item">
                <span class="themisdb-status-badge status-limited">🔧</span>
                <span><?php _e('Limited - Basic support or external integration required', 'themisdb-feature-matrix'); ?></span>
            </div>
            <div class="themisdb-legend-item">
                <span class="themisdb-status-badge status-not-available">❌</span>
                <span><?php _e('Not Available - Feature not supported', 'themisdb-feature-matrix'); ?></span>
            </div>
        </div>
    </div>

    <!-- Export Section -->
    <div class="themisdb-section themisdb-export">
        <button id="fm-export-csv" class="themisdb-btn-secondary">
            <span class="dashicons dashicons-download"></span>
            <?php _e('Export CSV', 'themisdb-feature-matrix'); ?>
        </button>
        <button id="fm-export-pdf" class="themisdb-btn-secondary">
            <span class="dashicons dashicons-pdf"></span>
            <?php _e('Export PDF', 'themisdb-feature-matrix'); ?>
        </button>
        <button id="fm-print" class="themisdb-btn-secondary">
            <span class="dashicons dashicons-printer"></span>
            <?php _e('Print', 'themisdb-feature-matrix'); ?>
        </button>
    </div>

    <!-- Footer -->
    <div class="themisdb-section themisdb-footer">
        <p class="themisdb-disclaimer">
            <small>
                <?php _e('Feature availability and support levels may vary based on version and configuration. Contact us for detailed information.', 'themisdb-feature-matrix'); ?>
            </small>
        </p>
        <p class="themisdb-branding">
            <small>
                <?php printf(
                    __('Powered by %s', 'themisdb-feature-matrix'),
                    '<a href="https://github.com/makr-code/ThemisDB" target="_blank">ThemisDB</a>'
                ); ?>
            </small>
        </p>
    </div>
</div>
