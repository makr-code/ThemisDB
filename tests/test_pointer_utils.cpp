#include <gtest/gtest.h>
#include "utils/pointer_utils.h"
#include <spdlog/spdlog.h>
#include <spdlog/sinks/ostream_sink.h>
#include <memory>
#include <map>
#include <vector>
#include <sstream>

using namespace themis::utils::pointer;

// Test fixture for pointer utilities
class PointerUtilsTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Capture logs for verification
        log_stream_ = std::make_shared<std::ostringstream>();
        auto ostream_sink = std::make_shared<spdlog::sinks::ostream_sink_mt>(*log_stream_);
        auto logger = std::make_shared<spdlog::logger>("test_logger", ostream_sink);
        spdlog::set_default_logger(logger);
        spdlog::set_level(spdlog::level::debug);
    }
    
    void TearDown() override {
        spdlog::set_default_logger(nullptr);
    }
    
    std::string get_logs() {
        return log_stream_->str();
    }
    
    std::shared_ptr<std::ostringstream> log_stream_;
};

// Test class hierarchy for dynamic_cast tests
struct Base {
    virtual ~Base() = default;
    virtual int getValue() const { return 0; }
};

struct Derived : public Base {
    int getValue() const override { return 42; }
    void derivedMethod() {}
};

struct OtherDerived : public Base {
    int getValue() const override { return 99; }
};

// ============================================================================
// require_non_null tests
// ============================================================================

TEST_F(PointerUtilsTest, RequireNonNull_ValidPointer_ReturnsPointer) {
    int value = 42;
    int* ptr = &value;
    
    int* result = require_non_null(ptr, "Test message");
    
    EXPECT_EQ(result, ptr);
    EXPECT_EQ(*result, 42);
}

TEST_F(PointerUtilsTest, RequireNonNull_NullPointer_ThrowsException) {
    int* null_ptr = nullptr;
    
    EXPECT_THROW(
        require_non_null(null_ptr, "Expected null pointer"),
        std::runtime_error
    );
    
    // Verify error was logged
    std::string logs = get_logs();
    EXPECT_TRUE(logs.find("Null pointer check failed") != std::string::npos);
    EXPECT_TRUE(logs.find("Expected null pointer") != std::string::npos);
}

TEST_F(PointerUtilsTest, RequireNonNull_DefaultMessage_ThrowsWithDefaultMessage) {
    int* null_ptr = nullptr;
    
    try {
        require_non_null(null_ptr);
        FAIL() << "Expected exception";
    } catch (const std::runtime_error& e) {
        std::string msg = e.what();
        EXPECT_TRUE(msg.find("Null pointer") != std::string::npos);
    }
}

// ============================================================================
// as_optional tests
// ============================================================================

TEST_F(PointerUtilsTest, AsOptional_ValidPointer_ReturnsOptional) {
    int value = 42;
    int* ptr = &value;
    
    auto opt = as_optional(ptr);
    
    ASSERT_TRUE(opt.has_value());
    EXPECT_EQ(*opt.value(), 42);
}

TEST_F(PointerUtilsTest, AsOptional_NullPointer_ReturnsNullopt) {
    int* null_ptr = nullptr;
    
    auto opt = as_optional(null_ptr);
    
    EXPECT_FALSE(opt.has_value());
}

TEST_F(PointerUtilsTest, AsOptional_CanBeUsedInIfStatement) {
    int value = 42;
    int* ptr = &value;
    
    if (auto opt = as_optional(ptr)) {
        EXPECT_EQ(**opt, 42);
    } else {
        FAIL() << "Optional should have value";
    }
    
    // Test null case
    int* null_ptr = nullptr;
    bool entered_block = false;
    if (auto opt = as_optional(null_ptr)) {
        FAIL() << "Should not enter this block";
    } else {
        entered_block = true;
    }
    EXPECT_TRUE(entered_block);
}

// ============================================================================
// safe_invoke tests
// ============================================================================

TEST_F(PointerUtilsTest, SafeInvoke_ValidPointer_InvokesFunction) {
    int value = 10;
    int* ptr = &value;
    
    auto result = safe_invoke(ptr, [](int& val) {
        return val * 2;
    });
    
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result.value(), 20);
}

