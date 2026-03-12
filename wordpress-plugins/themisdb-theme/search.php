<?php
/**
 * The template for displaying search results
 *
 * @package ThemisDB
 */

get_header();
?>

<main id="primary" class="content-area">
    <?php if ( have_posts() ) : ?>

        <header class="page-header">
            <h1 class="page-title">
                <?php
                printf(
                    esc_html__( 'Search Results for: %s', 'themisdb' ),
                    '<span>' . get_search_query() . '</span>'
                );
                ?>
            </h1>
        </header>

        <div class="posts-container">
            <?php
            while ( have_posts() ) :
                the_post();
                get_template_part( 'template-parts/content', 'search' );
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
