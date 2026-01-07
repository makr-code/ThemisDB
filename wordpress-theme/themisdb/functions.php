<?php
/**
 * ThemisDB Theme Functions
 *
 * @package ThemisDB
 * @since 1.0.0
 */

if ( ! defined( 'ABSPATH' ) ) {
    exit; // Exit if accessed directly
}

/**
 * Theme setup
 */
function themisdb_setup() {
    // Make theme available for translation
    load_theme_textdomain( 'themisdb', get_template_directory() . '/languages' );

    // Add default posts and comments RSS feed links to head
    add_theme_support( 'automatic-feed-links' );

    // Let WordPress manage the document title
    add_theme_support( 'title-tag' );

    // Enable support for Post Thumbnails
    add_theme_support( 'post-thumbnails' );
    set_post_thumbnail_size( 1200, 675, true );

    // Add custom image sizes
    add_image_size( 'themisdb-featured', 1200, 675, true );
    add_image_size( 'themisdb-thumbnail', 400, 300, true );

    // Register navigation menus
    register_nav_menus( array(
        'primary' => esc_html__( 'Primary Menu', 'themisdb' ),
        'footer'  => esc_html__( 'Footer Menu', 'themisdb' ),
    ) );

    // Switch default core markup to output valid HTML5
    add_theme_support( 'html5', array(
        'search-form',
        'comment-form',
        'comment-list',
        'gallery',
        'caption',
        'style',
        'script',
    ) );

    // Add theme support for selective refresh for widgets
    add_theme_support( 'customize-selective-refresh-widgets' );

    // Add support for custom logo
    add_theme_support( 'custom-logo', array(
        'height'      => 60,
        'width'       => 200,
        'flex-height' => true,
        'flex-width'  => true,
    ) );

    // Add support for custom background
    add_theme_support( 'custom-background', array(
        'default-color' => 'f8f9fa',
    ) );

    // Add support for custom header
    add_theme_support( 'custom-header', array(
        'default-image' => '',
        'width'         => 1920,
        'height'        => 400,
        'flex-width'    => true,
        'flex-height'   => true,
    ) );

    // Add support for editor styles
    add_theme_support( 'editor-styles' );
    add_editor_style( 'editor-style.css' );

    // Add support for responsive embeds
    add_theme_support( 'responsive-embeds' );

    // Add support for wide alignment
    add_theme_support( 'align-wide' );

    // Add support for editor color palette
    add_theme_support( 'editor-color-palette', array(
        array(
            'name'  => esc_html__( 'Primary', 'themisdb' ),
            'slug'  => 'primary',
            'color' => '#2c3e50',
        ),
        array(
            'name'  => esc_html__( 'Secondary', 'themisdb' ),
            'slug'  => 'secondary',
            'color' => '#3498db',
        ),
        array(
            'name'  => esc_html__( 'Accent Purple', 'themisdb' ),
            'slug'  => 'accent-purple',
            'color' => '#7c4dff',
        ),
        array(
            'name'  => esc_html__( 'Success', 'themisdb' ),
            'slug'  => 'success',
            'color' => '#27ae60',
        ),
        array(
            'name'  => esc_html__( 'Warning', 'themisdb' ),
            'slug'  => 'warning',
            'color' => '#f39c12',
        ),
        array(
            'name'  => esc_html__( 'Danger', 'themisdb' ),
            'slug'  => 'danger',
            'color' => '#e74c3c',
        ),
    ) );
}
add_action( 'after_setup_theme', 'themisdb_setup' );

/**
 * Set the content width in pixels
 */
function themisdb_content_width() {
    $GLOBALS['content_width'] = apply_filters( 'themisdb_content_width', 1200 );
}
add_action( 'after_setup_theme', 'themisdb_content_width', 0 );

/**
 * Register widget areas
 */
function themisdb_widgets_init() {
    register_sidebar( array(
        'name'          => esc_html__( 'Sidebar', 'themisdb' ),
        'id'            => 'sidebar-1',
        'description'   => esc_html__( 'Add widgets here to appear in your sidebar.', 'themisdb' ),
        'before_widget' => '<section id="%1$s" class="widget %2$s">',
        'after_widget'  => '</section>',
        'before_title'  => '<h2 class="widget-title">',
        'after_title'   => '</h2>',
    ) );

    // Footer widgets
    for ( $i = 1; $i <= 3; $i++ ) {
        register_sidebar( array(
            'name'          => sprintf( esc_html__( 'Footer Widget %d', 'themisdb' ), $i ),
            'id'            => 'footer-' . $i,
            'description'   => sprintf( esc_html__( 'Add widgets here to appear in footer column %d.', 'themisdb' ), $i ),
            'before_widget' => '<div id="%1$s" class="footer-widget %2$s">',
            'after_widget'  => '</div>',
            'before_title'  => '<h3 class="widget-title">',
            'after_title'   => '</h3>',
        ) );
    }
}
add_action( 'widgets_init', 'themisdb_widgets_init' );

