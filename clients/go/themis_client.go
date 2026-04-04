package themisdb

import (
	"bytes"
	"crypto/tls"
	"crypto/x509"
	"encoding/binary"
	"encoding/json"
	"fmt"
	"hash/crc32"
	"io"
	"log"
	"net"
	"os"
	"strings"
	"sync"
	"time"
)

// Wire Protocol v1 Constants
const (
	WIRE_MAGIC          = 0x544D4442 // "TMDB"
	WIRE_VERSION        = 0x01
	HEADER_SIZE         = 12
	CHECKSUM_SIZE       = 4
	MAX_PAYLOAD_SIZE    = 64 * 1024 * 1024 // 64MB
	DEFAULT_TIMEOUT     = 30 * time.Second
	REQUEST_TIMEOUT     = 5 * time.Second
)

// OpCode definitions
const (
	// Connection opcodes
	OpCodeHello          = 0x01
	OpCodeHelloAck       = 0x02
	OpCodeAuthRequest    = 0x03
	OpCodeAuthResponse   = 0x04
	OpCodeAuthSuccess    = 0x05
	OpCodeAuthFailure    = 0x06

	// CRUD opcodes
	OpCodeGet            = 0x10
	OpCodePut            = 0x11
	OpCodeDelete         = 0x12
	OpCodeBatchGet       = 0x13
	OpCodeBatchPut       = 0x14

	// Query opcodes
	OpCodeQueryAQL       = 0x20
	OpCodeQueryResult    = 0x21
	OpCodeQueryCursor    = 0x22
	OpCodeCursorNext     = 0x23
	OpCodeCursorClose    = 0x24

	// Transaction opcodes
	OpCodeTransBegin     = 0x30
	OpCodeTransCommit    = 0x31
	OpCodeTransAbort     = 0x32

	// Advanced opcodes
	OpCodeVectorSearch   = 0x40
	OpCodeGraphTraverse  = 0x41
	OpCodeGeoQuery       = 0x50
	OpCodeTimeseriesQuery = 0x51
	OpCodeBPMNStartProc  = 0x60
	OpCodeBPMNTaskComp   = 0x61
	OpCodeBPMNQueryInst  = 0x62

	// System opcodes
	OpCodeError          = 0xF0
	OpCodeOK             = 0xF1
	OpCodePing           = 0xFE
	OpCodeClose          = 0xFF
)

// Message Flags
const (
	FlagNone          = 0x0000
	FlagSkipChecksum  = 0x0001
	FlagCompressed    = 0x0002
	FlagEncrypted     = 0x0004
)

// Exceptions
type ThemisDBError struct {
	Message string
}

func (e *ThemisDBError) Error() string {
	return e.Message
}

type ConnectionError struct {
	*ThemisDBError
}

type AuthenticationError struct {
	*ThemisDBError
}

type TLSConfigurationError struct {
	*ThemisDBError
}

// WireFrame represents a network message
type WireFrame struct {
	Version      byte
	Opcode       byte
	Flags        uint16
	PayloadSize  uint32
	Sequence     uint32
	Payload      []byte
}

// ToBytes serializes WireFrame to bytes
func (wf *WireFrame) ToBytes() ([]byte, error) {
	buf := new(bytes.Buffer)

	// Write header
	binary.Write(buf, binary.BigEndian, uint32(WIRE_MAGIC))
	binary.Write(buf, binary.BigEndian, wf.Version)
	binary.Write(buf, binary.BigEndian, wf.Opcode)
	binary.Write(buf, binary.BigEndian, wf.Flags)
	binary.Write(buf, binary.BigEndian, uint32(len(wf.Payload)))

	// Write payload
	buf.Write(wf.Payload)

	// Calculate and write checksum
	if (wf.Flags & FlagSkipChecksum) == 0 {
		headerAndPayload := buf.Bytes()
		checksum := crc32.ChecksumIEEE(headerAndPayload)
		binary.Write(buf, binary.BigEndian, checksum)
	}

	return buf.Bytes(), nil
}

