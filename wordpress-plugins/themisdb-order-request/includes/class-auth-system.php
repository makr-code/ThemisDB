<?php
/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            class-auth-system.php                              ║
  Version:         0.0.2                                              ║
  Last Modified:   2026-03-09 04:08:19                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     440                                            ║
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
 * Authentication System for ThemisDB Order Request Plugin
 * Handles license file authentication and custom login
 */

if (!defined('ABSPATH')) {
    exit;
}

class ThemisDB_Auth_System {
    
    public function __construct() {
        // Add custom login page
        add_action('init', array($this, 'register_custom_login'));
        add_shortcode('themisdb_login', array($this, 'login_form_shortcode'));
        add_shortcode('themisdb_license_upload', array($this, 'license_upload_form_shortcode'));
        
        // AJAX handlers
        add_action('wp_ajax_nopriv_themisdb_login', array($this, 'handle_login'));
        add_action('wp_ajax_nopriv_themisdb_license_auth', array($this, 'handle_license_auth'));
        add_action('wp_ajax_themisdb_license_auth', array($this, 'handle_license_auth'));
    }
    
    /**
     * Register custom login page
     */
    public function register_custom_login() {
        // Add rewrite rules for custom login
        add_rewrite_rule('^themisdb-login/?$', 'index.php?themisdb_login=1', 'top');
        add_rewrite_tag('%themisdb_login%', '1');
    }
    
