#include <cassert>
#include <iostream>
#include <memory>
#include <utility>

template <typename T = int, typename Deleter = std::default_delete<T>>
class unique_ptr {
 private:
  T* source;
  Deleter deleter;

 public:
  using pointer = T*;
  using element_type = T;
  using deleter_type = Deleter;


  unique_ptr() noexcept : source(nullptr), deleter() {}
  explicit unique_ptr(T* p) : source(p), deleter() {}
  unique_ptr(T* p, const Deleter& d) : source(p), deleter(d) {}
  unique_ptr(T* p, Deleter&& d) : source(p), deleter(std::move(d)) {}
  unique_ptr(unique_ptr&& other) noexcept
      : source(other.source), deleter(std::move(other.deleter)) {
    other.source = nullptr;
  }

  unique_ptr(const unique_ptr& other) = delete;
  unique_ptr& operator=(const unique_ptr& other) = delete;

  ~unique_ptr() { reset(); }

  unique_ptr& operator=(unique_ptr&& other) noexcept {
    if (this != &other) {
      reset();
      source = other.source;
      deleter = std::move(other.deleter);
      other.source = nullptr;
    }
    return *this;
  }

  unique_ptr& operator=(std::nullptr_t) noexcept {
    reset();
    return *this;
  }

  T* release() noexcept {
    T* ptr = source;
    source = nullptr;
    return ptr;
  }

  void reset(T* p = nullptr) noexcept {
    if (source != nullptr) {
      deleter(source);
    }
    source = p;
  }

  void swap(unique_ptr& other) noexcept {
    std::swap(source, other.source);
    std::swap(deleter, other.deleter);
  }

  T* get() const noexcept { return source; }
  Deleter& get_deleter() noexcept { return deleter; }
  const Deleter& get_deleter() const noexcept { return deleter; }

  explicit operator bool() const noexcept { return source != nullptr; }

  T& operator*() const {
    assert(source != nullptr);
    return *source;
  }

  T* operator->() const {
    assert(source != nullptr);
    return source;
  }
};

template <typename T, typename... Args>
unique_ptr<T> make_unique(Args&&... args) {
  return unique_ptr<T>(new T(std::forward<Args>(args)...));
}