// FromBytes deserializes WireFrame from bytes
func FromBytes(data []byte, offset int) (*WireFrame, error) {
	if len(data)-offset < HEADER_SIZE {
		return nil, &ThemisDBError{Message: "Incomplete frame"}
	}

	reader := bytes.NewReader(data[offset:])

	// Read header
	var magic uint32
	binary.Read(reader, binary.BigEndian, &magic)
	if magic != WIRE_MAGIC {
		return nil, fmt.Errorf("Invalid magic: 0x%x", magic)
	}

	var version, opcode byte
	var flags uint16
	var payloadSize uint32

	binary.Read(reader, binary.BigEndian, &version)
	binary.Read(reader, binary.BigEndian, &opcode)
	binary.Read(reader, binary.BigEndian, &flags)
	binary.Read(reader, binary.BigEndian, &payloadSize)

	if payloadSize > MAX_PAYLOAD_SIZE {
		return nil, fmt.Errorf("Payload too large: %d", payloadSize)
	}

	// Read payload
	payload := make([]byte, payloadSize)
	_, err := io.ReadFull(reader, payload)
	if err != nil {
		return nil, err
	}

	// Read and verify checksum
	if (flags & FlagSkipChecksum) == 0 {
		var checksum uint32
		err = binary.Read(reader, binary.BigEndian, &checksum)
		if err != nil {
			return nil, err
		}

		headerAndPayload := data[offset : offset+HEADER_SIZE+int(payloadSize)]
		calculatedChecksum := crc32.ChecksumIEEE(headerAndPayload)
		if calculatedChecksum != checksum {
			return nil, &ThemisDBError{Message: "Checksum mismatch"}
		}
	}

	return &WireFrame{
		Version:     version,
		Opcode:      opcode,
		Flags:       flags,
		PayloadSize: payloadSize,
		Payload:     payload,
	}, nil
}

// Client represents ThemisDB connection
// WireClient represents a ThemisDB native wire protocol client
type WireClient struct {
	conn              net.Conn
	host              string
	port              int
	username          string
	password          string
	authenticated     bool
	sequence          uint32
	sequenceMu        sync.Mutex
	pendingRequests   map[uint32]chan *WireFrame
	pendingMu         sync.RWMutex
	receiveBuffer     []byte
	receiveMu         sync.Mutex
	running           bool
	done              chan struct{}
	requestTimeout    time.Duration
	connectionTimeout time.Duration
	tlsConfig         *TLSConfig
}

// TLSConfig holds TLS/mTLS configuration for Wire Protocol connections
type TLSConfig struct {
	// Enable TLS (required for production)
	Enabled bool
	
	// CA certificate path for server verification
	CACertPath string
	
	// Client certificate and key for mTLS (optional)
	ClientCertPath string
	ClientKeyPath  string
	
	// Minimum TLS version (default: TLS 1.2)
	MinVersion uint16
	
	// Skip certificate verification (INSECURE - only for testing)
	InsecureSkipVerify bool
	
	// Server name for certificate verification
	ServerName string
	
	// Enforce production-safe behavior: fail if TLS is disabled in production
	ProductionMode bool
}

// NewTLSConfig creates a secure TLS configuration with defaults
func NewTLSConfig() *TLSConfig {
	return &TLSConfig{
		Enabled:        true,
		MinVersion:     tls.VersionTLS12,
		ProductionMode: false,
	}
}

// NewProductionTLSConfig creates a TLS configuration enforcing production security
func NewProductionTLSConfig(caCertPath string) *TLSConfig {
	return &TLSConfig{
		Enabled:        true,
		CACertPath:     caCertPath,
		MinVersion:     tls.VersionTLS13, // Enforce TLS 1.3 for production
		ProductionMode: true,
	}
}

