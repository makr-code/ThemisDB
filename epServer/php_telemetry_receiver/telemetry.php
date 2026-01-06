<?php
/**
 * ThemisDB Telemetry Receiver - PHP Implementation
 * 
 * Standalone PHP script for receiving telemetry heartbeats from ThemisDB instances.
 * Can be used as an alternative to the Python/FastAPI server.
 * 
 * Endpoints:
 * - POST /telemetry.php?action=heartbeat - Receive instance heartbeat
 * - GET /telemetry.php?action=statistics - Get global statistics
 * - GET /telemetry.php?action=license_instances&key=LICENSE_KEY - Get instances for license
 * - GET /telemetry.php?action=instance&id=INSTANCE_ID - Get instance details
 * - POST /telemetry.php?action=cleanup - Clean old data
 */

// Configuration
define('DB_HOST', getenv('DB_HOST') ?: 'localhost');
define('DB_NAME', getenv('DB_NAME') ?: 'themisdb_pricing');
define('DB_USER', getenv('DB_USER') ?: 'themisdb_user');
define('DB_PASS', getenv('DB_PASS') ?: 'secure_password');
define('DB_CHARSET', 'utf8mb4');

// Security settings
define('REQUIRE_HTTPS', getenv('REQUIRE_HTTPS') !== 'false'); // Set to 'false' for dev only
define('MAX_RETENTION_DAYS', 90);
define('RATE_LIMIT_SECONDS', 300); // 5 minutes minimum between heartbeats

// Headers
header('Content-Type: application/json');
header('X-Powered-By: ThemisDB Telemetry Receiver');

// CORS headers (configure for production)
header('Access-Control-Allow-Origin: *'); // TODO: Restrict in production
header('Access-Control-Allow-Methods: GET, POST, OPTIONS');
header('Access-Control-Allow-Headers: Content-Type, Authorization');

if ($_SERVER['REQUEST_METHOD'] === 'OPTIONS') {
    http_response_code(200);
    exit();
}

// HTTPS enforcement
if (REQUIRE_HTTPS && (!isset($_SERVER['HTTPS']) || $_SERVER['HTTPS'] !== 'on')) {
    error_response(400, 'HTTPS required');
}

/**
 * Database connection
 */
function get_db_connection() {
    static $pdo = null;
    
    if ($pdo === null) {
        try {
            $dsn = sprintf('mysql:host=%s;dbname=%s;charset=%s', DB_HOST, DB_NAME, DB_CHARSET);
            $options = [
                PDO::ATTR_ERRMODE => PDO::ERRMODE_EXCEPTION,
                PDO::ATTR_DEFAULT_FETCH_MODE => PDO::FETCH_ASSOC,
                PDO::ATTR_EMULATE_PREPARES => false,
            ];
            $pdo = new PDO($dsn, DB_USER, DB_PASS, $options);
        } catch (PDOException $e) {
            error_log('Database connection failed: ' . $e->getMessage());
            error_response(500, 'Database connection failed');
        }
    }
    
    return $pdo;
}

/**
 * Send JSON response
 */
function json_response($data, $status_code = 200) {
    http_response_code($status_code);
    echo json_encode($data, JSON_PRETTY_PRINT);
    exit();
}

/**
 * Send error response
 */
function error_response($status_code, $message, $details = null) {
    $response = [
        'error' => $message,
        'status_code' => $status_code,
    ];
    
    if ($details !== null) {
        $response['details'] = $details;
    }
    
    json_response($response, $status_code);
}

/**
 * Validate and get JSON input
 */
function get_json_input() {
    $input = file_get_contents('php://input');
    
    if (empty($input)) {
        error_response(400, 'Request body is empty');
    }
    
    $data = json_decode($input, true);
    
    if (json_last_error() !== JSON_ERROR_NONE) {
        error_response(400, 'Invalid JSON: ' . json_last_error_msg());
    }
    
    return $data;
}

/**
 * Validate license key exists
 */
function validate_license_key($pdo, $license_key) {
    $stmt = $pdo->prepare('SELECT id FROM subscriptions WHERE license_key = ? AND status = ?');
    $stmt->execute([$license_key, 'active']);
    $result = $stmt->fetch();
    
    if (!$result) {
        error_response(404, 'Invalid or inactive license key');
    }
    
    return $result['id'];
}

