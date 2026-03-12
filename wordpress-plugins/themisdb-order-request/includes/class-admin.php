<?php
/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            class-admin.php                                    ║
  Version:         0.0.2                                              ║
  Last Modified:   2026-03-09 04:08:19                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     1350                                           ║
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
 * Admin interface for ThemisDB Order Request Plugin
 */

if (!defined('ABSPATH')) {
    exit;
}

class ThemisDB_Order_Admin {
    
    public function __construct() {
        add_action('admin_menu', array($this, 'add_admin_menu'));
        add_action('admin_init', array($this, 'register_settings'));
        add_action('admin_post_themisdb_sync_epserver', array($this, 'handle_sync'));
    }
    
    /**
     * Add admin menu
     */
    public function add_admin_menu() {
        add_menu_page(
            __('ThemisDB Bestellungen', 'themisdb-order-request'),
            __('ThemisDB Orders', 'themisdb-order-request'),
            'manage_options',
            'themisdb-orders',
            array($this, 'orders_page'),
            'dashicons-cart',
            30
        );
        
        add_submenu_page(
            'themisdb-orders',
            __('Alle Bestellungen', 'themisdb-order-request'),
            __('Bestellungen', 'themisdb-order-request'),
            'manage_options',
            'themisdb-orders',
            array($this, 'orders_page')
        );
        
        add_submenu_page(
            'themisdb-orders',
            __('Verträge', 'themisdb-order-request'),
            __('Verträge', 'themisdb-order-request'),
            'manage_options',
            'themisdb-contracts',
            array($this, 'contracts_page')
        );
        
        add_submenu_page(
            'themisdb-orders',
            __('Produkte', 'themisdb-order-request'),
            __('Produkte', 'themisdb-order-request'),
            'manage_options',
            'themisdb-products',
            array($this, 'products_page')
        );
        
        add_submenu_page(
            'themisdb-orders',
            __('Zahlungen', 'themisdb-order-request'),
            __('Zahlungen', 'themisdb-order-request'),
            'manage_options',
            'themisdb-payments',
            array($this, 'payments_page')
        );
        
        add_submenu_page(
            'themisdb-orders',
            __('Lizenzen', 'themisdb-order-request'),
            __('Lizenzen', 'themisdb-order-request'),
            'manage_options',
            'themisdb-licenses',
            array($this, 'licenses_page')
        );
        
        add_submenu_page(
            'themisdb-orders',
            __('E-Mail Log', 'themisdb-order-request'),
            __('E-Mail Log', 'themisdb-order-request'),
            'manage_options',
            'themisdb-email-log',
            array($this, 'email_log_page')
        );

        add_submenu_page(
            'themisdb-orders',
            __('License Audit Log', 'themisdb-order-request'),
            __('License Audit Log', 'themisdb-order-request'),
            'manage_options',
            'themisdb-license-audit',
            array($this, 'license_audit_page')
        );
        
        add_submenu_page(
            'themisdb-orders',
            __('Einstellungen', 'themisdb-order-request'),
            __('Einstellungen', 'themisdb-order-request'),
            'manage_options',
            'themisdb-order-settings',
            array($this, 'settings_page')
        );
    }
    
    /**
     * Register settings
     */
    public function register_settings() {
        register_setting('themisdb_order_settings', 'themisdb_order_epserver_url');
        register_setting('themisdb_order_settings', 'themisdb_order_epserver_api_key');
        register_setting('themisdb_order_settings', 'themisdb_order_email_from');
        register_setting('themisdb_order_settings', 'themisdb_order_email_from_name');
        register_setting('themisdb_order_settings', 'themisdb_order_pdf_storage');
        register_setting('themisdb_order_settings', 'themisdb_order_legal_compliance');
        register_setting('themisdb_order_settings', 'themisdb_license_api_key');
        register_setting('themisdb_order_settings', 'themisdb_license_admin_secret');
        register_setting('themisdb_order_settings', 'themisdb_license_renewal_reminder_days');
    }
    
    /**
     * Orders page
     */
    public function orders_page() {
        $action = isset($_GET['action']) ? sanitize_text_field($_GET['action']) : 'list';
        $order_id = isset($_GET['order_id']) ? intval($_GET['order_id']) : 0;
        
        if ($action === 'view' && $order_id) {
            $this->view_order($order_id);
        } else {
            $this->list_orders();
        }
    }
    
    /**
     * List orders
     */
    private function list_orders() {
        $orders = ThemisDB_Order_Manager::get_all_orders();
        
        ?>
        <div class="wrap">
            <h1><?php _e('Bestellungen', 'themisdb-order-request'); ?></h1>
            
            <table class="wp-list-table widefat fixed striped">
                <thead>
                    <tr>
                        <th><?php _e('Bestellnummer', 'themisdb-order-request'); ?></th>
                        <th><?php _e('Kunde', 'themisdb-order-request'); ?></th>
                        <th><?php _e('Produkt', 'themisdb-order-request'); ?></th>
                        <th><?php _e('Betrag', 'themisdb-order-request'); ?></th>
                        <th><?php _e('Status', 'themisdb-order-request'); ?></th>
                        <th><?php _e('Datum', 'themisdb-order-request'); ?></th>
                        <th><?php _e('Aktionen', 'themisdb-order-request'); ?></th>
                    </tr>
                </thead>
                <tbody>
                    <?php if (empty($orders)): ?>
                    <tr>
                        <td colspan="7"><?php _e('Keine Bestellungen gefunden', 'themisdb-order-request'); ?></td>
                    </tr>
                    <?php else: ?>
                        <?php foreach ($orders as $order): ?>
                        <tr>
                            <td><strong><?php echo esc_html($order['order_number']); ?></strong></td>
                            <td>
                                <?php echo esc_html($order['customer_name']); ?>
                                <?php if ($order['customer_company']): ?>
                                    <br><small><?php echo esc_html($order['customer_company']); ?></small>
                                <?php endif; ?>
                            </td>
                            <td><?php echo esc_html(ucfirst($order['product_edition'])); ?></td>
                            <td><?php echo number_format($order['total_amount'], 2, ',', '.'); ?> <?php echo esc_html($order['currency']); ?></td>
                            <td>
                                <span class="order-status status-<?php echo esc_attr($order['status']); ?>">
                                    <?php echo esc_html(ucfirst($order['status'])); ?>
                                </span>
                            </td>
                            <td><?php echo esc_html(wp_date('d.m.Y H:i', strtotime($order['created_at']))); ?></td>
                            <td>
                                <a href="?page=themisdb-orders&action=view&order_id=<?php echo absint($order['id']); ?>" class="button button-small">
                                    <?php _e('Ansehen', 'themisdb-order-request'); ?>
                                </a>
                            </td>
                        </tr>
                        <?php endforeach; ?>
                    <?php endif; ?>
                </tbody>
            </table>
        </div>
        <?php
    }
    
