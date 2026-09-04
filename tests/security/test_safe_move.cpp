/**
 * @file tests/security/test_safe_move.cpp
 * @brief Test suite for SafeMove security utilities
 *
 * Comprehensive tests for move operation validation, guards, and safety mechanisms.
 *
 * @author ThemisDB Team
 * @date 2026-07-05
 * @license Apache 2.0
 */

#include <gtest/gtest.h>
#include <memory>
#include <vector>
#include <string>
#include "security/safe_move.h"

namespace themis { namespace security { namespace test { 

// ============================================================================
// MoveValidator Tests
// ============================================================================

class MoveValidatorTest : public ::testing::Test {
 protected:
  struct TestObject {
    std::vector<int> data;
    bool moved_from = false;

    TestObject() = default;
    TestObject(std::vector<int> d) : data(std::move(d)) {}
    
    TestObject(TestObject&& other) noexcept 
        : data(std::move(other.data)), moved_from(false) {
      other.moved_from = true;
    }

    TestObject& operator=(TestObject&& other) noexcept {
      if (this != &other) {
        data = std::move(other.data);
        moved_from = false;
        other.moved_from = true;
      }
      return *this;
    }

    bool empty() const { return data.empty(); }
  };
};

TEST_F(MoveValidatorTest, ValidatePreMoveWithValidObject) {
  TestObject obj({1, 2, 3});
  EXPECT_NO_THROW(
    MoveValidator<TestObject>::validatePreMove(obj)
  );
}

TEST_F(MoveValidatorTest, ValidatePreMoveWithEmptyObject) {
  TestObject obj;
  EXPECT_NO_THROW(
    MoveValidator<TestObject>::validatePreMove(obj)
  );
}

TEST_F(MoveValidatorTest, ValidatePostMoveVector) {
  std::vector<int> vec({1, 2, 3});
  auto moved = std::move(vec);
  // vec should now be empty
  EXPECT_NO_THROW(
    MoveValidator<std::vector<int>>::validatePostMove(vec)
  );
  EXPECT_TRUE(vec.empty());
}

TEST_F(MoveValidatorTest, ValidateUniquePtr) {
  auto ptr = std::make_unique<int>(42);
  auto moved_ptr = std::move(ptr);
  // ptr should be null after move
  EXPECT_NO_THROW(
    MoveValidator<std::unique_ptr<int>>::validatePostMove(ptr)
  );
  EXPECT_EQ(ptr, nullptr);
  EXPECT_EQ(*moved_ptr, 42);
}

TEST_F(MoveValidatorTest, ValidateSharedPtr) {
  auto ptr = std::make_shared<int>(42);
  auto moved_ptr = std::move(ptr);
  // ptr should be null after move
  EXPECT_NO_THROW(
    MoveValidator<std::shared_ptr<int>>::validatePostMove(ptr)
  );
  EXPECT_EQ(ptr, nullptr);
  EXPECT_EQ(*moved_ptr, 42);
}

// ============================================================================
// MoveGuard Tests
// ============================================================================

class MoveGuardTest : public ::testing::Test {
 protected:
  struct GuardedObject {
    int value = 0;
    std::vector<int> data;

    GuardedObject(int v = 0) : value(v) {}
    GuardedObject(GuardedObject&& other) noexcept
        : value(other.value), data(std::move(other.data)) {}

    GuardedObject& operator=(GuardedObject&& other) noexcept {
      if (this != &other) {
        value = other.value;
        data = std::move(other.data);
      }
      return *this;
    }
  };
};

TEST_F(MoveGuardTest, GuardCreation) {
  GuardedObject obj(42);
  MoveGuard<GuardedObject> guard(&obj);
  EXPECT_TRUE(guard.checkNotMovedFrom());
  EXPECT_EQ(guard.get()->value, 42);
}

TEST_F(MoveGuardTest, GuardDetectsAfterMove) {
  GuardedObject obj(42);
  {
    MoveGuard<GuardedObject> guard(&obj);
    guard.markMovedFrom();
    EXPECT_THROW(guard.checkNotMovedFrom(), UseAfterMoveException);
  }
}

TEST_F(MoveGuardTest, GuardAccessAfterMove) {
  GuardedObject obj(42);
  {
    MoveGuard<GuardedObject> guard(&obj);
    guard.markMovedFrom();
    EXPECT_THROW(guard.get(), UseAfterMoveException);
    EXPECT_THROW(*guard, UseAfterMoveException);
    EXPECT_THROW(guard->value, UseAfterMoveException);
  }
}

TEST_F(MoveGuardTest, GuardMoveConstruction) {
  GuardedObject obj(42);
  MoveGuard<GuardedObject> guard1(&obj);
  MoveGuard<GuardedObject> guard2(std::move(guard1));
  
  // guard1 should now be empty
  EXPECT_EQ(guard1.get(), nullptr);
  // guard2 should have the object
  EXPECT_EQ(guard2.get()->value, 42);
}

TEST_F(MoveGuardTest, GuardMoveAssignment) {
  GuardedObject obj1(42);
  GuardedObject obj2(99);
  MoveGuard<GuardedObject> guard1(&obj1);
  MoveGuard<GuardedObject> guard2(&obj2);
  
  guard1 = std::move(guard2);
  
  EXPECT_EQ(guard1.get()->value, 99);
  EXPECT_EQ(guard2.get(), nullptr);
}

TEST_F(MoveGuardTest, NullPointerAssertion) {
  // Creating guard with null pointer should assert in debug mode
  // In release mode, behavior is undefined - this test documents expected behavior
  #ifdef NDEBUG
    // Skip in release mode - behavior undefined
  #else
    EXPECT_DEATH(
      (MoveGuard<GuardedObject>(nullptr)),
      ""
    );
  #endif
}

// ============================================================================
// SafeMove Tests
// ============================================================================

class SafeMoveTest : public ::testing::Test {
 protected:
  struct SafeObject {
    std::vector<int> data;
    
