package themis

import (
	"bytes"
	"encoding/binary"
	"encoding/json"
	"fmt"
	"hash/crc32"
	"io"
	"net"
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
type Client struct {
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
}

// NewClient creates new ThemisDB client
func NewClient(host string, port int, username, password string) *Client {
	return &Client{
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
	}
}

// Connect establishes connection to ThemisDB
func (c *Client) Connect() error {
	var dialer net.Dialer
	dialer.Timeout = c.connectionTimeout
	conn, err := dialer.Dial("tcp", fmt.Sprintf("%s:%d", c.host, c.port))
	if err != nil {
		return &ConnectionError{&ThemisDBError{Message: fmt.Sprintf("Failed to connect: %v", err)}}
	}

	c.conn = conn
	c.running = true

	// Start receive goroutine
	go c.receiveLoop()

	// Authenticate
	return c.authenticate()
}

// Disconnect closes connection
func (c *Client) Disconnect() {
	c.running = false
	if c.conn != nil {
		c.conn.Close()
	}
	close(c.done)
}

// authenticate performs server authentication
func (c *Client) authenticate() error {
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
func (c *Client) Get(key string) (map[string]interface{}, error) {
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
func (c *Client) Put(key string, value interface{}) error {
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
func (c *Client) Delete(key string) error {
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
func (c *Client) Query(aql string, options map[string]interface{}) ([]map[string]interface{}, error) {
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
func (c *Client) VectorSearch(collection string, vector []float64, options map[string]interface{}) ([]map[string]interface{}, error) {
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
func (c *Client) GeoQuery(collection string, lat, lon, radiusKm float64, options map[string]interface{}) ([]map[string]interface{}, error) {
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
func (c *Client) TimeseriesQuery(collection, startTime, endTime string, options map[string]interface{}) ([]map[string]interface{}, error) {
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

func (c *Client) nextSequence() uint32 {
	c.sequenceMu.Lock()
	defer c.sequenceMu.Unlock()
	c.sequence++
	return c.sequence
}

func (c *Client) sendFrame(frame *WireFrame) error {
	data, err := frame.ToBytes()
	if err != nil {
		return err
	}

	_, err = c.conn.Write(data)
	return err
}

func (c *Client) sendAndWait(frame *WireFrame) (*WireFrame, error) {
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

func (c *Client) receiveLoop() {
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