    /**
     * View single order
     */
    private function view_order($order_id) {
        $order = ThemisDB_Order_Manager::get_order($order_id);
        
        if (!$order) {
            echo '<div class="notice notice-error"><p>' . __('Bestellung nicht gefunden', 'themisdb-order-request') . '</p></div>';
            return;
        }
        
        $contracts = ThemisDB_Contract_Manager::get_contracts_by_order($order_id);
        
        ?>
        <div class="wrap">
            <h1><?php _e('Bestellung', 'themisdb-order-request'); ?>: <?php echo esc_html($order['order_number']); ?></h1>
            
            <div class="card">
                <h2><?php _e('Kundendaten', 'themisdb-order-request'); ?></h2>
                <table class="form-table">
                    <tr>
                        <th><?php _e('Name', 'themisdb-order-request'); ?>:</th>
                        <td><?php echo esc_html($order['customer_name']); ?></td>
                    </tr>
                    <?php if ($order['customer_company']): ?>
                    <tr>
                        <th><?php _e('Unternehmen', 'themisdb-order-request'); ?>:</th>
                        <td><?php echo esc_html($order['customer_company']); ?></td>
                    </tr>
                    <?php endif; ?>
                    <tr>
                        <th><?php _e('E-Mail', 'themisdb-order-request'); ?>:</th>
                        <td><?php echo esc_html($order['customer_email']); ?></td>
                    </tr>
                </table>
            </div>
            
            <div class="card">
                <h2><?php _e('Bestelldetails', 'themisdb-order-request'); ?></h2>
                <table class="form-table">
                    <tr>
                        <th><?php _e('Bestellnummer', 'themisdb-order-request'); ?>:</th>
                        <td><?php echo esc_html($order['order_number']); ?></td>
                    </tr>
                    <tr>
                        <th><?php _e('Produkt', 'themisdb-order-request'); ?>:</th>
                        <td>ThemisDB <?php echo esc_html(ucfirst($order['product_edition'])); ?> Edition</td>
                    </tr>
                    <tr>
                        <th><?php _e('Gesamtbetrag', 'themisdb-order-request'); ?>:</th>
                        <td><strong><?php echo number_format($order['total_amount'], 2, ',', '.'); ?> <?php echo esc_html($order['currency']); ?></strong></td>
                    </tr>
                    <tr>
                        <th><?php _e('Status', 'themisdb-order-request'); ?>:</th>
                        <td><?php echo esc_html(ucfirst($order['status'])); ?></td>
                    </tr>
                    <tr>
                        <th><?php _e('Erstellt am', 'themisdb-order-request'); ?>:</th>
                        <td><?php echo date('d.m.Y H:i', strtotime($order['created_at'])); ?></td>
                    </tr>
                </table>
                
                <?php if (!empty($order['modules'])): ?>
                <h3><?php _e('Module', 'themisdb-order-request'); ?></h3>
                <ul>
                    <?php 
                    $modules = ThemisDB_Order_Manager::get_modules();
                    foreach ($modules as $module):
                        if (in_array($module['module_code'], $order['modules'])):
                    ?>
                    <li><?php echo esc_html($module['module_name']); ?> - <?php echo number_format($module['price'], 2, ',', '.'); ?> €</li>
                    <?php 
                        endif;
                    endforeach; 
                    ?>
                </ul>
                <?php endif; ?>
                
                <?php if (!empty($order['training_modules'])): ?>
                <h3><?php _e('Schulungen', 'themisdb-order-request'); ?></h3>
                <ul>
                    <?php 
                    $trainings = ThemisDB_Order_Manager::get_training_modules();
                    foreach ($trainings as $training):
                        if (in_array($training['training_code'], $order['training_modules'])):
                    ?>
                    <li><?php echo esc_html($training['training_name']); ?> - <?php echo number_format($training['price'], 2, ',', '.'); ?> €</li>
                    <?php 
                        endif;
                    endforeach; 
                    ?>
                </ul>
                <?php endif; ?>
            </div>
            
            <?php if (!empty($contracts)): ?>
            <div class="card">
                <h2><?php _e('Verträge', 'themisdb-order-request'); ?></h2>
                <table class="wp-list-table widefat">
                    <thead>
                        <tr>
                            <th><?php _e('Vertragsnummer', 'themisdb-order-request'); ?></th>
                            <th><?php _e('Typ', 'themisdb-order-request'); ?></th>
                            <th><?php _e('Status', 'themisdb-order-request'); ?></th>
                            <th><?php _e('Aktionen', 'themisdb-order-request'); ?></th>
                        </tr>
                    </thead>
                    <tbody>
                        <?php foreach ($contracts as $contract): ?>
                        <tr>
                            <td><?php echo esc_html($contract['contract_number']); ?></td>
                            <td><?php echo esc_html(ucfirst($contract['contract_type'])); ?></td>
                            <td><?php echo esc_html(ucfirst($contract['status'])); ?></td>
                            <td>
                                <a href="?page=themisdb-contracts&action=view&contract_id=<?php echo absint($contract['id']); ?>" class="button button-small">
                                    <?php _e('Ansehen', 'themisdb-order-request'); ?>
                                </a>
                            </td>
                        </tr>
                        <?php endforeach; ?>
                    </tbody>
                </table>
            </div>
            <?php endif; ?>
            
            <p>
                <a href="?page=themisdb-orders" class="button"><?php _e('Zurück zur Übersicht', 'themisdb-order-request'); ?></a>
            </p>
        </div>
        <?php
    }
    
    /**
     * Contracts page
     */
    public function contracts_page() {
        $action = isset($_GET['action']) ? sanitize_text_field($_GET['action']) : 'list';
        $contract_id = isset($_GET['contract_id']) ? intval($_GET['contract_id']) : 0;
        
        if ($action === 'view' && $contract_id) {
            $this->view_contract($contract_id);
        } else {
            $this->list_contracts();
        }
    }
    
    /**
     * List contracts
     */
    private function list_contracts() {
        $contracts = ThemisDB_Contract_Manager::get_all_contracts();
        
        ?>
        <div class="wrap">
            <h1><?php _e('Verträge', 'themisdb-order-request'); ?></h1>
            
            <table class="wp-list-table widefat fixed striped">
                <thead>
                    <tr>
                        <th><?php _e('Vertragsnummer', 'themisdb-order-request'); ?></th>
                        <th><?php _e('Typ', 'themisdb-order-request'); ?></th>
                        <th><?php _e('Status', 'themisdb-order-request'); ?></th>
                        <th><?php _e('Gültig von', 'themisdb-order-request'); ?></th>
                        <th><?php _e('Gültig bis', 'themisdb-order-request'); ?></th>
                        <th><?php _e('Erstellt', 'themisdb-order-request'); ?></th>
                        <th><?php _e('Aktionen', 'themisdb-order-request'); ?></th>
                    </tr>
                </thead>
                <tbody>
                    <?php if (empty($contracts)): ?>
                    <tr>
                        <td colspan="7"><?php _e('Keine Verträge gefunden', 'themisdb-order-request'); ?></td>
                    </tr>
                    <?php else: ?>
                        <?php foreach ($contracts as $contract): ?>
                        <tr>
                            <td><strong><?php echo esc_html($contract['contract_number']); ?></strong></td>
                            <td><?php echo esc_html(ucfirst($contract['contract_type'])); ?></td>
                            <td><?php echo esc_html(ucfirst($contract['status'])); ?></td>
                            <td><?php echo wp_date('d.m.Y', strtotime($contract['valid_from'])); ?></td>
                            <td><?php echo $contract['valid_until'] ? esc_html(wp_date('d.m.Y', strtotime($contract['valid_until']))) : '-'; ?></td>
                            <td><?php echo esc_html(wp_date('d.m.Y', strtotime($contract['created_at']))); ?></td>
                            <td>
                                <a href="?page=themisdb-contracts&action=view&contract_id=<?php echo absint($contract['id']); ?>" class="button button-small">
                                    <?php _e('Ansehen', 'themisdb-order-request'); ?>
                                </a>
                            </td>
                        </tr>
                        <?php endforeach; ?>
                    <?php endif; ?>
                </tbody>
            </table>
        </div>
        <?php
    }
    
