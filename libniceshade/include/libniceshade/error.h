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

#include <string>
#include <sstream>
#include <utility>
#include <assert.h>

/**
 * @file
 * @brief
 */

namespace niceshade {

/**
 * Base class for the \ref value_or_error wrapper.
 */
class error {
public:
  error() = default;

  /**
   * Convenience ctor used to create formatted error messages.
   */
  template<class... Args> explicit error(Args&&... args) noexcept {
    std::ostringstream stream;
    ((stream << args), ...);
    stream << "\n";
    msg_ = std::move(stream.str());
  }

public:

  /**
   * @return true if the wrapped value is an error-value.
   */
  bool               is_error() const noexcept { return !msg_.empty(); }

  /**
   * @return the error message, if the wrapped value is an error-value; empty string otherwise.
   */
  const std::string& error_message() const noexcept { return msg_; }

private:
  std::string msg_;
};

/**
 * A wrapper that contains either a value of a type ValueT, or an error-value.
 */
template<class ValueT> class value_or_error : public error {
public:
  value_or_error(ValueT&& val) : val_ {std::move(val)} {}
  value_or_error(error&& err) : error {std::move(err)} {}
  value_or_error(value_or_error&) = delete;
  value_or_error& operator=(value_or_error&) = delete;

  /**
   * @return the wrapped value, if the wrapper contains a ValueT. If it contains an error-value, this method exits the application.
   */
  ValueT&       get() { return val_; }

  /**
   * @return the wrapped value, if the wrapper contains a ValueT. If it contains an error-value, this method exits the application.
   */
  const ValueT& get() const { return val_; }

private:
  ValueT val_ {};
};


}  // namespace niceshade
