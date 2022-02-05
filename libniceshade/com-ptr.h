/**
 * Copyright (c) 2021 nicegraf contributors
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to
 * deal in the Software without restriction, including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 */

#pragma once

#define _CRT_SECURE_NO_WARNINGS

#include "libniceshade/platform.h"

#include <type_traits>

namespace libniceshade {

/* COM pointer wrapper. */
template<class T> class com_ptr {
  public:
  com_ptr() = default;

  explicit com_ptr(T* ptr) : ptr_(ptr) {
    static_assert(std::is_base_of<IUnknown, T>::value);
  }

  template<class F> explicit com_ptr(F create_fn) {
    static_assert(std::is_invocable_r<HRESULT, F, T**>::value);
    const HRESULT create_result = create_fn(&ptr_);
    if (create_result != S_OK) ptr_ = nullptr;
  }

  com_ptr(com_ptr&& other) {
    *this = std::move(other);
  }

  ~com_ptr() {
    release();
  }

  com_ptr(const com_ptr&) = delete;
  com_ptr& operator=(const com_ptr&) = delete;

  com_ptr& operator=(com_ptr&& other) noexcept {
    release();
    ptr_       = other.ptr_;
    other.ptr_ = nullptr;
    return *this;
  }

  T* operator->() {
    return ptr_;
  }

  const T* operator->() const {
    return ptr_;
  }

  T* get() {
    return ptr_;
  }

  const T* get() const {
    return ptr_;
  }

  T** PtrToContent() {
    return &ptr_;
  }

  private:
  void release() {
    if (ptr_) { ptr_->Release(); }
  }

  T* ptr_ = nullptr;
};

}  // namespace libniceshade
