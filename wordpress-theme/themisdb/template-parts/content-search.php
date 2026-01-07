<?php
/**
 * Template part for displaying search results
 *
 * @package ThemisDB
 */
?>

<article id="post-<?php the_ID(); ?>" <?php post_class(); ?>>
    <header class="entry-header">
        <?php the_title( sprintf( '<h2 class="entry-title"><a href="%s" rel="bookmark">', esc_url( get_permalink() ) ), '</a></h2>' ); ?>

        <?php if ( 'post' === get_post_type() ) : ?>
            <div class="entry-meta">
                <?php
                themisdb_posted_on();
                themisdb_posted_by();
                ?>
            </div>
        <?php endif; ?>
    </header>

    <div class="entry-summary">
        <?php the_excerpt(); ?>
    </div>

    <footer class="entry-footer">
        <?php
        echo '<a href="' . esc_url( get_permalink() ) . '" class="button">' . esc_html__( 'Read More', 'themisdb' ) . '</a>';
        ?>
    </footer>
</article>
