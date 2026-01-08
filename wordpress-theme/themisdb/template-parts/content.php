<?php
/**
 * Template part for displaying posts
 *
 * @package ThemisDB
 */
?>

<article id="post-<?php the_ID(); ?>" <?php post_class(); ?>>
    <?php if ( has_post_thumbnail() ) : ?>
        <div class="entry-thumbnail">
            <a href="<?php the_permalink(); ?>">
                <?php the_post_thumbnail( 'themisdb-featured' ); ?>
            </a>
        </div>
    <?php endif; ?>

    <header class="entry-header">
        <?php
        if ( is_singular() ) :
            the_title( '<h1 class="entry-title">', '</h1>' );
        else :
            the_title( '<h2 class="entry-title"><a href="' . esc_url( get_permalink() ) . '" rel="bookmark">', '</a></h2>' );
        endif;
        ?>

        <div class="entry-meta">
            <?php
            themisdb_posted_on();
            themisdb_posted_by();
            themisdb_categories();
            themisdb_comments_link();
            ?>
        </div>
    </header>

    <div class="entry-content">
        <?php
        if ( is_singular() ) {
            the_content();
        } else {
            the_excerpt();
            echo '<a href="' . esc_url( get_permalink() ) . '" class="button">📖 ' . esc_html__( 'Read More', 'themisdb' ) . '</a>';
        }

        wp_link_pages( array(
            'before' => '<div class="page-links">' . esc_html__( 'Pages:', 'themisdb' ),
            'after'  => '</div>',
        ) );
        ?>
    </div>

    <?php if ( is_singular() ) : ?>
        <footer class="entry-footer">
            <?php
            themisdb_categories();
            themisdb_tags();
            ?>
        </footer>
    <?php endif; ?>
</article>
