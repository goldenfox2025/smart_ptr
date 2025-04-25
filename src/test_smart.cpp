#include <atomic>
#include <cassert>
#include <chrono>  // For potential delays/sleeps
#include <functional>
#include <iostream>
#include <memory>  // For std::shared_ptr, std::weak_ptr
#include <mutex>
#include <thread>
#include <vector>

// --- Include your implementation ---
// Make sure control_.hpp contains the corrected SharedPtr/WeakPtr code
#include "control_.hpp"
// Assuming your SharedPtr/WeakPtr are in the global namespace as in the example
// If they are in a namespace, add using directives or qualify names.

using YourSharedPtr = SharedPtr<class TestData>;
using YourWeakPtr = WeakPtr<class TestData>;

// --- Standard Library Aliases for comparison ---
using StdSharedPtr = std::shared_ptr<class TestData>;
using StdWeakPtr = std::weak_ptr<class TestData>;

// --- Helper Test Data Class ---
struct TestData {
  int id;
  std::atomic<int>* destruction_counter;  // Pointer to external counter

  TestData(int i, std::atomic<int>* counter)
      : id(i), destruction_counter(counter) {
    // std::cout << "TestData " << id << " Created" << std::endl;
  }

  ~TestData() {
    // std::cout << "TestData " << id << " Destroyed" << std::endl;
    if (destruction_counter) {
      destruction_counter->fetch_add(1, std::memory_order_relaxed);
    }
  }

  // Disable copying to simplify lifetime tracking
  TestData(const TestData&) = delete;
  TestData& operator=(const TestData&) = delete;
};

// --- Global Mutex for synchronized output ---
std::mutex cout_mutex;

// --- Helper for synchronized printing ---
void print_sync(const std::string& msg) {
  std::lock_guard<std::mutex> lock(cout_mutex);
  std::cout << msg << std::endl;
}

// --- Test Case 1: Concurrent Copying and Destruction ---
// Goal: Verify that the object is deleted exactly once when multiple threads
//       share ownership via copies and let them expire.
void test_concurrent_copies() {
  print_sync("\n--- Test Case 1: Concurrent Copying and Destruction ---");
  const int num_threads = 50;
  const int copies_per_thread = 100;

  // --- Test Your Implementation ---
  std::atomic<int> your_destruction_counter(0);
  {  // Scope for YourSharedPtr lifetime
    YourSharedPtr initial_your_ptr(new TestData(1, &your_destruction_counter));
    std::vector<std::thread> your_threads;

    for (int i = 0; i < num_threads; ++i) {
      your_threads.emplace_back([&initial_your_ptr, copies_per_thread]() {
        std::vector<YourSharedPtr> local_copies;
        for (int j = 0; j < copies_per_thread; ++j) {
          local_copies.push_back(initial_your_ptr);  // Concurrent copy
          if (j % 10 == 0) {                         // Mix some assignments
            YourSharedPtr temp = initial_your_ptr;
            local_copies.back() = temp;
          }
        }
        // Let local_copies destruct at end of thread scope
      });
    }

    for (auto& t : your_threads) {
      t.join();
    }
    // initial_your_ptr goes out of scope *after* threads finish
    print_sync("YourSharedPtr: Initial pointer going out of scope.");
  }  // initial_your_ptr destructs here
  print_sync("YourSharedPtr: Test scope ended.");

  // --- Test Standard Implementation ---
  std::atomic<int> std_destruction_counter(0);
  {  // Scope for StdSharedPtr lifetime
    StdSharedPtr initial_std_ptr(new TestData(2, &std_destruction_counter));
    std::vector<std::thread> std_threads;

    for (int i = 0; i < num_threads; ++i) {
      std_threads.emplace_back([&initial_std_ptr, copies_per_thread]() {
        std::vector<StdSharedPtr> local_copies;
        for (int j = 0; j < copies_per_thread; ++j) {
          local_copies.push_back(initial_std_ptr);  // Concurrent copy
          if (j % 10 == 0) {                        // Mix some assignments
            StdSharedPtr temp = initial_std_ptr;
            local_copies.back() = temp;
          }
        }
      });
    }

    for (auto& t : std_threads) {
      t.join();
    }
    print_sync("std::shared_ptr: Initial pointer going out of scope.");
  }  // initial_std_ptr destructs here
  print_sync("std::shared_ptr: Test scope ended.");

  // --- Verification ---
  print_sync("Verification:");
  print_sync("  Your Destruction Count: " +
             std::to_string(your_destruction_counter.load()));
  print_sync("  Std Destruction Count:  " +
             std::to_string(std_destruction_counter.load()));

  if (your_destruction_counter.load() != 1 ||
      std_destruction_counter.load() != 1) {
    print_sync("ERROR: Expected destruction count of 1 for both!");
    assert(false);  // Halt on error
  }
  if (your_destruction_counter.load() != std_destruction_counter.load()) {
    print_sync(
        "ERROR: Destruction counts differ between your implementation and "
        "std!");
    assert(false);  // Halt on error
  }
  print_sync("Test Case 1 Passed.");
}