    /**
     * View single contract
     */
    private function view_contract($contract_id) {
        $contract = ThemisDB_Contract_Manager::get_contract($contract_id);
        
        if (!$contract) {
            echo '<div class="notice notice-error"><p>' . __('Vertrag nicht gefunden', 'themisdb-order-request') . '</p></div>';
            return;
        }
        
        $order = ThemisDB_Order_Manager::get_order($contract['order_id']);
        $revisions = ThemisDB_Contract_Manager::get_contract_revisions($contract_id);
        
        ?>
        <div class="wrap">
            <h1><?php _e('Vertrag', 'themisdb-order-request'); ?>: <?php echo esc_html($contract['contract_number']); ?></h1>
            
            <div class="card">
                <h2><?php _e('Vertragsdetails', 'themisdb-order-request'); ?></h2>
                <table class="form-table">
                    <tr>
                        <th><?php _e('Vertragsnummer', 'themisdb-order-request'); ?>:</th>
                        <td><?php echo esc_html($contract['contract_number']); ?></td>
                    </tr>
                    <tr>
                        <th><?php _e('Typ', 'themisdb-order-request'); ?>:</th>
                        <td><?php echo esc_html(ucfirst($contract['contract_type'])); ?></td>
                    </tr>
                    <tr>
                        <th><?php _e('Status', 'themisdb-order-request'); ?>:</th>
                        <td><?php echo esc_html(ucfirst($contract['status'])); ?></td>
                    </tr>
                    <tr>
                        <th><?php _e('Gültig von', 'themisdb-order-request'); ?>:</th>
                        <td><?php echo date('d.m.Y', strtotime($contract['valid_from'])); ?></td>
                    </tr>
                    <?php if ($contract['valid_until']): ?>
                    <tr>
                        <th><?php _e('Gültig bis', 'themisdb-order-request'); ?>:</th>
                        <td><?php echo date('d.m.Y', strtotime($contract['valid_until'])); ?></td>
                    </tr>
                    <?php endif; ?>
                    <?php if ($contract['signed_at']): ?>
                    <tr>
                        <th><?php _e('Unterzeichnet am', 'themisdb-order-request'); ?>:</th>
                        <td><?php echo date('d.m.Y H:i', strtotime($contract['signed_at'])); ?></td>
                    </tr>
                    <?php endif; ?>
                </table>
            </div>
            
            <?php if ($order): ?>
            <div class="card">
                <h2><?php _e('Zugehörige Bestellung', 'themisdb-order-request'); ?></h2>
                <p>
                    <strong><?php _e('Bestellnummer', 'themisdb-order-request'); ?>:</strong> <?php echo esc_html($order['order_number']); ?><br>
                    <strong><?php _e('Kunde', 'themisdb-order-request'); ?>:</strong> <?php echo esc_html($order['customer_name']); ?><br>
                    <a href="?page=themisdb-orders&action=view&order_id=<?php echo $order['id']; ?>" class="button button-small">
                        <?php _e('Bestellung ansehen', 'themisdb-order-request'); ?>
                    </a>
                </p>
            </div>
            <?php endif; ?>
            
            <?php if (!empty($revisions)): ?>
            <div class="card">
                <h2><?php _e('Revisionen', 'themisdb-order-request'); ?> (<?php echo count($revisions); ?>)</h2>
                <table class="wp-list-table widefat">
                    <thead>
                        <tr>
                            <th><?php _e('Revision', 'themisdb-order-request'); ?></th>
                            <th><?php _e('Geändert von', 'themisdb-order-request'); ?></th>
                            <th><?php _e('Grund', 'themisdb-order-request'); ?></th>
                            <th><?php _e('Datum', 'themisdb-order-request'); ?></th>
                        </tr>
                    </thead>
                    <tbody>
                        <?php foreach ($revisions as $revision): ?>
                        <?php $user = get_userdata($revision['changed_by']); ?>
                        <tr>
                            <td><?php echo $revision['revision_number']; ?></td>
                            <td><?php echo $user ? esc_html($user->display_name) : 'N/A'; ?></td>
                            <td><?php echo esc_html($revision['change_reason']); ?></td>
                            <td><?php echo date('d.m.Y H:i', strtotime($revision['created_at'])); ?></td>
                        </tr>
                        <?php endforeach; ?>
                    </tbody>
                </table>
            </div>
            <?php endif; ?>
            
            <p>
                <a href="?page=themisdb-contracts" class="button"><?php _e('Zurück zur Übersicht', 'themisdb-order-request'); ?></a>
            </p>
        </div>
        <?php
    }
    
    /**
     * Products page
     */
    public function products_page() {
        $products = ThemisDB_Order_Manager::get_products();
        $modules = ThemisDB_Order_Manager::get_modules();
        $trainings = ThemisDB_Order_Manager::get_training_modules();
        
        ?>
        <div class="wrap">
            <h1><?php _e('Produkte und Module', 'themisdb-order-request'); ?></h1>
            
            <div class="card">
                <h2><?php _e('Produkte', 'themisdb-order-request'); ?></h2>
                <table class="wp-list-table widefat">
                    <thead>
                        <tr>
                            <th><?php _e('Code', 'themisdb-order-request'); ?></th>
                            <th><?php _e('Name', 'themisdb-order-request'); ?></th>
                            <th><?php _e('Edition', 'themisdb-order-request'); ?></th>
                            <th><?php _e('Preis', 'themisdb-order-request'); ?></th>
                        </tr>
                    </thead>
                    <tbody>
                        <?php foreach ($products as $product): ?>
                        <tr>
                            <td><?php echo esc_html($product['product_code']); ?></td>
                            <td><?php echo esc_html($product['product_name']); ?></td>
                            <td><?php echo esc_html(ucfirst($product['edition'])); ?></td>
                            <td><?php echo number_format($product['price'], 2, ',', '.'); ?> <?php echo esc_html($product['currency']); ?></td>
                        </tr>
                        <?php endforeach; ?>
                    </tbody>
                </table>
            </div>
            
            <div class="card">
                <h2><?php _e('Module', 'themisdb-order-request'); ?></h2>
                <table class="wp-list-table widefat">
                    <thead>
                        <tr>
                            <th><?php _e('Code', 'themisdb-order-request'); ?></th>
                            <th><?php _e('Name', 'themisdb-order-request'); ?></th>
                            <th><?php _e('Kategorie', 'themisdb-order-request'); ?></th>
                            <th><?php _e('Preis', 'themisdb-order-request'); ?></th>
                        </tr>
                    </thead>
                    <tbody>
                        <?php foreach ($modules as $module): ?>
                        <tr>
                            <td><?php echo esc_html($module['module_code']); ?></td>
                            <td><?php echo esc_html($module['module_name']); ?></td>
                            <td><?php echo esc_html($module['module_category']); ?></td>
                            <td><?php echo number_format($module['price'], 2, ',', '.'); ?> <?php echo esc_html($module['currency']); ?></td>
                        </tr>
                        <?php endforeach; ?>
                    </tbody>
                </table>
            </div>
            
            <div class="card">
                <h2><?php _e('Schulungen', 'themisdb-order-request'); ?></h2>
                <table class="wp-list-table widefat">
                    <thead>
                        <tr>
                            <th><?php _e('Code', 'themisdb-order-request'); ?></th>
                            <th><?php _e('Name', 'themisdb-order-request'); ?></th>
                            <th><?php _e('Typ', 'themisdb-order-request'); ?></th>
                            <th><?php _e('Dauer', 'themisdb-order-request'); ?></th>
                            <th><?php _e('Preis', 'themisdb-order-request'); ?></th>
                        </tr>
                    </thead>
                    <tbody>
                        <?php foreach ($trainings as $training): ?>
                        <tr>
                            <td><?php echo esc_html($training['training_code']); ?></td>
                            <td><?php echo esc_html($training['training_name']); ?></td>
                            <td><?php echo esc_html(ucfirst($training['training_type'])); ?></td>
                            <td><?php echo $training['duration_hours']; ?> Stunden</td>
                            <td><?php echo number_format($training['price'], 2, ',', '.'); ?> <?php echo esc_html($training['currency']); ?></td>
                        </tr>
                        <?php endforeach; ?>
                    </tbody>
                </table>
            </div>
        </div>
        <?php
    }
    