TEST_F(PointerUtilsTest, SafeInvoke_NullPointer_ReturnsNullopt) {
    int* null_ptr = nullptr;
    
    auto result = safe_invoke(null_ptr, [](int& val) {
        return val * 2;
    });
    
    EXPECT_FALSE(result.has_value());
}

TEST_F(PointerUtilsTest, SafeInvoke_VoidReturn_WorksCorrectly) {
    int value = 10;
    int* ptr = &value;
    int side_effect = 0;
    
    auto result = safe_invoke(ptr, [&](int& val) {
        side_effect = val;
    });
    
    EXPECT_EQ(side_effect, 10);
}

// ============================================================================
// wrap_c_ptr tests
// ============================================================================

TEST_F(PointerUtilsTest, WrapCPtr_ValidPointer_CreatesUniquePtr) {
    int* raw_ptr = new int(42);
    
    auto smart_ptr = wrap_c_ptr(raw_ptr, [](int* p) {
        delete p;
    });
    
    EXPECT_EQ(*smart_ptr, 42);
    // Memory will be cleaned up automatically
}

TEST_F(PointerUtilsTest, WrapCPtr_NullPointer_ThrowsException) {
    int* null_ptr = nullptr;
    
    EXPECT_THROW(
        wrap_c_ptr(null_ptr, [](int* p) { delete p; }),
        std::runtime_error
    );
    
    // Verify error was logged
    std::string logs = get_logs();
    EXPECT_TRUE(logs.find("C API returned null pointer") != std::string::npos);
}

TEST_F(PointerUtilsTest, WrapCPtr_CustomDeleter_CallsDeleterOnDestruction) {
    bool deleter_called = false;
    
    {
        int* raw_ptr = new int(42);
        auto smart_ptr = wrap_c_ptr(raw_ptr, [&](int* p) {
            deleter_called = true;
            delete p;
        });
    } // smart_ptr goes out of scope
    
    EXPECT_TRUE(deleter_called);
}

// ============================================================================
// safe_dynamic_cast tests
// ============================================================================

TEST_F(PointerUtilsTest, SafeDynamicCast_ValidCast_ReturnsOptional) {
    Base* base = new Derived();
    
    auto derived = safe_dynamic_cast<Derived>(base);
    
    ASSERT_TRUE(derived.has_value());
    EXPECT_EQ((*derived)->getValue(), 42);
    
    delete base;
}

TEST_F(PointerUtilsTest, SafeDynamicCast_InvalidCast_ReturnsNullopt) {
    Base* base = new OtherDerived();
    
    auto derived = safe_dynamic_cast<Derived>(base);
    
    EXPECT_FALSE(derived.has_value());
    
    delete base;
}

TEST_F(PointerUtilsTest, SafeDynamicCast_NullPointer_ReturnsNullopt) {
    Base* null_base = nullptr;
    
    auto derived = safe_dynamic_cast<Derived>(null_base);
    
    EXPECT_FALSE(derived.has_value());
}

TEST_F(PointerUtilsTest, SafeDynamicCast_CanBeUsedInIfStatement) {
    Base* base = new Derived();
    
    if (auto derived = safe_dynamic_cast<Derived>(base)) {
        EXPECT_EQ((*derived)->getValue(), 42);
        (*derived)->derivedMethod(); // Safe to call derived method
    } else {
        FAIL() << "Cast should succeed";
    }
    
    delete base;
}

// ============================================================================
// safe_lock tests
// ============================================================================

TEST_F(PointerUtilsTest, SafeLock_ValidWeakPtr_ReturnsSharedPtr) {
    auto shared = std::make_shared<int>(42);
    std::weak_ptr<int> weak = shared;
    
    auto result = safe_lock(weak, "Test lock");
    
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(*result.value(), 42);
}

