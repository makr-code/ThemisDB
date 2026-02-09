package themisdb

import (
	"crypto/tls"
	"os"
	"path/filepath"
	"testing"

	"github.com/stretchr/testify/assert"
	"github.com/stretchr/testify/require"
)

// TestTLSConfig_NewTLSConfig tests default TLS configuration
func TestTLSConfig_NewTLSConfig(t *testing.T) {
	config := NewTLSConfig()
	
	assert.NotNil(t, config)
	assert.True(t, config.Enabled)
	assert.Equal(t, uint16(tls.VersionTLS12), config.MinVersion)
	assert.False(t, config.ProductionMode)
	assert.False(t, config.InsecureSkipVerify)
}

// TestTLSConfig_NewProductionTLSConfig tests production TLS configuration
func TestTLSConfig_NewProductionTLSConfig(t *testing.T) {
	config := NewProductionTLSConfig("/path/to/ca.crt")
	
	assert.NotNil(t, config)
	assert.True(t, config.Enabled)
	assert.Equal(t, "/path/to/ca.crt", config.CACertPath)
	assert.Equal(t, uint16(tls.VersionTLS13), config.MinVersion)
	assert.True(t, config.ProductionMode)
}

// TestTLSConfig_Validate_ProductionModeWithoutTLS tests production mode enforcement
func TestTLSConfig_Validate_ProductionModeWithoutTLS(t *testing.T) {
	config := &TLSConfig{
		Enabled:        false,
		ProductionMode: true,
	}
	
	err := config.Validate()
	require.Error(t, err)
	assert.Contains(t, err.Error(), "TLS is REQUIRED in production mode")
}

// TestTLSConfig_Validate_InsecureSkipVerifyInProduction tests insecure config in production
func TestTLSConfig_Validate_InsecureSkipVerifyInProduction(t *testing.T) {
	config := &TLSConfig{
		Enabled:            true,
		InsecureSkipVerify: true,
		ProductionMode:     true,
	}
	
	err := config.Validate()
	require.Error(t, err)
	assert.Contains(t, err.Error(), "InsecureSkipVerify cannot be used in production mode")
}

// TestTLSConfig_Validate_MinimumTLSVersion tests TLS version enforcement
func TestTLSConfig_Validate_MinimumTLSVersion(t *testing.T) {
	config := &TLSConfig{
		Enabled:    true,
		MinVersion: tls.VersionTLS10, // Too old
	}
	
	err := config.Validate()
	require.Error(t, err)
	assert.Contains(t, err.Error(), "TLS version must be at least TLS 1.2")
}

// TestTLSConfig_Validate_CACertNotFound tests missing CA certificate
func TestTLSConfig_Validate_CACertNotFound(t *testing.T) {
	config := &TLSConfig{
		Enabled:    true,
		CACertPath: "/nonexistent/ca.crt",
	}
	
	err := config.Validate()
	require.Error(t, err)
	assert.Contains(t, err.Error(), "CA certificate file not found")
}

// TestTLSConfig_Validate_IncompleteMTLSConfig tests incomplete mTLS configuration
func TestTLSConfig_Validate_IncompleteMTLSConfig(t *testing.T) {
	tests := []struct {
		name           string
		clientCertPath string
		clientKeyPath  string
	}{
		{
			name:           "cert without key",
			clientCertPath: "/path/to/cert.pem",
			clientKeyPath:  "",
		},
		{
			name:           "key without cert",
			clientCertPath: "",
			clientKeyPath:  "/path/to/key.pem",
		},
	}
	
	for _, tt := range tests {
		t.Run(tt.name, func(t *testing.T) {
			config := &TLSConfig{
				Enabled:        true,
				ClientCertPath: tt.clientCertPath,
				ClientKeyPath:  tt.clientKeyPath,
			}
			
			err := config.Validate()
			require.Error(t, err)
			assert.Contains(t, err.Error(), "Both ClientCertPath and ClientKeyPath must be provided")
		})
	}
}

