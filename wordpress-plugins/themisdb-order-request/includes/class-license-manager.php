<?php
/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            class-license-manager.php                          ║
  Version:         0.0.2                                              ║
  Last Modified:   2026-03-09 04:08:19                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     774                                            ║
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
 * License Manager for ThemisDB Order Request Plugin
 * Handles license generation, validation, and authentication
 */

if (!defined('ABSPATH')) {
    exit;
}

class ThemisDB_License_Manager {
    
    /**
     * Create a new license
     */
    public static function create_license($data) {
        global $wpdb;
        
        $table_licenses = $wpdb->prefix . 'themisdb_licenses';
        
        // Generate license key (matching ThemisDB format)
        $license_key = self::generate_license_key($data['product_edition']);
        
        // Set tier-specific limits based on ThemisDB source code
        // From epServer/models/__init__.py: max_nodes, max_cores, max_storage_tb
        // -1 = unlimited (like in ThemisDB)
        $tier_limits = array(
            'community' => array(
                'max_nodes' => 1,
                'max_cores' => -1,  // unlimited for single node
                'max_storage_tb' => -1  // unlimited
            ),
            'enterprise' => array(
                'max_nodes' => 100,  // typical enterprise cluster
                'max_cores' => -1,  // unlimited
                'max_storage_tb' => -1  // unlimited
            ),
            'hyperscaler' => array(
                'max_nodes' => -1,  // unlimited nodes
                'max_cores' => -1,  // unlimited cores
                'max_storage_tb' => -1  // unlimited storage
            ),
            'reseller' => array(
                'max_nodes' => -1,  // unlimited for reseller
                'max_cores' => -1,
                'max_storage_tb' => -1
            )
        );
        
        $edition = strtolower($data['product_edition']);
        $limits = isset($tier_limits[$edition]) ? $tier_limits[$edition] : $tier_limits['community'];
        
        $license_data = array(
            'license_key' => $license_key,
            'order_id' => intval($data['order_id']),
            'contract_id' => intval($data['contract_id']),
            'customer_id' => intval($data['customer_id']),
            'product_edition' => sanitize_text_field($data['product_edition']),
            'license_type' => isset($data['license_type']) ? sanitize_text_field($data['license_type']) : 'standard',
            'max_nodes' => isset($data['max_nodes']) ? intval($data['max_nodes']) : $limits['max_nodes'],
            'max_cores' => isset($data['max_cores']) ? intval($data['max_cores']) : $limits['max_cores'],
            'max_storage_gb' => isset($data['max_storage_gb']) ? intval($data['max_storage_gb']) : ($limits['max_storage_tb'] * 1024),  // Convert TB to GB
            'license_status' => 'pending',
            'expiry_date' => isset($data['expiry_date']) ? $data['expiry_date'] : null,
            'epserver_subscription_id' => isset($data['epserver_subscription_id']) ? sanitize_text_field($data['epserver_subscription_id']) : null
        );
        
        $result = $wpdb->insert($table_licenses, $license_data);
        
        if ($result) {
            $license_id = $wpdb->insert_id;
            
            // Generate license file
            self::generate_license_file($license_id);
            
            return $license_id;
        }
        
        return false;
    }
    
    /**
     * Activate license
     */
    public static function activate_license($license_id) {
        global $wpdb;
        
        $table_licenses = $wpdb->prefix . 'themisdb_licenses';
        
        $update_data = array(
            'license_status' => 'active',
            'activation_date' => current_time('mysql')
        );
        
        return $wpdb->update(
            $table_licenses,
            $update_data,
            array('id' => $license_id),
            null,
            array('%d')
        ) !== false;
    }
    
    /**
     * Suspend license
     */
    public static function suspend_license($license_id, $reason = '') {
        global $wpdb;
        
        $table_licenses = $wpdb->prefix . 'themisdb_licenses';
        
        $license = self::get_license($license_id);
        if (!$license) {
            return false;
        }
        
        $usage_data = $license['usage_data'] ? json_decode($license['usage_data'], true) : array();
        $usage_data['suspension_reason'] = $reason;
        $usage_data['suspended_at'] = current_time('mysql');
        
        $update_data = array(
            'license_status' => 'suspended',
            'usage_data' => json_encode($usage_data)
        );
        
        return $wpdb->update(
            $table_licenses,
            $update_data,
            array('id' => $license_id),
            null,
            array('%d')
        ) !== false;
    }
    