    SafeObject() = default;
    SafeObject(std::vector<int> d) : data(std::move(d)) {}
    
    SafeObject(SafeObject&& other) noexcept : data(std::move(other.data)) {}
    SafeObject& operator=(SafeObject&& other) noexcept {
      data = std::move(other.data);
      return *this;
    }
  };
};

TEST_F(SafeMoveTest, CreateAndTake) {
  std::vector<int> vec({1, 2, 3});
  auto safe = SafeMove<std::vector<int>>::create(std::move(vec));
  
  auto result = std::move(safe).take();
  EXPECT_EQ(result.size(), 3);
  EXPECT_EQ(result[0], 1);
  EXPECT_EQ(result[1], 2);
  EXPECT_EQ(result[2], 3);
}

TEST_F(SafeMoveTest, SafeMoveWithObject) {
  SafeObject obj({1, 2, 3, 4});
  auto safe = SafeMove<SafeObject>::create(std::move(obj));
  
  auto result = std::move(safe).take();
  EXPECT_EQ(result.data.size(), 4);
}

TEST_F(SafeMoveTest, SafeMoveIsValid) {
  std::vector<int> vec({1, 2, 3});
  auto safe = SafeMove<std::vector<int>>::create(std::move(vec));
  EXPECT_TRUE(safe.isValid());
}

TEST_F(SafeMoveTest, SafeMoveGetConst) {
  std::vector<int> vec({1, 2, 3});
  auto safe = SafeMove<std::vector<int>>::create(std::move(vec));
  
  const auto& ref = safe.get();
  EXPECT_EQ(ref.size(), 3);
}

TEST_F(SafeMoveTest, SafeMoveGetMutable) {
  std::vector<int> vec({1, 2, 3});
  auto safe = SafeMove<std::vector<int>>::create(std::move(vec));
  
  auto& ref = safe.get();
  ref.push_back(4);
  
  auto result = std::move(safe).take();
  EXPECT_EQ(result.size(), 4);
}

TEST_F(SafeMoveTest, SafeMoveMacro) {
  std::vector<int> vec({1, 2, 3});
  auto result = THEMIS_SAFE_MOVE(std::vector<int>, std::move(vec)).take();
  EXPECT_EQ(result.size(), 3);
}

// ============================================================================
// MoveChainTracker Tests
// ============================================================================

class MoveChainTrackerTest : public ::testing::Test {
 protected:
  MoveChainTracker tracker;
};

TEST_F(MoveChainTrackerTest, InitialState) {
  EXPECT_TRUE(tracker.isValid());
  EXPECT_EQ(tracker.depth.load(), 0);
  EXPECT_FALSE(tracker.error_state.load());
}

TEST_F(MoveChainTrackerTest, ChainBeginEnd) {
  tracker.onMoveBegin();
  EXPECT_EQ(tracker.depth.load(), 1);
  EXPECT_TRUE(tracker.isValid());
  
  tracker.onMoveEnd();
  EXPECT_EQ(tracker.depth.load(), 0);
  EXPECT_TRUE(tracker.isValid());
}

TEST_F(MoveChainTrackerTest, MultipleChainLevels) {
  for (int i = 0; i < 5; ++i) {
    tracker.onMoveBegin();
  }
  EXPECT_EQ(tracker.depth.load(), 5);
  EXPECT_TRUE(tracker.isValid());
  
  for (int i = 0; i < 5; ++i) {
    tracker.onMoveEnd();
  }
  EXPECT_EQ(tracker.depth.load(), 0);
  EXPECT_TRUE(tracker.isValid());
}

TEST_F(MoveChainTrackerTest, ChainDepthExceeded) {
  // Try to exceed max chain depth
  for (int i = 0; i < MoveChainTracker::MAX_CHAIN_DEPTH; ++i) {
    tracker.onMoveBegin();
  }
  EXPECT_EQ(tracker.depth.load(), MoveChainTracker::MAX_CHAIN_DEPTH);
  
  // Next onMoveBegin should throw
  EXPECT_THROW(
    tracker.onMoveBegin(),
    MoveViolationException
  );
  EXPECT_TRUE(tracker.error_state.load());
}

TEST_F(MoveChainTrackerTest, Reset) {
  tracker.onMoveBegin();
  tracker.onMoveBegin();
  EXPECT_EQ(tracker.depth.load(), 2);
  
  tracker.reset();
  EXPECT_EQ(tracker.depth.load(), 0);
  EXPECT_FALSE(tracker.error_state.load());
  EXPECT_TRUE(tracker.isValid());
}

// ============================================================================
// Exception Tests
// ============================================================================

class MoveExceptionTest : public ::testing::Test {};

TEST_F(MoveExceptionTest, UseAfterMoveException) {
  EXPECT_THROW(
    throw UseAfterMoveException(),
    MoveViolationException
  );
}

TEST_F(MoveExceptionTest, MoveSourceNotClearedException) {
  EXPECT_THROW(
    throw MoveSourceNotCleared(),
    MoveViolationException
  );
}

TEST_F(MoveExceptionTest, ExceptionMessage) {
  try {
    throw UseAfterMoveException();
  } catch (const MoveViolationException& e) {
    std::string msg(e.what());
    EXPECT_TRUE(msg.find("use-after-move") != std::string::npos ||
                msg.find("moved from") != std::string::npos);
  }
}

// ============================================================================
// Integration Tests
// ============================================================================

class MoveIntegrationTest : public ::testing::Test {
 protected:
  struct ComplexObject {
    std::vector<int> data;
    std::string name = {};
    std::unique_ptr<std::vector<double>> expensive;