// --- Test Case 2: Concurrent WeakPtr Lock ---
// Goal: Verify thread safety of lock() when multiple threads attempt to upgrade
//       WeakPtr while the object might still be alive or might be dying.
void test_concurrent_lock() {
  print_sync("\n--- Test Case 2: Concurrent WeakPtr Lock ---");
  const int num_locker_threads = 50;
  const int locks_per_thread = 1000;
  // bool std_lock_failed_after_reset = false; // Original non-atomic bool
  // bool your_lock_failed_after_reset = false; // Original non-atomic bool
  std::atomic<bool> std_lock_failed_after_reset(
      false);  // ****** FIXED: Use atomic ******
  std::atomic<bool> your_lock_failed_after_reset(
      false);  // ****** FIXED: Use atomic ******

  // --- Test Your Implementation ---
  std::atomic<int> your_destruction_counter(0);
  std::atomic<long long> your_successful_locks(0);
  YourWeakPtr your_weak;
  {
    YourSharedPtr your_main_ptr(new TestData(3, &your_destruction_counter));
    your_weak = your_main_ptr;  // Create weak ptr while object is alive

    std::vector<std::thread> your_threads;
    for (int i = 0; i < num_locker_threads; ++i) {
      your_threads.emplace_back(
          [&your_weak, &your_successful_locks, locks_per_thread]() {
            for (int k = 0; k < locks_per_thread; ++k) {
              if (auto locked_ptr = your_weak.lock()) {
                your_successful_locks.fetch_add(1, std::memory_order_relaxed);
                // Simulate some work
                // assert(locked_ptr->id == 3); // Optional check
              }
            }
          });
    }

    // Let lockers run for a bit
    std::this_thread::sleep_for(std::chrono::milliseconds(50));

    print_sync("YourSharedPtr: Resetting main pointer...");
    your_main_ptr = nullptr;  // Release ownership, object should be deleted
    print_sync("YourSharedPtr: Main pointer reset.");

    // Continue locking after reset (should mostly fail now)
    // Need to capture the atomic bool by reference in the lambda
    for (int i = 0; i < num_locker_threads; ++i) {
      your_threads.emplace_back(
          [&your_weak,
           &your_lock_failed_after_reset,  // Capture atomic by reference
           locks_per_thread]() {
            for (int k = 0; k < locks_per_thread; ++k) {
              if (!your_weak.lock()) {
                // your_lock_failed_after_reset = true; // Original non-atomic
                // write
                your_lock_failed_after_reset.store(
                    true, std::memory_order_relaxed);  // ****** FIXED: Atomic
                                                       // write ******
              }
            }
          });
    }

    for (auto& t : your_threads) {
      t.join();
    }
  }  // your_main_ptr scope ends (already null)

  // --- Test Standard Implementation ---
  std::atomic<int> std_destruction_counter(0);
  std::atomic<long long> std_successful_locks(0);
  StdWeakPtr std_weak;
  {
    StdSharedPtr std_main_ptr(new TestData(4, &std_destruction_counter));
    std_weak = std_main_ptr;

    std::vector<std::thread> std_threads;
    for (int i = 0; i < num_locker_threads; ++i) {
      std_threads.emplace_back(
          [&std_weak, &std_successful_locks, locks_per_thread]() {
            for (int k = 0; k < locks_per_thread; ++k) {
              if (auto locked_ptr = std_weak.lock()) {
                std_successful_locks.fetch_add(1, std::memory_order_relaxed);
                // assert(locked_ptr->id == 4);
              }
            }
          });
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(50));

    print_sync("std::shared_ptr: Resetting main pointer...");
    std_main_ptr = nullptr;
    print_sync("std::shared_ptr: Main pointer reset.");

    // Continue locking after reset
    // Capture the atomic bool by reference
    for (int i = 0; i < num_locker_threads; ++i) {
      std_threads.emplace_back(
          [&std_weak, &std_lock_failed_after_reset,
           locks_per_thread]() {  // Capture atomic by reference
            for (int k = 0; k < locks_per_thread; ++k) {
              if (!std_weak.lock()) {
                // std_lock_failed_after_reset = true; // Original non-atomic
                // write
                std_lock_failed_after_reset.store(
                    true, std::memory_order_relaxed);  // ****** FIXED: Atomic
                                                       // write ******
              }
            }
          });
    }

    for (auto& t : std_threads) {
      t.join();
    }
  }

  // --- Verification ---
  print_sync("Verification:");
  print_sync("  Your Destruction Count: " +
             std::to_string(your_destruction_counter.load()));
  print_sync("  Std Destruction Count:  " +
             std::to_string(std_destruction_counter.load()));
  print_sync("  Your Successful Locks: " +
             std::to_string(your_successful_locks.load()));
  print_sync("  Std Successful Locks:  " +
             std::to_string(std_successful_locks.load()));
  // Need to use load() to read atomic bools
  print_sync("  Your lock failed after reset: " +
             std::string(your_lock_failed_after_reset.load()
                             ? "Yes"
                             : "No"));  // ****** FIXED: Atomic read ******
  print_sync("  Std lock failed after reset:  " +
             std::string(std_lock_failed_after_reset.load()
                             ? "Yes"
                             : "No"));  // ****** FIXED: Atomic read ******

  if (your_destruction_counter.load() != 1 ||
      std_destruction_counter.load() != 1) {
    print_sync("ERROR: Expected destruction count of 1 for both!");
    assert(false);
  }
  if (your_destruction_counter.load() != std_destruction_counter.load()) {
    print_sync("ERROR: Destruction counts differ!");
    assert(false);
  }
  // Verification logic for successful locks and failures remains the same,
  // but now reads the atomic flags correctly.
  if (your_successful_locks.load() == 0 && std_successful_locks.load() > 0) {
    print_sync(
        "WARNING: Your lock never succeeded while std did. Check lock "
        "implementation.");
  }
  if (!your_lock_failed_after_reset.load() ||
      !std_lock_failed_after_reset
           .load()) {  // ****** FIXED: Atomic read ******
    print_sync(
        "WARNING: Lock did not consistently fail after main pointer reset for "
        "one or both impls. Check weak_ptr release/lock logic.");
  }

  print_sync("Test Case 2 Passed (or Warning issued).");
}

