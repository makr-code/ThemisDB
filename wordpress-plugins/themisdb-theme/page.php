<?php
/**
 * The template for displaying all pages
 *
 * @package ThemisDB
 */

get_header();
?>

<main id="primary" class="content-area">
    <?php
    while ( have_posts() ) :
        the_post();

        get_template_part( 'template-parts/content', 'page' );

        // If comments are open or we have at least one comment, load up the comment template.
        if ( comments_open() || get_comments_number() ) :
            comments_template();
        endif;

    endwhile;
    ?>
</main>

<?php
get_sidebar();
get_footer();