    /**
     * Validate license
     */
    public static function validate_license($license_key) {
        global $wpdb;
        
        // First validate format (matching ThemisDB's validate_license_key_format)
        if (!self::validate_license_key_format($license_key)) {
            return array(
                'valid' => false,
                'status' => 'invalid_format',
                'error' => 'License key format is invalid'
            );
        }
        
        $table_licenses = $wpdb->prefix . 'themisdb_licenses';
        
        $license = $wpdb->get_row($wpdb->prepare(
            "SELECT * FROM $table_licenses WHERE license_key = %s",
            $license_key
        ), ARRAY_A);
        
        if (!$license) {
            return array(
                'valid' => false,
                'status' => 'not_found',
                'error' => 'License key not found'
            );
        }
        
        // Check license status (matching ThemisDB statuses)
        $status_map = array(
            'cancelled' => 'cancelled',
            'suspended' => 'suspended',
            'pending' => 'pending_payment'
        );
        
        if (isset($status_map[$license['license_status']])) {
            $error_messages = array(
                'cancelled' => 'License has been cancelled',
                'suspended' => 'License has been suspended',
                'pending' => 'License is pending payment activation'
            );
            
            return array(
                'valid' => false,
                'status' => $status_map[$license['license_status']],
                'error' => $error_messages[$license['license_status']],
                'tier' => $license['product_edition']
            );
        }
        
        // Check expiry date
        if ($license['expiry_date']) {
            $expiry = strtotime($license['expiry_date']);
            $now = time();
            
            if ($expiry < $now) {
                // Auto-update status to expired
                if ($license['license_status'] === 'active') {
                    $wpdb->update(
                        $table_licenses,
                        array('license_status' => 'expired'),
                        array('id' => $license['id']),
                        null,
                        array('%d')
                    );
                }
                
                return array(
                    'valid' => false,
                    'status' => 'expired',
                    'error' => 'License expired on ' . date('Y-m-d', $expiry),
                    'tier' => $license['product_edition'],
                    'expiry_date' => date('c', $expiry)
                );
            }
        }
        
        // Update last check time
        $wpdb->update(
            $table_licenses,
            array('last_check' => current_time('mysql')),
            array('id' => $license['id']),
            null,
            array('%d')
        );
        
        // Build limits (matching ThemisDB response format)
        $limits = array(
            'max_nodes' => $license['max_nodes'] ? intval($license['max_nodes']) : -1,
            'max_cores' => $license['max_cores'] ? intval($license['max_cores']) : -1,
            'max_storage_tb' => $license['max_storage_gb'] ? floatval($license['max_storage_gb']) / 1024 : -1
        );
        
        // Calculate days remaining
        $days_remaining = null;
        if ($license['expiry_date']) {
            $expiry_timestamp = strtotime($license['expiry_date']);
            $days_remaining = floor(($expiry_timestamp - time()) / 86400);
        }
        
        // Get order for organization info
        $order = ThemisDB_Order_Manager::get_order($license['order_id']);
        
        return array(
            'valid' => true,
            'status' => 'active',
            'message' => 'License is valid and active',
            'tier' => $license['product_edition'],
            'license_key' => $license_key,
            'organization' => $order ? $order['customer_company'] : null,
            'limits' => $limits,
            'start_date' => $license['activation_date'] ? date('c', strtotime($license['activation_date'])) : null,
            'end_date' => $license['expiry_date'] ? date('c', strtotime($license['expiry_date'])) : null,
            'days_remaining' => $days_remaining,
            'license' => $license
        );
    }
    
    /**
     * Validate license key format (matching ThemisDB)
     */
    private static function validate_license_key_format($license_key) {
        // Format: THEMIS-{TIER}-{HASH}-{RANDOM}
        // Example: THEMIS-ENT-A1B2C3D4-E5F6G7H8
        
        if (!is_string($license_key) || strpos($license_key, 'THEMIS-') !== 0) {
            return false;
        }
        
        $parts = explode('-', $license_key);
        if (count($parts) !== 4) {
            return false;
        }
        
        // Check tier code
        $valid_tiers = array('COM', 'ENT', 'HYP', 'RES');
        if (!in_array($parts[1], $valid_tiers)) {
            return false;
        }
        
        // Check hash length (8 chars)
        if (strlen($parts[2]) !== 8) {
            return false;
        }
        
        // Check random part length (8 chars)
        if (strlen($parts[3]) !== 8) {
            return false;
        }
        
        return true;
    }
    
