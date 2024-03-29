/**
 * Copyright (c) 2022 nicegraf contributors
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

#include <stddef.h>

namespace niceshade {

/// Replacement for std::span to avoid requiring C++20.
template<class T> class span {
public:
  using iterator       = T*;
  using const_iterator = const T*;
  span() = default;
  span(T* ptr, size_t size) : ptr_(ptr), size_(size) {}

  size_t         size() const { return size_; }
  iterator       begin() { return ptr_; }
  const_iterator begin() const { return ptr_; }
  const_iterator cbegin() const { return ptr_; }
  iterator       end() { return ptr_ + size_; }
  const_iterator end() const { return ptr_ + size_; }
  const_iterator cend() const { return ptr_ + size_; }
  T&             operator[](size_t idx) { return ptr_[idx]; }
  const T&       operator[](size_t idx) const { return ptr_[idx]; }

private:
  T*     ptr_ = nullptr;
  size_t size_ = 0u;
};

/// Constant \ref span
template<class T> using const_span = span<const T>;

}  // namespace niceshade