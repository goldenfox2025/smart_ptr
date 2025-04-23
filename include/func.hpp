#ifndef MY_STD_FUNCTION_H
#define MY_STD_FUNCTION_H

#include <functional> // 主要为了 std::bad_function_call 和可能的 std::invoke
#include <type_traits> // 需要大量类型萃取工具
#include <memory>      // 可能需要智能指针管理动态分配的 callable
#include <utility>     // std::move, std::forward, std::swap
#include <exception>   // std::exception
#include <new>         // placement new (如果实现 SBO)
#include <cstddef>     // std::nullptr_t, std::size_t
#include <algorithm>   // std::swap (用于非成员 swap)

namespace mystd {

// --- 异常类 ---
// 直接复用或模仿 std::bad_function_call
using std::bad_function_call; // 为了方便，我们直接用 std 的

// --- 前向声明 ---
template<typename Signature>
class function; // 主模板声明

// --- function 类的特化 ---
// 核心实现：针对函数签名 R(Args...) 的特化版本
template<typename R, typename... Args>
class function<R(Args...)> {
private:
    // --- 内部实现细节 ---

    // 核心思想：类型擦除 (Type Erasure)
    // function 需要能存储任意类型的可调用对象 (函数指针、Lambda、函数对象等)，
    // 只要它们的调用签名与 R(Args...) 兼容即可。
    // 这通常通过一个内部的抽象基类和派生类模板实现，或者通过函数指针表和缓冲区实现。

    // 方式一：基于继承的类型擦除 (常见实现)
    struct function_base {
        virtual ~function_base() = default;
        // 纯虚函数，用于克隆对象 (用于拷贝构造/赋值)
        virtual std::unique_ptr<function_base> clone() const = 0;
        // 纯虚函数，用于实际调用存储的对象
        virtual R invoke(Args... args) const = 0;
        // (可选) 获取 target 的 type_info，用于 target_type()
        // virtual const std::type_info& target_type() const noexcept = 0;
        // (可选) 获取 target 的指针，用于 target()
        // virtual void* target_ptr() noexcept = 0;
        // virtual const void* target_ptr() const noexcept = 0;
    };

    template<typename F>
    struct function_derived : function_base {
        F callable;

        // 构造函数，接收可调用对象
        template<typename Func> // 使用模板以支持完美转发
        explicit function_derived(Func&& f) : callable(std::forward<Func>(f)) {}

        // 实现克隆
        std::unique_ptr<function_base> clone() const override {
            // 创建当前类型的新实例
            // return std::make_unique<function_derived<F>>(callable);
            return nullptr; // TODO: 实现克隆逻辑
        }

        // 实现调用
        R invoke(Args... args) const override {
            // 调用存储的 callable 对象
            // return callable(std::forward<Args>(args)...);
             // 注意：标准库使用 std::invoke 来处理成员函数指针等复杂情况
            // return std::invoke(callable, std::forward<Args>(args)...);
            throw bad_function_call(); // Placeholder
        }

        // (可选) 实现 target_type 和 target
        // const std::type_info& target_type() const noexcept override { return typeid(F); }
        // void* target_ptr() noexcept override { return std::addressof(callable); }
        // const void* target_ptr() const noexcept override { return std::addressof(callable); }
    };

    // 方式二：基于函数指针和存储 (SBO - Small Buffer Optimization)
    // 这种方式更接近某些高性能的 std::function 实现。
    // 它在 function 对象内部预留一小块缓冲区 (storage)。
    // 如果可调用对象足够小，就直接存储在缓冲区中 (placement new)，避免堆分配。
    // 如果对象太大，才在堆上分配。
    // 同时，需要存储一组函数指针 (manager) 来处理对象的拷贝、移动、销毁和调用。

    // union storage_t {
    //     // 用于存储小对象
    //     std::aligned_storage_t<sizeof(void*) * 2, alignof(void*)> buffer;
    //     // 用于存储大对象的指针
    //     void* ptr;
    // };
    // storage_t storage_;

    // using manager_func_t = void(*)(void* /*dest*/, const void* /*src*/, /* operation type */);
    // using invoker_func_t = R(*)(const void* /*storage*/, Args...);

    // manager_func_t manager_ = nullptr; // 指向管理函数 (拷贝/移动/销毁)
    // invoker_func_t invoker_ = nullptr; // 指向调用函数
    // bool uses_heap_ = false; // 标记是否使用了堆内存

    // 当前选择的实现方式：基于继承 (更易于理解和初步实现)
    std::unique_ptr<function_base> base_ptr_ = nullptr;