    /**
     * Payments page
     */
    public function payments_page() {
        $action = isset($_GET['action']) ? sanitize_text_field($_GET['action']) : 'list';
        $payment_id = isset($_GET['payment_id']) ? intval($_GET['payment_id']) : 0;
        
        // Handle payment verification
        if ($action === 'verify' && $payment_id && check_admin_referer('verify_payment_' . $payment_id)) {
            ThemisDB_Payment_Manager::verify_payment($payment_id);
            wp_redirect(admin_url('admin.php?page=themisdb-payments&verified=1'));
            exit;
        }
        
        if ($action === 'view' && $payment_id) {
            $this->view_payment($payment_id);
        } else {
            $this->list_payments();
        }
    }
    
    /**
     * List payments
     */
    private function list_payments() {
        $payments = ThemisDB_Payment_Manager::get_all_payments();
        $stats = ThemisDB_Payment_Manager::get_payment_stats();
        
        ?>
        <div class="wrap">
            <h1><?php _e('Zahlungen', 'themisdb-order-request'); ?></h1>
            
            <?php if (isset($_GET['verified'])): ?>
            <div class="notice notice-success"><p><?php _e('Zahlung wurde erfolgreich verifiziert', 'themisdb-order-request'); ?></p></div>
            <?php endif; ?>
            
            <div class="card" style="max-width: none; margin-bottom: 20px;">
                <h2><?php _e('Zahlungsübersicht', 'themisdb-order-request'); ?></h2>
                <table class="form-table">
                    <tr>
                        <th><?php _e('Gesamt Zahlungen', 'themisdb-order-request'); ?>:</th>
                        <td><strong><?php echo $stats['total_payments']; ?></strong></td>
                        <th><?php _e('Gesamtbetrag', 'themisdb-order-request'); ?>:</th>
                        <td><strong><?php echo number_format($stats['total_amount'], 2, ',', '.'); ?> €</strong></td>
                    </tr>
                    <tr>
                        <th><?php _e('Verifiziert', 'themisdb-order-request'); ?>:</th>
                        <td><span style="color: green;"><strong><?php echo $stats['verified_payments']; ?></strong></span></td>
                        <th><?php _e('Verifizierter Betrag', 'themisdb-order-request'); ?>:</th>
                        <td><span style="color: green;"><strong><?php echo number_format($stats['verified_amount'], 2, ',', '.'); ?> €</strong></span></td>
                    </tr>
                    <tr>
                        <th><?php _e('Ausstehend', 'themisdb-order-request'); ?>:</th>
                        <td><span style="color: orange;"><strong><?php echo $stats['pending_payments']; ?></strong></span></td>
                        <th><?php _e('Fehlgeschlagen', 'themisdb-order-request'); ?>:</th>
                        <td><span style="color: red;"><strong><?php echo $stats['failed_payments']; ?></strong></span></td>
                    </tr>
                </table>
            </div>
            
            <table class="wp-list-table widefat fixed striped">
                <thead>
                    <tr>
                        <th><?php _e('Zahlungsnummer', 'themisdb-order-request'); ?></th>
                        <th><?php _e('Bestellung', 'themisdb-order-request'); ?></th>
                        <th><?php _e('Betrag', 'themisdb-order-request'); ?></th>
                        <th><?php _e('Methode', 'themisdb-order-request'); ?></th>
                        <th><?php _e('Status', 'themisdb-order-request'); ?></th>
                        <th><?php _e('Datum', 'themisdb-order-request'); ?></th>
                        <th><?php _e('Aktionen', 'themisdb-order-request'); ?></th>
                    </tr>
                </thead>
                <tbody>
                    <?php if (empty($payments)): ?>
                    <tr>
                        <td colspan="7"><?php _e('Keine Zahlungen gefunden', 'themisdb-order-request'); ?></td>
                    </tr>
                    <?php else: ?>
                        <?php foreach ($payments as $payment): ?>
                        <?php $order = ThemisDB_Order_Manager::get_order($payment['order_id']); ?>
                        <tr>
                            <td><strong><?php echo esc_html($payment['payment_number']); ?></strong></td>
                            <td>
                                <?php if ($order): ?>
                                    <a href="?page=themisdb-orders&action=view&order_id=<?php echo absint($order['id']); ?>">
                                        <?php echo esc_html($order['order_number']); ?>
                                    </a>
                                <?php else: ?>
                                    -
                                <?php endif; ?>
                            </td>
                            <td><?php echo number_format($payment['amount'], 2, ',', '.'); ?> <?php echo esc_html($payment['currency']); ?></td>
                            <td><?php echo esc_html(ucfirst($payment['payment_method'])); ?></td>
                            <td>
                                <span class="payment-status status-<?php echo esc_attr($payment['payment_status']); ?>">
                                    <?php echo esc_html(ucfirst($payment['payment_status'])); ?>
                                </span>
                            </td>
                            <td><?php echo date('d.m.Y H:i', strtotime($payment['created_at'])); ?></td>
                            <td>
                                <a href="?page=themisdb-payments&action=view&payment_id=<?php echo $payment['id']; ?>" class="button button-small">
                                    <?php _e('Ansehen', 'themisdb-order-request'); ?>
                                </a>
                                <?php if ($payment['payment_status'] === 'pending'): ?>
                                <a href="<?php echo wp_nonce_url(admin_url('admin.php?page=themisdb-payments&action=verify&payment_id=' . $payment['id']), 'verify_payment_' . $payment['id']); ?>" 
                                   class="button button-small button-primary">
                                    <?php _e('Verifizieren', 'themisdb-order-request'); ?>
                                </a>
                                <?php endif; ?>
                            </td>
                        </tr>
                        <?php endforeach; ?>
                    <?php endif; ?>
                </tbody>
            </table>
        </div>
        <?php
    }
    