/**
 * Enqueue scripts and styles
 */
function themisdb_scripts() {
    // Main stylesheet
    wp_enqueue_style( 'themisdb-style', get_stylesheet_uri(), array(), '1.0.0' );

    // Theme JavaScript
    wp_enqueue_script( 'themisdb-navigation', get_template_directory_uri() . '/js/navigation.js', array(), '1.0.0', true );

    // Graph Navigation (Neo4j Bloom inspired)
    wp_enqueue_script( 'themisdb-graph-navigation', get_template_directory_uri() . '/js/graph-navigation.js', array(), '1.0.0', true );

    // Comment reply script
    if ( is_singular() && comments_open() && get_option( 'thread_comments' ) ) {
        wp_enqueue_script( 'comment-reply' );
    }
}
add_action( 'wp_enqueue_scripts', 'themisdb_scripts' );

/**
 * Add body classes
 */
function themisdb_body_classes( $classes ) {
    // Add class if sidebar is active
    if ( is_active_sidebar( 'sidebar-1' ) && ! is_page_template( 'template-full-width.php' ) ) {
        $classes[] = 'has-sidebar';
    }

    // Add class for single posts
    if ( is_singular() ) {
        $classes[] = 'singular';
    }

    return $classes;
}
add_filter( 'body_class', 'themisdb_body_classes' );

/**
 * Custom excerpt length
 */
function themisdb_excerpt_length( $length ) {
    return 40;
}
add_filter( 'excerpt_length', 'themisdb_excerpt_length', 999 );

/**
 * Custom excerpt more
 */
function themisdb_excerpt_more( $more ) {
    return '&hellip;';
}
add_filter( 'excerpt_more', 'themisdb_excerpt_more' );

/**
 * Display post meta information
 */
function themisdb_posted_on() {
    $time_string = '<time class="entry-date published updated" datetime="%1$s">%2$s</time>';
    if ( get_the_time( 'U' ) !== get_the_modified_time( 'U' ) ) {
        $time_string = '<time class="entry-date published" datetime="%1$s">%2$s</time><time class="updated" datetime="%3$s">%4$s</time>';
    }

    $time_string = sprintf( $time_string,
        esc_attr( get_the_date( DATE_W3C ) ),
        esc_html( get_the_date() ),
        esc_attr( get_the_modified_date( DATE_W3C ) ),
        esc_html( get_the_modified_date() )
    );

    echo '<span class="posted-on">';
    echo '<svg width="16" height="16" fill="currentColor" viewBox="0 0 16 16"><path d="M3.5 0a.5.5 0 0 1 .5.5V1h8V.5a.5.5 0 0 1 1 0V1h1a2 2 0 0 1 2 2v11a2 2 0 0 1-2 2H2a2 2 0 0 1-2-2V3a2 2 0 0 1 2-2h1V.5a.5.5 0 0 1 .5-.5zM1 4v10a1 1 0 0 0 1 1h12a1 1 0 0 0 1-1V4H1z"/></svg>';
    echo $time_string;
    echo '</span>';
}

/**
 * Display post author
 */
function themisdb_posted_by() {
    echo '<span class="byline">';
    echo '<svg width="16" height="16" fill="currentColor" viewBox="0 0 16 16"><path d="M8 8a3 3 0 1 0 0-6 3 3 0 0 0 0 6zm2-3a2 2 0 1 1-4 0 2 2 0 0 1 4 0zm4 8c0 1-1 1-1 1H3s-1 0-1-1 1-4 6-4 6 3 6 4zm-1-.004c-.001-.246-.154-.986-.832-1.664C11.516 10.68 10.289 10 8 10c-2.29 0-3.516.68-4.168 1.332-.678.678-.83 1.418-.832 1.664h10z"/></svg>';
    printf(
        '<a href="%s">%s</a>',
        esc_url( get_author_posts_url( get_the_author_meta( 'ID' ) ) ),
        esc_html( get_the_author() )
    );
    echo '</span>';
}

/**
 * Display category links
 */
function themisdb_categories() {
    $categories_list = get_the_category_list( ', ' );
    if ( $categories_list ) {
        echo '<span class="cat-links">';
        echo '<svg width="16" height="16" fill="currentColor" viewBox="0 0 16 16"><path d="M1 2.5A1.5 1.5 0 0 1 2.5 1h3.879a1.5 1.5 0 0 1 1.06.44l3.122 3.12A1.5 1.5 0 0 1 11 5.622V13.5a1.5 1.5 0 0 1-1.5 1.5h-7A1.5 1.5 0 0 1 1 13.5v-11z"/></svg>';
        echo $categories_list;
        echo '</span>';
    }
}

/**
 * Display tag links
 */