    ComplexObject(const std::string& n = "")
        : name(n), expensive(std::make_unique<std::vector<double>>()) {}

    ComplexObject(ComplexObject&& other) noexcept
        : data(std::move(other.data)), name(std::move(other.name)),
          expensive(std::move(other.expensive)) {}

    ComplexObject& operator=(ComplexObject&& other) noexcept {
      if (this != &other) {
        data = std::move(other.data);
        name = std::move(other.name);
        expensive = std::move(other.expensive);
      }
      return *this;
    }
  };
};

TEST_F(MoveIntegrationTest, ComplexObjectMove) {
  ComplexObject obj("test");
  obj.data = {1, 2, 3};
  obj.expensive->push_back(3.14);

  auto safe = SafeMove<ComplexObject>::create(std::move(obj));
  EXPECT_TRUE(safe.isValid());

  auto result = std::move(safe).take();
  EXPECT_EQ(result.name, "test");
  EXPECT_EQ(result.data.size(), 3);
  EXPECT_EQ(result.expensive->size(), 1);
}

TEST_F(MoveIntegrationTest, MoveChainWithGuard) {
  ComplexObject obj("chain_test");
  obj.data = {10, 20, 30};

  MoveGuard<ComplexObject> guard(&obj);
  auto moved = std::move(obj);

  // Guard should work before marking moved from
  EXPECT_NO_THROW(guard.checkNotMovedFrom());

  // Mark as moved from
  guard.markMovedFrom();

  // Now accessing through guard should fail
  EXPECT_THROW(guard.get(), UseAfterMoveException);
}

TEST_F(MoveIntegrationTest, SequentialMoves) {
  ComplexObject obj1("first");
  obj1.data = {1, 2, 3};

  ComplexObject obj2 = std::move(obj1);
  EXPECT_EQ(obj2.name, "first");
  EXPECT_EQ(obj2.data.size(), 3);

  ComplexObject obj3 = std::move(obj2);
  EXPECT_EQ(obj3.name, "first");
  EXPECT_EQ(obj3.data.size(), 3);
}

// ============================================================================
// Specialization Tests
// ============================================================================

class MoveValidatorSpecializationTest : public ::testing::Test {};

TEST_F(MoveValidatorSpecializationTest, VectorSpecialization) {
  std::vector<int> vec({1, 2, 3});
  EXPECT_NO_THROW(
    MoveValidator<std::vector<int>>::validatePreMove(vec)
  );

  auto moved = std::move(vec);
  EXPECT_NO_THROW(
    MoveValidator<std::vector<int>>::validatePostMove(vec)
  );
  EXPECT_TRUE(vec.empty());
}

TEST_F(MoveValidatorSpecializationTest, UniquePtrSpecialization) {
  auto ptr = std::make_unique<int>(42);
  EXPECT_NO_THROW(
    MoveValidator<std::unique_ptr<int>>::validatePreMove(ptr)
  );

  auto moved_ptr = std::move(ptr);
  EXPECT_NO_THROW(
    MoveValidator<std::unique_ptr<int>>::validatePostMove(ptr)
  );
  EXPECT_EQ(ptr, nullptr);
}

TEST_F(MoveValidatorSpecializationTest, SharedPtrSpecialization) {
  auto ptr = std::make_shared<std::string>("hello");
  EXPECT_NO_THROW(
    MoveValidator<std::shared_ptr<std::string>>::validatePreMove(ptr)
  );

  auto moved_ptr = std::move(ptr);
  EXPECT_NO_THROW(
    MoveValidator<std::shared_ptr<std::string>>::validatePostMove(ptr)
  );
  EXPECT_EQ(ptr, nullptr);
}
} } } // namespace themis::security::test
