    </div><!-- #content -->

    <footer id="colophon" class="site-footer">
        <?php if ( is_active_sidebar( 'footer-1' ) || is_active_sidebar( 'footer-2' ) || is_active_sidebar( 'footer-3' ) ) : ?>
            <div class="footer-widgets">
                <?php
                for ( $i = 1; $i <= 3; $i++ ) {
                    if ( is_active_sidebar( 'footer-' . $i ) ) {
                        dynamic_sidebar( 'footer-' . $i );
                    }
                }
                ?>
            </div>
        <?php endif; ?>

        <div class="site-info">
            <p>
                <?php
                printf(
                    esc_html__( '⚡ Powered by %1$s | 🎨 Theme: %2$s', 'themisdb' ),
                    '<a href="' . esc_url( 'https://github.com/makr-code/wordpressPlugins' ) . '">ThemisDB</a>',
                    '<a href="' . esc_url( 'https://github.com/makr-code/wordpressPlugins' ) . '">ThemisDB Theme</a>'
                );
                ?>
            </p>
            <?php
            if ( has_nav_menu( 'footer' ) ) {
                wp_nav_menu( array(
                    'theme_location' => 'footer',
                    'menu_class'     => 'footer-menu',
                    'depth'          => 1,
                ) );
            }
            ?>
        </div>
    </footer>
    
    <!-- Scroll to Top Button -->
    <button id="scroll-to-top" class="scroll-to-top" aria-label="Scroll to top" title="Scroll to top">
        ⬆
    </button>
</div><!-- #page -->

<?php wp_footer(); ?>

</body>
</html>