TEST_F(PointerUtilsTest, SafeLock_ExpiredWeakPtr_ReturnsNullopt) {
    std::weak_ptr<int> weak;
    {
        auto shared = std::make_shared<int>(42);
        weak = shared;
    } // shared goes out of scope, weak expires
    
    auto result = safe_lock(weak, "Expected expired");
    
    EXPECT_FALSE(result.has_value());
    
    // Verify debug message was logged
    std::string logs = get_logs();
    EXPECT_TRUE(logs.find("weak_ptr lock failed") != std::string::npos);
}

TEST_F(PointerUtilsTest, SafeLock_CanBeUsedInIfStatement) {
    auto shared = std::make_shared<int>(42);
    std::weak_ptr<int> weak = shared;
    
    if (auto locked = safe_lock(weak)) {
        EXPECT_EQ(**locked, 42);
    } else {
        FAIL() << "Lock should succeed";
    }
}

// ============================================================================
// safe_at tests (map)
// ============================================================================

TEST_F(PointerUtilsTest, SafeAt_Map_ExistingKey_ReturnsValue) {
    std::map<std::string, int> map = {{"key1", 42}, {"key2", 99}};
    
    auto result = safe_at(map, std::string("key1"));
    
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result.value(), 42);
}

TEST_F(PointerUtilsTest, SafeAt_Map_NonExistingKey_ReturnsNullopt) {
    std::map<std::string, int> map = {{"key1", 42}};
    
    auto result = safe_at(map, std::string("key2"));
    
    EXPECT_FALSE(result.has_value());
}

// ============================================================================
// safe_at tests (vector)
// ============================================================================

TEST_F(PointerUtilsTest, SafeAt_Vector_ValidIndex_ReturnsReference) {
    std::vector<int> vec = {10, 20, 30};
    
    auto result = safe_at(vec, 1);
    
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result.value().get(), 20);
}

TEST_F(PointerUtilsTest, SafeAt_Vector_InvalidIndex_ReturnsNullopt) {
    std::vector<int> vec = {10, 20, 30};
    
    auto result = safe_at(vec, 10);
    
    EXPECT_FALSE(result.has_value());
}

TEST_F(PointerUtilsTest, SafeAt_Vector_MutableAccess_CanModify) {
    std::vector<int> vec = {10, 20, 30};
    
    auto result = safe_at(vec, 1);
    ASSERT_TRUE(result.has_value());
    
    result.value().get() = 99;
    EXPECT_EQ(vec[1], 99);
}

TEST_F(PointerUtilsTest, SafeAt_Vector_ConstAccess_ProvidesConstReference) {
    const std::vector<int> vec = {10, 20, 30};
    
    auto result = safe_at(vec, 1);
    
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result.value().get(), 20);
}

// ============================================================================
// Integration tests
// ============================================================================

TEST_F(PointerUtilsTest, Integration_ChainedOperations) {
    // Simulate a real-world scenario with multiple safety checks
    std::map<std::string, std::shared_ptr<Derived>> object_map;
    object_map["obj1"] = std::make_shared<Derived>();
    
    // Safe lookup in map
    auto it = object_map.find("obj1");
    if (it != object_map.end()) {
        auto obj_ptr = it->second.get();
        
        // Safe dynamic cast
        if (auto derived = safe_dynamic_cast<Derived>(static_cast<Base*>(obj_ptr))) {
            // Safe invoke
            auto result = safe_invoke(*derived, [](Derived& d) {
                return d.getValue();
            });
            
            ASSERT_TRUE(result.has_value());
            EXPECT_EQ(result.value(), 42);
        }
    }
}

TEST_F(PointerUtilsTest, Integration_WeakPtrPattern) {
    // Common pattern: weak_ptr to shared resource
    std::weak_ptr<int> weak;
    
    {
        auto shared = std::make_shared<int>(42);
        weak = shared;
        
        // Use while alive
        if (auto locked = safe_lock(weak)) {
            EXPECT_EQ(**locked, 42);
        } else {
            FAIL() << "Lock should succeed while shared is alive";
        }
    } // shared destroyed
    
    // Try to use after destruction
    if (auto locked = safe_lock(weak)) {
        FAIL() << "Lock should fail after shared is destroyed";
    }
    // This is expected behavior
}