    /**
     * Check license limits (matching ThemisDB check_license_limits)
     */
    public static function check_license_limits($license_key, $current_nodes, $current_cores = null, $current_storage_tb = null) {
        $validation = self::validate_license($license_key);
        
        if (!$validation['valid']) {
            return array(
                'compliant' => false,
                'reason' => $validation['error'],
                'limits_check' => null
            );
        }
        
        $limits = $validation['limits'];
        $checks = array();
        $compliant = true;
        
        // Check nodes
        if ($limits['max_nodes'] != -1) { // -1 means unlimited
            $nodes_ok = $current_nodes <= $limits['max_nodes'];
            $checks['nodes'] = array(
                'limit' => $limits['max_nodes'],
                'current' => $current_nodes,
                'compliant' => $nodes_ok
            );
            if (!$nodes_ok) {
                $compliant = false;
            }
        } else {
            $checks['nodes'] = array(
                'limit' => 'unlimited',
                'current' => $current_nodes,
                'compliant' => true
            );
        }
        
        // Check cores
        if ($current_cores !== null) {
            if ($limits['max_cores'] != -1) {
                $cores_ok = $current_cores <= $limits['max_cores'];
                $checks['cores'] = array(
                    'limit' => $limits['max_cores'],
                    'current' => $current_cores,
                    'compliant' => $cores_ok
                );
                if (!$cores_ok) {
                    $compliant = false;
                }
            } else {
                $checks['cores'] = array(
                    'limit' => 'unlimited',
                    'current' => $current_cores,
                    'compliant' => true
                );
            }
        }
        
        // Check storage
        if ($current_storage_tb !== null) {
            if ($limits['max_storage_tb'] != -1) {
                $storage_ok = $current_storage_tb <= $limits['max_storage_tb'];
                $checks['storage_tb'] = array(
                    'limit' => $limits['max_storage_tb'],
                    'current' => $current_storage_tb,
                    'compliant' => $storage_ok
                );
                if (!$storage_ok) {
                    $compliant = false;
                }
            } else {
                $checks['storage_tb'] = array(
                    'limit' => 'unlimited',
                    'current' => $current_storage_tb,
                    'compliant' => true
                );
            }
        }
        
        return array(
            'compliant' => $compliant,
            'limits_check' => $checks,
            'tier' => $validation['tier']
        );
    }
    
    /**
     * Authenticate via license file
     */
    public static function authenticate_with_license_file($file_content) {
        // Parse license file
        $license_data = self::parse_license_file($file_content);
        
        if (!$license_data || !isset($license_data['license_key'])) {
            return array(
                'success' => false,
                'error' => 'Invalid license file format'
            );
        }
        
        // Validate license
        $validation = self::validate_license($license_data['license_key']);
        
        if (!$validation['valid']) {
            return array(
                'success' => false,
                'error' => $validation['error']
            );
        }
        
        $license = $validation['license'];
        
        // Log authentication attempt
        self::log_auth_attempt($license['id'], 'license_file', 'success', $license_data);
        
        // Create or login WordPress user
        $user = self::get_or_create_user_for_license($license);
        
        if ($user) {
            // Log user in
            wp_set_current_user($user->ID);
            wp_set_auth_cookie($user->ID);
            
            return array(
                'success' => true,
                'user_id' => $user->ID,
                'license' => $license
            );
        }
        
        return array(
            'success' => false,
            'error' => 'Failed to authenticate user'
        );
    }
    
    /**
     * Get license by ID
     */
    public static function get_license($license_id) {
        global $wpdb;
        
        $table_licenses = $wpdb->prefix . 'themisdb_licenses';
        
        $license = $wpdb->get_row($wpdb->prepare(
            "SELECT * FROM $table_licenses WHERE id = %d",
            $license_id
        ), ARRAY_A);
        
        if ($license && $license['usage_data']) {
            $license['usage_data'] = json_decode($license['usage_data'], true);
        }
        
        if ($license && $license['license_file_data']) {
            $license['license_file_data'] = json_decode($license['license_file_data'], true);
        }
        
        return $license;
    }
    