    // --- SFINAE 辅助模板 (用于构造函数和赋值运算符) ---
    // 检查类型 F 是否可以用来构造 function<R(Args...)>
    template<typename F>
    using enable_if_callable = std::enable_if_t<
        // 1. F 不是 function 类型本身 (防止 function(const function&) 被模板捕获)
        !std::is_same_v<std::decay_t<F>, function> &&
        // 2. F 必须是可调用的，且其结果可以转换/返回为 R
        std::is_invocable_r_v<R, F&, Args...>
        // C++11 可以用 std::is_constructible 和 result_of 等组合模拟
    >;

public:
    // --- 类型别名 (符合标准库风格) ---
    using result_type = R;

    // --- 构造函数 ---

    // 1. 默认构造函数: 创建一个空的 function 对象
    function() noexcept {
        // 默认构造，base_ptr_ 为 nullptr，表示没有目标。
        // SBO 版本：将 manager 和 invoker 设为 nullptr，清空 buffer。
    }

    // 2. 空指针构造函数: 创建一个空的 function 对象
    function(std::nullptr_t) noexcept : function() {
        // 和默认构造函数一样，创建一个空对象。
    }

    // 3. 拷贝构造函数
    function(const function& other) {
        // 如果 other 非空，则克隆 other 内部存储的对象。
        // if (other.base_ptr_) {
        //     base_ptr_ = other.base_ptr_->clone();
        // }
        // SBO 版本：需要调用 manager 函数执行拷贝操作，可能涉及堆分配或缓冲区复制。
        // 需要处理 std::bad_alloc 异常。
    }

    // 4. 移动构造函数
    function(function&& other) noexcept {
        // 从 other 窃取资源 (base_ptr_)，并将 other 置为空状态。
        // base_ptr_ = std::move(other.base_ptr_);
        // other.base_ptr_ = nullptr; // 确保 other 被置空
        // SBO 版本：移动指针或缓冲区内容，并将 other 的 manager/invoker 置空。通常是 noexcept。
    }

    // 5. 模板构造函数: 从任意可调用对象 f 构造
    template<typename F, typename = enable_if_callable<F>>
    function(F f) {
        // 这是核心的类型擦除构造函数。
        // 1. 对 F 进行类型衰变 (decay) 得到实际存储的类型 CleanF。
        // using CleanF = std::decay_t<F>;
        // 2. 创建一个 function_derived<CleanF> 的实例来包装 f。
        //    使用 std::forward<F>(f) 完美转发 f。
        // base_ptr_ = std::make_unique<function_derived<CleanF>>(std::forward<F>(f));
        // SBO 版本：
        //    - 判断 sizeof(CleanF) 是否适合放入 buffer。
        //    - 如果适合：使用 placement new 在 storage_.buffer 中构造 CleanF，设置相应的 manager 和 invoker 函数指针。
        //    - 如果不适合：在堆上 new 一个 CleanF，将指针存入 storage_.ptr，设置相应的 manager 和 invoker 函数指针，标记 uses_heap_ = true。
        // 需要处理 std::bad_alloc 异常。
    }

    // --- 析构函数 ---
    ~function() {
        // unique_ptr 会自动管理 function_base 的生命周期。
        // SBO 版本：如果使用了堆内存 (uses_heap_ = true)，需要调用 manager 函数销毁堆对象并释放内存。如果对象在 buffer 中，需要调用 manager 销毁它 (调用析构函数)。
    }

    // --- 赋值运算符 ---

    // 1. 拷贝赋值运算符
    function& operator=(const function& other) {
        // 实现标准的 copy-and-swap idiom 或者直接拷贝。
        // if (this != &other) {
        //     if (other.base_ptr_) {
        //         base_ptr_ = other.base_ptr_->clone(); // 克隆对方内容
        //     } else {
        //         base_ptr_.reset(); // 对方为空，则置空自己
        //     }
        // }
        // return *this;
        // 更好的方式是 copy-and-swap:
        // function(other).swap(*this);
        // return *this;
        // 需要处理异常安全 (strong guarantee via copy-and-swap)。
        return *this; // Placeholder
    }

    // 2. 移动赋值运算符
    function& operator=(function&& other) noexcept {
        // 实现标准的 move-and-swap 或者直接移动。
        // if (this != &other) {
        //     base_ptr_ = std::move(other.base_ptr_); // 窃取资源
        //     other.base_ptr_ = nullptr;            // 将对方置空
        // }
        // return *this;
        // 或者使用 swap:
        // function(std::move(other)).swap(*this); // 移动构造一个临时的，然后交换
        // return *this;
        // 通常是 noexcept。
        return *this; // Placeholder
    }

