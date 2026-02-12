#include <gtest/gtest.h>
#include "utils/safe_access.h"
#include <vector>
#include <map>
#include <memory>

using namespace themis::utils;

// ============================================================================
// Vector Access Tests
// ============================================================================

TEST(SafeAccess, VectorInBounds) {
    std::vector<int> vec = {1, 2, 3, 4, 5};
    
    auto result = safe_get(vec, 2);
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result->get(), 3);
}

TEST(SafeAccess, VectorOutOfBounds) {
    std::vector<int> vec = {1, 2, 3};
    
    auto result = safe_get(vec, 10);
    EXPECT_FALSE(result.has_value());
}

TEST(SafeAccess, VectorEmptyAccess) {
    std::vector<int> vec;
    
    auto result = safe_get(vec, 0);
    EXPECT_FALSE(result.has_value());
}

TEST(SafeAccess, VectorConstAccess) {
    const std::vector<std::string> vec = {"hello", "world"};
    
    auto result = safe_get(vec, 1);
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result->get(), "world");
}

TEST(SafeAccess, VectorModification) {
    std::vector<int> vec = {1, 2, 3};
    
    auto result = safe_get(vec, 1);
    ASSERT_TRUE(result.has_value());
    result->get() = 42;
    
    EXPECT_EQ(vec[1], 42);
}

// ============================================================================
// Map Access Tests
// ============================================================================

TEST(SafeAccess, MapKeyExists) {
    std::map<std::string, int> map = {{"a", 1}, {"b", 2}, {"c", 3}};
    
    auto result = safe_get(map, std::string("b"));
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result->get(), 2);
}

TEST(SafeAccess, MapKeyNotFound) {
    std::map<std::string, int> map = {{"a", 1}, {"b", 2}};
    
    auto result = safe_get(map, std::string("z"));
    EXPECT_FALSE(result.has_value());
}

TEST(SafeAccess, MapEmptyAccess) {
    std::map<int, std::string> map;
    
    auto result = safe_get(map, 42);
    EXPECT_FALSE(result.has_value());
}

TEST(SafeAccess, MapConstAccess) {
    const std::map<int, std::string> map = {{1, "one"}, {2, "two"}};
    
    auto result = safe_get(map, 2);
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result->get(), "two");
}

TEST(SafeAccess, MapModification) {
    std::map<std::string, int> map = {{"score", 100}};
    
    auto result = safe_get(map, std::string("score"));
    ASSERT_TRUE(result.has_value());
    result->get() = 200;
    
    EXPECT_EQ(map["score"], 200);
}

// ============================================================================
// Pointer Dereference Tests
// ============================================================================

TEST(SafeAccess, PointerValid) {
    int value = 42;
    int* ptr = &value;
    
    auto result = safe_deref(ptr);
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result->get(), 42);
}

TEST(SafeAccess, PointerNull) {
    int* ptr = nullptr;
    
    auto result = safe_deref(ptr);
    EXPECT_FALSE(result.has_value());
}

TEST(SafeAccess, PointerConstAccess) {
    const double value = 3.14;
    const double* ptr = &value;
    
    auto result = safe_deref(ptr);
    ASSERT_TRUE(result.has_value());
    EXPECT_DOUBLE_EQ(result->get(), 3.14);
}

TEST(SafeAccess, PointerModification) {
    std::string value = "hello";
    std::string* ptr = &value;
    
    auto result = safe_deref(ptr);
    ASSERT_TRUE(result.has_value());
    result->get() = "world";
    
    EXPECT_EQ(value, "world");
}

// ============================================================================
// Smart Pointer Tests
// ============================================================================

TEST(SafeAccess, SharedPtrValid) {
    auto ptr = std::make_shared<int>(42);
    
    EXPECT_NO_THROW({
        int* raw = checked_get(ptr, "test");
        EXPECT_NE(raw, nullptr);
        EXPECT_EQ(*raw, 42);
    });
}

TEST(SafeAccess, SharedPtrNull) {
    std::shared_ptr<int> ptr;
    
    EXPECT_THROW({
        checked_get(ptr, "test context");
    }, std::runtime_error);
}