    /**
     * View single payment
     */
    private function view_payment($payment_id) {
        $payment = ThemisDB_Payment_Manager::get_payment($payment_id);
        
        if (!$payment) {
            echo '<div class="notice notice-error"><p>' . __('Zahlung nicht gefunden', 'themisdb-order-request') . '</p></div>';
            return;
        }
        
        $order = ThemisDB_Order_Manager::get_order($payment['order_id']);
        
        ?>
        <div class="wrap">
            <h1><?php _e('Zahlung', 'themisdb-order-request'); ?>: <?php echo esc_html($payment['payment_number']); ?></h1>
            
            <div class="card">
                <h2><?php _e('Zahlungsdetails', 'themisdb-order-request'); ?></h2>
                <table class="form-table">
                    <tr>
                        <th><?php _e('Zahlungsnummer', 'themisdb-order-request'); ?>:</th>
                        <td><?php echo esc_html($payment['payment_number']); ?></td>
                    </tr>
                    <tr>
                        <th><?php _e('Betrag', 'themisdb-order-request'); ?>:</th>
                        <td><strong><?php echo number_format($payment['amount'], 2, ',', '.'); ?> <?php echo esc_html($payment['currency']); ?></strong></td>
                    </tr>
                    <tr>
                        <th><?php _e('Zahlungsmethode', 'themisdb-order-request'); ?>:</th>
                        <td><?php echo esc_html(ucfirst($payment['payment_method'])); ?></td>
                    </tr>
                    <tr>
                        <th><?php _e('Status', 'themisdb-order-request'); ?>:</th>
                        <td>
                            <span class="payment-status status-<?php echo esc_attr($payment['payment_status']); ?>">
                                <?php echo esc_html(ucfirst($payment['payment_status'])); ?>
                            </span>
                        </td>
                    </tr>
                    <?php if ($payment['transaction_id']): ?>
                    <tr>
                        <th><?php _e('Transaktions-ID', 'themisdb-order-request'); ?>:</th>
                        <td><?php echo esc_html($payment['transaction_id']); ?></td>
                    </tr>
                    <?php endif; ?>
                    <?php if ($payment['payment_date']): ?>
                    <tr>
                        <th><?php _e('Zahlungsdatum', 'themisdb-order-request'); ?>:</th>
                        <td><?php echo date('d.m.Y H:i', strtotime($payment['payment_date'])); ?></td>
                    </tr>
                    <?php endif; ?>
                    <?php if ($payment['verified_at']): ?>
                    <tr>
                        <th><?php _e('Verifiziert am', 'themisdb-order-request'); ?>:</th>
                        <td><?php echo date('d.m.Y H:i', strtotime($payment['verified_at'])); ?></td>
                    </tr>
                    <?php endif; ?>
                    <tr>
                        <th><?php _e('Erstellt am', 'themisdb-order-request'); ?>:</th>
                        <td><?php echo date('d.m.Y H:i', strtotime($payment['created_at'])); ?></td>
                    </tr>
                    <?php if ($payment['notes']): ?>
                    <tr>
                        <th><?php _e('Notizen', 'themisdb-order-request'); ?>:</th>
                        <td><?php echo esc_html($payment['notes']); ?></td>
                    </tr>
                    <?php endif; ?>
                </table>
            </div>
            
            <?php if ($order): ?>
            <div class="card">
                <h2><?php _e('Zugehörige Bestellung', 'themisdb-order-request'); ?></h2>
                <p>
                    <strong><?php _e('Bestellnummer', 'themisdb-order-request'); ?>:</strong> <?php echo esc_html($order['order_number']); ?><br>
                    <strong><?php _e('Kunde', 'themisdb-order-request'); ?>:</strong> <?php echo esc_html($order['customer_name']); ?><br>
                    <a href="?page=themisdb-orders&action=view&order_id=<?php echo absint($order['id']); ?>" class="button button-small">
                        <?php _e('Bestellung ansehen', 'themisdb-order-request'); ?>
                    </a>
                </p>
            </div>
            <?php endif; ?>
            
            <p>
                <a href="?page=themisdb-payments" class="button"><?php _e('Zurück zur Übersicht', 'themisdb-order-request'); ?></a>
                <?php if ($payment['payment_status'] === 'pending'): ?>
                <a href="<?php echo wp_nonce_url(admin_url('admin.php?page=themisdb-payments&action=verify&payment_id=' . $payment['id']), 'verify_payment_' . $payment['id']); ?>" 
                   class="button button-primary">
                    <?php _e('Zahlung verifizieren', 'themisdb-order-request'); ?>
                </a>
                <?php endif; ?>
            </p>
        </div>
        <?php
    }
    
    /**
     * Licenses page
     */
    public function licenses_page() {
        $action = isset($_GET['action']) ? sanitize_text_field($_GET['action']) : 'list';
        $license_id = isset($_GET['license_id']) ? intval($_GET['license_id']) : 0;
        
        if ($action === 'view' && $license_id) {
            $this->view_license($license_id);
        } else {
            $this->list_licenses();
        }
    }
    
    /**
     * List licenses
     */
    private function list_licenses() {
        global $wpdb;
        $table_licenses = $wpdb->prefix . 'themisdb_licenses';
        // Use esc_sql to ensure table name is safe, and use prepare for the entire query
        $licenses = $wpdb->get_results(
            $wpdb->prepare(
                "SELECT * FROM {$wpdb->prefix}themisdb_licenses ORDER BY created_at DESC LIMIT %d",
                50
            ),
            ARRAY_A
        );
        $stats = ThemisDB_License_Manager::get_license_stats();
        
        ?>
        <div class="wrap">
            <h1><?php _e('Lizenzen', 'themisdb-order-request'); ?></h1>
            
            <div class="card" style="max-width: none; margin-bottom: 20px;">
                <h2><?php _e('Lizenzübersicht', 'themisdb-order-request'); ?></h2>
                <table class="form-table">
                    <tr>
                        <th><?php _e('Gesamt Lizenzen', 'themisdb-order-request'); ?>:</th>
                        <td><strong><?php echo $stats['total_licenses']; ?></strong></td>
                        <th><?php _e('Aktiv', 'themisdb-order-request'); ?>:</th>
                        <td><span style="color: green;"><strong><?php echo $stats['active_licenses']; ?></strong></span></td>
                    </tr>
                    <tr>
                        <th><?php _e('Ausstehend', 'themisdb-order-request'); ?>:</th>
                        <td><span style="color: orange;"><strong><?php echo $stats['pending_licenses']; ?></strong></span></td>
                        <th><?php _e('Suspendiert', 'themisdb-order-request'); ?>:</th>
                        <td><span style="color: red;"><strong><?php echo $stats['suspended_licenses']; ?></strong></span></td>
                    </tr>
                </table>
            </div>
            
            <table class="wp-list-table widefat fixed striped">
                <thead>
                    <tr>
                        <th><?php _e('Lizenzschlüssel', 'themisdb-order-request'); ?></th>
                        <th><?php _e('Edition', 'themisdb-order-request'); ?></th>
                        <th><?php _e('Kunde', 'themisdb-order-request'); ?></th>
                        <th><?php _e('Status', 'themisdb-order-request'); ?></th>
                        <th><?php _e('Aktiviert', 'themisdb-order-request'); ?></th>
                        <th><?php _e('Läuft ab', 'themisdb-order-request'); ?></th>
                        <th><?php _e('Aktionen', 'themisdb-order-request'); ?></th>
                    </tr>
                </thead>
                <tbody>
                    <?php if (empty($licenses)): ?>
                    <tr>
                        <td colspan="7"><?php _e('Keine Lizenzen gefunden', 'themisdb-order-request'); ?></td>
                    </tr>
                    <?php else: ?>
                        <?php foreach ($licenses as $license): ?>
                        <?php $order = ThemisDB_Order_Manager::get_order($license['order_id']); ?>
                        <tr>
                            <td><code style="font-size: 10px;"><?php echo esc_html(substr($license['license_key'], 0, 20)); ?>...</code></td>
                            <td><?php echo esc_html(ucfirst($license['product_edition'])); ?></td>
                            <td>
                                <?php if ($order): ?>
                                    <?php echo esc_html($order['customer_name']); ?>
                                <?php else: ?>
                                    -
                                <?php endif; ?>
                            </td>
                            <td>
                                <span class="license-status status-<?php echo esc_attr($license['license_status']); ?>">
                                    <?php echo esc_html(ucfirst($license['license_status'])); ?>
                                </span>
                            </td>
                            <td><?php echo $license['activation_date'] ? date('d.m.Y', strtotime($license['activation_date'])) : '-'; ?></td>
                            <td><?php echo $license['expiry_date'] ? date('d.m.Y', strtotime($license['expiry_date'])) : '∞'; ?></td>
                            <td>
                                <a href="?page=themisdb-licenses&action=view&license_id=<?php echo $license['id']; ?>" class="button button-small">
                                    <?php _e('Ansehen', 'themisdb-order-request'); ?>
                                </a>
                            </td>
                        </tr>
                        <?php endforeach; ?>
                    <?php endif; ?>
                </tbody>
            </table>
        </div>
        <?php
    }
    
