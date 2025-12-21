package themisdb

import (
	"testing"

	"github.com/stretchr/testify/assert"
	"github.com/stretchr/testify/require"
)

// TestWireFrame_ToBytes tests wire frame serialization
func TestWireFrame_ToBytes(t *testing.T) {
	tests := []struct {
		name    string
		frame   *WireFrame
		wantErr bool
	}{
		{
			name: "valid frame with Hello opcode",
			frame: &WireFrame{
				Version:     WIRE_VERSION,
				Opcode:      OpCodeHello,
				Sequence:    123,
				Payload:     []byte(`{"client":"test"}`),
				PayloadSize: uint32(len([]byte(`{"client":"test"}`))),
			},
			wantErr: false,
		},
		{
			name: "empty payload with Ping",
			frame: &WireFrame{
				Version:     WIRE_VERSION,
				Opcode:      0x50, // Using a ping-like opcode
				Sequence:    1,
				Payload:     []byte{},
				PayloadSize: 0,
			},
			wantErr: false,
		},
		{
			name: "large payload",
			frame: &WireFrame{
				Version:     WIRE_VERSION,
				Opcode:      OpCodePut,
				Sequence:    999,
				Payload:     make([]byte, 1024*1024), // 1MB
				PayloadSize: 1024 * 1024,
			},
			wantErr: false,
		},
	}

	for _, tt := range tests {
		t.Run(tt.name, func(t *testing.T) {
			data, err := tt.frame.ToBytes()
			if tt.wantErr {
				assert.Error(t, err)
			} else {
				require.NoError(t, err)
				assert.NotNil(t, data)
				
				// Verify version
				assert.Equal(t, tt.frame.Version, data[4])
				
				// Verify opcode
				assert.Equal(t, tt.frame.Opcode, data[5])
			}
		})
	}
}

// TestFromBytes tests wire frame deserialization
func TestFromBytes(t *testing.T) {
	tests := []struct {
		name    string
		data    []byte
		offset  int
		wantErr bool
	}{
		{
			name: "valid frame",
			data: func() []byte {
				frame := &WireFrame{
					Version:     WIRE_VERSION,
					Opcode:      OpCodeHello,
					Sequence:    123,
					Payload:     []byte(`{"test":"data"}`),
					PayloadSize: uint32(len([]byte(`{"test":"data"}`))),
				}
				data, _ := frame.ToBytes()
				return data
			}(),
			offset:  0,
			wantErr: false,
		},
		{
			name:    "too short data",
			data:    make([]byte, HEADER_SIZE-1),
			offset:  0,
			wantErr: true,
		},
	}

	for _, tt := range tests {
		t.Run(tt.name, func(t *testing.T) {
			frame, err := FromBytes(tt.data, tt.offset)
			if tt.wantErr {
				assert.Error(t, err)
			} else {
				require.NoError(t, err)
				assert.NotNil(t, frame)
			}
		})
	}
}

// TestWireClient_Creation tests wire client creation
func TestWireClient_Creation(t *testing.T) {
	tests := []struct {
		name     string
		host     string
		port     int
		username string
		password string
	}{
		{
			name:     "default parameters",
			host:     "localhost",
			port:     9090,
			username: "admin",
			password: "password",
		},
		{
			name:     "custom parameters",
			host:     "192.168.1.100",
			port:     8888,
			username: "testuser",
			password: "testpass",
		},
	}

	for _, tt := range tests {
		t.Run(tt.name, func(t *testing.T) {
			client := NewWireClient(tt.host, tt.port, tt.username, tt.password)
			require.NotNil(t, client)
			assert.NotNil(t, client.pendingRequests)
			assert.NotNil(t, client.done)
			assert.False(t, client.running)
			assert.False(t, client.authenticated)
		})
	}
}

