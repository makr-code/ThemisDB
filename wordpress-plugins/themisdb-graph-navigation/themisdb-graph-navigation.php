<?php
/**
 * Plugin Name: ThemisDB Graph Navigation
 * Plugin URI: https://github.com/makr-code/wordpressPlugins
 * Description: Lagert die Graph-Navigation aus dem Theme in ein eigenstaendiges Plugin aus.
 * Version: 1.0.0
 * Author: ThemisDB Team
 * Author URI: https://github.com/makr-code/wordpressPlugins
 * License: MIT
 * Text Domain: themisdb-graph-navigation
 * Requires at least: 5.8
 * Requires PHP: 7.4
 */

if (!defined('ABSPATH')) {
    exit;
}

define('THEMISDB_GRAPH_NAV_VERSION', '1.0.0');
define('THEMISDB_GRAPH_NAV_PLUGIN_DIR', plugin_dir_path(__FILE__));
define('THEMISDB_GRAPH_NAV_PLUGIN_URL', plugin_dir_url(__FILE__));
define('THEMISDB_GRAPH_NAV_PLUGIN_FILE', __FILE__);

// Load updater class (prefer local copy for standalone ZIP distribution)
$themisdb_updater_local = THEMISDB_GRAPH_NAV_PLUGIN_DIR . 'includes/class-themisdb-plugin-updater.php';
$themisdb_updater_shared = dirname(THEMISDB_GRAPH_NAV_PLUGIN_DIR) . '/includes/class-themisdb-plugin-updater.php';

if (file_exists($themisdb_updater_local)) {
    require_once $themisdb_updater_local;
} elseif (file_exists($themisdb_updater_shared)) {
    require_once $themisdb_updater_shared;
}

// Initialize automatic updates
if (class_exists('ThemisDB_Plugin_Updater')) {
    new ThemisDB_Plugin_Updater(
        THEMISDB_GRAPH_NAV_PLUGIN_FILE,
        'themisdb-graph-navigation',
        THEMISDB_GRAPH_NAV_VERSION
    );
}

/**
 * Enqueue graph navigation assets.
 */
function themisdb_graph_navigation_enqueue_assets()
{
    if (is_admin()) {
        return;
    }

    wp_enqueue_script(
        'themisdb-graph-navigation',
        THEMISDB_GRAPH_NAV_PLUGIN_URL . 'assets/js/graph-navigation.js',
        array(),
        THEMISDB_GRAPH_NAV_VERSION,
        true
    );

    wp_localize_script('themisdb-graph-navigation', 'themisdbGraphData', themisdb_graph_navigation_get_graph_data());
}
add_action('wp_enqueue_scripts', 'themisdb_graph_navigation_enqueue_assets', 20);

/**
 * Build graph data for visualization.
 *
 * @return array{nodes: array, links: array}
 */
function themisdb_graph_navigation_get_graph_data()
{
    $nodes = array();
    $links = array();

    // Keep backward compatibility with previous filter names.
    $post_limit = apply_filters('themisdb_graph_post_limit', apply_filters('themisdb_graph_navigation_post_limit', 50));
    $page_limit = apply_filters('themisdb_graph_page_limit', apply_filters('themisdb_graph_navigation_page_limit', 30));

    $categories = get_categories(array(
        'hide_empty' => false,
    ));

    $tags = get_tags(array(
        'hide_empty' => false,
    ));

    $posts = get_posts(array(
        'numberposts' => $post_limit,
        'post_status' => 'publish',
        'post_type' => 'post',
    ));

    $pages = get_posts(array(
        'numberposts' => $page_limit,
        'post_status' => 'publish',
        'post_type' => 'page',
    ));

    $nodes[] = array(
        'id' => 'home',
        'label' => get_bloginfo('name'),
        'url' => home_url('/'),
        'type' => 'home',
        'level' => 0,
    );

    foreach ($categories as $category) {
        $nodes[] = array(
            'id' => 'cat_' . $category->term_id,
            'label' => $category->name,
            'url' => get_category_link($category->term_id),
            'type' => 'category',
            'level' => 1,
            'count' => $category->count,
        );

        $links[] = array(
            'source' => 'home',
            'target' => 'cat_' . $category->term_id,
            'type' => 'contains',
        );
    }

    foreach ($tags as $tag) {
        $nodes[] = array(
            'id' => 'tag_' . $tag->term_id,
            'label' => $tag->name,
            'url' => get_tag_link($tag->term_id),
            'type' => 'tag',
            'level' => 1,
            'count' => $tag->count,
        );

        $links[] = array(
            'source' => 'home',
            'target' => 'tag_' . $tag->term_id,
            'type' => 'tagged',
        );
    }

    foreach ($posts as $post) {
        $post_id = 'post_' . $post->ID;

        $nodes[] = array(
            'id' => $post_id,
            'label' => $post->post_title,
            'url' => get_permalink($post->ID),
            'type' => 'post',
            'level' => 2,
            'date' => $post->post_date,
            'excerpt' => wp_trim_words(get_the_excerpt($post->ID), 20),
        );

        $post_categories = get_the_category($post->ID);
        foreach ($post_categories as $cat) {
            $links[] = array(
                'source' => 'cat_' . $cat->term_id,
                'target' => $post_id,
                'type' => 'has_post',
            );
        }

        $post_tags = get_the_tags($post->ID);
        if ($post_tags) {
            foreach ($post_tags as $tag) {
                $links[] = array(
                    'source' => 'tag_' . $tag->term_id,
                    'target' => $post_id,
                    'type' => 'has_tag',
                );
            }
        }
    }

    foreach ($pages as $page) {
        $page_id = 'page_' . $page->ID;

        $nodes[] = array(
            'id' => $page_id,
            'label' => $page->post_title,
            'url' => get_permalink($page->ID),
            'type' => 'page',
            'level' => 1,
            'excerpt' => wp_trim_words(get_the_excerpt($page->ID), 20),
        );

        $links[] = array(
            'source' => 'home',
            'target' => $page_id,
            'type' => 'page_of',
        );
    }

    return array(
        'nodes' => $nodes,
        'links' => $links,
    );
}

/**
 * Backward-compatible function name from theme implementation.
 *
 * @return array{nodes: array, links: array}
 */
if (!function_exists('themisdb_get_graph_data')) {
    function themisdb_get_graph_data()
    {
        return themisdb_graph_navigation_get_graph_data();
    }
}
