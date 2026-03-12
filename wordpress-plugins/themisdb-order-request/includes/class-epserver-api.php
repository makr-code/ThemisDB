<?php
/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            class-epserver-api.php                             ║
  Version:         0.0.2                                              ║
  Last Modified:   2026-03-09 04:08:19                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     343                                            ║
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
 * epServer API Integration for ThemisDB Order Request Plugin
 * Handles master data synchronization with epServer
 */

if (!defined('ABSPATH')) {
    exit;
}

class ThemisDB_EPServer_API {
    
    private static $base_url = null;
    private static $api_key = null;
    
    /**
     * Initialize API settings
     */
    private static function init() {
        if (self::$base_url === null) {
            self::$base_url = get_option('themisdb_order_epserver_url', 'https://service.themisdb.org:6734');
            self::$api_key = get_option('themisdb_order_epserver_api_key', '');
        }
    }
    
    /**
     * Make API request
     */
    private static function make_request($endpoint, $method = 'GET', $data = null) {
        self::init();
        
        if (empty(self::$base_url)) {
            return array('error' => 'epServer URL not configured');
        }
        
        $url = rtrim(self::$base_url, '/') . '/' . ltrim($endpoint, '/');
        
        $args = array(
            'method' => $method,
            'timeout' => 30,
            'headers' => array(
                'Content-Type' => 'application/json',
            ),
        );
        
        // Add API key if configured
        if (!empty(self::$api_key)) {
            $args['headers']['Authorization'] = 'Bearer ' . self::$api_key;
        }
        
        // Add body for POST/PUT requests
        if ($data && in_array($method, array('POST', 'PUT', 'PATCH'))) {
            $args['body'] = json_encode($data);
        }
        
        $response = wp_remote_request($url, $args);
        
        if (is_wp_error($response)) {
            return array('error' => $response->get_error_message());
        }
        
        $status_code = wp_remote_retrieve_response_code($response);
        $body = wp_remote_retrieve_body($response);
        
        if ($status_code >= 200 && $status_code < 300) {
            return json_decode($body, true);
        }
        
        return array(
            'error' => 'API request failed',
            'status_code' => $status_code,
            'body' => $body
        );
    }
    
    /**
     * Sync products from epServer
     */
    public static function sync_products() {
        $response = self::make_request('/pricing');
        
        if (isset($response['error'])) {
            return false;
        }
        
        global $wpdb;
        $table_products = $wpdb->prefix . 'themisdb_products';
        
        // Map pricing tiers to products
        $pricing_map = array(
            'Community' => array(
                'product_code' => 'THEMIS-COMMUNITY',
                'product_name' => 'ThemisDB Community Edition',
                'product_type' => 'database',
                'edition' => 'community',
                'description' => 'Kostenlose Single-Node Edition'
            ),
            'Enterprise' => array(
                'product_code' => 'THEMIS-ENTERPRISE',
                'product_name' => 'ThemisDB Enterprise Edition',
                'product_type' => 'database',
                'edition' => 'enterprise',
                'description' => 'Enterprise Edition bis 100 Nodes'
            ),
            'Hyperscaler' => array(
                'product_code' => 'THEMIS-HYPERSCALER',
                'product_name' => 'ThemisDB Hyperscaler Edition',
                'product_type' => 'database',
                'edition' => 'hyperscaler',
                'description' => 'Hyperscaler Edition unbegrenzte Nodes'
            )
        );
        
        foreach ($response as $tier => $info) {
            if (isset($pricing_map[$tier])) {
                $product_data = $pricing_map[$tier];
                $product_data['price'] = floatval($info['price']);
                $product_data['currency'] = 'EUR';
                $product_data['metadata'] = json_encode($info);
                
                // Check if product exists
                $existing = $wpdb->get_var($wpdb->prepare(
                    "SELECT id FROM $table_products WHERE product_code = %s",
                    $product_data['product_code']
                ));
                
                if ($existing) {
                    // Update existing
                    $wpdb->update(
                        $table_products,
                        $product_data,
                        array('product_code' => $product_data['product_code'])
                    );
                } else {
                    // Insert new
                    $wpdb->insert($table_products, $product_data);
                }
            }
        }
        
        return true;
    }
    