// TestWireClient_SequenceIncrement tests sequence number generation
func TestWireClient_SequenceIncrement(t *testing.T) {
	client := NewWireClient("localhost", 9090, "test", "test")

	// Test sequence increment
	seq1 := client.nextSequence()
	seq2 := client.nextSequence()
	seq3 := client.nextSequence()

	assert.Equal(t, uint32(1), seq1)
	assert.Equal(t, uint32(2), seq2)
	assert.Equal(t, uint32(3), seq3)

	// Test concurrent access
	done := make(chan bool)
	sequences := make(map[uint32]bool)
	seqChan := make(chan uint32, 100)
	
	for i := 0; i < 100; i++ {
		go func() {
			seq := client.nextSequence()
			seqChan <- seq
			done <- true
		}()
	}

	for i := 0; i < 100; i++ {
		<-done
	}
	close(seqChan)

	for seq := range seqChan {
		sequences[seq] = true
	}

	// All sequences should be unique
	assert.Equal(t, 100, len(sequences))
}

// TestOpCodes tests all defined operation codes
func TestOpCodes(t *testing.T) {
	opcodes := []struct {
		name   string
		opcode byte
	}{
		{"OpCodeHello", OpCodeHello},
		{"OpCodeHelloAck", OpCodeHelloAck},
		{"OpCodeAuthRequest", OpCodeAuthRequest},
		{"OpCodeAuthResponse", OpCodeAuthResponse},
		{"OpCodeAuthSuccess", OpCodeAuthSuccess},
		{"OpCodeAuthFailure", OpCodeAuthFailure},
		{"OpCodeGet", OpCodeGet},
		{"OpCodePut", OpCodePut},
		{"OpCodeDelete", OpCodeDelete},
		{"OpCodeBatchGet", OpCodeBatchGet},
		{"OpCodeBatchPut", OpCodeBatchPut},
		{"OpCodeQueryAQL", OpCodeQueryAQL},
		{"OpCodeQueryResult", OpCodeQueryResult},
		{"OpCodeVectorSearch", OpCodeVectorSearch},
	}

	for _, tc := range opcodes {
		t.Run(tc.name, func(t *testing.T) {
			assert.NotEqual(t, byte(0), tc.opcode, "OpCode should not be zero")
		})
	}
}

// TestThemisDBError tests error types
func TestThemisDBError(t *testing.T) {
	err := &ThemisDBError{
		Message: "Internal Server Error",
	}

	assert.Equal(t, "Internal Server Error", err.Error())
}

// TestConstants tests wire protocol constants
func TestConstants(t *testing.T) {
	assert.Equal(t, 0x544D4442, WIRE_MAGIC, "WIRE_MAGIC should be 'TMDB'")
	assert.Equal(t, 0x01, WIRE_VERSION)
	assert.Equal(t, 12, HEADER_SIZE)
	assert.Equal(t, 4, CHECKSUM_SIZE)
	assert.Equal(t, 64*1024*1024, MAX_PAYLOAD_SIZE)
}

// TestWireFrame_RoundTrip tests serialization and deserialization
func TestWireFrame_RoundTrip(t *testing.T) {
	original := &WireFrame{
		Version:     WIRE_VERSION,
		Opcode:      OpCodeQueryAQL,
		Sequence:    42, // Note: Sequence is not serialized, only used in-memory
		Payload:     []byte(`{"query":"SELECT * FROM users"}`),
		PayloadSize: uint32(len([]byte(`{"query":"SELECT * FROM users"}`))),
	}

	// Serialize
	data, err := original.ToBytes()
	require.NoError(t, err)

	// Deserialize
	restored, err := FromBytes(data, 0)
	require.NoError(t, err)

	// Compare (excluding Sequence which is not serialized)
	assert.Equal(t, original.Version, restored.Version)
	assert.Equal(t, original.Opcode, restored.Opcode)
	assert.Equal(t, original.Payload, restored.Payload)
}

// TestWireFrame_PayloadSizes tests various payload sizes
func TestWireFrame_PayloadSizes(t *testing.T) {
	sizes := []int{
		0,
		1,
		100,
		1024,
		10 * 1024,
		100 * 1024,
		1024 * 1024,
	}

	for _, size := range sizes {
		t.Run(string(rune(size)), func(t *testing.T) {
			payload := make([]byte, size)
			frame := &WireFrame{
				Version:     WIRE_VERSION,
				Opcode:      OpCodePut,
				Sequence:    1,
				Payload:     payload,
				PayloadSize: uint32(size),
			}

			data, err := frame.ToBytes()
			require.NoError(t, err)

			restored, err := FromBytes(data, 0)
			require.NoError(t, err)
			assert.Equal(t, size, len(restored.Payload))
		})
	}
}
