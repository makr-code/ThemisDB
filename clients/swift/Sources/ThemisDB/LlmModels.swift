import Foundation

// MARK: - LLM Models

/// Represents a message in an LLM conversation
public struct LlmMessage: Codable, Sendable {
    public let role: String
    public let content: String
    public let imageUrl: String?
    
    enum CodingKeys: String, CodingKey {
        case role
        case content
        case imageUrl = "image_url"
    }
    
    public init(role: String, content: String, imageUrl: String? = nil) {
        self.role = role
        self.content = content
        self.imageUrl = imageUrl
    }
}

/// Represents a reasoning step in LLM interaction
public struct ReasoningStep: Codable, Sendable {
    public let type: String
    public let content: [String]
    
    public init(type: String, content: [String]) {
        self.type = type
        self.content = content
    }
}

/// Represents a stored LLM interaction
public struct LlmInteraction: Codable, Sendable {
    public let id: String
    public let createdAt: String
    public let model: String
    public let messages: [LlmMessage]
    public let reasoningSteps: [ReasoningStep]?
    public let metadata: [String: AnyCodable]?
    
    enum CodingKeys: String, CodingKey {
        case id
        case createdAt = "created_at"
        case model
        case messages
        case reasoningSteps = "reasoning_steps"
        case metadata
    }
}

/// Result of creating an LLM interaction
public struct LlmInteractionResult: Codable, Sendable {
    public let id: String
    public let success: Bool
}

/// Helper for encoding/decoding Any values
public struct AnyCodable: Codable {
    public let value: Any
    
    public init(_ value: Any) {
        self.value = value
    }
    
    public init(from decoder: Decoder) throws {
        let container = try decoder.singleValueContainer()
        
        if let intValue = try? container.decode(Int.self) {
            value = intValue
        } else if let doubleValue = try? container.decode(Double.self) {
            value = doubleValue
        } else if let stringValue = try? container.decode(String.self) {
            value = stringValue
        } else if let boolValue = try? container.decode(Bool.self) {
            value = boolValue
        } else if let arrayValue = try? container.decode([AnyCodable].self) {
            value = arrayValue.map { $0.value }
        } else if let dictValue = try? container.decode([String: AnyCodable].self) {
            value = dictValue.mapValues { $0.value }
        } else {
            value = NSNull()
        }
    }
    
    public func encode(to encoder: Encoder) throws {
        var container = encoder.singleValueContainer()
        
        switch value {
        case let intValue as Int:
            try container.encode(intValue)
        case let doubleValue as Double:
            try container.encode(doubleValue)
        case let stringValue as String:
            try container.encode(stringValue)
        case let boolValue as Bool:
            try container.encode(boolValue)
        case let arrayValue as [Any]:
            try container.encode(arrayValue.map { AnyCodable($0) })
        case let dictValue as [String: Any]:
            try container.encode(dictValue.mapValues { AnyCodable($0) })
        default:
            try container.encodeNil()
        }
    }
}

/// Response for listing LLM interactions
struct LlmInteractionsResponse: Codable {
    let interactions: [LlmInteraction]
}