// Validate checks if the TLS configuration is valid and safe for production
func (tc *TLSConfig) Validate() error {
	if tc == nil {
		return &TLSConfigurationError{&ThemisDBError{Message: "TLS configuration is nil"}}
	}
	
	// Production mode enforcement
	if tc.ProductionMode && !tc.Enabled {
		return &TLSConfigurationError{&ThemisDBError{
			Message: "SECURITY VIOLATION: TLS is REQUIRED in production mode but is disabled. " +
				"Set TLSConfig.Enabled=true or disable ProductionMode (NOT RECOMMENDED)",
		}}
	}
	
	// If TLS is enabled, validate required configuration
	if tc.Enabled {
		// Warn about insecure configurations
		if tc.InsecureSkipVerify {
			log.Println("WARNING: InsecureSkipVerify is enabled. This is INSECURE and should only be used for testing.")
			if tc.ProductionMode {
				return &TLSConfigurationError{&ThemisDBError{
					Message: "SECURITY VIOLATION: InsecureSkipVerify cannot be used in production mode",
				}}
			}
		}
		
		// Validate CA certificate if provided
		if tc.CACertPath != "" {
			if _, err := os.Stat(tc.CACertPath); err != nil {
				if os.IsNotExist(err) {
					return &TLSConfigurationError{&ThemisDBError{
						Message: fmt.Sprintf("CA certificate file not found: %s", tc.CACertPath),
					}}
				}
				return &TLSConfigurationError{&ThemisDBError{
					Message: fmt.Sprintf("CA certificate file not accessible: %s (%v)", tc.CACertPath, err),
				}}
			}
		} else if !tc.InsecureSkipVerify {
			log.Println("INFO: No CA certificate provided, using system certificate pool")
		}
		
		// Validate client certificates for mTLS
		if tc.ClientCertPath != "" || tc.ClientKeyPath != "" {
			if tc.ClientCertPath == "" || tc.ClientKeyPath == "" {
				return &TLSConfigurationError{&ThemisDBError{
					Message: "Both ClientCertPath and ClientKeyPath must be provided for mTLS",
				}}
			}
			if _, err := os.Stat(tc.ClientCertPath); err != nil {
				if os.IsNotExist(err) {
					return &TLSConfigurationError{&ThemisDBError{
						Message: fmt.Sprintf("Client certificate file not found: %s", tc.ClientCertPath),
					}}
				}
				return &TLSConfigurationError{&ThemisDBError{
					Message: fmt.Sprintf("Client certificate file not accessible: %s (%v)", tc.ClientCertPath, err),
				}}
			}
			if _, err := os.Stat(tc.ClientKeyPath); err != nil {
				if os.IsNotExist(err) {
					return &TLSConfigurationError{&ThemisDBError{
						Message: fmt.Sprintf("Client key file not found: %s", tc.ClientKeyPath),
					}}
				}
				return &TLSConfigurationError{&ThemisDBError{
					Message: fmt.Sprintf("Client key file not accessible: %s (%v)", tc.ClientKeyPath, err),
				}}
			}
		}
		
		// Enforce minimum TLS version
		if tc.MinVersion < tls.VersionTLS12 {
			return &TLSConfigurationError{&ThemisDBError{
				Message: "TLS version must be at least TLS 1.2 (versions below 1.2 are deprecated and insecure)",
			}}
		}
		
		// Production mode should use TLS 1.3
		if tc.ProductionMode && tc.MinVersion < tls.VersionTLS13 {
			log.Println("WARNING: Production mode should use TLS 1.3. TLS 1.2 is allowed but not recommended.")
		}
	} else {
		// TLS is disabled - warn user
		log.Println("WARNING: TLS is disabled. Connection is NOT encrypted. This should only be used for local development.")
	}
	
	return nil
}

// BuildTLSConfig creates a *tls.Config from TLSConfig
func (tc *TLSConfig) BuildTLSConfig() (*tls.Config, error) {
	if !tc.Enabled {
		return nil, nil
	}
	
	config := &tls.Config{
		MinVersion:         tc.MinVersion,
		InsecureSkipVerify: tc.InsecureSkipVerify,
		ServerName:         tc.ServerName,
	}
	
	// Load CA certificate if provided
	if tc.CACertPath != "" {
		caCert, err := os.ReadFile(tc.CACertPath)
		if err != nil {
			return nil, &TLSConfigurationError{&ThemisDBError{
				Message: fmt.Sprintf("Failed to read CA certificate: %v", err),
			}}
		}
		
		// Start with system cert pool and add custom CA
		caCertPool, err := x509.SystemCertPool()
		if err != nil {
			// If system pool is unavailable (e.g., on some platforms), create new pool
			caCertPool = x509.NewCertPool()
		}
		
		if !caCertPool.AppendCertsFromPEM(caCert) {
			return nil, &TLSConfigurationError{&ThemisDBError{
				Message: "Failed to parse CA certificate",
			}}
		}
		config.RootCAs = caCertPool
	}
	
	// Load client certificate for mTLS if provided
	if tc.ClientCertPath != "" && tc.ClientKeyPath != "" {
		cert, err := tls.LoadX509KeyPair(tc.ClientCertPath, tc.ClientKeyPath)
		if err != nil {
			return nil, &TLSConfigurationError{&ThemisDBError{
				Message: fmt.Sprintf("Failed to load client certificate: %v", err),
			}}
		}
		config.Certificates = []tls.Certificate{cert}
		log.Println("INFO: mTLS enabled - client certificate loaded")
	}
	
	return config, nil
}