function themisdb_tags() {
    $tags_list = get_the_tag_list( '', ', ' );
    if ( $tags_list ) {
        echo '<span class="tags-links">';
        echo '<svg width="16" height="16" fill="currentColor" viewBox="0 0 16 16"><path d="M2 1a1 1 0 0 0-1 1v4.586a1 1 0 0 0 .293.707l7 7a1 1 0 0 0 1.414 0l4.586-4.586a1 1 0 0 0 0-1.414l-7-7A1 1 0 0 0 6.586 1H2zm4 3.5a1.5 1.5 0 1 1-3 0 1.5 1.5 0 0 1 3 0z"/></svg>';
        echo $tags_list;
        echo '</span>';
    }
}

/**
 * Display comments link
 */
function themisdb_comments_link() {
    if ( ! is_single() && ! post_password_required() && ( comments_open() || get_comments_number() ) ) {
        echo '<span class="comments-link">';
        echo '<svg width="16" height="16" fill="currentColor" viewBox="0 0 16 16"><path d="M2.678 11.894a1 1 0 0 1 .287.801 10.97 10.97 0 0 1-.398 2c1.395-.323 2.247-.697 2.634-.893a1 1 0 0 1 .71-.074A8.06 8.06 0 0 0 8 14c3.996 0 7-2.807 7-6 0-3.192-3.004-6-7-6S1 4.808 1 8c0 1.468.617 2.83 1.678 3.894zm-.493 3.905a21.682 21.682 0 0 1-.713.129c-.2.032-.352-.176-.273-.362a9.68 9.68 0 0 0 .244-.637l.003-.01c.248-.72.45-1.548.524-2.319C.743 11.37 0 9.76 0 8c0-3.866 3.582-7 8-7s8 3.134 8 7-3.582 7-8 7a9.06 9.06 0 0 1-2.347-.306c-.52.263-1.639.742-3.468 1.105z"/></svg>';
        comments_popup_link(
            sprintf(
                wp_kses(
                    __( 'Leave a Comment<span class="screen-reader-text"> on %s</span>', 'themisdb' ),
                    array(
                        'span' => array(
                            'class' => array(),
                        ),
                    )
                ),
                wp_kses_post( get_the_title() )
            )
        );
        echo '</span>';
    }
}

/**
 * Customize the read more link
 */
function themisdb_read_more_link() {
    return sprintf(
        '<div class="more-link-wrapper"><a href="%s" class="more-link">%s</a></div>',
        esc_url( get_permalink() ),
        esc_html__( 'Read More', 'themisdb' )
    );
}
add_filter( 'the_content_more_link', 'themisdb_read_more_link' );

/**
 * Customizer additions
 */
function themisdb_customize_register( $wp_customize ) {
    // Theme Colors Section
    $wp_customize->add_section( 'themisdb_colors', array(
        'title'    => esc_html__( 'Theme Colors', 'themisdb' ),
        'priority' => 30,
    ) );

    // Primary Color
    $wp_customize->add_setting( 'themisdb_primary_color', array(
        'default'           => '#2c3e50',
        'sanitize_callback' => 'sanitize_hex_color',
    ) );

    $wp_customize->add_control( new WP_Customize_Color_Control( $wp_customize, 'themisdb_primary_color', array(
        'label'    => esc_html__( 'Primary Color', 'themisdb' ),
        'section'  => 'themisdb_colors',
        'settings' => 'themisdb_primary_color',
    ) ) );

    // Secondary Color
    $wp_customize->add_setting( 'themisdb_secondary_color', array(
        'default'           => '#3498db',
        'sanitize_callback' => 'sanitize_hex_color',
    ) );

    $wp_customize->add_control( new WP_Customize_Color_Control( $wp_customize, 'themisdb_secondary_color', array(
        'label'    => esc_html__( 'Secondary Color', 'themisdb' ),
        'section'  => 'themisdb_colors',
        'settings' => 'themisdb_secondary_color',
    ) ) );

    // Accent Color
    $wp_customize->add_setting( 'themisdb_accent_color', array(
        'default'           => '#7c4dff',
        'sanitize_callback' => 'sanitize_hex_color',
    ) );

    $wp_customize->add_control( new WP_Customize_Color_Control( $wp_customize, 'themisdb_accent_color', array(
        'label'    => esc_html__( 'Accent Color', 'themisdb' ),
        'section'  => 'themisdb_colors',
        'settings' => 'themisdb_accent_color',
    ) ) );
}
add_action( 'customize_register', 'themisdb_customize_register' );

/**
 * Output custom colors CSS
 */
function themisdb_custom_colors_css() {
    $primary_color   = get_theme_mod( 'themisdb_primary_color', '#2c3e50' );
    $secondary_color = get_theme_mod( 'themisdb_secondary_color', '#3498db' );
    $accent_color    = get_theme_mod( 'themisdb_accent_color', '#7c4dff' );

    $css = "
        :root {
            --primary-color: {$primary_color};
            --secondary-color: {$secondary_color};
            --accent-purple: {$accent_color};
        }
    ";

    wp_add_inline_style( 'themisdb-style', $css );
}
add_action( 'wp_enqueue_scripts', 'themisdb_custom_colors_css' );
