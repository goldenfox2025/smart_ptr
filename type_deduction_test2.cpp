#include <iostream>
#include <type_traits>
#include <typeinfo>

// 辅助模板：移除引用后检查const和volatile
template <typename T>
void check_type() {
    using NonRef = typename std::remove_reference<T>::type;
    std::cout << "  - 移除引用后是const: " << std::is_const<NonRef>::value << "\n";
    std::cout << "  - 移除引用后是volatile: " << std::is_volatile<NonRef>::value << "\n";
    std::cout << "  - 是引用: " << std::is_reference<T>::value << "\n";
    std::cout << "  - 是左值引用: " << std::is_lvalue_reference<T>::value << "\n";
    std::cout << "  - 是右值引用: " << std::is_rvalue_reference<T>::value << "\n";
    std::cout << std::endl;
}

// 测试模板参数推导 - 左值引用版本
template <typename T>
void test_lvalue_ref(T& param) {
    std::cout << "=== 测试左值引用参数 T& ===" << std::endl;
    std::cout << "传入参数类型: ";
    if (std::is_const<std::remove_reference_t<decltype(param)>>::value) std::cout << "const ";
    if (std::is_volatile<std::remove_reference_t<decltype(param)>>::value) std::cout << "volatile ";
    std::cout << "int\n";
    
    std::cout << "推导出的T类型:\n";
    check_type<T>();
    
    std::cout << "参数param的实际类型:\n";
    check_type<decltype(param)>();
}

// 测试auto推导
void test_auto_deduction() {
    std::cout << "=== 测试auto推导 ===" << std::endl;
    
    int x = 42;
    const int cx = x;
    volatile int vx = x;
    const volatile int cvx = x;
    
    // 测试auto& (左值引用)
    std::cout << "--- auto& 左值引用 ---\n";
    
    auto& r1 = x;
    std::cout << "auto& r1 = x (int):\n";
    check_type<decltype(r1)>();
    
    auto& r2 = cx;
    std::cout << "auto& r2 = cx (const int):\n";
    check_type<decltype(r2)>();
    
    auto& r3 = vx;
    std::cout << "auto& r3 = vx (volatile int):\n";
    check_type<decltype(r3)>();
    
    auto& r4 = cvx;
    std::cout << "auto& r4 = cvx (const volatile int):\n";
    check_type<decltype(r4)>();
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
    
    std::cout << "\n\n";
    
    // 测试auto推导规则
    test_auto_deduction();
    
    return 0;
}
