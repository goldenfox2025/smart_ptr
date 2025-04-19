#pragma once
#include <atomic>
#include <functional>
#include <iostream>
using namespace std;

struct control_block_base {
 public:
  std::atomic<int> ref_cnt;
  std::atomic<int> weak_cnt;
  control_block_base(int r, int w) : ref_cnt(r), weak_cnt(w) {}
  virtual void delete_ptr() = 0;
  virtual ~control_block_base() {}
};

template <typename T>
class control_block : public control_block_base {
 public:
  function<void(T*)> deleter;
  T* ptr;
  void delete_ptr() { deleter(ptr); }
  control_block() {}
  control_block(T* p)
      : ptr(p),
        deleter([](T* p) -> void { delete p; }),
        control_block_base(1, 0) {}
  control_block(T* p, function<void(T*)> d)
      : ptr(p), deleter(d), control_block_base(1, 0) {}
  control_block(const control_block& other) = delete;
  ~control_block() {}
};

template <typename T>
class WeakPtr;

template <typename T>
class SharedPtr {
 private:
  template <typename Y>
  friend class SharedPtr;
  template <typename X>
  friend class WeakPtr;
  T* ptr;
  control_block_base* ctrl;
  SharedPtr(T* p, control_block_base* c) : ptr(p), ctrl(c) {}

 public:
  SharedPtr() noexcept : ptr(nullptr), ctrl(nullptr) {}
  SharedPtr(T* p) : ptr(p), ctrl(new control_block<T>(p)) {}
  SharedPtr(T* p, function<void(T*)> d)
      : ptr(p), ctrl(new control_block<T>(p, d)) {}
  SharedPtr(const SharedPtr& other) {
    if (other.ctrl) {
      other.ctrl->ref_cnt.fetch_add(1, std::memory_order_release);
      ctrl = other.ctrl;
      ptr = other.ptr;
    }
  }
  SharedPtr(SharedPtr&& other) noexcept {
    ctrl = other.ctrl;
    ptr = other.ptr;
    other.ctrl = nullptr;
    other.ptr = nullptr;
  }
  T* get() { return ptr; }
  explicit operator bool() { return ptr != nullptr; }
  SharedPtr& operator=(const SharedPtr& other) {
    if (this == &other) {
      return *this;
    }
    SharedPtr temp(other);
    swap(temp);
    return *this;
  }
  SharedPtr& operator=(SharedPtr&& other) noexcept {
    if (this == &other) {
      return *this;
    }
    release();
    ctrl = other.ctrl;
    ptr = other.ptr;
    other.ctrl = nullptr;
    other.ptr = nullptr;
  }
  T& operator*() { return *ptr; }
  T* operator->() { return ptr; }
  void release() {
    if (!ctrl) return;  // 由于可能对空指针赋值 必须当心
    if (ctrl->ref_cnt.fetch_sub(1, std::memory_order_acq_rel) == 1) {
      ctrl->delete_ptr();
      if (ctrl->weak_cnt.load(std::memory_order_acquire) == 0) {
        delete ctrl;
      }
    };
  }
  ~SharedPtr() { release(); }
  void swap(SharedPtr& other) noexcept {
    control_block_base* temp_ctrl = other.ctrl;
    other.ctrl = ctrl;
    ctrl = temp_ctrl;
    T* temp_p = other.ptr;
    other.ptr = ptr;
    ptr = temp_p;
  }
  template <typename U>
  SharedPtr(SharedPtr<U>& other) : ctrl(nullptr), ptr(nullptr) {
    static_assert(std::is_convertible<U*, T*>::value,
                  "U* must be convertible to T*");
    if (other.ctrl) {
      other.ctrl->ref_cnt.fetch_add(1, std::memory_order_release);
      ptr = static_cast<T*>(other.ptr);
      ctrl = other.ctrl;
    }
  }
  template <typename U>
  SharedPtr& operator=(SharedPtr<U>& other) {
    static_assert(std::is_convertible<U*, T*>::value,
                  "U* must be convertible to T*");

    SharedPtr<T> temp(other);
    swap(temp);
    return *this;
  }
  template <typename U>
  SharedPtr(SharedPtr<U>&& other) noexcept {
    static_assert(std::is_convertible<U*, T*>::value,
                  "U* must be convertible to T*");
    ptr = static_cast<T*>(other.ptr);
    ctrl = other.ctrl;
    other.ctrl = nullptr;
    other.ptr = nullptr;
  }
  template <typename U>
  SharedPtr& operator=(SharedPtr<U>&& other) noexcept {
    static_assert(std::is_convertible<U*, T*>::value,
                  "U* must be convertible to T*");
    release();
    ptr = static_cast<T*>(other.ptr);
    ctrl = other.ctrl;
    other.ctrl = nullptr;
    other.ptr = nullptr;
  }
  SharedPtr& operator=(std::nullptr_t) noexcept {
    release();
    ptr = nullptr;
    ctrl = nullptr;
    return *this;
  }
};
template <typename T>
class WeakPtr {
 private:
  template <typename U>
  friend class SharedPtr;
  control_block_base* ctrl;
  template <typename U>
  friend class WeakPtr;
  T* ptr;

