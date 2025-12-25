-- ThemisDB Enterprise Pricing Server - PostgreSQL Database Setup
-- Execute this script to create the production database and tables

-- =============================================================================
-- 1. DATABASE AND USER SETUP
-- =============================================================================
-- Run these commands as postgres superuser

-- Create database user
CREATE USER themis_pricing WITH PASSWORD 'CHANGE_THIS_PASSWORD_IN_PRODUCTION';

-- Create database
CREATE DATABASE themis_pricing
    WITH 
    OWNER = themis_pricing
    ENCODING = 'UTF8'
    LC_COLLATE = 'en_US.UTF-8'
    LC_CTYPE = 'en_US.UTF-8'
    TEMPLATE = template0;

-- Grant privileges
GRANT ALL PRIVILEGES ON DATABASE themis_pricing TO themis_pricing;

-- Connect to the database
\c themis_pricing

-- Grant schema privileges
GRANT ALL ON SCHEMA public TO themis_pricing;

-- =============================================================================
-- 2. EXTENSIONS
-- =============================================================================

-- Enable UUID generation (optional)
CREATE EXTENSION IF NOT EXISTS "uuid-ossp";

-- Enable pgcrypto for encryption functions
CREATE EXTENSION IF NOT EXISTS pgcrypto;

-- =============================================================================
-- 3. TABLES
-- =============================================================================

-- Customers table
CREATE TABLE IF NOT EXISTS customers (
    id SERIAL PRIMARY KEY,
    email VARCHAR(255) UNIQUE NOT NULL,
    hashed_password VARCHAR(255) NOT NULL,
    organization_name VARCHAR(255) NOT NULL,
    contact_name VARCHAR(255) NOT NULL,
    phone VARCHAR(50),
    country VARCHAR(100),
    is_active BOOLEAN DEFAULT TRUE,
    created_at TIMESTAMP WITH TIME ZONE DEFAULT CURRENT_TIMESTAMP,
    updated_at TIMESTAMP WITH TIME ZONE DEFAULT CURRENT_TIMESTAMP
);

-- Create index on email for fast lookups
CREATE INDEX idx_customers_email ON customers(email);
CREATE INDEX idx_customers_is_active ON customers(is_active);

-- Subscriptions table
CREATE TABLE IF NOT EXISTS subscriptions (
    id SERIAL PRIMARY KEY,
    customer_id INTEGER NOT NULL REFERENCES customers(id) ON DELETE CASCADE,
    tier VARCHAR(50) NOT NULL CHECK (tier IN ('community', 'enterprise', 'hyperscaler', 'reseller')),
    status VARCHAR(50) NOT NULL DEFAULT 'pending' CHECK (status IN ('active', 'pending', 'cancelled', 'expired', 'suspended')),
    license_key VARCHAR(100) UNIQUE NOT NULL,
    max_nodes INTEGER DEFAULT 1,
    max_cores INTEGER DEFAULT -1,
    max_storage_tb INTEGER DEFAULT -1,
    price_per_month DECIMAL(10, 2) NOT NULL,
    start_date TIMESTAMP WITH TIME ZONE,
    end_date TIMESTAMP WITH TIME ZONE,
    created_at TIMESTAMP WITH TIME ZONE DEFAULT CURRENT_TIMESTAMP,
    updated_at TIMESTAMP WITH TIME ZONE DEFAULT CURRENT_TIMESTAMP
);

-- Create indexes for subscriptions
CREATE INDEX idx_subscriptions_customer_id ON subscriptions(customer_id);
CREATE INDEX idx_subscriptions_license_key ON subscriptions(license_key);
CREATE INDEX idx_subscriptions_status ON subscriptions(status);
CREATE INDEX idx_subscriptions_end_date ON subscriptions(end_date);

-- Payments table
CREATE TABLE IF NOT EXISTS payments (
    id SERIAL PRIMARY KEY,
    customer_id INTEGER NOT NULL REFERENCES customers(id) ON DELETE CASCADE,
    subscription_id INTEGER NOT NULL REFERENCES subscriptions(id) ON DELETE CASCADE,
    amount DECIMAL(10, 2) NOT NULL,
    currency VARCHAR(3) DEFAULT 'EUR',
    status VARCHAR(50) NOT NULL DEFAULT 'pending' CHECK (status IN ('pending', 'completed', 'failed', 'refunded')),
    payment_method VARCHAR(100),
    transaction_id VARCHAR(255) UNIQUE,
    external_payment_id VARCHAR(255),
    created_at TIMESTAMP WITH TIME ZONE DEFAULT CURRENT_TIMESTAMP,
    updated_at TIMESTAMP WITH TIME ZONE DEFAULT CURRENT_TIMESTAMP
);

-- Create indexes for payments
CREATE INDEX idx_payments_customer_id ON payments(customer_id);
CREATE INDEX idx_payments_subscription_id ON payments(subscription_id);
CREATE INDEX idx_payments_transaction_id ON payments(transaction_id);
CREATE INDEX idx_payments_status ON payments(status);