    /**
     * View single license
     */
    private function view_license($license_id) {
        $license = ThemisDB_License_Manager::get_license($license_id);
        
        if (!$license) {
            echo '<div class="notice notice-error"><p>' . __('Lizenz nicht gefunden', 'themisdb-order-request') . '</p></div>';
            return;
        }
        
        $order = ThemisDB_Order_Manager::get_order($license['order_id']);
        $contract = ThemisDB_Contract_Manager::get_contract($license['contract_id']);
        
        ?>
        <div class="wrap">
            <h1><?php _e('Lizenz', 'themisdb-order-request'); ?>: <?php echo esc_html($license['product_edition']); ?></h1>
            
            <div class="card">
                <h2><?php _e('Lizenzdetails', 'themisdb-order-request'); ?></h2>
                <table class="form-table">
                    <tr>
                        <th><?php _e('Lizenzschlüssel', 'themisdb-order-request'); ?>:</th>
                        <td><code><?php echo esc_html($license['license_key']); ?></code></td>
                    </tr>
                    <tr>
                        <th><?php _e('Edition', 'themisdb-order-request'); ?>:</th>
                        <td><strong><?php echo esc_html(ucfirst($license['product_edition'])); ?></strong></td>
                    </tr>
                    <tr>
                        <th><?php _e('Lizenztyp', 'themisdb-order-request'); ?>:</th>
                        <td><?php echo esc_html(ucfirst($license['license_type'])); ?></td>
                    </tr>
                    <tr>
                        <th><?php _e('Status', 'themisdb-order-request'); ?>:</th>
                        <td>
                            <span class="license-status status-<?php echo esc_attr($license['license_status']); ?>">
                                <?php echo esc_html(ucfirst($license['license_status'])); ?>
                            </span>
                        </td>
                    </tr>
                    <tr>
                        <th><?php _e('Max. Nodes', 'themisdb-order-request'); ?>:</th>
                        <td><?php echo $license['max_nodes'] ? $license['max_nodes'] : '∞'; ?></td>
                    </tr>
                    <?php if ($license['max_cores']): ?>
                    <tr>
                        <th><?php _e('Max. Cores', 'themisdb-order-request'); ?>:</th>
                        <td><?php echo $license['max_cores']; ?></td>
                    </tr>
                    <?php endif; ?>
                    <?php if ($license['max_storage_gb']): ?>
                    <tr>
                        <th><?php _e('Max. Storage', 'themisdb-order-request'); ?>:</th>
                        <td><?php echo $license['max_storage_gb']; ?> GB</td>
                    </tr>
                    <?php endif; ?>
                    <?php if ($license['activation_date']): ?>
                    <tr>
                        <th><?php _e('Aktivierungsdatum', 'themisdb-order-request'); ?>:</th>
                        <td><?php echo date('d.m.Y H:i', strtotime($license['activation_date'])); ?></td>
                    </tr>
                    <?php endif; ?>
                    <?php if ($license['expiry_date']): ?>
                    <tr>
                        <th><?php _e('Ablaufdatum', 'themisdb-order-request'); ?>:</th>
                        <td><?php echo date('d.m.Y', strtotime($license['expiry_date'])); ?></td>
                    </tr>
                    <?php endif; ?>
                    <?php if ($license['last_check']): ?>
                    <tr>
                        <th><?php _e('Letzte Prüfung', 'themisdb-order-request'); ?>:</th>
                        <td><?php echo date('d.m.Y H:i', strtotime($license['last_check'])); ?></td>
                    </tr>
                    <?php endif; ?>
                    <tr>
                        <th><?php _e('Erstellt am', 'themisdb-order-request'); ?>:</th>
                        <td><?php echo date('d.m.Y H:i', strtotime($license['created_at'])); ?></td>
                    </tr>
                </table>
            </div>
            
            <?php if ($order): ?>
            <div class="card">
                <h2><?php _e('Zugehörige Bestellung', 'themisdb-order-request'); ?></h2>
                <p>
                    <strong><?php _e('Bestellnummer', 'themisdb-order-request'); ?>:</strong> <?php echo esc_html($order['order_number']); ?><br>
                    <strong><?php _e('Kunde', 'themisdb-order-request'); ?>:</strong> <?php echo esc_html($order['customer_name']); ?><br>
                    <strong><?php _e('E-Mail', 'themisdb-order-request'); ?>:</strong> <?php echo esc_html($order['customer_email']); ?><br>
                    <a href="?page=themisdb-orders&action=view&order_id=<?php echo absint($order['id']); ?>" class="button button-small">
                        <?php _e('Bestellung ansehen', 'themisdb-order-request'); ?>
                    </a>
                </p>
            </div>
            <?php endif; ?>
            
            <?php if ($contract): ?>
            <div class="card">
                <h2><?php _e('Zugehöriger Vertrag', 'themisdb-order-request'); ?></h2>
                <p>
                    <strong><?php _e('Vertragsnummer', 'themisdb-order-request'); ?>:</strong> <?php echo esc_html($contract['contract_number']); ?><br>
                    <strong><?php _e('Status', 'themisdb-order-request'); ?>:</strong> <?php echo esc_html(ucfirst($contract['status'])); ?><br>
                    <a href="?page=themisdb-contracts&action=view&contract_id=<?php echo absint($contract['id']); ?>" class="button button-small">
                        <?php _e('Vertrag ansehen', 'themisdb-order-request'); ?>
                    </a>
                </p>
            </div>
            <?php endif; ?>
            
            <?php if ($license['license_file_data']): ?>
            <div class="card">
                <h2><?php _e('Lizenzdatei', 'themisdb-order-request'); ?></h2>
                <p><?php _e('Die Lizenzdatei wurde generiert und kann für die Authentifizierung verwendet werden.', 'themisdb-order-request'); ?></p>
                <textarea readonly style="width: 100%; height: 200px; font-family: monospace; font-size: 12px;"><?php echo esc_textarea(json_encode($license['license_file_data'], JSON_PRETTY_PRINT)); ?></textarea>
            </div>
            <?php endif; ?>
            
            <p>
                <a href="?page=themisdb-licenses" class="button"><?php _e('Zurück zur Übersicht', 'themisdb-order-request'); ?></a>
            </p>
        </div>
        <?php
    }
    
