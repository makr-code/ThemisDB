<?php
/**
 * Feature Matrix Template
 */

if (!defined('ABSPATH')) {
    exit;
}

// Get features
$features = ThemisDB_Feature_Matrix_Core::get_filtered_features($atts['category']);

// Localize data for JavaScript
$json_features = json_encode($features);
?>

<script>
window.themisdbFeatureData = <?php echo $json_features; ?>;
</script>

<div class="themisdb-feature-wrapper" role="region" aria-label="Database Feature Comparison">
    
    <!-- Filter Bar -->
    <?php if ($atts['filterable']): ?>
    <div class="matrix-filter-bar" role="toolbar" aria-label="Filter options">
        <div class="category-filters" role="group" aria-label="Category filters">
            <button class="category-btn active" data-category="all" role="button" tabindex="0">
                <?php _e('All Features', 'themisdb-feature-matrix'); ?>
            </button>
            <button class="category-btn" data-category="data_models" role="button" tabindex="0">
                <?php _e('Data Models', 'themisdb-feature-matrix'); ?>
            </button>
            <button class="category-btn" data-category="ai_ml" role="button" tabindex="0">
                <?php _e('AI/ML', 'themisdb-feature-matrix'); ?>
            </button>
            <button class="category-btn" data-category="performance" role="button" tabindex="0">
                <?php _e('Performance', 'themisdb-feature-matrix'); ?>
            </button>
            <button class="category-btn" data-category="compatibility" role="button" tabindex="0">
                <?php _e('Compatibility', 'themisdb-feature-matrix'); ?>
            </button>
            <button class="category-btn" data-category="pricing" role="button" tabindex="0">
                <?php _e('Licensing', 'themisdb-feature-matrix'); ?>
            </button>
        </div>
        
        <?php if (get_option('themisdb_matrix_enable_export', 1)): ?>
        <button class="export-btn" role="button" tabindex="0" aria-label="Export to CSV">
            <?php _e('Export CSV', 'themisdb-feature-matrix'); ?>
        </button>
        <?php endif; ?>
    </div>
    <?php endif; ?>
    
    <!-- Main Table -->
    <table class="matrix-table <?php echo $atts['sticky_header'] ? 'sticky-header' : ''; ?>" 
           role="table" 
           aria-label="Database Feature Comparison">
        <caption class="sr-only">
            <?php _e('Comparison of ThemisDB, PostgreSQL, MongoDB, and Neo4j features', 'themisdb-feature-matrix'); ?>
        </caption>
        <thead>
            <tr>
                <th scope="col"><?php _e('Feature', 'themisdb-feature-matrix'); ?></th>
                <th scope="col" class="themisdb-col sortable" data-column="themisdb" tabindex="0" role="columnheader">
                    <?php _e('ThemisDB', 'themisdb-feature-matrix'); ?>
                </th>
                <th scope="col" class="sortable" data-column="postgresql" tabindex="0" role="columnheader">
                    <?php _e('PostgreSQL', 'themisdb-feature-matrix'); ?>
                </th>
                <th scope="col" class="sortable" data-column="mongodb" tabindex="0" role="columnheader">
                    <?php _e('MongoDB', 'themisdb-feature-matrix'); ?>
                </th>
                <th scope="col" class="sortable" data-column="neo4j" tabindex="0" role="columnheader">
                    <?php _e('Neo4j', 'themisdb-feature-matrix'); ?>
                </th>
            </tr>
        </thead>
        <tbody>
            <?php
            foreach ($features as $category_key => $category) {
                // Category header
                echo '<tr class="category-header">';
                echo '<td colspan="5" role="rowheader">' . esc_html($category['name']) . '</td>';
                echo '</tr>';
                
                // Feature rows
                foreach ($category['features'] as $feature_key => $feature) {
                    $highlight_class = isset($feature['highlight']) && $feature['highlight'] ? 'highlight' : '';
                    echo '<tr class="' . $highlight_class . '">';
                    
                    // Feature name
                    echo '<td><div class="feature-name">' . esc_html($feature['name']);
                    if (isset($feature['tooltip'])) {
                        echo ' <span class="info-icon tooltip" role="tooltip">ℹ️';
                        echo '<span class="tooltiptext">' . esc_html($feature['tooltip']) . '</span>';
                        echo '</span>';
                    }
                    echo '</div></td>';
                    
                    // Database columns
                    $databases = array('themisdb', 'postgresql', 'mongodb', 'neo4j');
                    foreach ($databases as $db) {
                        $status = isset($feature[$db]) ? $feature[$db] : 'no';
                        
                        echo '<td class="text-center">';
                        if (isset($feature['display_text']) && $feature['display_text']) {
                            echo '<span class="status-text">' . esc_html($status) . '</span>';
                        } else {
                            $status_info = ThemisDB_Feature_Matrix_Core::get_status_info($status);
                            echo '<span class="status-badge status-' . esc_attr($status) . '" role="img" aria-label="' . esc_attr($status_info['label']) . '">';
                            echo esc_html($status_info['icon']);
                            echo '</span>';
                        }
                        echo '</td>';
                    }
                    
                    echo '</tr>';
                }
            }
            ?>
        </tbody>
    </table>
    
    <!-- Mobile Card View -->
    <div class="matrix-cards" aria-label="Mobile feature comparison view">
        <!-- Populated by JavaScript -->
    </div>
    
    <!-- Legend -->
    <?php if ($atts['show_legend']): ?>
    <div class="matrix-legend" role="region" aria-label="Status legend">
        <h3><?php _e('Feature Status Legend', 'themisdb-feature-matrix'); ?></h3>
        <div class="legend-items">
            <div class="legend-item">
                <span class="status-badge status-full" role="img" aria-label="Full Support">✓</span>
                <span><?php _e('Full Support - Fully supported natively', 'themisdb-feature-matrix'); ?></span>
            </div>
            <div class="legend-item">
                <span class="status-badge status-limited" role="img" aria-label="Limited Support">◐</span>
                <span><?php _e('Limited Support - Available with limitations', 'themisdb-feature-matrix'); ?></span>
            </div>
            <div class="legend-item">
                <span class="status-badge status-no" role="img" aria-label="No Support">✗</span>
                <span><?php _e('No Support - Feature not supported', 'themisdb-feature-matrix'); ?></span>
            </div>
        </div>
    </div>
    <?php endif; ?>
    
</div>