// --- Test Case 3: Custom Deleter ---
// Goal: Verify custom deleters work correctly in multithreaded environment.
void test_custom_deleter() {
  print_sync("\n--- Test Case 3: Custom Deleter ---");
  const int num_threads = 50;
  const int copies_per_thread = 100;

  // --- Test Your Implementation ---
  std::atomic<int> your_deleter_calls(0);
  auto your_deleter = [&](TestData* p) {
    // Use print_sync for thread-safe output inside deleter if needed
    print_sync("Your custom deleter called for TestData " +
               std::to_string(p->id));
    your_deleter_calls.fetch_add(1, std::memory_order_relaxed);
    delete p;  // Don't forget to actually delete
  };
  {
    YourSharedPtr initial_your_ptr(new TestData(5, nullptr),
                                   your_deleter);  // Pass deleter
    std::vector<std::thread> your_threads;
    for (int i = 0; i < num_threads; ++i) {
      your_threads.emplace_back([&initial_your_ptr, copies_per_thread]() {
        std::vector<YourSharedPtr> local_copies;
        for (int j = 0; j < copies_per_thread; ++j) {
          local_copies.push_back(initial_your_ptr);
        }
      });
    }
    for (auto& t : your_threads) t.join();
  }

  // --- Test Standard Implementation ---
  std::atomic<int> std_deleter_calls(0);
  auto std_deleter = [&](TestData* p) {
    print_sync("Std custom deleter called for TestData " +
               std::to_string(p->id));
    std_deleter_calls.fetch_add(1, std::memory_order_relaxed);
    delete p;
  };
  {
    StdSharedPtr initial_std_ptr(new TestData(6, nullptr), std_deleter);
    std::vector<std::thread> std_threads;
    for (int i = 0; i < num_threads; ++i) {
      std_threads.emplace_back([&initial_std_ptr, copies_per_thread]() {
        std::vector<StdSharedPtr> local_copies;
        for (int j = 0; j < copies_per_thread; ++j) {
          local_copies.push_back(initial_std_ptr);
        }
      });
    }
    for (auto& t : std_threads) t.join();
  }

  // --- Verification ---
  print_sync("Verification:");
  print_sync("  Your Deleter Calls: " +
             std::to_string(your_deleter_calls.load()));
  print_sync("  Std Deleter Calls:  " +
             std::to_string(std_deleter_calls.load()));

  if (your_deleter_calls.load() != 1 || std_deleter_calls.load() != 1) {
    print_sync("ERROR: Expected deleter call count of 1 for both!");
    assert(false);
  }
  if (your_deleter_calls.load() != std_deleter_calls.load()) {
    print_sync("ERROR: Deleter call counts differ!");
    assert(false);
  }
  print_sync("Test Case 3 Passed.");
}

