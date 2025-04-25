#include <iostream>
#include <type_traits>
#include <typeinfo>

// 辅助函数：打印类型信息
template <typename T>
void print_type_info(const std::string& name) {
    std::cout << name << ":\n";
    std::cout << "  - is const: " << std::is_const<T>::value << "\n";
    std::cout << "  - is volatile: " << std::is_volatile<T>::value << "\n";
    std::cout << "  - is reference: " << std::is_reference<T>::value << "\n";
    std::cout << "  - is lvalue reference: " << std::is_lvalue_reference<T>::value << "\n";
    std::cout << "  - is rvalue reference: " << std::is_rvalue_reference<T>::value << "\n";
    std::cout << std::endl;
}

// 测试模板参数推导 - 左值引用版本
template <typename T>
void test_lvalue_ref(T& param) {
    std::cout << "=== 测试左值引用参数 T& ===" << std::endl;
    print_type_info<T>("参数类型T");
    print_type_info<decltype(param)>("参数实际类型");
}

// 测试模板参数推导 - 转发引用版本
template <typename T>
void test_forwarding_ref(T&& param) {
    std::cout << "=== 测试转发引用参数 T&& ===" << std::endl;
    print_type_info<T>("参数类型T");
    print_type_info<decltype(param)>("参数实际类型");
}

// 测试模板参数推导 - 值传递版本
template <typename T>
void test_by_value(T param) {
    std::cout << "=== 测试值传递参数 T ===" << std::endl;
    print_type_info<T>("参数类型T");
    print_type_info<decltype(param)>("参数实际类型");
}

// 测试auto推导
void test_auto_deduction() {
    std::cout << "=== 测试auto推导 ===" << std::endl;
    
    int x = 42;
    const int cx = x;
    volatile int vx = x;
    const volatile int cvx = x;
    
    // 测试auto (值)
    auto a1 = x;
    auto a2 = cx;
    auto a3 = vx;
    auto a4 = cvx;
    
    print_type_info<decltype(a1)>("auto a1 = x");
    print_type_info<decltype(a2)>("auto a2 = cx");
    print_type_info<decltype(a3)>("auto a3 = vx");
    print_type_info<decltype(a4)>("auto a4 = cvx");
    
    // 测试auto& (左值引用)
    auto& r1 = x;
    auto& r2 = cx;
    auto& r3 = vx;
    auto& r4 = cvx;
    
    print_type_info<decltype(r1)>("auto& r1 = x");
    print_type_info<decltype(r2)>("auto& r2 = cx");
    print_type_info<decltype(r3)>("auto& r3 = vx");
    print_type_info<decltype(r4)>("auto& r4 = cvx");
    
    // 测试auto&& (转发引用)
    auto&& f1 = x;
    auto&& f2 = cx;
    auto&& f3 = vx;
    auto&& f4 = cvx;
    auto&& f5 = 42; // 右值
    
    print_type_info<decltype(f1)>("auto&& f1 = x");
    print_type_info<decltype(f2)>("auto&& f2 = cx");
    print_type_info<decltype(f3)>("auto&& f3 = vx");
    print_type_info<decltype(f4)>("auto&& f4 = cvx");
    print_type_info<decltype(f5)>("auto&& f5 = 42");
}

int main() {
    int x = 42;
    const int cx = x;
    volatile int vx = x;
    const volatile int cvx = x;
    
    std::cout << "测试模板参数推导规则\n" << std::endl;
    
    // 测试左值引用参数
    test_lvalue_ref(x);
    test_lvalue_ref(cx);
    test_lvalue_ref(vx);
    test_lvalue_ref(cvx);
    
    // 测试转发引用参数
    test_forwarding_ref(x);
    test_forwarding_ref(cx);
    test_forwarding_ref(vx);
    test_forwarding_ref(cvx);
    test_forwarding_ref(42); // 右值
    
    // 测试值传递参数
    test_by_value(x);
    test_by_value(cx);
    test_by_value(vx);
    test_by_value(cvx);
    
    std::cout << "\n\n";
    
    // 测试auto推导规则
    test_auto_deduction();
    
    return 0;
}
