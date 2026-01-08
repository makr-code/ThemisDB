<?php
/**
 * Template part for displaying page content
 *
 * @package ThemisDB
 */
?>

<article id="post-<?php the_ID(); ?>" <?php post_class(); ?>>
    <?php if ( has_post_thumbnail() && ! post_password_required() ) : ?>
        <div class="entry-thumbnail">
            <?php the_post_thumbnail( 'themisdb-featured' ); ?>
        </div>
    <?php endif; ?>

    <header class="entry-header">
        <?php the_title( '<h1 class="entry-title">', '</h1>' ); ?>
    </header>

    <div class="entry-content">
        <?php
        the_content();

        wp_link_pages( array(
            'before' => '<div class="page-links">' . esc_html__( 'Pages:', 'themisdb' ),
            'after'  => '</div>',
        ) );
        ?>
    </div>
</article>