    /**
     * Login form shortcode
     */
    public function login_form_shortcode($atts) {
        if (is_user_logged_in()) {
            return '<p>' . __('Sie sind bereits angemeldet.', 'themisdb-order-request') . ' <a href="' . wp_logout_url() . '">' . __('Abmelden', 'themisdb-order-request') . '</a></p>';
        }
        
        ob_start();
        ?>
        <div class="themisdb-login-form">
            <h2><?php _e('Anmeldung', 'themisdb-order-request'); ?></h2>
            
            <div class="login-tabs">
                <button class="tab-button active" data-tab="standard"><?php _e('Standard-Anmeldung', 'themisdb-order-request'); ?></button>
                <button class="tab-button" data-tab="license"><?php _e('Lizenz-Anmeldung', 'themisdb-order-request'); ?></button>
            </div>
            
            <!-- Standard Login -->
            <div class="login-tab-content active" id="standard-login">
                <form id="themisdb-standard-login-form" method="post">
                    <?php wp_nonce_field('themisdb_login', 'themisdb_login_nonce'); ?>
                    
                    <div class="form-group">
                        <label for="login_username"><?php _e('E-Mail oder Benutzername', 'themisdb-order-request'); ?></label>
                        <input type="text" id="login_username" name="username" required>
                    </div>
                    
                    <div class="form-group">
                        <label for="login_password"><?php _e('Passwort', 'themisdb-order-request'); ?></label>
                        <input type="password" id="login_password" name="password" required>
                    </div>
                    
                    <div class="form-group">
                        <label>
                            <input type="checkbox" name="remember" value="1">
                            <?php _e('Angemeldet bleiben', 'themisdb-order-request'); ?>
                        </label>
                    </div>
                    
                    <div class="form-group">
                        <button type="submit" class="button button-primary"><?php _e('Anmelden', 'themisdb-order-request'); ?></button>
                    </div>
                    
                    <div class="login-links">
                        <a href="<?php echo wp_lostpassword_url(); ?>"><?php _e('Passwort vergessen?', 'themisdb-order-request'); ?></a>
                    </div>
                </form>
            </div>
            
            <!-- License File Login -->
            <div class="login-tab-content" id="license-login">
                <form id="themisdb-license-login-form" method="post" enctype="multipart/form-data">
                    <?php wp_nonce_field('themisdb_license_auth', 'themisdb_license_auth_nonce'); ?>
                    
                    <div class="license-upload-info">
                        <p><?php _e('Melden Sie sich mit Ihrer ThemisDB-Lizenzdatei an:', 'themisdb-order-request'); ?></p>
                        <ul>
                            <li><?php _e('Laden Sie Ihre Lizenzdatei (.json) hoch', 'themisdb-order-request'); ?></li>
                            <li><?php _e('Die Datei wird sicher verifiziert', 'themisdb-order-request'); ?></li>
                            <li><?php _e('Bei erfolgreicher Verifizierung werden Sie automatisch angemeldet', 'themisdb-order-request'); ?></li>
                        </ul>
                    </div>
                    
                    <div class="form-group">
                        <label for="license_file"><?php _e('Lizenzdatei hochladen', 'themisdb-order-request'); ?></label>
                        <input type="file" id="license_file" name="license_file" accept=".json" required>
                        <small><?php _e('Nur .json Dateien erlaubt', 'themisdb-order-request'); ?></small>
                    </div>
                    
                    <div class="form-group">
                        <button type="submit" class="button button-primary"><?php _e('Mit Lizenz anmelden', 'themisdb-order-request'); ?></button>
                    </div>
                </form>
            </div>
            
            <div class="login-messages"></div>
        </div>
        
        <style>
            .themisdb-login-form {
                max-width: 500px;
                margin: 40px auto;
                padding: 30px;
                background: white;
                border-radius: 8px;
                box-shadow: 0 2px 4px rgba(0,0,0,0.1);
            }
            
            .login-tabs {
                display: flex;
                gap: 10px;
                margin-bottom: 20px;
                border-bottom: 2px solid #ddd;
            }
            
            .tab-button {
                padding: 10px 20px;
                border: none;
                background: transparent;
                cursor: pointer;
                font-size: 14px;
                border-bottom: 2px solid transparent;
                margin-bottom: -2px;
            }
            
            .tab-button.active {
                border-bottom-color: #0073aa;
                color: #0073aa;
                font-weight: bold;
            }
            
            .login-tab-content {
                display: none;
            }
            
            .login-tab-content.active {
                display: block;
            }
            
            .form-group {
                margin-bottom: 20px;
            }
            
            .form-group label {
                display: block;
                margin-bottom: 5px;
                font-weight: bold;
            }
            
            .form-group input[type="text"],
            .form-group input[type="password"],
            .form-group input[type="email"],
            .form-group input[type="file"] {
                width: 100%;
                padding: 10px;
                border: 1px solid #ddd;
                border-radius: 4px;
            }
            
            .license-upload-info {
                background: #f0f8ff;
                padding: 15px;
                border-radius: 4px;
                margin-bottom: 20px;
            }
            
            .license-upload-info ul {
                margin: 10px 0 0 20px;
            }
            
            .login-links {
                margin-top: 15px;
                text-align: center;
            }
            
            .login-messages {
                margin-top: 20px;
            }
            
            .notice {
                padding: 12px;
                border-radius: 4px;
                margin-bottom: 15px;
            }
            
            .notice-success {
                background: #d4edda;
                border: 1px solid #c3e6cb;
                color: #155724;
            }
            
            .notice-error {
                background: #f8d7da;
                border: 1px solid #f5c6cb;
                color: #721c24;
            }
        </style>
        
        <script>
        jQuery(document).ready(function($) {
            // Tab switching
            $('.tab-button').on('click', function() {
                var tab = $(this).data('tab');
                $('.tab-button').removeClass('active');
                $(this).addClass('active');
                $('.login-tab-content').removeClass('active');
                $('#' + tab + '-login').addClass('active');
            });
            
            // Standard login
            $('#themisdb-standard-login-form').on('submit', function(e) {
                e.preventDefault();
                
                var $form = $(this);
                var $button = $form.find('button[type="submit"]');
                var $messages = $('.login-messages');
                
                $button.prop('disabled', true).text('<?php _e('Anmeldung läuft...', 'themisdb-order-request'); ?>');
                $messages.empty();
                
                $.ajax({
                    url: themisdbOrder.ajaxUrl,
                    type: 'POST',
                    data: {
                        action: 'themisdb_login',
                        username: $('#login_username').val(),
                        password: $('#login_password').val(),
                        remember: $('input[name="remember"]').is(':checked') ? 1 : 0,
                        nonce: $('input[name="themisdb_login_nonce"]').val()
                    },
                    success: function(response) {
                        if (response.success) {
                            $messages.html('<div class="notice notice-success"><p>' + response.data.message + '</p></div>');
                            setTimeout(function() {
                                window.location.href = response.data.redirect || '/';
                            }, 1000);
                        } else {
                            $messages.html('<div class="notice notice-error"><p>' + response.data.message + '</p></div>');
                            $button.prop('disabled', false).text('<?php _e('Anmelden', 'themisdb-order-request'); ?>');
                        }
                    },
                    error: function() {
                        $messages.html('<div class="notice notice-error"><p><?php _e('Ein Fehler ist aufgetreten', 'themisdb-order-request'); ?></p></div>');
                        $button.prop('disabled', false).text('<?php _e('Anmelden', 'themisdb-order-request'); ?>');
                    }
                });
            });
            
            // License file login
            $('#themisdb-license-login-form').on('submit', function(e) {
                e.preventDefault();
                
                var $form = $(this);
                var $button = $form.find('button[type="submit"]');
                var $messages = $('.login-messages');
                var fileInput = document.getElementById('license_file');
                
                if (!fileInput.files.length) {
                    $messages.html('<div class="notice notice-error"><p><?php _e('Bitte wählen Sie eine Lizenzdatei aus', 'themisdb-order-request'); ?></p></div>');
                    return;
                }
                
                $button.prop('disabled', true).text('<?php _e('Lizenz wird verifiziert...', 'themisdb-order-request'); ?>');
                $messages.empty();
                
                var reader = new FileReader();
                reader.onload = function(e) {
                    var fileContent = e.target.result;
                    
                    $.ajax({
                        url: themisdbOrder.ajaxUrl,
                        type: 'POST',
                        data: {
                            action: 'themisdb_license_auth',
                            license_file_content: fileContent,
                            nonce: $('input[name="themisdb_license_auth_nonce"]').val()
                        },
                        success: function(response) {
                            if (response.success) {
                                $messages.html('<div class="notice notice-success"><p>' + response.data.message + '</p></div>');
                                setTimeout(function() {
                                    window.location.href = response.data.redirect || '/';
                                }, 1000);
                            } else {
                                $messages.html('<div class="notice notice-error"><p>' + response.data.message + '</p></div>');
                                $button.prop('disabled', false).text('<?php _e('Mit Lizenz anmelden', 'themisdb-order-request'); ?>');
                            }
                        },
                        error: function() {
                            $messages.html('<div class="notice notice-error"><p><?php _e('Ein Fehler ist aufgetreten', 'themisdb-order-request'); ?></p></div>');
                            $button.prop('disabled', false).text('<?php _e('Mit Lizenz anmelden', 'themisdb-order-request'); ?>');
                        }
                    });
                };
                
                reader.readAsText(fileInput.files[0]);
            });
        });
        </script>
        <?php
        return ob_get_clean();
    }
    
