<?php
/**
 * ThemisDB Theme v2 – Functions
 *
 * WordPress Block Theme (Full Site Editing) – complete theme setup,
 * pattern registration, block styles, editor support and plugin
 * compatibility layer for all ThemisDB plugins.
 *
 * @package ThemisDB_V2
 * @since   2.0.0
 */

if ( ! defined( 'ABSPATH' ) ) {
	exit;
}

define( 'THEMISDB_V2_VERSION', '2.0.0' );
define( 'THEMISDB_V2_DIR',     get_template_directory() );
define( 'THEMISDB_V2_URI',     get_template_directory_uri() );

/* ================================================================
   1. THEME SETUP
   ================================================================ */

add_action( 'after_setup_theme', 'themisdb_v2_setup' );

function themisdb_v2_setup() {
	load_theme_textdomain( 'themisdb-v2', THEMISDB_V2_DIR . '/languages' );

	add_theme_support( 'automatic-feed-links' );
	add_theme_support( 'title-tag' );
	add_theme_support( 'post-thumbnails' );
	add_theme_support( 'responsive-embeds' );
	add_theme_support( 'align-wide' );
	add_theme_support( 'html5', array(
		'search-form', 'comment-form', 'comment-list',
		'gallery', 'caption', 'style', 'script',
	) );

	// Block theme: Full Site Editing
	add_theme_support( 'block-templates' );

	// Custom logo
	add_theme_support( 'custom-logo', array(
		'height'      => 48,
		'width'       => 180,
		'flex-height' => true,
		'flex-width'  => true,
		'header-text' => array( 'site-title' ),
	) );

	// Editor styles (applied inside Gutenberg)
	add_theme_support( 'editor-styles' );
	add_editor_style( array(
		'style.css',
		'assets/css/editor.css',
	) );

	// Custom image sizes used by ThemisDB plugins / content
	add_image_size( 'themisdb-v2-hero',      1920, 960,  true );
	add_image_size( 'themisdb-v2-featured',  1200, 675,  true );
	add_image_size( 'themisdb-v2-card',      640,  400,  true );
	add_image_size( 'themisdb-v2-thumbnail', 400,  300,  true );
	add_image_size( 'themisdb-v2-avatar',    80,   80,   true );

	// Navigation menus (used by classic nav fallback + widget areas)
	register_nav_menus( array(
		'primary'      => __( 'Primary Menu',      'themisdb-v2' ),
		'docs'         => __( 'Documentation Menu', 'themisdb-v2' ),
		'footer-col-1' => __( 'Footer: Products',  'themisdb-v2' ),
		'footer-col-2' => __( 'Footer: Resources', 'themisdb-v2' ),
		'footer-col-3' => __( 'Footer: Company',   'themisdb-v2' ),
		'footer-col-4' => __( 'Footer: Legal',     'themisdb-v2' ),
	) );
}

/* ================================================================
   2. ENQUEUE ASSETS
   ================================================================ */

add_action( 'wp_enqueue_scripts', 'themisdb_v2_enqueue' );

function themisdb_v2_enqueue() {
	// Main theme stylesheet (style.css = block theme required file)
	wp_enqueue_style(
		'themisdb-v2-style',
		get_stylesheet_uri(),
		array(),
		THEMISDB_V2_VERSION
	);

	// Navigation / mobile menu JS (vanilla JS, no jQuery needed)
	wp_enqueue_script(
		'themisdb-v2-navigation',
		THEMISDB_V2_URI . '/assets/js/navigation.js',
		array(),
		THEMISDB_V2_VERSION,
		true
	);

	// Pass AJAX URL and nonce for any front-end AJAX
	wp_localize_script( 'themisdb-v2-navigation', 'themisdbV2', array(
		'ajaxUrl' => admin_url( 'admin-ajax.php' ),
		'nonce'   => wp_create_nonce( 'themisdb_v2_nonce' ),
		'homeUrl' => esc_url( home_url( '/' ) ),
	) );
}

/* ================================================================
   3. BLOCK PATTERNS
   ================================================================ */

add_action( 'init', 'themisdb_v2_register_pattern_categories' );

