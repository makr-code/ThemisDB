<?php
/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            class-license-api.php                              ║
  Version:         0.0.2                                              ║
  Last Modified:   2026-03-09 04:08:19                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     388                                            ║
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
 * License Provisioning REST API for ThemisDB
 *
 * Exposes the following WordPress REST endpoints:
 *
 *   POST /wp-json/themisdb/v1/license/validate
 *        Body: { "license_key": "THEMIS-ENT-...", "machine_fingerprint": "..." }
 *        Returns signed validation status for ThemisDB server consumption.
 *
 *   GET  /wp-json/themisdb/v1/license/download/{license_key}
 *        Returns the license JSON file for the given key.
 *        Requires Bearer token (API key stored in plugin settings).
 *
 *   POST /wp-json/themisdb/v1/license/renew
 *        Body: { "license_key": "...", "extend_days": 365 }
 *        Extends the expiry date of a license (admin only).
 *
 *   POST /wp-json/themisdb/v1/license/revoke
 *        Body: { "license_key": "...", "reason": "..." }
 *        Revokes (suspends) a license (admin only).
 *
 * All responses include an HMAC-SHA256 signature so the ThemisDB C++ server
 * can verify the response has not been tampered with in transit.
 *
 * Audit logging: every request (success or failure) is written to
 * {$wpdb->prefix}themisdb_license_audit_log.
 */

if ( ! defined( 'ABSPATH' ) ) {
    exit;
}

class ThemisDB_License_API {

    /** REST namespace / version */
    const NAMESPACE = 'themisdb/v1';

    public function __construct() {
        add_action( 'rest_api_init', array( $this, 'register_routes' ) );
    }

    // -------------------------------------------------------------------------
    // Route registration
    // -------------------------------------------------------------------------

    public function register_routes() {
        // Validate license (called by ThemisDB server on startup or periodically)
        register_rest_route( self::NAMESPACE, '/license/validate', array(
            'methods'             => WP_REST_Server::CREATABLE,
            'callback'            => array( $this, 'validate_license' ),
            'permission_callback' => array( $this, 'check_api_key' ),
            'args'                => array(
                'license_key'         => array( 'required' => true,  'sanitize_callback' => 'sanitize_text_field' ),
                'machine_fingerprint' => array( 'required' => false, 'sanitize_callback' => 'sanitize_text_field' ),
                'edition'             => array( 'required' => false, 'sanitize_callback' => 'sanitize_text_field' ),
            ),
        ) );

        // Download license file (called by customer / automated provisioning)
        register_rest_route( self::NAMESPACE, '/license/download/(?P<license_key>[A-Z0-9\-]+)', array(
            'methods'             => WP_REST_Server::READABLE,
            'callback'            => array( $this, 'download_license' ),
            'permission_callback' => array( $this, 'check_api_key' ),
        ) );

        // Renew license expiry (admin only)
        register_rest_route( self::NAMESPACE, '/license/renew', array(
            'methods'             => WP_REST_Server::CREATABLE,
            'callback'            => array( $this, 'renew_license' ),
            'permission_callback' => array( $this, 'check_admin_api_key' ),
            'args'                => array(
                'license_key'  => array( 'required' => true, 'sanitize_callback' => 'sanitize_text_field' ),
                'extend_days'  => array( 'required' => true, 'validate_callback' => function( $v ) { return is_numeric( $v ) && (int) $v > 0; } ),
            ),
        ) );

        // Revoke license (admin only)
        register_rest_route( self::NAMESPACE, '/license/revoke', array(
            'methods'             => WP_REST_Server::CREATABLE,
            'callback'            => array( $this, 'revoke_license' ),
            'permission_callback' => array( $this, 'check_admin_api_key' ),
            'args'                => array(
                'license_key' => array( 'required' => true,  'sanitize_callback' => 'sanitize_text_field' ),
                'reason'      => array( 'required' => false, 'sanitize_callback' => 'sanitize_text_field' ),
            ),
        ) );
    }

    // -------------------------------------------------------------------------
    // Permission callbacks
    // -------------------------------------------------------------------------

    /**
     * Verify the Bearer token matches the configured API key.
     */
    public function check_api_key( WP_REST_Request $request ) {
        $configured_key = get_option( 'themisdb_license_api_key', '' );
        if ( empty( $configured_key ) ) {
            return new WP_Error( 'rest_forbidden', 'License API key not configured.', array( 'status' => 503 ) );
        }

        $auth_header = $request->get_header( 'Authorization' );
        if ( ! $auth_header || strpos( $auth_header, 'Bearer ' ) !== 0 ) {
            return new WP_Error( 'rest_unauthorized', 'Missing or invalid Authorization header.', array( 'status' => 401 ) );
        }

        $token = substr( $auth_header, 7 );
        if ( ! hash_equals( $configured_key, $token ) ) {
            $this->audit_log( null, 'auth_failed', 'invalid_api_key', $request );
            return new WP_Error( 'rest_forbidden', 'Invalid API key.', array( 'status' => 403 ) );
        }

        return true;
    }

