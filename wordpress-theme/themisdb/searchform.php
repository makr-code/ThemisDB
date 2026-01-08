<?php
/**
 * The template for displaying search forms
 *
 * @package ThemisDB
 */
?>

<form role="search" method="get" class="search-form" action="<?php echo esc_url( home_url( '/' ) ); ?>">
    <label>
        <span class="screen-reader-text"><?php esc_html_e( 'Search for:', 'themisdb' ); ?></span>
        <input type="search" class="search-field" placeholder="<?php esc_attr_e( '🔍 Search...', 'themisdb' ); ?>" value="<?php echo get_search_query(); ?>" name="s" />
    </label>
    <button type="submit" class="search-submit">
        <span class="screen-reader-text"><?php esc_html_e( 'Search', 'themisdb' ); ?></span>
        <span class="search-icon">🔎</span>
    </button>
</form>