// TestTLSConfig_Validate_ValidConfiguration tests valid configurations
func TestTLSConfig_Validate_ValidConfiguration(t *testing.T) {
	tests := []struct {
		name   string
		config *TLSConfig
	}{
		{
			name: "TLS disabled (development)",
			config: &TLSConfig{
				Enabled:        false,
				ProductionMode: false,
			},
		},
		{
			name: "TLS enabled with InsecureSkipVerify (testing)",
			config: &TLSConfig{
				Enabled:            true,
				InsecureSkipVerify: true,
				MinVersion:         tls.VersionTLS12, // Need to specify min version
				ProductionMode:     false,
			},
		},
		{
			name: "TLS enabled without CA cert (system pool)",
			config: &TLSConfig{
				Enabled:    true,
				MinVersion: tls.VersionTLS12,
			},
		},
	}
	
	for _, tt := range tests {
		t.Run(tt.name, func(t *testing.T) {
			err := tt.config.Validate()
			assert.NoError(t, err)
		})
	}
}

// TestTLSConfig_BuildTLSConfig tests TLS config building
func TestTLSConfig_BuildTLSConfig(t *testing.T) {
	t.Run("disabled TLS returns nil", func(t *testing.T) {
		config := &TLSConfig{
			Enabled: false,
		}
		
		tlsCfg, err := config.BuildTLSConfig()
		require.NoError(t, err)
		assert.Nil(t, tlsCfg)
	})
	
	t.Run("basic TLS config", func(t *testing.T) {
		config := &TLSConfig{
			Enabled:            true,
			MinVersion:         tls.VersionTLS12,
			InsecureSkipVerify: true,
			ServerName:         "example.com",
		}
		
		tlsCfg, err := config.BuildTLSConfig()
		require.NoError(t, err)
		assert.NotNil(t, tlsCfg)
		assert.Equal(t, uint16(tls.VersionTLS12), tlsCfg.MinVersion)
		assert.True(t, tlsCfg.InsecureSkipVerify)
		assert.Equal(t, "example.com", tlsCfg.ServerName)
	})
}

// TestNewWireClientWithTLS tests TLS client creation
func TestNewWireClientWithTLS(t *testing.T) {
	t.Run("valid TLS config", func(t *testing.T) {
		tlsConfig := &TLSConfig{
			Enabled:            true,
			InsecureSkipVerify: true,
			MinVersion:         tls.VersionTLS12,
		}
		
		client, err := NewWireClientWithTLS("localhost", 18765, "user", "pass", tlsConfig)
		require.NoError(t, err)
		assert.NotNil(t, client)
		assert.True(t, client.IsTLSEnabled())
		assert.Equal(t, tlsConfig, client.GetTLSConfig())
	})
	
	t.Run("invalid TLS config", func(t *testing.T) {
		tlsConfig := &TLSConfig{
			Enabled:        false,
			ProductionMode: true, // Invalid: production mode without TLS
		}
		
		client, err := NewWireClientWithTLS("localhost", 18765, "user", "pass", tlsConfig)
		require.Error(t, err)
		assert.Nil(t, client)
		assert.Contains(t, err.Error(), "TLS is REQUIRED in production mode")
	})
}

// TestNewWireClient_NoTLS tests backward compatibility
func TestNewWireClient_NoTLS(t *testing.T) {
	client := NewWireClient("localhost", 18765, "user", "pass")
	
	assert.NotNil(t, client)
	assert.False(t, client.IsTLSEnabled())
	assert.Nil(t, client.GetTLSConfig())
}