    /**
     * Admin-only endpoint: API key must also have admin privileges.
     */
    public function check_admin_api_key( WP_REST_Request $request ) {
        $base_check = $this->check_api_key( $request );
        if ( is_wp_error( $base_check ) ) {
            return $base_check;
        }
        // Additionally require a separate admin secret or WordPress admin session
        $admin_secret = get_option( 'themisdb_license_admin_secret', '' );
        $provided     = $request->get_header( 'X-ThemisDB-Admin-Secret' );
        if ( ! empty( $admin_secret ) && ( empty( $provided ) || ! hash_equals( $admin_secret, $provided ) ) ) {
            $this->audit_log( null, 'admin_auth_failed', 'invalid_admin_secret', $request );
            return new WP_Error( 'rest_forbidden', 'Admin secret required.', array( 'status' => 403 ) );
        }
        return true;
    }

    // -------------------------------------------------------------------------
    // Endpoint handlers
    // -------------------------------------------------------------------------

    /**
     * POST /wp-json/themisdb/v1/license/validate
     */
    public function validate_license( WP_REST_Request $request ) {
        $license_key         = $request->get_param( 'license_key' );
        $machine_fingerprint = $request->get_param( 'machine_fingerprint' ) ?? '';
        $edition             = $request->get_param( 'edition' ) ?? '';

        $result = ThemisDB_License_Manager::validate_license( $license_key );

        $status_code = $result['valid'] ? 200 : 402;

        $response_body = array(
            'valid'               => $result['valid'],
            'status'              => $result['valid'] ? 'active' : ( $result['status'] ?? 'invalid' ),
            'license_key'         => $license_key,
            'tier'                => $result['tier']   ?? null,
            'organization'        => $result['organization'] ?? null,
            'limits'              => $result['limits'] ?? null,
            'start_date'          => $result['start_date'] ?? null,
            'end_date'            => $result['end_date']   ?? null,
            'days_remaining'      => $result['days_remaining'] ?? null,
            'machine_fingerprint' => $machine_fingerprint,
            'timestamp'           => gmdate( 'c' ),
        );

        if ( ! $result['valid'] ) {
            $response_body['error'] = $result['error'] ?? 'License validation failed';
        }

        $response_body['signature'] = $this->sign_response( $response_body );

        $this->audit_log( $license_key, 'validate', $result['valid'] ? 'success' : $result['status'], $request );

        return new WP_REST_Response( $response_body, $status_code );
    }

    /**
     * GET /wp-json/themisdb/v1/license/download/{license_key}
     */
    public function download_license( WP_REST_Request $request ) {
        $license_key = $request->get_param( 'license_key' );

        $license = ThemisDB_License_Manager::get_license_by_key( $license_key );

        if ( ! $license ) {
            $this->audit_log( $license_key, 'download', 'not_found', $request );
            return new WP_Error( 'not_found', 'License not found.', array( 'status' => 404 ) );
        }

        $file_data = $license['license_file_data'] ?? null;
        if ( empty( $file_data ) ) {
            $this->audit_log( $license_key, 'download', 'no_file', $request );
            return new WP_Error( 'not_found', 'License file not yet generated.', array( 'status' => 404 ) );
        }

        $this->audit_log( $license_key, 'download', 'success', $request );

        // Return license file data as JSON with a download hint
        $response = new WP_REST_Response( $file_data, 200 );
        $response->header( 'Content-Disposition', 'attachment; filename="themis-license.json"' );
        return $response;
    }

    /**
     * POST /wp-json/themisdb/v1/license/renew
     */
    public function renew_license( WP_REST_Request $request ) {
        global $wpdb;

        $license_key  = $request->get_param( 'license_key' );
        $extend_days  = (int) $request->get_param( 'extend_days' );

        $license = ThemisDB_License_Manager::get_license_by_key( $license_key );
        if ( ! $license ) {
            $this->audit_log( $license_key, 'renew', 'not_found', $request );
            return new WP_Error( 'not_found', 'License not found.', array( 'status' => 404 ) );
        }

        // Calculate new expiry
        $current_expiry = ! empty( $license['expiry_date'] ) ? strtotime( $license['expiry_date'] ) : time();
        $new_expiry     = max( $current_expiry, time() ) + $extend_days * DAY_IN_SECONDS;
        $new_expiry_str = gmdate( 'Y-m-d', $new_expiry );

        $table = $wpdb->prefix . 'themisdb_licenses';
        $updated = $wpdb->update(
            $table,
            array(
                'expiry_date'    => $new_expiry_str,
                'license_status' => 'active',
            ),
            array( 'id' => $license['id'] ),
            array( '%s', '%s' ),
            array( '%d' )
        );

        if ( $updated === false ) {
            $this->audit_log( $license_key, 'renew', 'db_error', $request );
            return new WP_Error( 'server_error', 'Failed to update license.', array( 'status' => 500 ) );
        }

        $this->audit_log( $license_key, 'renew', 'success', $request );

        return new WP_REST_Response( array(
            'success'     => true,
            'license_key' => $license_key,
            'new_expiry'  => $new_expiry_str,
            'extend_days' => $extend_days,
            'timestamp'   => gmdate( 'c' ),
        ), 200 );
    }