    /**
     * License Audit Log admin page
     */
    public function license_audit_page() {
        global $wpdb;
        $table   = $wpdb->prefix . 'themisdb_license_audit_log';
        $per_page = 50;
        $page_num = max(1, isset($_GET['paged']) ? absint($_GET['paged']) : 1);
        $offset   = ($page_num - 1) * $per_page;

        // Ensure table exists before querying
        if ( $wpdb->get_var( "SHOW TABLES LIKE '$table'" ) !== $table ) {
            echo '<div class="wrap"><h1>' . esc_html__( 'License Audit Log', 'themisdb-order-request' ) . '</h1>';
            echo '<p>' . esc_html__( 'No audit log entries yet. The log table is created on first REST API call.', 'themisdb-order-request' ) . '</p></div>';
            return;
        }

        $total = (int) $wpdb->get_var( "SELECT COUNT(*) FROM $table" );
        $rows  = $wpdb->get_results(
            $wpdb->prepare(
                "SELECT * FROM $table ORDER BY created_at DESC LIMIT %d OFFSET %d",
                $per_page,
                $offset
            ),
            ARRAY_A
        );

        $total_pages = (int) ceil( $total / $per_page );
        ?>
        <div class="wrap">
            <h1><?php esc_html_e( 'License Audit Log', 'themisdb-order-request' ); ?></h1>
            <p class="description">
                <?php printf(
                    esc_html__( 'Showing %d of %d total entries.', 'themisdb-order-request' ),
                    count( $rows ),
                    $total
                ); ?>
            </p>

            <table class="wp-list-table widefat fixed striped">
                <thead>
                    <tr>
                        <th><?php esc_html_e( 'Date / Time', 'themisdb-order-request' ); ?></th>
                        <th><?php esc_html_e( 'License Key', 'themisdb-order-request' ); ?></th>
                        <th><?php esc_html_e( 'Action', 'themisdb-order-request' ); ?></th>
                        <th><?php esc_html_e( 'Result', 'themisdb-order-request' ); ?></th>
                        <th><?php esc_html_e( 'IP Address', 'themisdb-order-request' ); ?></th>
                        <th><?php esc_html_e( 'User Agent', 'themisdb-order-request' ); ?></th>
                    </tr>
                </thead>
                <tbody>
                    <?php if ( empty( $rows ) ) : ?>
                    <tr><td colspan="6"><?php esc_html_e( 'No entries found.', 'themisdb-order-request' ); ?></td></tr>
                    <?php else : ?>
                        <?php foreach ( $rows as $row ) : ?>
                        <tr>
                            <td><?php echo esc_html( $row['created_at'] ); ?></td>
                            <td><code><?php echo esc_html( $row['license_key'] ?? '—' ); ?></code></td>
                            <td><?php echo esc_html( $row['action'] ); ?></td>
                            <td>
                                <span class="license-status status-<?php echo esc_attr( $row['result'] ); ?>">
                                    <?php echo esc_html( $row['result'] ); ?>
                                </span>
                            </td>
                            <td><?php echo esc_html( $row['ip_address'] ?? '—' ); ?></td>
                            <td style="max-width:200px;overflow:hidden;text-overflow:ellipsis;white-space:nowrap;"
                                title="<?php echo esc_attr( $row['user_agent'] ?? '' ); ?>">
                                <?php echo esc_html( $row['user_agent'] ?? '—' ); ?>
                            </td>
                        </tr>
                        <?php endforeach; ?>
                    <?php endif; ?>
                </tbody>
            </table>

            <?php if ( $total_pages > 1 ) : ?>
            <div class="tablenav bottom">
                <div class="tablenav-pages">
                    <?php
                    echo paginate_links( array(
                        'base'      => add_query_arg( 'paged', '%#%' ),
                        'format'    => '',
                        'prev_text' => '&laquo;',
                        'next_text' => '&raquo;',
                        'total'     => $total_pages,
                        'current'   => $page_num,
                    ) );
                    ?>
                </div>
            </div>
            <?php endif; ?>
        </div>
        <?php
    }

    /**
     * Email log page
     */
    public function email_log_page() {
        $logs = ThemisDB_Email_Handler::get_email_logs();
        
        ?>
        <div class="wrap">
            <h1><?php _e('E-Mail Log', 'themisdb-order-request'); ?></h1>
            
            <table class="wp-list-table widefat fixed striped">
                <thead>
                    <tr>
                        <th><?php _e('Empfänger', 'themisdb-order-request'); ?></th>
                        <th><?php _e('Betreff', 'themisdb-order-request'); ?></th>
                        <th><?php _e('Status', 'themisdb-order-request'); ?></th>
                        <th><?php _e('Gesendet am', 'themisdb-order-request'); ?></th>
                        <th><?php _e('Erstellt am', 'themisdb-order-request'); ?></th>
                    </tr>
                </thead>
                <tbody>
                    <?php if (empty($logs)): ?>
                    <tr>
                        <td colspan="5"><?php _e('Keine E-Mails gefunden', 'themisdb-order-request'); ?></td>
                    </tr>
                    <?php else: ?>
                        <?php foreach ($logs as $log): ?>
                        <tr>
                            <td><?php echo esc_html($log['recipient']); ?></td>
                            <td><?php echo esc_html($log['subject']); ?></td>
                            <td>
                                <span class="status-<?php echo esc_attr($log['status']); ?>">
                                    <?php echo esc_html(ucfirst($log['status'])); ?>
                                </span>
                            </td>
                            <td><?php echo $log['sent_at'] ? date('d.m.Y H:i', strtotime($log['sent_at'])) : '-'; ?></td>
                            <td><?php echo date('d.m.Y H:i', strtotime($log['created_at'])); ?></td>
                        </tr>
                        <?php endforeach; ?>
                    <?php endif; ?>
                </tbody>
            </table>
        </div>
        <?php
    }
    
