<?php
/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            class-license-portal.php                           ║
  Version:         0.0.2                                              ║
  Last Modified:   2026-03-09 04:08:19                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     447                                            ║
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
 * Customer License Portal for ThemisDB
 *
 * Provides a self-service portal that customers can embed on any WordPress
 * page via the [themisdb_license_portal] shortcode.
 *
 * Features:
 *  - Overview of all licenses associated with the logged-in customer
 *  - Download button for each license file
 *  - Expiry / renewal status with colour-coded badges
 *  - Trial start form (creates a 30-day trial COMMUNITY license)
 *
 * Usage:
 *   [themisdb_license_portal]           – full portal
 *   [themisdb_license_portal mode="download" key="THEMIS-ENT-..."]
 *                                        – single license download link
 */

if ( ! defined( 'ABSPATH' ) ) {
    exit;
}

class ThemisDB_License_Portal {

    public function __construct() {
        add_shortcode( 'themisdb_license_portal', array( $this, 'render_portal' ) );
        add_action( 'wp_enqueue_scripts', array( $this, 'enqueue_assets' ) );

        // AJAX handlers for portal actions
        add_action( 'wp_ajax_themisdb_portal_download',    array( $this, 'ajax_download_license' ) );
        add_action( 'wp_ajax_themisdb_portal_start_trial', array( $this, 'ajax_start_trial' ) );
    }

    // -------------------------------------------------------------------------
    // Assets
    // -------------------------------------------------------------------------

    public function enqueue_assets() {
        global $post;
        if ( ! is_a( $post, 'WP_Post' ) || ! has_shortcode( $post->post_content, 'themisdb_license_portal' ) ) {
            return;
        }
        wp_enqueue_style(
            'themisdb-portal',
            THEMISDB_ORDER_PLUGIN_URL . 'assets/css/license-portal.css',
            array(),
            THEMISDB_ORDER_VERSION
        );
        wp_enqueue_script(
            'themisdb-portal',
            THEMISDB_ORDER_PLUGIN_URL . 'assets/js/license-portal.js',
            array( 'jquery' ),
            THEMISDB_ORDER_VERSION,
            true
        );
        wp_localize_script( 'themisdb-portal', 'themisdbPortal', array(
            'ajaxUrl' => admin_url( 'admin-ajax.php' ),
            'nonce'   => wp_create_nonce( 'themisdb_portal_nonce' ),
            'strings' => array(
                'downloading'  => __( 'Downloading…', 'themisdb-order-request' ),
                'trial_start'  => __( 'Starting trial…', 'themisdb-order-request' ),
                'error'        => __( 'An error occurred. Please try again.', 'themisdb-order-request' ),
            ),
        ) );
    }

    // -------------------------------------------------------------------------
    // Shortcode
    // -------------------------------------------------------------------------

    public function render_portal( $atts ) {
        $atts = shortcode_atts( array(
            'mode' => 'full',
            'key'  => '',
        ), $atts, 'themisdb_license_portal' );

        if ( ! is_user_logged_in() ) {
            return $this->render_login_notice();
        }

        $customer_id = $this->get_current_customer_id();

        if ( $atts['mode'] === 'download' && ! empty( $atts['key'] ) ) {
            return $this->render_single_download( $atts['key'], $customer_id );
        }

        return $this->render_full_portal( $customer_id );
    }

    // -------------------------------------------------------------------------
    // Render helpers
    // -------------------------------------------------------------------------

    private function render_login_notice() {
        return '<div class="themisdb-portal-notice">'
            . '<p>' . esc_html__( 'Please log in to access your licenses.', 'themisdb-order-request' ) . '</p>'
            . '<a href="' . esc_url( wp_login_url( get_permalink() ) ) . '" class="button">'
            . esc_html__( 'Log in', 'themisdb-order-request' )
            . '</a></div>';
    }

