<?php
/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            diagram.php                                        ║
  Version:         0.0.2                                              ║
  Last Modified:   2026-03-09 04:08:16                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     253                                            ║
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
 * Architecture Diagram Template
 */

if (!defined('ABSPATH')) {
    exit;
}

// Get diagram description based on view
$descriptions = array(
    'high_level' => __('System architecture diagram showing the client layer connecting to the API layer, which interfaces with the query engine and LLM engine. The query engine connects to the storage layer.', 'themisdb-architecture-diagrams'),
    'storage_layer' => __('Storage layer architecture showing RocksDB-based multi-model storage with indexes for graph, vector, relational and document data.', 'themisdb-architecture-diagrams'),
    'llm_integration' => __('LLM integration architecture showing llama.cpp integration, model management, and vector embeddings.', 'themisdb-architecture-diagrams'),
    'sharding_raid' => __('Distributed system architecture showing data sharding, RAID configuration, and replication across nodes.', 'themisdb-architecture-diagrams'),
    'hardware_architecture' => __('Hardware architecture mapping software components to CPU, GPU, RAM, storage, and network resources.', 'themisdb-architecture-diagrams'),
    'database_comparison' => __('Comparison of ThemisDB with other popular databases showing unique advantages.', 'themisdb-architecture-diagrams'),
    'llm_comparison' => __('Comparison of embedded LLM approach versus cloud-based LLM services.', 'themisdb-architecture-diagrams'),
    'performance_comparison' => __('Performance scales dramatically with hardware investment showing ROI at each tier.', 'themisdb-architecture-diagrams'),
    'tco_comparison' => __('Total Cost of Ownership analysis over 1, 3, and 5 years comparing different deployment options.', 'themisdb-architecture-diagrams'),
    'feature_matrix' => __('Comprehensive feature comparison across all major database systems.', 'themisdb-architecture-diagrams'),
    'deployment_options' => __('Flexible deployment models including on-premise, cloud, hybrid and SaaS options.', 'themisdb-architecture-diagrams'),
    'use_case_recommendations' => __('Recommended database choices for different application scenarios.', 'themisdb-architecture-diagrams'),
    'migration_paths' => __('Clear migration paths from legacy databases to ThemisDB.', 'themisdb-architecture-diagrams'),
);

$view_title = ucwords(str_replace('_', ' ', $atts['view']));
$diagram_id = 'diagram-' . uniqid();
?>