    /**
     * License upload form shortcode (alternative view)
     */
    public function license_upload_form_shortcode($atts) {
        if (is_user_logged_in()) {
            $user = wp_get_current_user();
            $license_id = get_user_meta($user->ID, 'themisdb_license_id', true);
            
            if ($license_id) {
                $license = ThemisDB_License_Manager::get_license($license_id);
                
                ob_start();
                ?>
                <div class="themisdb-license-info">
                    <h3><?php _e('Ihre Lizenz', 'themisdb-order-request'); ?></h3>
                    <p><strong><?php _e('Status', 'themisdb-order-request'); ?>:</strong> 
                        <span class="license-status-<?php echo esc_attr($license['license_status']); ?>">
                            <?php echo esc_html(ucfirst($license['license_status'])); ?>
                        </span>
                    </p>
                    <p><strong><?php _e('Edition', 'themisdb-order-request'); ?>:</strong> <?php echo esc_html(ucfirst($license['product_edition'])); ?></p>
                    <p><strong><?php _e('Lizenzschlüssel', 'themisdb-order-request'); ?>:</strong> <code><?php echo esc_html(substr($license['license_key'], 0, 20)); ?>...</code></p>
                    <p><a href="<?php echo wp_logout_url(); ?>" class="button"><?php _e('Abmelden', 'themisdb-order-request'); ?></a></p>
                </div>
                <?php
                return ob_get_clean();
            }
        }
        
        return $this->login_form_shortcode($atts);
    }
    
    /**
     * Handle standard login
     */
    public function handle_login() {
        check_ajax_referer('themisdb_login', 'nonce');
        
        $username = isset($_POST['username']) ? sanitize_text_field($_POST['username']) : '';
        // Password is not sanitized to preserve all characters - wp_signon() handles it securely
        $password = isset($_POST['password']) ? $_POST['password'] : '';
        $remember = isset($_POST['remember']) ? (bool) $_POST['remember'] : false;
        
        if (empty($username) || empty($password)) {
            wp_send_json_error(array(
                'message' => __('Bitte füllen Sie alle Felder aus', 'themisdb-order-request')
            ));
        }
        
        $credentials = array(
            'user_login' => $username,
            'user_password' => $password,
            'remember' => $remember
        );
        
        $user = wp_signon($credentials, is_ssl());
        
        if (is_wp_error($user)) {
            wp_send_json_error(array(
                'message' => __('Anmeldung fehlgeschlagen. Bitte überprüfen Sie Ihre Zugangsdaten.', 'themisdb-order-request')
            ));
        }
        
        wp_send_json_success(array(
            'message' => __('Erfolgreich angemeldet!', 'themisdb-order-request'),
            'redirect' => home_url()
        ));
    }
    
    /**
     * Handle license file authentication
     */
    public function handle_license_auth() {
        check_ajax_referer('themisdb_license_auth', 'nonce');
        
        $license_file_content = isset($_POST['license_file_content']) ? $_POST['license_file_content'] : '';
        
        if (empty($license_file_content)) {
            wp_send_json_error(array(
                'message' => __('Lizenzdatei ist leer', 'themisdb-order-request')
            ));
        }
        
        // Authenticate with license file
        $result = ThemisDB_License_Manager::authenticate_with_license_file($license_file_content);
        
        if (!$result['success']) {
            wp_send_json_error(array(
                'message' => $result['error']
            ));
        }
        
        wp_send_json_success(array(
            'message' => __('Erfolgreich mit Lizenz angemeldet!', 'themisdb-order-request'),
            'redirect' => home_url()
        ));
    }
}

// Initialize authentication system
new ThemisDB_Auth_System();
