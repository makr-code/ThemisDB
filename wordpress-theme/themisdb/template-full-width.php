<?php
/**
 * Full Width Page Template
 *
 * Template Name: Full Width
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

        if ( comments_open() || get_comments_number() ) :
            comments_template();
        endif;

    endwhile;
    ?>
</main>

<?php
get_footer();