// NewWireClient creates new ThemisDB wire protocol client
// For production use, call NewWireClientWithTLS instead
func NewWireClient(host string, port int, username, password string) *WireClient {
	return &WireClient{
		host:              host,
		port:              port,
		username:          username,
		password:          password,
		authenticated:     false,
		sequence:          0,
		pendingRequests:   make(map[uint32]chan *WireFrame),
		receiveBuffer:     make([]byte, 0),
		running:           false,
		done:              make(chan struct{}),
		requestTimeout:    REQUEST_TIMEOUT,
		connectionTimeout: DEFAULT_TIMEOUT,
		tlsConfig:         nil, // No TLS by default
	}
}

// NewWireClientWithTLS creates new ThemisDB wire protocol client with TLS
func NewWireClientWithTLS(host string, port int, username, password string, tlsConfig *TLSConfig) (*WireClient, error) {
	// Validate TLS configuration
	if err := tlsConfig.Validate(); err != nil {
		return nil, err
	}
	
	return &WireClient{
		host:              host,
		port:              port,
		username:          username,
		password:          password,
		authenticated:     false,
		sequence:          0,
		pendingRequests:   make(map[uint32]chan *WireFrame),
		receiveBuffer:     make([]byte, 0),
		running:           false,
		done:              make(chan struct{}),
		requestTimeout:    REQUEST_TIMEOUT,
		connectionTimeout: DEFAULT_TIMEOUT,
		tlsConfig:         tlsConfig,
	}, nil
}

// Connect establishes connection to ThemisDB
func (c *WireClient) Connect() error {
	// Validate TLS configuration if present
	if c.tlsConfig != nil {
		if err := c.tlsConfig.Validate(); err != nil {
			return err
		}
	}
	
	// Create base TCP connection
	var dialer net.Dialer
	dialer.Timeout = c.connectionTimeout
	
	addr := fmt.Sprintf("%s:%d", c.host, c.port)
	
	// If TLS is enabled, establish TLS connection
	if c.tlsConfig != nil && c.tlsConfig.Enabled {
		tlsCfg, err := c.tlsConfig.BuildTLSConfig()
		if err != nil {
			return err
		}
		
		// Set server name for SNI if not already set
		if tlsCfg.ServerName == "" {
			tlsCfg.ServerName = c.host
		}
		
		log.Printf("INFO: Establishing TLS connection to %s (MinVersion: %s)", 
			addr, tlsVersionString(tlsCfg.MinVersion))
		
		tlsConn, err := tls.DialWithDialer(&dialer, "tcp", addr, tlsCfg)
		if err != nil {
			return &ConnectionError{&ThemisDBError{
				Message: fmt.Sprintf("Failed to establish TLS connection: %v", err),
			}}
		}
		
		// Log connection state
		state := tlsConn.ConnectionState()
		log.Printf("INFO: TLS connection established (Version: %s, CipherSuite: %s, ServerName: %s)", 
			tlsVersionString(state.Version), 
			tls.CipherSuiteName(state.CipherSuite),
			state.ServerName)
		
		c.conn = tlsConn
	} else {
		// Plain TCP connection (no TLS)
		log.Printf("WARNING: Establishing unencrypted connection to %s", addr)
		
		conn, err := dialer.Dial("tcp", addr)
		if err != nil {
			return &ConnectionError{&ThemisDBError{
				Message: fmt.Sprintf("Failed to connect: %v", err),
			}}
		}
		c.conn = conn
	}
	
	c.running = true
	
	// Start receive goroutine
	go c.receiveLoop()
	
	// Authenticate
	return c.authenticate()
}

// tlsVersionString returns a human-readable TLS version string
func tlsVersionString(version uint16) string {
	switch version {
	case tls.VersionTLS10:
		return "TLS 1.0"
	case tls.VersionTLS11:
		return "TLS 1.1"
	case tls.VersionTLS12:
		return "TLS 1.2"
	case tls.VersionTLS13:
		return "TLS 1.3"
	default:
		return fmt.Sprintf("Unknown (0x%04x)", version)
	}
}

