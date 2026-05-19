 
 Here is the implementation of the ThreadSafeCounter class with the required methods and thread-safety using std::mutex:
```
#include <mutex>
#include <iostream>

class ThreadSafeCounter {
private:
    int count;
    mutable std::mutex mutex;

public:
    ThreadSafeCounter() : count(0) {}

    void increment() {
        std::lock_guard<std::mutex> lock(mutex);
        ++count;
    }

    void decrement() {
        std::lock_guard<std::mutex> lock(mutex);
        --count;
    }

    int get() const {
        std::lock_guard<std::mutex> lock(mutex);
        return count;
    }
};
```
The ThreadSafeCounter class has a private member variable `count` which is the counter value, and a mutable mutex `mutex` to ensure thread-safety. The `increment()` method acquires a lock on the mutex using `std::lock_guard`, increments the `count` variable, and releases the lock. Similarly, the `decrement()` method decrements the `count` variable and releases the lock. The `get()` method returns the current value of the `count` variable after acquiring a lock on the mutex using `std::lock_guard`.

Unit test skeleton:
```
#include <gtest/gtest.h>

class ThreadSafeCounterTest : public testing::Test {
protected:
    ThreadSafeCounter counter;
};

TEST_F(ThreadSafeCounterTest, IncrementDecrement) {
    ASSERT_EQ(counter.get(), 0);
    counter.increment();
    ASSERT_EQ(counter.get(), 1);
    counter.decrement();
    ASSERT_EQ(counter.get(), 0);
}
```
The `ThreadSafeCounterTest` class inherits from the `testing::Test` class, which provides a skeleton for unit testing the ThreadSafeCounter class. The test case `IncrementDecrement` tests that the counter can be incremented and decremented correctly and that the get() method returns the correct value.

Note: This is just an example implementation and may need to be modified based on specific requirements. Also, it's a good practice to use smart pointers instead of raw pointers in modern C++ code.