    /**
     * POST /wp-json/themisdb/v1/license/revoke
     */
    public function revoke_license( WP_REST_Request $request ) {
        $license_key = $request->get_param( 'license_key' );
        $reason      = $request->get_param( 'reason' ) ?? 'Revoked via API';

        $license = ThemisDB_License_Manager::get_license_by_key( $license_key );
        if ( ! $license ) {
            $this->audit_log( $license_key, 'revoke', 'not_found', $request );
            return new WP_Error( 'not_found', 'License not found.', array( 'status' => 404 ) );
        }

        $suspended = ThemisDB_License_Manager::suspend_license( $license['id'], $reason );
        if ( ! $suspended ) {
            $this->audit_log( $license_key, 'revoke', 'db_error', $request );
            return new WP_Error( 'server_error', 'Failed to revoke license.', array( 'status' => 500 ) );
        }

        $this->audit_log( $license_key, 'revoke', 'success', $request );

        return new WP_REST_Response( array(
            'success'     => true,
            'license_key' => $license_key,
            'status'      => 'suspended',
            'reason'      => $reason,
            'timestamp'   => gmdate( 'c' ),
        ), 200 );
    }

    // -------------------------------------------------------------------------
    // Helpers
    // -------------------------------------------------------------------------

    /**
     * Sign a response array with HMAC-SHA256 using the configured API key
     * so the ThemisDB server can verify authenticity.
     *
     * @param array $data  Response body (without the 'signature' key).
     * @return string Hex-encoded HMAC-SHA256 signature.
     */
    private function sign_response( array $data ) {
        $secret = get_option( 'themisdb_license_api_key', '' );
        // Deterministic canonical string: sorted keys, JSON-encoded values
        ksort( $data );
        $canonical = json_encode( $data, JSON_UNESCAPED_UNICODE | JSON_UNESCAPED_SLASHES );
        return hash_hmac( 'sha256', $canonical, $secret );
    }

    /**
     * Write an entry to the audit log table.
     *
     * @param string|null      $license_key  The license key involved (may be null for auth failures).
     * @param string           $action       Audit action: validate | download | renew | revoke | auth_failed
     * @param string           $result       Outcome: success | not_found | expired | invalid | db_error | ...
     * @param WP_REST_Request  $request      The original request (for IP / user-agent logging).
     */
    private function audit_log( $license_key, string $action, string $result, WP_REST_Request $request ) {
        global $wpdb;

        $table = $wpdb->prefix . 'themisdb_license_audit_log';

        // Table may not exist yet; create it lazily
        $this->ensure_audit_table();

        $wpdb->insert( $table, array(
            'license_key' => $license_key,
            'action'      => $action,
            'result'      => $result,
            'ip_address'  => $this->get_client_ip(),
            'user_agent'  => substr( $request->get_header( 'User-Agent' ) ?? '', 0, 255 ),
            'created_at'  => current_time( 'mysql', true ),
        ), array( '%s', '%s', '%s', '%s', '%s', '%s' ) );
    }

    /**
     * Lazily create the audit log table if it does not exist.
     */
    private function ensure_audit_table() {
        global $wpdb;
        $table = $wpdb->prefix . 'themisdb_license_audit_log';

        if ( $wpdb->get_var( "SHOW TABLES LIKE '$table'" ) !== $table ) {
            $charset_collate = $wpdb->get_charset_collate();
            $sql = "CREATE TABLE IF NOT EXISTS $table (
                id          BIGINT(20) UNSIGNED NOT NULL AUTO_INCREMENT,
                license_key VARCHAR(64)         DEFAULT NULL,
                action      VARCHAR(32)         NOT NULL,
                result      VARCHAR(64)         NOT NULL,
                ip_address  VARCHAR(45)         DEFAULT NULL,
                user_agent  VARCHAR(255)        DEFAULT NULL,
                created_at  DATETIME            NOT NULL,
                PRIMARY KEY (id),
                KEY idx_license_key (license_key),
                KEY idx_created_at (created_at)
            ) $charset_collate;";
            require_once ABSPATH . 'wp-admin/includes/upgrade.php';
            dbDelta( $sql );
        }
    }

    /**
     * Return the client IP address, handling common proxy headers.
     */
    private function get_client_ip() {
        foreach ( array( 'HTTP_X_FORWARDED_FOR', 'HTTP_X_REAL_IP', 'REMOTE_ADDR' ) as $key ) {
            if ( ! empty( $_SERVER[ $key ] ) ) {
                $ip = trim( explode( ',', sanitize_text_field( wp_unslash( $_SERVER[ $key ] ) ) )[0] );
                if ( filter_var( $ip, FILTER_VALIDATE_IP ) ) {
                    return $ip;
                }
            }
        }
        return '';
    }
}