// Disconnect closes connection
func (c *WireClient) Disconnect() {
	c.running = false
	if c.conn != nil {
		c.conn.Close()
	}
	close(c.done)
}

// authenticate performs server authentication
func (c *WireClient) authenticate() error {
	// Send HELLO
	helloPayload := []byte("ThemisDB/1.0")
	helloFrame := &WireFrame{
		Version:     WIRE_VERSION,
		Opcode:      OpCodeHello,
		Flags:       FlagNone,
		Payload:     helloPayload,
		Sequence:    0,
	}

	if err := c.sendFrame(helloFrame); err != nil {
		return &AuthenticationError{&ThemisDBError{Message: fmt.Sprintf("Failed to send HELLO: %v", err)}}
	}

	// Send AUTH_REQUEST
	authPayload := []byte(c.username + ":" + c.password)
	authFrame := &WireFrame{
		Version:     WIRE_VERSION,
		Opcode:      OpCodeAuthRequest,
		Flags:       FlagNone,
		Payload:     authPayload,
		Sequence:    0,
	}

	if err := c.sendFrame(authFrame); err != nil {
		return &AuthenticationError{&ThemisDBError{Message: fmt.Sprintf("Failed to send AUTH: %v", err)}}
	}

	c.authenticated = true
	return nil
}

// Get retrieves document by key
func (c *WireClient) Get(key string) (map[string]interface{}, error) {
	payload := map[string]string{"key": key}
	payloadJSON, _ := json.Marshal(payload)

	frame := &WireFrame{
		Version:     WIRE_VERSION,
		Opcode:      OpCodeGet,
		Flags:       FlagNone,
		Payload:     payloadJSON,
		Sequence:    c.nextSequence(),
	}

	response, err := c.sendAndWait(frame)
	if err != nil {
		return nil, err
	}

	var result map[string]interface{}
	json.Unmarshal(response.Payload, &result)
	return result, nil
}

// Put stores document
func (c *WireClient) Put(key string, value interface{}) error {
	payload := map[string]interface{}{
		"key":   key,
		"value": value,
	}
	payloadJSON, _ := json.Marshal(payload)

	frame := &WireFrame{
		Version:     WIRE_VERSION,
		Opcode:      OpCodePut,
		Flags:       FlagNone,
		Payload:     payloadJSON,
		Sequence:    c.nextSequence(),
	}

	_, err := c.sendAndWait(frame)
	return err
}

// Delete removes document
func (c *WireClient) Delete(key string) error {
	payload := map[string]string{"key": key}
	payloadJSON, _ := json.Marshal(payload)

	frame := &WireFrame{
		Version:     WIRE_VERSION,
		Opcode:      OpCodeDelete,
		Flags:       FlagNone,
		Payload:     payloadJSON,
		Sequence:    c.nextSequence(),
	}

	_, err := c.sendAndWait(frame)
	return err
}

// Query executes AQL query
func (c *WireClient) Query(aql string, options map[string]interface{}) ([]map[string]interface{}, error) {
	payload := map[string]interface{}{
		"aql":     aql,
		"options": options,
	}
	payloadJSON, _ := json.Marshal(payload)

	frame := &WireFrame{
		Version:     WIRE_VERSION,
		Opcode:      OpCodeQueryAQL,
		Flags:       FlagNone,
		Payload:     payloadJSON,
		Sequence:    c.nextSequence(),
	}

	response, err := c.sendAndWait(frame)
	if err != nil {
		return nil, err
	}

	var result []map[string]interface{}
	json.Unmarshal(response.Payload, &result)
	return result, nil
}

// VectorSearch performs vector similarity search
func (c *WireClient) VectorSearch(collection string, vector []float64, options map[string]interface{}) ([]map[string]interface{}, error) {
	if options == nil {
		options = make(map[string]interface{})
	}
	if _, ok := options["top_k"]; !ok {
		options["top_k"] = 10
	}
	if _, ok := options["metric"]; !ok {
		options["metric"] = "cosine"
	}

	payload := map[string]interface{}{
		"collection": collection,
		"vector":     vector,
		"options":    options,
	}
	payloadJSON, _ := json.Marshal(payload)

	frame := &WireFrame{
		Version:     WIRE_VERSION,
		Opcode:      OpCodeVectorSearch,
		Flags:       FlagNone,
		Payload:     payloadJSON,
		Sequence:    c.nextSequence(),
	}

	response, err := c.sendAndWait(frame)
	if err != nil {
		return nil, err
	}

	var result []map[string]interface{}
	json.Unmarshal(response.Payload, &result)
	return result, nil
}

