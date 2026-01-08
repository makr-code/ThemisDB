<?php
/**
 * The main template file
 *
 * @package ThemisDB
 */

get_header();
?>

<main id="primary" class="content-area">
    <?php
    if ( have_posts() ) :

        // Display featured slider on front page
        if ( is_front_page() && ! is_paged() ) :
            $sticky_posts = get_option( 'sticky_posts' );
            if ( ! empty( $sticky_posts ) ) :
                ?>
                <section class="featured-slider-section">
                    <h2 class="section-title"><?php esc_html_e( 'Featured Articles', 'themisdb' ); ?></h2>
                    <?php
                    $featured_query = new WP_Query( array(
                        'posts_per_page' => 5,
                        'post__in'       => $sticky_posts,
                        'ignore_sticky_posts' => 1,
                    ) );

                    if ( $featured_query->have_posts() ) :
                        ?>
                        <div class="themisdb-slider-container homepage-slider">
                            <div class="themisdb-slider">
                                <?php while ( $featured_query->have_posts() ) : $featured_query->the_post(); ?>
                                    <div class="slider-item">
                                        <?php if ( has_post_thumbnail() ) : ?>
                                            <div class="slider-image">
                                                <a href="<?php the_permalink(); ?>">
                                                    <?php the_post_thumbnail( 'themisdb-featured' ); ?>
                                                </a>
                                            </div>
                                        <?php endif; ?>
                                        <div class="slider-content">
                                            <h3 class="slider-title">
                                                <a href="<?php the_permalink(); ?>"><?php the_title(); ?></a>
                                            </h3>
                                            <div class="slider-meta">
                                                <span class="slider-date"><?php echo esc_html( get_the_date() ); ?></span>
                                                <?php if ( get_the_category() ) : ?>
                                                    <span class="slider-category"> • <?php the_category( ', ' ); ?></span>
                                                <?php endif; ?>
                                            </div>
                                            <div class="slider-excerpt">
                                                <?php echo wp_trim_words( get_the_excerpt(), 25 ); ?>
                                            </div>
                                            <a href="<?php the_permalink(); ?>" class="slider-readmore">
                                                <?php esc_html_e( 'Read More', 'themisdb' ); ?> →
                                            </a>
                                        </div>
                                    </div>
                                <?php endwhile; ?>
                            </div>
                            <?php if ( $featured_query->post_count > 1 ) : ?>
                                <button class="slider-nav slider-prev" aria-label="<?php esc_attr_e( 'Previous slide', 'themisdb' ); ?>">‹</button>
                                <button class="slider-nav slider-next" aria-label="<?php esc_attr_e( 'Next slide', 'themisdb' ); ?>">›</button>
                                <div class="slider-dots"></div>
                            <?php endif; ?>
                        </div>
                        <?php
                        wp_reset_postdata();
                    endif;
                    ?>
                </section>
                <?php
            endif;
        endif;

        if ( is_home() && ! is_front_page() ) :
            ?>
            <header class="page-header">
                <h1 class="page-title"><?php single_post_title(); ?></h1>
            </header>
            <?php
        endif;
        ?>

        <div class="posts-container">
            <?php
            while ( have_posts() ) :
                the_post();
                get_template_part( 'template-parts/content', get_post_type() );
            endwhile;
            ?>
        </div>

        <?php
        the_posts_pagination( array(
            'mid_size'  => 2,
            'prev_text' => __( '&laquo; Previous', 'themisdb' ),
            'next_text' => __( 'Next &raquo;', 'themisdb' ),
        ) );

    else :

        get_template_part( 'template-parts/content', 'none' );

    endif;
    ?>
</main>

<?php
get_sidebar();
get_footer();
