<?php
/**
 * The sidebar containing the main widget area
 *
 * @package ThemisDB
 */

if ( ! is_active_sidebar( 'sidebar-1' ) ) {
    return;
}
?>

<aside id="secondary" class="sidebar widget-area">
    <?php dynamic_sidebar( 'sidebar-1' ); ?>
</aside>