// GeoQuery performs geospatial search
func (c *WireClient) GeoQuery(collection string, lat, lon, radiusKm float64, options map[string]interface{}) ([]map[string]interface{}, error) {
	payload := map[string]interface{}{
		"collection": collection,
		"lat":        lat,
		"lon":        lon,
		"radius_km":  radiusKm,
		"options":    options,
	}
	payloadJSON, _ := json.Marshal(payload)

	frame := &WireFrame{
		Version:     WIRE_VERSION,
		Opcode:      OpCodeGeoQuery,
		Flags:       FlagNone,
		Payload:     payloadJSON,
		Sequence:    c.nextSequence(),
	}

	response, err := c.sendAndWait(frame)
	if err != nil {
		return nil, err
	}

	var result []map[string]interface{}
	json.Unmarshal(response.Payload, &result)
	return result, nil
}

// TimeseriesQuery performs time-series aggregation
func (c *WireClient) TimeseriesQuery(collection, startTime, endTime string, options map[string]interface{}) ([]map[string]interface{}, error) {
	payload := map[string]interface{}{
		"collection": collection,
		"start_time": startTime,
		"end_time":   endTime,
		"options":    options,
	}
	payloadJSON, _ := json.Marshal(payload)

	frame := &WireFrame{
		Version:     WIRE_VERSION,
		Opcode:      OpCodeTimeseriesQuery,
		Flags:       FlagNone,
		Payload:     payloadJSON,
		Sequence:    c.nextSequence(),
	}

	response, err := c.sendAndWait(frame)
	if err != nil {
		return nil, err
	}

	var result []map[string]interface{}
	json.Unmarshal(response.Payload, &result)
	return result, nil
}

// Helper methods

func (c *WireClient) nextSequence() uint32 {
	c.sequenceMu.Lock()
	defer c.sequenceMu.Unlock()
	c.sequence++
	return c.sequence
}

func (c *WireClient) sendFrame(frame *WireFrame) error {
	data, err := frame.ToBytes()
	if err != nil {
		return err
	}

	_, err = c.conn.Write(data)
	return err
}

func (c *WireClient) sendAndWait(frame *WireFrame) (*WireFrame, error) {
	responseChan := make(chan *WireFrame, 1)
	
	c.pendingMu.Lock()
	c.pendingRequests[frame.Sequence] = responseChan
	c.pendingMu.Unlock()

	if err := c.sendFrame(frame); err != nil {
		c.pendingMu.Lock()
		delete(c.pendingRequests, frame.Sequence)
		c.pendingMu.Unlock()
		return nil, err
	}

	select {
	case response := <-responseChan:
		return response, nil
	case <-time.After(c.requestTimeout):
		c.pendingMu.Lock()
		delete(c.pendingRequests, frame.Sequence)
		c.pendingMu.Unlock()
		return nil, &ThemisDBError{Message: "Request timeout"}
	}
}

func (c *WireClient) receiveLoop() {
	buffer := make([]byte, 1024*1024)

	for c.running {
		n, err := c.conn.Read(buffer)
		if err != nil {
			c.running = false
			break
		}

		c.receiveMu.Lock()
		c.receiveBuffer = append(c.receiveBuffer, buffer[:n]...)

		offset := 0
		for offset < len(c.receiveBuffer) {
			frame, err := FromBytes(c.receiveBuffer, offset)
			if err != nil {
				if err.Error() == "Incomplete frame" {
					break
				}
				offset += HEADER_SIZE
				continue
			}

			offset += HEADER_SIZE + int(frame.PayloadSize) + CHECKSUM_SIZE

			if frame.Sequence > 0 {
				c.pendingMu.RLock()
				responseChan, exists := c.pendingRequests[frame.Sequence]
				c.pendingMu.RUnlock()

				if exists {
					c.pendingMu.Lock()
					delete(c.pendingRequests, frame.Sequence)
					c.pendingMu.Unlock()

					responseChan <- frame
					close(responseChan)
				}
			}
		}

		c.receiveBuffer = c.receiveBuffer[offset:]
		c.receiveMu.Unlock()
	}
}

