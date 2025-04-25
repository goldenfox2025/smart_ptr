#ifndef MY_STD_FUNCTION_H
#define MY_STD_FUNCTION_H

#include <algorithm> // std::swap (用于非成员 swap)
#include <cstddef>   // std::nullptr_t, std::size_t
#include <exception> // std::exception
#include <functional> // std::bad_function_call
#include <memory>      // 管理动态分配的 callable
#include <new>         // placement new (SBO)
#include <type_traits> // 需要大量类型萃取工具
#include <utility>     // std::move, std::forward, std::swap

namespace mystd {


using std::bad_function_call; 


template <typename Signature> class function;


// 核心实现：针对函数签名 R(Args...) 的特化版本
template <typename R, typename... Args> class function<R(Args...)> {
private:


  // 核心思想：类型擦除 (Type Erasure)
  // function 需要能存储任意类型的可调用对象 (函数指针、Lambda、函数对象等)，
  // 只要它们的调用签名与 R(Args...) 兼容即可。
  // 这通常通过一个内部的抽象基类和派生类模板实现，或者通过函数指针表和缓冲区实现。


  struct function_base {
    virtual ~function_base() = default;
    virtual std::unique_ptr<function_base> clone() const = 0;
    virtual R invoke(Args... args) const = 0;
  };
  // F是被擦除的类型
  template <typename F> struct function_derived : function_base {
    F callable;
    template <typename Func> 
    explicit function_derived(Func &&f) : callable(std::forward<Func>(f)) {}
    std::unique_ptr<function_base> clone() const override {
        return std::make_unique<function_derived<F>>(callable);
    }

    // 实现调用
    R invoke(Args... args) const override {
      return std::invoke(callable, std::forward<Args>(args)...);
    }
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

  // using manager_func_t = void(*)(void* /*dest*/, const void* /*src*/, /*
  // operation type */); using invoker_func_t = R(*)(const void* /*storage*/,
  // Args...);

  // manager_func_t manager_ = nullptr; // 指向管理函数 (拷贝/移动/销毁)
  // invoker_func_t invoker_ = nullptr; // 指向调用函数
  // bool uses_heap_ = false; // 标记是否使用了堆内存


  std::unique_ptr<function_base> base_ptr_;

  // --- (用于构造函数和赋值运算符) ---
  // 检查类型 F 是否可以用来构造 function<R(Args...)>
  template <typename F>
  using enable_if_callable = std::enable_if_t<
      // 1. F 不是 function 类型本身 (防止 function(const function&) 被模板捕获)
      !std::is_same_v<std::decay_t<F>, function> &&
      // 2. F 必须是可调用的，且其结果可以转换/返回为 R
      std::is_invocable_r_v<R, F &, Args...>
      >;

public:
  using result_type = R;
  // --- 构造函数 ---
  // 1. 默认构造函数: 创建一个空的 function 对象
  function() noexcept : base_ptr_(nullptr) {
  }
  // 2. 空指针构造函数: 创建一个空的 function 对象
  function(std::nullptr_t) noexcept : function() {
  }
  // 3. 拷贝构造函数：如果 other 非空，则克隆 other 内部存储的对象。
  function(const function &other) {
    if (other.base_ptr_) {
      base_ptr_ = other.base_ptr_->clone();
    }
  }

  // 4. 移动构造函数
  function(function &&other) noexcept {
    base_ptr_ = std::move(other.base_ptr_);
    other.base_ptr_ = nullptr; 
  }


  // 5. 模板构造函数: 从任意可调用对象 f 构造
  template <typename F, typename = enable_if_callable<F>> function(F f) {
    // 这是核心的类型擦除构造函数。
    // 1. 对 F 进行类型衰变 (decay) 得到实际存储的类型 CleanF。
    using CleanF = std::decay_t<F>;
    // 2. 创建一个 function_derived<CleanF> 的实例来包装 f。
    //    使用 std::forward<F>(f) 完美转发 f。
    base_ptr_ = std::make_unique<function_derived<CleanF>>(std::forward<F>(f));
  }

  // --- 析构函数 ---
  ~function() {
    // unique_ptr 会自动管理 function_base 的生命周期。
  }

  // --- 赋值运算符 ---

  // 1. 拷贝赋值运算符
  function &operator=(const function &other) {
    function(other).swap(*this);
    return *this; // Placeholder
  }

  // 2. 移动赋值运算符
  function &operator=(function &&other) noexcept {
    // 实现标准的 move-and-swap 或者直接移动。
    // if (this != &other) {
    //     base_ptr_ = std::move(other.base_ptr_); // 窃取资源
    //     other.base_ptr_ = nullptr;            // 将对方置空
    // }
    // return *this;
    // 或者使用 swap:
    function(std::move(other)).swap(*this); // 移动构造一个临时的，然后交换
    // return *this;
    // 通常是 noexcept。
    return *this; // Placeholder
  }

  // 3. 空指针赋值运算符
  function &operator=(std::nullptr_t) noexcept {
    // 将 function 置为空状态。
    base_ptr_.reset();
    // SBO 版本：调用 manager 清理可能存在的对象，并将 manager/invoker 置空。
    return *this;
  }

  // 4. 模板赋值运算符: 从任意可调用对象 f 赋值
  template <typename F, typename = enable_if_callable<F>>
  function &operator=(F &&f) {
    // 先创建一个临时的 function 对象包含 f，然后与 *this 交换。
    // 这样可以利用构造函数的逻辑，并保证强异常安全。
    function(std::forward<F>(f)).swap(*this);
    // return *this;
    return *this; // Placeholder
  }

  // --- 修饰符 (Modifiers) ---

  // 交换两个 function 对象的状态
  void swap(function &other) noexcept {
    std::swap(base_ptr_, other.base_ptr_);
  }

  // --- 容量 (Capacity) ---
  explicit operator bool() const noexcept {
    return static_cast<bool>(base_ptr_);
  }

  // --- 调用 (Invocation) ---
  R operator()(Args... args) const {
    if (!base_ptr_) {
        throw bad_function_call();
    }
    return base_ptr_->invoke(std::forward<Args>(args)...);
  }
}; // class function<R(Args...)>

// --- 非成员函数 ---

// 1. 非成员 swap 函数
template <typename R, typename... Args>
void swap(function<R(Args...)> &lhs, function<R(Args...)> &rhs) noexcept {
  lhs.swap(rhs);
}


} // namespace mystd

#endif // MY_STD_FUNCTION_H