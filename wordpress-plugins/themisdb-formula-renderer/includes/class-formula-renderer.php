<?php
/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            class-formula-renderer.php                         ║
  Version:         0.0.2                                              ║
  Last Modified:   2026-03-09 04:08:18                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     311                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 2a1fb0423  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
    • 9d3ecaa0e  2026-02-28  Add ThemisDB Wiki Integration plugin with documentation i... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */


/**
 * Formula Renderer Class
 * 
 * Handles the rendering of mathematical formulas in content
 */

if (!defined('ABSPATH')) {
    exit;
}

class ThemisDB_Formula_Renderer {
    
    /**
     * Constructor
     */
    public function __construct() {
        // Add filter to process content
        add_filter('the_content', array($this, 'render_formulas'), 10);
        add_filter('comment_text', array($this, 'render_formulas'), 10);
        add_filter('widget_text', array($this, 'render_formulas'), 10);
        
        // Add settings page
        add_action('admin_menu', array($this, 'add_settings_page'));
        add_action('admin_init', array($this, 'register_settings'));
    }
    
    /**
     * Render formulas in content
     * 
     * @param string $content The content to process
     * @return string The processed content
     */
    public function render_formulas($content) {
        $auto_render = get_option('themisdb_formula_auto_render', 1);
        
        if (!$auto_render) {
            return $content;
        }
        
        // Add a wrapper div to help JavaScript identify processed content
        $content = '<div class="themisdb-formula-content">' . $content . '</div>';
        
        return $content;
    }
    
    /**
     * Sanitize delimiter input
     * 
     * @param string $delimiter The delimiter to sanitize
     * @return string The sanitized delimiter
     */
    public function sanitize_delimiter($delimiter) {
        // Remove any HTML and trim whitespace
        $delimiter = sanitize_text_field($delimiter);
        
        // Ensure delimiter is not empty
        if (empty($delimiter)) {
            return '$';
        }
        
        // Limit length to prevent abuse
        if (strlen($delimiter) > 10) {
            $delimiter = substr($delimiter, 0, 10);
        }
        
        // Only allow safe characters: $, \, {, }, [, ], (, ), and alphanumeric
        $delimiter = preg_replace('/[^$\\\{\}\[\]\(\)a-zA-Z0-9]/', '', $delimiter);
        
        // If delimiter becomes empty after sanitization, use default
        if (empty($delimiter)) {
            return '$';
        }
        
        return $delimiter;
    }
    
    /**
     * Add settings page to WordPress admin
     */
    public function add_settings_page() {
        add_options_page(
            __('ThemisDB Formula Renderer', 'themisdb-formula-renderer'),
            __('Formula Renderer', 'themisdb-formula-renderer'),
            'manage_options',
            'themisdb-formula-renderer',
            array($this, 'render_settings_page')
        );
    }
    
    /**
     * Register plugin settings
     */
    public function register_settings() {
        register_setting('themisdb_formula_settings', 'themisdb_formula_auto_render', array(
            'type' => 'integer',
            'sanitize_callback' => 'absint',
            'default' => 1
        ));
        register_setting('themisdb_formula_settings', 'themisdb_formula_inline_delimiter', array(
            'type' => 'string',
            'sanitize_callback' => array($this, 'sanitize_delimiter'),
            'default' => '$'
        ));
        register_setting('themisdb_formula_settings', 'themisdb_formula_block_delimiter', array(
            'type' => 'string',
            'sanitize_callback' => array($this, 'sanitize_delimiter'),
            'default' => '$$'
        ));
        
        add_settings_section(
            'themisdb_formula_main_section',
            __('Formula Rendering Settings', 'themisdb-formula-renderer'),
            array($this, 'render_settings_section'),
            'themisdb-formula-renderer'
        );
        
        add_settings_field(
            'themisdb_formula_auto_render',
            __('Auto-Render Formulas', 'themisdb-formula-renderer'),
            array($this, 'render_auto_render_field'),
            'themisdb-formula-renderer',
            'themisdb_formula_main_section'
        );
        
        add_settings_field(
            'themisdb_formula_inline_delimiter',
            __('Inline Delimiter', 'themisdb-formula-renderer'),
            array($this, 'render_inline_delimiter_field'),
            'themisdb-formula-renderer',
            'themisdb_formula_main_section'
        );
        
        add_settings_field(
            'themisdb_formula_block_delimiter',
            __('Block Delimiter', 'themisdb-formula-renderer'),
            array($this, 'render_block_delimiter_field'),
            'themisdb-formula-renderer',
            'themisdb_formula_main_section'
        );
    }
    
