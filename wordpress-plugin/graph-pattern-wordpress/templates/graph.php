<?php
/**
 * Graph Visualization Template
 */

if (!defined('ABSPATH')) {
    exit;
}
?>

<div class="themisdb-graph-wrapper">
    <!-- Header -->
    <div class="themisdb-section themisdb-graph-header">
        <h2><?php _e('ThemisDB Graph Pattern Visualizer', 'themisdb-graph-pattern'); ?></h2>
        <p class="themisdb-description">
            <?php _e('Interactive graph visualization with filtering, searching, and color-coded node groups. Inspired by Neo4j Bloom.', 'themisdb-graph-pattern'); ?>
        </p>
    </div>

    <?php if ($atts['show_controls'] === 'true' || $atts['show_controls'] === true): ?>
    <!-- Controls Bar -->
    <div class="themisdb-section themisdb-graph-controls">
        <div class="themisdb-control-group">
            <label for="gp-layout-select">
                <strong><?php _e('Layout:', 'themisdb-graph-pattern'); ?></strong>
            </label>
            <select id="gp-layout-select" class="themisdb-select">
                <option value="force_directed" <?php selected($atts['layout'], 'force_directed'); ?>>
                    <?php _e('Force Directed', 'themisdb-graph-pattern'); ?>
                </option>
                <option value="hierarchical_top" <?php selected($atts['layout'], 'hierarchical_top'); ?>>
                    <?php _e('Hierarchical (Top-Down)', 'themisdb-graph-pattern'); ?>
                </option>
                <option value="hierarchical_left" <?php selected($atts['layout'], 'hierarchical_left'); ?>>
                    <?php _e('Hierarchical (Left-Right)', 'themisdb-graph-pattern'); ?>
                </option>
                <option value="circular" <?php selected($atts['layout'], 'circular'); ?>>
                    <?php _e('Circular', 'themisdb-graph-pattern'); ?>
                </option>
            </select>
        </div>

        <div class="themisdb-control-group">
            <button id="gp-zoom-in" class="themisdb-btn themisdb-btn-icon" title="<?php _e('Zoom In', 'themisdb-graph-pattern'); ?>">
                <span class="dashicons dashicons-plus-alt"></span>
            </button>
            <button id="gp-zoom-out" class="themisdb-btn themisdb-btn-icon" title="<?php _e('Zoom Out', 'themisdb-graph-pattern'); ?>">
                <span class="dashicons dashicons-minus"></span>
            </button>
            <button id="gp-zoom-fit" class="themisdb-btn themisdb-btn-icon" title="<?php _e('Fit to Screen', 'themisdb-graph-pattern'); ?>">
                <span class="dashicons dashicons-editor-expand"></span>
            </button>
            <button id="gp-fullscreen" class="themisdb-btn themisdb-btn-icon" title="<?php _e('Fullscreen', 'themisdb-graph-pattern'); ?>">
                <span class="dashicons dashicons-fullscreen-alt"></span>
            </button>
        </div>

        <div class="themisdb-control-group">
            <span id="gp-node-count" style="color: var(--themisdb-text-secondary); font-size: 14px;">
                <?php _e('Loading...', 'themisdb-graph-pattern'); ?>
            </span>
        </div>
    </div>
    <?php endif; ?>

    <!-- Graph Visualization Section -->
    <div class="themisdb-section" style="position: relative; padding: 0; overflow: hidden;">
        <!-- Loading State -->
        <div id="gp-loading" class="themisdb-loading">
            <div class="themisdb-spinner"></div>
            <p><?php _e('Loading graph data...', 'themisdb-graph-pattern'); ?></p>
        </div>

        <!-- Graph Container -->
        <div class="themisdb-graph-container" id="gp-graph-container">
            <div id="gp-graph-canvas" 
                 style="height: <?php echo esc_attr($atts['height']); ?>;"
                 data-source="<?php echo esc_attr($atts['data_source']); ?>">
                <!-- Graph will be rendered here -->
            </div>

            <!-- Overlay Toggle Button (shown when panel is hidden) -->
            <button id="gp-toggle-overlay" class="themisdb-overlay-toggle">
                <span class="dashicons dashicons-admin-generic"></span>
            </button>

            <?php if ($atts['show_overlay'] === 'true' || $atts['show_overlay'] === true): ?>
            <!-- Options Overlay Panel -->
            <div class="themisdb-overlay-panel">
                <div class="themisdb-overlay-header">
                    <h3><?php _e('Options', 'themisdb-graph-pattern'); ?></h3>
                    <button class="themisdb-overlay-close">
                        <span class="dashicons dashicons-no-alt"></span>
                    </button>
                </div>

                <div class="themisdb-overlay-body">
                    <!-- Search Section -->
                    <div class="themisdb-search-box">
                        <input type="text" 
                               id="gp-search-input" 
                               placeholder="<?php _e('Search nodes...', 'themisdb-graph-pattern'); ?>"
                               autocomplete="off">
                    </div>

                    <!-- Node Groups Filter -->
                    <div class="themisdb-filter-section">
                        <h4><?php _e('Node Groups', 'themisdb-graph-pattern'); ?></h4>
                        <div id="gp-filter-list">
                            <!-- Populated dynamically -->
                        </div>
                    </div>

                    <!-- Layout Controls -->
                    <div class="themisdb-filter-section">
                        <h4><?php _e('Layout Settings', 'themisdb-graph-pattern'); ?></h4>
                        
                        <!-- Physics Toggle -->
                        <div class="themisdb-toggle">
                            <label><?php _e('Physics Simulation', 'themisdb-graph-pattern'); ?></label>
                            <div class="themisdb-toggle-switch">
                                <input type="checkbox" id="gp-toggle-physics" checked>
                                <span class="themisdb-toggle-slider"></span>
                            </div>
                        </div>

                        <!-- Labels Toggle -->
                        <div class="themisdb-toggle">
                            <label><?php _e('Show Labels', 'themisdb-graph-pattern'); ?></label>
                            <div class="themisdb-toggle-switch">
                                <input type="checkbox" id="gp-toggle-labels" checked>
                                <span class="themisdb-toggle-slider"></span>
                            </div>
                        </div>

                        <!-- Node Spacing Slider -->
                        <div class="themisdb-slider-control">
                            <label>
                                <?php _e('Node Spacing', 'themisdb-graph-pattern'); ?>
                                <span class="themisdb-slider-value" id="gp-node-spacing-value">120</span>
                            </label>
                            <input type="range" 
                                   id="gp-node-spacing" 
                                   min="50" 
                                   max="300" 
                                   value="120" 
                                   step="10">
                        </div>

                        <!-- Edge Strength Slider -->
                        <div class="themisdb-slider-control">
                            <label>
                                <?php _e('Edge Strength', 'themisdb-graph-pattern'); ?>
                                <span class="themisdb-slider-value" id="gp-edge-strength-value">4</span>
                            </label>
                            <input type="range" 
                                   id="gp-edge-strength" 
                                   min="1" 
                                   max="10" 
                                   value="4" 
                                   step="0.5">
                        </div>
                    </div>

                    <!-- Export Section -->
                    <div class="themisdb-filter-section">
                        <h4><?php _e('Export', 'themisdb-graph-pattern'); ?></h4>
                        <button id="gp-export-png" class="themisdb-btn" style="width: 100%; margin-bottom: 8px;">
                            <span class="dashicons dashicons-format-image"></span>
                            <?php _e('Export as PNG', 'themisdb-graph-pattern'); ?>
                        </button>
                        <button id="gp-export-json" class="themisdb-btn" style="width: 100%;">
                            <span class="dashicons dashicons-media-code"></span>
                            <?php _e('Export as JSON', 'themisdb-graph-pattern'); ?>
                        </button>
                    </div>

                    <!-- Node Details (populated on click) -->
                    <div id="gp-node-details" style="display: none;">
                        <!-- Node details will be inserted here -->
                    </div>
                </div>
            </div>
            <?php endif; ?>
        </div>
    </div>

    <!-- Legend -->
    <div class="themisdb-section">
        <h3><?php _e('Legend', 'themisdb-graph-pattern'); ?></h3>
        <div class="themisdb-legend">
            <div class="themisdb-legend-item">
                <span class="themisdb-legend-box" style="background: #3498db;"></span>
                <span><?php _e('Client Components', 'themisdb-graph-pattern'); ?></span>
            </div>
            <div class="themisdb-legend-item">
                <span class="themisdb-legend-box" style="background: #2ea44f;"></span>
                <span><?php _e('Query Layer', 'themisdb-graph-pattern'); ?></span>
            </div>
            <div class="themisdb-legend-item">
                <span class="themisdb-legend-box" style="background: #e67e22;"></span>
                <span><?php _e('LLM Components', 'themisdb-graph-pattern'); ?></span>
            </div>
            <div class="themisdb-legend-item">
                <span class="themisdb-legend-box" style="background: #27ae60;"></span>
                <span><?php _e('Index Layer', 'themisdb-graph-pattern'); ?></span>
            </div>
            <div class="themisdb-legend-item">
                <span class="themisdb-legend-box" style="background: #34495e;"></span>
                <span><?php _e('Storage Layer', 'themisdb-graph-pattern'); ?></span>
            </div>
        </div>
    </div>

    <!-- Instructions -->
    <div class="themisdb-section">
        <h3><?php _e('How to Use', 'themisdb-graph-pattern'); ?></h3>
        <ul style="line-height: 1.8; color: var(--themisdb-text-secondary);">
            <li><strong><?php _e('Click', 'themisdb-graph-pattern'); ?>:</strong> <?php _e('Select a node to view details', 'themisdb-graph-pattern'); ?></li>
            <li><strong><?php _e('Double-click', 'themisdb-graph-pattern'); ?>:</strong> <?php _e('Expand node connections (future feature)', 'themisdb-graph-pattern'); ?></li>
            <li><strong><?php _e('Drag', 'themisdb-graph-pattern'); ?>:</strong> <?php _e('Move nodes or pan the canvas', 'themisdb-graph-pattern'); ?></li>
            <li><strong><?php _e('Scroll', 'themisdb-graph-pattern'); ?>:</strong> <?php _e('Zoom in/out', 'themisdb-graph-pattern'); ?></li>
            <li><strong><?php _e('Search', 'themisdb-graph-pattern'); ?>:</strong> <?php _e('Find nodes by name in the overlay panel', 'themisdb-graph-pattern'); ?></li>
            <li><strong><?php _e('Filter', 'themisdb-graph-pattern'); ?>:</strong> <?php _e('Show/hide node groups using checkboxes', 'themisdb-graph-pattern'); ?></li>
            <li><strong><?php _e('Customize', 'themisdb-graph-pattern'); ?>:</strong> <?php _e('Change node colors with the color picker', 'themisdb-graph-pattern'); ?></li>
            <li><strong><?php _e('Layout', 'themisdb-graph-pattern'); ?>:</strong> <?php _e('Adjust spacing and physics with sliders', 'themisdb-graph-pattern'); ?></li>
        </ul>
    </div>

    <!-- Footer -->
    <div class="themisdb-section themisdb-footer">
        <p class="themisdb-disclaimer">
            <small>
                <?php _e('This is a visualization of the ThemisDB architecture. Node positions and relationships are for illustrative purposes.', 'themisdb-graph-pattern'); ?>
            </small>
        </p>
        <p class="themisdb-branding">
            <small>
                <?php printf(
                    __('Powered by %s | Graph visualization by %s', 'themisdb-graph-pattern'),
                    '<a href="https://github.com/makr-code/ThemisDB" target="_blank">ThemisDB</a>',
                    '<a href="https://visjs.org/" target="_blank">vis-network.js</a>'
                ); ?>
            </small>
        </p>
    </div>
</div>
