package themisdb

import (
	"testing"
)

// BenchmarkWireFrame_ToBytes benchmarks frame serialization
func BenchmarkWireFrame_ToBytes(b *testing.B) {
	payload := []byte(`{"key":"user-123","collection":"users"}`)
	frame := &WireFrame{
		Version:     WIRE_VERSION,
		Opcode:      OpCodeGet,
		Sequence:    123,
		Payload:     payload,
		PayloadSize: uint32(len(payload)),
	}

	b.ResetTimer()
	for i := 0; i < b.N; i++ {
		_, _ = frame.ToBytes()
	}
}

// BenchmarkWireFrame_FromBytes benchmarks frame deserialization
func BenchmarkWireFrame_FromBytes(b *testing.B) {
	payload := []byte(`{"key":"user-123","collection":"users"}`)
	frame := &WireFrame{
		Version:     WIRE_VERSION,
		Opcode:      OpCodeGet,
		Sequence:    123,
		Payload:     payload,
		PayloadSize: uint32(len(payload)),
	}
	data, _ := frame.ToBytes()

	b.ResetTimer()
	for i := 0; i < b.N; i++ {
		_, _ = FromBytes(data, 0)
	}
}

// BenchmarkWireFrame_RoundTrip benchmarks serialize + deserialize
func BenchmarkWireFrame_RoundTrip(b *testing.B) {
	payload := []byte(`{"query":"SELECT * FROM users WHERE age > 25"}`)
	frame := &WireFrame{
		Version:     WIRE_VERSION,
		Opcode:      OpCodeQueryAQL,
		Sequence:    123,
		Payload:     payload,
		PayloadSize: uint32(len(payload)),
	}

	b.ResetTimer()
	for i := 0; i < b.N; i++ {
		data, _ := frame.ToBytes()
		_, _ = FromBytes(data, 0)
	}
}

// BenchmarkWireFrame_SmallPayload benchmarks with small payload
func BenchmarkWireFrame_SmallPayload(b *testing.B) {
	frame := &WireFrame{
		Version:     WIRE_VERSION,
		Opcode:      0x50,
		Sequence:    1,
		Payload:     []byte{},
		PayloadSize: 0,
	}

	b.ResetTimer()
	for i := 0; i < b.N; i++ {
		_, _ = frame.ToBytes()
	}
}

// BenchmarkWireFrame_MediumPayload benchmarks with medium payload (1KB)
func BenchmarkWireFrame_MediumPayload(b *testing.B) {
	payload := make([]byte, 1024)
	for i := range payload {
		payload[i] = byte(i % 256)
	}

	frame := &WireFrame{
		Version:     WIRE_VERSION,
		Opcode:      OpCodePut,
		Sequence:    1,
		Payload:     payload,
		PayloadSize: 1024,
	}

	b.ResetTimer()
	for i := 0; i < b.N; i++ {
		_, _ = frame.ToBytes()
	}
}

// BenchmarkWireFrame_LargePayload benchmarks with large payload (1MB)
func BenchmarkWireFrame_LargePayload(b *testing.B) {
	payload := make([]byte, 1024*1024)
	for i := range payload {
		payload[i] = byte(i % 256)
	}

	frame := &WireFrame{
		Version:     WIRE_VERSION,
		Opcode:      OpCodePut,
		Sequence:    1,
		Payload:     payload,
		PayloadSize: 1024 * 1024,
	}

	b.ResetTimer()
	for i := 0; i < b.N; i++ {
		_, _ = frame.ToBytes()
	}
}

// BenchmarkWireClient_SequenceGeneration benchmarks sequence number generation
func BenchmarkWireClient_SequenceGeneration(b *testing.B) {
	client := NewWireClient("localhost", 9090, "test", "test")

	b.ResetTimer()
	for i := 0; i < b.N; i++ {
		_ = client.nextSequence()
	}
}

// BenchmarkWireClient_ParallelSequenceGeneration benchmarks concurrent sequence generation
func BenchmarkWireClient_ParallelSequenceGeneration(b *testing.B) {
	client := NewWireClient("localhost", 9090, "test", "test")

	b.ResetTimer()
	b.RunParallel(func(pb *testing.PB) {
		for pb.Next() {
			_ = client.nextSequence()
		}
	})
}

// BenchmarkWireFrame_PayloadSizes benchmarks different payload sizes
func BenchmarkWireFrame_PayloadSizes(b *testing.B) {
	sizes := []int{
		0,
		64,
		256,
		1024,
		4096,
		16384,
		65536,
		262144,
		1048576,
	}

	for _, size := range sizes {
		b.Run(string(rune(size)), func(b *testing.B) {
			payload := make([]byte, size)
			frame := &WireFrame{
				Version:     WIRE_VERSION,
				Opcode:      OpCodePut,
				Sequence:    1,
				Payload:     payload,
				PayloadSize: uint32(size),
			}

			b.ResetTimer()
			for i := 0; i < b.N; i++ {
				_, _ = frame.ToBytes()
			}
		})
	}
}

// BenchmarkWireFrame_DifferentOpCodes benchmarks different operation codes
func BenchmarkWireFrame_DifferentOpCodes(b *testing.B) {
	opcodes := []struct {
		name   string
		opcode byte
	}{
		{"Get", OpCodeGet},
		{"Put", OpCodePut},
		{"Delete", OpCodeDelete},
		{"QueryAQL", OpCodeQueryAQL},
		{"VectorSearch", OpCodeVectorSearch},
	}

	payload := []byte(`{"data":"test"}`)

	for _, op := range opcodes {
		b.Run(op.name, func(b *testing.B) {
			frame := &WireFrame{
				Version:     WIRE_VERSION,
				Opcode:      op.opcode,
				Sequence:    1,
				Payload:     payload,
				PayloadSize: uint32(len(payload)),
			}

			b.ResetTimer()
			for i := 0; i < b.N; i++ {
				_, _ = frame.ToBytes()
			}
		})
	}
}

// BenchmarkWireFrame_Deserialization_PayloadSizes benchmarks deserialization with different sizes
func BenchmarkWireFrame_Deserialization_PayloadSizes(b *testing.B) {
	sizes := []int{0, 256, 1024, 4096, 16384, 65536}

	for _, size := range sizes {
		b.Run(string(rune(size)), func(b *testing.B) {
			payload := make([]byte, size)
			frame := &WireFrame{
				Version:     WIRE_VERSION,
				Opcode:      OpCodeGet,
				Sequence:    1,
				Payload:     payload,
				PayloadSize: uint32(size),
			}
			data, _ := frame.ToBytes()

			b.ResetTimer()
			for i := 0; i < b.N; i++ {
				_, _ = FromBytes(data, 0)
			}
		})
	}
}
