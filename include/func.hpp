#ifndef MY_STD_FUNCTION_SBO_H
#define MY_STD_FUNCTION_SBO_H

#include <cstddef>
#include <utility>
#include <memory>
#include <functional> // std::bad_function_call
#include <type_traits>

namespace mystd {

// =======================================
// 一个带 SBO 的简化版 function 实现
// 我们将分多步引导：
// 1. 定义存储缓冲与函数指针类型
// 2. 实现类型擦除的管理函数
// 3. 构造、拷贝、移动、析构逻辑
// 4. 调用 operator()
// =======================================

using bad_function_call = std::bad_function_call;

template <typename> class function;

// ===== 特化 R(Args...) =====
template <typename R, typename... Args>
class function<R(Args...)> {
private:
    // ================= Step 1: 定义内部缓冲区 ================
    // 为什么要预留一段固定大小的缓冲区？
    // 如果可调用对象足够小，则直接放在缓冲区中，零堆分配。
    // 否则再用 heap。这样避免了大量小对象的频繁 new/delete。
    static constexpr std::size_t SBO_BUFFER_SIZE  = 128; // 增加缓冲区大小以容纳嵌套function对象
    static constexpr std::size_t SBO_BUFFER_ALIGN = alignof(std::max_align_t);

    // 原始存储区
    alignas(SBO_BUFFER_ALIGN) unsigned char buffer_[SBO_BUFFER_SIZE];
    // 如果对象太大，会在堆上分配，存储指针
    void* heap_ptr_ = nullptr;
    bool uses_heap_ = false;

    // ================ Step 2: 定义函数指针 ================
    // copy: 负责从 src 拷贝到 dst
    using copy_fn_t    = void(*)(const function&, function&);
    // move: 负责从 src 移动到 dst
    using move_fn_t    = void(*)(function&, function&);
    // destroy: 负责析构
    using destroy_fn_t = void(*)(function&);
    // invoke: 负责实际调用
    using invoke_fn_t  = R(*)(const function&, Args&&...);

    copy_fn_t    copy_fn_    = nullptr;
    move_fn_t    move_fn_    = nullptr;
    destroy_fn_t destroy_fn_ = nullptr;
    invoke_fn_t  invoke_fn_  = nullptr;

    // ================ Step 2.1: Helper to获取指针 ================
    template <typename F>
    static F* get_ptr(function const& self) {
        return self.uses_heap_
            ? reinterpret_cast<F*>(self.heap_ptr_)
            : reinterpret_cast<F*>(const_cast<unsigned char*>(self.buffer_));
    }

    // ================ Step 2.2: 不同 F 的管理函数 ================
    template <typename F>
    static void copy_impl(const function& src, function& dst) {
        // 检查大小和对齐要求
        constexpr bool can_use_sbo = sizeof(F) <= SBO_BUFFER_SIZE &&
                                     alignof(F) <= SBO_BUFFER_ALIGN;

        // 使用if constexpr来让编译器在编译时选择正确的分支
        if constexpr (!can_use_sbo) {
            // 对象太大，必须使用堆
            dst.heap_ptr_ = new F(*get_ptr<F>(src));
            dst.uses_heap_ = true;
        } else {
            // 对象可以放在SBO缓冲区，但如果源对象在堆上，我们也应该使用堆
            if (src.uses_heap_) {
                dst.heap_ptr_ = new F(*get_ptr<F>(src));
                dst.uses_heap_ = true;
            } else {
                new (dst.buffer_) F(*get_ptr<F>(src));
                dst.uses_heap_ = false;
            }
        }
        // 复制函数指针
        dst.copy_fn_    = src.copy_fn_;
        dst.move_fn_    = src.move_fn_;
        dst.destroy_fn_ = src.destroy_fn_;
        dst.invoke_fn_  = src.invoke_fn_;
    }

    template <typename F>
    static void move_impl(function& src, function& dst) noexcept {
        // 检查大小和对齐要求
        constexpr bool can_use_sbo = sizeof(F) <= SBO_BUFFER_SIZE &&
                                     alignof(F) <= SBO_BUFFER_ALIGN;

        // 使用if constexpr来让编译器在编译时选择正确的分支
        if constexpr (!can_use_sbo) {
            // 对象太大，必须使用堆
            if (src.uses_heap_) {
                dst.heap_ptr_ = src.heap_ptr_;
            } else {
                dst.heap_ptr_ = new F(std::move(*get_ptr<F>(src)));
            }
            dst.uses_heap_ = true;
        } else {
            // 对象可以放在SBO缓冲区，但如果源对象在堆上，我们也应该使用堆
            if (src.uses_heap_) {
                dst.heap_ptr_ = src.heap_ptr_;
                dst.uses_heap_ = true;
            } else {
                new (dst.buffer_) F(std::move(*get_ptr<F>(src)));
                dst.uses_heap_ = false;
            }
        }

        // 复制函数指针
        dst.copy_fn_    = src.copy_fn_;
        dst.move_fn_    = src.move_fn_;
        dst.destroy_fn_ = src.destroy_fn_;
        dst.invoke_fn_  = src.invoke_fn_;

        // 清理源对象
        if (src.uses_heap_) {
            // 如果目标也使用了源对象的堆指针，则不要删除源对象的内存
            if (dst.heap_ptr_ == src.heap_ptr_) {
                src.heap_ptr_ = nullptr;
            } else {
                delete get_ptr<F>(src);
            }
        } else {
            get_ptr<F>(src)->~F();
        }

        src.uses_heap_ = false;
        src.heap_ptr_ = nullptr;
        // 分别设置为nullptr，避免类型转换错误
        src.copy_fn_    = nullptr;
        src.move_fn_    = nullptr;
        src.destroy_fn_ = nullptr;
        src.invoke_fn_  = nullptr;
    }

