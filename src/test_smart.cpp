#include <cassert>
#include <iostream>
#include <memory>
#include <unique_ptr.hpp>
#include <utility>

// 辅助宏，用于输出测试信息
#define TEST_SECTION(name) std::cout << "\n--- Testing: " << name << " ---\n";
#define ASSERT_EQUAL(std_expr, my_expr, msg)                                \
  do {                                                                      \
    if ((std_expr) != (my_expr)) {                                          \
      std::cerr << "Assertion failed: " << msg << std::endl;                \
      std::cerr << "  std::unique_ptr result: " << (std_expr) << std::endl; \
      std::cerr << "  my_unique_ptr result: " << (my_expr) << std::endl;    \
      assert(false);                                                        \
    }                                                                       \
  } while (0)

// 自定义删除器，用于测试删除器行为
struct MyDeleter {
  void operator()(int* p) const noexcept {
    std::cout << "MyDeleter is called for value: ";
    if (p) {
      std::cout << *p << std::endl;
      delete p;
    } else {
      std::cout << "nullptr" << std::endl;
    }
  }
};

int main() {
  TEST_SECTION("Default Construction");
  {
    std::unique_ptr<int> std_ptr;
    unique_ptr<int> my_ptr;
    ASSERT_EQUAL(static_cast<bool>(std_ptr), static_cast<bool>(my_ptr),
                 "Default construction bool check");
    std::cout << "Default construction: std_ptr is "
              << (std_ptr ? "not null" : "null") << ", my_ptr is "
              << (my_ptr ? "not null" : "null") << std::endl;
  }

  TEST_SECTION("Pointer Construction");
  {
    int* raw_ptr_std = new int(42);
    int* raw_ptr_my = new int(42);
    std::unique_ptr<int> std_ptr(raw_ptr_std);
    unique_ptr<int> my_ptr(raw_ptr_my);
    ASSERT_EQUAL(static_cast<bool>(std_ptr), static_cast<bool>(my_ptr),
                 "Pointer construction bool check");
    ASSERT_EQUAL(*std_ptr, *my_ptr, "Pointer construction value check");
    std::cout << "Pointer construction: std_ptr manages " << *std_ptr
              << ", my_ptr manages " << *my_ptr << std::endl;
  }

  TEST_SECTION("Pointer Construction with Custom Deleter");
  {
    int* raw_ptr_std = new int(99);
    int* raw_ptr_my = new int(99);
    std::unique_ptr<int, MyDeleter> std_ptr(raw_ptr_std, MyDeleter());
    unique_ptr<int, MyDeleter> my_ptr(raw_ptr_my, MyDeleter());
    ASSERT_EQUAL(static_cast<bool>(std_ptr), static_cast<bool>(my_ptr),
                 "Pointer and Deleter construction bool check");
    ASSERT_EQUAL(*std_ptr, *my_ptr,
                 "Pointer and Deleter construction value check");
    std::cout << "Pointer and Deleter construction: std_ptr manages "
              << *std_ptr << ", my_ptr manages " << *my_ptr << std::endl;
    // Deleter will be called when ptrs go out of scope
  }
  TEST_SECTION("Move Construction");
  {
    std::unique_ptr<int> std_ptr1(new int(100));
    unique_ptr<int> my_ptr1(new int(101));

    std::unique_ptr<int> std_ptr2(std::move(std_ptr1));
    unique_ptr<int> my_ptr2(std::move(my_ptr1));

    ASSERT_EQUAL(static_cast<bool>(std_ptr1), static_cast<bool>(my_ptr1),
                 "Move construction after move ptr1 bool check");
    ASSERT_EQUAL(static_cast<bool>(std_ptr2), static_cast<bool>(my_ptr2),
                 "Move construction ptr2 bool check");
    ASSERT_EQUAL(*std_ptr2 + 1, *my_ptr2, "Move construction value check");

    std::cout << "Move construction: std_ptr2 manages " << *std_ptr2
              << ", my_ptr2 manages " << *my_ptr2 << std::endl;
    std::cout << "After move, std_ptr1 is " << (std_ptr1 ? "not null" : "null")
              << ", my_ptr1 is " << (my_ptr1 ? "not null" : "null")
              << std::endl;
  }

  TEST_SECTION("Release");
  {
    std::unique_ptr<int> std_ptr(new int(50));
    unique_ptr<int> my_ptr(new int(51));

    int* std_raw_ptr = std_ptr.release();
    int* my_raw_ptr = my_ptr.release();

    ASSERT_EQUAL(static_cast<bool>(std_ptr), static_cast<bool>(my_ptr),
                 "Release after release unique_ptr bool check");
    ASSERT_EQUAL(*std_raw_ptr + 1, *my_raw_ptr,
                 "Release raw pointer value check");

    std::cout << "Release: std_raw_ptr points to " << *std_raw_ptr
              << ", my_raw_ptr points to " << *my_raw_ptr << std::endl;
    std::cout << "After release, std_ptr is " << (std_ptr ? "not null" : "null")
              << ", my_ptr is " << (my_ptr ? "not null" : "null") << std::endl;

    delete std_raw_ptr;  // Remember to delete raw pointers after release!
    delete my_raw_ptr;
  }

  TEST_SECTION("Reset");
  {
    std::unique_ptr<int> std_ptr(new int(60));
    unique_ptr<int> my_ptr(new int(61));

    std_ptr.reset(new int(70));
    my_ptr.reset(new int(71));
    ASSERT_EQUAL(*std_ptr + 1, *my_ptr, "Reset with new pointer value check");
    std::cout << "Reset with new pointer: std_ptr manages " << *std_ptr
              << ", my_ptr manages " << *my_ptr << std::endl;

    std_ptr.reset(nullptr);
    my_ptr.reset(nullptr);
    ASSERT_EQUAL(static_cast<bool>(std_ptr), static_cast<bool>(my_ptr),
                 "Reset with nullptr bool check");
    std::cout << "Reset with nullptr: std_ptr is "
              << (std_ptr ? "not null" : "null") << ", my_ptr is "
              << (my_ptr ? "not null" : "null") << std::endl;
  }

  TEST_SECTION("Swap");
  {
    std::unique_ptr<int> std_ptr1(new int(80));
    std::unique_ptr<int> std_ptr2(new int(90));
    unique_ptr<int> my_ptr1(new int(81));
    unique_ptr<int> my_ptr2(new int(91));

    std_ptr1.swap(std_ptr2);
    my_ptr1.swap(my_ptr2);

    ASSERT_EQUAL(*std_ptr1 + 1, *my_ptr1, "Swap ptr1 value check");
    ASSERT_EQUAL(*std_ptr2 + 1, *my_ptr2, "Swap ptr2 value check");

    std::cout << "Swap: std_ptr1 now manages " << *std_ptr1
              << ", std_ptr2 manages " << *std_ptr2 << ", my_ptr1 now manages "
              << *my_ptr1 << ", my_ptr2 manages " << *my_ptr2 << std::endl;
  }

  TEST_SECTION("Get");
  {
    std::unique_ptr<int> std_ptr(new int(102));
    unique_ptr<int> my_ptr(new int(103));

    int* std_raw_ptr = std_ptr.get();
    int* my_raw_ptr = my_ptr.get();

    ASSERT_EQUAL(*std_raw_ptr + 1, *my_raw_ptr, "Get raw pointer value check");
    ASSERT_EQUAL(static_cast<bool>(std_ptr), static_cast<bool>(my_ptr),
                 "Get after get() unique_ptr bool check");

    std::cout << "Get: std_ptr.get() points to " << *std_raw_ptr
              << ", my_ptr.get() points to " << *my_raw_ptr << std::endl;
    std::cout << "After get(), std_ptr is still "
              << (std_ptr ? "not null" : "null") << ", my_ptr is still "
              << (my_ptr ? "not null" : "null") << std::endl;
  }

  TEST_SECTION("Operator* and Operator->");
  {
    std::unique_ptr<int> std_ptr(new int(110));
    unique_ptr<int> my_ptr(new int(111));

    ASSERT_EQUAL(*std_ptr + 1, *my_ptr, "Operator* value check");
    std::cout << "Operator*: *std_ptr is " << *std_ptr << ", *my_ptr is "
              << *my_ptr << std::endl;

    struct TestStruct {
      int value;
    };
    std::unique_ptr<TestStruct> std_struct_ptr(new TestStruct{120});
    unique_ptr<TestStruct> my_struct_ptr(new TestStruct{121});

    ASSERT_EQUAL(std_struct_ptr->value + 1, my_struct_ptr->value,
                 "Operator-> value check");
    std::cout << "Operator->: std_struct_ptr->value is "
              << std_struct_ptr->value << ", my_struct_ptr->value is "
              << my_struct_ptr->value << std::endl;
  }

  TEST_SECTION("Operator bool");
  {
    std::unique_ptr<int> std_ptr1(new int(130));
    std::unique_ptr<int> std_ptr2;
    unique_ptr<int> my_ptr1(new int(131));
    unique_ptr<int> my_ptr2;

    ASSERT_EQUAL(static_cast<bool>(std_ptr1), static_cast<bool>(my_ptr1),
                 "Operator bool ptr1 check");
    ASSERT_EQUAL(static_cast<bool>(std_ptr2), static_cast<bool>(my_ptr2),
                 "Operator bool ptr2 check");

    std::cout << "Operator bool: std_ptr1 is " << (std_ptr1 ? "true" : "false")
              << ", std_ptr2 is " << (std_ptr2 ? "true" : "false")
              << ", my_ptr1 is " << (my_ptr1 ? "true" : "false")
              << ", my_ptr2 is " << (my_ptr2 ? "true" : "false") << std::endl;
  }

  TEST_SECTION("Move Assignment");
  {
    std::unique_ptr<int> std_ptr1(new int(140));
    std::unique_ptr<int> std_ptr2;
    unique_ptr<int> my_ptr1(new int(141));
    unique_ptr<int> my_ptr2;

    std_ptr2 = std::move(std_ptr1);
    my_ptr2 = std::move(my_ptr1);

    ASSERT_EQUAL(static_cast<bool>(std_ptr1), static_cast<bool>(my_ptr1),
                 "Move Assignment after move ptr1 bool check");
    ASSERT_EQUAL(static_cast<bool>(std_ptr2), static_cast<bool>(my_ptr2),
                 "Move Assignment ptr2 bool check");
    ASSERT_EQUAL(*std_ptr2 + 1, *my_ptr2, "Move Assignment value check");

    std::cout << "Move assignment: std_ptr2 manages " << *std_ptr2
              << ", my_ptr2 manages " << *my_ptr2 << std::endl;
    std::cout << "After move assignment, std_ptr1 is "
              << (std_ptr1 ? "not null" : "null") << ", my_ptr1 is "
              << (my_ptr1 ? "not null" : "null") << std::endl;
  }

  TEST_SECTION("Assignment from nullptr");
  {
    std::unique_ptr<int> std_ptr(new int(150));
    unique_ptr<int> my_ptr(new int(151));

    std_ptr = nullptr;
    my_ptr = nullptr;

    ASSERT_EQUAL(static_cast<bool>(std_ptr), static_cast<bool>(my_ptr),
                 "Assignment from nullptr bool check");
    std::cout << "Assignment from nullptr: std_ptr is "
              << (std_ptr ? "not null" : "null") << ", my_ptr is "
              << (my_ptr ? "not null" : "null") << std::endl;
  }

  TEST_SECTION("Custom Deleter Called on Destruction");
  {
    std::cout << "Expect 'MyDeleter is called for value: 160' and 'MyDeleter "
                 "is called for value: 161' when exiting scope:"
              << std::endl;
    {
      std::unique_ptr<int, MyDeleter> std_ptr(new int(160), MyDeleter());
      unique_ptr<int, MyDeleter> my_ptr(new int(161), MyDeleter());
      // Destructors will be called at the end of this scope, invoking MyDeleter
    }
  }

  std::cout << "\n--- All tests completed ---\n";
  return 0;
}