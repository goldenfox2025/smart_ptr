#pragma once
#include <iostream>

struct ControlBlock {
  int shared_count;
  int weak_count;
  ControlBlock() : shared_count(1), weak_count(0) {}
};

template <typename T>
class WeakPtr;

template <typename T>
class SharedPtr {
 private:
  T *ptr_;
  ControlBlock *ctrl_;
  template <typename U>
  friend class WeakPtr;
  void release() {
    if (ctrl_) {
      --ctrl_->shared_count;
      if (ctrl_->shared_count == 0) {
        delete ptr_;
        if (ctrl_->weak_count == 0) {
          delete ctrl_;
        }
      }
    }
  }

 public:
  operator bool() const { return ptr_ != nullptr; }

  explicit SharedPtr(T *ptr = nullptr)
      : ptr_(ptr), ctrl_(ptr ? new ControlBlock() : nullptr) {};
  explicit SharedPtr(const WeakPtr<T> &wp) {
    ctrl_ = wp.ctrl_;
    if (ctrl_ && ctrl_->shared_count > 0) {
      ptr_ = wp.ptr_;
      ++ctrl_->shared_count;
    } else {
      ptr_ = nullptr;
      ctrl_ = nullptr;
    }
  }
  SharedPtr(const SharedPtr &other) : ptr_(other.ptr_), ctrl_(other.ctrl_) {
    if (ctrl_) {
      ++ctrl_->shared_count;
    }
  }
  SharedPtr &operator=(const SharedPtr &other) {
    if (this != &other) {
      release();
      ptr_ = other.ptr_;
      ctrl_ = other.ctrl_;
      if (ctrl_) {
        ++ctrl_->shared_count;
      }
    }
    return *this;
  }
  ~SharedPtr() { release(); }
  T *operator->() const { return ptr_; }
  T &operator*() const { return *ptr_; }
  int use_count() const { return ctrl_ ? ctrl_->shared_count : 0; }
  T *get() const { return ptr_; }
};

struct ControlBlock;
template <typename T>
class WeakPtr {
 private:
  template <typename U>
  friend class SharedPtr;
  T *ptr_;
  ControlBlock *ctrl_;
  void release() {
    if (ctrl_) {
      --ctrl_->weak_count;

      if (ctrl_->shared_count == 0 && ctrl_->weak_count == 0) {
        delete ctrl_;
      }
    }
    ctrl_ = nullptr;
    ptr_ = nullptr;
  }

 public:
  SharedPtr<T> lock() const {
    if (ctrl_ && ctrl_->shared_count > 0) {
      return SharedPtr<T>(*this);
    }
    return SharedPtr<T>();
  }
  operator bool() const { return ptr_ != nullptr; }
  WeakPtr() : ptr_(nullptr), ctrl_(nullptr) {}

  WeakPtr(const SharedPtr<T> &sp) : ptr_(sp.ptr_), ctrl_(sp.ctrl_) {
    if (ctrl_) {
      ++ctrl_->weak_count;
    }
  }
  WeakPtr(const WeakPtr &other) : ptr_(other.ptr_), ctrl_(other.ctrl_) {
    if (ctrl_) {
      ++ctrl_->weak_count;
    }
  }
  WeakPtr &operator=(const WeakPtr &other) {
    if (this != &other) {
      release();
      ptr_ = other.ptr_;
      ctrl_ = other.ctrl_;
      if (ctrl_) {
        ++ctrl_->weak_count;
      }
    }
    return *this;
  }
  ~WeakPtr() { release(); }
};