    /**
     * Get license by key
     */
    public static function get_license_by_key($license_key) {
        global $wpdb;
        
        $table_licenses = $wpdb->prefix . 'themisdb_licenses';
        
        $license = $wpdb->get_row($wpdb->prepare(
            "SELECT * FROM $table_licenses WHERE license_key = %s",
            $license_key
        ), ARRAY_A);
        
        if ($license && $license['usage_data']) {
            $license['usage_data'] = json_decode($license['usage_data'], true);
        }
        
        if ($license && $license['license_file_data']) {
            $license['license_file_data'] = json_decode($license['license_file_data'], true);
        }
        
        return $license;
    }
    
    /**
     * Get license by contract
     */
    public static function get_license_by_contract($contract_id) {
        global $wpdb;
        
        $table_licenses = $wpdb->prefix . 'themisdb_licenses';
        
        $license = $wpdb->get_row($wpdb->prepare(
            "SELECT * FROM $table_licenses WHERE contract_id = %d",
            $contract_id
        ), ARRAY_A);
        
        if ($license && $license['usage_data']) {
            $license['usage_data'] = json_decode($license['usage_data'], true);
        }
        
        return $license;
    }
    
    /**
     * Get licenses by customer
     */
    public static function get_customer_licenses($customer_id) {
        global $wpdb;
        
        $table_licenses = $wpdb->prefix . 'themisdb_licenses';
        
        $licenses = $wpdb->get_results($wpdb->prepare(
            "SELECT * FROM $table_licenses WHERE customer_id = %d ORDER BY created_at DESC",
            $customer_id
        ), ARRAY_A);
        
        foreach ($licenses as &$license) {
            if ($license['usage_data']) {
                $license['usage_data'] = json_decode($license['usage_data'], true);
            }
        }
        
        return $licenses;
    }
    
    /**
     * Generate license key
     */
    private static function generate_license_key($edition) {
        // Match ThemisDB source code format: THEMIS-{TIER}-{HASH}-{RANDOM}
        // From epServer/utils/license.py
        
        // Map edition to tier code (matching ThemisDB)
        $tier_codes = array(
            'community' => 'COM',
            'enterprise' => 'ENT',
            'hyperscaler' => 'HYP',
            'reseller' => 'RES'
        );
        
        $tier_code = isset($tier_codes[strtolower($edition)]) ? $tier_codes[strtolower($edition)] : 'UNK';
        
        // Create hash component (8 chars) - using customer data + timestamp like ThemisDB
        $timestamp = gmdate('c'); // ISO 8601 format
        $data = get_current_user_id() . ':' . $edition . ':' . $timestamp;
        $hash_digest = strtoupper(substr(hash('sha256', $data), 0, 8));
        
        // Generate random component (8 chars) - matching ThemisDB's secrets.token_hex(4)
        $random_part = strtoupper(bin2hex(random_bytes(4)));
        
        // Format: THEMIS-{TIER}-{HASH}-{RANDOM}
        // Example: THEMIS-ENT-A1B2C3D4-E5F6G7H8
        return "THEMIS-{$tier_code}-{$hash_digest}-{$random_part}";
    }
    
    /**
     * Generate license file
     */
    private static function generate_license_file($license_id) {
        $license = self::get_license($license_id);
        
        if (!$license) {
            return false;
        }
        
        // Get order and contract details
        $order = ThemisDB_Order_Manager::get_order($license['order_id']);
        $contract = ThemisDB_Contract_Manager::get_contract($license['contract_id']);
        
        $license_file_data = array(
            'version' => '1.0',
            'license_key' => $license['license_key'],
            'product_edition' => $license['product_edition'],
            'license_type' => $license['license_type'],
            'customer_name' => $order['customer_name'],
            'customer_email' => $order['customer_email'],
            'customer_company' => $order['customer_company'],
            'max_nodes' => $license['max_nodes'],
            'max_cores' => $license['max_cores'],
            'max_storage_gb' => $license['max_storage_gb'],
            'issued_date' => $license['created_at'],
            'activation_date' => $license['activation_date'],
            'expiry_date' => $license['expiry_date'],
            'signature' => self::sign_license_data($license)
        );
        
        // Store license file data
        global $wpdb;
        $table_licenses = $wpdb->prefix . 'themisdb_licenses';
        
        $wpdb->update(
            $table_licenses,
            array('license_file_data' => json_encode($license_file_data)),
            array('id' => $license_id),
            null,
            array('%d')
        );
        
        return $license_file_data;
    }
    