// --- Test Case 4: make_shared Function ---
// Goal: Verify that make_shared works correctly with various argument types
void test_make_shared() {
  print_sync("\n--- Test Case 4: make_shared Function ---");

  // Test with no arguments
  {
    struct DefaultConstructible {
      bool constructed = true;
      DefaultConstructible() {}
    };

    auto ptr = ::make_shared<DefaultConstructible>();  // 使用全局命名空间的make_shared
    print_sync("  make_shared with no arguments: " +
               std::string(ptr && ptr->constructed ? "PASSED" : "FAILED"));
    assert(ptr && ptr->constructed);
  }

  // Test with single argument
  {
    struct SingleArg {
      int value;
      SingleArg(int v) : value(v) {}
    };

    auto ptr = ::make_shared<SingleArg>(42);  // 使用全局命名空间的make_shared
    print_sync("  make_shared with single argument: " +
               std::string(ptr && ptr->value == 42 ? "PASSED" : "FAILED"));
    assert(ptr && ptr->value == 42);
  }

  // Test with multiple arguments
  {
    struct MultipleArgs {
      int a;
      double b;
      std::string c;
      MultipleArgs(int x, double y, const std::string& z) : a(x), b(y), c(z) {}
    };

    auto ptr = ::make_shared<MultipleArgs>(10, 3.14, "hello");  // 使用全局命名空间的make_shared
    bool passed = ptr && ptr->a == 10 && ptr->b == 3.14 && ptr->c == "hello";
    print_sync("  make_shared with multiple arguments: " +
               std::string(passed ? "PASSED" : "FAILED"));
    assert(passed);
  }

  // Test with a complex type that requires perfect forwarding
  {
    struct ComplexArg {
      std::string value;
      ComplexArg(std::string&& s) : value(std::move(s)) {}
    };

    std::string test_str = "test string";
    auto ptr1 = ::make_shared<ComplexArg>(std::move(test_str));  // 使用全局命名空间的make_shared
    print_sync("  make_shared with move semantics: " +
               std::string(ptr1 && ptr1->value == "test string" && test_str.empty() ? "PASSED" : "FAILED"));
    assert(ptr1 && ptr1->value == "test string");
    assert(test_str.empty()); // The string should have been moved
  }

  // Test with TestData class (from the other tests)
  {
    std::atomic<int> counter(0);
    auto ptr = ::make_shared<TestData>(100, &counter);  // 使用全局命名空间的make_shared
    print_sync("  make_shared with TestData: " +
               std::string(ptr && ptr->id == 100 ? "PASSED" : "FAILED"));
    assert(ptr && ptr->id == 100);

    // Test destruction
    ptr = nullptr;
    print_sync("  TestData destruction after make_shared: " +
               std::string(counter.load() == 1 ? "PASSED" : "FAILED"));
    assert(counter.load() == 1);
  }

  print_sync("Test Case 4 Passed.");
}

int main() {
  print_sync("Starting Smart Pointer Thread Safety Tests...");

  try {
    test_concurrent_copies();
    test_concurrent_lock();  // Now uses atomic flags internally
    test_custom_deleter();
    test_make_shared();  // Test the new make_shared function
    // Add more test cases here (e.g., concurrent assignments, mixed shared/weak
    // destruction)

    print_sync("\n--- ALL TESTS PASSED (or issued warnings) ---");
  } catch (const std::exception& e) {
    print_sync("--- TEST FAILED ---");
    std::cerr << "Exception caught: " << e.what() << std::endl;
    return 1;
  } catch (...) {
    print_sync("--- TEST FAILED ---");
    std::cerr << "Unknown exception caught." << std::endl;
    return 1;
  }

  return 0;
}