    template <typename F>
    static void destroy_impl(function& self) noexcept {
        if (self.uses_heap_) {
            delete reinterpret_cast<F*>(self.heap_ptr_);
        } else {
            get_ptr<F>(self)->~F();
        }
    }

    template <typename F>
    static R invoke_impl(const function& self, Args&&... args) {
        return std::invoke(*get_ptr<F>(self), std::forward<Args>(args)...);
    }

public:
    using result_type = R;

    // ===== 构造与析构 =====
    function() noexcept = default;
    function(std::nullptr_t) noexcept {}

    ~function() {
        // Step 3: 调用析构函数
        if (destroy_fn_) destroy_fn_(*this);
    }

    // 拷贝构造
    function(const function& other) {
        if (other.copy_fn_) other.copy_fn_(other, *this);
    }
    // 移动构造
    function(function&& other) noexcept {
        if (other.move_fn_) other.move_fn_(other, *this);
    }

    // ===== 从可调用对象构造 =====
    template <typename F,
              typename = std::enable_if_t<
                  !std::is_same_v<std::decay_t<F>, function> &&
                  std::is_invocable_r_v<R, F, Args...>>>
    function(F f) {
        using CleanF = std::decay_t<F>;
        // Step 4: 放置对象
        // 检查大小和对齐要求
        constexpr bool can_use_sbo = sizeof(CleanF) <= SBO_BUFFER_SIZE &&
                                     alignof(CleanF) <= SBO_BUFFER_ALIGN;

        if constexpr (can_use_sbo) {
            // 在缓冲区上
            new (buffer_) CleanF(std::forward<F>(f));
            uses_heap_ = false;
        } else {
            // 在堆上
            heap_ptr_ = new CleanF(std::forward<F>(f));
            uses_heap_ = true;
        }
        // 设置管理与调用函数指针
        copy_fn_    = &copy_impl<CleanF>;
        move_fn_    = &move_impl<CleanF>;
        destroy_fn_ = &destroy_impl<CleanF>;
        invoke_fn_  = &invoke_impl<CleanF>;
    }

    // ===== 赋值操作 =====
    function& operator=(const function& other) {
        if (this != &other) {
            function tmp(other);
            swap(tmp);
        }
        return *this;
    }
    function& operator=(function&& other) noexcept {
        if (this != &other) {
            function tmp(std::move(other));
            swap(tmp);
        }
        return *this;
    }
    function& operator=(std::nullptr_t) noexcept {
        if (destroy_fn_) destroy_fn_(*this);
        // 分别设置为nullptr，避免类型转换错误
        copy_fn_ = nullptr;
        move_fn_ = nullptr;
        destroy_fn_ = nullptr;
        invoke_fn_ = nullptr;
        uses_heap_ = false;
        heap_ptr_ = nullptr;
        return *this;
    }

    template <typename F,
              typename = std::enable_if_t<
                  !std::is_same_v<std::decay_t<F>, function> &&
                  std::is_invocable_r_v<R, F, Args...>>>
    function& operator=(F&& f) {
        function tmp(std::forward<F>(f));
        swap(tmp);
        return *this;
    }

    // ===== swap =====
    void swap(function& other) noexcept {
        std::swap(buffer_,   other.buffer_);
        std::swap(heap_ptr_, other.heap_ptr_);
        std::swap(uses_heap_, other.uses_heap_);
        std::swap(copy_fn_,    other.copy_fn_);
        std::swap(move_fn_,    other.move_fn_);
        std::swap(destroy_fn_, other.destroy_fn_);
        std::swap(invoke_fn_,  other.invoke_fn_);
    }

    // ===== 容量与调用 =====
    explicit operator bool() const noexcept {
        return invoke_fn_ != nullptr;
    }

    R operator()(Args... args) const {
        if (!invoke_fn_) throw bad_function_call();
        return invoke_fn_(*this, std::forward<Args>(args)...);
    }
}; // class function<R(Args...)>

// 非成员 swap

template <typename R, typename... Args>
void swap(function<R(Args...)>& lhs, function<R(Args...)>& rhs) noexcept {
    lhs.swap(rhs);
}

} // namespace mystd

#endif // MY_STD_FUNCTION_SBO_H

// =======================================
// 以上是完整代码。接下来，请先思考：
// "为何要预留一段固定大小的缓冲区？"
// 然后我们将深入探讨 SBO 如何减少堆分配次数及其性能意义。