    /**
     * Parse license file
     */
    private static function parse_license_file($file_content) {
        // License file is JSON format
        $data = json_decode($file_content, true);
        
        if (!$data || !isset($data['license_key'])) {
            return false;
        }
        
        // Verify signature
        if (!self::verify_license_signature($data)) {
            return false;
        }
        
        return $data;
    }
    
    /**
     * Sign license data
     */
    private static function sign_license_data($license) {
        $data_to_sign = $license['license_key'] . 
                       $license['product_edition'] . 
                       $license['customer_id'] . 
                       $license['created_at'];
        
        return hash_hmac('sha256', $data_to_sign, wp_salt('auth'));
    }
    
    /**
     * Verify license signature
     */
    private static function verify_license_signature($license_data) {
        if (!isset($license_data['signature'])) {
            return false;
        }
        
        $license = self::get_license_by_key($license_data['license_key']);
        
        if (!$license) {
            return false;
        }
        
        $expected_signature = self::sign_license_data($license);
        
        return hash_equals($expected_signature, $license_data['signature']);
    }
    
    /**
     * Get or create user for license
     */
    private static function get_or_create_user_for_license($license) {
        // Get order to get customer email
        $order = ThemisDB_Order_Manager::get_order($license['order_id']);
        
        if (!$order) {
            return false;
        }
        
        // Check if user exists
        $user = get_user_by('email', $order['customer_email']);
        
        if (!$user) {
            // Create new user
            $username = sanitize_user($order['customer_email']);
            $password = wp_generate_password(16, true, true);
            
            $user_id = wp_create_user($username, $password, $order['customer_email']);
            
            if (is_wp_error($user_id)) {
                return false;
            }
            
            $user = get_user_by('id', $user_id);
            
            // Update user meta
            update_user_meta($user_id, 'first_name', $order['customer_name']);
            update_user_meta($user_id, 'company', $order['customer_company']);
            update_user_meta($user_id, 'themisdb_license_id', $license['id']);
            update_user_meta($user_id, 'themisdb_customer_id', $license['customer_id']);
        }
        
        return $user;
    }
    
    /**
     * Log authentication attempt
     */
    private static function log_auth_attempt($license_id, $method, $status, $auth_data = null) {
        global $wpdb;
        
        $table_auth_log = $wpdb->prefix . 'themisdb_license_auth_log';
        
        $log_data = array(
            'license_id' => $license_id,
            'auth_method' => $method,
            'auth_status' => $status,
            'ip_address' => $_SERVER['REMOTE_ADDR'] ?? null,
            'user_agent' => $_SERVER['HTTP_USER_AGENT'] ?? null,
            'auth_data' => $auth_data ? json_encode($auth_data) : null
        );
        
        return $wpdb->insert($table_auth_log, $log_data);
    }
    
    /**
     * Check license with epServer
     */
    public static function check_license_with_epserver($license_key) {
        return ThemisDB_EPServer_API::validate_license($license_key);
    }
    
    /**
     * Get license statistics
     */
    public static function get_license_stats() {
        global $wpdb;
        
        $table_licenses = $wpdb->prefix . 'themisdb_licenses';
        
        $stats = array(
            'total_licenses' => 0,
            'active_licenses' => 0,
            'pending_licenses' => 0,
            'suspended_licenses' => 0,
            'expired_licenses' => 0
        );
        
        $results = $wpdb->get_results(
            "SELECT 
                license_status,
                COUNT(*) as count
            FROM $table_licenses
            GROUP BY license_status",
            ARRAY_A
        );
        
        foreach ($results as $row) {
            $stats['total_licenses'] += $row['count'];
            
            switch ($row['license_status']) {
                case 'active':
                    $stats['active_licenses'] = $row['count'];
                    break;
                case 'pending':
                    $stats['pending_licenses'] = $row['count'];
                    break;
                case 'suspended':
                    $stats['suspended_licenses'] = $row['count'];
                    break;
                case 'expired':
                    $stats['expired_licenses'] = $row['count'];
                    break;
            }
        }
        
        return $stats;
    }
}