    private function render_full_portal( $customer_id ) {
        $licenses = $customer_id
            ? ThemisDB_License_Manager::get_customer_licenses( $customer_id )
            : array();

        ob_start();
        ?>
        <div class="themisdb-license-portal">
            <h2><?php esc_html_e( 'My Licenses', 'themisdb-order-request' ); ?></h2>

            <?php if ( empty( $licenses ) ) : ?>
                <div class="themisdb-portal-notice">
                    <p><?php esc_html_e( 'No licenses found for your account.', 'themisdb-order-request' ); ?></p>
                    <?php echo $this->render_trial_form(); ?>
                </div>
            <?php else : ?>
                <table class="themisdb-licenses-table">
                    <thead>
                        <tr>
                            <th><?php esc_html_e( 'License Key', 'themisdb-order-request' ); ?></th>
                            <th><?php esc_html_e( 'Edition', 'themisdb-order-request' ); ?></th>
                            <th><?php esc_html_e( 'Status', 'themisdb-order-request' ); ?></th>
                            <th><?php esc_html_e( 'Expires', 'themisdb-order-request' ); ?></th>
                            <th><?php esc_html_e( 'Actions', 'themisdb-order-request' ); ?></th>
                        </tr>
                    </thead>
                    <tbody>
                        <?php foreach ( $licenses as $license ) : ?>
                            <tr>
                                <td><code><?php echo esc_html( $license['license_key'] ); ?></code></td>
                                <td><?php echo esc_html( strtoupper( $license['product_edition'] ) ); ?></td>
                                <td><?php echo $this->render_status_badge( $license ); ?></td>
                                <td><?php echo $this->render_expiry( $license ); ?></td>
                                <td><?php echo $this->render_actions( $license ); ?></td>
                            </tr>
                        <?php endforeach; ?>
                    </tbody>
                </table>

                <div class="themisdb-portal-footer">
                    <p><?php esc_html_e( 'Need a trial license?', 'themisdb-order-request' ); ?></p>
                    <?php echo $this->render_trial_form(); ?>
                </div>
            <?php endif; ?>

            <div class="themisdb-portal-messages" style="display:none;"></div>
        </div>
        <?php
        return ob_get_clean();
    }

    private function render_single_download( $license_key, $customer_id ) {
        $license = ThemisDB_License_Manager::get_license_by_key( $license_key );

        if ( ! $license ) {
            return '<p class="themisdb-error">' . esc_html__( 'License not found.', 'themisdb-order-request' ) . '</p>';
        }

        // Verify ownership
        if ( $customer_id && (int) $license['customer_id'] !== (int) $customer_id ) {
            return '<p class="themisdb-error">' . esc_html__( 'Access denied.', 'themisdb-order-request' ) . '</p>';
        }

        return '<div class="themisdb-single-download">'
            . '<p>' . esc_html__( 'License Key: ', 'themisdb-order-request' )
            . '<code>' . esc_html( $license_key ) . '</code></p>'
            . '<button class="themisdb-download-btn button button-primary" data-key="' . esc_attr( $license_key ) . '">'
            . esc_html__( 'Download License File', 'themisdb-order-request' )
            . '</button>'
            . '<noscript><p>' . esc_html__( 'JavaScript required for download.', 'themisdb-order-request' ) . '</p></noscript>'
            . '</div>';
    }

    private function render_status_badge( array $license ) {
        $status = $license['license_status'] ?? 'unknown';
        $classes = array(
            'active'    => 'themisdb-badge-green',
            'expired'   => 'themisdb-badge-red',
            'suspended' => 'themisdb-badge-orange',
            'pending'   => 'themisdb-badge-yellow',
            'cancelled' => 'themisdb-badge-gray',
        );
        $css = isset( $classes[ $status ] ) ? $classes[ $status ] : 'themisdb-badge-gray';
        return '<span class="themisdb-badge ' . esc_attr( $css ) . '">' . esc_html( ucfirst( $status ) ) . '</span>';
    }

    private function render_expiry( array $license ) {
        if ( empty( $license['expiry_date'] ) ) {
            return '<em>' . esc_html__( 'Perpetual', 'themisdb-order-request' ) . '</em>';
        }
        $ts   = strtotime( $license['expiry_date'] );
        $days = (int) floor( ( $ts - time() ) / DAY_IN_SECONDS );

        $date_str = esc_html( date_i18n( get_option( 'date_format' ), $ts ) );
        if ( $days < 0 ) {
            return '<span class="themisdb-expired">' . $date_str . ' ('
                . esc_html__( 'expired', 'themisdb-order-request' ) . ')</span>';
        }
        if ( $days <= 30 ) {
            return '<span class="themisdb-expiring-soon">' . $date_str . ' ('
                . sprintf( esc_html__( '%d days', 'themisdb-order-request' ), $days ) . ')</span>';
        }
        return $date_str;
    }