// IsTLSEnabled returns true if this client has TLS enabled
func (c *WireClient) IsTLSEnabled() bool {
	return c.tlsConfig != nil && c.tlsConfig.Enabled
}

// GetTLSConfig returns the TLS configuration (may be nil)
func (c *WireClient) GetTLSConfig() *TLSConfig {
	return c.tlsConfig
}

// NewWireClientFromEnv creates a Wire Protocol client from environment variables
// Supports the following environment variables:
//   THEMIS_WIRE_HOST - Server hostname (default: localhost)
//   THEMIS_WIRE_PORT - Server port (default: 18765)
//   THEMIS_WIRE_USERNAME - Username for authentication
//   THEMIS_WIRE_PASSWORD - Password for authentication
//   THEMIS_WIRE_TLS_ENABLED - Enable TLS (true/false, default: false)
//   THEMIS_WIRE_TLS_CA_CERT - Path to CA certificate
//   THEMIS_WIRE_TLS_CLIENT_CERT - Path to client certificate (for mTLS)
//   THEMIS_WIRE_TLS_CLIENT_KEY - Path to client key (for mTLS)
//   THEMIS_WIRE_TLS_INSECURE_SKIP_VERIFY - Skip certificate verification (INSECURE, default: false)
//   THEMIS_WIRE_TLS_SERVER_NAME - Server name for SNI
//   THEMIS_WIRE_PRODUCTION_MODE - Enable production mode (fail if TLS disabled, default: false)
func NewWireClientFromEnv() (*WireClient, error) {
	host := getEnv("THEMIS_WIRE_HOST", "localhost")
	port := getEnvInt("THEMIS_WIRE_PORT", 18765)
	username := getEnv("THEMIS_WIRE_USERNAME", "")
	password := getEnv("THEMIS_WIRE_PASSWORD", "")
	
	// Check if TLS should be enabled
	tlsEnabled := getEnvBool("THEMIS_WIRE_TLS_ENABLED", false)
	productionMode := getEnvBool("THEMIS_WIRE_PRODUCTION_MODE", false)
	
	// If neither TLS settings nor production mode is set, create basic client
	if !tlsEnabled && !productionMode {
		return NewWireClient(host, port, username, password), nil
	}
	
	// Create TLS configuration
	tlsConfig := &TLSConfig{
		Enabled:            tlsEnabled,
		CACertPath:         getEnv("THEMIS_WIRE_TLS_CA_CERT", ""),
		ClientCertPath:     getEnv("THEMIS_WIRE_TLS_CLIENT_CERT", ""),
		ClientKeyPath:      getEnv("THEMIS_WIRE_TLS_CLIENT_KEY", ""),
		InsecureSkipVerify: getEnvBool("THEMIS_WIRE_TLS_INSECURE_SKIP_VERIFY", false),
		ServerName:         getEnv("THEMIS_WIRE_TLS_SERVER_NAME", host),
		MinVersion:         tls.VersionTLS12,
		ProductionMode:     productionMode,
	}
	
	// In production mode, recommend TLS 1.3 but allow TLS 1.2 with warning
	if productionMode && tlsConfig.MinVersion < tls.VersionTLS13 {
		log.Printf("WARNING: Production mode enabled but TLS minimum version is TLS 1.2; TLS 1.3 is recommended for production")
	}
	
	return NewWireClientWithTLS(host, port, username, password, tlsConfig)
}

// Helper functions for environment variable parsing
func getEnv(key, defaultValue string) string {
	if value := os.Getenv(key); value != "" {
		return value
	}
	return defaultValue
}

func getEnvInt(key string, defaultValue int) int {
	if value := os.Getenv(key); value != "" {
		var intVal int
		if _, err := fmt.Sscanf(value, "%d", &intVal); err != nil {
			log.Printf("WARNING: Invalid integer value for %s: %s, using default: %d", key, value, defaultValue)
			return defaultValue
		}
		return intVal
	}
	return defaultValue
}

func getEnvBool(key string, defaultValue bool) bool {
	if value := os.Getenv(key); value != "" {
		lower := strings.ToLower(value)
		return lower == "true" || lower == "1" || lower == "yes"
	}
	return defaultValue
}