    /**
     * Render settings section description
     */
    public function render_settings_section() {
        echo '<p>' . __('Konfigurieren Sie die Einstellungen für die Formeldarstellung. Formeln werden automatisch in Ihren Beiträgen und Seiten gerendert.', 'themisdb-formula-renderer') . '</p>';
    }
    
    /**
     * Render auto-render checkbox field
     */
    public function render_auto_render_field() {
        $auto_render = get_option('themisdb_formula_auto_render', 1);
        ?>
        <label>
            <input type="checkbox" name="themisdb_formula_auto_render" value="1" <?php checked($auto_render, 1); ?> />
            <?php _e('Automatisch Formeln im Inhalt rendern', 'themisdb-formula-renderer'); ?>
        </label>
        <p class="description">
            <?php _e('Wenn aktiviert, werden alle Formeln in $$...$$ automatisch gerendert.', 'themisdb-formula-renderer'); ?>
        </p>
        <?php
    }
    
    /**
     * Render inline delimiter field
     */
    public function render_inline_delimiter_field() {
        $delimiter = get_option('themisdb_formula_inline_delimiter', '$');
        $delimiter = sanitize_text_field($delimiter);
        ?>
        <input type="text" name="themisdb_formula_inline_delimiter" value="<?php echo esc_attr($delimiter); ?>" maxlength="10" />
        <p class="description">
            <?php _e('Trennzeichen für Inline-Formeln (Standard: $). Beispiel: $E = mc^2$', 'themisdb-formula-renderer'); ?>
        </p>
        <?php
    }
    
    /**
     * Render block delimiter field
     */
    public function render_block_delimiter_field() {
        $delimiter = get_option('themisdb_formula_block_delimiter', '$$');
        $delimiter = sanitize_text_field($delimiter);
        ?>
        <input type="text" name="themisdb_formula_block_delimiter" value="<?php echo esc_attr($delimiter); ?>" maxlength="10" />
        <p class="description">
            <?php _e('Trennzeichen für Block-Formeln (Standard: $$). Beispiel: $$D = (t_n - t_{n-1}) - (t_{n-1} - t_{n-2})$$', 'themisdb-formula-renderer'); ?>
        </p>
        <?php
    }
    
