#include <iostream>
#include <string>
#include <cassert>
#include <functional>
#include <memory>
#include <vector>
#include <type_traits>

// 引入我们自己的function实现
#include "func.hpp"

// 测试用的简单函数
int add(int a, int b) {
    return a + b;
}

// 测试用的函数对象
struct Multiplier {
    int operator()(int a, int b) const {
        return a * b;
    }
};

// 带状态的函数对象
class Adder {
private:
    int base;
public:
    explicit Adder(int base) : base(base) {}
    
    int operator()(int x) const {
        return base + x;
    }
};

// 测试基本功能
void test_basic_functionality() {
    std::cout << "=== 测试基本功能 ===" << std::endl;
    
    // 测试默认构造
    mystd::function<int(int, int)> f1;
    assert(!f1);
    std::cout << "默认构造测试通过" << std::endl;
    
    // 测试nullptr构造
    mystd::function<int(int, int)> f2 = nullptr;
    assert(!f2);
    std::cout << "nullptr构造测试通过" << std::endl;
    
    // 测试函数指针
    mystd::function<int(int, int)> f3 = add;
    assert(f3);
    assert(f3(10, 20) == 30);
    std::cout << "函数指针测试通过" << std::endl;
    
    // 测试函数对象
    Multiplier mult;
    mystd::function<int(int, int)> f4 = mult;
    assert(f4);
    assert(f4(10, 20) == 200);
    std::cout << "函数对象测试通过" << std::endl;
    
    // 测试lambda表达式
    mystd::function<int(int, int)> f5 = [](int a, int b) { return a - b; };
    assert(f5);
    assert(f5(30, 10) == 20);
    std::cout << "Lambda表达式测试通过" << std::endl;
    
    // 测试带捕获的lambda
    int capture = 100;
    mystd::function<int(int)> f6 = [&capture](int x) { return capture + x; };
    assert(f6);
    assert(f6(50) == 150);
    std::cout << "带捕获的Lambda测试通过" << std::endl;
    
    // 测试带状态的函数对象
    Adder add10(10);
    mystd::function<int(int)> f7 = add10;
    assert(f7);
    assert(f7(5) == 15);
    std::cout << "带状态的函数对象测试通过" << std::endl;
}

// 测试拷贝和移动语义
void test_copy_move_semantics() {
    std::cout << "\n=== 测试拷贝和移动语义 ===" << std::endl;
    
    // 测试拷贝构造
    mystd::function<int(int)> f1 = [](int x) { return x * 2; };
    mystd::function<int(int)> f2 = f1;
    assert(f2(5) == 10);
    std::cout << "拷贝构造测试通过" << std::endl;
    
    // 测试拷贝赋值
    mystd::function<int(int)> f3;
    f3 = f1;
    assert(f3(5) == 10);
    std::cout << "拷贝赋值测试通过" << std::endl;
    
    // 测试移动构造
    mystd::function<int(int)> f4 = std::move(f1);
    assert(f4(5) == 10);
    // f1现在应该是空的
    assert(!f1);
    std::cout << "移动构造测试通过" << std::endl;
    
    // 测试移动赋值
    mystd::function<int(int)> f5;
    f5 = std::move(f2);
    assert(f5(5) == 10);
    // f2现在应该是空的
    assert(!f2);
    std::cout << "移动赋值测试通过" << std::endl;
    
    // 测试nullptr赋值
    f5 = nullptr;
    assert(!f5);
    std::cout << "nullptr赋值测试通过" << std::endl;
}

// 测试异常处理
void test_exception_handling() {
    std::cout << "\n=== 测试异常处理 ===" << std::endl;
    
    // 测试调用空function
    mystd::function<int(int)> f;
    bool exception_caught = false;
    try {
        f(10);
    } catch (const std::bad_function_call&) {
        exception_caught = true;
    }
    assert(exception_caught);
    std::cout << "空function调用异常测试通过" << std::endl;
}

// 测试swap功能
void test_swap() {
    std::cout << "\n=== 测试swap功能 ===" << std::endl;
    
    mystd::function<int(int)> f1 = [](int x) { return x * 2; };
    mystd::function<int(int)> f2 = [](int x) { return x * 3; };
    
    // 测试成员swap
    f1.swap(f2);
    assert(f1(5) == 15);
    assert(f2(5) == 10);
    std::cout << "成员swap测试通过" << std::endl;
    
    // 测试非成员swap
    mystd::swap(f1, f2);
    assert(f1(5) == 10);
    assert(f2(5) == 15);
    std::cout << "非成员swap测试通过" << std::endl;
}

// 测试大型可调用对象
void test_large_callables() {
    std::cout << "\n=== 测试大型可调用对象 ===" << std::endl;
    
    // 创建一个大型的函数对象
    struct LargeCallable {
        char buffer[1024]; // 1KB的缓冲区
        int value;
        
        LargeCallable(int v) : value(v) {
            // 初始化buffer以避免未定义行为
            for (int i = 0; i < 1024; ++i) {
                buffer[i] = static_cast<char>(i % 256);
            }
        }
        
        int operator()(int x) const {
            // 使用buffer中的一些值来确保它不被优化掉
            int sum = 0;
            for (int i = 0; i < 10; ++i) {
                sum += static_cast<int>(buffer[i]);
            }
            return value * x + (sum % 10);
        }
    };
    
    LargeCallable large(10);
    mystd::function<int(int)> f = large;
    
    // 计算预期结果
    int expected = large(5);
    int actual = f(5);
    
    assert(actual == expected);
    std::cout << "大型可调用对象测试通过" << std::endl;
}

// 测试类型转换和兼容性
void test_type_compatibility() {
    std::cout << "\n=== 测试类型转换和兼容性 ===" << std::endl;
    
    // 测试隐式转换
    mystd::function<int(int, int)> f1 = [](int a, int b) { return a + b; };
    mystd::function<long(int, int)> f2 = f1; // int可以隐式转换为long
    assert(f2(10, 20) == 30L);
    std::cout << "返回值隐式转换测试通过" << std::endl;
    
    // 测试参数转换
    mystd::function<int(long, long)> f3 = [](int a, int b) { return a + b; };
    assert(f3(10L, 20L) == 30);
    std::cout << "参数隐式转换测试通过" << std::endl;
}

int main() {
    std::cout << "开始测试mystd::function实现...\n" << std::endl;
    
    test_basic_functionality();
    test_copy_move_semantics();
    test_exception_handling();
    test_swap();
    test_large_callables();
    test_type_compatibility();
    
    std::cout << "\n所有测试通过！" << std::endl;
    return 0;
}
