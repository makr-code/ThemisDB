<?php
/**
 * Benchmark Visualizer Template
 * 
 * This template displays the benchmark visualizer interface
 */

if (!defined('ABSPATH')) {
    exit;
}
?>

<div class="themisdb-benchmark-wrapper">
    <div class="themisdb-section themisdb-benchmark-header">
        <h2><?php _e('ThemisDB Performance Benchmarks', 'themisdb-benchmark-visualizer'); ?></h2>
        <p class="themisdb-description">
            <?php _e('Interactive visualization of ThemisDB performance benchmarks compared to leading databases.', 'themisdb-benchmark-visualizer'); ?>
        </p>
    </div>

    <!-- Filters Section -->
    <div class="themisdb-section themisdb-filters">
        <div class="themisdb-filter-group">
            <label for="bv-category-filter">
                <strong><?php _e('Category:', 'themisdb-benchmark-visualizer'); ?></strong>
            </label>
            <select id="bv-category-filter" class="themisdb-select">
                <option value="all" <?php selected($atts['category'], 'all'); ?>><?php _e('All Operations', 'themisdb-benchmark-visualizer'); ?></option>
                <option value="vector_search" <?php selected($atts['category'], 'vector_search'); ?>><?php _e('Vector Search', 'themisdb-benchmark-visualizer'); ?></option>
                <option value="aql_query" <?php selected($atts['category'], 'aql_query'); ?>><?php _e('AQL Queries', 'themisdb-benchmark-visualizer'); ?></option>
                <option value="graph_traversal" <?php selected($atts['category'], 'graph_traversal'); ?>><?php _e('Graph Traversal', 'themisdb-benchmark-visualizer'); ?></option>
                <option value="document" <?php selected($atts['category'], 'document'); ?>><?php _e('Document Operations', 'themisdb-benchmark-visualizer'); ?></option>
                <option value="transaction" <?php selected($atts['category'], 'transaction'); ?>><?php _e('Transactions', 'themisdb-benchmark-visualizer'); ?></option>
            </select>
        </div>

        <div class="themisdb-filter-group">
            <label for="bv-metric-filter">
                <strong><?php _e('Metric:', 'themisdb-benchmark-visualizer'); ?></strong>
            </label>
            <select id="bv-metric-filter" class="themisdb-select">
                <option value="latency" <?php selected($atts['metric'], 'latency'); ?>><?php _e('Latency (ms)', 'themisdb-benchmark-visualizer'); ?></option>
                <option value="throughput" <?php selected($atts['metric'], 'throughput'); ?>><?php _e('Throughput (ops/sec)', 'themisdb-benchmark-visualizer'); ?></option>
                <option value="memory" <?php selected($atts['metric'], 'memory'); ?>><?php _e('Memory Usage (MB)', 'themisdb-benchmark-visualizer'); ?></option>
            </select>
        </div>

        <div class="themisdb-filter-group">
            <label for="bv-chart-type">
                <strong><?php _e('Chart Type:', 'themisdb-benchmark-visualizer'); ?></strong>
            </label>
            <select id="bv-chart-type" class="themisdb-select">
                <option value="bar" <?php selected($atts['chart_type'], 'bar'); ?>><?php _e('Bar Chart', 'themisdb-benchmark-visualizer'); ?></option>
                <option value="line" <?php selected($atts['chart_type'], 'line'); ?>><?php _e('Line Chart', 'themisdb-benchmark-visualizer'); ?></option>
                <option value="radar" <?php selected($atts['chart_type'], 'radar'); ?>><?php _e('Radar Chart', 'themisdb-benchmark-visualizer'); ?></option>
            </select>
        </div>

        <button id="bv-refresh-data" class="themisdb-btn-secondary">
            <span class="dashicons dashicons-update"></span>
            <?php _e('Refresh', 'themisdb-benchmark-visualizer'); ?>
        </button>
    </div>

    <!-- Chart Section -->
    <div class="themisdb-section themisdb-chart-section">
        <div class="themisdb-chart-container">
            <canvas id="themisdb-benchmark-chart"></canvas>
        </div>
        <div id="bv-loading" class="themisdb-loading" style="display: none;">
            <div class="themisdb-spinner"></div>
            <p><?php _e('Loading benchmark data...', 'themisdb-benchmark-visualizer'); ?></p>
        </div>
    </div>

    <!-- Results Section -->
    <div class="themisdb-section themisdb-results">
        <h3><?php _e('Benchmark Details', 'themisdb-benchmark-visualizer'); ?></h3>
        <div id="bv-results-table" class="themisdb-results-table">
            <!-- Results will be populated by JavaScript -->
        </div>
    </div>

    <!-- Insights Section -->
    <div class="themisdb-section themisdb-insights">
        <h3>
            <span class="dashicons dashicons-lightbulb"></span>
            <?php _e('Performance Insights', 'themisdb-benchmark-visualizer'); ?>
        </h3>
        <div id="bv-insights" class="themisdb-insights-content">
            <!-- Insights will be populated by JavaScript -->
        </div>
    </div>

    <!-- Export Section -->
    <div class="themisdb-section themisdb-export">
        <button id="bv-export-csv" class="themisdb-btn-secondary">
            <span class="dashicons dashicons-download"></span>
            <?php _e('Export CSV', 'themisdb-benchmark-visualizer'); ?>
        </button>
        <button id="bv-export-pdf" class="themisdb-btn-secondary">
            <span class="dashicons dashicons-pdf"></span>
            <?php _e('Export PDF', 'themisdb-benchmark-visualizer'); ?>
        </button>
        <button id="bv-print" class="themisdb-btn-secondary">
            <span class="dashicons dashicons-printer"></span>
            <?php _e('Print', 'themisdb-benchmark-visualizer'); ?>
        </button>
    </div>

    <!-- Footer -->
    <div class="themisdb-section themisdb-footer">
        <p class="themisdb-disclaimer">
            <small>
                <?php _e('Benchmark results may vary based on hardware, configuration, and workload. These benchmarks are provided for reference purposes.', 'themisdb-benchmark-visualizer'); ?>
            </small>
        </p>
        <p class="themisdb-branding">
            <small>
                <?php printf(
                    __('Powered by %s', 'themisdb-benchmark-visualizer'),
                    '<a href="https://github.com/makr-code/ThemisDB" target="_blank">ThemisDB</a>'
                ); ?>
            </small>
        </p>
    </div>
</div>
