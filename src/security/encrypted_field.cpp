/**
 * @file encrypted_field.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=1, M=0, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "security/encryption.h"
#include <sstream>
#include <iomanip>

namespace themis {

// ===== Template Method Implementations =====

template<typename T>
void EncryptedField<T>::setFieldEncryption(std::shared_ptr<FieldEncryption> encryption) {
    field_encryption_ = encryption;
}

template<typename T>
EncryptedField<T>::EncryptedField() {}

template<typename T>
EncryptedField<T>::EncryptedField(const T& value, const std::string& key_id) {
    encrypt(value, key_id);
}

template<typename T>
EncryptedField<T>::EncryptedField(const EncryptedBlob& blob)
    : blob_(blob) {}

template<typename T>
void EncryptedField<T>::encrypt(const T& value, const std::string& key_id) {
    if (!field_encryption_) {
        throw std::runtime_error("FieldEncryption not set. Call setFieldEncryption() first.");
    }
    
    std::string serialized = serialize(value);
    blob_ = field_encryption_->encrypt(serialized, key_id);
}

template<typename T>
T EncryptedField<T>::decrypt() const {
    if (!field_encryption_) {
        throw std::runtime_error("FieldEncryption not set. Call setFieldEncryption() first.");
    }
    
    if (!hasValue()) {
        throw std::runtime_error("No encrypted value to decrypt");
    }
    
    std::string decrypted = field_encryption_->decryptToString(blob_);
    return deserialize(decrypted);
}

template<typename T>
bool EncryptedField<T>::hasValue() const {
    return !blob_.ciphertext.empty();
}

template<typename T>
bool EncryptedField<T>::isEncrypted() const {
    return !blob_.ciphertext.empty();
}

template<typename T>
std::string EncryptedField<T>::toBase64() const {
    return blob_.toBase64();
}

template<typename T>
EncryptedField<T> EncryptedField<T>::fromBase64(const std::string& b64) {
    return EncryptedField<T>(EncryptedBlob::fromBase64(b64));
}

template<typename T>
nlohmann::json EncryptedField<T>::toJson() const {
    return blob_.toJson();
}

template<typename T>
EncryptedField<T> EncryptedField<T>::fromJson(const nlohmann::json& j) {
    return EncryptedField<T>(EncryptedBlob::fromJson(j));
}

// ===== Type-Specific Serialization =====

// std::string specialization
template<>
std::string EncryptedField<std::string>::serialize(const std::string& value) {
    return value;
}

template<>
std::string EncryptedField<std::string>::deserialize(const std::string& str) {
    return str;
}

// int64_t specialization
template<>
std::string EncryptedField<int64_t>::serialize(const int64_t& value) {
    return std::to_string(value);
}

template<>
int64_t EncryptedField<int64_t>::deserialize(const std::string& str) {
    return std::stoll(str);
}

// double specialization
template<>
std::string EncryptedField<double>::serialize(const double& value) {
    std::ostringstream oss;
    oss << std::setprecision(17) << value;
    return oss.str();
}

template<>
double EncryptedField<double>::deserialize(const std::string& str) {
    return std::stod(str);
}

// std::vector<float> specialization (for vector embeddings)
template<>
std::string EncryptedField<std::vector<float>>::serialize(const std::vector<float>& value) {
    std::string result;
    uint32_t size = static_cast<uint32_t>(value.size());
    
    // Append size (4 bytes, little-endian)
    result.append(reinterpret_cast<const char*>(&size), sizeof(size));
    
    // Append float data
    if (size > 0) {
        result.append(reinterpret_cast<const char*>(value.data()), 
                      value.size() * sizeof(float));
    }
    
    return result;
}

template<>
std::vector<float> EncryptedField<std::vector<float>>::deserialize(const std::string& str) {
    if (str.size() < sizeof(uint32_t)) {
        throw DecryptionException("Invalid vector serialization: too short");
    }
    
    // Read size
    uint32_t size;
    std::memcpy(&size, str.data(), sizeof(size));
    
    // Validate size
    size_t expected_bytes = sizeof(uint32_t) + size * sizeof(float);
    if (str.size() != expected_bytes) {
        throw DecryptionException(
            "Invalid vector serialization: size mismatch (expected " + 
            std::to_string(expected_bytes) + " bytes, got " + 
            std::to_string(str.size()) + " bytes)");
    }
    
    // Read floats
    std::vector<float> result(size);
    if (size > 0) {
        std::memcpy(result.data(), 
                    str.data() + sizeof(uint32_t), 
                    size * sizeof(float));
    }
    
    return result;
}

// std::vector<uint8_t> specialization (for HNSW index encryption)
// Note: For large binary data (multi-GB HNSW indexes), this creates copies.
// This is acceptable for Phase 2 initial implementation.
// Future optimization: Use move semantics or memory-mapped files.
template<>
std::string EncryptedField<std::vector<uint8_t>>::serialize(const std::vector<uint8_t>& value) {
    // For binary data, convert to string (creates copy)
    return std::string(value.begin(), value.end());
}

template<>
std::vector<uint8_t> EncryptedField<std::vector<uint8_t>>::deserialize(const std::string& str) {
    // Convert back to bytes (creates copy)
    return std::vector<uint8_t>(str.begin(), str.end());
}

// Explicit template instantiations
template class EncryptedField<std::string>;
template class EncryptedField<int64_t>;
template class EncryptedField<double>;
template class EncryptedField<std::vector<float>>;
template class EncryptedField<std::vector<uint8_t>>;

}  // namespace themis