-- Instance Telemetry table
CREATE TABLE IF NOT EXISTS instance_telemetry (
    id SERIAL PRIMARY KEY,
    license_key VARCHAR(255) NOT NULL REFERENCES subscriptions(license_key) ON DELETE CASCADE,
    instance_id VARCHAR(255) NOT NULL UNIQUE,
    hostname VARCHAR(255),
    version VARCHAR(50) NOT NULL,
    
    -- Metrics
    nodes_count INTEGER DEFAULT 1,
    total_cores INTEGER DEFAULT 0,
    used_storage_tb DECIMAL(10, 2) DEFAULT 0.0,
    uptime_seconds INTEGER DEFAULT 0,
    query_count_24h INTEGER DEFAULT 0,
    
    -- Geolocation
    country VARCHAR(2),
    region VARCHAR(100),
    
    -- Timestamps
    first_seen TIMESTAMP WITH TIME ZONE DEFAULT CURRENT_TIMESTAMP,
    last_seen TIMESTAMP WITH TIME ZONE DEFAULT CURRENT_TIMESTAMP,
    report_count INTEGER DEFAULT 1,
    
    -- User agent and IP
    user_agent TEXT,
    ip_address VARCHAR(45)
);

-- Create indexes for telemetry
CREATE INDEX idx_telemetry_license_key ON instance_telemetry(license_key);
CREATE INDEX idx_telemetry_instance_id ON instance_telemetry(instance_id);
CREATE INDEX idx_telemetry_last_seen ON instance_telemetry(last_seen);
CREATE INDEX idx_telemetry_version ON instance_telemetry(version);
CREATE INDEX idx_telemetry_country ON instance_telemetry(country);

-- =============================================================================
-- 4. TRIGGERS FOR UPDATED_AT
-- =============================================================================

-- Function to update updated_at timestamp
CREATE OR REPLACE FUNCTION update_updated_at_column()
RETURNS TRIGGER AS $$
BEGIN
    NEW.updated_at = CURRENT_TIMESTAMP;
    RETURN NEW;
END;
$$ language 'plpgsql';

-- Apply trigger to customers table
CREATE TRIGGER update_customers_updated_at
    BEFORE UPDATE ON customers
    FOR EACH ROW
    EXECUTE FUNCTION update_updated_at_column();

-- Apply trigger to subscriptions table
CREATE TRIGGER update_subscriptions_updated_at
    BEFORE UPDATE ON subscriptions
    FOR EACH ROW
    EXECUTE FUNCTION update_updated_at_column();

-- Apply trigger to payments table
CREATE TRIGGER update_payments_updated_at
    BEFORE UPDATE ON payments
    FOR EACH ROW
    EXECUTE FUNCTION update_updated_at_column();

-- =============================================================================
-- 5. VIEWS FOR REPORTING
-- =============================================================================

-- Active subscriptions view
CREATE OR REPLACE VIEW active_subscriptions AS
SELECT 
    s.id,
    s.license_key,
    s.tier,
    s.status,
    s.max_nodes,
    s.start_date,
    s.end_date,
    c.email,
    c.organization_name,
    c.contact_name
FROM subscriptions s
JOIN customers c ON s.customer_id = c.id
WHERE s.status = 'active'
  AND (s.end_date IS NULL OR s.end_date > CURRENT_TIMESTAMP);

-- Revenue summary view
CREATE OR REPLACE VIEW revenue_summary AS
SELECT 
    DATE_TRUNC('month', p.created_at) as month,
    COUNT(DISTINCT p.customer_id) as customer_count,
    COUNT(p.id) as payment_count,
    SUM(p.amount) as total_revenue,
    AVG(p.amount) as avg_payment
FROM payments p
WHERE p.status = 'completed'
GROUP BY DATE_TRUNC('month', p.created_at)
ORDER BY month DESC;

-- =============================================================================
-- 6. PERMISSIONS
-- =============================================================================

-- Grant permissions on tables
GRANT SELECT, INSERT, UPDATE, DELETE ON ALL TABLES IN SCHEMA public TO themis_pricing;
GRANT USAGE, SELECT ON ALL SEQUENCES IN SCHEMA public TO themis_pricing;

-- Grant permissions on views
GRANT SELECT ON active_subscriptions TO themis_pricing;
GRANT SELECT ON revenue_summary TO themis_pricing;

-- =============================================================================
-- 7. INITIAL DATA (Optional)
-- =============================================================================

-- You can add initial admin user or test data here if needed

-- =============================================================================
-- VERIFICATION
-- =============================================================================

-- Verify tables were created
SELECT tablename FROM pg_tables WHERE schemaname = 'public';

-- Verify indexes
SELECT indexname FROM pg_indexes WHERE schemaname = 'public';

-- Show database size
SELECT pg_size_pretty(pg_database_size('themis_pricing')) as database_size;

-- =============================================================================
-- BACKUP COMMAND
-- =============================================================================
-- To backup the database, run:
-- pg_dump -U themis_pricing -h localhost themis_pricing > backup_$(date +%Y%m%d).sql
--
-- To restore:
-- psql -U themis_pricing -h localhost themis_pricing < backup_20251221.sql
-- =============================================================================