    private function render_actions( array $license ) {
        $key = esc_attr( $license['license_key'] );
        $out = '';

        // Download button
        $out .= '<button class="themisdb-download-btn button" data-key="' . $key . '">'
            . esc_html__( 'Download', 'themisdb-order-request' )
            . '</button> ';

        // Renewal link (opens contact form / mailto)
        if ( in_array( $license['license_status'], array( 'active', 'expired' ), true ) ) {
            $renew_url = apply_filters(
                'themisdb_renewal_url',
                admin_url( 'admin.php?page=themisdb-orders&action=renew&license=' . urlencode( $license['license_key'] ) ),
                $license
            );
            $out .= '<a class="button" href="' . esc_url( $renew_url ) . '">'
                . esc_html__( 'Renew', 'themisdb-order-request' )
                . '</a>';
        }

        return $out;
    }

    private function render_trial_form() {
        ob_start();
        ?>
        <form id="themisdb-trial-form" class="themisdb-trial-form">
            <?php wp_nonce_field( 'themisdb_portal_nonce', 'themisdb_portal_nonce_field' ); ?>
            <input type="hidden" name="action" value="themisdb_portal_start_trial">
            <button type="submit" class="button button-secondary">
                <?php esc_html_e( 'Start 30-Day Free Trial', 'themisdb-order-request' ); ?>
            </button>
        </form>
        <?php
        return ob_get_clean();
    }

    // -------------------------------------------------------------------------
    // AJAX handlers
    // -------------------------------------------------------------------------

    /**
     * Return the license file JSON for download by the logged-in customer.
     */
    public function ajax_download_license() {
        check_ajax_referer( 'themisdb_portal_nonce', 'nonce' );

        if ( ! is_user_logged_in() ) {
            wp_send_json_error( array( 'message' => __( 'Not logged in.', 'themisdb-order-request' ) ), 401 );
        }

        $license_key = isset( $_POST['license_key'] ) ? sanitize_text_field( wp_unslash( $_POST['license_key'] ) ) : '';
        if ( empty( $license_key ) ) {
            wp_send_json_error( array( 'message' => __( 'Missing license key.', 'themisdb-order-request' ) ), 400 );
        }

        $license = ThemisDB_License_Manager::get_license_by_key( $license_key );
        if ( ! $license ) {
            wp_send_json_error( array( 'message' => __( 'License not found.', 'themisdb-order-request' ) ), 404 );
        }

        // Ownership check
        $customer_id = $this->get_current_customer_id();
        if ( $customer_id && (int) $license['customer_id'] !== (int) $customer_id ) {
            wp_send_json_error( array( 'message' => __( 'Access denied.', 'themisdb-order-request' ) ), 403 );
        }

        $file_data = $license['license_file_data'] ?? null;
        if ( empty( $file_data ) ) {
            wp_send_json_error( array( 'message' => __( 'License file not available.', 'themisdb-order-request' ) ), 404 );
        }

        wp_send_json_success( array(
            'filename' => 'themis-license-' . sanitize_file_name( $license_key ) . '.json',
            'content'  => is_array( $file_data ) ? wp_json_encode( $file_data ) : $file_data,
        ) );
    }