    // 3. 空指针赋值运算符
    function& operator=(std::nullptr_t) noexcept {
        // 将 function 置为空状态。
        // base_ptr_.reset();
        // SBO 版本：调用 manager 清理可能存在的对象，并将 manager/invoker 置空。
        return *this;
    }

    // 4. 模板赋值运算符: 从任意可调用对象 f 赋值
    template<typename F, typename = enable_if_callable<F>>
    function& operator=(F&& f) {
        // 先创建一个临时的 function 对象包含 f，然后与 *this 交换。
        // 这样可以利用构造函数的逻辑，并保证强异常安全。
        // function(std::forward<F>(f)).swap(*this);
        // return *this;
        return *this; // Placeholder
    }

    // --- 修饰符 (Modifiers) ---

    // 交换两个 function 对象的状态
    void swap(function& other) noexcept {
        // 交换内部指针 (或 SBO 状态)。
        // std::swap(base_ptr_, other.base_ptr_);
        // SBO 版本：需要交换 storage_, manager_, invoker_, uses_heap_ 等所有状态。
    }

    // --- 容量 (Capacity) ---

    // 检查 function 是否包含一个可调用目标
    explicit operator bool() const noexcept {
        // 如果内部指针非空 (或 SBO 状态表示有目标)，则返回 true。
        // return static_cast<bool>(base_ptr_);
        // SBO 版本：return invoker_ != nullptr;
        return false; // Placeholder
    }

    // --- 调用 (Invocation) ---

    // 调用存储的目标
    R operator()(Args... args) const {
        // 检查是否为空。如果为空，则抛出 bad_function_call 异常。
        // if (!base_ptr_) {
        //     throw bad_function_call();
        // }
        // // 通过基类指针调用 invoke 虚函数
        // return base_ptr_->invoke(std::forward<Args>(args)...);

        // SBO 版本：
        // if (!invoker_) {
        //     throw bad_function_call();
        // }
        // // 调用 invoker 函数指针，传递存储区域和参数
        // return invoker_(storage_ptr(), std::forward<Args>(args)...); // storage_ptr() 需要根据 uses_heap_ 返回 buffer 地址或 ptr
        throw bad_function_call(); // Placeholder
    }

    // --- 目标访问 (Target Access) (可选，实现较复杂) ---

    // 获取存储目标的 type_info
    // const std::type_info& target_type() const noexcept {
        // 如果为空，返回 typeid(void)。
        // 如果非空，通过 base_ptr_ (或 manager) 获取存储对象的 type_info。
        // return base_ptr_ ? base_ptr_->target_type() : typeid(void);
    // }

    // 获取存储目标的指针 (如果类型匹配)
    // template<typename T>
    // T* target() noexcept {
        // 检查 target_type() 是否匹配 T 的 type_info。
        // 如果匹配，返回指向存储对象的指针 (通过 base_ptr_->target_ptr() 或 SBO 的 manager)。
        // 否则返回 nullptr。
        // const T* const_this = static_cast<const function*>(this)->target<T>();
        // return const_cast<T*>(const_this);
    // }

    // template<typename T>
    // const T* target() const noexcept {
        // 检查 target_type() 是否匹配 T 的 type_info。
        // 如果匹配，返回指向存储对象的 const 指针。
        // 否则返回 nullptr。
        // if (target_type() == typeid(T)) {
        //    // 从 base_ptr_ 或 SBO 存储中获取指针
        //    // return static_cast<const T*>(base_ptr_->target_ptr()); // 假设有 target_ptr
        //    return nullptr; // Placeholder
        // }
        // return nullptr;
    // }

}; // class function<R(Args...)>

// --- 非成员函数 ---

// 1. 非成员 swap 函数
template<typename R, typename... Args>
void swap(function<R(Args...)>& lhs, function<R(Args...)>& rhs) noexcept {
    lhs.swap(rhs);
}

// 2. 空函数比较运算符 (==, !=)
template<typename R, typename... Args>
bool operator==(const function<R(Args...)>& f, std::nullptr_t) noexcept {
    return !f; // 如果 function 对象为 false (即空)，则等于 nullptr
}

template<typename R, typename... Args>
bool operator==(std::nullptr_t, const function<R(Args...)>& f) noexcept {
    return !f;
}

template<typename R, typename... Args>
bool operator!=(const function<R(Args...)>& f, std::nullptr_t) noexcept {
    return static_cast<bool>(f); // 如果 function 对象为 true (即非空)，则不等于 nullptr
}

template<typename R, typename... Args>
bool operator!=(std::nullptr_t, const function<R(Args...)>& f) noexcept {
    return static_cast<bool>(f);
}

// 注意：std::function 没有定义 function == function 或 function != function

} // namespace mystd

#endif // MY_STD_FUNCTION_H