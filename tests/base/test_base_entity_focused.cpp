#include <gtest/gtest.h>
#include "storage/base_entity.h"
#include <memory>

namespace themis { namespace storage { 

// Test 1: Guard rejects empty field_name
TEST(BaseEntitySetFieldTest, SetFieldFailsClosedForEmptyFieldName) {
    BaseEntity entity("test_pk_001");
    Value test_value = std::string("test_value");
    
    // Before: field_cache_ should be empty
    auto fields_before = entity.getAllFields();
    EXPECT_EQ(fields_before.size(), 0);
    
    // Call with empty field_name (should be silently rejected)
    entity.setField("", test_value);
    
    // After: field_cache_ should still be empty (no corruption)
    auto fields_after = entity.getAllFields();
    EXPECT_EQ(fields_after.size(), 0);
}

// Test 2: Guard accepts valid field_name
TEST(BaseEntitySetFieldTest, SetFieldAcceptsValidFieldName) {
    BaseEntity entity("test_pk_002");
    Value test_value = std::string("test_data");
    
    entity.setField("field_001", test_value);
    
    // Verify field was added
    auto fields = entity.getAllFields();
    EXPECT_EQ(fields.size(), 1);
    EXPECT_TRUE(fields.contains("field_001"));
}

// Test 3: Multiple fields can be set correctly
TEST(BaseEntitySetFieldTest, MultipleFieldsCanBeSetCorrectly) {
    BaseEntity entity("test_pk_003");
    Value val1 = std::string("data1");
    Value val2 = int64_t(42);
    Value val3 = double(3.14);
    
    entity.setField("field_a", val1);
    entity.setField("field_b", val2);
    entity.setField("field_c", val3);
    
    // Verify all fields were added
    auto fields = entity.getAllFields();
    EXPECT_EQ(fields.size(), 3);
    EXPECT_TRUE(fields.contains("field_a"));
    EXPECT_TRUE(fields.contains("field_b"));
    EXPECT_TRUE(fields.contains("field_c"));
}

// Test 4: Guard independence (empty/valid/empty/valid cycles work correctly)
TEST(BaseEntitySetFieldTest, FailClosedGuardsAreIndependent) {
    BaseEntity entity("test_pk_004");
    Value val_valid = std::string("valid_data");
    
    // Cycle: empty -> valid -> empty -> valid
    entity.setField("", val_valid);           // Empty: rejected
    EXPECT_EQ(entity.getAllFields().size(), 0);
    
    entity.setField("field_1", val_valid);    // Valid: accepted
    EXPECT_EQ(entity.getAllFields().size(), 1);
    
    entity.setField("", val_valid);           // Empty: rejected
    EXPECT_EQ(entity.getAllFields().size(), 1);
    
    entity.setField("field_2", val_valid);    // Valid: accepted
    EXPECT_EQ(entity.getAllFields().size(), 2);
    
    // Verify both valid fields are present
    auto fields = entity.getAllFields();
    EXPECT_TRUE(fields.contains("field_1"));
    EXPECT_TRUE(fields.contains("field_2"));
}

// Test 5: Field map corruption is prevented (no empty-key entries)
TEST(BaseEntitySetFieldTest, FieldMapCorruptionPrevented) {
    BaseEntity entity("test_pk_005");
    Value test_value = std::string("corruption_test");
    
    // Attempt to pollute field cache with empty field_name
    entity.setField("", test_value);
    entity.setField("", test_value);
    entity.setField("", test_value);
    
    // Verify no empty-key entries were created
    auto fields = entity.getAllFields();
    for (const auto& [key, val] : fields) {
        EXPECT_FALSE(key.empty()) << "Found empty-key entry in field map (data corruption)";
    }
    
    // Verify field cache remains empty
    EXPECT_EQ(fields.size(), 0);
    
    // Set one valid field
    entity.setField("valid_field", test_value);
    fields = entity.getAllFields();
    
    // Verify only the valid field exists, no corruption
    EXPECT_EQ(fields.size(), 1);
    EXPECT_TRUE(fields.contains("valid_field"));
    EXPECT_FALSE(fields.contains(""));
}
} } // namespace themis::storage