 public:
  WeakPtr() : ctrl(nullptr), ptr(nullptr) {};
  // 只允许从共享指针构造
  WeakPtr(const SharedPtr<T>& sp) {
    if (sp.ctrl) {
      sp.ctrl->weak_cnt.fetch_add(1, std::memory_order_release);
      ctrl = sp.ctrl;
      ptr = sp.ptr;
    }
  }
  WeakPtr(const WeakPtr& other) : ctrl(nullptr), ptr(nullptr) {
    if (other.ctrl) {
      other.ctrl->weak_cnt.fetch_add(1, std::memory_order_release);
      ctrl = other.ctrl;
      ptr = other.ptr;
    }
  }
  WeakPtr(WeakPtr&& other) : ctrl(nullptr), ptr(nullptr) {
    if (other.ctrl) {
      ctrl = other.ctrl;
      other.ctrl = nullptr;
      ptr = other.ptr;
      other.ptr = nullptr;
    }
  }
  template <typename U>
  WeakPtr(WeakPtr<U>& other) : ctrl(nullptr), ptr(nullptr) {
    if (other.ctrl) {
      ctrl = other.ctrl;
      ctrl->weak_cnt.fetch_add(1, std::memory_order_release);
      static_assert(std::is_convertible<U*, T*>::value,
                    "U* must be convertible to T*");
      ptr = static_cast<T*>(other.ptr);
    }
  }
  template <typename U>
  WeakPtr& operator=(WeakPtr<U>& other) {
    if (this == &other) {
      return *this;
    }
    WeakPtr temp(other);
    swap(temp);
    return *this;
  }

  WeakPtr& operator=(SharedPtr<T>& other) {
    WeakPtr temp(other);
    swap(temp);
    return *this;
  }

  void swap(WeakPtr& other) {
    std::swap(ptr, other.ptr);
    std::swap(ctrl, other.ctrl);
  }
  template <typename U>
  WeakPtr(WeakPtr<U>&& other) : ctrl(nullptr), ptr(nullptr) {
    if (other.ctrl) {
      static_assert(std::is_convertible<U*, T*>::value,
                    "U* must be convertible to T*");
      ptr = static_cast<T*>(other.ptr);
      ctrl = other.ctrl;
      other.ptr = nullptr;
      other.ctrl = nullptr;
    }
  }
  template <typename U>
  WeakPtr& operator=(WeakPtr<U>&& other) {
    if (this != &other) {
      release();
      ptr = static_cast<T*>(other.ptr);
      ctrl = other.ctrl;
      other.ctrl = nullptr;
      other.ptr = nullptr;
    }
    return *this;
  }
  SharedPtr<T> lock() noexcept {
    control_block_base* observed_ctrl = ctrl;
    if (!observed_ctrl) {
      return SharedPtr<T>();
    }
    int expected_count = observed_ctrl->ref_cnt.load(std::memory_order_acquire);
    while (true) {
      if (expected_count == 0) {
        return SharedPtr<T>();
      }
      if (observed_ctrl->ref_cnt.compare_exchange_weak(
              expected_count, expected_count + 1, std::memory_order_release,
              std::memory_order_relaxed)) {
        return SharedPtr<T>(ptr, observed_ctrl);
      }
    }
  }
  void release() {
    if (!ctrl) {
      return;
    }
    if (ctrl->weak_cnt.fetch_sub(1, std::memory_order_acq_rel) == 1) {
      if (ctrl->ref_cnt.load() == 0) {
        delete ctrl;
      }
    }
  }
  WeakPtr& operator=(std::nullptr_t) noexcept {
    release();
    ptr = nullptr;
    ctrl = nullptr;
    return *this;
  }
  explicit operator bool() { return ptr != nullptr; }
  ~WeakPtr() { release(); }
};