    /**
     * Render settings page
     */
    public function render_settings_page() {
        if (!current_user_can('manage_options')) {
            return;
        }
        
        // Check if settings were saved
        if (isset($_GET['settings-updated']) && $_GET['settings-updated']) {
            add_settings_error(
                'themisdb_formula_messages',
                'themisdb_formula_message',
                __('Einstellungen gespeichert', 'themisdb-formula-renderer'),
                'updated'
            );
        }
        
        settings_errors('themisdb_formula_messages');
        ?>
        <div class="wrap">
            <h1><?php echo esc_html(get_admin_page_title()); ?></h1>
            
            <div class="themisdb-formula-admin-header">
                <p><?php _e('Dieses Plugin rendert mathematische Formeln in LaTeX-Notation automatisch mit KaTeX.', 'themisdb-formula-renderer'); ?></p>
            </div>
            
            <form action="options.php" method="post">
                <?php
                settings_fields('themisdb_formula_settings');
                do_settings_sections('themisdb-formula-renderer');
                submit_button(__('Einstellungen speichern', 'themisdb-formula-renderer'));
                ?>
            </form>
            
            <div class="themisdb-formula-examples">
                <h2><?php _e('Beispiele', 'themisdb-formula-renderer'); ?></h2>
                
                <h3><?php _e('Inline-Formel', 'themisdb-formula-renderer'); ?></h3>
                <p><?php _e('Code:', 'themisdb-formula-renderer'); ?> <code>Die Formel $E = mc^2$ ist Einsteins berühmte Gleichung.</code></p>
                <p><?php _e('Ergebnis:', 'themisdb-formula-renderer'); ?> Die Formel <span class="themisdb-formula-inline">$E = mc^2$</span> ist Einsteins berühmte Gleichung.</p>
                
                <h3><?php _e('Block-Formel', 'themisdb-formula-renderer'); ?></h3>
                <p><?php _e('Code:', 'themisdb-formula-renderer'); ?></p>
                <pre><code>$$D = (t_n - t_{n-1}) - (t_{n-1} - t_{n-2})$$</code></pre>
                <p><?php _e('Ergebnis:', 'themisdb-formula-renderer'); ?></p>
                <div class="themisdb-formula-block">$$D = (t_n - t_{n-1}) - (t_{n-1} - t_{n-2})$$</div>
                
                <h3><?php _e('Weitere Beispiele', 'themisdb-formula-renderer'); ?></h3>
                <ul>
                    <li><code>$$\int_{0}^{\infty} e^{-x^2} dx = \frac{\sqrt{\pi}}{2}$$</code></li>
                    <li><code>$$\sum_{n=1}^{\infty} \frac{1}{n^2} = \frac{\pi^2}{6}$$</code></li>
                    <li><code>$$\lim_{x \to 0} \frac{\sin x}{x} = 1$$</code></li>
                    <li><code>$$\nabla \times \vec{E} = -\frac{\partial \vec{B}}{\partial t}$$</code></li>
                </ul>
                
                <h3><?php _e('Verwendung im Content', 'themisdb-formula-renderer'); ?></h3>
                <p><?php _e('Fügen Sie einfach Formeln in Ihren Beiträgen, Seiten oder Kommentaren ein:', 'themisdb-formula-renderer'); ?></p>
                <ol>
                    <li><?php _e('Für Inline-Formeln: Umschließen Sie die Formel mit einfachen $-Zeichen: <code>$...$</code>', 'themisdb-formula-renderer'); ?></li>
                    <li><?php _e('Für Block-Formeln: Umschließen Sie die Formel mit doppelten $$-Zeichen: <code>$$...$$</code>', 'themisdb-formula-renderer'); ?></li>
                    <li><?php _e('Verwenden Sie LaTeX-Syntax für die Formeln', 'themisdb-formula-renderer'); ?></li>
                </ol>
                
                <p><strong><?php _e('Hinweis:', 'themisdb-formula-renderer'); ?></strong> <?php _e('Sie können auch den Shortcode [themisdb_formula]...[/themisdb_formula] verwenden.', 'themisdb-formula-renderer'); ?></p>
            </div>
            
            <div class="themisdb-formula-resources">
                <h2><?php _e('Ressourcen', 'themisdb-formula-renderer'); ?></h2>
                <ul>
                    <li><a href="https://katex.org/" target="_blank">KaTeX Dokumentation</a></li>
                    <li><a href="https://katex.org/docs/supported.html" target="_blank">Unterstützte LaTeX-Befehle</a></li>
                    <li><a href="https://en.wikibooks.org/wiki/LaTeX/Mathematics" target="_blank">LaTeX Mathematik Guide</a></li>
                </ul>
            </div>
        </div>
        
        <script>
        document.addEventListener('DOMContentLoaded', function() {
            if (typeof renderMathInElement !== 'undefined') {
                // Render examples on settings page
                var examples = document.querySelectorAll('.themisdb-formula-inline, .themisdb-formula-block');
                examples.forEach(function(element) {
                    renderMathInElement(element, {
                        delimiters: [
                            {left: '$$', right: '$$', display: true},
                            {left: '$', right: '$', display: false}
                        ]
                    });
                });
            }
        });
        </script>
        <?php
    }
}
