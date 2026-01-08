<?php
/**
 * Template part for displaying single posts
 *
 * @package ThemisDB
 */
?>

<article id="post-<?php the_ID(); ?>" <?php post_class(); ?>>
    <?php if ( has_post_thumbnail() ) : ?>
        <div class="entry-thumbnail">
            <?php the_post_thumbnail( 'themisdb-featured' ); ?>
        </div>
    <?php endif; ?>

    <header class="entry-header">
        <?php the_title( '<h1 class="entry-title">', '</h1>' ); ?>

        <div class="entry-meta">
            <?php
            themisdb_posted_on();
            themisdb_posted_by();
            themisdb_reading_time();
            themisdb_categories();
            themisdb_comments_link();
            ?>
        </div>
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

    <footer class="entry-footer">
        <?php
        themisdb_categories();
        themisdb_tags();
        ?>
    </footer>
</article>