    /**
     * Settings page
     */
    public function settings_page() {
        // Test connection if requested
        if (isset($_GET['test_connection'])) {
            $test_result = ThemisDB_EPServer_API::test_connection();
            if ($test_result['success']) {
                echo '<div class="notice notice-success"><p>' . __('Verbindung erfolgreich!', 'themisdb-order-request') . '</p></div>';
            } else {
                echo '<div class="notice notice-error"><p>' . __('Verbindung fehlgeschlagen:', 'themisdb-order-request') . ' ' . esc_html($test_result['message']) . '</p></div>';
            }
        }
        
        ?>
        <div class="wrap">
            <h1><?php _e('Einstellungen', 'themisdb-order-request'); ?></h1>
            
            <form method="post" action="options.php">
                <?php settings_fields('themisdb_order_settings'); ?>
                
                <h2><?php _e('epServer Integration', 'themisdb-order-request'); ?></h2>
                <table class="form-table">
                    <tr>
                        <th><label for="themisdb_order_epserver_url"><?php _e('epServer URL', 'themisdb-order-request'); ?></label></th>
                        <td>
                            <input type="text" id="themisdb_order_epserver_url" name="themisdb_order_epserver_url" 
                                   value="<?php echo esc_attr(get_option('themisdb_order_epserver_url')); ?>" 
                                   class="regular-text" />
                            <p class="description"><?php _e('Standard: https://service.themisdb.org:6734', 'themisdb-order-request'); ?></p>
                        </td>
                    </tr>
                    <tr>
                        <th><label for="themisdb_order_epserver_api_key"><?php _e('API Schlüssel', 'themisdb-order-request'); ?></label></th>
                        <td>
                            <input type="password" id="themisdb_order_epserver_api_key" name="themisdb_order_epserver_api_key" 
                                   value="<?php echo esc_attr(get_option('themisdb_order_epserver_api_key')); ?>" 
                                   class="regular-text" />
                            <p class="description"><?php _e('Optional: Bearer Token für epServer API', 'themisdb-order-request'); ?></p>
                        </td>
                    </tr>
                    <tr>
                        <th></th>
                        <td>
                            <a href="?page=themisdb-order-settings&test_connection=1" class="button">
                                <?php _e('Verbindung testen', 'themisdb-order-request'); ?>
                            </a>
                            <a href="<?php echo admin_url('admin-post.php?action=themisdb_sync_epserver'); ?>" class="button">
                                <?php _e('Daten synchronisieren', 'themisdb-order-request'); ?>
                            </a>
                        </td>
                    </tr>
                </table>
                
                <h2><?php _e('E-Mail Einstellungen', 'themisdb-order-request'); ?></h2>
                <table class="form-table">
                    <tr>
                        <th><label for="themisdb_order_email_from"><?php _e('Absender E-Mail', 'themisdb-order-request'); ?></label></th>
                        <td>
                            <input type="email" id="themisdb_order_email_from" name="themisdb_order_email_from" 
                                   value="<?php echo esc_attr(get_option('themisdb_order_email_from')); ?>" 
                                   class="regular-text" />
                        </td>
                    </tr>
                    <tr>
                        <th><label for="themisdb_order_email_from_name"><?php _e('Absender Name', 'themisdb-order-request'); ?></label></th>
                        <td>
                            <input type="text" id="themisdb_order_email_from_name" name="themisdb_order_email_from_name" 
                                   value="<?php echo esc_attr(get_option('themisdb_order_email_from_name')); ?>" 
                                   class="regular-text" />
                        </td>
                    </tr>
                </table>
                
                <h2><?php _e('PDF Einstellungen', 'themisdb-order-request'); ?></h2>
                <table class="form-table">
                    <tr>
                        <th><label for="themisdb_order_pdf_storage"><?php _e('PDF Speicherung', 'themisdb-order-request'); ?></label></th>
                        <td>
                            <select id="themisdb_order_pdf_storage" name="themisdb_order_pdf_storage">
                                <option value="database" <?php selected(get_option('themisdb_order_pdf_storage'), 'database'); ?>>
                                    <?php _e('Datenbank', 'themisdb-order-request'); ?>
                                </option>
                                <option value="filesystem" <?php selected(get_option('themisdb_order_pdf_storage'), 'filesystem'); ?>>
                                    <?php _e('Dateisystem', 'themisdb-order-request'); ?>
                                </option>
                            </select>
                            <p class="description"><?php _e('Wo sollen PDF-Dateien gespeichert werden?', 'themisdb-order-request'); ?></p>
                        </td>
                    </tr>
                </table>
                
                <h2><?php _e('Rechtliche Einstellungen', 'themisdb-order-request'); ?></h2>
                <table class="form-table">
                    <tr>
                        <th><label for="themisdb_order_legal_compliance"><?php _e('Rechtliche Compliance', 'themisdb-order-request'); ?></label></th>
                        <td>
                            <input type="checkbox" id="themisdb_order_legal_compliance" name="themisdb_order_legal_compliance" 
                                   value="1" <?php checked(get_option('themisdb_order_legal_compliance'), '1'); ?> />
                            <label for="themisdb_order_legal_compliance"><?php _e('Rechtliche Compliance-Prüfungen aktivieren', 'themisdb-order-request'); ?></label>
                        </td>
                    </tr>
                </table>

                <h2><?php _e('License API Settings', 'themisdb-order-request'); ?></h2>
                <p class="description"><?php _e('These credentials are used by ThemisDB server instances to validate licenses via the REST API.', 'themisdb-order-request'); ?></p>
                <table class="form-table">
                    <tr>
                        <th><label for="themisdb_license_api_key"><?php _e('License API Key', 'themisdb-order-request'); ?></label></th>
                        <td>
                            <input type="password" id="themisdb_license_api_key" name="themisdb_license_api_key"
                                   value="<?php echo esc_attr(get_option('themisdb_license_api_key')); ?>"
                                   class="regular-text" autocomplete="new-password" />
                            <p class="description"><?php echo wp_kses( __('Shared secret sent by ThemisDB servers as <code>Authorization: Bearer &lt;key&gt;</code>. Generate a strong random value (32+ chars).', 'themisdb-order-request'), array( 'code' => array() ) ); ?></p>
                        </td>
                    </tr>
                    <tr>
                        <th><label for="themisdb_license_admin_secret"><?php _e('Admin Secret', 'themisdb-order-request'); ?></label></th>
                        <td>
                            <input type="password" id="themisdb_license_admin_secret" name="themisdb_license_admin_secret"
                                   value="<?php echo esc_attr(get_option('themisdb_license_admin_secret')); ?>"
                                   class="regular-text" autocomplete="new-password" />
                            <p class="description"><?php echo wp_kses( __('Additional secret required for admin endpoints (renew, revoke). Sent as <code>X-ThemisDB-Admin-Secret</code> header.', 'themisdb-order-request'), array( 'code' => array() ) ); ?></p>
                        </td>
                    </tr>
                    <tr>
                        <th><label for="themisdb_license_renewal_reminder_days"><?php _e('Renewal Reminder (days)', 'themisdb-order-request'); ?></label></th>
                        <td>
                            <input type="number" id="themisdb_license_renewal_reminder_days" name="themisdb_license_renewal_reminder_days"
                                   value="<?php echo esc_attr(get_option('themisdb_license_renewal_reminder_days', '30')); ?>"
                                   min="1" max="365" class="small-text" />
                            <p class="description"><?php _e('Send renewal reminder e-mail this many days before a license expires. A daily cron job checks for upcoming expirations.', 'themisdb-order-request'); ?></p>
                        </td>
                    </tr>
                    <tr>
                        <th></th>
                        <td>
                            <a href="<?php echo esc_url(admin_url('admin.php?page=themisdb-license-audit')); ?>" class="button">
                                <?php _e('View License Audit Log', 'themisdb-order-request'); ?>
                            </a>
                        </td>
                    </tr>
                </table>
                
                <?php submit_button(); ?>
            </form>
        </div>
        <?php
    }
    
    /**
     * Handle sync action
     */
    public function handle_sync() {
        if (!current_user_can('manage_options')) {
            wp_die(__('Keine Berechtigung', 'themisdb-order-request'));
        }
        
        $result = ThemisDB_EPServer_API::sync_all();
        
        if ($result['products']) {
            wp_redirect(admin_url('admin.php?page=themisdb-order-settings&sync=success'));
        } else {
            wp_redirect(admin_url('admin.php?page=themisdb-order-settings&sync=error'));
        }
        exit;
    }
}