function themisdb_v2_register_pattern_categories() {
	register_block_pattern_category( 'themisdb', array(
		'label' => __( 'ThemisDB', 'themisdb-v2' ),
	) );
	register_block_pattern_category( 'themisdb-landing', array(
		'label' => __( 'ThemisDB – Landing Page', 'themisdb-v2' ),
	) );
	register_block_pattern_category( 'themisdb-docs', array(
		'label' => __( 'ThemisDB – Documentation', 'themisdb-v2' ),
	) );
}

// Patterns are auto-discovered from /patterns/ directory (WP 6.0+).
// The PHP header comment in each file handles registration metadata.

/* ================================================================
   4. BLOCK STYLES (custom variants for core blocks)
   ================================================================ */

add_action( 'init', 'themisdb_v2_register_block_styles' );

function themisdb_v2_register_block_styles() {

	// Buttons
	register_block_style( 'core/button', array(
		'name'  => 'themis-primary',
		'label' => __( 'Themis Primary', 'themisdb-v2' ),
	) );
	register_block_style( 'core/button', array(
		'name'  => 'themis-accent',
		'label' => __( 'Themis Accent (Purple)', 'themisdb-v2' ),
	) );
	register_block_style( 'core/button', array(
		'name'  => 'themis-dark',
		'label' => __( 'Themis Dark', 'themisdb-v2' ),
	) );
	register_block_style( 'core/button', array(
		'name'  => 'themis-ghost',
		'label' => __( 'Themis Ghost (White Outline)', 'themisdb-v2' ),
	) );

	// Group / section
	register_block_style( 'core/group', array(
		'name'  => 'themis-card',
		'label' => __( 'Themis Card', 'themisdb-v2' ),
	) );
	register_block_style( 'core/group', array(
		'name'  => 'themis-callout',
		'label' => __( 'Themis Callout Box', 'themisdb-v2' ),
	) );

	// Code
	register_block_style( 'core/code', array(
		'name'  => 'themis-terminal',
		'label' => __( 'Terminal / Shell', 'themisdb-v2' ),
	) );

	// Image
	register_block_style( 'core/image', array(
		'name'  => 'themis-rounded',
		'label' => __( 'Rounded', 'themisdb-v2' ),
	) );
	register_block_style( 'core/image', array(
		'name'  => 'themis-shadow',
		'label' => __( 'Shadow', 'themisdb-v2' ),
	) );

	// Separator
	register_block_style( 'core/separator', array(
		'name'  => 'themis-accent',
		'label' => __( 'Themis Accent Line', 'themisdb-v2' ),
	) );

	// Quote
	register_block_style( 'core/quote', array(
		'name'  => 'themis-featured',
		'label' => __( 'Featured Quote', 'themisdb-v2' ),
	) );
}

/* ================================================================
   5. PLUGIN COMPATIBILITY
   ================================================================ */

/**
 * Shortcode wrapper for all ThemisDB plugin shortcodes.
 * Adds a wrapper div so the theme CSS can target them.
 */
add_filter( 'themisdb_shortcode_wrapper_class', 'themisdb_v2_shortcode_wrapper' );
function themisdb_v2_shortcode_wrapper( $class ) {
	return $class . ' themisdb-v2-plugin-output';
}

/**
 * Yoast SEO: remove default breadcrumb wrapper if plugin is active.
 * We use our own parts/breadcrumbs.html template part.
 */
add_action( 'wp_head', 'themisdb_v2_yoast_breadcrumb_support' );
function themisdb_v2_yoast_breadcrumb_support() {
	if ( function_exists( 'yoast_breadcrumb' ) ) {
		add_theme_support( 'yoast-seo-breadcrumbs' );
	}
}

/**
 * Easy Table of Contents: apply theme-specific container class.
 */
add_filter( 'ez_toc_container_class', function( $class ) {
	return 'themis-toc ' . $class;
} );

/**
 * Relevanssi: ensure we get styled search results.
 */
add_filter( 'relevanssi_search_ok', '__return_true' );

/**
 * Filter Kadence Blocks color palette to match Themis tokens.
 */
