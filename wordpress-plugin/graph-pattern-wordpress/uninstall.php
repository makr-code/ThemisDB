<?php
/**
 * Uninstall Script
 * Fired when the plugin is uninstalled.
 */

// Exit if accessed directly or not via WP uninstall
if (!defined('WP_UNINSTALL_PLUGIN')) {
    exit;
}

// Delete all plugin options
delete_option('themisdb_gp_default_layout');
delete_option('themisdb_gp_node_color_scheme');
delete_option('themisdb_gp_enable_physics');
delete_option('themisdb_gp_show_labels');
delete_option('themisdb_gp_edge_smooth');
delete_option('themisdb_gp_max_nodes');
delete_option('themisdb_gp_enable_search');
delete_option('themisdb_gp_enable_filters');
delete_option('themisdb_gp_enable_export');

// Delete transients
delete_transient('themisdb_gp_cached_graph_data');

// Delete user meta for saved layouts
$users = get_users(array('fields' => 'ID'));
foreach ($users as $user_id) {
    delete_user_meta($user_id, 'themisdb_gp_custom_layout');
}
