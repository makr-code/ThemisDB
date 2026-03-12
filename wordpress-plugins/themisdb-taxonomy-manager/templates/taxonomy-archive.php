<?php
/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            taxonomy-archive.php                               ║
  Version:         0.0.2                                              ║
  Last Modified:   2026-03-09 04:08:21                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     112                                            ║
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
 * Template: Taxonomy Archive Header
 * 
 * This template can be included in theme's taxonomy templates
 * or used via the themisdb_taxonomy_archive_header action
 */

if (!defined('ABSPATH')) {
    exit;
}

$term = get_queried_object();
if (!$term || !is_a($term, 'WP_Term')) {
    return;
}

$icon = get_term_meta($term->term_id, 'icon', true);
$color = get_term_meta($term->term_id, 'color', true);
if (empty($color)) {
    $color = '#3498db';
}

// Get parent term if exists
$parent_term = null;
if ($term->parent > 0) {
    $parent_term = get_term($term->parent, $term->taxonomy);
}

$taxonomy_obj = get_taxonomy($term->taxonomy);
?>

<div class="themisdb-taxonomy-archive">
    <?php
    // Breadcrumbs
    do_action('themisdb_before_taxonomy_archive');
    ?>
    
    <header class="taxonomy-header" style="background: linear-gradient(135deg, <?php echo esc_attr($color); ?>, #3498db);">
        <?php if (!empty($icon)): ?>
            <span class="taxonomy-icon"><?php echo esc_html($icon); ?></span>
        <?php endif; ?>
        
        <h1><?php echo esc_html($term->name); ?></h1>
        
        <?php if (!empty($term->description)): ?>
            <p class="taxonomy-description">
                <?php echo esc_html($term->description); ?>
            </p>
        <?php endif; ?>
        
        <div class="taxonomy-meta">
            <span class="meta-item">
                <strong><?php echo $term->count; ?></strong> 
                <?php echo $term->count === 1 ? __('post', 'themisdb-taxonomy') : __('posts', 'themisdb-taxonomy'); ?>
            </span>
            
            <?php if ($parent_term): ?>
                <span class="meta-item">
                    <?php _e('Category:', 'themisdb-taxonomy'); ?> 
                    <a href="<?php echo esc_url(get_term_link($parent_term)); ?>" style="color: rgba(255,255,255,0.9);">
                        <?php echo esc_html($parent_term->name); ?>
                    </a>
                </span>
            <?php elseif ($taxonomy_obj): ?>
                <span class="meta-item">
                    <?php echo esc_html($taxonomy_obj->labels->singular_name); ?>
                </span>
            <?php endif; ?>
        </div>
    </header>
    
    <div class="taxonomy-posts">
        <?php
        // The main loop will be handled by the theme
        // This hook allows themes to customize the post display
        do_action('themisdb_taxonomy_posts_before');
        ?>
    </div>
</div>

<?php
// Add widget styles if needed
wp_enqueue_style(
    'themisdb-taxonomy-widget',
    THEMISDB_TAXONOMY_PLUGIN_URL . 'assets/css/taxonomy-widget.css',
    array(),
    THEMISDB_TAXONOMY_VERSION
);
?>
