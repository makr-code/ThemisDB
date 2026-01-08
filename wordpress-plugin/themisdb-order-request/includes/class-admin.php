<?php
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
            __('E-Mail Log', 'themisdb-order-request'),
            __('E-Mail Log', 'themisdb-order-request'),
            'manage_options',
            'themisdb-email-log',
            array($this, 'email_log_page')
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
    }
    
    /**
     * Orders page
     */
    public function orders_page() {
        $action = isset($_GET['action']) ? $_GET['action'] : 'list';
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
                            <td><?php echo date('d.m.Y H:i', strtotime($order['created_at'])); ?></td>
                            <td>
                                <a href="?page=themisdb-orders&action=view&order_id=<?php echo $order['id']; ?>" class="button button-small">
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
                                <a href="?page=themisdb-contracts&action=view&contract_id=<?php echo $contract['id']; ?>" class="button button-small">
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
        $action = isset($_GET['action']) ? $_GET['action'] : 'list';
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
                            <td><?php echo date('d.m.Y', strtotime($contract['valid_from'])); ?></td>
                            <td><?php echo $contract['valid_until'] ? date('d.m.Y', strtotime($contract['valid_until'])) : '-'; ?></td>
                            <td><?php echo date('d.m.Y', strtotime($contract['created_at'])); ?></td>
                            <td>
                                <a href="?page=themisdb-contracts&action=view&contract_id=<?php echo $contract['id']; ?>" class="button button-small">
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