/**
 * Check rate limiting
 */
function check_rate_limit($pdo, $instance_id) {
    $stmt = $pdo->prepare('SELECT last_seen FROM instance_telemetry WHERE instance_id = ?');
    $stmt->execute([$instance_id]);
    $result = $stmt->fetch();
    
    if ($result) {
        $last_seen = strtotime($result['last_seen']);
        $now = time();
        
        if (($now - $last_seen) < RATE_LIMIT_SECONDS) {
            $wait_seconds = RATE_LIMIT_SECONDS - ($now - $last_seen);
            error_response(429, 'Rate limit exceeded', [
                'retry_after_seconds' => $wait_seconds
            ]);
        }
    }
}

/**
 * Handle heartbeat - POST /telemetry.php?action=heartbeat
 */
function handle_heartbeat() {
    $data = get_json_input();
    
    // Validate required fields
    $required = ['instance_id', 'license_key', 'metrics'];
    foreach ($required as $field) {
        if (!isset($data[$field])) {
            error_response(400, "Missing required field: $field");
        }
    }
    
    $instance_id = $data['instance_id'];
    $license_key = $data['license_key'];
    $metrics = $data['metrics'];
    $server_info = $data['server_info'] ?? [];
    
    // Validate metrics
    $metrics_required = ['nodes', 'cores', 'storage_tb', 'uptime_seconds', 'query_count_24h'];
    foreach ($metrics_required as $field) {
        if (!isset($metrics[$field])) {
            error_response(400, "Missing required metric: $field");
        }
    }
    
    $pdo = get_db_connection();
    
    // Validate license
    $subscription_id = validate_license_key($pdo, $license_key);
    
    // Check rate limit
    check_rate_limit($pdo, $instance_id);
    
    // Upsert telemetry data
    try {
        $pdo->beginTransaction();
        
        // Check if exists
        $stmt = $pdo->prepare('SELECT id, report_count FROM instance_telemetry WHERE instance_id = ?');
        $stmt->execute([$instance_id]);
        $existing = $stmt->fetch();
        
        $now = date('Y-m-d H:i:s');
        
        if ($existing) {
            // Update existing
            $stmt = $pdo->prepare('
                UPDATE instance_telemetry 
                SET subscription_id = ?,
                    hostname = ?,
                    version = ?,
                    nodes = ?,
                    cores = ?,
                    storage_tb = ?,
                    uptime_seconds = ?,
                    query_count_24h = ?,
                    location = ?,
                    last_seen = ?,
                    report_count = ?
                WHERE instance_id = ?
            ');
            
            $stmt->execute([
                $subscription_id,
                $server_info['hostname'] ?? null,
                $server_info['version'] ?? null,
                $metrics['nodes'],
                $metrics['cores'],
                $metrics['storage_tb'],
                $metrics['uptime_seconds'],
                $metrics['query_count_24h'],
                $server_info['location'] ?? null,
                $now,
                $existing['report_count'] + 1,
                $instance_id
            ]);
            
            $telemetry_id = $existing['id'];
        } else {
            // Insert new
            $stmt = $pdo->prepare('
                INSERT INTO instance_telemetry 
                (instance_id, subscription_id, hostname, version, nodes, cores, storage_tb, 
                 uptime_seconds, query_count_24h, location, first_seen, last_seen, report_count)
                VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)
            ');
            
            $stmt->execute([
                $instance_id,
                $subscription_id,
                $server_info['hostname'] ?? null,
                $server_info['version'] ?? null,
                $metrics['nodes'],
                $metrics['cores'],
                $metrics['storage_tb'],
                $metrics['uptime_seconds'],
                $metrics['query_count_24h'],
                $server_info['location'] ?? null,
                $now,
                $now,
                1
            ]);
            
            $telemetry_id = $pdo->lastInsertId();
        }
        
        $pdo->commit();
        
        json_response([
            'status' => 'success',
            'message' => 'Heartbeat received',
            'telemetry_id' => (int)$telemetry_id,
            'timestamp' => $now
        ], 200);
        
    } catch (PDOException $e) {
        $pdo->rollBack();
        error_log('Heartbeat error: ' . $e->getMessage());
        error_response(500, 'Failed to store telemetry data');
    }
}

/**
 * Get global statistics - GET /telemetry.php?action=statistics
 */
function handle_statistics() {
    $pdo = get_db_connection();
    
    try {
        // Total instances
        $stmt = $pdo->query('SELECT COUNT(*) as total FROM instance_telemetry');
        $total_instances = $stmt->fetch()['total'];
        
        // Active instances (seen in last 24 hours)
        $stmt = $pdo->query('SELECT COUNT(*) as active FROM instance_telemetry WHERE last_seen > DATE_SUB(NOW(), INTERVAL 24 HOUR)');
        $active_instances = $stmt->fetch()['active'];
        
        // Aggregate metrics
        $stmt = $pdo->query('
            SELECT 
                SUM(nodes) as total_nodes,
                SUM(cores) as total_cores,
                SUM(storage_tb) as total_storage_tb,
                SUM(query_count_24h) as total_queries_24h
            FROM instance_telemetry
            WHERE last_seen > DATE_SUB(NOW(), INTERVAL 24 HOUR)
        ');
        $metrics = $stmt->fetch();
        
        // Version distribution
        $stmt = $pdo->query('
            SELECT version, COUNT(*) as count 
            FROM instance_telemetry 
            WHERE version IS NOT NULL AND last_seen > DATE_SUB(NOW(), INTERVAL 24 HOUR)
            GROUP BY version 
            ORDER BY count DESC
        ');
        $version_distribution = $stmt->fetchAll();
        
        // Pricing tier distribution
        $stmt = $pdo->query('
            SELECT s.tier, COUNT(DISTINCT it.instance_id) as instance_count
            FROM instance_telemetry it
            JOIN subscriptions s ON it.subscription_id = s.id
            WHERE it.last_seen > DATE_SUB(NOW(), INTERVAL 24 HOUR)
            GROUP BY s.tier
            ORDER BY instance_count DESC
        ');
        $tier_distribution = $stmt->fetchAll();
        
        json_response([
            'total_instances' => (int)$total_instances,
            'active_instances' => (int)$active_instances,
            'total_nodes' => (int)($metrics['total_nodes'] ?? 0),
            'total_cores' => (int)($metrics['total_cores'] ?? 0),
            'total_storage_tb' => (float)($metrics['total_storage_tb'] ?? 0),
            'total_queries_24h' => (int)($metrics['total_queries_24h'] ?? 0),
            'version_distribution' => $version_distribution,
            'tier_distribution' => $tier_distribution,
            'timestamp' => date('Y-m-d H:i:s')
        ]);
        
    } catch (PDOException $e) {
        error_log('Statistics error: ' . $e->getMessage());
        error_response(500, 'Failed to retrieve statistics');
    }
}

/**
 * Get instances for license - GET /telemetry.php?action=license_instances&key=LICENSE_KEY
 */
function handle_license_instances() {
    if (!isset($_GET['key'])) {
        error_response(400, 'Missing license key parameter');
    }
    
    $license_key = $_GET['key'];
    $pdo = get_db_connection();
    
    try {
        $stmt = $pdo->prepare('
            SELECT 
                it.instance_id,
                it.hostname,
                it.version,
                it.nodes,
                it.cores,
                it.storage_tb,
                it.uptime_seconds,
                it.query_count_24h,
                it.location,
                it.first_seen,
                it.last_seen,
                it.report_count,
                CASE 
                    WHEN it.last_seen > DATE_SUB(NOW(), INTERVAL 24 HOUR) THEN TRUE 
                    ELSE FALSE 
                END as is_active
            FROM instance_telemetry it
            JOIN subscriptions s ON it.subscription_id = s.id
            WHERE s.license_key = ?
            ORDER BY it.last_seen DESC
        ');
        
        $stmt->execute([$license_key]);
        $instances = $stmt->fetchAll();
        
        json_response([
            'license_key' => $license_key,
            'instance_count' => count($instances),
            'instances' => $instances
        ]);
        
    } catch (PDOException $e) {
        error_log('License instances error: ' . $e->getMessage());
        error_response(500, 'Failed to retrieve instances');
    }
}

/**
 * Get instance details - GET /telemetry.php?action=instance&id=INSTANCE_ID
 */
function handle_instance_details() {
    if (!isset($_GET['id'])) {
        error_response(400, 'Missing instance ID parameter');
    }
    
    $instance_id = $_GET['id'];
    $pdo = get_db_connection();
    
    try {
        $stmt = $pdo->prepare('
            SELECT 
                it.*,
                s.license_key,
                s.tier,
                s.status as subscription_status,
                CASE 
                    WHEN it.last_seen > DATE_SUB(NOW(), INTERVAL 24 HOUR) THEN TRUE 
                    ELSE FALSE 
                END as is_active
            FROM instance_telemetry it
            JOIN subscriptions s ON it.subscription_id = s.id
            WHERE it.instance_id = ?
        ');
        
        $stmt->execute([$instance_id]);
        $instance = $stmt->fetch();
        
        if (!$instance) {
            error_response(404, 'Instance not found');
        }
        
        json_response($instance);
        
    } catch (PDOException $e) {
        error_log('Instance details error: ' . $e->getMessage());
        error_response(500, 'Failed to retrieve instance details');
    }
}

/**
 * Clean old data - POST /telemetry.php?action=cleanup
 */
function handle_cleanup() {
    $data = get_json_input();
    $days = $data['days'] ?? MAX_RETENTION_DAYS;
    
    if ($days < 1 || $days > 365) {
        error_response(400, 'Days must be between 1 and 365');
    }
    
    $pdo = get_db_connection();
    
    try {
        $stmt = $pdo->prepare('DELETE FROM instance_telemetry WHERE last_seen < DATE_SUB(NOW(), INTERVAL ? DAY)');
        $stmt->execute([$days]);
        $deleted = $stmt->rowCount();
        
        json_response([
            'status' => 'success',
            'deleted_count' => $deleted,
            'retention_days' => $days
        ]);
        
    } catch (PDOException $e) {
        error_log('Cleanup error: ' . $e->getMessage());
        error_response(500, 'Failed to clean old data');
    }
}

/**
 * Main router
 */
function main() {
    $action = $_GET['action'] ?? '';
    $method = $_SERVER['REQUEST_METHOD'];
    
    try {
        switch ($action) {
            case 'heartbeat':
                if ($method !== 'POST') {
                    error_response(405, 'Method not allowed', ['allowed' => 'POST']);
                }
                handle_heartbeat();
                break;
                
            case 'statistics':
                if ($method !== 'GET') {
                    error_response(405, 'Method not allowed', ['allowed' => 'GET']);
                }
                handle_statistics();
                break;
                
            case 'license_instances':
                if ($method !== 'GET') {
                    error_response(405, 'Method not allowed', ['allowed' => 'GET']);
                }
                handle_license_instances();
                break;
                
            case 'instance':
                if ($method !== 'GET') {
                    error_response(405, 'Method not allowed', ['allowed' => 'GET']);
                }
                handle_instance_details();
                break;
                
            case 'cleanup':
                if ($method !== 'POST') {
                    error_response(405, 'Method not allowed', ['allowed' => 'POST']);
                }
                handle_cleanup();
                break;
                
            default:
                json_response([
                    'name' => 'ThemisDB Telemetry Receiver',
                    'version' => '1.0.0',
                    'endpoints' => [
                        'POST /telemetry.php?action=heartbeat' => 'Receive instance heartbeat',
                        'GET /telemetry.php?action=statistics' => 'Get global statistics',
                        'GET /telemetry.php?action=license_instances&key=KEY' => 'Get instances for license',
                        'GET /telemetry.php?action=instance&id=ID' => 'Get instance details',
                        'POST /telemetry.php?action=cleanup' => 'Clean old data'
                    ]
                ]);
        }
    } catch (Exception $e) {
        error_log('Unhandled exception: ' . $e->getMessage());
        error_response(500, 'Internal server error');
    }
}

// Run
main();