    /**
     * Create customer in epServer
     */
    public static function create_customer($customer_data) {
        $data = array(
            'email' => $customer_data['customer_email'],
            'password' => wp_generate_password(16, true, true), // Generate random password
            'organization_name' => isset($customer_data['customer_company']) ? $customer_data['customer_company'] : '',
            'contact_name' => $customer_data['customer_name'],
            'country' => isset($customer_data['country']) ? $customer_data['country'] : 'Germany'
        );
        
        $response = self::make_request('/auth/register', 'POST', $data);
        
        if (isset($response['error'])) {
            return false;
        }
        
        return $response;
    }
    
    /**
     * Create subscription in epServer
     */
    public static function create_subscription($order) {
        // Map edition to tier
        $tier_map = array(
            'community' => 'community',
            'enterprise' => 'enterprise',
            'hyperscaler' => 'hyperscaler'
        );
        
        $tier = isset($tier_map[$order['product_edition']]) ? $tier_map[$order['product_edition']] : 'community';
        
        $data = array(
            'tier' => $tier,
            'max_nodes' => $tier === 'enterprise' ? 100 : ($tier === 'hyperscaler' ? -1 : 1),
            'billing_period_months' => 12
        );
        
        $response = self::make_request('/subscriptions', 'POST', $data);
        
        if (isset($response['error'])) {
            return false;
        }
        
        return $response;
    }
    
    /**
     * Validate license key
     */
    public static function validate_license($license_key) {
        $response = self::make_request('/license/validate/' . $license_key);
        
        if (isset($response['error'])) {
            return false;
        }
        
        return $response;
    }
    
    /**
     * Create payment in epServer
     */
    public static function create_payment($order, $subscription_id) {
        $data = array(
            'subscription_id' => $subscription_id,
            'amount' => floatval($order['total_amount']),
            'currency' => $order['currency'],
            'payment_method' => 'bank_transfer'
        );
        
        $response = self::make_request('/payments', 'POST', $data);
        
        if (isset($response['error'])) {
            return false;
        }
        
        return $response;
    }
    
    /**
     * Check payment status in epServer
     */
    public static function check_payment_status($epserver_payment_id) {
        $response = self::make_request('/payments/' . $epserver_payment_id);
        
        if (isset($response['error'])) {
            return false;
        }
        
        return $response;
    }
    
    /**
     * Verify payment in epServer
     */
    public static function verify_payment($epserver_payment_id) {
        $response = self::make_request('/payments/' . $epserver_payment_id . '/verify', 'POST');
        
        if (isset($response['error'])) {
            return false;
        }
        
        return $response;
    }
    
    /**
     * Get subscription details from epServer
     */
    public static function get_subscription($subscription_id) {
        $response = self::make_request('/subscriptions/' . $subscription_id);
        
        if (isset($response['error'])) {
            return false;
        }
        
        return $response;
    }
    
    /**
     * Get license information from epServer
     */
    public static function get_license_info($license_key) {
        $response = self::make_request('/license/info/' . $license_key);
        
        if (isset($response['error'])) {
            return false;
        }
        
        return $response;
    }
    
    /**
     * Test API connection
     */
    public static function test_connection() {
        $response = self::make_request('/health');
        
        if (isset($response['error'])) {
            return array(
                'success' => false,
                'message' => $response['error']
            );
        }
        
        return array(
            'success' => true,
            'message' => 'Connection successful',
            'data' => $response
        );
    }
    
    /**
     * Get pricing information
     */
    public static function get_pricing() {
        $response = self::make_request('/pricing');
        
        if (isset($response['error'])) {
            return false;
        }
        
        return $response;
    }
    
    /**
     * Sync all master data
     */
    public static function sync_all() {
        $results = array(
            'products' => self::sync_products()
        );
        
        return $results;
    }
}
