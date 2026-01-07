-- ThemisDB Telemetry Receiver - Database Setup for PHP Implementation
-- Compatible with MySQL/MariaDB

-- Create database (if not exists)
CREATE DATABASE IF NOT EXISTS themisdb_pricing CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci;
USE themisdb_pricing;

-- Subscriptions table (simplified for standalone deployment)
CREATE TABLE IF NOT EXISTS subscriptions (
    id INT AUTO_INCREMENT PRIMARY KEY,
    license_key VARCHAR(255) NOT NULL UNIQUE,
    tier VARCHAR(50) NOT NULL,
    status VARCHAR(20) NOT NULL DEFAULT 'active',
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    updated_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
    INDEX idx_license_key (license_key),
    INDEX idx_status (status)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

-- Instance telemetry table
CREATE TABLE IF NOT EXISTS instance_telemetry (
    id INT AUTO_INCREMENT PRIMARY KEY,
    instance_id VARCHAR(255) NOT NULL UNIQUE,
    subscription_id INT NOT NULL,
    hostname VARCHAR(255),
    version VARCHAR(50),
    nodes INT NOT NULL DEFAULT 0,
    cores INT NOT NULL DEFAULT 0,
    storage_tb DECIMAL(10, 2) NOT NULL DEFAULT 0.00,
    uptime_seconds BIGINT NOT NULL DEFAULT 0,
    query_count_24h BIGINT NOT NULL DEFAULT 0,
    location VARCHAR(255),
    first_seen TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    last_seen TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    report_count INT NOT NULL DEFAULT 1,
    INDEX idx_instance_id (instance_id),
    INDEX idx_subscription_id (subscription_id),
    INDEX idx_last_seen (last_seen),
    INDEX idx_version (version),
    INDEX idx_location (location),
    FOREIGN KEY (subscription_id) REFERENCES subscriptions(id) ON DELETE CASCADE
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

-- Create database user (change password!)
-- Uncomment and run separately with root privileges:
-- CREATE USER IF NOT EXISTS 'themisdb_user'@'localhost' IDENTIFIED BY 'secure_password_here';
-- GRANT SELECT, INSERT, UPDATE, DELETE ON themisdb_pricing.* TO 'themisdb_user'@'localhost';
-- FLUSH PRIVILEGES;

-- Insert sample data for testing
INSERT INTO subscriptions (license_key, tier, status) VALUES
    ('THEMIS-COMMUNITY-0000-TEST1', 'community', 'active'),
    ('THEMIS-ENT-1234-TEST2', 'enterprise', 'active'),
    ('THEMIS-HYPER-5678-TEST3', 'hyperscaler', 'active')
ON DUPLICATE KEY UPDATE tier = VALUES(tier);

-- View for active instances (last 24 hours)
CREATE OR REPLACE VIEW active_instances AS
SELECT 
    it.instance_id,
    it.hostname,
    it.version,
    it.nodes,
    it.cores,
    it.storage_tb,
    it.location,
    it.last_seen,
    s.license_key,
    s.tier
FROM instance_telemetry it
JOIN subscriptions s ON it.subscription_id = s.id
WHERE it.last_seen > DATE_SUB(NOW(), INTERVAL 24 HOUR);

-- View for global statistics
CREATE OR REPLACE VIEW telemetry_statistics AS
SELECT 
    COUNT(*) as total_instances,
    SUM(CASE WHEN last_seen > DATE_SUB(NOW(), INTERVAL 24 HOUR) THEN 1 ELSE 0 END) as active_instances,
    SUM(nodes) as total_nodes,
    SUM(cores) as total_cores,
    SUM(storage_tb) as total_storage_tb,
    SUM(query_count_24h) as total_queries_24h
FROM instance_telemetry;

-- Cleanup old data procedure
DELIMITER //
CREATE PROCEDURE IF NOT EXISTS cleanup_old_telemetry(IN days INT)
BEGIN
    DELETE FROM instance_telemetry 
    WHERE last_seen < DATE_SUB(NOW(), INTERVAL days DAY);
    
    SELECT ROW_COUNT() as deleted_count;
END //
DELIMITER ;

-- Event for automatic cleanup (runs daily at 2 AM)
-- Note: Requires event_scheduler to be enabled
-- SET GLOBAL event_scheduler = ON;
CREATE EVENT IF NOT EXISTS daily_telemetry_cleanup
ON SCHEDULE EVERY 1 DAY
STARTS TIMESTAMP(CURRENT_DATE + INTERVAL 1 DAY, '02:00:00')
DO
    CALL cleanup_old_telemetry(90);

SHOW TABLES;