<div class="themisdb-architecture-wrapper">
    <figure class="themisdb-section themisdb-architecture-header" 
            role="img" 
            aria-labelledby="<?php echo esc_attr($diagram_id); ?>-header-title">
        <h2 id="<?php echo esc_attr($diagram_id); ?>-header-title">
            <?php _e('ThemisDB Architecture', 'themisdb-architecture-diagrams'); ?>
        </h2>
        <p class="themisdb-description">
            <?php _e('Interactive visualization of ThemisDB system architecture. Explore different layers and components.', 'themisdb-architecture-diagrams'); ?>
        </p>
    </figure>

    <?php if ($atts['show_controls'] === 'true' || $atts['show_controls'] === true): ?>
    <!-- View Controls -->
    <div class="themisdb-section themisdb-controls" role="toolbar" aria-label="<?php _e('Diagram Controls', 'themisdb-architecture-diagrams'); ?>">
        <div class="themisdb-view-selector">
            <label for="ad-view-select">
                <strong><?php _e('Architecture View:', 'themisdb-architecture-diagrams'); ?></strong>
            </label>
            <select id="ad-view-select" class="themisdb-select" aria-label="<?php _e('Select Architecture View', 'themisdb-architecture-diagrams'); ?>">
                <optgroup label="<?php _e('ThemisDB Architecture', 'themisdb-architecture-diagrams'); ?>">
                    <option value="high_level" <?php selected($atts['view'], 'high_level'); ?>>
                        <?php _e('High-Level Architecture', 'themisdb-architecture-diagrams'); ?>
                    </option>
                    <option value="storage_layer" <?php selected($atts['view'], 'storage_layer'); ?>>
                        <?php _e('Storage Layer', 'themisdb-architecture-diagrams'); ?>
                    </option>
                    <option value="llm_integration" <?php selected($atts['view'], 'llm_integration'); ?>>
                        <?php _e('LLM Integration', 'themisdb-architecture-diagrams'); ?>
                    </option>
                    <option value="sharding_raid" <?php selected($atts['view'], 'sharding_raid'); ?>>
                        <?php _e('Sharding & RAID', 'themisdb-architecture-diagrams'); ?>
                    </option>
                    <option value="hardware_architecture" <?php selected($atts['view'], 'hardware_architecture'); ?>>
                        <?php _e('Hardware Architecture', 'themisdb-architecture-diagrams'); ?>
                    </option>
                </optgroup>
                <optgroup label="<?php _e('Comparisons', 'themisdb-architecture-diagrams'); ?>">
                    <option value="database_comparison" <?php selected($atts['view'], 'database_comparison'); ?>>
                        <?php _e('Database Comparison', 'themisdb-architecture-diagrams'); ?>
                    </option>
                    <option value="llm_comparison" <?php selected($atts['view'], 'llm_comparison'); ?>>
                        <?php _e('LLM Services Comparison', 'themisdb-architecture-diagrams'); ?>
                    </option>
                    <option value="performance_comparison" <?php selected($atts['view'], 'performance_comparison'); ?>>
                        <?php _e('Performance by Hardware', 'themisdb-architecture-diagrams'); ?>
                    </option>
                    <option value="tco_comparison" <?php selected($atts['view'], 'tco_comparison'); ?>>
                        <?php _e('TCO Over Time (1-5 Years)', 'themisdb-architecture-diagrams'); ?>
                    </option>
                    <option value="feature_matrix" <?php selected($atts['view'], 'feature_matrix'); ?>>
                        <?php _e('Feature Matrix', 'themisdb-architecture-diagrams'); ?>
                    </option>
                    <option value="deployment_options" <?php selected($atts['view'], 'deployment_options'); ?>>
                        <?php _e('Deployment Options', 'themisdb-architecture-diagrams'); ?>
                    </option>
                    <option value="use_case_recommendations" <?php selected($atts['view'], 'use_case_recommendations'); ?>>
                        <?php _e('Use Case Recommendations', 'themisdb-architecture-diagrams'); ?>
                    </option>
                    <option value="migration_paths" <?php selected($atts['view'], 'migration_paths'); ?>>
                        <?php _e('Migration Paths', 'themisdb-architecture-diagrams'); ?>
                    </option>
                </optgroup>
            </select>
        </div>

        <div class="themisdb-diagram-actions">
            <button type="button" id="ad-zoom-in" class="themisdb-btn-secondary" 
                    aria-label="<?php _e('Zoom In', 'themisdb-architecture-diagrams'); ?>"
                    title="<?php _e('Zoom In', 'themisdb-architecture-diagrams'); ?>">
                <span class="dashicons dashicons-plus" aria-hidden="true"></span>
            </button>
            <button type="button" id="ad-zoom-out" class="themisdb-btn-secondary" 
                    aria-label="<?php _e('Zoom Out', 'themisdb-architecture-diagrams'); ?>"
                    title="<?php _e('Zoom Out', 'themisdb-architecture-diagrams'); ?>">
                <span class="dashicons dashicons-minus" aria-hidden="true"></span>
            </button>
            <button type="button" id="ad-zoom-reset" class="themisdb-btn-secondary" 
                    aria-label="<?php _e('Reset Zoom', 'themisdb-architecture-diagrams'); ?>"
                    title="<?php _e('Reset Zoom', 'themisdb-architecture-diagrams'); ?>">
                <span class="dashicons dashicons-image-rotate" aria-hidden="true"></span>
            </button>
            <button type="button" id="ad-fullscreen" class="themisdb-btn-secondary" 
                    aria-label="<?php _e('Toggle Fullscreen', 'themisdb-architecture-diagrams'); ?>"
                    title="<?php _e('Fullscreen', 'themisdb-architecture-diagrams'); ?>">
                <span class="dashicons dashicons-fullscreen-alt" aria-hidden="true"></span>
            </button>
        </div>
    </div>
    <?php endif; ?>

    <!-- Diagram Section -->
    <figure class="themisdb-section themisdb-diagram-section"
            role="img"
            aria-labelledby="<?php echo esc_attr($diagram_id); ?>-title"
            aria-describedby="<?php echo esc_attr($diagram_id); ?>-desc">
        
        <!-- Screen reader title -->
        <figcaption id="<?php echo esc_attr($diagram_id); ?>-title" class="sr-only">
            <?php echo esc_html(sprintf(__('ThemisDB %s Architecture Diagram', 'themisdb-architecture-diagrams'), $view_title)); ?>
        </figcaption>
        
        <!-- Screen reader description -->
        <div id="<?php echo esc_attr($diagram_id); ?>-desc" class="sr-only">
            <?php echo isset($descriptions[$atts['view']]) ? esc_html($descriptions[$atts['view']]) : ''; ?>
        </div>
        <div id="ad-loading" class="themisdb-loading" style="display: none;" role="status" aria-live="polite">
            <div class="themisdb-spinner"></div>
            <p><?php _e('Loading architecture diagram...', 'themisdb-architecture-diagrams'); ?></p>
        </div>
        
        <div class="themisdb-diagram-container" id="ad-diagram-container">
            <div class="mermaid" id="ad-mermaid-diagram" role="presentation" tabindex="0">
                <!-- Diagram will be rendered here -->
            </div>
        </div>
    </figure>

    <!-- Component Details Panel -->
    <div class="themisdb-section themisdb-details-panel" id="ad-details-panel" style="display: none;">
        <h3>
            <span class="dashicons dashicons-info"></span>
            <span id="ad-details-title"><?php _e('Component Details', 'themisdb-architecture-diagrams'); ?></span>
        </h3>
        <div id="ad-details-content">
            <!-- Component details will be inserted here -->
        </div>
    </div>

    <!-- Architecture Description -->
    <div class="themisdb-section themisdb-description-panel">
        <h3><?php _e('Architecture Overview', 'themisdb-architecture-diagrams'); ?></h3>
        <div id="ad-description-content" class="themisdb-description-text">
            <p><?php _e('ThemisDB features a modern, multi-layered architecture designed for performance, scalability, and flexibility.', 'themisdb-architecture-diagrams'); ?></p>
        </div>
    </div>

    <!-- Legend -->
    <div class="themisdb-section themisdb-legend">
        <h3><?php _e('Legend', 'themisdb-architecture-diagrams'); ?></h3>
        <div class="themisdb-legend-items">
            <div class="themisdb-legend-item">
                <span class="themisdb-legend-box" style="background: #2ea44f;"></span>
                <span><?php _e('Storage Components', 'themisdb-architecture-diagrams'); ?></span>
            </div>
            <div class="themisdb-legend-item">
                <span class="themisdb-legend-box" style="background: #3498db;"></span>
                <span><?php _e('AI/LLM Components', 'themisdb-architecture-diagrams'); ?></span>
            </div>
            <div class="themisdb-legend-item">
                <span class="themisdb-legend-box" style="background: #f39c12;"></span>
                <span><?php _e('Persistence Layer', 'themisdb-architecture-diagrams'); ?></span>
            </div>
            <div class="themisdb-legend-item">
                <span class="themisdb-legend-box" style="background: #e74c3c;"></span>
                <span><?php _e('Consensus/Coordination', 'themisdb-architecture-diagrams'); ?></span>
            </div>
        </div>
    </div>

    <!-- Export Section -->
    <div class="themisdb-section themisdb-export" role="group" aria-label="<?php _e('Export Options', 'themisdb-architecture-diagrams'); ?>">
        <button type="button" id="ad-export-svg" class="themisdb-btn-secondary"
                aria-label="<?php _e('Export as SVG', 'themisdb-architecture-diagrams'); ?>">
            <span class="dashicons dashicons-download" aria-hidden="true"></span>
            <?php _e('Export SVG', 'themisdb-architecture-diagrams'); ?>
        </button>
        <button type="button" id="ad-export-png" class="themisdb-btn-secondary"
                aria-label="<?php _e('Export as PNG', 'themisdb-architecture-diagrams'); ?>">
            <span class="dashicons dashicons-format-image" aria-hidden="true"></span>
            <?php _e('Export PNG', 'themisdb-architecture-diagrams'); ?>
        </button>
        <button type="button" id="ad-export-code" class="themisdb-btn-secondary"
                aria-label="<?php _e('Export Mermaid Code', 'themisdb-architecture-diagrams'); ?>">
            <span class="dashicons dashicons-media-code" aria-hidden="true"></span>
            <?php _e('Export Code', 'themisdb-architecture-diagrams'); ?>
        </button>
        <button type="button" id="ad-print" class="themisdb-btn-secondary"
                aria-label="<?php _e('Print Diagram', 'themisdb-architecture-diagrams'); ?>">
            <span class="dashicons dashicons-printer" aria-hidden="true"></span>
            <?php _e('Print', 'themisdb-architecture-diagrams'); ?>
        </button>
    </div>

    <!-- Footer -->
    <div class="themisdb-section themisdb-footer">
        <p class="themisdb-disclaimer">
            <small>
                <?php _e('Architecture diagrams are simplified representations. Actual implementation may vary based on configuration.', 'themisdb-architecture-diagrams'); ?>
            </small>
        </p>
        <p class="themisdb-branding">
            <small>
                <?php printf(
                    __('Powered by %s', 'themisdb-architecture-diagrams'),
                    '<a href="https://github.com/makr-code/wordpressPlugins" target="_blank">ThemisDB</a>'
                ); ?>
            </small>
        </p>
    </div>
</div>
