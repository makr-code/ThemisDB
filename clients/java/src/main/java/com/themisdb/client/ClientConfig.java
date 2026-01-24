package com.themisdb.client;

import java.time.Duration;

/**
 * Configuration for the ThemisDB client
 * 
 * Provides comprehensive configuration options including connection pooling,
 * retry logic, circuit breaker, and logging.
 */
public class ClientConfig {
    
    private final int maxRetries;
    private final Duration timeout;
    private final CircuitBreakerConfig circuitBreaker;
    private final LoggingConfig logging;
    private final ConnectionPoolConfig connectionPool;
    
    private ClientConfig(Builder builder) {
        this.maxRetries = builder.maxRetries;
        this.timeout = builder.timeout;
        this.circuitBreaker = builder.circuitBreaker;
        this.logging = builder.logging;
        this.connectionPool = builder.connectionPool;
    }
    
    public int getMaxRetries() {
        return maxRetries;
    }
    
    public Duration getTimeout() {
        return timeout;
    }
    
    public CircuitBreakerConfig getCircuitBreaker() {
        return circuitBreaker;
    }
    
    public LoggingConfig getLogging() {
        return logging;
    }
    
    public ConnectionPoolConfig getConnectionPool() {
        return connectionPool;
    }
    
    /**
     * Builder for ClientConfig
     */
    public static class Builder {
        private int maxRetries = 3;
        private Duration timeout = Duration.ofSeconds(30);
        private CircuitBreakerConfig circuitBreaker = null;
        private LoggingConfig logging = null;
        private ConnectionPoolConfig connectionPool = null;
        
        public Builder maxRetries(int maxRetries) {
            this.maxRetries = maxRetries;
            return this;
        }
        
        public Builder timeout(Duration timeout) {
            this.timeout = timeout;
            return this;
        }
        
        public Builder circuitBreaker(CircuitBreakerConfig circuitBreaker) {
            this.circuitBreaker = circuitBreaker;
            return this;
        }
        
        public Builder logging(LoggingConfig logging) {
            this.logging = logging;
            return this;
        }
        
        public Builder connectionPool(ConnectionPoolConfig connectionPool) {
            this.connectionPool = connectionPool;
            return this;
        }
        
        public ClientConfig build() {
            return new ClientConfig(this);
        }
    }
    
    /**
     * Circuit breaker configuration
     */
    public static class CircuitBreakerConfig {
        private final boolean enabled;
        private final int failureThreshold;
        private final Duration resetTimeout;
        private final int halfOpenMaxRequests;
        
        private CircuitBreakerConfig(Builder builder) {
            this.enabled = builder.enabled;
            this.failureThreshold = builder.failureThreshold;
            this.resetTimeout = builder.resetTimeout;
            this.halfOpenMaxRequests = builder.halfOpenMaxRequests;
        }
        
        public boolean isEnabled() {
            return enabled;
        }
        
        public int getFailureThreshold() {
            return failureThreshold;
        }
        
        public Duration getResetTimeout() {
            return resetTimeout;
        }
        
        public int getHalfOpenMaxRequests() {
            return halfOpenMaxRequests;
        }
        
        public static class Builder {
            private boolean enabled = true;
            private int failureThreshold = 5;
            private Duration resetTimeout = Duration.ofSeconds(60);
            private int halfOpenMaxRequests = 3;
            
            public Builder enabled(boolean enabled) {
                this.enabled = enabled;
                return this;
            }
            
            public Builder failureThreshold(int failureThreshold) {
                this.failureThreshold = failureThreshold;
                return this;
            }
            
            public Builder resetTimeout(Duration resetTimeout) {
                this.resetTimeout = resetTimeout;
                return this;
            }
            
            public Builder halfOpenMaxRequests(int halfOpenMaxRequests) {
                this.halfOpenMaxRequests = halfOpenMaxRequests;
                return this;
            }
            
            public CircuitBreakerConfig build() {
                return new CircuitBreakerConfig(this);
            }
        }
    }
    
    /**
     * Logging configuration
     */
    public static class LoggingConfig {
        private final boolean enabled;
        private final boolean logRequests;
        private final boolean logResponses;
        private final Logger logger;
        
        private LoggingConfig(Builder builder) {
            this.enabled = builder.enabled;
            this.logRequests = builder.logRequests;
            this.logResponses = builder.logResponses;
            this.logger = builder.logger != null ? builder.logger : new DefaultLogger();
        }
        
        public boolean isEnabled() {
            return enabled;
        }
        
        public boolean isLogRequests() {
            return logRequests;
        }
        
        public boolean isLogResponses() {
            return logResponses;
        }
        
        public Logger getLogger() {
            return logger;
        }
        
        public static class Builder {
            private boolean enabled = true;
            private boolean logRequests = true;
            private boolean logResponses = true;
            private Logger logger = null;
            
            public Builder enabled(boolean enabled) {
                this.enabled = enabled;
                return this;
            }
            
            public Builder logRequests(boolean logRequests) {
                this.logRequests = logRequests;
                return this;
            }
            
            public Builder logResponses(boolean logResponses) {
                this.logResponses = logResponses;
                return this;
            }
            
            public Builder logger(Logger logger) {
                this.logger = logger;
                return this;
            }
            
            public LoggingConfig build() {
                return new LoggingConfig(this);
            }
        }
    }
    
    /**
     * Logger interface
     */
    public interface Logger {
        void log(String message, Level level);
        
        enum Level {
            INFO, WARN, ERROR
        }
    }
    
    /**
     * Default logger implementation
     */
    private static class DefaultLogger implements Logger {
        @Override
        public void log(String message, Level level) {
            System.out.println("[ThemisDB] [" + level + "] " + message);
        }
    }
    
    /**
     * Connection pool configuration
     */
    public static class ConnectionPoolConfig {
        private final int maxConnections;
        private final int maxConnectionsPerRoute;
        private final Duration keepAliveTimeout;
        private final Duration idleTimeout;
        
        private ConnectionPoolConfig(Builder builder) {
            this.maxConnections = builder.maxConnections;
            this.maxConnectionsPerRoute = builder.maxConnectionsPerRoute;
            this.keepAliveTimeout = builder.keepAliveTimeout;
            this.idleTimeout = builder.idleTimeout;
        }
        
        public int getMaxConnections() {
            return maxConnections;
        }
        
        public int getMaxConnectionsPerRoute() {
            return maxConnectionsPerRoute;
        }
        
        public Duration getKeepAliveTimeout() {
            return keepAliveTimeout;
        }
        
        public Duration getIdleTimeout() {
            return idleTimeout;
        }
        
        public static class Builder {
            private int maxConnections = 100;
            private int maxConnectionsPerRoute = 50;
            private Duration keepAliveTimeout = Duration.ofSeconds(60);
            private Duration idleTimeout = Duration.ofSeconds(30);
            
            public Builder maxConnections(int maxConnections) {
                this.maxConnections = maxConnections;
                return this;
            }
            
            public Builder maxConnectionsPerRoute(int maxConnectionsPerRoute) {
                this.maxConnectionsPerRoute = maxConnectionsPerRoute;
                return this;
            }
            
            public Builder keepAliveTimeout(Duration keepAliveTimeout) {
                this.keepAliveTimeout = keepAliveTimeout;
                return this;
            }
            
            public Builder idleTimeout(Duration idleTimeout) {
                this.idleTimeout = idleTimeout;
                return this;
            }
            
            public ConnectionPoolConfig build() {
                return new ConnectionPoolConfig(this);
            }
        }
    }
}
