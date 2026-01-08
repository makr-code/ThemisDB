<?php
/**
 * The template for displaying 404 pages (not found)
 *
 * @package ThemisDB
 */

get_header();
?>

<main id="primary" class="content-area">
    <section class="error-404 not-found">
        <header class="page-header">
            <div class="error-icon">🔍❌</div>
            <h1 class="page-title"><?php esc_html_e( '404 - Page Not Found', 'themisdb' ); ?></h1>
            <p class="error-subtitle"><?php esc_html_e( 'Oops! The page you are looking for does not exist.', 'themisdb' ); ?></p>
        </header>

        <div class="page-content">
            <div class="error-suggestions">
                <h2>💡 <?php esc_html_e( 'Try searching for what you need:', 'themisdb' ); ?></h2>
                <?php get_search_form(); ?>
            </div>

            <div class="error-navigation">
                <h2>🧭 <?php esc_html_e( 'Or explore these sections:', 'themisdb' ); ?></h2>
                
                <div class="error-widgets">
                    <div class="error-widget">
                        <h3>📝 <?php esc_html_e( 'Recent Posts', 'themisdb' ); ?></h3>
                        <ul>
                            <?php
                            $recent_posts = wp_get_recent_posts( array(
                                'numberposts' => 5,
                                'post_status' => 'publish',
                            ) );
                            
                            foreach ( $recent_posts as $post ) {
                                printf(
                                    '<li><a href="%s">📄 %s</a></li>',
                                    esc_url( get_permalink( $post['ID'] ) ),
                                    esc_html( $post['post_title'] )
                                );
                            }
                            wp_reset_postdata();
                            ?>
                        </ul>
                    </div>

                    <div class="error-widget">
                        <h3>📁 <?php esc_html_e( 'Categories', 'themisdb' ); ?></h3>
                        <ul>
                            <?php
                            wp_list_categories( array(
                                'title_li' => '',
                                'number'   => 10,
                            ) );
                            ?>
                        </ul>
                    </div>

                    <div class="error-widget">
                        <h3>📅 <?php esc_html_e( 'Archives', 'themisdb' ); ?></h3>
                        <ul>
                            <?php
                            wp_get_archives( array(
                                'type'  => 'monthly',
                                'limit' => 12,
                            ) );
                            ?>
                        </ul>
                    </div>
                </div>
            </div>

            <div class="error-home">
                <a href="<?php echo esc_url( home_url( '/' ) ); ?>" class="button button-primary">
                    🏠 <?php esc_html_e( 'Return to Homepage', 'themisdb' ); ?>
                </a>
            </div>
        </div>
    </section>
</main>

<?php
get_footer();
