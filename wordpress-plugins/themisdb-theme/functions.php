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
        'primary'   => esc_html__( 'Primary Menu', 'themisdb' ),
        'footer'    => esc_html__( 'Footer Menu', 'themisdb' ),
        'hamburger' => esc_html__( 'Hamburger Menu', 'themisdb' ),
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
 * Load custom widgets
 */
require get_template_directory() . '/inc/widgets.php';

/**
 * Enqueue scripts and styles
 */
function themisdb_scripts() {
    // Main stylesheet
    wp_enqueue_style( 'themisdb-style', get_stylesheet_uri(), array(), '1.0.0' );

    // Widgets stylesheet
    wp_enqueue_style( 'themisdb-widgets', get_template_directory_uri() . '/css/widgets.css', array( 'themisdb-style' ), '1.0.0' );

    // Theme JavaScript
    wp_enqueue_script( 'themisdb-navigation', get_template_directory_uri() . '/js/navigation.js', array(), '1.0.0', true );

    // Modern enhancements (animations, lazy loading, etc.)
    wp_enqueue_script( 'themisdb-enhancements', get_template_directory_uri() . '/js/enhancements.js', array(), '1.0.0', true );

    // Featured Slider
    wp_enqueue_script( 'themisdb-slider', get_template_directory_uri() . '/js/slider.js', array(), '1.0.0', true );

    // Carousel widgets (testimonials, images, timeline)
    wp_enqueue_script( 'themisdb-carousel', get_template_directory_uri() . '/js/carousel.js', array(), '1.0.0', true );

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
    echo '📅 ';
    echo $time_string;
    echo '</span>';
}

/**
 * Display post author
 */