    /**
     * Create a 30-day COMMUNITY trial license for the logged-in user.
     */
    public function ajax_start_trial() {
        check_ajax_referer( 'themisdb_portal_nonce', 'nonce' );

        if ( ! is_user_logged_in() ) {
            wp_send_json_error( array( 'message' => __( 'Not logged in.', 'themisdb-order-request' ) ), 401 );
        }

        $customer_id = $this->get_current_customer_id();

        // Prevent multiple trials
        if ( $customer_id ) {
            $existing = ThemisDB_License_Manager::get_customer_licenses( $customer_id );
            foreach ( $existing as $lic ) {
                if ( $lic['license_type'] === 'trial' ) {
                    wp_send_json_error( array(
                        'message' => __( 'You already have a trial license.', 'themisdb-order-request' ),
                    ), 409 );
                }
            }
        }

        $user     = wp_get_current_user();
        $expiry   = gmdate( 'Y-m-d', strtotime( '+30 days' ) );

        // We need an order_id and contract_id; create minimal placeholder records
        $order_id    = $this->get_or_create_trial_order( $user );
        $contract_id = $this->get_or_create_trial_contract( $order_id );

        if ( ! $order_id || ! $contract_id ) {
            wp_send_json_error( array( 'message' => __( 'Failed to create trial records.', 'themisdb-order-request' ) ), 500 );
        }

        $license_id = ThemisDB_License_Manager::create_license( array(
            'order_id'        => $order_id,
            'contract_id'     => $contract_id,
            'customer_id'     => $customer_id ?? get_current_user_id(),
            'product_edition' => 'community',
            'license_type'    => 'trial',
            'expiry_date'     => $expiry,
            'max_nodes'       => 1,
            'max_cores'       => -1,
            'max_storage_gb'  => -1,
        ) );

        if ( ! $license_id ) {
            wp_send_json_error( array( 'message' => __( 'Failed to create trial license.', 'themisdb-order-request' ) ), 500 );
        }

        ThemisDB_License_Manager::activate_license( $license_id );
        $license = ThemisDB_License_Manager::get_license( $license_id );

        wp_send_json_success( array(
            'message'     => __( 'Trial license created. Valid for 30 days.', 'themisdb-order-request' ),
            'license_key' => $license['license_key'],
            'expiry_date' => $expiry,
        ) );
    }

    // -------------------------------------------------------------------------
    // Private helpers
    // -------------------------------------------------------------------------

    /**
     * Return the ThemisDB customer ID for the currently logged-in WP user.
     * Returns null if none is found.
     */
    private function get_current_customer_id() {
        $user_id = get_current_user_id();
        if ( ! $user_id ) {
            return null;
        }
        $cid = get_user_meta( $user_id, 'themisdb_customer_id', true );
        return $cid ? (int) $cid : null;
    }

    /**
     * Get or create a minimal order record for a trial.
     *
     * @param WP_User $user
     * @return int|false  Order ID on success, false on failure.
     */
    private function get_or_create_trial_order( WP_User $user ) {
        global $wpdb;
        $table = $wpdb->prefix . 'themisdb_orders';

        // Check for existing trial order
        $existing = $wpdb->get_var( $wpdb->prepare(
            "SELECT id FROM $table WHERE customer_email = %s AND notes LIKE %s LIMIT 1",
            $user->user_email,
            '%trial%'
        ) );
        if ( $existing ) {
            return (int) $existing;
        }

        $result = $wpdb->insert( $table, array(
            'customer_name'    => $user->display_name,
            'customer_email'   => $user->user_email,
            'customer_company' => get_user_meta( $user->ID, 'company', true ) ?: 'Trial',
            'product_edition'  => 'community',
            'notes'            => 'Automatically created trial order',
            'status'           => 'active',
            'created_at'       => current_time( 'mysql', true ),
        ) );

        return $result ? (int) $wpdb->insert_id : false;
    }

    /**
     * Get or create a minimal contract record for a trial.
     *
     * @param int $order_id
     * @return int|false  Contract ID on success, false on failure.
     */
    private function get_or_create_trial_contract( $order_id ) {
        global $wpdb;
        $table = $wpdb->prefix . 'themisdb_contracts';

        $existing = $wpdb->get_var( $wpdb->prepare(
            "SELECT id FROM $table WHERE order_id = %d LIMIT 1",
            $order_id
        ) );
        if ( $existing ) {
            return (int) $existing;
        }

        $result = $wpdb->insert( $table, array(
            'order_id'   => $order_id,
            'status'     => 'active',
            'notes'      => 'Trial contract',
            'created_at' => current_time( 'mysql', true ),
        ) );

        return $result ? (int) $wpdb->insert_id : false;
    }
}