// TestNewWireClientFromEnv tests environment variable configuration
func TestNewWireClientFromEnv(t *testing.T) {
	// Save original environment
	originalEnv := make(map[string]string)
	envVars := []string{
		"THEMIS_WIRE_HOST",
		"THEMIS_WIRE_PORT",
		"THEMIS_WIRE_USERNAME",
		"THEMIS_WIRE_PASSWORD",
		"THEMIS_WIRE_TLS_ENABLED",
		"THEMIS_WIRE_TLS_CA_CERT",
		"THEMIS_WIRE_TLS_INSECURE_SKIP_VERIFY",
		"THEMIS_WIRE_PRODUCTION_MODE",
	}
	for _, key := range envVars {
		originalEnv[key] = os.Getenv(key)
	}
	
	// Restore environment after test
	defer func() {
		for key, value := range originalEnv {
			if value == "" {
				os.Unsetenv(key)
			} else {
				os.Setenv(key, value)
			}
		}
	}()
	
	t.Run("default configuration", func(t *testing.T) {
		// Clear all environment variables
		for _, key := range envVars {
			os.Unsetenv(key)
		}
		
		client, err := NewWireClientFromEnv()
		require.NoError(t, err)
		assert.NotNil(t, client)
		assert.Equal(t, "localhost", client.host)
		assert.Equal(t, 18765, client.port)
		assert.False(t, client.IsTLSEnabled())
	})
	
	t.Run("TLS enabled via environment", func(t *testing.T) {
		os.Setenv("THEMIS_WIRE_HOST", "db.example.com")
		os.Setenv("THEMIS_WIRE_PORT", "9999")
		os.Setenv("THEMIS_WIRE_TLS_ENABLED", "true")
		os.Setenv("THEMIS_WIRE_TLS_INSECURE_SKIP_VERIFY", "true")
		
		client, err := NewWireClientFromEnv()
		require.NoError(t, err)
		assert.NotNil(t, client)
		assert.Equal(t, "db.example.com", client.host)
		assert.Equal(t, 9999, client.port)
		assert.True(t, client.IsTLSEnabled())
		assert.True(t, client.tlsConfig.InsecureSkipVerify)
	})
	
	t.Run("production mode enforcement", func(t *testing.T) {
		os.Setenv("THEMIS_WIRE_PRODUCTION_MODE", "true")
		os.Setenv("THEMIS_WIRE_TLS_ENABLED", "false")
		
		client, err := NewWireClientFromEnv()
		require.Error(t, err)
		assert.Nil(t, client)
		assert.Contains(t, err.Error(), "TLS is REQUIRED in production mode")
	})
}

// TestTLSVersionString tests TLS version string helper
func TestTLSVersionString(t *testing.T) {
	tests := []struct {
		version  uint16
		expected string
	}{
		{tls.VersionTLS10, "TLS 1.0"},
		{tls.VersionTLS11, "TLS 1.1"},
		{tls.VersionTLS12, "TLS 1.2"},
		{tls.VersionTLS13, "TLS 1.3"},
		{0x9999, "Unknown (0x9999)"},
	}
	
	for _, tt := range tests {
		t.Run(tt.expected, func(t *testing.T) {
			result := tlsVersionString(tt.version)
			assert.Equal(t, tt.expected, result)
		})
	}
}

// TestTLSConfig_BuildTLSConfig_WithFiles tests TLS config with actual files
func TestTLSConfig_BuildTLSConfig_WithFiles(t *testing.T) {
	// Create temporary test files
	tmpDir := t.TempDir()
	
	// Create dummy CA certificate (this won't be a valid cert, just for file existence testing)
	caCertPath := filepath.Join(tmpDir, "ca.crt")
	err := os.WriteFile(caCertPath, []byte("-----BEGIN CERTIFICATE-----\ntest\n-----END CERTIFICATE-----"), 0600)
	require.NoError(t, err)
	
	t.Run("with CA certificate file", func(t *testing.T) {
		config := &TLSConfig{
			Enabled:    true,
			CACertPath: caCertPath,
			MinVersion: tls.VersionTLS12,
		}
		
		// Validate should pass
		err := config.Validate()
		assert.NoError(t, err)
		
		// BuildTLSConfig will fail because it's not a valid cert, but that's expected
		_, err = config.BuildTLSConfig()
		// We expect an error here because the cert content is invalid
		// but the important thing is that the file was found and read
		assert.Error(t, err)
		assert.Contains(t, err.Error(), "Failed to parse CA certificate")
	})
}
