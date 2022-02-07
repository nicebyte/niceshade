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

#include <string>
#include <utility>
#include <assert.h>

namespace libniceshade {

class error {
public:
  error() = default;
  explicit error(const char* msg) : msg_ {msg} {}

  bool               is_error() const { return !msg_.empty(); }
  const std::string& error_message() const { return msg_; }

private:
  std::string msg_;
};

template<class ValueT> class value_or_error : public error {
public:
  value_or_error(ValueT&& val) : val_ {std::forward<ValueT>(val)} {}
  value_or_error(error&& err) : error {std::move(err)} {}

  ValueT&       get() { return val_; }
  const ValueT& get() const { return val_; }

private:
  ValueT val_ {};
};

#define NICESHADE_RETURN_IF_ERROR(x) \
  {                                  \
    if (x.is_error()) return x;      \
  }

}  // namespace libniceshade