TEST(SafeAccess, UniquePtrValid) {
    auto ptr = std::make_unique<std::string>("test");
    
    EXPECT_NO_THROW({
        std::string* raw = checked_get(ptr, "test");
        EXPECT_NE(raw, nullptr);
        EXPECT_EQ(*raw, "test");
    });
}

TEST(SafeAccess, UniquePtrNull) {
    std::unique_ptr<int> ptr;
    
    EXPECT_THROW({
        checked_get(ptr, "null unique_ptr");
    }, std::runtime_error);
}

// ============================================================================
// Dynamic Cast Tests
// ============================================================================

struct Base {
    virtual ~Base() = default;
    virtual int getValue() const { return 0; }
};

struct Derived : Base {
    int getValue() const override { return 42; }
    void specificMethod() const {}
};

struct OtherDerived : Base {
    int getValue() const override { return 99; }
};

TEST(SafeAccess, SafeCastSuccess) {
    Derived derived;
    Base* base = &derived;
    
    auto result = safe_cast<Derived>(base);
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result.value()->getValue(), 42);
}

TEST(SafeAccess, SafeCastFailure) {
    OtherDerived other;
    Base* base = &other;
    
    auto result = safe_cast<Derived>(base);
    EXPECT_FALSE(result.has_value());
}

TEST(SafeAccess, SafeCastNull) {
    Base* base = nullptr;
    
    auto result = safe_cast<Derived>(base);
    EXPECT_FALSE(result.has_value());
}

TEST(SafeAccess, SafeCastConst) {
    const Derived derived;
    const Base* base = &derived;
    
    auto result = safe_cast<Derived>(base);
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result.value()->getValue(), 42);
}

// ============================================================================
// Integration Tests
// ============================================================================

TEST(SafeAccess, ChainedAccess) {
    std::map<std::string, std::vector<int>> data = {
        {"numbers", {1, 2, 3, 4, 5}}
    };
    
    // Chain safe_get for map and vector
    auto vec_ref = safe_get(data, std::string("numbers"));
    ASSERT_TRUE(vec_ref.has_value());
    
    auto val_ref = safe_get(vec_ref->get(), 2);
    ASSERT_TRUE(val_ref.has_value());
    EXPECT_EQ(val_ref->get(), 3);
}

TEST(SafeAccess, ChainedAccessFailure) {
    std::map<std::string, std::vector<int>> data = {
        {"numbers", {1, 2, 3}}
    };
    
    // Try to access non-existent key
    auto vec_ref = safe_get(data, std::string("missing"));
    EXPECT_FALSE(vec_ref.has_value());
}

TEST(SafeAccess, PointerAndCastIntegration) {
    auto derived = std::make_unique<Derived>();
    Base* base = derived.get();
    
    // First check pointer is valid
    auto deref = safe_deref(base);
    ASSERT_TRUE(deref.has_value());
    
    // Then try to cast
    auto casted = safe_cast<Derived>(base);
    ASSERT_TRUE(casted.has_value());
    EXPECT_EQ(casted.value()->getValue(), 42);
}

// ============================================================================
// Edge Cases
// ============================================================================

TEST(SafeAccess, VectorBoundary) {
    std::vector<int> vec = {1, 2, 3};
    
    // Last valid index
    auto result1 = safe_get(vec, 2);
    ASSERT_TRUE(result1.has_value());
    EXPECT_EQ(result1->get(), 3);
    
    // First invalid index
    auto result2 = safe_get(vec, 3);
    EXPECT_FALSE(result2.has_value());
}

TEST(SafeAccess, LargeIndex) {
    std::vector<int> vec = {1};
    
    auto result = safe_get(vec, SIZE_MAX);
    EXPECT_FALSE(result.has_value());
}

TEST(SafeAccess, MultiLevelInheritance) {
    struct MiddleDerived : Derived {
        int getValue() const override { return 100; }
    };
    
    MiddleDerived middle;
    Base* base = &middle;
    
    // Cast to intermediate type
    auto result1 = safe_cast<Derived>(base);
    ASSERT_TRUE(result1.has_value());
    
    // Cast to most derived type
    auto result2 = safe_cast<MiddleDerived>(base);
    ASSERT_TRUE(result2.has_value());
    EXPECT_EQ(result2.value()->getValue(), 100);
}
