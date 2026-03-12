<?php
/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            playground.php                                     ║
  Version:         0.0.2                                              ║
  Last Modified:   2026-03-09 04:08:20                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     194                                            ║
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
 * Query Playground Template
 */

if (!defined('ABSPATH')) {
    exit;
}
?>

<div class="themisdb-query-wrapper">
    <div class="themisdb-section themisdb-query-header">
        <h2><?php _e('ThemisDB Query Playground', 'themisdb-query-playground'); ?></h2>
        <p class="themisdb-description">
            <?php _e('Interactive AQL query editor for ThemisDB. Execute queries and explore results in real-time.', 'themisdb-query-playground'); ?>
        </p>
        <?php if (!$client_available): ?>
        <div class="themisdb-alert themisdb-alert-warning">
            <strong><?php _e('Note:', 'themisdb-query-playground'); ?></strong>
            <?php _e('ThemisDB client is not configured. Please configure the connection in Settings.', 'themisdb-query-playground'); ?>
        </div>
        <?php endif; ?>
    </div>

    <!-- Example Queries -->
    <div class="themisdb-section themisdb-examples">
        <h3>
            <span class="dashicons dashicons-editor-code"></span>
            <?php _e('Example Queries', 'themisdb-query-playground'); ?>
        </h3>
        <div class="themisdb-example-buttons">
            <button class="themisdb-btn-secondary qp-load-example" data-category="basic">
                <?php _e('Basic', 'themisdb-query-playground'); ?>
            </button>
            <button class="themisdb-btn-secondary qp-load-example" data-category="vector">
                <?php _e('Vector Search', 'themisdb-query-playground'); ?>
            </button>
            <button class="themisdb-btn-secondary qp-load-example" data-category="graph">
                <?php _e('Graph Traversal', 'themisdb-query-playground'); ?>
            </button>
            <button class="themisdb-btn-secondary qp-load-example" data-category="llm">
                <?php _e('LLM Integration', 'themisdb-query-playground'); ?>
            </button>
            <button class="themisdb-btn-secondary qp-load-example" data-category="analytics">
                <?php _e('Analytics', 'themisdb-query-playground'); ?>
            </button>
        </div>
        <div id="qp-example-list" class="themisdb-example-list" style="display: none;">
            <!-- Examples will be loaded here -->
        </div>
    </div>

    <!-- Query Editor -->
    <div class="themisdb-section themisdb-editor-section">
        <div class="themisdb-editor-header">
            <h3><?php _e('AQL Query Editor', 'themisdb-query-playground'); ?></h3>
            <div class="themisdb-editor-actions">
                <button id="qp-execute" class="themisdb-btn-primary" <?php echo !$client_available ? 'disabled' : ''; ?>>
                    <span class="dashicons dashicons-controls-play"></span>
                    <?php _e('Execute Query', 'themisdb-query-playground'); ?>
                </button>
                <button id="qp-clear" class="themisdb-btn-secondary">
                    <span class="dashicons dashicons-trash"></span>
                    <?php _e('Clear', 'themisdb-query-playground'); ?>
                </button>
                <button id="qp-format" class="themisdb-btn-secondary">
                    <span class="dashicons dashicons-editor-alignleft"></span>
                    <?php _e('Format', 'themisdb-query-playground'); ?>
                </button>
            </div>
        </div>
        <div class="themisdb-editor-container">
            <textarea id="qp-editor" style="height: <?php echo esc_attr($atts['height']); ?>;"><?php echo esc_textarea($atts['default_query']); ?></textarea>
        </div>
        <div class="themisdb-editor-info">
            <span id="qp-line-info">Line 1, Col 1</span>
            <span class="themisdb-separator">|</span>
            <span id="qp-char-count">0 characters</span>
        </div>
    </div>

    <!-- Query Status -->
    <div id="qp-status" class="themisdb-section themisdb-status" style="display: none;">
        <div id="qp-status-content"></div>
    </div>

    <!-- Results Section -->
    <div class="themisdb-section themisdb-results-section" id="qp-results-section" style="display: none;">
        <div class="themisdb-results-header">
            <h3><?php _e('Query Results', 'themisdb-query-playground'); ?></h3>
            <div class="themisdb-results-actions">
                <button id="qp-export-json" class="themisdb-btn-secondary">
                    <span class="dashicons dashicons-download"></span>
                    <?php _e('Export JSON', 'themisdb-query-playground'); ?>
                </button>
                <button id="qp-export-csv" class="themisdb-btn-secondary">
                    <span class="dashicons dashicons-media-spreadsheet"></span>
                    <?php _e('Export CSV', 'themisdb-query-playground'); ?>
                </button>
            </div>
        </div>
        <div class="themisdb-results-stats">
            <span id="qp-result-count">0 results</span>
            <span class="themisdb-separator">|</span>
            <span id="qp-execution-time">0 ms</span>
        </div>
        <div class="themisdb-results-view-toggle">
            <button class="qp-view-btn active" data-view="table">
                <span class="dashicons dashicons-list-view"></span>
                <?php _e('Table', 'themisdb-query-playground'); ?>
            </button>
            <button class="qp-view-btn" data-view="json">
                <span class="dashicons dashicons-editor-code"></span>
                <?php _e('JSON', 'themisdb-query-playground'); ?>
            </button>
            <button class="qp-view-btn" data-view="chart">
                <span class="dashicons dashicons-chart-bar"></span>
                <?php _e('Chart', 'themisdb-query-playground'); ?>
            </button>
        </div>
        <div id="qp-results-container" class="themisdb-results-container">
            <!-- Results will be displayed here -->
        </div>
    </div>

    <!-- Query Info -->
    <div class="themisdb-section themisdb-query-info">
        <h3>
            <span class="dashicons dashicons-info"></span>
            <?php _e('AQL Query Language Guide', 'themisdb-query-playground'); ?>
        </h3>
        <div class="themisdb-info-grid">
            <div class="themisdb-info-card">
                <h4><?php _e('Basic SELECT', 'themisdb-query-playground'); ?></h4>
                <code>SELECT * FROM urn:themis:relational:collection</code>
                <p><?php _e('Retrieve data from relational model', 'themisdb-query-playground'); ?></p>
            </div>
            <div class="themisdb-info-card">
                <h4><?php _e('Vector Search', 'themisdb-query-playground'); ?></h4>
                <code>WHERE VECTOR_SIMILARITY(embedding, [...])</code>
                <p><?php _e('Find similar vectors using HNSW index', 'themisdb-query-playground'); ?></p>
            </div>
            <div class="themisdb-info-card">
                <h4><?php _e('Graph Pattern', 'themisdb-query-playground'); ?></h4>
                <code>MATCH (a)-[:REL]->(b)</code>
                <p><?php _e('Traverse graph relationships', 'themisdb-query-playground'); ?></p>
            </div>
            <div class="themisdb-info-card">
                <h4><?php _e('LLM Functions', 'themisdb-query-playground'); ?></h4>
                <code>LLM_SIMILARITY(text, query)</code>
                <p><?php _e('Use integrated LLM for semantic search', 'themisdb-query-playground'); ?></p>
            </div>
        </div>
    </div>

    <!-- Footer -->
    <div class="themisdb-section themisdb-footer">
        <p class="themisdb-disclaimer">
            <small>
                <?php _e('Query execution is performed against the configured ThemisDB instance. Use responsibly.', 'themisdb-query-playground'); ?>
            </small>
        </p>
        <p class="themisdb-branding">
            <small>
                <?php printf(
                    __('Powered by %s', 'themisdb-query-playground'),
                    '<a href="https://github.com/makr-code/wordpressPlugins" target="_blank">ThemisDB</a>'
                ); ?>
            </small>
        </p>
    </div>
</div>