add_filter( 'kadence_blocks_color_palette', 'themisdb_v2_kadence_palette' );
function themisdb_v2_kadence_palette( $palette ) {
	return array(
		array( 'color' => '#2c3e50', 'name' => 'Themis Primary'   ),
		array( 'color' => '#1a252f', 'name' => 'Themis Dark'       ),
		array( 'color' => '#3498db', 'name' => 'Themis Secondary'  ),
		array( 'color' => '#7c4dff', 'name' => 'Themis Accent'     ),
		array( 'color' => '#27ae60', 'name' => 'Themis Success'    ),
		array( 'color' => '#f39c12', 'name' => 'Themis Warning'    ),
		array( 'color' => '#e74c3c', 'name' => 'Themis Error'      ),
		array( 'color' => '#f8f9fa', 'name' => 'Themis Light BG'   ),
		array( 'color' => '#ecf0f1', 'name' => 'Themis Gray 100'   ),
		array( 'color' => '#ffffff', 'name' => 'White'             ),
	);
}

/* ================================================================
   6. WIDGET AREAS (backwards compat for classic widgets in blocks)
   ================================================================ */

add_action( 'widgets_init', 'themisdb_v2_widgets' );

function themisdb_v2_widgets() {
	$shared = array(
		'before_widget' => '<div id="%1$s" class="widget themisdb-v2-widget %2$s">',
		'after_widget'  => '</div>',
		'before_title'  => '<h4 class="widget-title">',
		'after_title'   => '</h4>',
	);

	register_sidebar( array_merge( $shared, array(
		'name'        => __( 'Blog Sidebar', 'themisdb-v2' ),
		'id'          => 'sidebar-blog',
		'description' => __( 'Sidebar for blog / single posts', 'themisdb-v2' ),
	) ) );

	register_sidebar( array_merge( $shared, array(
		'name'        => __( 'Docs Sidebar', 'themisdb-v2' ),
		'id'          => 'sidebar-docs',
		'description' => __( 'Sidebar for documentation pages', 'themisdb-v2' ),
	) ) );

	for ( $i = 1; $i <= 4; $i++ ) {
		register_sidebar( array_merge( $shared, array(
			'name'        => sprintf( __( 'Footer Column %d', 'themisdb-v2' ), $i ),
			'id'          => 'footer-' . $i,
			'description' => sprintf( __( 'Footer widget area column %d', 'themisdb-v2' ), $i ),
		) ) );
	}
}

/* ================================================================
   7. BODY CLASSES
   ================================================================ */

add_filter( 'body_class', 'themisdb_v2_body_classes' );

function themisdb_v2_body_classes( $classes ) {
	if ( is_singular() ) {
		$classes[] = 'is-singular';
	}
	if ( is_front_page() ) {
		$classes[] = 'is-front-page';
	}
	if ( is_page_template( 'page-docs' ) ) {
		$classes[] = 'is-docs-page';
	}
	return $classes;
}

/* ================================================================
   8. SECURITY / HARDENING
   ================================================================ */

// Remove WP version from HTML head
remove_action( 'wp_head', 'wp_generator' );

// Remove unnecessary REST API links from head (keep API itself)
remove_action( 'wp_head', 'rest_output_link_wp_head' );
remove_action( 'wp_head', 'wp_oembed_add_discovery_links' );

// Disable XML-RPC (not needed for this theme)
add_filter( 'xmlrpc_enabled', '__return_false' );

/* ================================================================
   9. PERFORMANCE
   ================================================================ */

// Defer non-critical JS
add_filter( 'script_loader_tag', 'themisdb_v2_defer_scripts', 10, 3 );
function themisdb_v2_defer_scripts( $tag, $handle, $src ) {
	$defer = array( 'themisdb-v2-navigation' );
	if ( in_array( $handle, $defer, true ) ) {
		return str_replace( ' src', ' defer src', $tag );
	}
	return $tag;
}

/* ================================================================
   10. EXCERPT SETTINGS
   ================================================================ */

add_filter( 'excerpt_length', function() { return 30; } );
add_filter( 'excerpt_more',   function() {
	return ' &hellip; <a class="read-more" href="' . esc_url( get_permalink() ) . '">'
		. esc_html__( 'Read more', 'themisdb-v2' ) . '</a>';
} );

/* ================================================================
   11. CUSTOM BLOCK CATEGORY
   ================================================================ */

add_filter( 'block_categories_all', 'themisdb_v2_block_categories', 10, 2 );

function themisdb_v2_block_categories( $categories, $post ) {
	return array_merge(
		array( array(
			'slug'  => 'themisdb',
			'title' => __( 'ThemisDB', 'themisdb-v2' ),
			'icon'  => 'database',
		) ),
		$categories
	);
}