function themisdb_posted_by() {
    echo '<span class="byline">';
    echo '👤 ';
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
        echo '📁 ';
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
        echo '🏷️ ';
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
        echo '💬 ';
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

/**
 * Breadcrumb Navigation
 * Display hierarchical navigation path
 */
function themisdb_breadcrumbs() {
    // Don't display on homepage
    if ( is_front_page() ) {
        return;
    }

    $separator = ' 🔸 ';
    $home_title = '🏠 ' . esc_html__( 'Home', 'themisdb' );

    echo '<nav class="breadcrumbs" aria-label="' . esc_attr__( 'Breadcrumb', 'themisdb' ) . '">';
    echo '<ol class="breadcrumb-list">';

    // Home link
    echo '<li class="breadcrumb-item"><a href="' . esc_url( home_url( '/' ) ) . '">' . $home_title . '</a></li>';

    if ( is_category() || is_single() ) {
        $categories = get_the_category();
        if ( ! empty( $categories ) ) {
            $category = $categories[0];
            if ( $category->parent ) {
                $parent_cats = array();
                $current_cat = $category;
                while ( $current_cat->parent ) {
                    $current_cat = get_category( $current_cat->parent );
                    $parent_cats[] = $current_cat;
                }
                $parent_cats = array_reverse( $parent_cats );
                foreach ( $parent_cats as $parent_cat ) {
                    echo '<li class="breadcrumb-item">' . $separator . '<a href="' . esc_url( get_category_link( $parent_cat->term_id ) ) . '">📁 ' . esc_html( $parent_cat->name ) . '</a></li>';
                }
            }
            echo '<li class="breadcrumb-item">' . $separator . '<a href="' . esc_url( get_category_link( $category->term_id ) ) . '">📁 ' . esc_html( $category->name ) . '</a></li>';
        }

        if ( is_single() ) {
            echo '<li class="breadcrumb-item active" aria-current="page">' . $separator . '📝 ' . esc_html( get_the_title() ) . '</li>';
        }
    } elseif ( is_page() ) {
        if ( $post = get_post() ) {
            if ( $post->post_parent ) {
                $parent_id  = $post->post_parent;
                $breadcrumbs = array();
                while ( $parent_id ) {
                    $page = get_post( $parent_id );
                    $breadcrumbs[] = '<li class="breadcrumb-item">' . $separator . '<a href="' . esc_url( get_permalink( $page->ID ) ) . '">📄 ' . esc_html( get_the_title( $page->ID ) ) . '</a></li>';
                    $parent_id = $page->post_parent;
                }
                $breadcrumbs = array_reverse( $breadcrumbs );
                foreach ( $breadcrumbs as $crumb ) {
                    echo $crumb;
                }
            }
            echo '<li class="breadcrumb-item active" aria-current="page">' . $separator . '📄 ' . esc_html( get_the_title() ) . '</li>';
        }
    } elseif ( is_tag() ) {
        echo '<li class="breadcrumb-item active" aria-current="page">' . $separator . '🏷️ ' . esc_html( single_tag_title( '', false ) ) . '</li>';
    } elseif ( is_author() ) {
        echo '<li class="breadcrumb-item active" aria-current="page">' . $separator . '👤 ' . esc_html( get_the_author() ) . '</li>';
    } elseif ( is_day() ) {
        echo '<li class="breadcrumb-item">' . $separator . '<a href="' . esc_url( get_year_link( get_the_time( 'Y' ) ) ) . '">📅 ' . esc_html( get_the_time( 'Y' ) ) . '</a></li>';
        echo '<li class="breadcrumb-item">' . $separator . '<a href="' . esc_url( get_month_link( get_the_time( 'Y' ), get_the_time( 'm' ) ) ) . '">' . esc_html( get_the_time( 'F' ) ) . '</a></li>';
        echo '<li class="breadcrumb-item active" aria-current="page">' . $separator . esc_html( get_the_time( 'd' ) ) . '</li>';
    } elseif ( is_month() ) {
        echo '<li class="breadcrumb-item">' . $separator . '<a href="' . esc_url( get_year_link( get_the_time( 'Y' ) ) ) . '">📅 ' . esc_html( get_the_time( 'Y' ) ) . '</a></li>';
        echo '<li class="breadcrumb-item active" aria-current="page">' . $separator . esc_html( get_the_time( 'F' ) ) . '</li>';
    } elseif ( is_year() ) {
        echo '<li class="breadcrumb-item active" aria-current="page">' . $separator . '📅 ' . esc_html( get_the_time( 'Y' ) ) . '</li>';
    } elseif ( is_search() ) {
        echo '<li class="breadcrumb-item active" aria-current="page">' . $separator . '🔍 ' . esc_html__( 'Search Results', 'themisdb' ) . '</li>';
    } elseif ( is_404() ) {
        echo '<li class="breadcrumb-item active" aria-current="page">' . $separator . '❌ ' . esc_html__( '404 Error', 'themisdb' ) . '</li>';
    }

    echo '</ol>';
    echo '</nav>';
}

/**
 * Social Share Buttons
 * Display social sharing options for posts
 */
function themisdb_social_share_buttons() {
    if ( ! is_single() ) {
        return;
    }

    $post_url = urlencode( get_permalink() );
    $post_title = urlencode( get_the_title() );
    
    ?>
    <div class="social-share">
        <h3 class="social-share-title">🔗 <?php esc_html_e( 'Share this post:', 'themisdb' ); ?></h3>
        <div class="social-share-buttons">
            <a href="https://twitter.com/intent/tweet?url=<?php echo $post_url; ?>&text=<?php echo $post_title; ?>" 
               target="_blank" 
               rel="noopener noreferrer" 
               class="share-button share-twitter"
               aria-label="<?php esc_attr_e( 'Share on Twitter', 'themisdb' ); ?>">
                🐦 Twitter
            </a>
            <a href="https://www.facebook.com/sharer/sharer.php?u=<?php echo $post_url; ?>" 
               target="_blank" 
               rel="noopener noreferrer" 
               class="share-button share-facebook"
               aria-label="<?php esc_attr_e( 'Share on Facebook', 'themisdb' ); ?>">
                📘 Facebook
            </a>
            <a href="https://www.linkedin.com/shareArticle?mini=true&url=<?php echo $post_url; ?>&title=<?php echo $post_title; ?>" 
               target="_blank" 
               rel="noopener noreferrer" 
               class="share-button share-linkedin"
               aria-label="<?php esc_attr_e( 'Share on LinkedIn', 'themisdb' ); ?>">
                💼 LinkedIn
            </a>
            <a href="mailto:?subject=<?php echo $post_title; ?>&body=<?php echo $post_url; ?>" 
               class="share-button share-email"
               aria-label="<?php esc_attr_e( 'Share via Email', 'themisdb' ); ?>">
                ✉️ Email
            </a>
            <button class="share-button share-copy" 
                    data-url="<?php echo esc_url( get_permalink() ); ?>"
                    onclick="themisdbCopyUrl(this)"
                    aria-label="<?php esc_attr_e( 'Copy link', 'themisdb' ); ?>">
                📋 <?php esc_html_e( 'Copy Link', 'themisdb' ); ?>
            </button>
        </div>
    </div>
    <?php
}

/**
 * Estimated Reading Time
 * Calculate and display reading time for posts
 */
function themisdb_reading_time() {
    $content = get_post_field( 'post_content', get_the_ID() );
    $word_count = str_word_count( strip_tags( $content ) );
    $reading_time = ceil( $word_count / 200 ); // Average reading speed: 200 words per minute

    if ( $reading_time > 0 ) {
        printf(
            '<span class="reading-time">⏱️ %s</span>',
            sprintf(
                _n( '%d minute read', '%d minutes read', $reading_time, 'themisdb' ),
                $reading_time
            )
        );
    }
}

/**
 * Hamburger Menu Fallback
 * Display default items when no menu is assigned
 */
function themisdb_hamburger_menu_fallback() {
    ?>
    <ul id="hamburger-menu" class="menu">
        <li><a href="<?php echo esc_url( admin_url( 'nav-menus.php' ) ); ?>">⚙️ <?php esc_html_e( 'Settings', 'themisdb' ); ?></a></li>
        <li><a href="<?php echo esc_url( home_url( '/about' ) ); ?>">ℹ️ <?php esc_html_e( 'About', 'themisdb' ); ?></a></li>
        <li><a href="<?php echo esc_url( home_url( '/contact' ) ); ?>">📧 <?php esc_html_e( 'Contact', 'themisdb' ); ?></a></li>
    </ul>
    <?php
}
