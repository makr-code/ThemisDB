<?php
/**
 * The template for displaying archive pages
 *
 * @package ThemisDB
 */

get_header();
?>

<main id="primary" class="content-area">
    <?php if ( have_posts() ) : ?>

        <header class="page-header">
            <?php
            the_archive_title( '<h1 class="page-title">', '</h1>' );
            the_archive_description( '<div class="archive-description">', '</div>' );
            ?>
        </header>

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
